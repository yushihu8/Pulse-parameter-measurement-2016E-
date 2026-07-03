#ifndef __SPI_REG_H__
#define __SPI_REG_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"  
#include "spi.h"            
/* Exported defines - 通信协议定义 --------------------------------------*/
/**
 * @brief  单次写操作命令码
 * @details WRITE_SIGLE = 0xAA01
 *          用于向外部芯片写入单个寄存器
 *          协议格式：[命令码][地址][数据]
 */
#define WRITE_SIGLE       0xaa01

/**
 * @brief  连续写操作命令码
 * @details WRITE_CURRENT = 0xAA02
 *          用于向外部芯片连续写入多个寄存器
 *          协议格式：[命令码][起始地址][数据1][数据2]...[数据N]
 */
#define WRITE_CURRENT     0xaa02

/**
 * @brief  单次读操作命令码
 * @details READ_SIGLE = 0xAA03
 *          用于从外部芯片读取单个寄存器
 *          协议格式：[命令码][地址] → [返回数据]
 */
#define READ_SIGLE        0xaa03

/**
 * @brief  连续读操作命令码
 * @details READ_CURRENT = 0xAA04
 *          用于从外部芯片连续读取多个寄存器
 *          协议格式：[命令码][起始地址] → [返回数据1][数据2]...[数据N]
 */
#define READ_CURRENT      0xaa04

/**
 * @brief  最后一帧标志
 * @details LAST_FRAME = 0xA5A5
 *          用于标识数据传输的最后一帧
 *          在批量传输时使用，便于接收方识别传输结束
 */
#define LAST_FRAME 0xa5a5

#define FPGA_ADC_RATE_ADDR       0xa010
#define FPGA_ADC_STATUS_ADDR     0xbb02
#define FPGA_ADC_CLK_HZ          65000000UL
#define FPGA_ADC_MAX_RATE_HZ     (FPGA_ADC_CLK_HZ / 2UL)
#define FPGA_ADC_STATUS_DONE     0x0001
#define FPGA_ADC_STATUS_BUSY     0x0002

/* Exported defines - 硬件配置 ----------------------------------------*/
/**
 * @brief  SPI外设句柄
 * @details M_SPI = hspi1
 *          使用STM32的SPI1外设进行通信
 *          硬件配置：
 *          - 主机模式
 *          - 16位数据格式
 *          - 1.5Mbps波特率
 *          - CPOL=0, CPHA=0模式
 */
#define M_SPI hspi1

/**
 * @brief  SPI片选引脚控制宏
 * @details SPI_CS_L: 设置片选为低电平（选中芯片）
 *          SPI_CS_H: 设置片选为高电平（取消选中）
 *          连接到STM32的PG10引脚
 */
#define SPI_CS_L HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET)
#define SPI_CS_H HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET)


void spi_reg_write(const uint16_t *src, uint16_t addr, uint16_t size);
void spi_reg_read(uint16_t *src, uint16_t addr, uint16_t size);
HAL_StatusTypeDef spi_reg_set_adc_rate_hz(uint32_t adc_rate_hz, uint32_t *actual_rate_hz);
HAL_StatusTypeDef spi_reg_wait_adc_capture_done(uint32_t timeout_ms);
HAL_StatusTypeDef spi_reg_read_adc_channel_samples(uint16_t *dst, uint16_t sample_count, uint8_t channel_index);

#endif /* __SPI_REG_H__ */
