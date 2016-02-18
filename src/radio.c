#include "radio.h"
#include "main.h"
#include "mad.h"
#include "string.h"
#include "init_periph.h"
#include "wifi_module.h"
#include "wifi_uart.h"
#include "timebase.h"

//������� �������� � ������ (sizes of arrays in header)
struct buffer {
  unsigned char *start;
  unsigned long length;
};


extern int alloc_free;
extern unsigned char *alloc_ptr;
extern unsigned char alloc_buffer[ALLOC_SIZE];

extern abuf_status_type	abuf_status;
extern main_status_type main_status;
extern abuf_run_type 	abuf_run;

main_err_type err_type = ERR_NO;

uint8_t MP3Buffer[TMP_ABUF_SIZE];	//��������� �����, � ������� ���������� ������ ��� ������������ ������������� (tepm buffer storing data to be decoded later)
uint8_t main_abuf[MAIN_ABUF_SIZE]; //�������� ����� �����������, ���������� ��� ������������ ���� �� UART (main raw data buffer, data comes here from UART)

uint8_t	*mp3_start = &main_abuf[0];//����������� ������� ������ (buffer for audio data)

RCC_ClocksTypeDef RCC_Clocks;
uint16_t audio_freq = 0;//������� ������� ������������� (current audio sample rate)
uint16_t sample_fp = 0;//����� ������� �� ����� (number of semples per frame)

volatile uint8_t j_flag = 0;//���������������, ���� ��������� ���������� ������� �� ������ ������ (set is read pointer was set to beginning of buffer)
//������������, ����� ��������� ������ ������� �� ������ (cleard when write cointer was set to beginning of buffer)
volatile  uint8_t overrun_st = 0;

volatile  int32_t delta = 0;



//****************************************************************************************************

//CALLBACK ��� ������������� (mad.h)
//CALLBACK for audio decoder
enum mad_flow input(void *data, struct mad_stream *stream)	{
	struct buffer *buffer = data;
uint8_t	*destination;

uint16_t	num_get;
uint32_t	rest;

	destination = buffer->start; //���� ����� ������������ ������
	rest = (uint32_t)(stream->bufend - stream->next_frame);//(buffer->start + num + rest) - stream->next_frame(����� ������, ��������������� ���������)

	if( stream->buffer )
	{
		memmove((void*)stream->buffer,(const void*)stream->next_frame, rest);
		destination = destination + rest;//��������������� ����� ��� � ������, ����� ����� ���
	}

	num_get = TMP_ABUF_SIZE - rest;//������� ���� �������� �������� � �����

	if (num_get == 0)//����� �������� ������ ��� ������
	  {
	    destination = stream->next_frame;//������� ���� �� ������
	    num_get = 100;
	  }
	read_data(destination, num_get);//��������� ������ �� ��������� ����������� �� ���������

	mad_stream_buffer( stream, buffer->start, TMP_ABUF_SIZE );//��������������� stream->buffer, stream->bufend, stream->next_frame

	if (TB_Counter_Rx_Elapsed() == true)//�������� �� timeout
	  {
	    asm("nop");
	    //���� ������� �� MAD_FLOW_CONTINUE, �� ������������� �����������
	    return MAD_FLOW_STOP;
	  }

return MAD_FLOW_CONTINUE;
} // MP3InputStream





//������������ len ������ �� ������ �������� ����������� � dest, � ��� �� ��������� ��������� �������� �����������
//copy "len" bytes from raw audio data buffer to "dest" and controlling uasge of main audio buffer
void read_data(uint8_t	*dest, uint16_t len)
{
	static uint32_t readed = 0;//����� ��� ����������� ������ �� ������ ������
	uint16_t left;
	if ((readed + len) < MAIN_ABUF_SIZE)
	{
		memmove((void*)dest,(const void*)(mp3_start + readed), len);
		readed += len;
	}
	else
	{//� ������ ������������ ������,����� ������ ����� ����� �� ��� ������(��� ������ ���� ��� ����� ������)
		memmove((void*)dest,(const void*)(mp3_start + readed), (MAIN_ABUF_SIZE - readed));//�� ����� ������
		dest+= MAIN_ABUF_SIZE - readed;
		left = len - (MAIN_ABUF_SIZE - readed);
		memmove((void*)dest,(const void*)mp3_start, left);//�� ������ ������
		readed = left;
		j_flag = 1;

	}
	if (j_flag == 0)
	  {
	    delta = (int32_t)readed - ((uint32_t)MAIN_ABUF_SIZE - (int32_t)(DMA1_Stream5->NDTR));//NDTR - ����� ����������
	  }
	else
	  {
	    delta = (int32_t)readed + (int32_t)(DMA1_Stream5->NDTR);//NDTR - ����� ����������
	  }

	if ((overrun_st == 1) || (delta < 0))
	  {
	    STM_EVAL_LEDOn(LED6);//������� ����� ������(��������� ������ ����� ���������� ������)
	    if ((overrun_st == 1) && (delta > 2000)) {overrun_st = 0;}
	    disable_uart_req();//���������� ������
	  }
	else
	  {
	    STM_EVAL_LEDOff(LED6);
	    eneble_uart_req();
	  }


}



