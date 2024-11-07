#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include <string.h>


#define WIFI_SSID "iPgone"      // 替换为你的 Wi-Fi 名称
#define WIFI_PASS "88888888"  // 替换为你的 Wi-Fi 密码


void wifi_connect();

