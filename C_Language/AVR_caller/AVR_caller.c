#include <AVR_caller.h>

// ��������� ���������� ���������� � �������
// ��� ��� � ������������ ���������� - volatile
// ��� ��� ������������� ������ � 1 ������� - static

#define 	buffer_max			14							// ����� ��������� ������� = ������ ���. ������ � ������� "+38 0XX XXX XX XX" = 13 + 1 ��� ��������� ������
volatile	unsigned	char	buffer[buffer_max];			// ��� �������� ������
volatile	unsigned	char	buffer_index = 0;			// ������� ������� ��������� �������
static					char	number1[buffer_max];		// ������ 1 ���. ������ � ������� "+38 0XX XXX XX XX" + 1 ��� ��������� ������
static					char	number2[buffer_max];		// ������ 2 ���. ������ � ������� "+38 0XX XXX XX XX" + 1 ��� ��������� ������
static					char	number3[buffer_max];		// ������ 3 ���. ������ � ������� "+38 0XX XXX XX XX" + 1 ��� ��������� ������
static		unsigned	char	pin_state;					// ���������� ��� ���������� ��������� ����� ������-���� �����
static		unsigned	char	programming_mode = 0;		// ��������� ��������� �������� ���������������� ������� �������
static		unsigned	char	parsing_fault = NUM_OF_ATTEMPT+1;// ������� ������ ��������
static		unsigned	char	simcom_mode = 1;			// ��������� ��������� �������� �������� ��-������
static		unsigned	char	simcom_init_mode = 0;		// ��������� ������������� ������ SIMCOM (0 - �� ����� ������� � ����� SMS, 1 - ����� ������� � ����� SMS) 
static		unsigned	int 	check_button_counter;		// ������� ���������� ������ ������ ������ ����������/������ �� ������� ������ ������
volatile	unsigned	int		debounce_delay = 0;			// �������� ������� ������ ������ ����������/������
volatile	unsigned	int		parsing_delay = 0;			// �������� �� ����� ��������
volatile	unsigned	int		exit_delay = 0;			// �������� �� ����/�����
volatile	unsigned	int		led_delay = 0;				// �������� �� ����� ��������� ����������� LED_WORK ��� LED_PROG
volatile	unsigned	int		siren_delay = 0;			// �������� �� �������� ������
volatile	unsigned	int		out_delay = 0;				// �������� �� ���������� ������� ����� ���������
volatile	unsigned	char	parsing_result = BAD;		// ��������� �������� ������ (0-������� �������������, 1-������� ������� ��������, 2-������ �������� ��� ��������� ����� ��������)
volatile	unsigned	char	ppk_mode = GUARD_OFF;		// ����� ������ �������
volatile	unsigned	char	flags = 0;
#define 	sms_flag			0

const	char *volatile	parsing_pointer;					// ��������� �� ������ �� �����, � ������� ����� ���������� �������� ������

// ������� ������ AT ������ �� �����. ����������� ��� main
const	char AT[] 		PROGMEM = "AT\r";
const	char ATE0[] 	PROGMEM = "ATE0\r";					// ��������� ���
const	char AT_IPR[]	PROGMEM = "AT+IPR=9600\r";			// ����������� �������� ������
const	char AT_CLCC[]	PROGMEM	= "AT+CLCC=0\r";			// ����������� �� ����������� ����� ��� �������� ������
const	char AT_CMGF[]	PROGMEM = "AT+CMGF=1\r";			// ��������� ����� ���
const	char AT_CLIP[]	PROGMEM = "AT+CLIP=1\r";			// �������� ���
const	char AT_CPAS[]	PROGMEM = "AT+CPAS\r";				// ������� ��������� ������
const	char AT_CREG[]	PROGMEM = "AT+CREG?\r";				// ������� ����������� � ����
const	char AT_CCALR[]	PROGMEM = "AT+CCALR?\r";			// ��������� ����������� ��������� ������
const	char ATD[]		PROGMEM = "ATD";					// ������
const	char RING_END[]	PROGMEM = ";\r";
const	char AT_CMGS[]	PROGMEM = "AT+CMGS=\"";				// ���������� ���
const	char AT_CMGS_2[]PROGMEM = "\"\r";
const	char NO_220[]	PROGMEM = "HET 220B\x1A";
const	char RETURN_220[]PROGMEM ="ECTb 220B\x1A";
const	char ATH[]		PROGMEM = "ATH\r";					// �������� �����
const	char AT_GSMBUSY_1[]	PROGMEM = "AT+GSMBUSY=1\r";		// ������ ���� �������� �������
const	char AT_GSMBUSY_0[]	PROGMEM = "AT+GSMBUSY=0\r";		// ���������� ���� �������� �������

// ������� ������ ������� �� AT ������� �� �����. ����������� ��� main
const	char AT_OK[]	PROGMEM = "AT\r\r\nOK\r\n";
const	char ATE0_OK[]	PROGMEM = "ATE0\r\r\nOK\r\n";
const	char _OK[]		PROGMEM = "\r\nOK\r\n";
const	char CPAS_OK[]	PROGMEM = "\r\n+CPAS: 0\r\n\r\nOK\r\n";
const	char CREG_OK[]	PROGMEM = "\r\n+CREG: 0,1\r\n\r\nOK\r\n";
const	char CCALR_OK[]	PROGMEM = "\r\n+CCALR: 1\r\n\r\nOK\r\n";
const	char RING[]		PROGMEM = "\r\nRING\r\n\r\n+CLIP: \"";
const	char POINTER[]	PROGMEM = "> ";
const	char BUSY[]		PROGMEM = "\r\nBUSY\r\n";

// ������� ������ ��� ���. ������ � EEPROM. ����������� ��� main
unsigned char EEMEM ppk_mode_save = GUARD_OFF;				// ��������� ����� ��������� ��� � EEPROM. ������������� ��� ������ ������ �������.
//unsigned char EEMEM ee_number1[buffer_max] = "+380962581099";// 1 ����� � EEPROM
//unsigned char EEMEM ee_number2[buffer_max] = "+380996755968";// 2 ����� � EEPROM
//unsigned char EEMEM ee_number3[buffer_max] = "+380962581099";// 3 ����� � EEPROM

