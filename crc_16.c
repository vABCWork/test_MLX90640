
#include	 "iodefine.h"
#include	 "misratypes.h"
#include	"crc_16.h"

uint8_t	crc_16_err;	// CRC���s��v�̏ꍇ=1

// CRC���Z��̐ݒ�
// CRC-CCITT
// ���������� : X^16 + X^12 + X^5 + 1
// �����l: 0xffff
//  MSB�t�@�[�X�g
//
void Init_CRC(void)
{
	
	CRC.CRCCR.BYTE = 0x87;		     // CRCDOR���W�X�^���N���A, MSB�t�@�[�X�g, 16�r�b�gCRC�iX^16 + X^12 + X^5 + 1 �j
	CRC.CRCDOR = 0xffff;		     // �����l

}

