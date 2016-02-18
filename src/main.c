//by iliasam(citizen) 07.10.2012
//CoIDE project

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "radio.h"
#include "mad.h"


//IMPORTANT - STACK_SIZE = 0x00001000
//�����, ����� STACK_SIZE ��� ����� 0x00001000

//LED4 - ���������� � �������� ����������� (connected to server)
//LED5 - ������ ��� ��������������� ����� (blinks at play process)
//LED6 - OVERRUN(����� ����������) (flow is not stable)
//LED3 - ��� ������� (errors found)

int main(void)
{ 
  /* Initialize LEDS */
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);
  STM_EVAL_LEDInit(LED5);
  STM_EVAL_LEDInit(LED6);
 
  STM_EVAL_LEDOn(LED3);
  delay_ms(200);
  STM_EVAL_LEDOff(LED3);

  main_cycle();
 while(1);
}


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
