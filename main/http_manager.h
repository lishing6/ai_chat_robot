#pragma once

#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>

//获取access/token时的client_id和client_secret，请按实际情况修改
#define CLIENT_ID ""
#define CLIENT_SECRET ""

//SPARK_API_URL
#define SPARK_API_URL ""

/**
 * @brief HttpManager 类封装了与服务器通信的功能，
 * 包括获取 access token、发送 Spark 请求、发送音频到百度 ASR、以及发送文本到百度 TTS。
 *
 * 采用面向对象设计，同时使用 cJSON 库进行 JSON 构造和解析，
 * 使用 mbedtls 的 Base64 编码函数和自定义 URL 编码函数。
 */
class HttpManager {
public:
    HttpManager();
    ~HttpManager();

    //播放 TTS 合成的音频数据
    char* global_response_buffer;
    int global_response_len;

    /**
     * @brief 请求百度的 OAuth 服务获取 access token，并解析保存到成员变量。
     * @return true 成功；false 失败。
     */
    bool fetchAccessToken();

    /**
     * @brief 向 Spark Max 发送请求，发送文本并解析返回的答案（存储到 answerSpark 中）。
     * @param query 要发送的查询文本。
     * @return true 成功；false 失败。
     */
    bool sendSparkRequest(const std::string &query);

    /**
     * @brief 将录音的音频数据（PCM 格式）发送给百度语音识别接口，
     * 并解析返回的识别文本存储到 resultSpeechToText 中。
     * @param audioData 指向录音数据的缓冲区（数据将由调用方释放）。
     * @param audioLen 音频数据长度（字节数）。
     * @param cuid 用户唯一标识。
     * @return true 成功；false 失败。
     */
    bool sendAudioToBaiduASR(uint8_t* audioData, size_t audioLen, const std::string &cuid);

    /**
     * @brief 将文本发送到百度语音合成接口，将文本合成为语音。
     * @param text 待合成的文本。
     * @param spd 语速（0～15）。
     * @param pit 音调（0～15）。
     * @param vol 音量（0～15）。
     * @param per 发音人选择。
     * @param aue 音频格式：3: mp3; 4: pcm-16k; 5: pcm-8k; 6: wav。
     * @return true 成功；false 失败。
     */
    bool sendTextToBaiduTTS(const std::string &text, int spd, int pit, int vol, int per, int aue);

    // Getter 方法
    std::string getAccessToken() const { return accessToken; }
    std::string getResultSpeechToText() const { return resultSpeechToText; }
    std::string getAnswerSpark() const { return answerSpark; }

    /**
     * @brief URL 编码工具函数（单次编码）。
     * @param value 输入字符串。
     * @return 编码后的字符串。
     */
    static std::string urlEncode(const std::string &value);

private:
    // 保存从百度 OAuth 获取到的 access token
    std::string accessToken;
    // 保存百度 ASR 返回的识别文本
    std::string resultSpeechToText;
    // 保存 Spark 请求返回的回答
    std::string answerSpark;

    /**
     * @brief 内部辅助函数：执行 HTTP 请求。
     * @param config HTTP 客户端配置结构体。
     * @return esp_err_t 返回操作结果。
     */
    esp_err_t performRequest(const esp_http_client_config_t &config);

    // 以下为静态 HTTP 事件处理函数，分别处理不同接口的响应

    /**
     * @brief 处理百度 OAuth 请求的 HTTP 事件，解析 access_token。
     */
    static esp_err_t eventHandlerBaiduToken(esp_http_client_event_t *evt);

    /**
     * @brief 处理 Spark 请求的 HTTP 事件，解析返回的答案。
     */
    static esp_err_t eventHandlerSpark(esp_http_client_event_t *evt);

    /**
     * @brief 处理百度语音识别请求的 HTTP 事件，解析返回的识别结果。
     */
    static esp_err_t eventHandlerASR(esp_http_client_event_t *evt);

    /**
     * @brief 处理百度语音合成请求的 HTTP 事件，根据 Content-Type 判断返回是音频数据还是 JSON 错误信息。
     */
    static esp_err_t eventHandlerTTS(esp_http_client_event_t *evt);

    // 为简化代码，使用静态变量存放 HTTP 响应数据缓冲区（各事件函数共享）
    static char *s_responseBuffer;
    static int s_responseLen;

    

    // 日志标签
    static const char* TAG;
};