unsigned char EEMEM ee_number1[buffer_max] = "             ";// 1 ����� � EEPROM
unsigned char EEMEM ee_number2[buffer_max] = "             ";// 2 ����� � EEPROM
unsigned char EEMEM ee_number3[buffer_max] = "             ";// 3 ����� � EEPROM

//=====================================================================================================================================================
// ��������� ��������� �������
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
	Init();													// ������������� ������ � ���������
	ppk_mode = eeprom_read_byte(&ppk_mode_save);			// ��������������� ��������� ��� �� EEPROM �� ���������� ���������, ��� �����������
	sei();
	wdt_enable(WDTO_2S);									// �������� ���������� ������ �� ������� ����� 2 �������

	if (ppk_mode == GUARD_OFF)								// ���� ��� ���� � ������								
	{
		pin_state = JUMPER_PINS;							// ������ ��������� ����� ����� c ��������� ����������������
		if (!(pin_state & (1<<JUMPER_PIN))) Programming();	// ���� ������� ���������������� � ��������� ���� (����� JUMPER_PIN �� �����), ��������� � ����� "����������������"
	}
	programming_mode = 0;

	wdt_reset();
	ReadNumbers();											// ������ ���������� ���������� ������ �� EEPROM � ���

	if (ppk_mode != GUARD_OFF)								// ���� �� � ������ "����� � ������"
	{
		GIFR = 1<<INTF1|1<<INTF0;							// ������� ����� �������� ����������� ����� ������� ����������
		GICR = 1<<INT1|1<<INT0;								// �������� ���������� INT1 � INT0
	#if defined (DEBUG)
		LED_PORT &= ~(1<<LED_WORK);							// ������ ��� �������
	#else
		LED_PORT |= 1<<LED_WORK;							// ������� ��������� ������
	#endif
	}

//=====================================================================================================================================================
// ������� ����
	while (1)
	{
		CheckButton(5000);									// ��������� ������ ����������/������ ������ 5000-� ������ �������� �����
		CheckSIMCOM();										// ��������� ��������� ������, ����������� � ����, � ������
		Siren_Outs_OFF();									// ��������� ������ � ������, ���� ���� - ���������
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_IN)&&(exit_delay == 0))	// ���� ��� ���������� � ��������� "�������� �� ����" � �������� �������
		{
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				ppk_mode = ALARM_ACTIVE;					// ���������� ����� "������� �������"
				eeprom_update_byte(&ppk_mode_save, ALARM_ACTIVE);// ��������� ��������� ��� � EEPROM
			}
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if (ppk_mode == ALARM_ACTIVE)						// ���� �������� �������
		{
		#if defined (DEBUG)
			OUT_PORT &= ~(1<<SIREN|1<<OUT_2|1<<OUT_1);		// ������ ��� �������
		#else
			OUT_PORT |= 1<<SIREN|1<<OUT_1;					// ������� ������, OUT_1
			OUT_PORT &= ~(1<<OUT_2);						// � OUT_2 (��������� ������ ������)
		#endif

			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				siren_delay = SIREN_TIME;					// ������ ����� �������� ������
				out_delay = OUT_TIME;						// ������ ����� ���������� �������
				ppk_mode = ALARM_SIREN_COMPL;				// ������ ���� �������� �� ������ �����, ������ ��������� ���
				eeprom_update_byte(&ppk_mode_save, ALARM_SIREN_COMPL);// ��������� ��������� ��� � EEPROM
			}
			GIFR = 1<<INTF1|1<<INTF0;						// ������� ����� ����������� ����� ����������
			GICR = 1<<INT1|1<<INT0;							// �������� ���������� INT1 � INT0				
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == ALARM_SIREN_COMPL)&&(simcom_init_mode == 1))// ���� ���� �������� ������, � ������ SIMCOM ���������� � ������� ������, �������� �������
		{
			Ring();											// ������
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if (((ppk_mode != GUARD_OFF)&&(ppk_mode != GUARD_ON))&&(led_delay == 0))// ���� ����� "�������" ��� "��������"
		{
			Blink_LED_WORK();								// ������ LED_WORK
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_OUT)&&(exit_delay == 0))	// ���� ��� ���������� � ��������� "�������� �� �����" � �������� �������
		{
				ppk_mode = GUARD_ON;						// ���������� ����� "��� �������"
				GIFR = 1<<INTF1|1<<INTF0;					// ������� ����� �������� ����������� ����� ����������
				GICR = 1<<INT1|1<<INT0;						// �������� ���������� INT1 � INT0
				ATOMIC_BLOCK(ATOMIC_FORCEON)
				{
					led_delay = 0;							// ���������� ������ LED_WORK
				}
			#if defined (DEBUG)
				LED_PORT &= ~(1<<LED_WORK);					// ������ ��� �������
			#else
				LED_PORT |= 1<<LED_WORK;					// �������� ��������� ������
			#endif
		}
