
#include "typedefine.h"
#include  "iodefine.h"
#include "misratypes.h"

#include "dsad.h"
    
    
// AD data (DSAD0)
volatile uint8_t ad_ch;		// �ϊ��`���l��: �i�[����Ă���f�[�^���ǂ̃`���l����A/D �ϊ����ʂ��������B
				// ( 0=���ϊ��܂��̓f�[�^����,�@1=�`���l��0�̃f�[�^,�@2=�`���l��1�̃f�[�^, 3=�`���l��2�̃f�[�^, 4=�`���l��3�̃f�[�^)
volatile uint8_t ad_err;	// 0:�ُ�Ȃ�, 1:�ُ팟�o
volatile uint8_t ad_ovf;	// 0:������(�͈͓�), 1:�I�[�o�t���[����

volatile uint32_t ad_cnt;	// A/D�J�E���g�l          
volatile int32_t ad_data;       //  A/D�f�[�^(2�̕␔�`��)

volatile int32_t ad_ch0_data[10];	// DSAD0 Ch0�̃f�[�^
volatile int32_t ad_ch1_data[10];	//       Ch1�̃f�[�^
volatile int32_t ad_ch2_data[10];	//       Ch2�̃f�[�^
volatile int32_t ad_ch3_data[10];	//       Ch3�̃f�[�^


volatile uint32_t ad_index;	// �f�[�^�i�[�ʒu������ index

int32_t ad_ch_avg[4];		// DSAD0 �e�`�����l������ ���ϒl, ac_ch_avg[0] : DSAD0 Ch0�̕��ϒl


volatile uint32_t dsad0_scan_over;	// dsad0 ch0�`ch3�̃X�L��������

volatile float ch0_volt;
volatile float ch0_volt_mili;


// AD data (DSAD1)
volatile uint8_t ad1_ch;   
volatile uint8_t ad1_err;
volatile uint8_t ad1_ovf;
		
volatile uint32_t ad1_cnt;        
volatile int32_t  ad1_data;
volatile int32_t  ad1_ch0_data[10];     //  DSAD1 Ch0�̃f�[�^  A/D�ϊ��l(2�̕␔�`��)

volatile uint32_t ad1_index;

int32_t ad1_ch_avg[1];		// DSAD1 �e�`�����l������ ���ϒl


volatile uint32_t dsad1_scan_over;	// dsad1 ch0�̃X�L��������



//  DSAD0 AD�ϊ������@���荞��
// �`�����l������A/D�ϊ��I���Ŕ����B
//  16.6 msec���ɔ���
//
// �����`�����l����ϊ�����ꍇ�A�e�`�����l���͏���ϊ��ƂȂ邽�߁A�f�W�^���t�B���^�̈��莞��(4T)������B
// 4*T=4x4=16[msec] ������B(T=OSR/0.5 = 2048/(0.5MHz) = 4 [msec])
// (�Q�l :�A�v���P�[�V�����m�[�g�@�uRX23E-A�O���[�v�@AFE�EDSAD�̎g�����v1.2 �`���l���@�\���g�p���������M���̃T���v�����O�@)
//
#pragma interrupt (Excep_DSAD0_ADI0(vect=206))
void Excep_DSAD0_ADI0(void)
{
					 
	ad_ch  = DSAD0.DR.BIT.CCH;	// �ϊ��`���l��( 0=���ϊ��܂��̓f�[�^����,�@1=�`���l��0�̃f�[�^,�@2=�`���l��1�̃f�[�^)
	ad_err =  DSAD0.DR.BIT.ERR;	// 0:�ُ�Ȃ�, 1:�ُ팟�o
	ad_ovf = DSAD0.DR.BIT.OVF;	// 0:������(�͈͓�), 1:�I�[�o�t���[����
	
	ad_cnt = DSAD0.DR.BIT.DATA;		// A/D�ϊ���̃f�[�^�@(32bit�����t���f�[�^)

	if (( ad_cnt & 0x800000 ) == 0x800000 ) {      // 24bit�����t���f�[�^�ɂ���B
		ad_data =  ad_cnt - 16777216;		// 2^24 = 16777216�@�@(2^23 = 8388608 = 0x800000)
	}
	else{
		ad_data = ad_cnt;
	}
	
	if (( ad_err == 0 ) && ( ad_ovf == 0 )) { // A/D�ϊ�����ŁA�I�[�o�t���[�Ȃ�
	
		if ( ad_ch == 1 ) {				// �`�����l��0�̃f�[�^�i�[	
			ad_ch0_data[ad_index] = ad_data;
		}
		else if ( ad_ch == 2 ) {
			ad_ch1_data[ad_index] = ad_data;
		}
		else if ( ad_ch == 3 ) {
			ad_ch2_data[ad_index] = ad_data;
		}
		else if ( ad_ch == 4 ) {
			ad_ch3_data[ad_index] = ad_data;
		}
	 }
	 else {
	  	if ( ad_ch == 1 ) {
			ad_ch0_data[ad_index] = 0x7fffff;
		}
		else if ( ad_ch == 2 ) {
			ad_ch1_data[ad_index] = 0x7fffff;
		}
		else if ( ad_ch == 3 ) {
			ad_ch2_data[ad_index] = 0x7fffff;
		}
		else if ( ad_ch == 4 ) {
			ad_ch3_data[ad_index] = 0x7fffff;
		}
	  }
	
	
	
	
}

