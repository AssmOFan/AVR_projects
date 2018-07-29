/*
 * Example Main.c file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 

#include "MultiFunction_Shield.h"
//=====================================================================================================================================================
// Объявляем глобальные переменные и дефайны
// Все что в обработчиках прерываний - volatile
// Все что используеться только в 1 функции - static

//=====================================================================================================================================================
// Обьявляем прототипы функций

//=====================================================================================================================================================
int main(void)
{
	#ifdef	USE_INTERRUPT_4_TSOP							// Если используем внешнее прерывание INT0 для получения кода RC5
	uint8_t	mode = 0;
	#endif	
	Init();													// Инициализация портов и периферии
	sei();
//	Главный цикл =======================================================================================================================================
	while (1)
	{
	#ifdef	USE_INTERRUPT_4_TSOP							// Если используем внешнее прерывание INT0 для получения кода RC5
		if (BUTTON_1 == 0)									// Опрос кнопок можно производить прямо в основном цикле программы
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
			if (adc_delay == 0)								// Если пришло время обновить показания АЦП																		 
			{	
				asm("sleep");								// Sleep and start new conversion
				Shield_set_display_value(adc_result);		// Set new ADC result for indication
				adc_delay--;								// Set new conversion delay
			}
		}		
		else if (mode == 1)
		{
			#ifndef	USE_INTERRUPT_4_TSOP					// Если не используем внешнее прерывание INT0 для получения кода RC5			
			uint16_t rc5_code = 0;			
			if (rc5_delay == 0) rc5_code = Get_RC5_code();	// И нет запрета на повторное считывания кода, используем функцию для получения кода RC5	
			#endif
									
			if (rc5_code != 0)
			{
				Shield_set_display_value(rc5_code & 0x07FF);// Выводим на индикатор, маскируя 2-й стартовый бит и toggle-бит
				Send_Byte((rc5_code>>8) & 0x07);			// Выводим в UART, маскируя 2-й стартовый бит и toggle-бит
				Send_Byte(rc5_code);
				rc5_code = 0;
				Buzer_Beep();
			}
		}
		else if (mode == 2)
		{
			if (temperature_delay == 0)						// Если пришло время обновить показания температуры
			{
				uint8_t one_wire_state = 0;	
				cli();										// Запрещаем прерывания ради точности задержек
				one_wire_state = Reset_1_wire();			// Инициализация обмена (Reset-Presence)			
				if (one_wire_state == 1)
				{
					Send_1_wire_byte(0xCC);					// Команда "skip ROM"
					Send_1_wire_byte(0x44);					// Начать преобразование	
					sei();									// Задержка на преобразование очень длинная и не строгая, разрешим прерывания
					_delay_ms(750);							// Задержка на время преобразования
					cli();									// Запрещаем прерывания ради точности задержек
					one_wire_state = Reset_1_wire();		// Повторная инициализация обмена (Reset-Presence)
					if (one_wire_state == 1)
					{
						Send_1_wire_byte(0xCC);				// Повторная команда "skip ROM"
						Send_1_wire_byte(0xBE);				// Будем читать значение температуры		
						Shield_set_display_value(Read_Temperature());// Читаем и сразу преобразуем для вывода на индикатор
						temperature_delay = 1000;			// Устанавливаем задержку до следующего обновления показаний температуры
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
