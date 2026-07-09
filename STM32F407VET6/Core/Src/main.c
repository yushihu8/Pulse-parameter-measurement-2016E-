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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "headfile.h"
#include "lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CAPTURE_WAIT_MS 20U
#define ADC_CAPTURE_SAMPLE_COUNT 1024U
#define ADC_SAMPLE_RATE_HZ 50000000.0f
#define ADC_FULL_SCALE_VPP 4.0f
#define ADC_MAX_CODE 16383U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static uint16_t adc_raw_buff[ADC_CAPTURE_SAMPLE_COUNT];
static uint16_t adc_work_buff[ADC_CAPTURE_SAMPLE_COUNT];
static pulse_measure_config_t g_pulse_cfg;
static pulse_measure_result_t g_pulse_result;

static uint32_t scale_positive_float(float value, float scale)
{
  if(value <= 0.0f)
  {
    return 0U;
  }

  return (uint32_t)((value * scale) + 0.5f);
}

static void lcd_clear_value_line(uint16_t y)
{
  LCD_Fill(150U, y, 305U, y + 24U, WHITE);
}

static void lcd_show_placeholder(uint16_t y)
{
  lcd_clear_value_line(y);
  LCD_ShowString(150U, y, (const uint8_t *)"--", RED, WHITE, 24U, 0U);
}

static void lcd_show_status_text(const char *text, uint16_t color)
{
  lcd_clear_value_line(70U);
  LCD_ShowString(150U, 70U, (const uint8_t *)text, color, WHITE, 24U, 0U);
}

static void lcd_draw_measure_ui(void)
{
  LCD_Fill(0U, 0U, 320U, 240U, WHITE);
  LCD_DrawRectangle(8U, 8U, 312U, 232U, BLUE);
  LCD_DrawLine(8U, 46U, 312U, 46U, LIGHTBLUE);

  LCD_ShowString(18U, 16U, (const uint8_t *)"Pulse Measure", BLUE, WHITE, 24U, 0U);
  LCD_ShowString(20U, 70U, (const uint8_t *)"State:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, 105U, (const uint8_t *)"Amp(Vpp):", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, 140U, (const uint8_t *)"Rise(ns):", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, 175U, (const uint8_t *)"ZeroCnt:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, 210U, (const uint8_t *)"KEY2 capture", GRAYBLUE, WHITE, 16U, 0U);

  lcd_show_status_text("IDLE", GRAYBLUE);
  lcd_show_placeholder(105U);
  lcd_show_placeholder(140U);
  lcd_show_placeholder(175U);
}

static void lcd_show_measure_result(const pulse_measure_result_t *result, bool capture_ok)
{
  uint32_t amplitude_mv = 0U;
  uint32_t rise_time_10x_ns = 0U;

  if(result == NULL)
  {
    lcd_show_status_text("NO_DATA", RED);
    lcd_show_placeholder(105U);
    lcd_show_placeholder(140U);
    lcd_show_placeholder(175U);
    return;
  }

  lcd_clear_value_line(175U);
  LcdSprintf(150U, 175U, BLACK, WHITE, 24U, 0U, "%u", result->zero_count);

  if(!capture_ok)
  {
    if(result->zero_count > g_pulse_cfg.bad_zero_limit)
    {
      lcd_show_status_text("ZERO_ERR", RED);
    }
    else
    {
      lcd_show_status_text("BAD_FRAME", RED);
    }

    lcd_show_placeholder(105U);
    lcd_show_placeholder(140U);
    return;
  }

  amplitude_mv = scale_positive_float(result->amplitude_vpp, 1000.0f);
  lcd_clear_value_line(105U);
  LcdSprintf(150U,
             105U,
             BLUE,
             WHITE,
             24U,
             0U,
             "%lu.%03lu",
             (unsigned long)(amplitude_mv / 1000U),
             (unsigned long)(amplitude_mv % 1000U));

  if(result->rise_time_valid)
  {
    rise_time_10x_ns = scale_positive_float(result->rise_time_ns, 10.0f);
    lcd_clear_value_line(140U);
    LcdSprintf(150U,
               140U,
               BLUE,
               WHITE,
               24U,
               0U,
               "%lu.%01lu",
               (unsigned long)(rise_time_10x_ns / 10U),
               (unsigned long)(rise_time_10x_ns % 10U));
    lcd_show_status_text("OK", GREEN);
  }
  else
  {
    lcd_clear_value_line(140U);
    LCD_ShowString(150U, 140U, (const uint8_t *)"N/A", MAGENTA, WHITE, 24U, 0U);
    lcd_show_status_text("NO_EDGE", MAGENTA);
  }
}

static void capture_and_print(uint16_t cmd)
{
  HAL_StatusTypeDef status;
  bool measure_ok;
  uint32_t amplitude_mv = 0U;
  uint32_t rise_time_10x_ns = 0U;

  spi_reg_write(&cmd, 0xbb01, 1);
  HAL_Delay(ADC_CAPTURE_WAIT_MS);

  status = spi_reg_read_adc_channel_samples(adc_raw_buff, ADC_CAPTURE_SAMPLE_COUNT, 1);
  if(status != HAL_OK)
  {
    printf("CAPTURE_READ_FAIL,cmd=0x%04X\r\n", cmd);
    lcd_show_status_text("SPI_FAIL", RED);
    lcd_show_placeholder(105U);
    lcd_show_placeholder(140U);
    lcd_show_placeholder(175U);
    return;
  }

  memcpy(adc_work_buff, adc_raw_buff, sizeof(adc_work_buff));
  measure_ok = pulse_measure_analyze(adc_work_buff,
                                     ADC_CAPTURE_SAMPLE_COUNT,
                                     &g_pulse_cfg,
                                     &g_pulse_result);
  lcd_show_measure_result(&g_pulse_result, measure_ok);

  amplitude_mv = scale_positive_float(g_pulse_result.amplitude_vpp, 1000.0f);
  rise_time_10x_ns = scale_positive_float(g_pulse_result.rise_time_ns, 10.0f);
  printf("MEASURE,cmd=0x%04X,ok=%u,zero=%u,amp_mv=%lu,rise_x10ns=%lu\r\n",
         cmd,
         measure_ok ? 1U : 0U,
         g_pulse_result.zero_count,
         (unsigned long)amplitude_mv,
         (unsigned long)rise_time_10x_ns);
  printf("CAPTURE_BEGIN,cmd=0x%04X,samples=%u\r\n", cmd, ADC_CAPTURE_SAMPLE_COUNT);

  for (uint16_t i = 0; i < ADC_CAPTURE_SAMPLE_COUNT; i++)
  {
    printf("%u\r\n", adc_raw_buff[i]);
  }
}

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();
  lcd_draw_measure_ui();
  pulse_measure_get_default_config(&g_pulse_cfg);
  g_pulse_cfg.sample_rate_hz = ADC_SAMPLE_RATE_HZ;
  g_pulse_cfg.adc_full_scale_vpp = ADC_FULL_SCALE_VPP;
  g_pulse_cfg.adc_max_code = ADC_MAX_CODE;

  key_init();
	uint16_t cmd = 1;
	capture_and_print(cmd);
	HAL_Delay(500);
	capture_and_print(cmd);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    key_proc();
		if(key_press_flag & KEY2_PRESS_FLAG)
		{
			capture_and_print(cmd);
		}
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