// DSAD0 �X�L�����������荞��
// �`�����l��0����`�����l��3�܂ŁA1������Ɗ��荞�݂ɓ���
//  
//
#pragma interrupt (Excep_DSAD0_SCANEND0(vect=207))
void Excep_DSAD0_SCANEND0(void)
{
	dsad0_scan_over = 1;		// dsad0 �X�L��������
	
	ad_index = ad_index + 1;	// �f�[�^�i�[�ʒu�̍X�V
	
	
	if ( ad_index > 9 ) {		
		ad_index = 0;		// �f�[�^�i�[�ʒu������
	}
	

}



//  DSAD1(�M�d�΂̊(��)�ړ_�⏞�@������R�̗p) AD�ϊ������@���荞��
// 
//
#pragma interrupt (Excep_DSAD1_ADI1(vect=209))
void Excep_DSAD1_ADI1(void)
{
	
	ad1_ch  = DSAD1.DR.BIT.CCH;	// �ϊ��`���l�� 0=���ϊ��܂��̓f�[�^����,�@1=�`���l��0,	2=�`���l��1
	ad1_err =  DSAD1.DR.BIT.ERR;	// 0:�ُ�Ȃ�, 1:�ُ팟�o
	ad1_ovf = DSAD1.DR.BIT.OVF;	// 0:������(�͈͓�), 1:�I�[�o�t���[����
	
	ad1_cnt = DSAD1.DR.BIT.DATA;	// A/D�ϊ���̃f�[�^�@(32bit�����t���f�[�^)

	if (( ad1_cnt & 0x800000 ) == 0x800000 ) {      // 24bit�����t���f�[�^�ɂ���B
		ad1_data =  ad1_cnt - 16777216;
	}
	else{
		ad1_data = ad1_cnt;
	}
	
	if (( ad1_err == 0 ) && ( ad1_ovf == 0 )) { // A/D�ϊ�����ŁA�I�[�o�t���[�Ȃ�
						
		ad1_ch0_data[ad1_index] = ad1_data;   // �`�����l��0�̃f�[�^�i�[
		
	}
	else{
		ad1_ch0_data[ad1_index] = 0x7fffff;
	}
	
	
}


// DSAD1 �X�L�����������荞��
// �`�����l��0�@AD�ϊ������Ŋ��荞�݂ɓ��� (�I�[�g�X�L�����J�n���犮���܂ŁA16.7msec�j
//
#pragma interrupt (Excep_DSAD1_SCANEND1(vect=210))
void Excep_DSAD1_SCANEND1(void)
{
	
	ad1_index = ad1_index + 1;	// �f�[�^�i�[�ʒu�̍X�V
	
	
	if ( ad1_index > 9 ) {		
		ad1_index = 0;		// �f�[�^�i�[�ʒu������
	}
	
	
}


// �I�t�Z�b�g�␳�l�A�Q�C���␳�l�̐ݒ� (�`�����l��0�`3) 
//  �I�t�Z�b�g�␳�l: 0
//  �Q�C�� �␳�l   : 1

