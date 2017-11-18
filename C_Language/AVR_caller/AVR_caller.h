#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/atomic.h>

#define baudrate 9600L
#define bauddivider (F_CPU/(16*baudrate)-1)
#define HI(x) ((x)>>8)
#define LO(x) ((x)& 0xFF)

#define SIMCOM_RESET_PORT	PORTD
#define SIMCOM_RESET_DDR	DDRD
#define SIMCOM_RESET_PIN	4

#define DATCHIK_1	2						// PD2 - INT0
#define DATCHIK_2	3						// PD3 - INT1

#define OUT_DDR 	DDRB
#define OUT_PORT 	PORTB
#define SIREN		0
#define OUT_1		1
#define OUT_2		2

#define LED_DDR 	DDRB
#define LED_PORT 	PORTB
#define LED_WORK	3
#define LED_PROG	4

#define BUTTON_PINS	PINB
#define BUTTON_PIN	5

#define JUMPER_PINS	PIND
#define JUMPER_PIN	5

// ������ ������ �������
#define GUARD_OFF			0				// 00000000		"����� � ������"
#define GUARD_ON			1				// 00000001		"��� �������"
#define ALARM_ACTIVE		3				// 00000011		"������� �������"
#define ALARM_SIREN_COMPL	5				// 00000101		"�������, ������ ���."
#define ALARM_RING_COMPL	9				// 00001001		"�������, ������ ���������"
#define DELAY_OUT			16				// 00010000		"�������� �� �����"
#define DELAY_IN			32				// 00100000		"�������� �� ����"
#define PROG				64				// 01000000		"����������������"

// ���������� �������� ��-�������
#define IN_PROCESS		0					// � ��������
#define OK				1					// �������
#define BAD				2					// ������

#define NUM_OF_ATTEMPT				5		// ���������� ������� �������� �����-���� �� ������� �� ����������� ������

// ��� ��������� ��������� ������� � �� (�� 1 �� �� 65,5 ������)
#define SIM800L_RESET_TIME			110		// ����� �������� RESET ��� ��������������� ������ ������ (�� �������� >105 ��)
#define WAIT_SIMCOM_READY			9000	// ����� �������� ���������� ������ (�� �������� >2,7 ���)
// 2,7-4 ��� - ������ �������� �� �� - AT...OK....RDY....+CFUN: 1.. ..+CPIN: READY.. ..Call Ready.. ..SMS Ready..  
// 4-9 ��� - ������ �������� �� �� - AT...OK.. ..Call Ready.. ..SMS Ready..
// >9 ��� - ������ �������� �� �� - AT...OK..
#define WAIT_INCOMING_CALL_TIME		65535	// ����� �������� ��������� ������ � ������ ���������������� 
#define AT_WAIT_TIME				3000	// ����� �������� ������ �� AT-������� 
#define BLOCK_ALARM_TIME			350		// ����� ������� ��������� �������� ������� �������

#define OUT_DELAY					10000	// �������� �� �����
#define IN_DELAY					10000	// �������� �� ����
#define SIREN_TIME					10000	// ����� �������� ������
#define OUT_TIME					10000	// ����� ���������� �������
#define RING_WAIT_TIME				20000	// ����� �������� ������ �� ��������� ������ �� �������

#define Abonent_SMS 				number1	// �������, ������� ����� �������� SMS-����������� � ������� ���� 220�

#define DEBUG 						1		// ���. ����� �������. ��������� ��� ���������� �������� ���������
