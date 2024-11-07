#pragma once

#include <driver/i2s.h>
#include "esp_log.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "i2sconfig.h"

#define I2S_WS 15        // 共用LRCLK
#define I2S_SD_IN 13     // 麦克风输入数据
#define I2S_SCK 2        // 共用BCLK
#define I2S_PORT I2S_NUM_0


void i2s_adc();
void i2sInitInput();
static void wavHeader(uint8_t* header, int wavSize);