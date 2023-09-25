
#include "typedefine.h"
#include  "iodefine.h"
#include "misratypes.h"
#include "timer.h"


// Timer 

volatile uint8_t flg_100msec_interval;	// 100msec����ON

volatile uint32_t timer_10msec_cnt;     // �@(10msec���ɃJ�E���g�A�b�v)

//  �R���y�A�}�b�`�^�C�} CMT0
//   10msec���̊��荞��
//

#pragma interrupt (Excep_CMT0_CMI0(vect=28))

void Excep_CMT0_CMI0(void){
	
	
	timer_10msec_cnt++;	       // �J�E���g�̃C���N�������g
	
	
	if ( timer_10msec_cnt > 9 ) {    // 100msec�o��
	
	       DSAD0.ADST.BIT.START = 1;	// DSAD0 �I�[�g�X�L�����J�n
	       DSAD1.ADST.BIT.START = 1;	// DSAD1 �I�[�g�X�L�����J�n
	
		flg_100msec_interval = 1;  // 100msec�t���O ON

		timer_10msec_cnt = 0;	  //  �J�E���^�[�̃N���A
	}
	
}



//
//    10msec �^�C�}(CMT0)
//    CMT���j�b�g0��CMT0���g�p�B 
//
//  PCLKB(=32MHz)��128�����ŃJ�E���g 32/128 = 1/4 MHz
//      1�J�E���g = 4/1 = 4 [usec]  
//    1*10,000 usec =  N * 4 usec 
//      N = 2500


void Timer10msec_Set(void)
{	
	IPR(CMT0,CMI0) = 3;		// ���荞�݃��x�� = 3�@�@�i15���ō����x��)
	IEN(CMT0,CMI0) = 1;		// CMT0 �����݋���
		
	CMT0.CMCR.BIT.CKS = 0x2;        // PCLKB/128       
	CMT0.CMCOR = 2499;		// CMCNT��0����J�E���g 	


}


//   CMT0 �^�C�}�J�n�@
//  ���荞�݋����ăJ�E���g�J�n

void Timer10msec_Start(void)
{	
	CMT0.CMCR.BIT.CMIE = 1;		// �R���y�A�}�b�`�����݁@����
		
	CMT.CMSTR0.BIT.STR0 = 1;	// CMT0 �J�E���g�J�n
	
	timer_10msec_cnt = 0;		//  �^�C�}�̃J�E���g�l�N���A
}







