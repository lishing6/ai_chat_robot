#include "play.h"
#include "record.h"
#include "wifi_connect.h"
#include "http_client.h"
#include <esp_task_wdt.h>


extern uint8_t* wav_buffer;

extern "C"
void app_main() {

    esp_task_wdt_deinit();//disable TWDT for debug
    wifi_connect();
    send_spark_request("hello");
    while (true)
    {
        /* code */
    }
}

