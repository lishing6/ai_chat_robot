#include "http_manager.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "esp_heap_caps.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

// 定义静态成员变量
const char* HttpManager::TAG = "HttpManager";
char* HttpManager::s_responseBuffer = nullptr;
int HttpManager::s_responseLen = 0;


HttpManager::HttpManager() {
    accessToken = "";
    resultSpeechToText = "";
    answerSpark = "";
}

HttpManager::~HttpManager() {
    // 确保静态缓冲区被释放
    if (s_responseBuffer) {
        free(s_responseBuffer);
        s_responseBuffer = nullptr;
        s_responseLen = 0;
    }
}

std::string HttpManager::urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0'); // 格式化数字时使用 '0' 填充
    escaped << std::hex; // 设置为十六进制输出

    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

/**
 * @brief 内部辅助函数：根据传入的配置执行 HTTP 请求。
 *
 * 此函数使用 ESP HTTP Client 执行请求，事件处理函数会将返回数据存入静态缓冲区 s_responseBuffer。
 */
esp_err_t HttpManager::performRequest(const esp_http_client_config_t &config) {
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    // 如果是 POST 请求且需要发送数据，事件处理函数中会负责接收响应数据
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return err;
}

bool HttpManager::fetchAccessToken() {
    char url[512];
    snprintf(url, sizeof(url),
             "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s",
              CLIENT_ID, CLIENT_SECRET);

    // 清空静态缓冲区
    if (s_responseBuffer) {
        free(s_responseBuffer);
        s_responseBuffer = nullptr;
        s_responseLen = 0;
    }

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.event_handler = eventHandlerBaiduToken;
    config.skip_cert_common_name_check = true;

    esp_err_t err = performRequest(config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status for token = %d", 200); // 状态码已通过事件处理
        if (s_responseBuffer != nullptr) {
            cJSON *json = cJSON_Parse(s_responseBuffer);
            if (json) {
                cJSON *token = cJSON_GetObjectItem(json, "access_token");
                if (token && cJSON_IsString(token)) {
                    accessToken = token->valuestring;
                    ESP_LOGI(TAG, "Access Token: %s", accessToken.c_str());
                } else {
                    ESP_LOGE(TAG, "Failed to find access_token in JSON response");
                    cJSON_Delete(json);
                    free(s_responseBuffer);
                    s_responseBuffer = nullptr;
                    s_responseLen = 0;
                    return false;
                }
                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                free(s_responseBuffer);
                s_responseBuffer = nullptr;
                s_responseLen = 0;
                return false;
            }
            free(s_responseBuffer);
            s_responseBuffer = nullptr;
            s_responseLen = 0;
            return true;
        }
    }
    return false;
}

bool HttpManager::sendSparkRequest(const std::string &query) {
    // 清空静态响应缓冲区
    if (s_responseBuffer) {
        free(s_responseBuffer);
        s_responseBuffer = nullptr;
        s_responseLen = 0;
    }

    // 构造 JSON 请求
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object for Spark request");
        return false;
    }
    cJSON_AddStringToObject(root, "model", "generalv3.5");
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");
    if (!messages) {
        ESP_LOGE(TAG, "Failed to create messages array");
        cJSON_Delete(root);
        return false;
    }
    cJSON *message = cJSON_CreateObject();
    if (!message) {
        ESP_LOGE(TAG, "Failed to create message object");
        cJSON_Delete(root);
        return false;
    }
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", query.c_str());
    cJSON_AddItemToArray(messages, message);
    char *postData = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!postData) {
        ESP_LOGE(TAG, "Failed to serialize Spark request JSON");
        return false;
    }

    // 配置 HTTP 客户端
    esp_http_client_config_t config = {};
    config.url = SPARK_API_URL;
    config.method = HTTP_METHOD_POST;
    config.event_handler = eventHandlerSpark;
    config.skip_cert_common_name_check = true;

    esp_err_t err = performRequest(config);
    free(postData);
    
    if (err == ESP_OK) {
                cJSON* json = cJSON_Parse(s_responseBuffer);
                if (json) {
                    cJSON* choices = cJSON_GetObjectItem(json, "choices");
                    if (choices && cJSON_IsArray(choices)) {
                        cJSON* firstChoice = cJSON_GetArrayItem(choices, 0);
                        if (firstChoice) {
                            cJSON* message = cJSON_GetObjectItem(firstChoice, "message");
                            if (message) {
                                cJSON* content = cJSON_GetObjectItem(message, "content");
                                if (content && cJSON_IsString(content)) {
                                    answerSpark = content->valuestring;
                                    ESP_LOGI(TAG, "Spark answer: %s", answerSpark.c_str());
                                }
                            }
                        }
                    } else {
                        ESP_LOGE(TAG, "Failed to find choices in Spark JSON response");
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG, "Failed to parse Spark JSON response");
                }
            
        return true;
    }
    return false;
}