//���������� ��������� ���������� ��� ������ WIFI
//controlling conection to server using wifi module
void wifi_routine(void)
{
	while(!((main_status == MAIN_STAT_DATA) && ((TB_Counter_Rx_Elapsed() || (abuf_status != ABUF_NONE) )) ))
	{
	    //����������� �� ��� ���, ���� �� ������ ������ ����������� ��� ���������� �����
	    rx_handler();
	    wifi_handler();
	}
	if (TB_Counter_Rx_Elapsed() == true){err_type = ERR_TIMEOUT; STM_EVAL_LEDOn(LED3);return;}
	audio_freq = search_mp3_header();//����������� ������� ������������� �����
	if (audio_freq == 0){err_type = ERR_HEADER; STM_EVAL_LEDOn(LED3);return;}
}


void main_cycle(void)
{
	init_all_periph();//������������� ���������

	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

	while(1)
	  {
	    wifi_routine();

	    if (err_type != ERR_NO) //���-�� ����� �����������, ������������
	    {
	        asm("nop");
	        //clear_buffer();
	        //wifi_reboot_cmd();
	        STM_EVAL_LEDOn(LED3);
	        delay_ms(1000);
	        NVIC_SystemReset();

	    }
	    else //��� ���� ���������, WIFI ��� �������� ������
	    {
	        main_status = MAIN_STAT_PLAY;
	        eneble_uart_req();

	    	WavePlayBack((uint32_t)audio_freq,sample_fp);//������������� ������
	    	start_decode();//������ �������� mp3
	    	//����� ������� ������������� ��������� �� ������� �� start_decode, �� ��� ��� ���� �� �������� ������
	    	//��� �������� ������ � ������� ����� ������������ ������� mad_flow input (radio.c)

	    	err_type = ERR_TIMEOUT;
	    	STM_EVAL_LEDOn(LED3);
	    	Set_TB_Counter_Rx(TIMEOUT);//����� ������ �� ��������� ������� overrun
	    	DisableCodec();
	    	delay_ms(1000);
	    	STM_EVAL_LEDOff(LED3);
	    	err_type = ERR_NO;
	    	//clear_buffer();
	    	//wifi_reboot_cmd();
	    	NVIC_SystemReset();
	    }
	  }






}



void start_decode(void)
{
	  alloc_free=ALLOC_SIZE;
	  alloc_ptr=alloc_buffer;
	  decode(&MP3Buffer[0],MAIN_ABUF_SIZE);
}


//������� ����������, ��� ����, ����� ����� ��������� �� ������ ������ ������
//cleaniing audio buffer (to protect finding old data during header search)
void clear_buffer(void)
{
  uint16_t i;
  for (i=0;i<4000;i++){main_abuf[i] = 0;}
}

//���������� ����� ��������� ������ mp3 � �� ���� ���������� ������� ������������� ����� � ����� ������� �� �����
//try to find mp3 header and calculate sample rate and nubmer samples per frame
uint16_t search_mp3_header(void)
{
  uint16_t pos = 0;
  uint16_t pos2 = 0;//�������������� �������
  uint8_t a =0;
  uint8_t b =0;
  uint8_t c =0;

  uint8_t b2 =0;//��� ����, ����� ���������, ��� pos2 - ������������� ���������
  uint8_t c2 =0;
  uint8_t found = 0;// 1- ��������� 1 ������, ��������, ������

  uint16_t freq = 0;
  uint8_t mpeg_m = 0;//������� ����� ������ mpeg
  sample_fp = 0;

  while (pos < 4000)//4000 - � ����� ����� ���� ����������� ������ ���� ��������� �������
  {
	  a = main_abuf[pos];
	  b = main_abuf[pos+1];
	  c = main_abuf[pos+2];

	  if ((( a & 0xff) == 0xff) && (( b & 0xe2) == 0xe2) && ((b & 0x04) == 0))//��������� � ������ ���������
	  {
		  if (found == 0)
		  {
			  pos2 = pos;
			  found = 1;//����� ����� �������� ������ ��������� ���������
			  b2 = b;
			  c2 = c;
		  }
		  else
		  {
			 //����� ��������� ��� ����������
			  if ((c2 == c) && (b2 == b))
			  {
				  //pos2 - ������������� ���������

				  if ((b & 16) == 0){mpeg_m = 25;} else//���������� ��� mpeg
				  {
					  if ((b & 8) == 0){mpeg_m = 2;} else {mpeg_m = 1;}
				  }

				  if (mpeg_m == 1)
				  {
					  sample_fp = 1152;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 44100; break;}
					  	case 1: {freq = 48000; break;}
					  	case 2: {freq = 32000; break;}
					  }
				  }
				  if (mpeg_m == 2)
				  {
					  sample_fp = 576;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 22050; break;}
					  	case 1: {freq = 24000; break;}
					  	case 2: {freq = 16000; break;}
					  }
				  }

				  if (mpeg_m == 25)
				  {
					  sample_fp = 576;
					  switch ((c>>2) & 0x03)
					  {
					  	case 0: {freq = 11025; break;}
					  	case 1: {freq = 12000; break;}
					  	case 2: {freq = 8000; break;}
					  }
				  }

				  for (pos = 0;pos< pos2;pos++) {main_abuf[pos] = 0;}//�������� ����� ������ �� 1 ���������
				  return freq;
			  }
			  else
			  {
				  //pos2 - �� ���������
				  pos = pos2+1;
				  found = 0;
			  }
		  }

	  }//end if
	  pos++;

  }//end while

  //���� ������ ����, �� ��������� �� �������, ������
  //header not found - error
  return 0;
}

