/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "app_config.h"
#include "lcd16x2.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t led_blink_enabled = 0;
volatile uint16_t led_toggle_interval = 15; // Chu kỳ nháy: interval * 10ms
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void DWT_Init(void);
void delay_us(uint32_t us);
uint32_t pulseIn(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState state, uint32_t timeout_us);
float measureDistanceCM(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}

void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
}

void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000UL);
  while ((DWT->CYCCNT - start) < ticks);
}

uint32_t pulseIn(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState state, uint32_t timeout_us)
{
  uint32_t timeout_ticks = timeout_us * (SystemCoreClock / 1000000UL);
  uint32_t min_ticks = 50UL * (SystemCoreClock / 1000000UL);
  uint32_t start_wait = DWT->CYCCNT;

  while ((DWT->CYCCNT - start_wait) < timeout_ticks)
  {
    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == state)
    {
      if ((DWT->CYCCNT - start_wait) > timeout_ticks) return 0;
    }
    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) != state)
    {
      if ((DWT->CYCCNT - start_wait) > timeout_ticks) return 0;
    }
    uint32_t pulse_start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == state)
    {
      if ((DWT->CYCCNT - pulse_start) > timeout_ticks) return 0;
    }
    uint32_t pulse_ticks = DWT->CYCCNT - pulse_start;
    if (pulse_ticks >= min_ticks)
    {
      return pulse_ticks / (SystemCoreClock / 1000000UL);
    }
  }
  return 0;
}

volatile uint32_t last_duration_us = 0;

