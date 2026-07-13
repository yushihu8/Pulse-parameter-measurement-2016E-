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
typedef enum
{
  MEASURE_MODE_WAVE = 0,
  MEASURE_MODE_FREQ = 1
} measure_mode_t;

typedef enum
{
  WAVE_CHANNEL_1_NORMAL = 0,
  WAVE_CHANNEL_2_ATTEN = 1
} wave_channel_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CAPTURE_WAIT_MS 20U
#define ADC_CAPTURE_SAMPLE_COUNT 1024U
#define ADC_SAMPLE_RATE_HZ FPGA_ADC_SAMPLE_RATE_HZ
#define ADC_FULL_SCALE_VPP 4.0f
#define ADC_MAX_CODE 16383U
#define WAVE_CHANNEL_1_GAIN 1.0f
#define WAVE_CHANNEL_2_GAIN 3.1623f
#define WAVE_CHANNEL_SWITCH_TO_NORMAL_VPP 3.2f
#define WAVE_CHANNEL_BACK_TO_NORMAL_FRAMES 5U
#define WAVE_CHANNEL_CH2_HOLD_MS 1000U
#define LCD_AUTO_REFRESH_INTERVAL_MS 200U
#define LCD_VALUE_X 150U
#define LCD_STATUS_Y 56U
#define LCD_AMP_Y 84U
#define LCD_RISE_Y 112U
#define LCD_FREQ_Y 140U
#define LCD_DUTY_Y 168U
#define LCD_ZERO_Y 196U
#define LCD_NOTE_Y 220U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
static uint16_t adc_raw_buff[ADC_CAPTURE_SAMPLE_COUNT];
static uint16_t adc_work_buff[ADC_CAPTURE_SAMPLE_COUNT];
static pulse_measure_config_t g_pulse_cfg;
static pulse_measure_result_t g_pulse_result;
static pulse_measure_result_t g_last_valid_result;
static bool g_have_last_valid_result = false;
static uint16_t g_freq_status = 0U;
static uint32_t g_freq_period_samples = 0U;
static uint32_t g_freq_high_samples = 0U;
static uint32_t g_freq_low_samples = 0U;
static measure_mode_t g_measure_mode = MEASURE_MODE_WAVE;
static wave_channel_t g_wave_channel = WAVE_CHANNEL_1_NORMAL;
static uint8_t g_channel2_low_amp_frames = 0U;
static uint32_t g_wave_channel_switch_tick = 0U;

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
  LCD_Fill(LCD_VALUE_X, y, 315U, y + 24U, WHITE);
}

static void lcd_show_placeholder(uint16_t y)
{
  lcd_clear_value_line(y);
  LCD_ShowString(LCD_VALUE_X, y, (const uint8_t *)"--", RED, WHITE, 24U, 0U);
}

static void lcd_show_status_text(const char *text, uint16_t color)
{
  lcd_clear_value_line(LCD_STATUS_Y);
  LCD_ShowString(LCD_VALUE_X, LCD_STATUS_Y, (const uint8_t *)text, color, WHITE, 24U, 0U);
}

static void lcd_show_mode_text(const char *text, uint16_t color)
{
  LCD_Fill(170U, LCD_NOTE_Y, 315U, LCD_NOTE_Y + 18U, WHITE);
  LCD_ShowString(170U, LCD_NOTE_Y, (const uint8_t *)text, color, WHITE, 16U, 0U);
}

static void lcd_show_channel_text(const char *text, uint16_t color)
{
  LCD_Fill(206U, 32U, 315U, 44U, WHITE);
  LCD_ShowString(206U, 32U, (const uint8_t *)text, color, WHITE, 16U, 0U);
}

static void apply_wave_channel(void)
{
  if(g_wave_channel == WAVE_CHANNEL_1_NORMAL)
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
    g_pulse_cfg.adc_full_scale_vpp = ADC_FULL_SCALE_VPP * WAVE_CHANNEL_1_GAIN;
    g_channel2_low_amp_frames = 0U;
    g_wave_channel_switch_tick = HAL_GetTick();
    lcd_show_channel_text("CH1 0dB", BLUE);
  }
  else
  {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
    g_pulse_cfg.adc_full_scale_vpp = ADC_FULL_SCALE_VPP * WAVE_CHANNEL_2_GAIN;
    g_channel2_low_amp_frames = 0U;
    g_wave_channel_switch_tick = HAL_GetTick();
    lcd_show_channel_text("CH2 -10dB", MAGENTA);
  }
}

static bool adc_frame_hit_ceiling(const uint16_t *samples, uint16_t sample_count)
{
  if(samples == NULL)
  {
    return false;
  }

  for(uint16_t i = 0U; i < sample_count; i++)
  {
    if(samples[i] >= ADC_MAX_CODE)
    {
      return true;
    }
  }

  return false;
}

static bool wave_channel_auto_adjust(const uint16_t *samples,
                                     uint16_t sample_count,
                                     const pulse_measure_result_t *result,
                                     bool capture_ok)
{
  if((g_wave_channel == WAVE_CHANNEL_1_NORMAL) &&
     adc_frame_hit_ceiling(samples, sample_count))
  {
    g_wave_channel = WAVE_CHANNEL_2_ATTEN;
    apply_wave_channel();
    return true;
  }

  if((g_wave_channel == WAVE_CHANNEL_2_ATTEN) &&
     ((HAL_GetTick() - g_wave_channel_switch_tick) >= WAVE_CHANNEL_CH2_HOLD_MS))
  {
    if(capture_ok &&
       (result != NULL) &&
       result->rise_time_valid &&
       (result->amplitude_vpp < WAVE_CHANNEL_SWITCH_TO_NORMAL_VPP))
    {
      if(g_channel2_low_amp_frames < WAVE_CHANNEL_BACK_TO_NORMAL_FRAMES)
      {
        g_channel2_low_amp_frames++;
      }

      if(g_channel2_low_amp_frames >= WAVE_CHANNEL_BACK_TO_NORMAL_FRAMES)
      {
        g_wave_channel = WAVE_CHANNEL_1_NORMAL;
        apply_wave_channel();
        return true;
      }
    }
    else
    {
      g_channel2_low_amp_frames = 0U;
    }
  }

  return false;
}

static void lcd_show_freq_duty_result(uint16_t status,
                                      uint32_t test_edge_count,
                                      uint32_t gate_clk_count,
                                      uint32_t high_level_clk_count)
{
  uint32_t freq_10x_hz;
  uint32_t duty_10x_pct;
  float freq_hz;
  float duty_pct;

  if(((status & FPGA_FREQ_STATUS_VALID) == 0U) ||
     (test_edge_count == 0U) ||
     (gate_clk_count == 0U) ||
     (high_level_clk_count > gate_clk_count))
  {
    lcd_show_placeholder(LCD_FREQ_Y);
    lcd_show_placeholder(LCD_DUTY_Y);
    return;
  }

  freq_hz = ((float)test_edge_count * FPGA_FREQ_STD_CLK_HZ) / (float)gate_clk_count;
  duty_pct = ((float)high_level_clk_count * 100.0f) / (float)gate_clk_count;

  freq_10x_hz = scale_positive_float(freq_hz, 10.0f);
  duty_10x_pct = scale_positive_float(duty_pct, 10.0f);

  lcd_clear_value_line(LCD_FREQ_Y);
  if(freq_hz >= 1000.0f)
  {
    uint32_t freq_10x_khz = scale_positive_float(freq_hz / 1000.0f, 10.0f);
    LcdSprintf(LCD_VALUE_X,
               LCD_FREQ_Y,
               BLUE,
               WHITE,
               24U,
               0U,
               "%lu.%01lu k",
               (unsigned long)(freq_10x_khz / 10U),
               (unsigned long)(freq_10x_khz % 10U));
  }
  else
  {
    LcdSprintf(LCD_VALUE_X,
               LCD_FREQ_Y,
               BLUE,
               WHITE,
               24U,
               0U,
               "%lu.%01lu",
               (unsigned long)(freq_10x_hz / 10U),
               (unsigned long)(freq_10x_hz % 10U));
  }

  lcd_clear_value_line(LCD_DUTY_Y);
  LcdSprintf(LCD_VALUE_X,
             LCD_DUTY_Y,
             BLUE,
             WHITE,
             24U,
             0U,
             "%lu.%01lu",
             (unsigned long)(duty_10x_pct / 10U),
             (unsigned long)(duty_10x_pct % 10U));
}