//-----------------------------------------------------------------------------------------------------------------------------------------------------			
		if ((ppk_mode == DELAY_IN)&&(exit_delay == IN_DELAY))// ���� ��� ���������� � ��������� "�������� �� ����" � ��� ������ ��������
		{
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				eeprom_update_byte(&ppk_mode_save, DELAY_IN);// ��������� ��������� ��� � EEPROM
			}
		}		
	}
}
//=====================================================================================================================================================
ISR (USART_RXC_vect)										// ���������� �� ������� ����� � ������ UART
{
	if (ppk_mode == PROG)									// ���� ������ � ������ "����������������" (������ ���������� �������)
	{
		buffer[buffer_index] = UDR;							// ������ ����� ������ (���������� ����� ���������) � ������		
		buffer_index++;										// ����������� ������
		if (buffer_index == buffer_max-1)					// ���� �������� ����� ������� 
		{
			buffer[buffer_index] = '\0';					// ������� ������� ����� ������
			UCSRB &= ~(1<<RXCIE);							// � �������� ���������� �� ������� ����� - ����� ��������� ���������� � ������
		}			
	}

	else													// ���� ������ ������ ���������� �������, ������ ������ �����������
	{
		if (UDR == pgm_read_byte(parsing_pointer))			// ���������� �������� ���� � �������� �� ������  
		{													// ���� ���������		
			parsing_pointer++;								// ����������� ���������, ������� ��������� ������ ������
			if (pgm_read_byte(parsing_pointer) == '\0')		// ���� ��������� ���� ������ 0 (����� ������)
			{			
				parsing_result = OK;						// ������������� ������� ��������� ��������� ��������
				UCSRB &= ~(1<<RXCIE);						// ��������� ���������� �� ������� ����� - ����� ��������			
			}
		}

		else parsing_result = BAD;							// ���� �� ��������� - ���������� ������� ������ ��������. �� 0, ����� ����� ������������� ��������� �������
	}
}
//=====================================================================================================================================================
ISR (TIMER1_COMPA_vect)										// ���������� �� ���������� Timer1
{
	if (parsing_delay != 65535)								// ���� ������� ������� �������� �� �������� (������� ������������� �������� � �������)
	{		
		if (parsing_delay != 0) parsing_delay--;
		else
		{
			if (parsing_result != OK)						// ����� �������� ���������, ���� �� ��� ���������� ������� ��������� ��������
			{
				parsing_result = BAD;						// ������������� ������� ����������� ��������
				UCSRB &= ~(1<<RXCIE);						// ��������� ���������� �� ������� ����� - ����� �� ����������� �� ������ �����
			}
			parsing_delay--;								// ��������� ������� ������� �������� (������� ������������� �������� � �������)
		}
	}
/*
	if (led_delay != 65535)									// ���� ������� ����������� �� ��������� (������� ������������� �������� � �������)
	{
		if (led_delay != 0) led_delay--;
		else 
		{
			if (ppk_mode == PROG) LED_PORT ^= 1<<LED_PROG;	// ����������� ������ ��������� � ����������� �� ������ ������ (���������� 50%)
			else LED_PORT ^= 1<<LED_WORK;
			led_delay--;									// ��������� ������ ������� ������� ����������� (������� ������������� �������� � �������)
		}
	}
*/	
	if (debounce_delay != 0) debounce_delay--;				// ������ ������� ������� ������ ������ ����������/������ ����� ����������� �������
	if (exit_delay != 0) exit_delay--;						// ������ �������� ����/�����, ���� ��� ����
	if (siren_delay != 0) siren_delay--;					// ������ ������� �������� ������
	if (out_delay != 0) out_delay--;						// ������ ������� ��������� �������
	if (led_delay != 0) led_delay--;						// ������ ������� ������� �����������
}
//=====================================================================================================================================================
ISR (INT0_vect)												// ���������� �� INT0
{
	if (ppk_mode == GUARD_ON)								// ���� ��� � ������ "��� �������" (������ �� ����� ������� �� ����)
	{
		ppk_mode = DELAY_IN;								// ��������� ��� � ��������� �������� �� ����
		exit_delay = IN_DELAY;
	} 
	else ppk_mode = ALARM_ACTIVE;							// ����� ����� ��������� ��������� �������	
	GICR = 0<<INT1|0<<INT0;									// �������� ���������� INT1 � INT0
}
//=====================================================================================================================================================
ISR (INT1_vect)												// ���������� �� INT1
{
	if (ppk_mode == GUARD_ON)								// ���� ��� � ������ "��� �������" (������ �� ����� ������� �� ����)
	{
		ppk_mode = DELAY_IN;								// ��������� ��� � ��������� �������� �� ����
		exit_delay = IN_DELAY;
	} 
	else ppk_mode = ALARM_ACTIVE;							// ����� ����� ��������� ��������� �������
	GICR = 0<<INT1|0<<INT0;									// �������� ���������� INT1 � INT0
}
//=====================================================================================================================================================
ISR (ANA_COMP_vect)											// ���������� �����������, ��������� ��� ������� ���� 220�
{
	flags |= 1<<sms_flag;									// ��������� ���� ������������� �������� SMS
	ACSR &= ~(1<<ACIE);										// �������� ���������� �� ����������� ��� ����������� �������� SMS
	ACSR ^= 1<<ACIS0;										// ������ ������� ������������� ���������� - ���� ��� ������� � 0 �� 1, ������ ������� � 1 �� 0 � ��������
}
//=====================================================================================================================================================
// �������� ������� �������. � ����������� �� �������� ring_mode, ������������ ��������� �����, ���� ������� ������� ��������
void Ring(void)
{
	unsigned char ring_mode = 1;							// ��������� �������� ����������� ������������� ������ ���� ������, ������� ���������� ���������

	while (parsing_result == IN_PROCESS){}					// ���� ��������� �������� ���������� �������

	while (ring_mode != 16)									// ���� ������� �� �������� � ��������� "������� ������� �� ��� ������ ������������"
	{
		wdt_reset();
		Siren_Outs_OFF();									// ��������� ������ � ������, ���� ���� - ���������

		if (led_delay == 0) Blink_LED_WORK();				// ������ LED_WORK

		CheckButton(10000);									// ��������� ������ ����������/������ ������ 10000-� ������ ����� ������� �� �������,
															// ����������� ����� ������� LED_WORK �� �������, ����� ������� ��������������� ��������� ������ ����� ������ � ������
		if ((ppk_mode == GUARD_OFF)&&(ring_mode != 15))		// ���� ��� ��� ��������� � ��������� "����� � ������" � ��� ������������� ������
		{
			ring_mode = 14;									// ���������� ������
		}
						
		switch (ring_mode)									// ������������ ������ �� ��� ������
		{
			case 1:
			{
				Ring_on_Number(number1);					// ������ 1 ��������				
				ring_mode = 2;								// ��������� ������� � ��������� "�������� ������ ������ �� ������ 1 ��������"
				break;
			}
			case 2:
			{
				if (parsing_result != IN_PROCESS)			// ���� ������ ����� �� ������
				{
					ring_mode = 3;							// ������ ������� ������	
				}

				if (parsing_result == OK)					// � ���� ����� ������ (������� OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// ���������� ������� ������ ��������
				}
				break;
			}
			case 3:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					SendStr_P(ATH);							// ����� ������					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 4;							// ��������� ������� � ��������� "�������� ����� ������"
				}
				break;
			}
			case 4:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					ring_mode = 5;							// ��������� ������� � ��������� "������ 2 ��������"
				}
				break;
			}
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 5:
			{
				Ring_on_Number(number2);					// ������ 2 ��������				
				ring_mode = 6;								// ��������� ������� � ��������� "�������� ������ ������ �� ������ 2 ��������"
				break;
			}
			case 6:
			{
				if (parsing_result != IN_PROCESS)			// ���� ������ ����� �� ������
				{
					ring_mode = 7;							// ������ ������� ������	
				}

				if (parsing_result == OK)					// � ���� ����� ������ (������� OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// ���������� ������� ������ ��������
				}
				break;
			}
			case 7:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					SendStr_P(ATH);							// ����� ������					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 8;							// ��������� ������� � ��������� "�������� ����� ������"
				}
				break;
			}
			case 8:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					ring_mode = 9;							// ��������� ������� � ��������� "������ 3 ��������"
				}
				break;
			}		
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 9:
			{
				Ring_on_Number(number3);					// ������ 3 ��������				
				ring_mode = 10;								// ��������� ������� � ��������� "�������� ������ ������ �� ������ 3 ��������"
				break;
			}
			case 10:
			{
				if (parsing_result != IN_PROCESS)			// ���� ������ ����� �� ������
				{
					ring_mode = 11;							// ������ ������� ������	
				}

				if (parsing_result == OK)					// � ���� ����� ������ (������� OK)
				{
					ActivateParsing(BUSY,RING_WAIT_TIME);	// ���������� ������� ������ ��������
				}
				break;
			}
			case 11:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					SendStr_P(ATH);							// ����� ������					
					ActivateParsing(_OK,AT_WAIT_TIME);
					ring_mode = 12;							// ��������� ������� � ��������� "�������� ����� ������"
				}
				break;
			}
			case 12:
			{
				if (parsing_result != IN_PROCESS)			// ���������� �� ������ ������
				{
					ring_mode =13;							// ��������� ������� � ��������� "��� ������ ������������"
				}
				break;
			}
