#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "main.h"

// ==========================================
// Cấu hình chân (GPIO Pins) cho STM32
// Chân Trigger và Echo được định nghĩa trong CubeMX / main.h
// ==========================================
#define TRIGGER_PIN             Trig_Pin          // Chân PB15 (Output)
#define TRIGGER_PORT            Trig_GPIO_Port    // GPIOB

#define ECHO_PIN                Echo_Pin          // Chân PA8 (Input)
#define ECHO_PORT               Echo_GPIO_Port    // GPIOA

// ==========================================
// Cấu hình LED báo trạng thái
// Đối với LED ngoài cắm vào chân PC13 và GND:
// - Chân PC13 xuất mức 1 (3.3V): Dòng chạy qua LED về GND -> LED SÁNG.
// - Chân PC13 xuất mức 0 (0.0V): Không có điện thế lệch -> LED TẮT HOÀN TOÀN.
// ==========================================
#define LED_PIN                 LED_Pin           // GPIO_PIN_13
#define LED_PORT                LED_GPIO_Port     // GPIOC

#define LED_ACTIVE_HIGH         1                 // 1: Active HIGH (LED ngoài cắm PC13 - GND), 0: Active LOW (LED trên board)

#if LED_ACTIVE_HIGH
#define LED_ON()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_OFF()               HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#else
#define LED_ON()                HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()               HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#endif

// ==========================================
// Cấu hình ngưỡng khoảng cách cảnh báo chớp tắt LED
// ==========================================
#define DISTANCE_WARN_MIN_CM    2.0f              // Khoảng cách tối thiểu (2.0 cm)
#define DISTANCE_WARN_MAX_CM    10.0f             // Ngưỡng cảnh báo tối đa (10.0 cm)

// ==========================================
// Cấu hình cảm biến siêu âm SRF05 / HC-SR04
// ==========================================
// Thời gian tối đa chờ phản hồi: 30000 us (~5 mét)
#define MAX_ECHO_TIMEOUT_US     30000UL

// Vận tốc âm thanh: 0.0343 cm/us -> khoảng cách = (thời gian * 0.0343) / 2
#define SOUND_SPEED_CM_US       0.0343f

// Thời gian nghỉ giữa các lần đo (ms) để sóng dội tan hết
#define MEASURE_INTERVAL_MS     150

// ==========================================
// Cấu hình giao tiếp Serial (USART2)
// TX: PA2, RX: PA3
// ==========================================
#define SERIAL_BAUD_RATE        115200

#endif // APP_CONFIG_H
