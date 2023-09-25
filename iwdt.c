#include "iodefine.h"
#include  "misratypes.h"
#include  "iwdt.h"


// 独立ウオッチドックタイマのリフレッシュ
//
void IWDT_Refresh(void)
{
	IWDT.IWDTRR = 0x00;
	
	IWDT.IWDTRR = 0xff;
}