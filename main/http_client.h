#pragma once 

#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include <string>
#include <iomanip>
#include <sstream>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "play.h"

// iFlytek credentials
inline const char *app_id = "1a23cfc9";
inline const char *api_password = "Bearer RljmzVZjCNLEsLkGNYFD:BeyXHjeMoIJTqMtzZHiu";
inline const char *api_url = "https://spark-api-open.xf-yun.com/v1/chat/completions";

//baidu credentials
inline const char *client_id = "MoYDwbkJPQ8TNOoATdFwuNCQ";
inline const char *client_secret = "bGwVrtVrsbDI5u62N2lkWkly2aN3vQzX";

extern char *global_response_buffer;
extern int global_response_len;
extern SemaphoreHandle_t response_mutex;  //为了线程安全

extern std::string result_speech_to_text;
extern std::string answer_spark;
extern size_t answer_spark_length;

void send_spark_request(const char *query);
void fetch_and_parse_access_token();
void send_audio_to_baidu_asr_array(uint8_t *audio_data, size_t audio_len, const char *cuid);
void send_text_to_baidu_tts(const std::string &text, int spd, int pit, int vol, int per, int aue);
