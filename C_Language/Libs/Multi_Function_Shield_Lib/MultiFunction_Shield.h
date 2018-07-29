/*
 * MultiFunction_Shield.h file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 
//=====================================================================================================================================================
// Объявляем стандартные используемые хидеры
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//#include <avr/eeprom.h>
//#include <util/atomic.h>
//#include <avr/pgmspace.h>

//=====================================================================================================================================================
// Объявляем дефайны
#define baudrate 9600L
#define bauddivider (F_CPU/(16*baudrate)-1)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)

#define OUTS_DDR_0_7 		DDRD
#define OUTS_PORT_0_7  		PORTD
#define OUTS_DDR_8_13 		DDRB
#define OUTS_PORT_8_13 		PORTB
#define ANALOG_PORT			PORTC
#define ANALOG_PINS			PINC

#define BUTTON_1_PIN		PC1
#define BUTTON_2_PIN		PC2
#define BUTTON_3_PIN		PC3
#define BUTTON_1			(ANALOG_PINS & _BV(BUTTON_1_PIN))
#define BUTTON_2			(ANALOG_PINS & _BV(BUTTON_2_PIN))
#define BUTTON_3			(ANALOG_PINS & _BV(BUTTON_3_PIN))

#define LED_1				PB5
#define LED_2				PB4
#define LED_3				PB3
#define LED_4				PB2

#define BUZZER				PD3
#define BUZZER_ON()			OUTS_PORT_0_7 &= ~_BV(BUZZER)
#define BUZZER_OFF()		OUTS_PORT_0_7 |= _BV(BUZZER)

#define SPI_DATA_PIN  		PB0
#define SPI_CLK_PIN  		PD7
#define SPI_LATCH_PIN  		PD4

#define	SPI_DATA_HIGH()		(PORTB |= _BV(SPI_DATA_PIN))
#define	SPI_DATA_LOW()		(PORTB &=~_BV(SPI_DATA_PIN))
#define	SPI_CLK_HIGH()		(PORTD |= _BV(SPI_CLK_PIN))
#define	SPI_CLK_LOW()		(PORTD &=~_BV(SPI_CLK_PIN))
#define	SPI_LATCH_HIGH()	(PORTD |= _BV(SPI_LATCH_PIN))
#define	SPI_LATCH_LOW()		(PORTD &=~_BV(SPI_LATCH_PIN))

#define TSOP_PIN			(PIND & _BV(PD2))
#define BIT_DELAY			1778
#define START_DELAY			(BIT_DELAY*3/4)
#define CODE_LEN			13								// количество принимаемых битов кода
//#define USE_INTERRUPT_4_TSOP								// Способ приема кода RC5. Если закоментировано - поллинг (обработку кнопок и смену режимов придется обрабатывать в прерывании таймера)

#define ONE_WIRE_PIN		PC4
#define ONE_WIRE_PIN_LOW()	(DDRC |=_BV(ONE_WIRE_PIN))
#define ONE_WIRE_PIN_HIGH()	(DDRC &=~_BV(ONE_WIRE_PIN))
#define ONE_WIRE_LINE		(ANALOG_PINS & _BV(ONE_WIRE_PIN))

//=====================================================================================================================================================
// Объявляем глобальные переменные БЕЗ ПРИСВАИВАНИЯ ЗНАЧЕНИЙ !!!
extern volatile	uint16_t temperature_delay;					// Задержка до следующего считывания температуры с датчика DS18B20
extern volatile	uint16_t led_delay;							// Задежка до изменения состояние светодиодов
extern volatile	uint8_t  digit_counter;						// Установим счетчик разрядов на 1 знакоместо
extern volatile	uint8_t  adc_result;						// Результат преобразования ADC0
extern volatile	uint8_t  adc_delay;							// Задержка до следующего получения показаний АЦП
extern volatile	uint8_t  rc5_delay;							// Задержка до следующего получения кода RC5

#ifdef	USE_INTERRUPT_4_TSOP								// Если используем внешнее прерывание INT0 для получения кода RC5
extern volatile	uint16_t rc5_code;							// Заводим глобальную переменную 

#else														// Если не используем внешнее прерывание INT0 для получения кода RC5
extern volatile	uint8_t  key_delay;							// Опрос кнопок придется производить в прерывании по таймеру
extern volatile	uint8_t  mode;								// Смену режимов придеться производить в прерывании по таймеру
#endif
//=====================================================================================================================================================
// Обьявляем глобальные прототипы функций
void Init(void);
void Buzer_Beep(void);										// Вызов 1 короткого звукового сигнала
void Send_Byte(uint8_t byte);								// Отправка одного байта по UART
void Shield_display_Err(void);								// Вывод признака ошибки на индикатор
void Shield_display_value(void);							// Вывод значения на индикатор
void Shield_set_display_value(uint16_t value);				// Установка значения для вывода на индикатор
void Send_1_wire_byte(uint8_t);								// Отправка 1 байта по шине 1-wire
uint8_t Read_Temperature(void);								// Чтение температуры
uint8_t Reset_1_wire(void);									// Инициализации обмена (Reset-Presence)

#ifndef	USE_INTERRUPT_4_TSOP								// Если не используем внешнее прерывание INT0 для получения кода RC5
uint16_t Get_RC5_code(void);								// Обьявляем функцию получения кода RC5
#endif