bool HttpManager::sendAudioToBaiduASR(uint8_t* audioData, size_t audioLen, const std::string &cuid) {
    // Base64 编码音频数据
    size_t encodedLen = ((audioLen + 2) / 3) * 4 + 1;
    char *base64Encoded = reinterpret_cast<char*>(heap_caps_calloc(encodedLen, sizeof(char), MALLOC_CAP_SPIRAM));
    if (!base64Encoded) {
        ESP_LOGE(TAG, "Failed to allocate memory for Base64 encoding");
        return false;
    }
    size_t outLen = 0;
    mbedtls_base64_encode((unsigned char*)base64Encoded, encodedLen, &outLen, audioData, audioLen);
    base64Encoded[outLen] = '\0';
    
    // 释放录音数据以释放PSRAM
    heap_caps_free(audioData);

    // 构造 JSON 请求
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON object for ASR");
        heap_caps_free(base64Encoded);
        return false;
    }
    cJSON_AddStringToObject(root, "format", "pcm");
    cJSON_AddNumberToObject(root, "rate", 16000);
    cJSON_AddNumberToObject(root, "channel", 1);
    cJSON_AddStringToObject(root, "token", accessToken.c_str());
    cJSON_AddStringToObject(root, "cuid", "Rwb0oGDKThTRRMQwOmu7XY3c18g5vsh2");
    cJSON_AddNumberToObject(root, "len", audioLen);
    if (!cJSON_AddStringToObject(root, "speech", base64Encoded)) {
         ESP_LOGE(TAG, "Failed to add speech field to JSON for ASR");
    }
    
    char *postData = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    heap_caps_free(base64Encoded);
    if (!postData) {
        ESP_LOGE(TAG, "Failed to serialize ASR JSON");
        return false;
    }
    ESP_LOGI(TAG, "ASR Post Data: %s", postData);

    // 配置 HTTP 客户端
    esp_http_client_config_t config = {};
    config.url = "https://vop.baidu.com/server_api";
    config.method = HTTP_METHOD_POST;
    config.event_handler = eventHandlerASR;
    config.skip_cert_common_name_check = true;

    
    esp_err_t err = performRequest(config);
    free(postData);
    
    if (err == ESP_OK) {
                cJSON* json = cJSON_Parse(s_responseBuffer);
                if (json) {
                    cJSON* result = cJSON_GetObjectItem(json, "result");
                    if (result && cJSON_IsString(result)) {
                        resultSpeechToText = result->valuestring;
                        ESP_LOGI(TAG, "ASR result: %s", resultSpeechToText.c_str());
                    } else {
                        ESP_LOGE(TAG, "Failed to find result in ASR JSON response");
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG, "Failed to parse ASR JSON response");
                }
        return true;
    }
    return false;
}