void Set_Error_offset_0_Gain_1(void)
{
	DSAD0.OFCR0 = 0;	// �`�����l��0 �I�t�Z�b�g�␳�l = 0
	DSAD0.OFCR1 = 0;	// �`�����l��1      :
	DSAD0.OFCR2 = 0;	// �`�����l��2	    :
	DSAD0.OFCR3 = 0;	// �`�����l��3	    :
	
	DSAD0.GCR0 = 0x00400000;  // �`�����l��0 �Q�C���␳�l = 1.0
	DSAD0.GCR1 = 0x00400000;  // �`�����l��1 �Q�C���␳�l = 1.0
	DSAD0.GCR2 = 0x00400000;  // �`�����l��2 �Q�C���␳�l = 1.0
	DSAD0.GCR3 = 0x00400000;  // �`�����l��3 �Q�C���␳�l = 1.0
	
}



// �I�t�Z�b�g�␳�l�A�Q�C���␳�l�̐ݒ� (�`�����l��0�`3) 
//  �I�t�Z�b�g : �␳�l (�I�t�Z�b�g= 0,�Q�C��=1�Ƃ��ē���ꂽ�l)
//�@�Q�C��     : 1
void Set_Error_offset_Calib_Gain_1(void)
{
	DSAD0.OFCR0 = -11444;	// �`�����l��0 �I�t�Z�b�g�␳�l
	DSAD0.OFCR1 = -11342;	// �`�����l��1      :
	DSAD0.OFCR2 = -11790;	// �`�����l��2	    :
	DSAD0.OFCR3 = -11517;	// �`�����l��3	    :
	
	DSAD0.GCR0 = 0x00400000;  // �`�����l��0 �Q�C���␳�l = 1.0
	DSAD0.GCR1 = 0x00400000;  // �`�����l��1 �Q�C���␳�l = 1.0
	DSAD0.GCR2 = 0x00400000;  // �`�����l��2 �Q�C���␳�l = 1.0
	DSAD0.GCR3 = 0x00400000;  // �`�����l��3 �Q�C���␳�l = 1.0
	
}


//�@�I�t�Z�b�g�␳�����s���B
// �Q�C���␳�l�́AGain=128�̕␳�f�[�^�����[�h����Ă���B
//
void Set_Error_offset_calib(void)
{
	DSAD0.OFCR0 = -11500;	// �`�����l��0 �I�t�Z�b�g�␳�l
	DSAD0.OFCR1 = -11500;	// �`�����l��1      :
	DSAD0.OFCR2 = -11500;	// �`�����l��2	    :
	DSAD0.OFCR3 = -11500;	// �`�����l��3	    :
}


// ���g�p
// �I�t�Z�b�g�␳�l�A�Q�C���␳�l�̐ݒ� (�`�����l��0�`3) 
//  �I�t�Z�b�g : �␳�l (�I�t�Z�b�g= 0,�Q�C��=1�Ƃ��ē���ꂽ�l)
//�@�Q�C��     : �␳�l (�I�t�Z�b�g=�␳�l, �Q�C��=1�Ƃ��ē���ꂽ�l)

void Set_Error_offset_Gain_Calib(void)
{
	DSAD0.OFCR0 = -7570;	// �`�����l��0 �I�t�Z�b�g�␳�l
	DSAD0.OFCR1 = -7941;	// �`�����l��1      :
	DSAD0.OFCR2 = -8106;	// �`�����l��2	    :
	DSAD0.OFCR3 = -8259;	// �`�����l��3	    :
	
	DSAD0.GCR0 = 0x00404d07;  // �`�����l��0 �Q�C���␳�l =
	DSAD0.GCR1 = 0x0040381a;  // �`�����l��1 �Q�C���␳�l = 
	DSAD0.GCR2 = 0x00402c55;  // �`�����l��2 �Q�C���␳�l = 
	DSAD0.GCR3 = 0x00401429;  // �`�����l��3 �Q�C���␳�l =
	
}



