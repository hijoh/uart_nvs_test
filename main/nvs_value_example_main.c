#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "nvs.h"
#include "driver/gpio.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

void uart_task(void *pvParameters) {
    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (len > 0) {
            // 从串口读取到数据
            int number = atoi((char *)data);
            printf("Received number: %d\n", number);

            // 保存数据到 NVS
            nvs_handle_t nvs_handle;
            esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
            if (err == ESP_OK) {
                err = nvs_set_i32(nvs_handle, "number", number);
                if (err == ESP_OK) {
                    err = nvs_commit(nvs_handle);
                    if (err != ESP_OK) {
                        printf("Failed to commit data to NVS\n");
                    }
                } else {
                    printf("Failed to set data in NVS\n");
                }
            } else {
                printf("Failed to open NVS\n");
            }
            nvs_close(nvs_handle);
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void app_main() {
    // 初始化 NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 配置串口
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0));

    // 创建串口任务
    xTaskCreate(uart_task, "uart_task", 2048, NULL, 10, NULL);

    // 从 NVS 中获取存储的数据
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        int stored_number;
        err = nvs_get_i32(nvs_handle, "number", &stored_number);
        if (err == ESP_OK) {
            printf("Restored number: %d\n", stored_number);
            // 执行相关命令（在此处添加你的代码）
            if (stored_number == 0) {
            // 设置 GPIO10 为输出模式
            esp_rom_gpio_pad_select_gpio(GPIO_NUM_10);
            gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT);
        
            // 将 GPIO10 的电平设置为0
            gpio_set_level(GPIO_NUM_10, 0);
            printf("GPIO10 level set to 0\n");
        }
        } else {
            printf("Failed to get data from NVS\n");
        }
    } else {
        printf("Failed to open NVS\n");
    }
    nvs_close(nvs_handle);
}