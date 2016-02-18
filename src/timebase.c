/*
* ������ � ��������� ��������
* ����� ��������� ���������� ���������� �������
* ���������� ��������
*/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "timebase.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint16_t TB_counter_rx=0;
static uint16_t TB_counter_status=0;//��� ������� 5 ������
static uint16_t TB_counter_rx_end=0;//��� ����������� ����� ��������

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  if (TB_counter_rx!=0){TB_counter_rx--;}
  if (TB_counter_status!=0){TB_counter_status--;}
  if (TB_counter_rx_end!=0){TB_counter_rx_end--;}
}


/*
* �������� ��������� �������� ������
* ���������� TRUE, ���� ������� ������ 0
*/
bool TB_Counter_Rx_Elapsed(void)
{
  if (TB_counter_rx==0)
  {
    return true;
  }
  else 
  {
    return false;
  }
}

/*
* �������� ��������� �������� ������� wifi
* ���������� TRUE, ���� ������� ������ 0
*/
bool TB_Counter_Status_Elapsed(void)
{
  if (TB_counter_status==0)
  {
    return true;
  }
  else 
  {
    return false;
  }
}


bool TB_Counter_Rx_End_Elapsed(void)
{
  if (TB_counter_rx_end==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}


//********************************************************************


/*
* ��������� �������� ���������� �� �������� � value ��
*/
void Set_TB_Counter_Rx(uint16_t value)
{
  TB_counter_rx=value;
}

/*
* ��������� �������� ������� wifi �� �������� � value ��
*/
void Set_TB_Counter_Status(uint16_t value)
{
  TB_counter_status=value;
}


void Set_TB_Counter_Rx_End(uint16_t value)
{
  TB_counter_rx_end=value;
}
