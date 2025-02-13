#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

// Wi-Fi 配置宏，根据需要修改
#define WIFI_SSID "iPgone"        // Wi-Fi 名称
#define WIFI_PASS "88888888"      // Wi-Fi 密码

// Wi-Fi 事件组中表示已连接的位
const int CONNECTED_BIT = BIT0;

/**
 * @brief WiFiManager 类封装了基于 FreeRTOS 事件组实现的 Wi-Fi 初始化和连接管理。
 *
 * 该类通过初始化 NVS、网络接口、事件循环和 Wi-Fi 驱动，实现 STA 模式下的 Wi-Fi 连接。
 * 同时，在事件处理函数中实现了断线重连机制，并通过事件组通知上层任务 Wi-Fi 连接成功。
 */
class WiFiManager {
public:
    /**
     * @brief 构造函数，创建 Wi-Fi 事件组。
     */
    WiFiManager();

    /**
     * @brief 连接到配置的 Wi-Fi 网络。
     *
     * 内部会先初始化 NVS，然后调用 wifi_init_sta() 进行网络接口、事件处理函数注册以及启动 Wi-Fi。
     * 最后，等待事件组中 CONNECTED_BIT 被置位，以确认设备已连接成功。
     */
    void connect();

    /**
     * @brief 获取 Wi-Fi 事件组句柄（供其他模块使用）。
     * @return EventGroupHandle_t 
     */
    EventGroupHandle_t getEventGroup() const { return wifi_event_group; }

private:
    EventGroupHandle_t wifi_event_group;  // FreeRTOS 事件组，用于同步 Wi-Fi 连接状态

    static const char* TAG;  // 日志标签

    /**
     * @brief 静态事件处理函数，用于处理 Wi-Fi 和 IP 相关事件。
     *
     * 该函数根据事件类型进行处理：
     *  - WIFI_EVENT_STA_START：调用 esp_wifi_connect() 发起连接；
     *  - WIFI_EVENT_STA_DISCONNECTED：打印日志并重新连接；
     *  - IP_EVENT_STA_GOT_IP：打印获得的 IP 并设置事件组中的 CONNECTED_BIT。
     *
     * @param arg 用户自定义参数（这里未使用）。
     * @param event_base 事件基，如 WIFI_EVENT 或 IP_EVENT。
     * @param event_id 事件 ID。
     * @param event_data 事件数据指针。
     */
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);
};

#endif // WIFI_MANAGER_H
