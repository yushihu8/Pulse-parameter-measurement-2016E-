#include  "spi_reg.h"
#include "stdio.h"

static const uint16_t spi_dummy_buf[32] = {0};

static uint32_t spi_reg_calc_adc_rate_hz(uint16_t clk_tim_count)
{
  return (uint32_t)(FPGA_ADC_CLK_HZ / (2UL * ((uint32_t)clk_tim_count + 1UL)));
}

void spi_reg_write(const uint16_t *src,uint16_t addr,uint16_t size)
{
  uint16_t cmd[2] = {0};
  SPI_CS_L;
  if(size <= 1)
  {     
    cmd[0] = WRITE_SIGLE;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)src,1,1000);
  }else 
  {
    cmd[0] = WRITE_CURRENT;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)src,size,1000);
    cmd[0] = LAST_FRAME;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,1,1000);
  }
  SPI_CS_H;
}

void spi_reg_read(uint16_t *src,uint16_t addr,uint16_t size)
{
  uint16_t cmd[2] = {0};
  SPI_CS_L;
  if(size <= 1)
  {     
    const uint16_t dummy_word = 0;
    cmd[0] = READ_SIGLE;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    HAL_SPI_TransmitReceive(&M_SPI,(const uint8_t *)&dummy_word,(uint8_t *)src,1,1000);
  }else 
  {
    uint16_t remaining = size;
    cmd[0] = READ_CURRENT;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    while(remaining > 0)
    {
      uint16_t chunk = remaining;
      if(chunk > (uint16_t)(sizeof(spi_dummy_buf) / sizeof(spi_dummy_buf[0])))
        chunk = (uint16_t)(sizeof(spi_dummy_buf) / sizeof(spi_dummy_buf[0]));

      HAL_SPI_TransmitReceive(&M_SPI,
                              (const uint8_t *)spi_dummy_buf,
                              (uint8_t *)src,
                              chunk,
                              1000);
      src += chunk;
      remaining -= chunk;
    }
    cmd[0] = LAST_FRAME;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,1,1000);
  }
  SPI_CS_H;
}

HAL_StatusTypeDef spi_reg_set_adc_rate_hz(uint32_t adc_rate_hz, uint32_t *actual_rate_hz)
{
  uint32_t divider;
  uint16_t clk_tim_count;

  if((adc_rate_hz == 0UL) || (adc_rate_hz > FPGA_ADC_MAX_RATE_HZ))
    return HAL_ERROR;

  /* FPGA uses Fs = Fclk / (2 * (clk_tim_count + 1)). */
  divider = (FPGA_ADC_CLK_HZ + adc_rate_hz) / (2UL * adc_rate_hz);
  if((divider == 0UL) || (divider > 65536UL))
    return HAL_ERROR;

  clk_tim_count = (uint16_t)(divider - 1UL);
  spi_reg_write(&clk_tim_count, FPGA_ADC_RATE_ADDR, 1);

  if(actual_rate_hz != NULL)
    *actual_rate_hz = spi_reg_calc_adc_rate_hz(clk_tim_count);

  return HAL_OK;
}

HAL_StatusTypeDef spi_reg_wait_adc_capture_done(uint32_t timeout_ms)
{
  uint16_t status = 0;
  uint32_t start_tick = HAL_GetTick();

  while((HAL_GetTick() - start_tick) < timeout_ms)
  {
    spi_reg_read(&status, FPGA_ADC_STATUS_ADDR, 1);
    if((status & FPGA_ADC_STATUS_DONE) != 0U)
      return HAL_OK;
  }

  return HAL_TIMEOUT;
}

HAL_StatusTypeDef spi_reg_read_adc_channel_samples(uint16_t *dst, uint16_t sample_count, uint8_t channel_index)
{
  uint16_t i;
  uint16_t addr;

  if((dst == NULL) || (sample_count > 1024U) || (channel_index > 1U))
    return HAL_ERROR;

  for(i = 0; i < sample_count; i++)
  {
    addr = (uint16_t)(2U * i + channel_index);
    spi_reg_read(&dst[i], addr, 1);
  }

  return HAL_OK;
}

HAL_StatusTypeDef spi_reg_read_freq_duty_raw(uint32_t *period_samples,
                                             uint32_t *high_samples,
                                             uint32_t *low_samples,
                                             uint16_t *status)
{
  uint16_t period_h = 0U;
  uint16_t period_l = 0U;
  uint16_t high_h = 0U;
  uint16_t high_l = 0U;
  uint16_t low_h = 0U;
  uint16_t low_l = 0U;

  if((period_samples == NULL) || (high_samples == NULL) || (low_samples == NULL) || (status == NULL))
    return HAL_ERROR;

  spi_reg_read(status, FPGA_FREQ_STATUS_ADDR, 1);
  spi_reg_read(&period_h, FPGA_PERIOD_H_ADDR, 1);
  spi_reg_read(&period_l, FPGA_PERIOD_L_ADDR, 1);
  spi_reg_read(&high_h, FPGA_HIGH_H_ADDR, 1);
  spi_reg_read(&high_l, FPGA_HIGH_L_ADDR, 1);
  spi_reg_read(&low_h, FPGA_LOW_H_ADDR, 1);
  spi_reg_read(&low_l, FPGA_LOW_L_ADDR, 1);

  *period_samples = ((uint32_t)period_h << 16) | period_l;
  *high_samples = ((uint32_t)high_h << 16) | high_l;
  *low_samples = ((uint32_t)low_h << 16) | low_l;

  return HAL_OK;
}