bool HttpManager::sendTextToBaiduTTS(const std::string &text, int spd, int pit, int vol, int per, int aue) {
    // 双重 URL 编码
    std::string encodedText = urlEncode(urlEncode(text));
    
    std::string postDataStr = "tex=" + encodedText +
                            "&tok=" + accessToken +
                            "&cuid=" + "Rwb0oGDKThTRRMQwOmu7XY3c18g5vsh2" +
                            "&ctp=1&lan=zh&spd=" + std::to_string(spd) +
                            "&pit=" + std::to_string(pit) +
                            "&vol=" + std::to_string(vol) +
                            "&per=" + std::to_string(per) +
                            "&aue=" + std::to_string(aue);
    
    // 清空静态响应缓冲区
    if(s_responseBuffer) {
        free(s_responseBuffer);
        s_responseBuffer = nullptr;
        s_responseLen = 0;
    }
    
    esp_http_client_config_t config = {};
    config.url = "https://tsn.baidu.com/text2audio";
    config.method = HTTP_METHOD_POST;
    config.event_handler = eventHandlerTTS;
    config.skip_cert_common_name_check = true;
    
    esp_err_t err = performRequest(config);
    if (err == ESP_OK) {
        if(s_responseBuffer != nullptr && s_responseLen > 0) {
            // 判断返回数据类型，根据 Content-Type 判断是音频数据还是 JSON 错误响应
            if (strstr(s_responseBuffer, "audio") == s_responseBuffer) {
                ESP_LOGI(TAG, "TTS audio data received, length: %d", s_responseLen);
                ESP_LOG_BUFFER_HEX(TAG, s_responseBuffer, 64);

                HttpManager::global_response_buffer = (char*)malloc(s_responseLen);
                memcpy(global_response_buffer,s_responseBuffer,s_responseLen);
                HttpManager::global_response_len = s_responseLen;

            } else {
                ESP_LOGI(TAG, "TTS JSON response: %.*s", s_responseLen, s_responseBuffer);
            }
            free(s_responseBuffer);
            s_responseBuffer = nullptr;
            s_responseLen = 0;
        }
        return true;
    }
    return false;
}

// Event handler for Baidu Token request
esp_err_t HttpManager::eventHandlerBaiduToken(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                if (s_responseBuffer == nullptr) {
                    s_responseBuffer = (char*) malloc(evt->data_len + 1);
                    if (s_responseBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory allocation failed in eventHandlerBaiduToken");
                        return ESP_FAIL;
                    }
                    s_responseLen = evt->data_len;
                    memcpy(s_responseBuffer, evt->data, evt->data_len);
                } else {
                    char* newBuffer = (char*) realloc(s_responseBuffer, s_responseLen + evt->data_len + 1);
                    if (newBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory reallocation failed in eventHandlerBaiduToken");
                        free(s_responseBuffer);
                        s_responseBuffer = nullptr;
                        s_responseLen = 0;
                        return ESP_FAIL;
                    }
                    s_responseBuffer = newBuffer;
                    memcpy(s_responseBuffer + s_responseLen, evt->data, evt->data_len);
                    s_responseLen += evt->data_len;
                }
                s_responseBuffer[s_responseLen] = '\0';
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "BaiduToken response: %s", s_responseBuffer ? s_responseBuffer : "NULL");
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR in eventHandlerBaiduToken");
            break;

        default:
            break;
    }
    return ESP_OK;
}

// Event handler for Spark request
esp_err_t HttpManager::eventHandlerSpark(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if(evt->data_len > 0) {
                if (s_responseBuffer == nullptr) {
                    s_responseBuffer = (char*) malloc(evt->data_len + 1);
                    if (s_responseBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory allocation failed in eventHandlerSpark");
                        return ESP_FAIL;
                    }
                    s_responseLen = evt->data_len;
                    memcpy(s_responseBuffer, evt->data, evt->data_len);
                } else {
                    char* newBuffer = (char*) realloc(s_responseBuffer, s_responseLen + evt->data_len + 1);
                    if (newBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory reallocation failed in eventHandlerSpark");
                        free(s_responseBuffer);
                        s_responseBuffer = nullptr;
                        s_responseLen = 0;
                        return ESP_FAIL;
                    }
                    s_responseBuffer = newBuffer;
                    memcpy(s_responseBuffer + s_responseLen, evt->data, evt->data_len);
                    s_responseLen += evt->data_len;
                }
                s_responseBuffer[s_responseLen] = '\0';
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "Spark response: %s", s_responseBuffer ? s_responseBuffer : "NULL");
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR in eventHandlerSpark");
            break;

        default:
            break;
    }
    return ESP_OK;
}

