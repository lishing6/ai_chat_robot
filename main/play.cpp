#include "play.h"

static const char *TAG = "AudioPlay";
extern int flash_wr_size;

// 从录音缓冲区播放音频
void i2s_play(uint8_t* wav_buffer, int wav_size) {
    size_t bytes_written;

    ESP_LOGI(TAG, "Starting playback...");

    // 播放 WAV 数据
    i2s_write(I2S_PORT_OUT, wav_buffer + WAV_HEADER_SIZE, wav_size - WAV_HEADER_SIZE, &bytes_written, portMAX_DELAY);
    ESP_LOGI(TAG, "Playback finished.");

    free(wav_buffer);

    vTaskDelay(3500 / portTICK_PERIOD_MS);
    i2s_driver_uninstall(I2S_PORT_OUT);
    free(wav_buffer);

}

// 初始化 I2S 输出
void i2sInitOutput() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 64,
        .dma_buf_len = 1024,
        .use_apll = 1
    };

    i2s_driver_install(I2S_PORT_OUT, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_OUT,
        .ws_io_num = I2S_WS_OUT,
        .data_out_num = I2S_SD_OUT,  // 输出数据引脚设置
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_set_pin(I2S_PORT_OUT, &pin_config);
}