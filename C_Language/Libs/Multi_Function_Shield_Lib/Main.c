/*
 * Example Main.c file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 

#include "MultiFunction_Shield.h"
//=====================================================================================================================================================
// ��������� ���������� ���������� � �������
// ��� ��� � ������������ ���������� - volatile
// ��� ��� ������������� ������ � 1 ������� - static

//=====================================================================================================================================================
// ��������� ��������� �������

//=====================================================================================================================================================
int main(void)
{
	#ifdef	USE_INTERRUPT_4_TSOP							// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
	uint8_t	mode = 0;
	#endif	
	Init();													// ������������� ������ � ���������
	sei();
//	������� ���� =======================================================================================================================================
	while (1)
	{
	#ifdef	USE_INTERRUPT_4_TSOP							// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
		if (BUTTON_1 == 0)									// ����� ������ ����� ����������� ����� � �������� ����� ���������
		{
			OUTS_PORT_8_13 |= _BV(LED_4)|_BV(LED_3);
			OUTS_PORT_8_13 &= ~_BV(LED_2);
			Buzer_Beep();
			mode = 0;
		}
		else if (BUTTON_2 == 0)
		{
			OUTS_PORT_8_13 |= _BV(LED_4)|_BV(LED_2);
			OUTS_PORT_8_13 &= ~_BV(LED_3);
			Buzer_Beep();
			mode = 1;
			Shield_set_display_value(0);
		}
		else if (BUTTON_3 == 0)
		{		
			OUTS_PORT_8_13 |= _BV(LED_3)|_BV(LED_2);
			OUTS_PORT_8_13 &= ~_BV(LED_4);
			Buzer_Beep();
			mode = 2;
		}
	#endif
		if (mode == 0)
		{
			if (adc_delay == 0)								// ���� ������ ����� �������� ��������� ���																		 
			{	
				asm("sleep");								// Sleep and start new conversion
				Shield_set_display_value(adc_result);		// Set new ADC result for indication
				adc_delay--;								// Set new conversion delay
			}
		}		
		else if (mode == 1)
		{
			#ifndef	USE_INTERRUPT_4_TSOP					// ���� �� ���������� ������� ���������� INT0 ��� ��������� ���� RC5			
			uint16_t rc5_code = 0;			
			if (rc5_delay == 0) rc5_code = Get_RC5_code();	// � ��� ������� �� ��������� ���������� ����, ���������� ������� ��� ��������� ���� RC5	
			#endif
									
			if (rc5_code != 0)
			{
				Shield_set_display_value(rc5_code & 0x07FF);// ������� �� ���������, �������� 2-� ��������� ��� � toggle-���
				Send_Byte((rc5_code>>8) & 0x07);			// ������� � UART, �������� 2-� ��������� ��� � toggle-���
				Send_Byte(rc5_code);
				rc5_code = 0;
				Buzer_Beep();
			}
		}
		else if (mode == 2)
		{
			if (temperature_delay == 0)						// ���� ������ ����� �������� ��������� �����������
			{
				uint8_t one_wire_state = 0;	
				cli();										// ��������� ���������� ���� �������� ��������
				one_wire_state = Reset_1_wire();			// ������������� ������ (Reset-Presence)			
				if (one_wire_state == 1)
				{
					Send_1_wire_byte(0xCC);					// ������� "skip ROM"
					Send_1_wire_byte(0x44);					// ������ ��������������	
					sei();									// �������� �� �������������� ����� ������� � �� �������, �������� ����������
					_delay_ms(750);							// �������� �� ����� ��������������
					cli();									// ��������� ���������� ���� �������� ��������
					one_wire_state = Reset_1_wire();		// ��������� ������������� ������ (Reset-Presence)
					if (one_wire_state == 1)
					{
						Send_1_wire_byte(0xCC);				// ��������� ������� "skip ROM"
						Send_1_wire_byte(0xBE);				// ����� ������ �������� �����������		
						Shield_set_display_value(Read_Temperature());// ������ � ����� ����������� ��� ������ �� ���������
						temperature_delay = 1000;			// ������������� �������� �� ���������� ���������� ��������� �����������
					}
					else Shield_display_Err();
				}
				else Shield_display_Err();
				sei();
			}
		}							
	}
}
//=====================================================================================================================================================
