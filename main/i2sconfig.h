#ifndef I2SCONFIG_H
#define I2SCONFIG_H

// 录音 I2S 配置（用于 AudioRecorder）
#define I2S_WS        15    // 录音时的 LRCLK 引脚（Word Select）
#define I2S_SD_IN     13    // 录音时的麦克风数据输入引脚
#define I2S_SCK       2     // 录音时的 BCLK 引脚（Bit Clock）
#define I2S_PORT      I2S_NUM_0  // 录音使用的 I2S 端口

// 播放 I2S 配置（用于 AudioPlayer）
#define I2S_WS_OUT    16    // 播放时的 LRCLK 引脚（Word Select）
#define I2S_SD_OUT    12    // 播放时的扬声器数据输出引脚（连接到 MAX98357 DIN）
#define I2S_SCK_OUT   1     // 播放时的 BCLK 引脚（Bit Clock）
#define I2S_PORT_OUT  I2S_NUM_1  // 播放使用的 I2S 端口

// 音频参数配置
#define I2S_SAMPLE_RATE   16000  // 采样率 16kHz
#define I2S_SAMPLE_BITS   16     // 采样位数 16位

// 录音缓冲区大小（字节数），根据需要调整
#define BUFFER_SIZE       160000

#endif // I2SCONFIG_H
