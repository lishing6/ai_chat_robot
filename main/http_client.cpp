#include "http_client.h"

static const char *TAG = "SPARK_MAX";
static const char *TAG2= "ACCESS_TOKEN";
static const char *TAG3 = "BAIDU_ASR";

std::string result_text;
std::string result_Token = "24.53c1041dd8cddb6d6e61a6bcc17dd811.2592000.1735831384.282335-115821649"; // To store the result
std::string result_speech_to_text;

//varities of loudspeaker
SemaphoreHandle_t response_mutex = NULL;
char *global_response_buffer = NULL;
int global_response_len = 0;

//answer of sprak
std::string answer_spark;
size_t answer_spark_length;


static std::string url_encode(const std::string &value);





static esp_err_t http_event_handler_baidu_token(esp_http_client_event_t *evt) {
    static char *global_response_buffer = NULL;
    static int global_response_len = 0;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (esp_http_client_is_chunked_response(evt->client)) {
                if (global_response_buffer == NULL) {
                    global_response_buffer = (char *)malloc(evt->data_len + 1);
                    global_response_len = evt->data_len;
                    memcpy(global_response_buffer, evt->data, evt->data_len);
                } else {
                    global_response_buffer = (char *)realloc(global_response_buffer, global_response_len + evt->data_len + 1);
                    memcpy(global_response_buffer + global_response_len, evt->data, evt->data_len);
                    global_response_len += evt->data_len;
                }
                global_response_buffer[global_response_len] = 0; // Null-terminate
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            if (global_response_buffer) {
                ESP_LOGI(TAG2, "HTTP Response: %s", global_response_buffer);

                // 解析JSON响应
                cJSON *json = cJSON_Parse(global_response_buffer);
                if (json) {
                    cJSON *access_token = cJSON_GetObjectItem(json, "access_token");
                    if (access_token) {
                        result_Token = access_token->valuestring;
                        ESP_LOGI(TAG2, "Access Token: %s", result_Token.c_str());
                        
                    } else {
                        ESP_LOGE(TAG2, "Failed to find access_token in JSON response");
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG2, "Failed to parse JSON response");
                }

                free(global_response_buffer);
                global_response_buffer = NULL;
                global_response_len = 0;
            }
            break;
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG2, "HTTP_EVENT_ERROR");
            
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t http_event_handler_spark(esp_http_client_event_t *evt) {
    static char *response_buffer = NULL;
    static int response_len = 0;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                if (response_buffer == NULL) {
                    // 初次分配内存
                    response_buffer = (char *)malloc(evt->data_len + 1);
                    if (response_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory");
                        return ESP_FAIL;
                    }
                    response_len = evt->data_len;
                } else {
                    // 追加数据
                    char *new_buffer = (char *)realloc(response_buffer, response_len + evt->data_len + 1);
                    if (new_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to reallocate memory");
                        free(response_buffer);
                        response_buffer = NULL;
                        return ESP_FAIL;
                    }
                    response_buffer = new_buffer;
                    response_len += evt->data_len;
                }
                memcpy(response_buffer + response_len - evt->data_len, evt->data, evt->data_len);
                response_buffer[response_len] = '\0'; // Null-terminate
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            // 打印完整响应
            if (response_buffer) {
                ESP_LOGI(TAG, "Full Response: %s", response_buffer);

                // 解析JSON响应
                cJSON *json = cJSON_Parse(response_buffer);
                if (json) {
                    cJSON * choices = cJSON_GetObjectItem(json, "choices");
                    if (choices) {
                            cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
                            if (first_choice) {
                                cJSON *message = cJSON_GetObjectItem(first_choice, "message");
                                if (message) {
                                   cJSON *content = cJSON_GetObjectItem(message, "content");
                                   if (content && cJSON_IsString(content)) {
                                        answer_spark = content->valuestring;
                                        answer_spark_length = answer_spark.length();

                                        //printf("Extracted content: %s\n", answer_spark.c_str());
                                   }
                               }
                           }                          
                    } else {
                        ESP_LOGE(TAG, "Failed to find choices in JSON response");
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG, "Failed to parse JSON response");
                }

                free(response_buffer);
                response_buffer = NULL;
                response_len = 0;
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            if (response_buffer) {
                free(response_buffer);
                response_buffer = NULL;
                response_len = 0;
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t http_event_handler_asr(esp_http_client_event_t *evt) {
    static char *asr_global_response_buffer = NULL;
    static int asr_global_response_len = 0;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                if (asr_global_response_buffer == NULL) {
                    // 初次分配内存
                    asr_global_response_buffer = (char *)malloc(evt->data_len + 1);
                    if (asr_global_response_buffer == NULL) {
                        ESP_LOGE(TAG3, "Failed to allocate memory");
                        return ESP_FAIL;
                    }
                    asr_global_response_len = evt->data_len;
                } else {
                    // 追加数据
                    char *new_buffer = (char *)realloc(asr_global_response_buffer, asr_global_response_len + evt->data_len + 1);
                    if (new_buffer == NULL) {
                        ESP_LOGE(TAG3, "Failed to reallocate memory");
                        free(asr_global_response_buffer);
                        asr_global_response_buffer = NULL;
                        return ESP_FAIL;
                    }
                    asr_global_response_buffer = new_buffer;
                    asr_global_response_len += evt->data_len;
                }
                memcpy(asr_global_response_buffer + asr_global_response_len - evt->data_len, evt->data, evt->data_len);
                asr_global_response_buffer[asr_global_response_len] = 0; // Null-terminate
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            // 打印完整响应
            if (asr_global_response_buffer) {
                ESP_LOGI(TAG3, "Full Response: %s", asr_global_response_buffer);

                // 解析JSON响应
                cJSON *json = cJSON_Parse(asr_global_response_buffer);
                if (json) {
                    cJSON * result = cJSON_GetObjectItem(json, "result");
                    if (result) {
                        result_speech_to_text = result->valuestring;
                        ESP_LOGI(TAG3, "result_speech_to_text: %s", result_speech_to_text.c_str());
                        
                    } else {
                        ESP_LOGE(TAG3, "Failed to find result_speech_to_text in JSON response");
                    }
                    cJSON_Delete(json);
                } else {
                    ESP_LOGE(TAG3, "Failed to parse JSON response");
                }

                free(asr_global_response_buffer);
                asr_global_response_buffer = NULL;
                asr_global_response_len = 0;
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG3, "HTTP_EVENT_ERROR");
            if (asr_global_response_buffer) {
                free(asr_global_response_buffer);
                asr_global_response_buffer = NULL;
                asr_global_response_len = 0;
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t http_event_handler_tts(esp_http_client_event_t *evt) {
    static const char *TAG = "BAIDU_TTS";
    static bool is_audio_response = false;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_HEADER:
            if (strcasecmp(evt->header_key, "Content-Type") == 0) {
                if (strstr(evt->header_value, "audio") == evt->header_value) {
                    is_audio_response = true;
                    ESP_LOGI(TAG, "Audio response detected: %s", evt->header_value);
                } else {
                    is_audio_response = false;
                    ESP_LOGI(TAG, "JSON error response detected: %s", evt->header_value);
                }
            }
            break;

        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                xSemaphoreTake(response_mutex, portMAX_DELAY);

                if (global_response_buffer == NULL) {
                    global_response_buffer = (char *)malloc(evt->data_len);
                    if (global_response_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory");
                        xSemaphoreGive(response_mutex);
                        return ESP_FAIL;
                    }
                    global_response_len = evt->data_len;
                } else {
                    char *new_buffer = (char *)realloc(global_response_buffer, global_response_len + evt->data_len);
                    if (new_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to reallocate memory");
                        free(global_response_buffer);
                        global_response_buffer = NULL;
                        global_response_len = 0;
                        xSemaphoreGive(response_mutex);
                        return ESP_FAIL;
                    }
                    global_response_buffer = new_buffer;
                    global_response_len += evt->data_len;
                }
                memcpy(global_response_buffer + global_response_len - evt->data_len, evt->data, evt->data_len);

                xSemaphoreGive(response_mutex);
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            if (global_response_buffer != NULL && global_response_len > 0) {
                if (is_audio_response) {
                    ESP_LOGI(TAG, "Audio response received, length: %d", global_response_len);
                    ESP_LOG_BUFFER_HEX(TAG, global_response_buffer, 64);  // 打印前 64 字节
                } else {
                    ESP_LOGI(TAG, "JSON error response: %.*s", global_response_len, global_response_buffer);
                }
            } else {
                ESP_LOGW(TAG, "No data received");
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;

        default:
            break;
    }
    return ESP_OK;
}


// Send request to Spark Max
void send_spark_request(const char *query) {
    // Prepare JSON payload
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "generalv3.5");
    cJSON *messages = cJSON_AddArrayToObject(root, "messages");

    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", "user");
    cJSON_AddStringToObject(message, "content", query);   //send data to spark max
    cJSON_AddItemToArray(messages, message);

    char *post_data = cJSON_Print(root);  // serializes the JSON structure into a string that can be transmitted over the network

    // Set up HTTP client configuration
    esp_http_client_config_t config = {
        .url = api_url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler_spark,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Add headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", api_password);

    // Send POST request
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    // Clean up
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(post_data);
}

void fetch_and_parse_access_token() {
    char url[512];
    snprintf(url, sizeof(url),
             "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%s&client_secret=%s",
             client_id, client_secret);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler_baidu_token,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE(TAG2, "Failed to initialize HTTP client");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG2, "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG2, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void send_audio_to_baidu_asr_array(uint8_t *audio_data, size_t audio_len, const char *cuid) {
    // Base64 编码音频数据
    size_t encoded_len = ((audio_len + 2) / 3) * 4 + 1; // +1 for null-terminator
    char *base64_encoded_audio = reinterpret_cast<char*>(heap_caps_calloc(encoded_len, sizeof(char), MALLOC_CAP_SPIRAM));
    if (!base64_encoded_audio) {
        ESP_LOGE(TAG3, "Failed to allocate memory for base64 encoding");
        return;
    }

    size_t out_len;
    mbedtls_base64_encode((unsigned char *)base64_encoded_audio, encoded_len, &out_len, audio_data, audio_len);
    heap_caps_free(audio_data);   // 清除录音的内容，减少占用PSRAM
    
    // 使用 cJSON 生成 JSON 数据
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG3, "Failed to create JSON object");
        heap_caps_free(base64_encoded_audio);
        return;
    }
    //ESP_LOGE(TAG3, "base64_encoded_audio:%s",base64_encoded_audio);
    
    cJSON_AddStringToObject(root, "format", "pcm");
    cJSON_AddNumberToObject(root, "rate", 16000);
    cJSON_AddNumberToObject(root, "channel", 1);
    cJSON_AddStringToObject(root, "token", result_Token.c_str());
    cJSON_AddStringToObject(root, "cuid", "Rwb0oGDKThTRRMQwOmu7XY3c18g5vsh2");
    cJSON_AddNumberToObject(root, "len", audio_len);

    // ESP_LOGI(TAG3, "Free heap: %d, Free internal heap: %d",
    //      esp_get_free_heap_size(),
    //      esp_get_free_internal_heap_size());

    if (!cJSON_AddStringToObject(root, "speech", base64_encoded_audio)) {
         ESP_LOGE(TAG3, "Failed to add speech field to JSON");
    }

    char *post_data = cJSON_Print(root);
    if (!post_data) {
        ESP_LOGE(TAG3, "Failed to serialize JSON");
        cJSON_Delete(root);
        heap_caps_free(base64_encoded_audio);
        return;
    }

    ESP_LOGI(TAG3, "Post Data: %s", post_data);

    // HTTP 客户端配置
    esp_http_client_config_t config = {
        .url = "https://vop.baidu.com/server_api",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler_asr,
        .skip_cert_common_name_check = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG3, "Failed to initialize HTTP client");
        cJSON_Delete(root);
        heap_caps_free(base64_encoded_audio);
        free(post_data);
        return;
    }

    // 设置 HTTP 请求头和正文
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client,"Accept","application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    // 发送 HTTP 请求
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    // 清理资源
    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    heap_caps_free(base64_encoded_audio);
    free(post_data);
}

void send_text_to_baidu_tts(const std::string &text, int spd, int pit, int vol, int per, int aue) {
    static const char *TAG = "BAIDU_TTS";

    // 双重 URL 编码
    std::string encoded_text = url_encode(url_encode(text));

    std::string post_data = "tex=" + encoded_text +
                            "&tok=" + result_Token +
                            "&cuid=" + "Rwb0oGDKThTRRMQwOmu7XY3c18g5vsh2" +
                            "&ctp=1&lan=zh&spd=" + std::to_string(spd) +
                            "&pit=" + std::to_string(pit) +
                            "&vol=" + std::to_string(vol) +
                            "&per=" + std::to_string(per) +
                            "&aue=" + std::to_string(aue);

    esp_http_client_config_t config = {
        .url = "https://tsn.baidu.com/text2audio",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler_tts,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data.c_str(), post_data.size());

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200 && global_response_buffer != NULL && global_response_len > 0) {
            if (strstr(global_response_buffer, "audio") != NULL) {
                //audio_play(reinterpret_cast<uint8_t *>(global_response_buffer), global_response_len);
            } else {
                //ESP_LOGE(TAG, "Failed to play audio: JSON response received");
            }
        } else {
            ESP_LOGE(TAG, "HTTP status: %d, response length: %d", status_code, global_response_len);
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}


static std::string url_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');                               //当格式化数字时，使用 0 填充到固定宽度，例如 %02 格式。
    escaped << std::hex;                             //将流的数值输出格式设为十六进制，用于生成 %XX 编码。

    for (char c : value) {
        //这里使用 static_cast<unsigned char>(c) 将字符转换为 unsigned char 类型，
        //以确保 isalnum 能正确处理所有字符（防止负值字符的问题）。
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}