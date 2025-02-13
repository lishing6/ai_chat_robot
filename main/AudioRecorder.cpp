#include "AudioRecorder.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "i2sconfig.h"

const char* AudioRecorder::TAG = "AudioRecorder";

/**
 * @brief AudioRecorder 构造函数
 *
 * 目前无特殊初始化，初始化工作在 init() 中完成。
 */
AudioRecorder::AudioRecorder() {
    // 可在此添加构造函数的初始化代码
}

/**
 * @brief AudioRecorder 析构函数
 *
 * 卸载 I2S 驱动，释放资源。
 */
AudioRecorder::~AudioRecorder() {
    // 卸载 I2S 驱动，确保释放内存资源
    i2s_driver_uninstall(I2S_PORT);
}

/**
 * @brief 初始化 I2S 录音接口
 *
 * 配置 I2S 作为主模式和接收模式，设置采样率、位深、DMA 缓冲区及引脚配置。
 * 
 * @return esp_err_t 返回 ESP_OK 表示初始化成功，否则返回错误码。
 */
esp_err_t AudioRecorder::init() {
    // 配置 I2S 参数
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = 1
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    // 配置 I2S 引脚（BCLK、LRCLK、数据输入）
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_IN
    };

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2S input initialized successfully.");
    return ESP_OK;
}

/**
 * @brief 从 I2S 接口录制音频数据
 *
 * 该方法使用 i2s_read() 从 I2S 录音接口读取数据，并将实际读取的字节数存入 size 中。
 *
 * @param data 录音数据存储缓冲区，由调用者分配足够空间（大小应不小于 BUFFER_SIZE）。
 * @param size 输出参数，保存实际读取的字节数。
 * @return esp_err_t 返回 ESP_OK 表示读取成功，否则返回错误码。
 */
esp_err_t AudioRecorder::record(uint8_t *data, size_t *size) {
    size_t bytes_read = 0;
    ESP_LOGI(TAG, "Start recording...");
    esp_err_t err = i2s_read(I2S_PORT, data, BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_read failed: %s", esp_err_to_name(err));
        return err;
    }
    *size = bytes_read;
    ESP_LOGI(TAG, "i2s read bytes: %d", bytes_read);
    return ESP_OK;
}