static void lcd_draw_measure_ui(void)
{
  LCD_Fill(0U, 0U, 320U, 240U, WHITE);
  LCD_DrawRectangle(8U, 8U, 312U, 232U, BLUE);
  LCD_DrawLine(8U, 46U, 312U, 46U, LIGHTBLUE);

  LCD_ShowString(18U, 16U, (const uint8_t *)"Pulse Measure", BLUE, WHITE, 24U, 0U);
  LCD_ShowString(206U, 18U, (const uint8_t *)"KEY2:Dump", GRAYBLUE, WHITE, 16U, 0U);
  LCD_ShowString(20U, LCD_STATUS_Y, (const uint8_t *)"State:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_AMP_Y, (const uint8_t *)"Amp(Vpp):", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_RISE_Y, (const uint8_t *)"Rise(ns):", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_FREQ_Y, (const uint8_t *)"Freq:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_DUTY_Y, (const uint8_t *)"Duty:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_ZERO_Y, (const uint8_t *)"ZeroCnt:", BLACK, WHITE, 24U, 0U);
  LCD_ShowString(20U, LCD_NOTE_Y, (const uint8_t *)"KEY3 Mode:", GRAYBLUE, WHITE, 16U, 0U);

  lcd_show_status_text("IDLE", GRAYBLUE);
  lcd_show_placeholder(LCD_AMP_Y);
  lcd_show_placeholder(LCD_RISE_Y);
  lcd_show_placeholder(LCD_FREQ_Y);
  lcd_show_placeholder(LCD_DUTY_Y);
  lcd_show_placeholder(LCD_ZERO_Y);
  lcd_show_mode_text("Wave", BLUE);
  lcd_show_channel_text("CH1 0dB", BLUE);
}

static void lcd_show_measure_result(const pulse_measure_result_t *result, bool capture_ok)
{
  uint32_t amplitude_mv = 0U;
  uint32_t rise_time_10x_ns = 0U;

  if(result == NULL)
  {
    lcd_show_status_text("NO_DATA", RED);
    lcd_show_placeholder(LCD_AMP_Y);
    lcd_show_placeholder(LCD_RISE_Y);
    lcd_show_placeholder(LCD_ZERO_Y);
    return;
  }

  lcd_clear_value_line(LCD_ZERO_Y);
  LcdSprintf(LCD_VALUE_X, LCD_ZERO_Y, BLACK, WHITE, 24U, 0U, "%u", result->zero_count);

  if(!capture_ok)
  {
    if((g_pulse_cfg.bad_zero_limit != 0xFFFFU) &&
       (result->zero_count > g_pulse_cfg.bad_zero_limit))
    {
      lcd_show_status_text("ZERO_ERR", RED);
    }
    else
    {
      lcd_show_status_text("BAD_FRAME", RED);
    }

    lcd_show_placeholder(LCD_AMP_Y);
    lcd_show_placeholder(LCD_RISE_Y);
    return;
  }

  amplitude_mv = scale_positive_float(result->amplitude_vpp, 1000.0f);
  if(result->rise_time_valid)
  {
    lcd_clear_value_line(LCD_AMP_Y);
    LcdSprintf(LCD_VALUE_X,
               LCD_AMP_Y,
               BLUE,
               WHITE,
               24U,  
               0U,
               "%lu.%03lu",
               (unsigned long)(amplitude_mv / 1000U),
               (unsigned long)(amplitude_mv % 1000U));
    rise_time_10x_ns = scale_positive_float(result->rise_time_ns, 10.0f);
    lcd_clear_value_line(LCD_RISE_Y);
    LcdSprintf(LCD_VALUE_X,
               LCD_RISE_Y,
               BLUE,
               WHITE,
               24U,
               0U,
               "%lu.%01lu",
               (unsigned long)(rise_time_10x_ns / 10U),
               (unsigned long)(rise_time_10x_ns % 10U));
    g_last_valid_result = *result;
    g_have_last_valid_result = true;
    lcd_show_status_text("OK", GREEN);
  }
  else
  {
    if(g_have_last_valid_result)
    {
      lcd_show_status_text("HOLD", MAGENTA);
    }
    else
    {
      lcd_show_placeholder(LCD_AMP_Y);
      lcd_clear_value_line(LCD_RISE_Y);
      LCD_ShowString(LCD_VALUE_X, LCD_RISE_Y, (const uint8_t *)"N/A", MAGENTA, WHITE, 24U, 0U);
      lcd_show_status_text("NO_EDGE", MAGENTA);
    }
  }
}

static void capture_wave_measure(uint16_t cmd, bool dump_samples, bool allow_auto_range)
{
  HAL_StatusTypeDef status;
  bool measure_ok;
  uint8_t capture_rounds = allow_auto_range ? 2U : 1U;

  for(uint8_t round = 0U; round < capture_rounds; round++)
  {
    spi_reg_write(&cmd, 0xbb01, 1);
    HAL_Delay(ADC_CAPTURE_WAIT_MS);

    status = spi_reg_read_adc_channel_samples(adc_raw_buff, ADC_CAPTURE_SAMPLE_COUNT, 1);
    if(status != HAL_OK)
    {
      printf("CAPTURE_READ_FAIL,cmd=0x%04X\r\n", cmd);
      lcd_show_status_text("SPI_FAIL", RED);
      lcd_show_placeholder(LCD_AMP_Y);
      lcd_show_placeholder(LCD_RISE_Y);
      lcd_show_placeholder(LCD_FREQ_Y);
      lcd_show_placeholder(LCD_DUTY_Y);
      lcd_show_placeholder(LCD_ZERO_Y);
      return;
    }

    memcpy(adc_work_buff, adc_raw_buff, sizeof(adc_work_buff));
    measure_ok = pulse_measure_analyze(adc_work_buff,
                                       ADC_CAPTURE_SAMPLE_COUNT,
                                       &g_pulse_cfg,
                                       &g_pulse_result);
    lcd_show_measure_result(&g_pulse_result, measure_ok);
    lcd_show_placeholder(LCD_FREQ_Y);
    lcd_show_placeholder(LCD_DUTY_Y);

    if((!allow_auto_range) ||
       (!wave_channel_auto_adjust(adc_raw_buff,
                                  ADC_CAPTURE_SAMPLE_COUNT,
                                  &g_pulse_result,
                                  measure_ok)))
    {
      break;
    }
  }

  if(!dump_samples)
  {
    return;
  }

  printf("CAPTURE_BEGIN,cmd=0x%04X,samples=%u\r\n", cmd, ADC_CAPTURE_SAMPLE_COUNT);

  for (uint16_t i = 0; i < ADC_CAPTURE_SAMPLE_COUNT; i++)
  {
    printf("%u\r\n", adc_raw_buff[i]);
  }
}

static void capture_freq_measure(void)
{
  lcd_show_placeholder(LCD_AMP_Y);
  lcd_show_placeholder(LCD_RISE_Y);
  lcd_show_placeholder(LCD_ZERO_Y);

  if(spi_reg_read_freq_duty_raw(&g_freq_period_samples,
                                &g_freq_high_samples,
                                &g_freq_low_samples,
                                &g_freq_status) == HAL_OK)
  {
    lcd_show_freq_duty_result(g_freq_status,
                              g_freq_period_samples,
                              g_freq_high_samples,
                              g_freq_low_samples);
    if((g_freq_status & FPGA_FREQ_STATUS_VALID) != 0U)
      lcd_show_status_text("OK", GREEN);
    else
      lcd_show_status_text("NO_SIG", MAGENTA);
  }
  else
  {
    lcd_show_status_text("SPI_FAIL", RED);
    lcd_show_placeholder(LCD_FREQ_Y);
    lcd_show_placeholder(LCD_DUTY_Y);
  }
}

static void measure_mode_refresh(uint16_t cmd, bool dump_samples, bool allow_auto_range)
{
  if(g_measure_mode == MEASURE_MODE_WAVE)
  {
    lcd_show_mode_text("Wave", BLUE);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_2,GPIO_PIN_RESET);
    capture_wave_measure(cmd, dump_samples, allow_auto_range);
  }
  else
  {
    lcd_show_mode_text("Freq", GREEN);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_2,GPIO_PIN_SET);
    capture_freq_measure();
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
  apply_wave_channel();

  key_init();
	uint16_t cmd = 1;
	measure_mode_refresh(cmd, false, true);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		static uint32_t last_capture_tick = 0U;
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    key_proc();
		if(key_press_flag & KEY3_PRESS_FLAG)
		{
			if(g_measure_mode == MEASURE_MODE_WAVE)
				g_measure_mode = MEASURE_MODE_FREQ;
			else
				g_measure_mode = MEASURE_MODE_WAVE;

			measure_mode_refresh(cmd, false, true);
			last_capture_tick = HAL_GetTick();
		}
		if(key_press_flag & KEY4_PRESS_FLAG)
		{
			if(g_wave_channel == WAVE_CHANNEL_1_NORMAL)
				g_wave_channel = WAVE_CHANNEL_2_ATTEN;
			else
				g_wave_channel = WAVE_CHANNEL_1_NORMAL;

			apply_wave_channel();
			if(g_measure_mode == MEASURE_MODE_WAVE)
			{
				measure_mode_refresh(cmd, false, false);
				last_capture_tick = HAL_GetTick();
			}
		}
		if((HAL_GetTick() - last_capture_tick) >= LCD_AUTO_REFRESH_INTERVAL_MS)
		{
			measure_mode_refresh(cmd, false, true);
			last_capture_tick = HAL_GetTick();
		}
		if((g_measure_mode == MEASURE_MODE_WAVE) &&
		   (key_press_flag & KEY2_PRESS_FLAG))
		{
			measure_mode_refresh(cmd, true, false);
			last_capture_tick = HAL_GetTick();
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
