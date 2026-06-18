#ifndef _KEY_APP_H_
#define _KEY_APP_H_

#include "main.h"

/* Key press event flags */
#define KEY1_PRESS_FLAG 0x01U
#define KEY2_PRESS_FLAG 0x02U
#define KEY3_PRESS_FLAG 0x04U
#define KEY4_PRESS_FLAG 0x08U

extern uint8_t key_press_flag;

void key_init(void);
void key_proc(void);


#endif
