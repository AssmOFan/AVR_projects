#include <stdint.h>
#include <stdbool.h>
//=====================================================================================================================================================
#define MAIN_TIMER_BITS  int16_t                            // ����������� �������� �������. �������� ��������� ��������� �������� ��������� (int16_t - max 32,7s)
//=====================================================================================================================================================
void            MainTimerInit(void);                        // ������������� �������
MAIN_TIMER_BITS MainTimerSet(MAIN_TIMER_BITS add_time_ms);  // ��������� �������
MAIN_TIMER_BITS MainTimerIsExpired(MAIN_TIMER_BITS delay);  // �������� �������
//=====================================================================================================================================================