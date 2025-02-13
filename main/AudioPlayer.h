#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <driver/i2s.h>
#include "esp_log.h"
#include "i2sconfig.h"

/**
 * @brief AudioPlayer 类封装了 I2S 输出（播放）功能
 *
 * 该类负责初始化 I2S 输出接口并播放音频数据，
 * 适用于通过 I2S 将音频数据输出给扬声器（如 MAX98357）。
 */
class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    /**
     * @brief 初始化 I2S 输出接口
     * @return esp_err_t 返回 ESP_OK 表示初始化成功，否则返回错误码。
     */
    esp_err_t init();

    /**
     * @brief 播放音频数据
     * @param audioData 指向音频数据的指针
     * @param size 数据的字节数
     * @return esp_err_t 返回 ESP_OK 表示播放成功，否则返回错误码。
     */
    esp_err_t play(uint8_t* audioData, int size);

private:
    static const char* TAG; // 日志标签
};

#endif // AUDIO_PLAYER_H
