#include "iodefine.h"
#include  "misratypes.h"
#include  "iwdt.h"


// �Ɨ��E�I�b�`�h�b�N�^�C�}�̃��t���b�V��
//
void IWDT_Refresh(void)
{
	IWDT.IWDTRR = 0x00;
	
	IWDT.IWDTRR = 0xff;
}