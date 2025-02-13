#include "AudioPlayer.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "i2sconfig.h"

const char* AudioPlayer::TAG = "AudioPlayer";

AudioPlayer::AudioPlayer() {
    // 构造函数：目前不需要额外的初始化工作，
    // 后续初始化将在 init() 方法中完成
}

AudioPlayer::~AudioPlayer() {
    // 析构函数：卸载 I2S 驱动以释放资源
    i2s_driver_uninstall(I2S_PORT_OUT);
}

/**
 * @brief 初始化 I2S 输出接口
 * 
 * 配置 I2S 为主模式和发送模式（TX），设置采样率、位深、通道格式、DMA 缓冲区以及时钟。
 * 同时，设置输出引脚（BCLK、LRCLK、数据输出）。
 *
 * @return esp_err_t 返回 ESP_OK 表示成功，否则返回错误码
 */
esp_err_t AudioPlayer::init() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = 1
    };

    esp_err_t err = i2s_driver_install(I2S_PORT_OUT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "i2s_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_OUT,
        .ws_io_num = I2S_WS_OUT,
        .data_out_num = I2S_SD_OUT,  // 连接到扬声器（例如 MAX98357 的 DIN 引脚）
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    err = i2s_set_pin(I2S_PORT_OUT, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_set_pin failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2S output initialized successfully.");
    return ESP_OK;
}

/**
 * @brief 播放音频数据
 * 
 * 通过 I2S 写接口将传入的音频数据输出到扬声器，
 * 并记录播放开始和结束的日志。
 *
 * @param audioData 指向音频数据的缓冲区
 * @param size 数据的字节数
 * @return esp_err_t 返回 ESP_OK 表示播放成功，否则返回错误码
 */
esp_err_t AudioPlayer::play(uint8_t* audioData, int size) {
    size_t bytes_written = 0;
    ESP_LOGI(TAG, "Starting playback...");
    esp_err_t err = i2s_write(I2S_PORT_OUT, audioData, size, &bytes_written, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_write failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Playback finished. Bytes written: %d", bytes_written);
    return ESP_OK;
}
