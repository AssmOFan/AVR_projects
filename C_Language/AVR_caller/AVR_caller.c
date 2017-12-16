#include <AVR_caller.h>

// Объявляем глобальные переменные и дефайны
// Все что в обработчиках прерываний - volatile
// Все что используеться только в 1 функции - static

#define 	buffer_max			14							// Длина приемного буффера = длинна тел. номера в формате "+38 0XX XXX XX XX" = 13 + 1 для окончания строки
volatile	unsigned	char	buffer[buffer_max];			// Сам приемный буффер
volatile	unsigned	char	buffer_index = 0;			// Текущий элемент приемного буффера
static					char	number1[buffer_max];		// Строка 1 тел. номера в формате "+38 0XX XXX XX XX" + 1 для окончания строки
static					char	number2[buffer_max];		// Строка 2 тел. номера в формате "+38 0XX XXX XX XX" + 1 для окончания строки
static					char	number3[buffer_max];		// Строка 3 тел. номера в формате "+38 0XX XXX XX XX" + 1 для окончания строки
static		unsigned	char	pin_state;					// Переменная для сохранения состояние ножек какого-либо порта
static		unsigned	char	programming_mode = 0;		// Состояния конечного автомата программирования номеров дозвона
static		unsigned	char	parsing_fault = NUM_OF_ATTEMPT+1;// Счетчик ошибок парсинга
static		unsigned	char	simcom_mode = 1;			// Состояния конечного автомата отправки АТ-команд
static		unsigned	char	simcom_init_mode = 0;		// Состояния инициализации модуля SIMCOM (0 - не готов звонить и слать SMS, 1 - готов звонить и слать SMS) 
static		unsigned	int 	check_button_counter;		// Счетчик пропущеных циклов опроса кнопки постановки/снятия из функции опроса кнопки
volatile	unsigned	int		debounce_delay = 0;			// Задержка запрета опроса кнопки постановки/снятия
volatile	unsigned	int		parsing_delay = 0;			// Задержка на время парсинга
volatile	unsigned	int		exit_delay = 0;			// Задержка на вход/выход
volatile	unsigned	int		led_delay = 0;				// Задержка до смены состояния светодиодов LED_WORK или LED_PROG
volatile	unsigned	int		siren_delay = 0;			// Задержка на звучание сирены
volatile	unsigned	int		out_delay = 0;				// Задержка на выключение выходов после активации
volatile	unsigned	char	parsing_result = BAD;		// Результат парсинга строки (0-парсинг продолжаеться, 1-парсинг успешно закончен, 2-ошибка парсинга или превышено время парсинга)
volatile	unsigned	char	ppk_mode = GUARD_OFF;		// Режим работы прибора
volatile	unsigned	char	flags = 0;
#define 	sms_flag			0

const	char *volatile	parsing_pointer;					// Указатель на строку во флеше, с которой будем сравнивать входящую строку

// Заводим строки AT команд во флеше. Обязательно вне main
const	char AT[] 		PROGMEM = "AT\r";
const	char ATE0[] 	PROGMEM = "ATE0\r";					// Отключаем эхо
const	char AT_IPR[]	PROGMEM = "AT+IPR=9600\r";			// Настраиваем скорость обмена
const	char AT_CLCC[]	PROGMEM	= "AT+CLCC=0\r";			// Переключаем на сокращенный ответ при входящем звонке
const	char AT_CMGF[]	PROGMEM = "AT+CMGF=1\r";			// Текстовый режим СМС
const	char AT_CLIP[]	PROGMEM = "AT+CLIP=1\r";			// Включаем АОН
const	char AT_CPAS[]	PROGMEM = "AT+CPAS\r";				// Смотрим состояние модуля
const	char AT_CREG[]	PROGMEM = "AT+CREG?\r";				// Смотрим регистрацию в сети
const	char AT_CCALR[]	PROGMEM = "AT+CCALR?\r";			// Проверяем возможность совершать звонки
const	char ATD[]		PROGMEM = "ATD";					// Звоним
const	char RING_END[]	PROGMEM = ";\r";
const	char AT_CMGS[]	PROGMEM = "AT+CMGS=\"";				// Отправляем СМС
const	char AT_CMGS_2[]PROGMEM = "\"\r";
const	char NO_220[]	PROGMEM = "HET 220B\x1A";
const	char RETURN_220[]PROGMEM ="ECTb 220B\x1A";
const	char ATH[]		PROGMEM = "ATH\r";					// Отбиваем вызов
const	char AT_GSMBUSY_1[]	PROGMEM = "AT+GSMBUSY=1\r";		// Запрет всех входящих звонков
const	char AT_GSMBUSY_0[]	PROGMEM = "AT+GSMBUSY=0\r";		// Разрешение всех входящих звонков

// Заводим строки ответов на AT команды во флеше. Обязательно вне main
const	char AT_OK[]	PROGMEM = "AT\r\r\nOK\r\n";
const	char ATE0_OK[]	PROGMEM = "ATE0\r\r\nOK\r\n";
const	char _OK[]		PROGMEM = "\r\nOK\r\n";
const	char CPAS_OK[]	PROGMEM = "\r\n+CPAS: 0\r\n\r\nOK\r\n";
const	char CREG_OK[]	PROGMEM = "\r\n+CREG: 0,1\r\n\r\nOK\r\n";
const	char CCALR_OK[]	PROGMEM = "\r\n+CCALR: 1\r\n\r\nOK\r\n";
const	char RING[]		PROGMEM = "\r\nRING\r\n\r\n+CLIP: \"";
const	char POINTER[]	PROGMEM = "> ";
const	char BUSY[]		PROGMEM = "\r\nBUSY\r\n";

// Заводим строки под тел. номера в EEPROM. Обязательно вне main
unsigned char EEMEM ppk_mode_save = GUARD_OFF;				// Резервная копия состояние ППК в EEPROM. Вычитываеться при каждом старте системы.
//unsigned char EEMEM ee_number1[buffer_max] = "+380962581099";// 1 номер в EEPROM
//unsigned char EEMEM ee_number2[buffer_max] = "+380996755968";// 2 номер в EEPROM
//unsigned char EEMEM ee_number3[buffer_max] = "+380962581099";// 3 номер в EEPROM

unsigned char EEMEM ee_number1[buffer_max] = "             ";// 1 номер в EEPROM
unsigned char EEMEM ee_number2[buffer_max] = "             ";// 2 номер в EEPROM
unsigned char EEMEM ee_number3[buffer_max] = "             ";// 3 номер в EEPROM

