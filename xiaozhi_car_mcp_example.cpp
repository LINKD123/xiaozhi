/*
 * 只改小智，不改 STM32。
 * 发送与原 Nano base_control 相同的 USART1 速度帧。
 *
 * 接线：ESP32 TX→40针脚8，RX→脚10（可先不接），GND→脚6。
 * 整套 Nano 必须拔掉。115200 8N1。
 * 原固件约 800ms 无新速度会停车，前进时要重复发帧。
 *
 * DevKitC-1 排针没有 GPIO17/18，默认用 GPIO8(TX)、GPIO9(RX)。
 */
#include "mcp_server.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

#define CAR_UART_NUM   UART_NUM_1
#define UART_TX_PIN    GPIO_NUM_8
#define UART_RX_PIN    GPIO_NUM_9
#define REPEAT_MS      200

static int16_t sVx = 0;
static int16_t sVy = 0;
static int16_t sWz = 0;
static bool bRepeat = false;

static void CarUartInit()
{
    uart_config_t cfg = {};
    cfg.baud_rate = 115200;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;
    uart_driver_install(CAR_UART_NUM, 256, 0, 0, NULL, 0);
    uart_param_config(CAR_UART_NUM, &cfg);
    uart_set_pin(CAR_UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/* 0x01 设置速度；末字节 0xFF 可跳过 STM32 CRC */
static void CarSendVel(int16_t vx, int16_t vy, int16_t wz)
{
    uint8_t f[12] = {0x5A, 0x0C, 0x01, 0x01, 0, 0, 0, 0, 0, 0, 0x00, 0xFF};
    f[4] = (uint8_t)(vx >> 8);
    f[5] = (uint8_t)vx;
    f[6] = (uint8_t)(vy >> 8);
    f[7] = (uint8_t)vy;
    f[8] = (uint8_t)(wz >> 8);
    f[9] = (uint8_t)wz;
    uart_write_bytes(CAR_UART_NUM, (const char *)f, 12);
}

static void CarSet(int16_t vx, int16_t vy, int16_t wz, bool repeat)
{
    sVx = vx;
    sVy = vy;
    sWz = wz;
    bRepeat = repeat;
    CarSendVel(vx, vy, wz);
}

static void RepeatTask(void *)
{
    while (true) {
        if (bRepeat) {
            CarSendVel(sVx, sVy, sWz);
        }
        vTaskDelay(pdMS_TO_TICKS(REPEAT_MS));
    }
}

void InitializeCarTools()
{
    CarUartInit();
    xTaskCreate(RepeatTask, "car_vel", 2048, NULL, 5, NULL);
    auto &mcp = McpServer::GetInstance();

    mcp.AddTool("self.car.forward", "让小车前进", PropertyList(),
        [](const PropertyList &) -> ReturnValue { CarSet(250, 0, 0, true); return true; });
    mcp.AddTool("self.car.back", "让小车后退", PropertyList(),
        [](const PropertyList &) -> ReturnValue { CarSet(-250, 0, 0, true); return true; });
    mcp.AddTool("self.car.left", "让小车左转", PropertyList(),
        [](const PropertyList &) -> ReturnValue { CarSet(0, 0, 350, true); return true; });
    mcp.AddTool("self.car.right", "让小车右转", PropertyList(),
        [](const PropertyList &) -> ReturnValue { CarSet(0, 0, -350, true); return true; });
    mcp.AddTool("self.car.stop", "让小车立即停止", PropertyList(),
        [](const PropertyList &) -> ReturnValue { CarSet(0, 0, 0, false); return true; });
}
