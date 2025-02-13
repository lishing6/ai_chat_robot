#include "wifi_manager.h"
#include "http_manager.h"
#include "AudioRecorder.h"
#include "AudioPlayer.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern "C" {
    #include "i2sconfig.h"
}

static const char* TAG = "Main";

// 全局信号量，用于通知录音任务完成
SemaphoreHandle_t recordSemaphore = NULL;

// 全局变量，用于保存录音数据及其长度（供后续任务使用）
uint8_t* g_audioBuffer = nullptr;
volatile size_t g_audioReadBytes = 0;

// 模块对象实例
WiFiManager wifiMgr;
HttpManager httpMgr;
AudioRecorder recorder;
AudioPlayer player;

/**
 * @brief 检查 PSRAM 是否可用
 */
void check_psram() {
    size_t psramTotal = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    if (psramTotal == 0) {
        ESP_LOGE("PSRAM", "PSRAM not available!");
        abort();
    } else {
        ESP_LOGI("PSRAM", "PSRAM is enabled. Total PSRAM: %d bytes", psramTotal);
    }
}

/**
 * @brief 初始化 SPIFFS 文件系统
 */
void init_spiffs(){
    static const char *TAG_SPIFFS = "SPIFFS_Example";
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SPIFFS, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG_SPIFFS, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG_SPIFFS, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SPIFFS, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_SPIFFS, "SPIFFS partition size: total: %d, used: %d", total, used);
    }
}

/**
 * @brief 录音任务
 *
 * 初始化录音模块后，从 I2S 录音接口读取数据，
 * 并通过信号量通知后续处理任务。
 */
void RecordTask(void* pvParameters) {
    ESP_LOGI(TAG, "RecordTask started");
    if (recorder.init() != ESP_OK) {
        ESP_LOGE(TAG, "Audio recorder initialization failed");
        vTaskDelete(NULL);
    }
    // 在 PSRAM 上分配录音缓冲区
    g_audioBuffer = reinterpret_cast<uint8_t*>(heap_caps_calloc(BUFFER_SIZE, sizeof(uint8_t), MALLOC_CAP_SPIRAM));
    if (!g_audioBuffer) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        vTaskDelete(NULL);
    }
    size_t bytesRead = 0;
    if (recorder.record(g_audioBuffer, &bytesRead) != ESP_OK) {
        ESP_LOGE(TAG, "Recording failed");
        heap_caps_free(g_audioBuffer);
        g_audioBuffer = nullptr;
        vTaskDelete(NULL);
    }
    g_audioReadBytes = bytesRead;
    ESP_LOGI(TAG, "Recorded %d bytes", bytesRead);
    // 录音完成后释放信号量，通知 ProcessTask
    xSemaphoreGive(recordSemaphore);
    vTaskDelete(NULL);
}

/**
 * @brief 处理任务
 *
 * 等待录音任务完成后，依次调用 HttpManager 处理 ASR、Spark、TTS 等请求，
 * 最后使用 AudioPlayer 播放 TTS 合成的音频数据。
 */
void ProcessTask(void* pvParameters) {
    ESP_LOGI(TAG, "ProcessTask started");
    // 等待录音完成信号
    if (xSemaphoreTake(recordSemaphore, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Recording complete, processing audio...");
        // 发送录音数据至百度 ASR，获取识别文本
        if (!httpMgr.sendAudioToBaiduASR(g_audioBuffer, g_audioReadBytes, "lishing")) {
            ESP_LOGE(TAG, "ASR processing failed");
        }
        // 使用 ASR 返回的文本作为 Spark 请求的输入，获取 Spark 答案
        std::string asrResult = httpMgr.getResultSpeechToText();
        if (!httpMgr.sendSparkRequest(asrResult)) {
            ESP_LOGE(TAG, "Failed to send Spark request based on ASR result");
        }
        // 使用 Spark 返回的答案文本作为 TTS 的输入，将文本合成为语音数据
        if (!httpMgr.sendTextToBaiduTTS(httpMgr.getAnswerSpark(), 5, 5, 5, 4, 4)) {
            ESP_LOGE(TAG, "TTS synthesis failed");
        }
        // 初始化音频播放模块
        if (player.init() != ESP_OK) {
            ESP_LOGE(TAG, "Audio player initialization failed");
        }
        // 播放 TTS 合成的音频数据（假定 TTS 音频数据存储在全局变量 global_response_buffer 中）

        if (httpMgr.global_response_buffer && httpMgr.global_response_len > 0) {
            if (player.play(reinterpret_cast<uint8_t*>(httpMgr.global_response_buffer), httpMgr.global_response_len) != ESP_OK) {
                ESP_LOGE(TAG, "Audio playback failed");
            }
            free(httpMgr.global_response_buffer);
            httpMgr.global_response_buffer = nullptr;
            httpMgr.global_response_len = 0;
        } else {
            ESP_LOGE(TAG, "No TTS audio data available for playback");
        }
    } else {
        ESP_LOGE(TAG, "Failed to take recordSemaphore");
    }
    vTaskDelete(NULL);
}

/**
 * @brief 主入口函数
 *
 * 初始化各模块，创建并启动 FreeRTOS 任务以并行处理录音与后续数据处理任务。
 */
extern "C" void app_main() {
    // 初始化 NVS 并启动 Wi-Fi 连接
    wifiMgr.connect();
    check_psram();
    //init_spiffs(); // 如需文件系统可启用

    // 创建录音完成信号量
    recordSemaphore = xSemaphoreCreateBinary();
    if (recordSemaphore == NULL) {
        ESP_LOGE(TAG, "Failed to create recordSemaphore");
        return;
    }

    // 创建录音任务
    xTaskCreate(RecordTask, "RecordTask", 4096, NULL, 5, NULL);
    // 创建处理任务
    xTaskCreate(ProcessTask, "ProcessTask", 8192, NULL, 5, NULL);

    // 主任务可以继续执行其他工作或等待子任务完成
}