//=====================================================================================================================================================
// Обьявляем прототипы функций
void 	Init(void);
void	ActivateParsing(const char *string, unsigned int parsing_delay);
void	SendByte(char byte);
void	SendStr(char *string);
void	SendStr_P(const char *string);
void	Programming(void);
void	ReadNumbers(void);
void	CheckButton(unsigned int);
void	ResetSIMCOM(void);
void	CheckSIMCOM(void);
void	SwitchSIMCOM_mode(void);
void 	Ring(void);
void	Ring_on_Number(char *number);
void	Switch_Programming_mode(void);
void 	Wait_RING(unsigned int _led_delay, unsigned char next_programming_mode);
void 	SaveNumber_2_RAM(char *number, unsigned char next_programming_mode);
void	Siren_Outs_OFF(void);
void	Blink_LED_WORK(void);
//=====================================================================================================================================================
int main(void)
{
	Init();													// Инициализация портов и периферии
	ppk_mode = eeprom_read_byte(&ppk_mode_save);			// Восстанавливаем состояние ППК из EEPROM до разрешения прерваний, для атомарности
	sei();
	wdt_enable(WDTO_2S);									// Включаем сторожевой таймер со сбросом через 2 секунды

	if (ppk_mode == GUARD_OFF)								// Если ППК снят с охраны								
	{
		pin_state = JUMPER_PINS;							// Читаем состояние всего порта c Джампером программирования
		if (!(pin_state & (1<<JUMPER_PIN))) Programming();	// Если Джампер программирования в положении ПРОГ (вывод JUMPER_PIN на земле), переходим в режим "ПРОГРАММИРОВАНИЕ"
	}
	programming_mode = 0;

	wdt_reset();
	ReadNumbers();											// Читаем записанные телефонные номера из EEPROM в ОЗУ

	if (ppk_mode != GUARD_OFF)								// Если НЕ в режиме "СНЯТО С ОХРАНЫ"
	{
		GIFR = 1<<INTF1|1<<INTF0;							// Сбросим флаги возможно возникавших ранее внешних прерываний
		GICR = 1<<INT1|1<<INT0;								// Разрешим прерывания INT1 и INT0
	#if defined (DEBUG)
		LED_PORT &= ~(1<<LED_WORK);							// ТОЛЬКО ДЛЯ ОТЛАДКИ
	#else
		LED_PORT |= 1<<LED_WORK;							// Включим светодиод охраны
	#endif
	}

//=====================================================================================================================================================
// Главный цикл
	while (1)
	{
		CheckButton(5000);									// Проверяем кнопку постановки/снятия каждый 5000-й проход главного цикла
		CheckSIMCOM();										// Проверяем состояние модуля, регистрацию в сети, и прочее
		Siren_Outs_OFF();									// Проверяем сирену и выходы, если пора - выключаем
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_IN)&&(exit_delay == 0))	// Если ППК находиться в состоянии "ЗАДЕРЖКА НА ВХОД" и задержка истекла
		{
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				ppk_mode = ALARM_ACTIVE;					// Активируем режим "ТРЕВОГА АКТИВНА"
				eeprom_update_byte(&ppk_mode_save, ALARM_ACTIVE);// Обновляем состояние ППК в EEPROM
			}
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if (ppk_mode == ALARM_ACTIVE)						// Если возникла тревога
		{
		#if defined (DEBUG)
			OUT_PORT &= ~(1<<SIREN|1<<OUT_2|1<<OUT_1);		// ТОЛЬКО ДЛЯ ОТЛАДКИ
		#else
			OUT_PORT |= 1<<SIREN|1<<OUT_1;					// Включим сирену, OUT_1
			OUT_PORT &= ~(1<<OUT_2);						// и OUT_2 (инверсная логика работы)
		#endif

			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				siren_delay = SIREN_TIME;					// Задаем время звучания сирены
				out_delay = OUT_TIME;						// Задаем время активности выходов
				ppk_mode = ALARM_SIREN_COMPL;				// Сирена была включена на нужное время, меняем состояние ППК
				eeprom_update_byte(&ppk_mode_save, ALARM_SIREN_COMPL);// Обновляем состояние ППК в EEPROM
			}
			GIFR = 1<<INTF1|1<<INTF0;						// Сбросим флаги возникавших ранее прерываний
			GICR = 1<<INT1|1<<INT0;							// Разрешим прерывания INT1 и INT0				
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == ALARM_SIREN_COMPL)&&(simcom_init_mode == 1))// Если была включена сирена, и модуль SIMCOM находиться в рабочем режиме, начинаем звонить
		{
			Ring();											// Звоним
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if (((ppk_mode != GUARD_OFF)&&(ppk_mode != GUARD_ON))&&(led_delay == 0))// Если режим "ТРЕВОГА" или "ЗАДЕРЖКА"
		{
			Blink_LED_WORK();								// Мигаем LED_WORK
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_OUT)&&(exit_delay == 0))	// Если ППК находиться в состоянии "ЗАДЕРЖКА НА ВЫХОД" и задержка истекла
		{
				ppk_mode = GUARD_ON;						// Активируем режим "ПОД ОХРАНОЙ"
				GIFR = 1<<INTF1|1<<INTF0;					// Сбросим флаги возможно возникавших ранее прерываний
				GICR = 1<<INT1|1<<INT0;						// Разрешим прерывания INT1 и INT0
				ATOMIC_BLOCK(ATOMIC_FORCEON)
				{
					led_delay = 0;							// Прекращаем мигать LED_WORK
				}
			#if defined (DEBUG)
				LED_PORT &= ~(1<<LED_WORK);					// ТОЛЬКО ДЛЯ ОТЛАДКИ
			#else
				LED_PORT |= 1<<LED_WORK;					// Зажигаем светодиод ОХРАНА
			#endif
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_IN)&&(exit_delay == IN_DELAY))// Если ППК находиться в состоянии "ЗАДЕРЖКА НА ВХОД" и она только началась
		{
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				eeprom_update_byte(&ppk_mode_save, DELAY_IN);// Обновляем состояние ППК в EEPROM
			}
		}		
	}
}
//=====================================================================================================================================================
ISR (USART_RXC_vect)										// Прерывание по приходу байта в буффер UART
{
	if (ppk_mode == PROG)									// Если прибор в режиме "ПРОГРАММИРОВАНИЕ" (записи телефонных номеров)
	{
		buffer[buffer_index] = UDR;							// Просто пишем данные (телефонный номер звонящего) в буффер		
		buffer_index++;										// Увеличиваем индекс
		if (buffer_index == buffer_max-1)					// Если достигли конца буффера 
		{
			buffer[buffer_index] = '\0';					// Запишем признак конца строки
			UCSRB &= ~(1<<RXCIE);							// И запретим прерывание по приходу байта - номер звонящего скопирован в буффер
		}			
	}

	else													// Если прибор парсит приходящие команды, парсим строку посимвольно
	{
		if (UDR == pgm_read_byte(parsing_pointer))			// Сравниваем принятый байт с символом из строки  
		{													// Если идентичны		
			parsing_pointer++;								// Увеличиваем указатель, выбирая следующий символ строки
			if (pgm_read_byte(parsing_pointer) == '\0')		// Если следующий байт строки 0 (конец строки)
			{			
				parsing_result = OK;						// Устанавливаем признак успешного окончания парсинга
				UCSRB &= ~(1<<RXCIE);						// Запрещаем прерывание по приходу байта - прием закончен			
			}
		}

		else parsing_result = BAD;							// Если не идентичны - возвращаем признак ошибки парсинга. Не 0, чтобы сразу зафиксировать неудачный парсинг
	}
}
//=====================================================================================================================================================
ISR (TIMER1_COMPA_vect)										// Прерывание по совпадению Timer1
{
	if (parsing_delay != 65535)								// Если подсчет времени парсинга не запрещен (записью максимального значения в счетчик)
	{		
		if (parsing_delay != 0) parsing_delay--;
		else
		{
			if (parsing_result != OK)						// Время парсинга исчерпано, если не был установлен признак успешного парсинга
			{
				parsing_result = BAD;						// Устанавливаем признак проваленого парсинга
				UCSRB &= ~(1<<RXCIE);						// Запрещаем прерывание по приходу байта - чтобы не отвлекаться на всякую дрянь
			}
			parsing_delay--;								// Запрещаем подсчет времени парсинга (записью максимального значения в счетчик)
		}
	}
/*
	if (led_delay != 65535)									// Если мигание светодиодов не запрещено (записью максимального значения в счетчик)
	{
		if (led_delay != 0) led_delay--;
		else 
		{
			if (ppk_mode == PROG) LED_PORT ^= 1<<LED_PROG;	// Переключаем нужный светодиод в зависимости от режима работы (скважность 50%)
			else LED_PORT ^= 1<<LED_WORK;
			led_delay--;									// Запрещаем отсчет времени мигания светодиодов (записью максимального значения в счетчик)
		}
	}
*/	
	if (debounce_delay != 0) debounce_delay--;				// Отсчет времени запрета опроса кнопки постановки/снятия после предыдущего нажатия
	if (exit_delay != 0) exit_delay--;						// Отсчет задержки вход/выход, если она есть
	if (siren_delay != 0) siren_delay--;					// Отсчет времени звучания сирены
	if (out_delay != 0) out_delay--;						// Отсчет времени активации выходов
	if (led_delay != 0) led_delay--;						// Отсчет времени мигания светодиодов
}
//=====================================================================================================================================================
ISR (INT0_vect)												// Прерывание по INT0
{
	if (ppk_mode == GUARD_ON)								// Если ППК в режиме "ПОД ОХРАНОЙ" (тривог до этого момента не было)
	{
		ppk_mode = DELAY_IN;								// Переводим ППК в состояние задержка на вход
		exit_delay = IN_DELAY;
	} 
	else ppk_mode = ALARM_ACTIVE;							// Иначе сразу формируем очередную тревогу	
	GICR = 0<<INT1|0<<INT0;									// Запретим прерывания INT1 и INT0
}
//=====================================================================================================================================================
ISR (INT1_vect)												// Прерывание по INT1
{
	if (ppk_mode == GUARD_ON)								// Если ППК в режиме "ПОД ОХРАНОЙ" (тривог до этого момента не было)
	{
		ppk_mode = DELAY_IN;								// Переводим ППК в состояние задержка на вход
		exit_delay = IN_DELAY;
	} 
	else ppk_mode = ALARM_ACTIVE;							// Иначе сразу формируем очередную тревогу
	GICR = 0<<INT1|0<<INT0;									// Запретим прерывания INT1 и INT0
}
//=====================================================================================================================================================
ISR (ANA_COMP_vect)											// Прерывание компаратора, возникает при пропаже сети 220В
{
	flags |= 1<<sms_flag;									// Установим флаг необходимости отправки SMS
	ACSR &= ~(1<<ACIE);										// Запретим прерывания от компаратора для однократной отправки SMS
	ACSR ^= 1<<ACIS0;										// Меняем условие возникновения прерывания - если был переход с 0 на 1, делаем переход с 1 на 0 и наоборот
}
//=====================================================================================================================================================
// Конечный автомат дозвона. В зависимости от значения ring_mode, осуществляет исходящий вызов, либо ожидает реакции абонента
void Ring(void)
{
	unsigned char ring_mode = 1;							// Состояние автомата изменяеться исключительно внутри него самого, поэтому переменная локальная

	while (parsing_result == IN_PROCESS){}					// Ждем окончания парсинга предыдущей команды

	while (ring_mode != 16)									// Пока автомат не перейдет в состояние "Попытки дозвона на все номера осуществлены"
	{
		wdt_reset();
		Siren_Outs_OFF();									// Проверяем сирену и выходы, если пора - выключаем

		if (led_delay == 0) Blink_LED_WORK();				// Мигаем LED_WORK

		CheckButton(10000);									// Проверяем кнопку постановки/снятия каждый 10000-й проход цикла дозвона по тревоге,
															// обязательно после мигания LED_WORK по тревоге, иначе получим инвертированиое состояние выхода после снятия с охраны
		if ((ppk_mode == GUARD_OFF)&&(ring_mode != 15))		// Если ППК был переведен в состояние "СНЯТО С ОХРАНЫ" и еще продолжаеться дозвон
		{
			ring_mode = 14;									// Прекращаем дозвон
		}
						
		switch (ring_mode)									// Осуществляем дозвон на все номера
		{
			case 1:
			{
				Ring_on_Number(number1);					// Звоним 1 абоненту				
				ring_mode = 2;								// Переводим автомат в состояние "Ожидание ответа модуля на дозвон 1 абоненту"
				break;
			}
			case 2:
			{
				if (parsing_result != IN_PROCESS)			// Если модуль начал не дозвон
				{
					ring_mode = 3;							// Просто положим трубку	
				}

				if (parsing_result == OK)					// А если начал дозвон (прислал OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// Активируем парсинг ответа абонента
				}
				break;
			}
			case 3:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					SendStr_P(ATH);							// Ложим трубку					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 4;							// Переводим автомат в состояние "Ожидание отбоя вызова"
				}
				break;
			}
			case 4:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					ring_mode = 5;							// Переводим автомат в состояние "Звонок 2 абоненту"
				}
				break;
			}
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 5:
			{
				Ring_on_Number(number2);					// Звоним 2 абоненту				
				ring_mode = 6;								// Переводим автомат в состояние "Ожидание ответа модуля на дозвон 2 абоненту"
				break;
			}
			case 6:
			{
				if (parsing_result != IN_PROCESS)			// Если модуль начал не дозвон
				{
					ring_mode = 7;							// Просто положим трубку	
				}

				if (parsing_result == OK)					// А если начал дозвон (прислал OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// Активируем парсинг ответа абонента
				}
				break;
			}
			case 7:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					SendStr_P(ATH);							// Ложим трубку					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 8;							// Переводим автомат в состояние "Ожидание отбоя вызова"
				}
				break;
			}
			case 8:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					ring_mode = 9;							// Переводим автомат в состояние "Звонок 3 абоненту"
				}
				break;
			}		
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 9:
			{
				Ring_on_Number(number3);					// Звоним 3 абоненту				
				ring_mode = 10;								// Переводим автомат в состояние "Ожидание ответа модуля на дозвон 3 абоненту"
				break;
			}
			case 10:
			{
				if (parsing_result != IN_PROCESS)			// Если модуль начал не дозвон
				{
					ring_mode = 11;							// Просто положим трубку	
				}

				if (parsing_result == OK)					// А если начал дозвон (прислал OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// Активируем парсинг ответа абонента
				}
				break;
			}
			case 11:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					SendStr_P(ATH);							// Ложим трубку					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 12;							// Переводим автомат в состояние "Ожидание отбоя вызова"
				}
				break;
			}
			case 12:
			{
				if (parsing_result != IN_PROCESS)			// Независимо от ответа модуля
				{
					ring_mode =13;							// Переводим автомат в состояние "Все звонки осуществлены"
				}
				break;
			}
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 13:
			{
				ATOMIC_BLOCK(ATOMIC_FORCEON)
				{
					ppk_mode = ALARM_RING_COMPL;			// Все звонки осуществлены. Активируем режим ППК "ТРЕВОГА, ЗВОНКИ СОВЕРШЕНЫ"
					eeprom_update_byte(&ppk_mode_save, ALARM_RING_COMPL);// И обновляем резервную копию состояния ППК в EEPROM
				}
				ring_mode = 16;								// Переводим автомат в состояние "Попытки дозвона на все номера осуществлены"
				break;
			}
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 14:
			{
				SendStr_P(ATH);
				ActivateParsing(_OK,AT_WAIT_TIME);
				ring_mode = 15;
				break;
			}
			case 15:
			{
				if (parsing_result != IN_PROCESS)
				{
				#if defined (DEBUG)
					LED_PORT |= 1<<LED_WORK;				// ТОЛЬКО ДЛЯ ОТЛАДКИ
				#else					
					LED_PORT &= ~(1<<LED_WORK);				// Гасим светодиод ОХРАНА
				#endif
					ring_mode = 16;
				}
				break;
			}
			case 16: break;
			default: ring_mode = 16; break;
		}
	}
}
//=====================================================================================================================================================
// Дозвон конкретному абоненту
void Ring_on_Number(char *number)							// В качестве параметра передаеться указатель на 1 символ номера абонента
{
	SendStr_P(ATD);											// Звоним абоненту
	SendStr(number);
	SendStr_P(RING_END);
	ActivateParsing(_OK,RING_WAIT_TIME);					// Активируем ожидание ответа
}
//=====================================================================================================================================================
// Функция проверки кнопки постановки/снятия
void CheckButton(unsigned int button_counter_delay)			// Опрашиваем кнопку постановки/снятия внутри функций, чтобы не нагружать таймер
{															// В качестве параметра передаеться количество пропусков циклов (внутри функции, из которой был запущен опрос кнопки) до фактического опроса кнопки
	if (debounce_delay == 0)								// Если нет запрета на опрос кнопки постановки/снятия
	{
		check_button_counter--;							
		if (check_button_counter == 0)
		{
			pin_state = BUTTON_PINS;						// Читаем состояние всего порта
			if (!(pin_state & (1<<BUTTON_PIN)))				// Если кнопка постановки/снятия нажата, активируем переход в другой режим
			{
				if (ppk_mode == GUARD_OFF)					// Если текущий режим "СНЯТО С ОХРАНЫ"						
				{
					ppk_mode = DELAY_OUT;					// Активируем режим "ЗАДЕРЖКА НА ВЫХОД"
					ATOMIC_BLOCK(ATOMIC_FORCEON)
					{
						eeprom_update_byte(&ppk_mode_save, DELAY_OUT);// В EEPROM пишем состояние "ПОД ОХРАНОЙ", чтобы при перезагрузке ППК во время задержки на выход получить охраняемый объект
						exit_delay = OUT_DELAY;			// Назначим задержку на выход, внутри запрета прерываний, для атомарности
					}
				#if defined (DEBUG)
					LED_PORT &= ~(1<<LED_WORK);				// ТОЛЬКО ДЛЯ ОТЛАДКИ
				#else
					LED_PORT |= 1<<LED_WORK;				// Зажигаем светодиод ОХРАНА			
				#endif
				}

				else										// Иначе текущий режим "ПОД ОХРАНОЙ" либо "ТРЕВОГА"
				{			
					ppk_mode = GUARD_OFF;					// Активируем переход в режим "СНЯТО С ОХРАНЫ"						
					GICR = 0<<INT1|0<<INT0;					// Запретим прерывания INT1 и INT0
					ATOMIC_BLOCK(ATOMIC_FORCEON)
					{
						eeprom_update_byte(&ppk_mode_save, GUARD_OFF);// Обновим состояние ППК в EEPROM
						siren_delay = 0;					// Убираем время звучания сирены, сама сирена выключиться в главном цикле
						out_delay = 0;						// Убираем время активности выходов, сами выходы выключаться в главном цикле						
						led_delay = 0;						// Прекращаем мигать светодиодом LED_WORK (ОХРАНА), если он мигал. Это проще чем допольнительная проверка						
					}					
					#if defined (DEBUG)
						LED_PORT |= 1<<LED_WORK;			// ТОЛЬКО ДЛЯ ОТЛАДКИ
					#else					
						LED_PORT &= ~(1<<LED_WORK);			// Гасим светодиод ОХРАНА
					#endif
				}		

				debounce_delay = 1000;						// Запрещаем реакцию на нажатие кнопки постановки/снятия на 1 сек, для исключения влияния дребезга
			}

			check_button_counter = button_counter_delay;	// Обновляем счетчик опроса кнопки постановки/снятия
		}
	}
}
//=====================================================================================================================================================
// Функция опроса SIMCOM. Содержит модуль анализа ответов. При неправильном ответе на 5 запросов подряд - перезапустит модуль SIMCOM и произведет его полную переинициализацию
void CheckSIMCOM(void)									
{
	wdt_reset();
								
	if ((parsing_result == OK)&&(parsing_delay == 65535))	// Если предыдущий парсинг закончился успешно, и истекло время парсинга (можно слать следующую АТ-команду)
	{
		parsing_fault = NUM_OF_ATTEMPT;						// Обновим счетчик ошибок парсинга
		switch (simcom_mode)								// Переключим состояние автомата SwitchSIMCOM_mode для отправки следующей команды
		{
			case 1: break;									// Если только зашли в автомат, перезапускаем модуль
			case 2: simcom_mode = 3; break;					// Если получили ответ на АТ, шлем ATE0
			case 3: simcom_mode = 4; break;					// Если получили ответ на АТЕ0, шлем AT+IPR
			case 4: simcom_mode = 5; break;					// Если получили ответ на AT+IPR, шлем AT+CMGF
			case 5: simcom_mode = 6; break;					// Если получили ответ на AT+CMGF, шлем AT+GSMBUSY=1
			case 6: simcom_mode = 7; break;					// Если получили ответ на AT+GSMBUSY=1, шлем AT+CPAS
			case 7: simcom_mode = 8; break;					// Если получили ответ на AT+CPAS, шлем AT+CREG
			case 8: simcom_mode = 9; break;					// Если получили ответ на AT+CREG, шлем AT+CCALR
			case 9:											// Если получили ответ на AT+CCALR
			{
				simcom_init_mode = 1;						// Модуль SIMCOM прошел полную инициализацию и может совершать звонки и слать SMS
				if (programming_mode == 1)					// Если ППК в режиме программирования, продолжаем спец. инициализацию
					 simcom_mode = 12;						// Шлем AT+CLCC=0
				else simcom_mode = 7;						// Иначе опять шлем AT+CPAS (и так по кругу гоняем состояния 7-8-9)
				break;
			}
			case 10: simcom_mode = 11; break;				// Если получили курсор приема тела SMS, отправляем тело SMS
			case 11:										// Если успешно отправили SMS
			{
				ACSR |= 1<<ACI|1<<ACIE;						// Разрешим прерывания от компаратора для повторной отправки SMS о пропаже 220В	
				simcom_mode = 7;							// Перелючаем автомат отправки АТ-команд на отправку 1-й команды циклического опроса модуля (AT+CPAS)
				break;
			}
			case 12: simcom_mode = 13; break;				// Если получили ответ на AT+CLCC=0, шлем AT+CLIP
			case 13: simcom_mode = 14; break;				// Если получили ответ на AT+CLIP, шлем AT+GSMBUSY=0
			case 14:										// Если получили ответ на AT+GSMBUSY=0
			{
				simcom_init_mode = 2;						// Модуль SIMCOM готов принимать и обрабатывать входящие звонки 
				simcom_mode = 15;							// Ничего не шлем
				break;
			}
			default: simcom_mode = 1;
		}
		SwitchSIMCOM_mode();								// Отправляем АТ-команду, конечный автомат сам выберет нужную		
	}

	if ((parsing_result == BAD)&&(parsing_delay == 65535))	// Если парсинг закончился неуспешно, и истекло время парсинга
	{
		if ((simcom_mode == 10)||(simcom_mode == 11))		// И мы не получили курсор приглашения ввода тела SMS, либо ОК после отправки тела SMS
		{
			ACSR |= 1<<ACI|1<<ACIE;							// Отправка SMS о пропаже 220В не удалась. Повтор делать не будем, но разрешаем прерывания компаратора, возможно будут еще пропажи сети 220В и их можно будет передать
			simcom_mode = 7;								// Перелючаем автомат отправки АТ-команд на отправку 1-й команды циклического опроса модуля (AT+CPAS)
		}

		else
		{			
			parsing_fault--;
			if (parsing_fault == 0)							// Если исчерпали попытки парсинга
			{
				simcom_init_mode = 0;						// Сбрасываем состояние инициализации модуля SIMCOM
				simcom_mode = 1;							// Переводим автомат в начальный режим - делаем переинициализацию модуля SIMCOM
				parsing_fault = NUM_OF_ATTEMPT;				// Обновим счетчик ошибок парсинга
			}
		}
		SwitchSIMCOM_mode();								// Повторно отправляем предыдущую АТ-команду без предварительного переключения состояния автомата
	}
}
//=====================================================================================================================================================
// Конечный автомат выбора отправляемой АТ-команды. В зависимости от значения simcom_mode, посылает определенную AT-команду
void SwitchSIMCOM_mode(void)														
{															// Если надо отправить SMS, изменяем выбраное ранее состояние автомата
	if ((flags & (1<<sms_flag))&&(simcom_init_mode == 1))	// Если установлен признак необходимости отправки SMS и модуль SIMCOM прошел полную инициализацию
	{
		simcom_mode = 10;									// Переключим автомат отправки АТ-команд в режим отправки SMS
		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			flags &= ~(1<<sms_flag);						//  Сразу запретим повторное переключение автомата в режим отправки SMS
		}
	}

	switch (simcom_mode)									// Состояние автомата определяеться ответами (верный/неверный) модуля SIMCOM
	{
		case 1:
		{
			SIMCOM_RESET_PORT &= ~(1<<SIMCOM_RESET_PIN);	// Садим SIMCOM_RESET на землю
			_delay_ms(SIM800L_RESET_TIME);					// Задержка на Reset модуля SIMCOM
			SIMCOM_RESET_PORT |= 1<<SIMCOM_RESET_PIN;		// Отпускаем SIMCOM_RESET
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				parsing_delay = WAIT_SIMCOM_READY;			// Воспользуемся таймером парсинга, все равно до перезапуска модяля по UART не используеться
				simcom_mode = 2;
			}			
			break;
		}
		case 2:
		{
			SendStr_P(AT);									// Шлем АТ
			ActivateParsing(AT_OK,AT_WAIT_TIME);			// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 3:
		{			
			SendStr_P(ATE0);								// Отключаем эхо
			ActivateParsing(ATE0_OK,AT_WAIT_TIME);			// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 4:
		{			
			SendStr_P(AT_IPR);								// Задаем скорость обмена с модулем
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 5:
		{
			SendStr_P(AT_CMGF);								// Задаем текстовый формат SMS
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect			
			break;
		}
		case 6:
		{			
			SendStr_P(AT_GSMBUSY_1);						// Запрет всех входящих звонков
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 7:
		{			
			SendStr_P(AT_CPAS);								// Делаем запрос на состояние модуля SIMCOM
			ActivateParsing(CPAS_OK,AT_WAIT_TIME);			// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 8:
		{			
			SendStr_P(AT_CREG);								// Делаем запрос на состояние регистрации в сети
			ActivateParsing(CREG_OK,AT_WAIT_TIME);			// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 9:
		{			
			SendStr_P(AT_CCALR);							// Делаем запрос на возможность совершать звонки
			ActivateParsing(CCALR_OK,AT_WAIT_TIME);			// Активируем парсинг ответа в обработчике USART_RX_vect
			break;	
		}
		case 10:
		{			
			SendStr_P(AT_CMGS);								// Делаем запрос на отправку SMS о пропаже сети 220В						
			SendStr(Abonent_SMS);
			SendStr_P(AT_CMGS_2);
			ActivateParsing(POINTER,AT_WAIT_TIME);			// Активируем парсинг курсора приглашения для отправки тела SMS
			break;
		}
		case 11:
		{
			if (!(ACSR & (1<<ACIS0)))						// Если следующие прерывание от компаратора ожидается по переходу выхода компаратора с 1 на 0 
				SendStr_P(NO_220);							// Отправляем тело SMS о пропаже сети 220В
			else SendStr_P(RETURN_220);						// Иначе отправляем тело SMS о восстановлении сети 220В			
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг отчета о успешной отправке SMS в обработчике USART_RX_vect
			break;
		}
		case 12:
		{
			SendStr_P(AT_CLCC);								// Переключаем на сокращенный ответ при входящем звонке
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 13:
		{
			SendStr_P(AT_CLIP);								// Включаем АОН
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 14:
		{
			SendStr_P(AT_GSMBUSY_0);						// Разрешение всех входящих звонков
			ActivateParsing(_OK,AT_WAIT_TIME);				// Активируем парсинг ответа в обработчике USART_RX_vect
			break;
		}
		case 15:
		{
			simcom_mode = 6;								// Выставляем автомат в состояние запрета всех входящих звонков, чтобы после процедуры программирования сразу запретить входящие звонки
			break;
		}		
		default: simcom_mode = 1;
	}
}
//=====================================================================================================================================================
// Программирование прибора
void Programming(void)
{
#if defined (DEBUG)
	LED_PORT &= ~(1<<LED_PROG);								// ТОЛЬКО ДЛЯ ОТЛАДКИ
#else
	LED_PORT |= 1<<LED_PROG;								// Включим светодиод программирования
#endif
/*
	while (simcom_init_mode != 1)							// Пока модуль SIMCOM не пройдет полную инициализацию
	{
		CheckSIMCOM();										// Проверяем состояние модуля, регистрацию в сети, и прочее
	}

	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_GSMBUSY_0);								// Разрешение всех входящих звонков
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_CLIP);										// Включаем АОН
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_CLCC);										// Переключаем на сокращенный ответ при входящем звонке
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();

	Switch_Programming_mode();								// Вызываем конечный автомат режима программирования

	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_GSMBUSY_1);								// Запрет всех входящих звонков
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
*/
	programming_mode = 1;

	while (simcom_init_mode != 2)							// Пока модуль SIMCOM не настроится на прием входящих звонков
	{
		CheckSIMCOM();										// Выполняем команды автомата
	}

	Switch_Programming_mode();								// Вызываем конечный автомат режима программирования

	ATOMIC_BLOCK(ATOMIC_FORCEON)							// Если произошел выход из автомата, значит есть 3 номера в ОЗУ. Копируем их из ОЗУ в EEPROM
	{
		led_delay = 0;										// Прекратим мигание светодиода программирования (LED_PROG)		
	#if defined (DEBUG)
		LED_PORT &= ~(1<<LED_PROG);							// ТОЛЬКО ДЛЯ ОТЛАДКИ
	#else
		LED_PORT |= 1<<LED_PROG;							// Зажигаем светодиод программирования			
	#endif
		eeprom_update_block(number1,ee_number1,14);			
		eeprom_update_block(number2,ee_number2,14);
		eeprom_update_block(number3,ee_number3,14);
	}
	#if defined (DEBUG)
		LED_PORT |= 1<<LED_PROG;							// ТОЛЬКО ДЛЯ ОТЛАДКИ
	#else
		LED_PORT &= ~(1<<LED_PROG);							// Гасим светодиод программирования
	#endif
	
	pin_state = JUMPER_PINS;								// Читаем состояние всего порта c Джампером программирования
	while(!(pin_state & (1<<JUMPER_PIN)))
	{
		wdt_reset();
	}														// Ждем возвращения Джампера программирования в положение "РАБ"		
}
//=====================================================================================================================================================
// Конечный автомат записи номеров дозвона. В зависимости от значения programming_mode, ожидает входящего звонка, либо записывает номер звонящего в ОЗУ
void Switch_Programming_mode(void)													
{
	parsing_result = BAD;

	while (programming_mode != 7)							// Пока не запишем 3 звонящих номера
	{
		wdt_reset();		
		switch (programming_mode)							// Гоняем конечный автомат записи номеров дозвона
		{
			case 1:
			{
				Wait_RING(1000, 2);							// Мигаем светодиодом LED_PROG с частотой 1 раз в 2 секунды
				break;
			}
			case 2:
			{			
				SaveNumber_2_RAM(number1, 3);				// Сохраняем номер 1-го абонента в ОЗУ, если он принят
				_delay_ms(500);
				break;
			}
			case 3:
			{
				Wait_RING(500, 4);							// Мигаем светодиодом LED_PROG с частотой 1 раз в секунду
				break;
			}
			case 4:
			{
				SaveNumber_2_RAM(number2, 5);				// Сохраняем номер 2-го абонента в ОЗУ, если он принят
				_delay_ms(500);
				break;
			}
			case 5:
			{			
				Wait_RING(250, 6);							// Мигаем светодиодом LED_PROG с частотой 2 раза в секунду
				break;
			}
			case 6:
			{			
				SaveNumber_2_RAM(number3, 7);				// Сохраняем номер 3-го абонента в ОЗУ, если он принят
				_delay_ms(500);
				break;
			}
			default: programming_mode = 1; break;
		}
	}
}
//=====================================================================================================================================================
// Активация парсинга признака входящего звонка - "RING", либо переключение на прием номера звонящего абонета в случае входящего звонка
void Wait_RING(unsigned int _led_delay, unsigned char next_programming_mode)// В качестве параметров передаються: следующее состояние автомата программирования номеров дозвона и период мигания светодиода LED_PROG
{
	if (parsing_result == BAD)								// Если входящего звонка нет
	{
		ActivateParsing(RING,WAIT_INCOMING_CALL_TIME);		// Активируем ожидание входящего звонка в течении Х секунд, потом перезупеск
	}
	if (parsing_result == OK)								// Если распознан входящий звонок
	{							
		ppk_mode = PROG;									// Включаем режим записи тел.номера
		UCSRB |= 1<<RXCIE;									// Разрешаем прерывание по приходу байта - запись номера началась
		programming_mode = next_programming_mode;			// Переводим автомат в состояние ожидания окончания записи очередного номера абонента в приемный буффер					
	}
	if (led_delay == 0)
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			led_delay = _led_delay;							// Мигаем светодиодом LED_PROG с нужной частотой
		}
		LED_PORT ^= 1<<LED_PROG;					
	}
}
//=====================================================================================================================================================
// Сохранение номера звонящего абонента в ОЗУ
void SaveNumber_2_RAM(char *number, unsigned char next_programming_mode)// В качестве параметров передаються: указатель на 1 символ номера абонента и следующее состояние автомата программирования номеров дозвона
{
	if (buffer_index == buffer_max-1)						// Если приемный буффер полный - приняли весь телефонный номер звонящего
	{
		for	(buffer_index = 0; buffer_index != buffer_max-1; buffer_index ++)// Копирем номер звонящего из приемного буффера в строку numberX
		{
			number[buffer_index] = buffer[buffer_index];
		}
		SendStr_P(ATH);										// Отклоняем входящий вызов
		buffer_index = 0;									// Обнуляем указатель массива, чтобы писАть в буффер сначала
		ppk_mode = GUARD_OFF;								// Включаем режим парсинга команд
		parsing_result = BAD;								// Активируем ожидание следующего звонка в следующем шаге конечного автомата
		programming_mode = next_programming_mode;			// Переводим автомат в состоние ожидания следующего звонка 
	}
}
//=====================================================================================================================================================
// Активации парсинга строки. Сам парсинг происходит в обработчике USART_RXC_vect. Контроль времени парсинга в обработчике TIMER1_COMPA_vect
void ActivateParsing(const char *string, unsigned int _parsing_delay)// На входе указатель на 1 символ строки, и время парсинга строки в мс
{
	unsigned char temp;

	parsing_result = IN_PROCESS;							// Начинаем парсинг с обнуления признака успешного парсинга/ошибки парсинга	
	parsing_pointer = string;								// Копируем указатель на 1 сивол строки, которую будем парсить в глобальную переменную 
	ATOMIC_BLOCK(ATOMIC_FORCEON){parsing_delay = _parsing_delay;}// Задаем максимальное время парсинга строки. Сам парсинг может закончиться и раньше
	temp = UDR;												// Читаем приемник, чтобы сбросить флаг прерывания от всякого мусора, который там был до этого	
	UCSRB |= 1<<RXCIE;										// Разрешаем прерывание по приходу байта - парсинг начался							
/*
	while (parsing_result == IN_PROCESS)					// Ждем успешного окончания, либо ошибки парсинга
	{
		_delay_ms(1);										// Задержка
		parsing_delay --;
		if (parsing_delay == 0)								// Если время парсинга вышло
		{
			UCSRB &= ~(1<<RXCIE);							// Запрещаем прерывание по приходу байта, чтобы не отвлекаться на всякую дрянь
			return BAD;										// И вываливаемся из цикла с ошибкой парсинга
		}
	}
	if (parsing_result == 1) return OK;
	else return BAD;
*/
}
//=====================================================================================================================================================
// Чтение записанных телефонных номеров из EEPROM в ОЗУ
void ReadNumbers(void)
{
	eeprom_read_block(number1,ee_number1,14);				// Прочесть строку Number_1 из EEPROM, в строку Number1
	eeprom_read_block(number2,ee_number2,14);				// Прочесть строку Number_2 из EEPROM, в строку Number2
	eeprom_read_block(number3,ee_number3,14);				// Прочесть строку Number_3 из EEPROM, в строку Number3
}
//=====================================================================================================================================================
// Отправка строки из флеша в UART
void SendStr_P(const char *string)							// На входе указатель на символ строки
{
	while (pgm_read_byte(string) != '\0')					// Пока байт строки не 0 (конец строки)
	{
		SendByte(pgm_read_byte(string++));					// Мы продолжаем слать строку, не забывая увеличивать указатель, выбирая следующий символ строки
	}
}
//=====================================================================================================================================================
// Отправка строки из ОЗУ в UART
void SendStr(char *string)									// На входе указатель на символ строки
{
	while (*string != '\0')									// Пока байт строки не 0 (конец строки)
	{
		SendByte(*string++);								// Мы продолжаем слать строку, не забывая увеличивать указатель, выбирая следующий символ строки
	}
}
//=====================================================================================================================================================
// Отправка одного символа строки в UART
void SendByte(char byte)									// На входе байт
{
	while(!(UCSRA & (1<<UDRE)));							// Ждем флага готовности UART
	UDR=byte;												// Засылаем байт в UART
}
//=====================================================================================================================================================
// Мигание светодиодом LED_WORK
void Blink_LED_WORK(void)
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		led_delay = 500;
	}
	LED_PORT ^= 1<<LED_WORK;
}
//=====================================================================================================================================================
// Выключение сирены и выходов, если пришло время
void Siren_Outs_OFF(void) 
{
	if (siren_delay == 0)									// Если нет отсчета время звучания сирены
	{
	#if defined (DEBUG)
		OUT_PORT |= 1<<SIREN;								// ТОЛЬКО ДЛЯ ОТЛАДКИ
	#else
		OUT_PORT &= ~(1<<SIREN);							// Выключим сирену
	#endif
	}

	if (out_delay == 0)										// Если нет отсчета время активности выходов
	{
	#if defined (DEBUG)
		OUT_PORT |= 1<<OUT_1;								// ТОЛЬКО ДЛЯ ОТЛАДКИ
	#else			
		OUT_PORT &= ~(1<<OUT_1);							// Выключим выходы OUT_1,			
	#endif
		OUT_PORT |= 1<<OUT_2;								// и OUT_2 (инверсная логика работы)
	}
}
//=====================================================================================================================================================
// Инициализация портов и периферии
void Init(void)
{
// Инициализация портов
	SIMCOM_RESET_DDR |= 1<<SIMCOM_RESET_PIN;				// SIMCOM_RESET на вывод
	DDRB |= 1<<LED_PROG|1<<LED_WORK|1<<OUT_2|1<<OUT_1|1<<SIREN;// Сирену, выходы, и светодиоды - на вывод
	
#if defined (DEBUG)
	PORTD |= 1<<JUMPER_PIN|1<<DATCHIK_2|1<<DATCHIK_1;		// ТОЛЬКО ДЛЯ ОТЛАДКИ
#else
	PORTD |= 1<<JUMPER_PIN|1<<DATCHIK_1;					// Включаем подтяжку для Джампера входа в режим программирования и DATCHIK_1 ( DATCHIK_2 не надо !!! )
#endif
	PORTB |= 1<<BUTTON_PIN|1<<OUT_2;						// Включаем подтяжку для Кнопки постановки/снятия, и активируем OUT_2 (выключиться при тревоге)

#if defined (DEBUG)
	LED_PORT |= 1<<LED_WORK|1<<LED_PROG;					// ТОЛЬКО ДЛЯ ОТЛАДКИ
#else
	LED_PORT &= ~(1<<LED_WORK|1<<LED_PROG);					// Выключим светодиоды
#endif

// Инициализация UART
	UBRRL = LO(bauddivider);
	UBRRH = HI(bauddivider);
	UCSRA = 0;
	UCSRB = 0<<RXCIE|0<<TXCIE|0<<UDRIE|1<<RXEN|1<<TXEN;		// Прерывание UDRIE сразу никогда не разрешаем, иначе контроллер сразу входит в это прерывание

#if defined (__AVR_ATmega8__)
	UCSRC = 1<<URSEL|1<<UCSZ1|1<<UCSZ0;
#endif

#if defined (__AVR_ATtiny2313__)||(__AVR_ATtiny2313A__)
	UCSRC = 1<<UCSZ1|1<<UCSZ0;
#endif

// Инициализация Таймеров
#if defined (__AVR_ATmega8__)
// Предделитель подбираеться экспериментально, для достижения периода прерывания Timer1 каждую мс
															// Везде вкл. режим прерывания Timer1 по совпадению (1<<WGM12)
	TCCR1B = 1<<WGM12|0<<CS12|0<<CS11|1<<CS10;				// Запуск таймера без предделителя
//	TCCR1B = 1<<WGM12|0<<CS12|1<<CS11|0<<CS10;				// Запуск таймера с предделителем 8
//	TCCR1B = 1<<WGM12|0<<CS12|1<<CS11|1<<CS10;				// Запуск таймера с предделителем 64
//	TCCR1B = 1<<WGM12|1<<CS12|0<<CS11|0<<CS10;				// Запуск таймера с предделителем 256
//	TCCR1B = 1<<WGM12|1<<CS12|0<<CS11|1<<CS10;				// Запуск таймера с предделителем 1024				
	OCR1A = 7999;											// Значение подбираеться экспериментально, для достижения периода прерывания Timer1 каждую мс
	TIMSK = 1<<OCIE1A;										// Разрешаем прерывание по совпадению значения OCR1A с заданым ранее
	TIFR = 1<<OCF1A;										// Сбросим флаг, чтобы прерывание не выскочило сразу
#endif

#if defined (__AVR_ATtiny2313__)||(__AVR_ATtiny2313A__)
// Предделитель подбираеться экспериментально, для достижения периода прерывания Timer0 каждую мс
	TCCR0A = 1<<WGM01;										// Вкл. режим прерывания Timer0 по совпадению
//	TCCR0B = 0<<CS02|0<<CS01|1<<CS00;						// Запуск таймера без предделителя
//	TCCR0B = 0<<CS02|1<<CS01|0<<CS00;						// Запуск таймера с предделителем 8
	TCCR0B = 0<<CS02|1<<CS01|1<<CS00;						// Запуск таймера с предделителем 64
//	TCCR0B = 1<<CS02|0<<CS00|0<<CS00;						// Запуск таймера с предделителем 256
//	TCCR0B = 1<<CS02|0<<CS00|1<<CS00;						// Запуск таймера с предделителем 1024
	OCR0A = 124;											// Значение подбираеться экспериментально, для достижения периода прерывания Timer0 каждую мс
	TIMSK = 1<<OCIE0A;										// Разрешаем прерывание по совпадению значения OCR0A с заданым ранее
#endif

// Настроим внешние прерывания (DATCHIK_1, DATCHIK_2)
#if defined (DEBUG)
	MCUCR |= 1<<ISC11|1<<ISC01;								// ТОЛЬКО ДЛЯ ОТЛАДКИ
#else														
	MCUCR |= 1<<ISC11|1<<ISC10|1<<ISC01;					// INT1 - прерывание по переднему фронту (DATCHIK_2), INT0 - прерывание по спаду (DATCHIK_1)				
#endif

// Настроим Аналоговый компаратор
	if (!(pin_state & (1<<PD7)))							// Если на момент инициализации на пине низкий уровень, значит нет сети 220В,				
	ACSR |= 1<<ACBG|1<<ACIE|1<<ACIS1|1<<ACIS0;				// поэтому подключаем внутренний ИОН, разрешаем прерывание от компаратора, условие возникновения прерывания - переход выхода компаратора с 0 на 1.
	else ACSR |= 1<<ACBG|1<<ACIE|1<<ACIS1;					// иначе есть сеть 220В, поэтому подключаем внутренний ИОН, разрешаем прерывание от компаратора, условие возникновения прерывания - переход выхода компаратора с 1 на 0.
}
//=====================================================================================================================================================
// Прерывание по опустошению буффера передатчика UART
/*
ISR (USART_UDRE_vect)		
{
	buffer_index ++;										// Увеличиваем индекс
 
	if (buffer_index == buffer_MAX)							// Вывели весь буффер? 
	{
		UCSRB &= ~(1<<UDRIE);								// Запрещаем прерывание по опустошению - передача закончена
	}
	else 
	{
		UDR = buffer[buffer_index];							// Берем данные из буффера. 
	}
}
*/
//=====================================================================================================================================================
// Пример работы со строками из ОЗУ
/*
	char String_AT[] = "AT\r\n";							// Организуем в памяти массив-строку
	char *u;												// И про указатель не забываем
	u=String_AT;											// Присваиваем указателю адрес начала строки. Оператор взятия адреса "&" в данном случае не нужен 
	SendStr(u);												// Обычно
	SendStr("AT\r\n");										// Инлайн
*/
// Пример работы со строками из флеша инлайн
//	SendStr_P(PSTR("AT\r\n"));