float measureDistanceCM(void)
{
  // 1. Đảm bảo Trigger ở mức LOW trước khi phát
  HAL_GPIO_WritePin(TRIGGER_PORT, TRIGGER_PIN, GPIO_PIN_RESET);
  delay_us(4);

  // 2. Phát xung 10us kích hoạt SRF05
  HAL_GPIO_WritePin(TRIGGER_PORT, TRIGGER_PIN, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(TRIGGER_PORT, TRIGGER_PIN, GPIO_PIN_RESET);

  // 3. Đọc độ rộng xung chân Echo
  uint32_t duration = pulseIn(ECHO_PORT, ECHO_PIN, GPIO_PIN_SET, MAX_ECHO_TIMEOUT_US);
  last_duration_us = duration;

  // 4. Lọc nhiễu:
  // - duration == 0: Hết thời gian chờ (không có sóng hồi / quá xa / tuột dây)
  // - duration < 116us: Khoảng cách < 2cm (ngoài vùng vật lý của SRF05 hoặc xung nhiễu điện)
  if (duration == 0)
  {
    return -1.0f; // Timeout / Không thấy vật thể
  }
  if (duration < 116)
  {
    return -2.0f; // Quá gần (< 2cm vùng mù) hoặc xung nhiễu
  }

  // Vận tốc âm thanh: 0.0343 cm/us -> khoảng cách = (thời gian * 0.0343) / 2
  return ((float)duration * SOUND_SPEED_CM_US) / 2.0f;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init();

  // Đảm bảo ban đầu LED tắt hoàn toàn
  LED_OFF();

  // Cấu hình TIM2 ngắt định kỳ 10ms cố định (Prescaler=7199 -> 10kHz, Period=99 -> 10ms)
  // Không bao giờ đổi ARR khi đang chạy để tránh lỗi CNT > ARR làm đơ ngắt
  __HAL_TIM_SET_AUTORELOAD(&htim2, 99);
  htim2.Instance->CNT = 0;
  HAL_TIM_Base_Start_IT(&htim2);

  // Khởi tạo màn hình LCD 16x2 chế độ 4-bit
  lcd16x2_init(LCD16X2_DISPLAY_ON_CURSOR_OFF_BLINK_OFF);
  lcd16x2_gotoxy(0, 0);
  lcd16x2_puts("SRF05 Ultrasonic");
  lcd16x2_gotoxy(0, 1);
  lcd16x2_puts("Khoi dong...    ");

  HAL_Delay(1000);

  printf("\r\n==================================================\r\n");
  printf("   STM32 - DO KHOANG CACH SIEU AM SRF05           \r\n");
  printf("==================================================\r\n");
  printf("Trigger: PB15 | Echo: PA8 | LED: PC13\r\n");
  printf("LCD 16x2 (4-bit): RS=PB12, RW=PB13, EN=PB14, D4=PA4, D5=PA5, D6=PA6, D7=PA7\r\n");
  printf("Canh bao: Khoang cach tu %.1f cm den %.1f cm -> LED chop tat\r\n", 
         DISTANCE_WARN_MIN_CM, DISTANCE_WARN_MAX_CM);
  printf("Output UART: USART1 (PA9_TX) & USART2 (PA2_TX) | Baud: %d\r\n", SERIAL_BAUD_RATE);

  HAL_Delay(200);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    float distance = measureDistanceCM();
    static uint8_t alert_active = 0;
    static uint8_t miss_count = 0;

    // Điều kiện cảnh báo: từ khoảng cách tối thiểu (2.0 cm) đến 10.0 cm hoặc vật thể sát cảm biến (< 2cm)
    if ((distance >= DISTANCE_WARN_MIN_CM && distance <= DISTANCE_WARN_MAX_CM) || (distance == -2.0f))
    {
      float d = (distance == -2.0f) ? DISTANCE_WARN_MIN_CM : distance;
      
      // Chu kỳ chớp tắt:
      // - Ở 10.0 cm: interval = 20 (toggle mỗi 200ms -> chu kỳ 400ms = 2.5 lần/giây, rất rõ ràng)
      // - Ở  2.0 cm: interval =  6 (toggle mỗi  60ms -> chu kỳ 120ms = ~8.3 lần/giây, cảnh báo nhanh)
      uint16_t interval = 6 + (uint16_t)((d - DISTANCE_WARN_MIN_CM) / (DISTANCE_WARN_MAX_CM - DISTANCE_WARN_MIN_CM) * 14.0f);
      if (interval < 5) interval = 5;
      
      led_toggle_interval = interval;
      led_blink_enabled = 1;
      alert_active = 1;
      miss_count = 0;

      printf("Dist: %6.2f cm [WARNING]\r\n",
             d, d * 10.0f, interval * 10);
    }
    else if (distance > DISTANCE_WARN_MAX_CM)
    {
      // Khoảng cách an toàn > 10cm: TẮT CẢNH BÁO NGAY LẬP TỨC
      alert_active = 0;
      miss_count = 0;
      led_blink_enabled = 0;
      LED_OFF();

      printf("Dist: %6.2f cm [SAFE]\r\n", 
             distance, distance * 10.0f);
    }
    else // distance == -1.0f (Timeout)
    {
      if (alert_active && (++miss_count < 3))
      {
        // Vừa phát hiện vật cản trước đó mà bị lỡ 1-2 lần ping -> Tiếp tục duy trì nháy LED cảnh báo
        printf("[!] Tam thoi mat Echo (%d/2) -> Duy tri chop LED canh bao\r\n", miss_count);
      }
      else
      {
        alert_active = 0;
        miss_count = 0;
        led_blink_enabled = 0;
        LED_OFF();
        printf("[!] Khong nhan duoc Echo (Timeout > %lu us) [LED: TAT]\r\n", MAX_ECHO_TIMEOUT_US);
      }
    }

    // ==========================================
    // Cập nhật hiển thị LCD 16x2 (4-bit mode)
    // Hàng 1: "Dist: xx.x cm"
    // Hàng 2: "SAFE" nếu khoảng cách > 10cm, "WARNING" nếu <= 10cm
    // ==========================================
    char lcd_line1[17];
    char lcd_line2[17];

    if (distance >= DISTANCE_WARN_MIN_CM)
    {
      snprintf(lcd_line1, sizeof(lcd_line1), "Dist: %4.1f cm   ", distance);
    }
    else if (distance == -2.0f)
    {
      snprintf(lcd_line1, sizeof(lcd_line1), "Dist: < 2.0 cm  ");
    }
    else // distance == -1.0f (Timeout: vật quá xa hoặc ngoài góc phản hồi)
    {
      snprintf(lcd_line1, sizeof(lcd_line1), "Dist: Out Range ");
    }

    if (distance > DISTANCE_WARN_MAX_CM || (!alert_active && distance == -1.0f))
    {
      snprintf(lcd_line2, sizeof(lcd_line2), "SAFE            ");
    }
    else
    {
      snprintf(lcd_line2, sizeof(lcd_line2), "WARNING         ");
    }

    lcd16x2_gotoxy(0, 0);
    lcd16x2_puts(lcd_line1);
    lcd16x2_gotoxy(0, 1);
    lcd16x2_puts(lcd_line2);

    // Nghỉ 150ms để sóng dội tan hoàn toàn trong không gian
    HAL_Delay(MEASURE_INTERVAL_MS);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|Trig_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Trig_Pin */
  GPIO_InitStruct.Pin = Trig_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(Trig_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Echo_Pin */
  GPIO_InitStruct.Pin = Echo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Echo_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  GPIO_InitStruct.Pin = Echo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(Echo_GPIO_Port, &GPIO_InitStruct);

  // Đảm bảo LED tắt ngay sau khi khởi tạo GPIO
  LED_OFF();
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    static uint16_t timer_10ms_count = 0;
    static uint8_t led_state = 0;

    if (led_blink_enabled)
    {
      timer_10ms_count++;
      if (timer_10ms_count >= led_toggle_interval)
      {
        timer_10ms_count = 0;
        led_state = !led_state;
        if (led_state)
        {
          LED_ON();
        }
        else
        {
          LED_OFF();
        }
      }
    }
    else
    {
      timer_10ms_count = 0;
      led_state = 0;
      LED_OFF(); // Đảm bảo LED tắt hoàn toàn 100%
    }
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
