#include "play.h"
#include "record.h"
#include "wifi_connect.h"
#include "http_client.h"
#include "esp_heap_caps.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <esp_task_wdt.h>

void check_psram();
void init_spiffs();

extern "C"
void app_main() {
    size_t audio_read_bytes;
    uint8_t* audio_data = reinterpret_cast<uint8_t*>(heap_caps_calloc(BUFFER_SIZE, sizeof(uint8_t), MALLOC_CAP_SPIRAM));



    esp_task_wdt_deinit();//disable TWDT for debug
    wifi_connect();//must running before init_spiffs()
    check_psram();
    i2sInitInput();
    i2sInitOutput();

    //response_mutex = xSemaphoreCreateMutex();

    fetch_and_parse_access_token();

    send_spark_request("你好，请你接下来回答我的问题时，字数控制在30以内，谢谢。");
    
    record_audio(audio_data,&audio_read_bytes);
    
    send_audio_to_baidu_asr_array(audio_data, audio_read_bytes, "lishing");

    send_spark_request(result_speech_to_text.c_str());

    send_text_to_baidu_tts(answer_spark, 5, 5, 5, 4, 4);

    audio_play(reinterpret_cast<uint8_t*>(global_response_buffer), global_response_len);

    free(global_response_buffer);

}


void check_psram() {
    if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0) {
        ESP_LOGE("PSRAM", "PSRAM not available!");
        abort();
    } else {
        ESP_LOGI("PSRAM", "PSRAM is enabled. Total PSRAM: %d bytes",
                 heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    }
}

void init_spiffs(){
    static const char *TAG = "SPIFFS_Example";

    // 配置 SPIFFS 参数
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",        // 文件系统挂载路径
        .partition_label = NULL,      // 使用默认分区
        .max_files = 5,               // 同时打开文件的最大数量
        .format_if_mount_failed = true // 如果挂载失败则格式化
    };
     // 初始化 SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    // 获取 SPIFFS 分区信息
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS partition size: total: %d, used: %d", total, used);
    }
}