//  DSAD0 �e�`�����l���̕��ϒl�𓾂�
//
void	Cal_ad_avg(void)
{
	uint32_t i;
	
	int32_t ad_ch0_avg_t;
	int32_t ad_ch1_avg_t;
	int32_t ad_ch2_avg_t;
	int32_t ad_ch3_avg_t;

	int32_t ad_avg_t;
	
	ad_ch0_avg_t = 0;
	ad_ch1_avg_t = 0;
	ad_ch2_avg_t = 0;
	ad_ch3_avg_t = 0;
	

	for ( i = 0;  i < 10 ; i++ ) {
	  ad_ch0_avg_t = ad_ch0_avg_t + ad_ch0_data[i];
	  ad_ch1_avg_t = ad_ch1_avg_t + ad_ch1_data[i];
	  ad_ch2_avg_t = ad_ch2_avg_t + ad_ch2_data[i];
	  ad_ch3_avg_t = ad_ch3_avg_t + ad_ch3_data[i];
	}
	
	
	ad_ch_avg[0] = ad_ch0_avg_t / 10;		// ch0 ���ϒl
	ad_ch_avg[1] = ad_ch1_avg_t / 10;		// ch1 ���ϒl
	ad_ch_avg[2] = ad_ch2_avg_t / 10;		// ch2 ���ϒl
	ad_ch_avg[3] = ad_ch3_avg_t / 10;		// ch3 ���ϒl
	
}




//  DSAD1 �e�`�����l���̕��ϒl�𓾂�
//
void	Cal_ad1_avg(void)
{
	uint32_t i;
	
	int32_t ad1_ch0_avg_t;
	
	ad1_ch0_avg_t = 0;

	for ( i = 0;  i < 10 ; i++ ) {
	  ad1_ch0_avg_t = ad1_ch0_avg_t + ad1_ch0_data[i];
	}
	
	
	
	ad1_ch_avg[0] = ad1_ch0_avg_t / 10;	// ch0 ���ϒl
	
}


// DSAD0�� �J�n 
////
//void  dsad0_start(void)
//{
//	DSAD0.ADST.BIT.START = 1;	// DSAD0 �I�[�g�X�L�����J�n
//}



// DDSAD1�� �J�n 
////
//void  dsad1_start(void)
//{
// 	DSAD1.ADST.BIT.START = 1;	// DSAD1 �I�[�g�X�L�����J�n
//}



// DSAD0�� ��~
////
//void  dsad0_stop(void)
//{

//	 DSAD0.ADSTP.BIT.STOP = 1;	// DSAD0 �I�[�g�X�L������~
	 
//	 while ( DSAD0.SR.BIT.ACT == 1 ) {    // �I�[�g�X�L�������s���̓��[�v�B(�I�[�g�X�L������~�҂�)
//	 }
//}



// DSAD1�� ��~
////
//void  dsad1_stop(void)
//{
	 
//	 DSAD1.ADSTP.BIT.STOP = 1;	// DSAD1 �I�[�g�X�L������~
	 
//	 while ( DSAD1.SR.BIT.ACT == 1 ) {    
//	 }	
//}






