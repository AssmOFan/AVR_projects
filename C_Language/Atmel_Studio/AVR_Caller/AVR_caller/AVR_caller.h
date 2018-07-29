#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
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

// Режимы работы прибора
#define GUARD_OFF				0			// 00000000		"СНЯТО С ОХРАНЫ"
#define GUARD_ON				1			// 00000001		"ПОД ОХРАНОЙ"
#define ALARM_ACTIVE			3			// 00000011		"ТРЕВОГА АКТИВНА"
#define ALARM_SIREN_COMPL		5			// 00000101		"ТРЕВОГА, СИРЕНА ВКЛ."
#define ALARM_RING_COMPL		9			// 00001001		"ТРЕВОГА, ЗВОНКИ СОВЕРШЕНЫ"
#define DELAY_OUT				16			// 00010000		"ЗАДЕРЖКА НА ВЫХОД"
#define DELAY_IN				32			// 00100000		"ЗАДЕРЖКА НА ВХОД"
#define PROG					64			// 01000000		"ПРОГРАММИРОВАНИЕ"
#define TRK						128			// 10000000		"СРАБОТКА ТРК"

// Результаты парсинга АТ-команда
#define IN_PROCESS				0			// В процессе
#define OK						1			// Успешно
#define BAD						2			// Ошибка

#define NUM_OF_ATTEMPT			5			// Количество попыток отправки какой-либо АТ команды до перезапуска модуля

// Все временные диапазоны указаны в мс (от 1 мс до 65,5 секунд максимум)
#define SIM800L_RESET_TIME		110			// Время просадки RESET для гарантированого сброса модуля (по даташиту >105 мс)
#define WAIT_SIMCOM_READY		5000		// Время ожидания готовности модуля (по даташиту >2,7 сек)
/*
2,7-4 сек - модуль отвечает на АТ - AT...OK....RDY....+CFUN: 1.. ..+CPIN: READY.. ..Call Ready.. ..SMS Ready..  
4-9 сек - модуль отвечает на АТ - AT...OK.. ..Call Ready.. ..SMS Ready..
>9 сек - модуль отвечает на АТ - AT...OK..
*/
#define WAIT_INCOMING_CALL_TIME	65535		// Время ожидания входящего звонка в режиме программирования 
#define AT_WAIT_TIME			3000		// Время ожидания ответа на AT-команды (период отправки АТ-команд)
#define TRASH_WAIT_TIME			100			// Время для пропуска "мусора" по UART после неудачного парсинга

#define OUT_DELAY_TIME			3000		// Задержка на выход
#define IN_DELAY_TIME			3000		// Задержка на вход
#define SIREN_TIME				3000		// Время звучания сирены
#define OUT_TIME				3000		// Время активности выходов
#define RING_WAIT_TIME			65535		// Время ожидания ответа на исходящий звонок по тревоге

#define Abonent_SMS 			number1		// Абонент, который будет получать SMS-уведомления о пропаже сети 220В

#define	ENABLE_LISTENING					// Включить режим прослушки при компиляции исходника
#ifdef	ENABLE_LISTENING
#define LISTENING_WAIT_TIME		65535		// Максимальоне время прослушки, по истечения которого ППК сам положит трубку, если до этого ее не положил абонент, которому дозвонлся ППК
#endif
/*
	Режим прослушки работает следующим образом:
	1. При тревоге ППК совершает звонок.
	2. Если пользователь отбивает входящий звонок, ППК просто продолжает выполнение конечного автомата дозвона абонентам.
	3. Если пользователь взял трубку, ППК приостанавливает дальнейшее выполнение конечного автомата, ожидая пока положат трубку, и затем, после завершения прослушки, продолжает выполнение конечного автомата дозвона абонентам.
*/
#define USE_INT1_AS_TRK
#define DEBUG								// Вкл. режим отладки. ОТКЛЮЧИТЬ ПРИ КОМПИЛЯЦИИ РАБОЧЕГО ИСХОДНИКА
#define ENABLE_WDT							// Вкл. WDT
