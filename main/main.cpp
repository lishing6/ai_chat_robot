#include "play.h"
#include "record.h"
#include <esp_task_wdt.h>

extern uint8_t* wav_buffer;

extern "C"
void app_main() {
    esp_task_wdt_deinit(); // 禁用任务看门狗
    i2sInitInput();
    i2s_adc();
    i2sInitOutput();
    i2s_play(wav_buffer,FLASH_RECORD_SIZE + WAV_HEADER_SIZE);

    while (true)
    {
        /* code */
    }
    
}
