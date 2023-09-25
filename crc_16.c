
#include	 "iodefine.h"
#include	 "misratypes.h"
#include	"crc_16.h"

uint8_t	crc_16_err;	// CRCが不一致の場合=1

// CRC演算器の設定
// CRC-CCITT
// 生成多項式 : X^16 + X^12 + X^5 + 1
// 初期値: 0xffff
//  MSBファースト
//
void Init_CRC(void)
{
	
	CRC.CRCCR.BYTE = 0x87;		     // CRCDORレジスタをクリア, MSBファースト, 16ビットCRC（X^16 + X^12 + X^5 + 1 ）
	CRC.CRCDOR = 0xffff;		     // 初期値

}

