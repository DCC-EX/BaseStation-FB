/*
 *  CommandStation-EX-ESP32.c
 * 
 *  This file is part of CommandStation-EX.
 *
 *  CommandStation-EX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation-EX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation-EX.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include <freertos/queue.h>

#include "wifi_config.h"
#include "withrottle_interface.h"

static const char *TAG = "CommandStation-EX-ESP32";

QueueHandle_t wifiToUartQueue;

void app_main(void)
{
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "Initialized flash successfully");

  wifi_init_sta();
  wifi_init_mdns();
  
  // Create a queue using char pointers to strings. It is the job of the user to
  // de-allocate the memory from these strings to avoid memory leaks.
  wifiToUartQueue = xQueueCreate(42, sizeof(char*));

  xTaskCreate(withrottle_server_task, "withrottle_server_task", 4098, wifiToUartQueue, 5, NULL);
  // Add a JMRI server task
  // Add a UART routing task  
}