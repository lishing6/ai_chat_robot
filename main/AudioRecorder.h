#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include "driver/i2s.h"
#include "esp_log.h"
#include "i2sconfig.h"  // 包含 I2S_SCK, I2S_WS, I2S_SD_IN, I2S_PORT, BUFFER_SIZE 等宏定义

/**
 * @brief AudioRecorder 类封装了 I2S 录音功能
 *
 * 该类用于初始化 I2S 接口用于录音，并通过 record() 方法从 I2S 读取音频数据。
 */
class AudioRecorder {
public:
    AudioRecorder();
    ~AudioRecorder();

    /**
     * @brief 初始化 I2S 录音接口
     * @return esp_err_t 操作结果
     */
    esp_err_t init();

    /**
     * @brief 从 I2S 录音接口读取数据到指定缓冲区
     * @param data 指向录音数据缓冲区
     * @param size 输出参数，记录实际读取的字节数
     * @return esp_err_t 操作结果
     */
    esp_err_t record(uint8_t *data, size_t *size);

private:
    static const char *TAG; // 日志标签
};

#endif // AUDIO_RECORDER_H
