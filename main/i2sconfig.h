#pragma once 

#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define I2S_CHANNEL_NUM   (1)


#define SAMPLE_COUNT    (I2S_SAMPLE_RATE * 5) // 采集5秒的音频
#define BUFFER_SIZE     (SAMPLE_COUNT * (I2S_SAMPLE_BITS / 8))



