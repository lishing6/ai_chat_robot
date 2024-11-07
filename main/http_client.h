#pragma once 

#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"

// Replace with your iFlytek credentials

inline char *app_id = "1a23cfc9";
inline const char *api_password = "Bearer RljmzVZjCNLEsLkGNYFD:BeyXHjeMoIJTqMtzZHiu";
inline char *api_url = "https://spark-api-open.xf-yun.com/v1/chat/completions";

void send_spark_request(const char *query);


