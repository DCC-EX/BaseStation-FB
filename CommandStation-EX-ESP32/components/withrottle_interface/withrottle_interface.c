/*
 *  withrottle_interface.c
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

#include "withrottle_interface.h"

#include <lwip/sockets.h>
#include <esp_log.h>
#include <string.h>
#include <errno.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <freertos/queue.h>

static const char *TAG = "WiThrottle interface";

struct withrottle_client_args {
    int             sock;
    QueueHandle_t   queue;
};

void withrottle_server_task(void* p) {
    struct sockaddr_in clientAddress;
    struct sockaddr_in serverAddress;

    QueueHandle_t queue = (QueueHandle_t)p;

    // Create a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0) {
        ESP_LOGE(TAG, "socket: %d %s", sock, strerror(errno));
        goto CLEAN_UP;
    }

    // Bind the socket to the port selected in config
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(CONFIG_WITHROTTLE_PORT);
    int rc = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if(rc < 0) {
        ESP_LOGE(TAG, "bind: %d %s", rc, strerror(errno));
        goto CLEAN_UP;
    }

    // Flag the socket as listening for new connections
    rc = listen(sock, 5);
    if(rc < 0) {
        ESP_LOGE(TAG, "listen: %d %s", rc, strerror(errno));
        goto CLEAN_UP;
    }

    while(1) {
        ESP_LOGI(TAG, "Waiting for new WiThrottle connections");
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if(rc < 0) {
            ESP_LOGE(TAG, "accept: %d %s", clientSock, strerror(errno));
            goto CLEAN_UP;
        }

        // Create a struct and allocate memory for it so we can pass multiple parameters to child task
        struct withrottle_client_args *client_args = malloc(sizeof(struct withrottle_client_args));
        if (client_args == NULL) {
            ESP_LOGE(TAG, "Failed to alloc task args!");
            goto CLEAN_UP;
        }
        client_args->sock = clientSock;
        client_args->queue = queue;

        xTaskCreate(&withrottle_client_task, "withrottle_client_task", 2048, client_args, 5, NULL);
    }

CLEAN_UP:
    vTaskDelete(NULL);
}

void withrottle_client_task(void* p) {
    struct withrottle_client_args *args = (struct withrottle_client_args *)p;
    
    int len;
    char rx_buffer[256];

    do {
        len = recv(args->sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if(len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
            goto CLEAN_UP;
        } else if(len == 0) {
            ESP_LOGW(TAG, "Connection closed");
            goto CLEAN_UP;
        } else {
            // Take received stuff and put it onto the send queue here
            rx_buffer[len] = 0; // Null terminate buffer
            ESP_LOGI(TAG, "%s", rx_buffer);
            xQueueSend(args->queue, rx_buffer, portMAX_DELAY);
        }
    } while(len > 0);

CLEAN_UP:
    close(args->sock);
}