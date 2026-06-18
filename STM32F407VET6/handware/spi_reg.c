#include  "spi_reg.h"
#include "stdio.h"
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
    cmd[0] = READ_SIGLE;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    HAL_SPI_Receive(&M_SPI,(uint8_t *)src,1,1000);
  }else 
  {
    cmd[0] = READ_CURRENT;
    cmd[1] = addr;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,2,1000);
    HAL_SPI_Receive(&M_SPI,(uint8_t *)src,size,1000);
    cmd[0] = LAST_FRAME;
    HAL_SPI_Transmit(&M_SPI,(uint8_t *)cmd,1,1000);
  }
  SPI_CS_H;
}





