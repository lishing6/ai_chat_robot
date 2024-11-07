#pragma once

#include <driver/i2s.h>
#include "esp_log.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "i2sconfig.h"

#define I2S_WS_OUT 16        // LRCLK
#define I2S_SD_OUT 12    // 扬声器输出数据（请连接 MAX98357 DIN 引脚）
#define I2S_SCK_OUT 1        // BCLK
#define I2S_PORT_OUT I2S_NUM_1




void i2sInitOutput();
void i2s_play(uint8_t* wav_buffer, int wav_size);

