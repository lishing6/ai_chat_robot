#include <driver/i2s.h>
#include "esp_log.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "esp_task_wdt.h"
#include "play.h"


#define I2S_WS 15
#define I2S_SD 13
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (3) // Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
#define WAV_HEADER_SIZE 44

static const char *TAG = "AudioRecord";

void wavHeader(uint8_t* header, int wavSize);
void i2sInit();

void i2s_adc(/*void *arg*/) {
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;
    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* wav_buffer = (uint8_t*) calloc(FLASH_RECORD_SIZE + WAV_HEADER_SIZE, sizeof(uint8_t)); // WAV buffer with header

    if (!i2s_read_buff || !wav_buffer) {
        ESP_LOGE(TAG, "Memory allocation failed!");
        free(i2s_read_buff);
        free(wav_buffer);
        vTaskDelete(NULL);
        return;
    }

    // Prepare WAV header and insert it into the buffer
    wavHeader(wav_buffer, FLASH_RECORD_SIZE);

    ESP_LOGI(TAG, "Recording Start");

    // Record audio data and store it in the WAV buffer after the header
    while (flash_wr_size + i2s_read_len < FLASH_RECORD_SIZE ) {
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        memcpy(wav_buffer + WAV_HEADER_SIZE + flash_wr_size, i2s_read_buff, bytes_read);
        flash_wr_size += bytes_read;
        ESP_LOGI(TAG, "Sound recording %u%%", flash_wr_size * 100 / FLASH_RECORD_SIZE);

        // 添加一个短暂延时，让出 CPU
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Output WAV file content through serial in hex format
    // ESP_LOGI(TAG, "Recording completed. Outputting WAV data...");
    // for (int i = 0; i < flash_wr_size; i++) {
       
    //     printf("0x%02X,", wav_buffer[i]);
    //     if ((i + 1) % 16 == 0) { // New line every 16 bytes for readability           
    //     }
    // }

     i2s_driver_uninstall(I2S_PORT);

        i2sInitOutput();
    i2s_play(wav_buffer, FLASH_RECORD_SIZE + WAV_HEADER_SIZE);

    free(i2s_read_buff);
    free(wav_buffer);
    //vTaskDelete(NULL);
}

void wavHeader(uint8_t* header, int wavSize) {
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSize = wavSize + WAV_HEADER_SIZE - 8;
    header[4] = (uint8_t)(fileSize & 0xFF);
    header[5] = (uint8_t)((fileSize >> 8) & 0xFF);
    header[6] = (uint8_t)((fileSize >> 16) & 0xFF);
    header[7] = (uint8_t)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10;
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01;
    header[21] = 0x00;
    header[22] = 0x01;
    header[23] = 0x00;
    header[24] = (uint8_t)(I2S_SAMPLE_RATE & 0xFF);
    header[25] = (uint8_t)((I2S_SAMPLE_RATE >> 8) & 0xFF);
    header[26] = (uint8_t)((I2S_SAMPLE_RATE >> 16) & 0xFF);
    header[27] = (uint8_t)((I2S_SAMPLE_RATE >> 24) & 0xFF);
    unsigned int byteRate = I2S_SAMPLE_RATE * I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8;
    header[28] = (uint8_t)(byteRate & 0xFF);
    header[29] = (uint8_t)((byteRate >> 8) & 0xFF);
    header[30] = (uint8_t)((byteRate >> 16) & 0xFF);
    header[31] = (uint8_t)((byteRate >> 24) & 0xFF);
    header[32] = I2S_CHANNEL_NUM * I2S_SAMPLE_BITS / 8;
    header[33] = 0x00;
    header[34] = I2S_SAMPLE_BITS;
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (uint8_t)(wavSize & 0xFF);
    header[41] = (uint8_t)((wavSize >> 8) & 0xFF);
    header[42] = (uint8_t)((wavSize >> 16) & 0xFF);
    header[43] = (uint8_t)((wavSize >> 24) & 0xFF);
}

void i2sInit() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)I2S_SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 64,
        .dma_buf_len = 1024,
        .use_apll = 1
    };

    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    i2s_set_pin(I2S_PORT, &pin_config);
}

extern "C"
void app_main() {
   esp_task_wdt_deinit(); // 禁用任务看门狗

    i2sInit();
    //xTaskCreate(i2s_adc, "i2s_adc", 8 * 1024, NULL, 1, NULL);
    i2s_adc();


}
