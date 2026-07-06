#include "uart_app.h"




uint32_t rx_tick;
uint8_t rx_buff[128];
uint8_t rx_index;


int fputc(int ch,FILE * str)
{
	HAL_UART_Transmit(&huart3,(uint8_t *)&ch,1,10);
	return ch;
}
void 	HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance==USART1)
	{
		rx_index++;
		HAL_UART_Receive_IT(&huart3,&rx_buff[rx_index],1);
		rx_tick=uwTick;
	}
}

void uart_proc(void)
{
	if(rx_index==0)return;
	if(uwTick>= (rx_tick+100))
	{
		if(strncmp((char*)rx_buff,"lqb",sizeof("lqb"))==0)
		{
			printf("error");
		}
		else
		{
			//printf("%s",rx_buff);
			HAL_UART_Transmit(&huart3, rx_buff, rx_index, 1000);
		}
		rx_index=0;
		huart3.pRxBuffPtr = rx_buff;
		memset(rx_buff,0,128);
	}
}
















