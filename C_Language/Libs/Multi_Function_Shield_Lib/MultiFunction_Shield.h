/*
 * MultiFunction_Shield.h file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 
//=====================================================================================================================================================
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//=====================================================================================================================================================
// Declare defines
//=====================================================================================================================================================
//#define USE_INTERRUPT_4_TSOP								// использовать прием кода RC5 в прерывании. Если закоментировано - опрос (поллинг) инфракрасного приемника производится в главном цикле программы

#define NUM_OF_MODES		3								// количество режимов в "Меню"
#define ADC_MODE			1
#define TSOP_MODE			2
#define DS18B20_MODE		3
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define baudrate 9600L
#define bauddivider (F_CPU/(16*baudrate)-1)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define OUTS_DDR_0_7 		DDRD
#define OUTS_PORT_0_7  		PORTD
#define OUTS_DDR_8_13 		DDRB
#define OUTS_PORT_8_13 		PORTB
#define ANALOG_PORT			PORTC
#define ANALOG_PINS			PINC
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define BUTTON_1_PIN		PC1
#define BUTTON_2_PIN		PC2
#define BUTTON_3_PIN		PC3
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define BUTTON_1			(ANALOG_PINS & _BV(BUTTON_1_PIN))
#define BUTTON_2			(ANALOG_PINS & _BV(BUTTON_2_PIN))
#define BUTTON_3			(ANALOG_PINS & _BV(BUTTON_3_PIN))
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define LED_1				PB5
#define LED_2				PB4
#define LED_3				PB3
#define LED_4				PB2
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define LED_4_ON()			OUTS_PORT_8_13 &= ~_BV(LED_4)
#define LED_4_OFF()		    OUTS_PORT_8_13 |= _BV(LED_4)
#define LED_4_TOGLE()       OUTS_PORT_8_13 ^= _BV(LED_4)
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define BUZZER				PD3
#define BUZZER_ON()			OUTS_PORT_0_7 &= ~_BV(BUZZER)
#define BUZZER_OFF()		OUTS_PORT_0_7 |= _BV(BUZZER)
#define BUZZER_IS_ON        !(OUTS_PORT_0_7 & _BV(BUZZER))
#define SHORT_BEEP          50                              // ms
#define LONG_BEEP           500                             // ms
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define SPI_DATA_PIN  		PB0
#define SPI_CLK_PIN  		PD7
#define SPI_LATCH_PIN  		PD4
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define	SPI_DATA_HIGH()		(PORTB |= _BV(SPI_DATA_PIN))
#define	SPI_DATA_LOW()		(PORTB &=~_BV(SPI_DATA_PIN))
#define	SPI_CLK_HIGH()		(PORTD |= _BV(SPI_CLK_PIN))
#define	SPI_CLK_LOW()		(PORTD &=~_BV(SPI_CLK_PIN))
#define	SPI_LATCH_HIGH()	(PORTD |= _BV(SPI_LATCH_PIN))
#define	SPI_LATCH_LOW()		(PORTD &=~_BV(SPI_LATCH_PIN))
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define ONE_WIRE_PIN		PC4
#define ONE_WIRE_PIN_LOW()	(DDRC |=_BV(ONE_WIRE_PIN))
#define ONE_WIRE_PIN_HIGH()	(DDRC &=~_BV(ONE_WIRE_PIN))
#define ONE_WIRE_LINE		(ANALOG_PINS & _BV(ONE_WIRE_PIN))
#define SKIP_ROM            0xCC
#define START_CONVERSION    0x44
#define READ_TEMPERATURE    0xBE
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#define TSOP_PIN			(PIND & _BV(PD2))
#define BIT_DELAY			1778							// задержка между битами кода RC5
#define START_DELAY			(BIT_DELAY*3/4)					// задержка между окончанием старт-бита и серединой 1 бита кода RC5
#define CODE_LEN			13								// количество принимаемых битов кода RC5
//=====================================================================================================================================================
// Объявляем глобальные (с квалификатором extern) переменные БЕЗ ПРИСВАИВАНИЯ ЗНАЧЕНИЙ !!!

extern volatile	uint8_t adc_result;
extern uint8_t  mode;
extern uint16_t buzzer_delay;
extern uint16_t adc_delay;
extern uint16_t rc5_delay;
extern uint16_t temperature_delay;

#ifdef	USE_INTERRUPT_4_TSOP								// если используем внешнее прерывание INT0 для получения кода RC5
extern volatile	uint16_t rc5_code;							// заводим глобальную переменную для принимаемого кода RC5
#endif

//=====================================================================================================================================================
// Обьявляем глобальные прототипы функций

void Init_Multi_Function_Shield(void);
void Send_Byte(uint8_t byte);								// отправка одного байта по UART
void Buzer_Beep(beep_delay);					    		// звуковой сигнал длительность X мс
void Buzer_OFF(void);                                       // выключение звукового сигнала
void Shield_set_display_value(uint16_t value);				// установка значения для вывода на индикатор
void Shield_display_value(void);							// вывод значения на индикатор
void Shield_display_Err(void);                              // вывод символов ошибки на индикатор
void Send_1_wire_byte(uint8_t);								// отправка 1 байта по шине 1-wire
uint8_t Read_Temperature(void);								// чтение температуры (только целая часть, без CRC)
uint8_t Reset_1_wire(void);									// инициализации обмена по шине 1-wire (Reset-Presence)
void Key_Press(void);										// опрос кнопок

#ifndef	USE_INTERRUPT_4_TSOP								// если не используем внешнее прерывание INT0 для получения кода RC5
uint16_t Get_RC5_code(void);								// обьявляем функцию получения кода RC5
#endif

// Обьявляем внешние прототипы функций

void Blink_Led_4(void);