//----------------------------------------------------------------------------------------------------------------------------------------------------
			case 13:
			{
				ATOMIC_BLOCK(ATOMIC_FORCEON)
				{
					ppk_mode = ALARM_RING_COMPL;			// ��� ������ ������������. ���������� ����� ��� "�������, ������ ���������"
					eeprom_update_byte(&ppk_mode_save, ALARM_RING_COMPL);// � ��������� ��������� ����� ��������� ��� � EEPROM
				}
				ring_mode = 16;								// ��������� ������� � ��������� "������� ������� �� ��� ������ ������������"
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
					LED_PORT |= 1<<LED_WORK;				// ������ ��� �������
				#else					
					LED_PORT &= ~(1<<LED_WORK);				// ����� ��������� ������
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
// ������ ����������� ��������
void Ring_on_Number(char *number)							// � �������� ��������� ����������� ��������� �� 1 ������ ������ ��������
{
	SendStr_P(ATD);											// ������ ��������
	SendStr(number);
	SendStr_P(RING_END);
	ActivateParsing(_OK,RING_WAIT_TIME);					// ���������� �������� ������
}
//=====================================================================================================================================================
// ������� �������� ������ ����������/������
void CheckButton(unsigned int button_counter_delay)			// ���������� ������ ����������/������ ������ �������, ����� �� ��������� ������
{															// � �������� ��������� ����������� ���������� ��������� ������ (������ �������, �� ������� ��� ������� ����� ������) �� ������������ ������ ������
	if (debounce_delay == 0)								// ���� ��� ������� �� ����� ������ ����������/������
	{
		check_button_counter--;							
		if (check_button_counter == 0)
		{
			pin_state = BUTTON_PINS;						// ������ ��������� ����� �����
			if (!(pin_state & (1<<BUTTON_PIN)))				// ���� ������ ����������/������ ������, ���������� ������� � ������ �����
			{
				if (ppk_mode == GUARD_OFF)					// ���� ������� ����� "����� � ������"						
				{
					ppk_mode = DELAY_OUT;					// ���������� ����� "�������� �� �����"
					ATOMIC_BLOCK(ATOMIC_FORCEON)
					{
						eeprom_update_byte(&ppk_mode_save, DELAY_OUT);// � EEPROM ����� ��������� "��� �������", ����� ��� ������������ ��� �� ����� �������� �� ����� �������� ���������� ������
						exit_delay = OUT_DELAY;			// �������� �������� �� �����, ������ ������� ����������, ��� �����������
					}
				#if defined (DEBUG)
					LED_PORT &= ~(1<<LED_WORK);				// ������ ��� �������
				#else
					LED_PORT |= 1<<LED_WORK;				// �������� ��������� ������			
				#endif
				}

				else										// ����� ������� ����� "��� �������" ���� "�������"
				{			
					ppk_mode = GUARD_OFF;					// ���������� ������� � ����� "����� � ������"						
					GICR = 0<<INT1|0<<INT0;					// �������� ���������� INT1 � INT0
					ATOMIC_BLOCK(ATOMIC_FORCEON)
					{
						eeprom_update_byte(&ppk_mode_save, GUARD_OFF);// ������� ��������� ��� � EEPROM
						siren_delay = 0;					// ������� ����� �������� ������, ���� ������ ����������� � ������� �����
						out_delay = 0;						// ������� ����� ���������� �������, ���� ������ ����������� � ������� �����						
						led_delay = 0;						// ���������� ������ ����������� LED_WORK (������), ���� �� �����. ��� ����� ��� ��������������� ��������						
					}					
					#if defined (DEBUG)
						LED_PORT |= 1<<LED_WORK;			// ������ ��� �������
					#else					
						LED_PORT &= ~(1<<LED_WORK);			// ����� ��������� ������
					#endif
				}		

				debounce_delay = 1000;						// ��������� ������� �� ������� ������ ����������/������ �� 1 ���, ��� ���������� ������� ��������
			}

			check_button_counter = button_counter_delay;	// ��������� ������� ������ ������ ����������/������
		}
	}
}
//=====================================================================================================================================================
// ������� ������ SIMCOM. �������� ������ ������� �������. ��� ������������ ������ �� 5 �������� ������ - ������������ ������ SIMCOM � ���������� ��� ������ �����������������
void CheckSIMCOM(void)									
{
	wdt_reset();
								
	if ((parsing_result == OK)&&(parsing_delay == 65535))	// ���� ���������� ������� ���������� �������, � ������� ����� �������� (����� ����� ��������� ��-�������)
	{
		parsing_fault = NUM_OF_ATTEMPT;						// ������� ������� ������ ��������
		switch (simcom_mode)								// ���������� ��������� �������� SwitchSIMCOM_mode ��� �������� ��������� �������
		{
			case 1: break;									// ���� ������ ����� � �������, ������������� ������
			case 2: simcom_mode = 3; break;					// ���� �������� ����� �� ��, ���� ATE0
			case 3: simcom_mode = 4; break;					// ���� �������� ����� �� ���0, ���� AT+IPR
			case 4: simcom_mode = 5; break;					// ���� �������� ����� �� AT+IPR, ���� AT+CMGF
			case 5: simcom_mode = 6; break;					// ���� �������� ����� �� AT+CMGF, ���� AT+GSMBUSY=1
			case 6: simcom_mode = 7; break;					// ���� �������� ����� �� AT+GSMBUSY=1, ���� AT+CPAS
			case 7: simcom_mode = 8; break;					// ���� �������� ����� �� AT+CPAS, ���� AT+CREG
			case 8: simcom_mode = 9; break;					// ���� �������� ����� �� AT+CREG, ���� AT+CCALR
			case 9:											// ���� �������� ����� �� AT+CCALR
			{
				simcom_init_mode = 1;						// ������ SIMCOM ������ ������ ������������� � ����� ��������� ������ � ����� SMS
				if (programming_mode == 1)					// ���� ��� � ������ ����������������, ���������� ����. �������������
					 simcom_mode = 12;						// ���� AT+CLCC=0
				else simcom_mode = 7;						// ����� ����� ���� AT+CPAS (� ��� �� ����� ������ ��������� 7-8-9)
				break;
			}
			case 10: simcom_mode = 11; break;				// ���� �������� ������ ������ ���� SMS, ���������� ���� SMS
			case 11:										// ���� ������� ��������� SMS
			{
				ACSR |= 1<<ACI|1<<ACIE;						// �������� ���������� �� ����������� ��� ��������� �������� SMS � ������� 220�	
				simcom_mode = 7;							// ���������� ������� �������� ��-������ �� �������� 1-� ������� ������������ ������ ������ (AT+CPAS)
				break;
			}
			case 12: simcom_mode = 13; break;				// ���� �������� ����� �� AT+CLCC=0, ���� AT+CLIP
			case 13: simcom_mode = 14; break;				// ���� �������� ����� �� AT+CLIP, ���� AT+GSMBUSY=0
			case 14:										// ���� �������� ����� �� AT+GSMBUSY=0
			{
				simcom_init_mode = 2;						// ������ SIMCOM ����� ��������� � ������������ �������� ������ 
				simcom_mode = 15;							// ������ �� ����
				break;
			}
			default: simcom_mode = 1;
		}
		SwitchSIMCOM_mode();								// ���������� ��-�������, �������� ������� ��� ������� ������		
	}

	if ((parsing_result == BAD)&&(parsing_delay == 65535))	// ���� ������� ���������� ���������, � ������� ����� ��������
	{
		if ((simcom_mode == 10)||(simcom_mode == 11))		// � �� �� �������� ������ ����������� ����� ���� SMS, ���� �� ����� �������� ���� SMS
		{
			ACSR |= 1<<ACI|1<<ACIE;							// �������� SMS � ������� 220� �� �������. ������ ������ �� �����, �� ��������� ���������� �����������, �������� ����� ��� ������� ���� 220� � �� ����� ����� ��������
			simcom_mode = 7;								// ���������� ������� �������� ��-������ �� �������� 1-� ������� ������������ ������ ������ (AT+CPAS)
		}

		else
		{			
			parsing_fault--;
			if (parsing_fault == 0)							// ���� ��������� ������� ��������
			{
				simcom_init_mode = 0;						// ���������� ��������� ������������� ������ SIMCOM
				simcom_mode = 1;							// ��������� ������� � ��������� ����� - ������ ����������������� ������ SIMCOM
				parsing_fault = NUM_OF_ATTEMPT;				// ������� ������� ������ ��������
			}
		}
		SwitchSIMCOM_mode();								// �������� ���������� ���������� ��-������� ��� ���������������� ������������ ��������� ��������
	}
}
//=====================================================================================================================================================
// �������� ������� ������ ������������ ��-�������. � ����������� �� �������� simcom_mode, �������� ������������ AT-�������
void SwitchSIMCOM_mode(void)														
{															// ���� ���� ��������� SMS, �������� �������� ����� ��������� ��������
	if ((flags & (1<<sms_flag))&&(simcom_init_mode == 1))	// ���� ���������� ������� ������������� �������� SMS � ������ SIMCOM ������ ������ �������������
	{
		simcom_mode = 10;									// ���������� ������� �������� ��-������ � ����� �������� SMS
		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			flags &= ~(1<<sms_flag);						//  ����� �������� ��������� ������������ �������� � ����� �������� SMS
		}
	}

	switch (simcom_mode)									// ��������� �������� ������������� �������� (������/��������) ������ SIMCOM
	{
		case 1:
		{
			SIMCOM_RESET_PORT &= ~(1<<SIMCOM_RESET_PIN);	// ����� SIMCOM_RESET �� �����
			_delay_ms(SIM800L_RESET_TIME);					// �������� �� Reset ������ SIMCOM
			SIMCOM_RESET_PORT |= 1<<SIMCOM_RESET_PIN;		// ��������� SIMCOM_RESET
			ATOMIC_BLOCK(ATOMIC_FORCEON)
			{
				parsing_delay = WAIT_SIMCOM_READY;			// ������������� �������� ��������, ��� ����� �� ����������� ������ �� UART �� �������������
				simcom_mode = 2;
			}			
			break;
		}
		case 2:
		{
			SendStr_P(AT);									// ���� ��
			ActivateParsing(AT_OK,AT_WAIT_TIME);			// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 3:
		{			
			SendStr_P(ATE0);								// ��������� ���
			ActivateParsing(ATE0_OK,AT_WAIT_TIME);			// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 4:
		{			
			SendStr_P(AT_IPR);								// ������ �������� ������ � �������
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 5:
		{
			SendStr_P(AT_CMGF);								// ������ ��������� ������ SMS
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect			
			break;
		}
		case 6:
		{			
			SendStr_P(AT_GSMBUSY_1);						// ������ ���� �������� �������
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 7:
		{			
			SendStr_P(AT_CPAS);								// ������ ������ �� ��������� ������ SIMCOM
			ActivateParsing(CPAS_OK,AT_WAIT_TIME);			// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 8:
		{			
			SendStr_P(AT_CREG);								// ������ ������ �� ��������� ����������� � ����
			ActivateParsing(CREG_OK,AT_WAIT_TIME);			// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 9:
		{			
			SendStr_P(AT_CCALR);							// ������ ������ �� ����������� ��������� ������
			ActivateParsing(CCALR_OK,AT_WAIT_TIME);			// ���������� ������� ������ � ����������� USART_RX_vect
			break;	
		}
		case 10:
		{			
			SendStr_P(AT_CMGS);								// ������ ������ �� �������� SMS � ������� ���� 220�						
			SendStr(Abonent_SMS);
			SendStr_P(AT_CMGS_2);
			ActivateParsing(POINTER,AT_WAIT_TIME);			// ���������� ������� ������� ����������� ��� �������� ���� SMS
			break;
		}
		case 11:
		{
			if (!(ACSR & (1<<ACIS0)))						// ���� ��������� ���������� �� ����������� ��������� �� �������� ������ ����������� � 1 �� 0 
				SendStr_P(NO_220);							// ���������� ���� SMS � ������� ���� 220�
			else SendStr_P(RETURN_220);						// ����� ���������� ���� SMS � �������������� ���� 220�			
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � �������� �������� SMS � ����������� USART_RX_vect
			break;
		}
		case 12:
		{
			SendStr_P(AT_CLCC);								// ����������� �� ����������� ����� ��� �������� ������
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 13:
		{
			SendStr_P(AT_CLIP);								// �������� ���
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 14:
		{
			SendStr_P(AT_GSMBUSY_0);						// ���������� ���� �������� �������
			ActivateParsing(_OK,AT_WAIT_TIME);				// ���������� ������� ������ � ����������� USART_RX_vect
			break;
		}
		case 15:
		{
			simcom_mode = 6;								// ���������� ������� � ��������� ������� ���� �������� �������, ����� ����� ��������� ���������������� ����� ��������� �������� ������
			break;
		}		
		default: simcom_mode = 1;
	}
}
//=====================================================================================================================================================
// ���������������� �������
void Programming(void)
{
#if defined (DEBUG)
	LED_PORT &= ~(1<<LED_PROG);								// ������ ��� �������
#else
	LED_PORT |= 1<<LED_PROG;								// ������� ��������� ����������������
#endif
/*
	while (simcom_init_mode != 1)							// ���� ������ SIMCOM �� ������� ������ �������������
	{
		CheckSIMCOM();										// ��������� ��������� ������, ����������� � ����, � ������
	}

	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_GSMBUSY_0);								// ���������� ���� �������� �������
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_CLIP);										// �������� ���
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_CLCC);										// ����������� �� ����������� ����� ��� �������� ������
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();

	Switch_Programming_mode();								// �������� �������� ������� ������ ����������������

	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	SendStr_P(AT_GSMBUSY_1);								// ������ ���� �������� �������
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
*/
	programming_mode = 1;

	while (simcom_init_mode != 2)							// ���� ������ SIMCOM �� ���������� �� ����� �������� �������
	{
		CheckSIMCOM();										// ��������� ������� ��������
	}

	Switch_Programming_mode();								// �������� �������� ������� ������ ����������������

	ATOMIC_BLOCK(ATOMIC_FORCEON)							// ���� ��������� ����� �� ��������, ������ ���� 3 ������ � ���. �������� �� �� ��� � EEPROM
	{
		led_delay = 0;										// ��������� ������� ���������� ���������������� (LED_PROG)		
	#if defined (DEBUG)
		LED_PORT &= ~(1<<LED_PROG);							// ������ ��� �������
	#else
		LED_PORT |= 1<<LED_PROG;							// �������� ��������� ����������������			
	#endif
		eeprom_update_block(number1,ee_number1,14);			
		eeprom_update_block(number2,ee_number2,14);
		eeprom_update_block(number3,ee_number3,14);
	}
	#if defined (DEBUG)
		LED_PORT |= 1<<LED_PROG;							// ������ ��� �������
	#else
		LED_PORT &= ~(1<<LED_PROG);							// ����� ��������� ����������������
	#endif
	
	pin_state = JUMPER_PINS;								// ������ ��������� ����� ����� c ��������� ����������������
	while(!(pin_state & (1<<JUMPER_PIN)))
	{
		wdt_reset();
	}														// ���� ����������� �������� ���������������� � ��������� "���"		
}
//=====================================================================================================================================================
// �������� ������� ������ ������� �������. � ����������� �� �������� programming_mode, ������� ��������� ������, ���� ���������� ����� ��������� � ���
void Switch_Programming_mode(void)													
{
	parsing_result = BAD;

	while (programming_mode != 7)							// ���� �� ������� 3 �������� ������
	{
		wdt_reset();		
		switch (programming_mode)							// ������ �������� ������� ������ ������� �������
		{
			case 1:
			{
				Wait_RING(1000, 2);							// ������ ����������� LED_PROG � �������� 1 ��� � 2 �������
				break;
			}
			case 2:
			{			
				SaveNumber_2_RAM(number1, 3);				// ��������� ����� 1-�� �������� � ���, ���� �� ������
				_delay_ms(500);
				break;
			}
			case 3:
			{
				Wait_RING(500, 4);							// ������ ����������� LED_PROG � �������� 1 ��� � �������
				break;
			}
			case 4:
			{
				SaveNumber_2_RAM(number2, 5);				// ��������� ����� 2-�� �������� � ���, ���� �� ������
				_delay_ms(500);
				break;
			}
			case 5:
			{			
				Wait_RING(250, 6);							// ������ ����������� LED_PROG � �������� 2 ���� � �������
				break;
			}
			case 6:
			{			
				SaveNumber_2_RAM(number3, 7);				// ��������� ����� 3-�� �������� � ���, ���� �� ������
				_delay_ms(500);
				break;
			}
			default: programming_mode = 1; break;
		}
	}
}
//=====================================================================================================================================================
// ��������� �������� �������� ��������� ������ - "RING", ���� ������������ �� ����� ������ ��������� ������� � ������ ��������� ������
void Wait_RING(unsigned int _led_delay, unsigned char next_programming_mode)// � �������� ���������� �����������: ��������� ��������� �������� ���������������� ������� ������� � ������ ������� ���������� LED_PROG
{
	if (parsing_result == BAD)								// ���� ��������� ������ ���
	{
		ActivateParsing(RING,WAIT_INCOMING_CALL_TIME);		// ���������� �������� ��������� ������ � ������� � ������, ����� ����������
	}
	if (parsing_result == OK)								// ���� ��������� �������� ������
	{							
		ppk_mode = PROG;									// �������� ����� ������ ���.������
		UCSRB |= 1<<RXCIE;									// ��������� ���������� �� ������� ����� - ������ ������ ��������
		programming_mode = next_programming_mode;			// ��������� ������� � ��������� �������� ��������� ������ ���������� ������ �������� � �������� ������					
	}
	if (led_delay == 0)
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			led_delay = _led_delay;							// ������ ����������� LED_PROG � ������ ��������
		}
		LED_PORT ^= 1<<LED_PROG;					
	}
}
//=====================================================================================================================================================
// ���������� ������ ��������� �������� � ���
void SaveNumber_2_RAM(char *number, unsigned char next_programming_mode)// � �������� ���������� �����������: ��������� �� 1 ������ ������ �������� � ��������� ��������� �������� ���������������� ������� �������
{
	if (buffer_index == buffer_max-1)						// ���� �������� ������ ������ - ������� ���� ���������� ����� ���������
	{
		for	(buffer_index = 0; buffer_index != buffer_max-1; buffer_index ++)// ������� ����� ��������� �� ��������� ������� � ������ numberX
		{
			number[buffer_index] = buffer[buffer_index];
		}
		SendStr_P(ATH);										// ��������� �������� �����
		buffer_index = 0;									// �������� ��������� �������, ����� ������ � ������ �������
		ppk_mode = GUARD_OFF;								// �������� ����� �������� ������
		parsing_result = BAD;								// ���������� �������� ���������� ������ � ��������� ���� ��������� ��������
		programming_mode = next_programming_mode;			// ��������� ������� � �������� �������� ���������� ������ 
	}
}
//=====================================================================================================================================================
// ��������� �������� ������. ��� ������� ���������� � ����������� USART_RXC_vect. �������� ������� �������� � ����������� TIMER1_COMPA_vect
void ActivateParsing(const char *string, unsigned int _parsing_delay)// �� ����� ��������� �� 1 ������ ������, � ����� �������� ������ � ��
{
	unsigned char temp;

	parsing_result = IN_PROCESS;							// �������� ������� � ��������� �������� ��������� ��������/������ ��������	
	parsing_pointer = string;								// �������� ��������� �� 1 ����� ������, ������� ����� ������� � ���������� ���������� 
	ATOMIC_BLOCK(ATOMIC_FORCEON){parsing_delay = _parsing_delay;}// ������ ������������ ����� �������� ������. ��� ������� ����� ����������� � ������
	temp = UDR;												// ������ ��������, ����� �������� ���� ���������� �� ������� ������, ������� ��� ��� �� �����	
	UCSRB |= 1<<RXCIE;										// ��������� ���������� �� ������� ����� - ������� �������							
/*
	while (parsing_result == IN_PROCESS)					// ���� ��������� ���������, ���� ������ ��������
	{
		_delay_ms(1);										// ��������
		parsing_delay --;
		if (parsing_delay == 0)								// ���� ����� �������� �����
		{
			UCSRB &= ~(1<<RXCIE);							// ��������� ���������� �� ������� �����, ����� �� ����������� �� ������ �����
			return BAD;										// � ������������ �� ����� � ������� ��������
		}
	}
	if (parsing_result == 1) return OK;
	else return BAD;
*/
}
//=====================================================================================================================================================
// ������ ���������� ���������� ������� �� EEPROM � ���
void ReadNumbers(void)
{
	eeprom_read_block(number1,ee_number1,14);				// �������� ������ Number_1 �� EEPROM, � ������ Number1
	eeprom_read_block(number2,ee_number2,14);				// �������� ������ Number_2 �� EEPROM, � ������ Number2
	eeprom_read_block(number3,ee_number3,14);				// �������� ������ Number_3 �� EEPROM, � ������ Number3
}
//=====================================================================================================================================================
// �������� ������ �� ����� � UART
void SendStr_P(const char *string)							// �� ����� ��������� �� ������ ������
{
	while (pgm_read_byte(string) != '\0')					// ���� ���� ������ �� 0 (����� ������)
	{
		SendByte(pgm_read_byte(string++));					// �� ���������� ����� ������, �� ������� ����������� ���������, ������� ��������� ������ ������
	}
}
//=====================================================================================================================================================
// �������� ������ �� ��� � UART
void SendStr(char *string)									// �� ����� ��������� �� ������ ������
{
	while (*string != '\0')									// ���� ���� ������ �� 0 (����� ������)
	{
		SendByte(*string++);								// �� ���������� ����� ������, �� ������� ����������� ���������, ������� ��������� ������ ������
	}
}
//=====================================================================================================================================================
// �������� ������ ������� ������ � UART
void SendByte(char byte)									// �� ����� ����
{
	while(!(UCSRA & (1<<UDRE)));							// ���� ����� ���������� UART
	UDR=byte;												// �������� ���� � UART
}
//=====================================================================================================================================================
// ������� ����������� LED_WORK
void Blink_LED_WORK(void)
{
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		led_delay = 500;
	}
	LED_PORT ^= 1<<LED_WORK;
}
//=====================================================================================================================================================
// ���������� ������ � �������, ���� ������ �����
void Siren_Outs_OFF(void) 
{
	if (siren_delay == 0)									// ���� ��� ������� ����� �������� ������
	{
	#if defined (DEBUG)
		OUT_PORT |= 1<<SIREN;								// ������ ��� �������
	#else
		OUT_PORT &= ~(1<<SIREN);							// �������� ������
	#endif
	}

	if (out_delay == 0)										// ���� ��� ������� ����� ���������� �������
	{
	#if defined (DEBUG)
		OUT_PORT |= 1<<OUT_1;								// ������ ��� �������
	#else			
		OUT_PORT &= ~(1<<OUT_1);							// �������� ������ OUT_1,			
	#endif
		OUT_PORT |= 1<<OUT_2;								// � OUT_2 (��������� ������ ������)
	}
}
//=====================================================================================================================================================
// ������������� ������ � ���������
void Init(void)
{
// ������������� ������
	SIMCOM_RESET_DDR |= 1<<SIMCOM_RESET_PIN;				// SIMCOM_RESET �� �����
	DDRB |= 1<<LED_PROG|1<<LED_WORK|1<<OUT_2|1<<OUT_1|1<<SIREN;// ������, ������, � ���������� - �� �����
	
#if defined (DEBUG)
	PORTD |= 1<<JUMPER_PIN|1<<DATCHIK_2|1<<DATCHIK_1;		// ������ ��� �������
#else
	PORTD |= 1<<JUMPER_PIN|1<<DATCHIK_1;					// �������� �������� ��� �������� ����� � ����� ���������������� � DATCHIK_1 ( DATCHIK_2 �� ���� !!! )
#endif
	PORTB |= 1<<BUTTON_PIN|1<<OUT_2;						// �������� �������� ��� ������ ����������/������, � ���������� OUT_2 (����������� ��� �������)

#if defined (DEBUG)
	LED_PORT |= 1<<LED_WORK|1<<LED_PROG;					// ������ ��� �������
#else
	LED_PORT &= ~(1<<LED_WORK|1<<LED_PROG);					// �������� ����������
#endif

// ������������� UART
	UBRRL = LO(bauddivider);
	UBRRH = HI(bauddivider);
	UCSRA = 0;
	UCSRB = 0<<RXCIE|0<<TXCIE|0<<UDRIE|1<<RXEN|1<<TXEN;		// ���������� UDRIE ����� ������� �� ���������, ����� ���������� ����� ������ � ��� ����������

#if defined (__AVR_ATmega8__)
	UCSRC = 1<<URSEL|1<<UCSZ1|1<<UCSZ0;
#endif

#if defined (__AVR_ATtiny2313__)||(__AVR_ATtiny2313A__)
	UCSRC = 1<<UCSZ1|1<<UCSZ0;
#endif

// ������������� ��������
#if defined (__AVR_ATmega8__)
// ������������ ������������ ����������������, ��� ���������� ������� ���������� Timer1 ������ ��
															// ����� ���. ����� ���������� Timer1 �� ���������� (1<<WGM12)
	TCCR1B = 1<<WGM12|0<<CS12|0<<CS11|1<<CS10;				// ������ ������� ��� ������������
//	TCCR1B = 1<<WGM12|0<<CS12|1<<CS11|0<<CS10;				// ������ ������� � ������������� 8
//	TCCR1B = 1<<WGM12|0<<CS12|1<<CS11|1<<CS10;				// ������ ������� � ������������� 64
//	TCCR1B = 1<<WGM12|1<<CS12|0<<CS11|0<<CS10;				// ������ ������� � ������������� 256
//	TCCR1B = 1<<WGM12|1<<CS12|0<<CS11|1<<CS10;				// ������ ������� � ������������� 1024				
	OCR1A = 7999;											// �������� ������������ ����������������, ��� ���������� ������� ���������� Timer1 ������ ��
	TIMSK = 1<<OCIE1A;										// ��������� ���������� �� ���������� �������� OCR1A � ������� �����
	TIFR = 1<<OCF1A;										// ������� ����, ����� ���������� �� ��������� �����
#endif

#if defined (__AVR_ATtiny2313__)||(__AVR_ATtiny2313A__)
// ������������ ������������ ����������������, ��� ���������� ������� ���������� Timer0 ������ ��
	TCCR0A = 1<<WGM01;										// ���. ����� ���������� Timer0 �� ����������
//	TCCR0B = 0<<CS02|0<<CS01|1<<CS00;						// ������ ������� ��� ������������
//	TCCR0B = 0<<CS02|1<<CS01|0<<CS00;						// ������ ������� � ������������� 8
	TCCR0B = 0<<CS02|1<<CS01|1<<CS00;						// ������ ������� � ������������� 64
//	TCCR0B = 1<<CS02|0<<CS00|0<<CS00;						// ������ ������� � ������������� 256
//	TCCR0B = 1<<CS02|0<<CS00|1<<CS00;						// ������ ������� � ������������� 1024
	OCR0A = 124;											// �������� ������������ ����������������, ��� ���������� ������� ���������� Timer0 ������ ��
	TIMSK = 1<<OCIE0A;										// ��������� ���������� �� ���������� �������� OCR0A � ������� �����
#endif

// �������� ������� ���������� (DATCHIK_1, DATCHIK_2)
#if defined (DEBUG)
	MCUCR |= 1<<ISC11|1<<ISC01;								// ������ ��� �������
#else														
	MCUCR |= 1<<ISC11|1<<ISC10|1<<ISC01;					// INT1 - ���������� �� ��������� ������ (DATCHIK_2), INT0 - ���������� �� ����� (DATCHIK_1)				
#endif

// �������� ���������� ����������
	if (!(pin_state & (1<<PD7)))							// ���� �� ������ ������������� �� ���� ������ �������, ������ ��� ���� 220�,				
	ACSR |= 1<<ACBG|1<<ACIE|1<<ACIS1|1<<ACIS0;				// ������� ���������� ���������� ���, ��������� ���������� �� �����������, ������� ������������� ���������� - ������� ������ ����������� � 0 �� 1.
	else ACSR |= 1<<ACBG|1<<ACIE|1<<ACIS1;					// ����� ���� ���� 220�, ������� ���������� ���������� ���, ��������� ���������� �� �����������, ������� ������������� ���������� - ������� ������ ����������� � 1 �� 0.
}
//=====================================================================================================================================================
// ���������� �� ����������� ������� ����������� UART
/*
ISR (USART_UDRE_vect)		
{
	buffer_index ++;										// ����������� ������
 
	if (buffer_index == buffer_MAX)							// ������ ���� ������? 
	{
		UCSRB &= ~(1<<UDRIE);								// ��������� ���������� �� ����������� - �������� ���������
	}
	else 
	{
		UDR = buffer[buffer_index];							// ����� ������ �� �������. 
	}
}
*/
//=====================================================================================================================================================
// ������ ������ �� �������� �� ���
/*
	char String_AT[] = "AT\r\n";							// ���������� � ������ ������-������
	char *u;												// � ��� ��������� �� ��������
	u=String_AT;											// ����������� ��������� ����� ������ ������. �������� ������ ������ "&" � ������ ������ �� ����� 
	SendStr(u);												// ������
	SendStr("AT\r\n");										// ������
*/
// ������ ������ �� �������� �� ����� ������
//	SendStr_P(PSTR("AT\r\n"));
