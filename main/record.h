#pragma once

#include <driver/i2s.h>
#include "esp_log.h"
#include "i2sconfig.h"


#define I2S_WS 15        // LRCLK
#define I2S_SD_IN 13     // 麦克风输入数据
#define I2S_SCK 2        // BCLK
#define I2S_PORT I2S_NUM_0




void i2sInitInput();
void record_audio(uint8_t *data, size_t *size);