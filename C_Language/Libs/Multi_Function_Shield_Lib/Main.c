/*
 * Example Main.c file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 

//#include <avr/io.h>
//#include <util/delay.h>
//#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <util/atomic.h>
//#include <avr/pgmspace.h>

#include "MultiFunction_Shield.h"
#include "tick.h"

//=====================================================================================================================================================
// ��������� ���������� ���������� � �������
// ��� ��� � ������������ ���������� - volatile
// ��� ��� ������������� ������ � 1 ������� - static

uint8_t EEMEM eeprom_mode = 1;                                  // ��������� ����� ������ ������. ������������ ��� ������ ������ �������
//=====================================================================================================================================================
// ��������� ��������� �������

void Blink_Led_4(void);
void Switch_Shield_Mode(void);
//=====================================================================================================================================================
int main(void)
{
	Init_Multi_Function_Shield();								// ������������� ������ � ��������� Multi Function Shield
    t0_init();                                                  // ������������� ��������� ������ 
        
//    mode = 1;
	mode = eeprom_read_byte(&eeprom_mode);                      // ��������������� �� ������ ���������� ����� ������
    uint8_t old_mode = mode;
	Switch_Mode();                                              // � �������� ��������������� ���������

    sei();
 
//	������� ���� =======================================================================================================================================
	while (1)
	{
        t0_update();                                            // ��������� �������� ������� �������� �����
        Blink_Led_4();                                          // ������ �����������
        Key_Press();                                            // ���������� ������
        if (mode != old_mode)                                   // ���� ����� ���������
        {
            old_mode = mode;                                    // ������� ���������� �����
            eeprom_update_byte(&eeprom_mode, mode);             // �������� ����� ����� ������
        }
        Switch_Shield_Mode();                                   // ������������� �������� ��� ������ �� ��������� �������� �������� ������ ������
        Shield_display_value();								    // ����� �������� �� ���������
        Buzer_OFF();                                            // ��������� �������, ���� � �������, � ����� �������� ��������
    }
}
//======================================================================================================================================================
void Switch_Shield_Mode(void)                                   // �������� ������� ��������� �������� ��� ������ �� ��������� �������� �������� ������ ������
{        
	switch(mode)
	{
		case 1:                                                 // ����� ����������� 0 ������ ���
		{		    																	 
			if (t_ms - adc_delay + 0xFF00 >= 0xFF00)            // ���� ������ ����� �������� ��������� ���	
            {	
				asm("sleep");								    // sleep and start new conversion
				Shield_set_display_value(adc_result);		    // set new ADC result for indication
				adc_delay += 500;						        // set new conversion delay
			}
			break;
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
		case 2:                                                 // ����� ����������� ��������� ���� RC5
		{       
            #ifdef	USE_INTERRUPT_4_TSOP					    // ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
            if (t_ms - rc5_delay + 0xFF00 >= 0xFF00)            // ���� ������ ����� ��������� ������� ���������					
            {               
                GIFR = 1<<INTF0;								// clear External Interrupt Flag 0
	            GICR = 1<<INT0;									// external Interrupt Request 0 Enable
				rc5_delay += 250;                               // ��������� �������� �� ���������� ���������� �������� ����������
            }					    			
			#else                                               // ���� �� ���������� ������� ���������� INT0 ��� ��������� ���� RC5            			
            uint16_t rc5_code = 0;                              // ������� ���������� ��� ������ ���� RC5                              
            if (t_ms - rc5_delay + 0xFF00 >= 0xFF00)            // ���� ��� ������� �� ��������� ���������� ���� RC5
            {                               
                rc5_code = Get_RC5_code();                      // ���������� ������� ��� ��������� ���� RC5	
            }
            #endif
								
			if (rc5_code != 0)                                  // ���� ���� ����� �������� ���
			{
				Shield_set_display_value(rc5_code & 0x07FF);    // ������� �� ���������, �������� 2-� ��������� ��� � toggle-���
				Send_Byte((rc5_code>>8) & 0x07);			    // ������� � UART, �������� 2-� ��������� ��� � toggle-���
				Send_Byte(rc5_code);
				rc5_code = 0;
				Buzer_Beep(SHORT_BEEP);
				rc5_delay += 250;                               // ��������� �������� �� ���������� ���������� ���������� ���������� ���� RC5
			}
			break;
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------
		case 3:                                                 // ����� ����������� ���������� � ������� DS18B20
		{     
			if (t_ms - temperature_delay + 0xFF00 >= 0xFF00)    // ���� ������ ����� �������� ��������� �����������/������� �������������� ��������
            {
			    static uint8_t ds18b20_state = 0;
            	uint8_t one_wire_state = 0;
                				
				if (ds18b20_state == 0)                         // ���� ��� ���� �������� ������� �� ������ ��������������
				{							
					one_wire_state = Reset_1_wire();		    // ������������� ������ (Reset-Presence)			
					if (one_wire_state == 1)                    // ���� ������ �����
					{
						Send_1_wire_byte(SKIP_ROM);				// ���������� ������ ������
						Send_1_wire_byte(START_CONVERSION);		// ������ ��������������	
						temperature_delay += 750;               // ������������� �������� �� ����� ��������������
						ds18b20_state = 1;                      // ��������� ������� � ���� ��������� ���������� ��������������
					}
					else                                        // ���� ������ �� ��������
					{
						Shield_display_Err();                   // ������� ����� ������
						Buzer_Beep(SHORT_BEEP);
						temperature_delay += 2000;              // ������������� �������� �� ���������� ���������� ��������� �����������
						ds18b20_state = 0;
					}
				}					
				else if (ds18b20_state == 1)                    // ���� ��� ���� ��������� ��������� ��������� ��������������	
				{			
					one_wire_state = Reset_1_wire();		    // ��������� ������������� ������ (Reset-Presence)
					if (one_wire_state == 1)                    // ���� ������ �����
					{
						Send_1_wire_byte(SKIP_ROM);				// ���������� ������ ������
						Send_1_wire_byte(READ_TEMPERATURE);		// ����� ������ �������� �����������		
						Shield_set_display_value(Read_Temperature());// ������, � ����� ������������� �������� ��� ������ �� ���������
					}
					else                                        // ���� ������ �� ��������
					{
						Shield_display_Err();                   // ������� ����� ������
						Buzer_Beep(SHORT_BEEP);
					}
					temperature_delay += 2000;                  // ������������� �������� �� ���������� ���������� ��������� �����������
					ds18b20_state = 0;                          // ���������� ������� � ���� �������� ������� �� ������ ��������������
				}
			}
			break;
		}
		default: mode = 1;
    }							
}
//=====================================================================================================================================================
void Blink_Led_4(void)
{
    static uint16_t led_4_delay = 0;                            // ���� �������� ���������� ������, �� ���������� ���������������� ���: next_blink = t0_ctr, ����� ������� ��������� ��� ������ �� �������
        
    if (t_ms - led_4_delay + 0xFF00 >= 0xFF00)
    {
		led_4_delay += 500;
        LED_4_TOGLE();
    }    
}