// AFE(�A�i���O�t�����g�G���h)�����ݒ� 
//
// �[�q: �p�r
//
//�EDSAD0�ւ̓��͐M���ݒ�
//  AIN4/REF1N: �`�����l��0 -��
//  AIN5/REF1P: �`�����l��0 +��
//  AIN6: �`�����l��1 -��
//  AIN7: �`�����l��1 -��
//�@AIN8: �`�����l��2 -��
//  AIN9: �`�����l��2 -��
//  AIN10: �`�����l��3 -��
//  AIN11: �`�����l��3 -��
//
// �E��N�d���o��
//  AIN2: IEXC0 ��N�d���o�� 500uA
//
//�EDSAD1�ւ̓��͐M���ݒ�
//  AIN0: �`�����l��0 -�� (RTD Pt100 -��)
//  AIN1: �`�����l��0 +�� (RTD Pt100 +��)
//  ��d���́A���t�@�����X��R(4.99 K)�ɂ�����d���B(REF0N,REF0P)
//
void afe_ini(void)
{
	   
     				//�@DSAD0�ւ̓��͐M���̐ݒ�
    				//�@�`���l��0(��H�}�̃��x���� Ch1)
    AFE.DS00ISR.BIT.NSEL = 4;   // AIN4:�`�����l��0 -�� (Ch1_N)�@			
    AFE.DS00ISR.BIT.PSEL = 5;	// AIN5:�`�����l��0 +�� (Ch1_P)�@
    AFE.DS00ISR.BIT.RSEL = 0x04;   // ��d�� +�� REFOUT(2.5V) , -�� AVSS0 ,���t�@�����X�o�b�t�@����
    
    				//�@�`���l��1(��H�}�̃��x���� Ch2)
    AFE.DS01ISR.BIT.NSEL = 6;   // AIN6:�`�����l��1 -�� (Ch2_N)�@			
    AFE.DS01ISR.BIT.PSEL = 7;	// AIN7:�`�����l��1 +�� (Ch2_P)�@
    AFE.DS01ISR.BIT.RSEL = 0x04;   // ��d�� +�� REFOUT(2.5V) , -�� AVSS0 ,���t�@�����X�o�b�t�@����
    
       				//�@�`���l��2(��H�}�̃��x���� Ch3)
    AFE.DS02ISR.BIT.NSEL = 8;   // AIN8:�`�����l��2 -�� (Ch3_N)�@			
    AFE.DS02ISR.BIT.PSEL = 9;	// AIN9:�`�����l��2 +�� (Ch3_P)�@
    AFE.DS02ISR.BIT.RSEL = 0x04;   // ��d�� +�� REFOUT(2.5V) , -�� AVSS0 ,���t�@�����X�o�b�t�@����
    
      				//�@�`���l��3(��H�}�̃��x���� Ch4)
    AFE.DS03ISR.BIT.NSEL = 10;  // AIN10:�`�����l��3 -�� (Ch4_N)�@			
    AFE.DS03ISR.BIT.PSEL = 11;	// AIN11:�`�����l��3 +�� (Ch4_P)�@
    AFE.DS03ISR.BIT.RSEL = 0x04;   // ��d�� +�� REFOUT(2.5V) , -�� AVSS0 ,���t�@�����X�o�b�t�@����
    
    
     				//�@DSAD1�ւ̓��͐M���̐ݒ�
    AFE.DS10ISR.BIT.NSEL = 0;	// AIN0:�`�����l��0 -�� (RTD Pt100 -��)
    AFE.DS10ISR.BIT.PSEL = 1;   // AIN1:�`�����l��0 +�� (RTD Pt100 +��)  
  
    AFE.DS10ISR.BIT.RSEL = 0x0b;   // ��d�� +�� REF0P , -�� REF0N ,  ���t�@�����X�o�b�t�@�L�� 1011(b) = 0x0b
  
     
   				// ��N�d����(IEXC)�̐ݒ� (RTD PT 100ohm�ɗ����d��)0.1 mA to 0.50 mA
    AFE.EXCCR.BIT.CUR = 3;	// ��N�d�����̏o�͓d�� 500[uA]
   
    AFE.EXCCR.BIT.MODE = 0;     // 2�`���l��(IEXC0, IEXC1)�o�̓��[�h
    AFE.EXCOSR.BIT.IEXC0SEL = 2; // IEXC0�o�͒[�q: AIN2
    
    
    AFE.OPCR.BIT.TEMPSEN = 0;    // ���x�Z���T(TEMPS) �̓���֎~
    AFE.OPCR.BIT.VREFEN = 1;	// ��d�������싖�� (REFOUT �[�q����VREF �Ő������ꂽ�d��(2.5 V) ���o��) (����܂ŁA1msec������B)
    AFE.OPCR.BIT.VBIASEN = 0;   // �o�C�A�X�d��������H(VBIAS) �̓���֎~
    AFE.OPCR.BIT.IEXCEN = 1;	// ��N�d����(IEXC)���싖��
    AFE.OPCR.BIT.DSAD0EN = 1;	// DSAD0 ���싖�� (���̃r�b�g���g1�h �ɂ��Ă���DSAD0 ���N������܂ŁA400 ��s �K�v)
    AFE.OPCR.BIT.DSAD1EN = 1;	// DSAD1 ���싖��
    
    AFE.OPCR.BIT.DSADLVM = 1;	// DSAD����d���I��  0: AVCC0=3.6�`5.5 V, 1:AVCC0 = 2.7�`5.5 V

    delay_msec(1);		// 1 msec�҂�
 
}





