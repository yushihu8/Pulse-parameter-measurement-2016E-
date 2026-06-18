#include "key_app.h"
#include "stm32f4xx.h"                  // Device header

uint8_t key1;
uint8_t key1_last =1;
uint8_t key2;
uint8_t key2_last =1;
uint8_t key3;
uint8_t key3_last =1;
uint8_t key4;
uint8_t key4_last =1;

uint8_t key_press_flag;

void key_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_8;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_13;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	key1 = GPIO_PIN_SET;
	key2 = GPIO_PIN_SET;
	key3 = GPIO_PIN_SET;
	key4 = GPIO_PIN_SET;
	key1_last = GPIO_PIN_SET;
	key2_last = GPIO_PIN_SET;
	key3_last = GPIO_PIN_SET;
	key4_last = GPIO_PIN_SET;
	key_press_flag = 0;
}

void key_proc(void)
{
	static uint32_t key_tick;

	if(HAL_GetTick() - key_tick < 10U)
	{
		return;
	}
	key_tick = HAL_GetTick();

	key_press_flag = 0;

	key1 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);
	key2 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	key3 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_8);
	key4 = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);

	if(key1 == GPIO_PIN_RESET && key1_last == GPIO_PIN_SET)
	{
		key_press_flag |= KEY1_PRESS_FLAG;
	}
	if(key2 == GPIO_PIN_RESET && key2_last == GPIO_PIN_SET)
	{
		key_press_flag |= KEY2_PRESS_FLAG;
	}
	if(key3 == GPIO_PIN_RESET && key3_last == GPIO_PIN_SET)
	{
		key_press_flag |= KEY3_PRESS_FLAG;
		HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_8);
		
	}
	if(key4 == GPIO_PIN_RESET && key4_last == GPIO_PIN_SET)
	{
		key_press_flag |= KEY4_PRESS_FLAG;
		HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_9);		
	}
	key1_last = key1;
	key2_last = key2;
	key3_last = key3;
	key4_last = key4;
	
}

