#include  "spi_reg.h"
#include "stdio.h"

static const uint16_t spi_dummy_buf[32] = {0};

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