//
// DASD0(�f���^�V�O�}(����)A/D�R���o�[�^)�̏������@(�M�d�Ηp)
//   �`�����l��0�`�`�����l��3: A/D�ϊ�����
//   �`�����l��4�`�`�����l��5: A/D�ϊ����Ȃ�
//
void dsad0_ini(void){
    
    DSAD0.CCR.BIT.CLKDIV = 7;	// PCLKB/8  (DSAD�́A�m�[�}�����[�h�ł�4MHz�œ��삷��BPCLKB=32MHz����A4MHz�𐶐����邽��8����)
    DSAD0.CCR.BIT.LPMD = 0;	// �m�[�}�����[�h (���W�����[�^�N���b�N���g��(fMOD) = 500[kHz] = 0.5[MHz] )
    
    DSAD0.MR.BIT.SCMD = 1;	// 0:�A���X�L�������[�h, 1:�V���O���X�L�������[�h
    DSAD0.MR.BIT.SYNCST = 0;	// ���j�b�g��(DSAD0,DSAD1)�����X�^�[�g�̖���
    DSAD0.MR.BIT.TRGMD = 0;	// �\�t�g�E�F�A�g���K(ADST���W�X�^�ւ̏������݂ŕϊ��J�n)
    DSAD0.MR.BIT.CH0EN = 0;	// �`�����l��0 A/D�ϊ�����
    DSAD0.MR.BIT.CH1EN = 0;	// �`�����l��1 A/D�ϊ�����
    DSAD0.MR.BIT.CH2EN = 0;	// �`�����l��2 A/D�ϊ�����
    DSAD0.MR.BIT.CH3EN = 0;	// �`�����l��3 A/D�ϊ�����
    DSAD0.MR.BIT.CH4EN = 1;	// �`�����l��4 A/D�ϊ����Ȃ�
    DSAD0.MR.BIT.CH5EN = 1;	// �`�����l��5 A/D�ϊ����Ȃ�
    
    				// �`�����l��0�̓��샂�[�h�ݒ�
    DSAD0.MR0.BIT.CVMD = 0;	// �ʏ퓮��
    DSAD0.MR0.BIT.SDF = 0;	// �o�C�i���`�� -8388608 (80 0000h) �` +8388607(7F FFFFh)
				// �o�C�i���`���̏ꍇ��DSAD�ւ̓��͓d�� = (Vref * 2/Gain) * DR_DATA/(2^24) , 2^24 = 16,777,216
    DSAD0.MR0.BIT.OSR = 5;	// �I�[�o�[�T���v�����O�� = 2048
    DSAD0.MR0.BIT.DISAP = 0;	// +�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR0.BIT.DISAN = 0;    // -�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR0.BIT.AVMD = 0;	// ���ω������Ȃ�
    DSAD0.MR0.BIT.AVDN = 0;	// ���ω��f�[�^���I��
    DSAD0.MR0.BIT.DISC = 0;     //�@�f�����o�A�V�X�g�d�� = 0.5 [uA]
    
    				// �`�����l��1�̓��샂�[�h�ݒ�
    DSAD0.MR1.BIT.CVMD = 0;	// �ʏ퓮��
    DSAD0.MR1.BIT.SDF = 0;	// �o�C�i���`�� -8388608 (80 0000h) �` +8388607(7F FFFFh)
    DSAD0.MR1.BIT.OSR = 5;	// �I�[�o�[�T���v�����O�� = 2048
    DSAD0.MR1.BIT.DISAP = 0;	// +�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR1.BIT.DISAN = 0;    // -�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR1.BIT.AVMD = 0;	// ���ω������Ȃ�
    DSAD0.MR1.BIT.AVDN = 0;	// ���ω��f�[�^���I��
    DSAD0.MR1.BIT.DISC = 0;     //�@�f�����o�A�V�X�g�d�� = 0.5 [uA]
    
    				// �`�����l��2�̓��샂�[�h�ݒ�
    DSAD0.MR2.BIT.CVMD = 0;	// �ʏ퓮��
    DSAD0.MR2.BIT.SDF = 0;	// �o�C�i���`�� -8388608 (80 0000h) �` +8388607(7F FFFFh)
    DSAD0.MR2.BIT.OSR = 5;	// �I�[�o�[�T���v�����O�� = 2048
    DSAD0.MR2.BIT.DISAP = 0;	// +�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR2.BIT.DISAN = 0;    // -�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR2.BIT.AVMD = 0;	// ���ω������Ȃ�
    DSAD0.MR2.BIT.AVDN = 0;	// ���ω��f�[�^���I��
    DSAD0.MR2.BIT.DISC = 0;     //�@�f�����o�A�V�X�g�d�� = 0.5 [uA]
    
    
    				// �`�����l��3�̓��샂�[�h�ݒ�
    DSAD0.MR3.BIT.CVMD = 0;	// �ʏ퓮��
    DSAD0.MR3.BIT.SDF = 0;	// �o�C�i���`�� -8388608 (80 0000h) �` +8388607(7F FFFFh)
    DSAD0.MR3.BIT.OSR = 5;	// �I�[�o�[�T���v�����O�� = 2048
    DSAD0.MR3.BIT.DISAP = 0;	// +�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR3.BIT.DISAN = 0;    // -�����͐M���f�����o�A�V�X�g �Ȃ�
    DSAD0.MR3.BIT.AVMD = 0;	// ���ω������Ȃ�
    DSAD0.MR3.BIT.AVDN = 0;	// ���ω��f�[�^���I��
    DSAD0.MR3.BIT.DISC = 0;     //�@�f�����o�A�V�X�g�d�� = 0.5 [uA]
    
    
    // �f�W�^���t�B���^��������(T)
    //    T = �I�[�o�[�T���v�����O��(OSR) / ���W�����[�^�N���b�N���g��(fMOD)
    //     OSR = 2048
    //     fMOD = 0.5 [MHz] ( �m�[�}�����[�h )
    //    T = 2048 / 0.5 = 4 [msec]
    //
    //  A/D�ϊ�����(�Z�g�����O����)  (�}�j���A�� 34.3.7.2 �Z�g�����O����)
    //    4 * T + 256[usec] = 16.3 msec
    //
    
				// �`�����l��0  A/D�ϊ���,�Q�C���ݒ�    
    				// A/D �ϊ��� N = x * 32 + y �A(CR0.CNMD = 1:���l���[�h�̏ꍇ)
    DSAD0.CR0.BIT.CNY = 1;	// 
    DSAD0.CR0.BIT.CNX = 0;	//                                                        
    DSAD0.CR0.BIT.CNMD = 1;	// A/D�ϊ��񐔉��Z���[�h �F���l���[�h(A/D�ϊ��񐔂�1�`255��)
    DSAD0.CR0.BIT.GAIN =0x17;	// PGA(�v���O���}�u���Q�C���v���A���v)�L���A�Q�C��=128 �A�i���O���̓o�b�t�@(BUF) �̗L��
                                 

    				// �`�����l��1   A/D�ϊ���,�Q�C���ݒ�    
    DSAD0.CR1.BIT.CNY = 1;	//   
    DSAD0.CR1.BIT.CNX = 0;	//                                                         
    DSAD0.CR1.BIT.CNMD = 1;	// A/D�ϊ��񐔉��Z���[�h �F���l���[�h(A/D�ϊ��񐔂�1�`255��)
    DSAD0.CR1.BIT.GAIN =0x17;	// PGA(�v���O���}�u���Q�C���v���A���v)�L���A�Q�C��=128
    
    
       				// �`�����l��2   A/D�ϊ���,�Q�C���ݒ�    
    DSAD0.CR2.BIT.CNY = 1;	// 
    DSAD0.CR2.BIT.CNX = 0;	//                                                        
    DSAD0.CR2.BIT.CNMD = 1;	// A/D�ϊ��񐔉��Z���[�h �F���l���[�h(A/D�ϊ��񐔂�1�`255��)
    DSAD0.CR2.BIT.GAIN =0x17;	// PGA(�v���O���}�u���Q�C���v���A���v)�L���A�Q�C��=128
    
    
          			// �`�����l��3   A/D�ϊ���,�Q�C���ݒ�    
    DSAD0.CR3.BIT.CNY = 1;	//
    DSAD0.CR3.BIT.CNX = 0;	//                                                         
    DSAD0.CR3.BIT.CNMD = 1;	// A/D�ϊ��񐔉��Z���[�h �F���l���[�h(A/D�ϊ��񐔂�1�`255��)
    DSAD0.CR3.BIT.GAIN =0x17;	// PGA(�v���O���}�u���Q�C���v���A���v)�L���A�Q�C��=128
    
    
    IPR(DSAD0,ADI0) = 4;	// ���荞�݃��x�� = 4�@�@�i15���ō����x��)
    IEN(DSAD0,ADI0) = 1;	// ADI0(A/D�ϊ�����) �����݋���
    
    IPR(DSAD0,SCANEND0) = 5;	// ���荞�݃��x�� = 5�@�@�i15���ō����x��)
    IEN(DSAD0,SCANEND0) = 1;	// �X�L�������� �����݋���
   
}