// Event handler for ASR request
esp_err_t HttpManager::eventHandlerASR(esp_http_client_event_t *evt) {

    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if(evt->data_len > 0) {
                if (s_responseBuffer == nullptr) {
                    s_responseBuffer = (char*) malloc(evt->data_len + 1);
                    if (s_responseBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory allocation failed in eventHandlerASR");
                        return ESP_FAIL;
                    }
                    s_responseLen = evt->data_len;
                    memcpy(s_responseBuffer, evt->data, evt->data_len);
                } else {
                    char* newBuffer = (char*) realloc(s_responseBuffer, s_responseLen + evt->data_len + 1);
                    if (newBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory reallocation failed in eventHandlerASR");
                        free(s_responseBuffer);
                        s_responseBuffer = nullptr;
                        s_responseLen = 0;
                        return ESP_FAIL;
                    }
                    s_responseBuffer = newBuffer;
                    memcpy(s_responseBuffer + s_responseLen, evt->data, evt->data_len);
                    s_responseLen += evt->data_len;
                }
                s_responseBuffer[s_responseLen] = '\0';
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "ASR response: %s", s_responseBuffer ? s_responseBuffer : "NULL");
            

            
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR in eventHandlerASR");
            break;

        default:
            break;
    }
    return ESP_OK;
}

// Event handler for TTS request
esp_err_t HttpManager::eventHandlerTTS(esp_http_client_event_t *evt) {
    static bool isAudioResponse = false;
    switch(evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            if (strcasecmp(evt->header_key, "Content-Type") == 0) {
                if (strstr(evt->header_value, "audio") == evt->header_value) {
                    isAudioResponse = true;
                    ESP_LOGI(TAG, "Audio response detected: %s", evt->header_value);
                } else {
                    isAudioResponse = false;
                    ESP_LOGI(TAG, "JSON error response detected: %s", evt->header_value);
                }
            }
            break;

        case HTTP_EVENT_ON_DATA:
            if(evt->data_len > 0) {
                if (s_responseBuffer == nullptr) {
                    s_responseBuffer = (char*) malloc(evt->data_len + 1);
                    if (s_responseBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory allocation failed in eventHandlerTTS");
                        return ESP_FAIL;
                    }
                    s_responseLen = evt->data_len;
                    memcpy(s_responseBuffer, evt->data, evt->data_len);
                } else {
                    char* newBuffer = (char*) realloc(s_responseBuffer, s_responseLen + evt->data_len + 1);
                    if (newBuffer == nullptr) {
                        ESP_LOGE(TAG, "Memory reallocation failed in eventHandlerTTS");
                        free(s_responseBuffer);
                        s_responseBuffer = nullptr;
                        s_responseLen = 0;
                        return ESP_FAIL;
                    }
                    s_responseBuffer = newBuffer;
                    memcpy(s_responseBuffer + s_responseLen, evt->data, evt->data_len);
                    s_responseLen += evt->data_len;
                }
                s_responseBuffer[s_responseLen] = '\0';
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            if (s_responseBuffer != nullptr && s_responseLen > 0) {
                if (isAudioResponse) {
                    ESP_LOGI(TAG, "TTS audio data received, length: %d", s_responseLen);
                    ESP_LOG_BUFFER_HEX(TAG, s_responseBuffer, 64);

                } else {
                    ESP_LOGI(TAG, "TTS JSON response: %.*s", s_responseLen, s_responseBuffer);
                }
            } else {
                ESP_LOGW(TAG, "No data received in TTS");
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR in eventHandlerTTS");
            break;

        default:
            break;
    }
    return ESP_OK;
}
