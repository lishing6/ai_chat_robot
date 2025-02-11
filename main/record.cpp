#include "record.h"



static const char *TAG = "AudioRecord";


// 初始化 I2S 输入
void i2sInitInput() {
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

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_IN  // 输入据引脚设置
        
    };

    i2s_set_pin(I2S_PORT, &pin_config);
}

void record_audio(uint8_t *data, size_t *size) {
    size_t bytes_read;
    ESP_LOGI(TAG,"Start recording...");
    esp_err_t err = i2s_read(I2S_PORT, data, BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(err));
        return;
    }
    *size = bytes_read;
    ESP_LOGI(TAG,"i2s read bytes:%d",*size);

}



