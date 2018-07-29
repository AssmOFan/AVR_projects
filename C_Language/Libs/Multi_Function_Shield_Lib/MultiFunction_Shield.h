/*
 * MultiFunction_Shield.h file for Multi_Function_Shield Library
 *
 * Created: 29.07.2018 17:00:00
 * Author : User007
 */ 
//=====================================================================================================================================================
// ��������� ����������� ������������ ������
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//#include <avr/eeprom.h>
//#include <util/atomic.h>
//#include <avr/pgmspace.h>

//=====================================================================================================================================================
// ��������� �������
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
#define CODE_LEN			13								// ���������� ����������� ����� ����
//#define USE_INTERRUPT_4_TSOP								// ������ ������ ���� RC5. ���� ��������������� - ������� (��������� ������ � ����� ������� �������� ������������ � ���������� �������)

#define ONE_WIRE_PIN		PC4
#define ONE_WIRE_PIN_LOW()	(DDRC |=_BV(ONE_WIRE_PIN))
#define ONE_WIRE_PIN_HIGH()	(DDRC &=~_BV(ONE_WIRE_PIN))
#define ONE_WIRE_LINE		(ANALOG_PINS & _BV(ONE_WIRE_PIN))

//=====================================================================================================================================================
// ��������� ���������� ���������� ��� ������������ �������� !!!
extern volatile	uint16_t temperature_delay;					// �������� �� ���������� ���������� ����������� � ������� DS18B20
extern volatile	uint16_t led_delay;							// ������� �� ��������� ��������� �����������
extern volatile	uint8_t  digit_counter;						// ��������� ������� �������� �� 1 ����������
extern volatile	uint8_t  adc_result;						// ��������� �������������� ADC0
extern volatile	uint8_t  adc_delay;							// �������� �� ���������� ��������� ��������� ���
extern volatile	uint8_t  rc5_delay;							// �������� �� ���������� ��������� ���� RC5

#ifdef	USE_INTERRUPT_4_TSOP								// ���� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
extern volatile	uint16_t rc5_code;							// ������� ���������� ���������� 

#else														// ���� �� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
extern volatile	uint8_t  key_delay;							// ����� ������ �������� ����������� � ���������� �� �������
extern volatile	uint8_t  mode;								// ����� ������� ��������� ����������� � ���������� �� �������
#endif
//=====================================================================================================================================================
// ��������� ���������� ��������� �������
void Init(void);
void Buzer_Beep(void);										// ����� 1 ��������� ��������� �������
void Send_Byte(uint8_t byte);								// �������� ������ ����� �� UART
void Shield_display_Err(void);								// ����� �������� ������ �� ���������
void Shield_display_value(void);							// ����� �������� �� ���������
void Shield_set_display_value(uint16_t value);				// ��������� �������� ��� ������ �� ���������
void Send_1_wire_byte(uint8_t);								// �������� 1 ����� �� ���� 1-wire
uint8_t Read_Temperature(void);								// ������ �����������
uint8_t Reset_1_wire(void);									// ������������� ������ (Reset-Presence)

#ifndef	USE_INTERRUPT_4_TSOP								// ���� �� ���������� ������� ���������� INT0 ��� ��������� ���� RC5
uint16_t Get_RC5_code(void);								// ��������� ������� ��������� ���� RC5
#endif