//
// DASD1(�f���^�V�O�}(����)A/D�R���o�[�^)�̏������@(��ړ_�⏞ RTD 1�p)
//   
///
void dsad1_ini(void){
    
    DSAD1.CCR.BIT.CLKDIV = 7;	// PCLKB/8  (DSAD�́A�m�[�}�����[�h�ł�4MHz�œ��삷��BPCLKB=32MHz����A4MHz�𐶐����邽��8����)
    DSAD1.CCR.BIT.LPMD = 0;	// �m�[�}�����[�h
    
    DSAD1.MR.BIT.SCMD = 1;	// 0:�A���X�L�������[�h, 1:�V���O���X�L�������[�h
    DSAD1.MR.BIT.SYNCST = 0;	// ���j�b�g��(DSAD1,DSAD1)�����X�^�[�g�̖���
    DSAD1.MR.BIT.TRGMD = 0;	// �\�t�g�E�F�A�g���K(ADST���W�X�^�ւ̏������݂ŕϊ��J�n)
    DSAD1.MR.BIT.CH0EN = 0;	// �`�����l��0 A/D�ϊ�����
    DSAD1.MR.BIT.CH1EN = 1;	// �`�����l��1 A/D�ϊ����Ȃ�
    DSAD1.MR.BIT.CH2EN = 1;	// �`�����l��2 A/D�ϊ����Ȃ�
    DSAD1.MR.BIT.CH3EN = 1;	// �`�����l��3 A/D�ϊ����Ȃ�
    DSAD1.MR.BIT.CH4EN = 1;	// �`�����l��4 A/D�ϊ����Ȃ�
    DSAD1.MR.BIT.CH5EN = 1;	// �`�����l��5 A/D�ϊ����Ȃ�
    
    
    				// �`�����l��0�̓��샂�[�h�ݒ�
//    DSAD1.MR0.BIT.CVMD = 0;	// A/D�ϊ����[�h :�ʏ퓮��
    DSAD1.MR0.BIT.CVMD = 1;	// A/D�ϊ����[�h :�V���O���T�C�N���Z�g�����O


    DSAD1.MR0.BIT.SDF = 0;	// �o�C�i���`�� -8388608 (80 0000h) �` +8388607(7F FFFFh)
				// �o�C�i���`���̏ꍇ��DSAD�ւ̓��͓d�� = (Vref * 2/Gain) * DR_DATA/(2^24) , 2^24 = 16,777,216
    DSAD1.MR0.BIT.OSR = 5;	// �I�[�o�[�T���v�����O�� = 2048
    DSAD1.MR0.BIT.DISAP = 0;	// +�����͐M���f�����o�A�V�X�g ����
    DSAD1.MR0.BIT.DISAN = 0;    // -�����͐M���f�����o�A�V�X�g ����
    DSAD1.MR0.BIT.AVMD = 0;	// ���ω������Ȃ�
    DSAD1.MR0.BIT.AVDN = 0;	// ���ω��f�[�^���I��
    DSAD1.MR0.BIT.DISC = 0;     //�@�f�����o�A�V�X�g�d�� = 0.5 [uA]
    
    
   
    				// �`�����l��0  A/D�ϊ���,�Q�C���ݒ�
    				// A/D �ϊ��� N= x * 32 + y (CR0.CNMD = 1:���l���[�h�̏ꍇ)
    DSAD1.CR0.BIT.CNY = 1;	//
    DSAD1.CR0.BIT.CNX = 0;	//                                                        
    DSAD1.CR0.BIT.CNMD = 1;	// A/D�ϊ��񐔉��Z���[�h �F���l���[�h(A/D�ϊ��񐔂�1�`255��)
//    DSAD1.CR0.BIT.GAIN = 0x10;	// �Q�C��= 1, PGA(�v���O���}�u���Q�C���v���A���v)�L��,BUF �L��  
 
    DSAD1.CR0.BIT.GAIN = 0x15;	// �Q�C��= 32, PGA(�v���O���}�u���Q�C���v���A���v)�L��,BUF �L��  
    
    
    
    IPR(DSAD1,ADI1) = 6;	// ���荞�݃��x�� = 6�@�@�i15���ō����x��)
    IEN(DSAD1,ADI1) = 1;	// ADI1(A/D�ϊ�����) �����݋���
    
    IPR(DSAD1,SCANEND1) = 7;	// ���荞�݃��x�� = 7�@�@�i15���ō����x��)
    IEN(DSAD1,SCANEND1) = 1;	// �X�L�������� �����݋���
}



