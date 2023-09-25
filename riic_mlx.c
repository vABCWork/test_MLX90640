#include "iodefine.h"
#include "misratypes.h"
#include "riic_mlx.h"



uint8_t iic_slave_adrs;  // IIC �X���[�u�A�h���X  00: 7bit�A�h���X( ��:100 0000 = 0x40 )

volatile uint8_t iic_rcv_data[16];   // IIC��M�f�[�^
volatile uint8_t iic_sd_data[32];    // ���M�f�[�^
volatile uint8_t iic_sd_pt;	    // ���M�f�[�^�ʒu
volatile uint8_t iic_rcv_pt;         // ��M�f�[�^�ʒu

volatile uint8_t  dummy_read_fg;    // ��M���荞�݂ŁA�_�~�[���[�h���邽�߃t���O

volatile uint8_t  iic_sd_rcv_fg;    // 0:���M�݂̂܂��͎�M�݂̂̏ꍇ,  1:�}�X�^����M�̏ꍇ(= ���X�^�[�g������ꍇ)
volatile uint8_t  iic_sd_num;	    // ���M�f�[�^��(�X���[�u�A�h���X���܂�)
volatile uint8_t  iic_rcv_num;      // ��M�f�[�^��
volatile uint8_t  iic_com_over_fg;  // 1:STOP�R���f�B�V�����̌��o��

				

// RIIC0 EEI0
// �ʐM�G���[/�ʐM�C�x���g����
//( �A�[�r�g���[�V�������X�g���o�ANACK ���o�A�^�C���A�E�g���o�A�X�^�[�g�R���f�B�V�������o�A�X�g�b�v�R���f�B�V�������o)
//
//   �A�[�r�g���[�V�������X�g���o�A�ƃ^�C���A�E�g���o�́A�g�p���Ă��Ȃ��B
//

#pragma interrupt (Excep_RIIC0_EEI0(vect=246))
void Excep_RIIC0_EEI0(void){
	
	if( RIIC0.ICSR2.BIT.START == 1 ) {      // �X�^�[�g(���X�^�[�g)�R���f�B�V�������o
		RIIC0.ICSR2.BIT.START = 0;	// �X�^�[�g�R���f�B�V�������o�t���O�̃N���A
		
	     if ( iic_sd_rcv_fg == 1 ) {	// �}�X�^����M�̏ꍇ(= ���X�^�[�g������ꍇ)
		if ( iic_sd_pt == 3) {      //  	�R�}���h(�ǂݏo�����W�X�^)���M������́A���X�^�[�g�R���f�B�V�������s
			
			RIIC0.ICDRT = iic_sd_data[iic_sd_pt];  // ���M ( �X���[�u�A�h���X(�ǂݏo���p)�̑��M )
			
			iic_sd_pt++;
			
			// �X���[�u�A�h���X(�ǂݏo���p)�̑��M��ɁAICCR2.TRS = 0(��M���[�h)�ƂȂ�A
			// ICSR2.RDRF (��M�f�[�^�t���t���O)�͎����I�Ɂg1�h(ICDRR���W�X�^�Ɏ�M�f�[�^����)�ɂȂ�B
			// �X���[�u�A�h���X(�ǂݏo���p)���M��́A��M���荞�݂ŁA�_�~�[���[�h���邽�߂̃t���O��ݒ�
			 
			 dummy_read_fg = 1;    // �_�~�[���[�h�L��
		  
		 	 RIIC0.ICIER.BIT.TEIE = 0;	// ���M�I�����荞��(TEI)�v���̋֎~
		}
	     }
		
	}
	
	else if ( RIIC0.ICSR2.BIT.STOP == 1 ) {      // STOP ���o
	
	      RIIC0.ICSR2.BIT.STOP = 0;	 //  STOP ���o�t���O�̃N���A	
	      
	     iic_com_over_fg = 1;		// �ʐM����
	      
	}
	
	else if ( RIIC0.ICSR2.BIT.NACKF == 1 ) {      // NACK ���o
	        
		RIIC0.ICSR2.BIT.NACKF = 0;	  // NACK ���o�t���O�̃N���A
	        
		RIIC0.ICCR2.BIT.SP = 1;		   // �X�g�b�v�R���f�B�V�����̔��s�v���̐ݒ�
	}
	
}

// RIIC0 RXI0
// ��M�f�[�^�t���@���荞��
// ICDRR���W�X�^�Ɏ�M�f�[�^����
#pragma interrupt (Excep_RIIC0_RXI0(vect=247))
void Excep_RIIC0_RXI0(void){
	
	uint8_t dummy;
	
	if ( dummy_read_fg == 1 ) {		// �X���[�u�A�h���X(�ǂݏo���p)���M��̃_�~�[���[�h
	
		dummy = RIIC0.ICDRR;		// �_�~�[���[�h�@(SCL�N���b�N���o�͂��āA��M����J�n)
		dummy_read_fg = 0;
	}
	else { 
		
		iic_rcv_data[iic_rcv_pt] = RIIC0.ICDRR;    // ��M�f�[�^�ǂݏo��

		iic_rcv_pt++;
		
		
		 if ( iic_rcv_pt < 2 ) {
		     RIIC0.ICMR3.BIT.ACKBT = 0;		// ACK ���M	
		 }
		 else {					// �ŏI�o�C�g�̎�M
		     RIIC0.ICMR3.BIT.ACKBT = 1;		// NACK ���M	
		      
		     RIIC0.ICCR2.BIT.SP = 1;		// �X�g�b�v�R���f�B�V�����̔��s�v���̐ݒ�
		}
	}
	
}

// RIIC0 TXI0
// ���M�f�[�^�G���v�e�B	���荞��
// ICDRT���W�X�^�ɑ��M�f�[�^�Ȃ��̎��ɁA����
//
//    

#pragma interrupt (Excep_RIIC0_TXI0(vect=248))
void Excep_RIIC0_TXI0(void){
	
	RIIC0.ICDRT = iic_sd_data[iic_sd_pt];  // ���M
	
	iic_sd_pt++;		// ���M�ʒu�̍X�V
	
	if ( iic_sd_rcv_fg == 1 ) {	// �}�X�^����M�̏ꍇ(= ���X�^�[�g������ꍇ)
	    if ( iic_sd_pt == 3) {      //  �R�}���h(�ǂݏo���A�h���X)���M�J�n��
		
		RIIC0.ICIER.BIT.TIE = 0;	// ���M�f�[�^�G���v�e�B���荞��(TXI)�v���̋֎~
		RIIC0.ICIER.BIT.TEIE = 1;	// ���M�I�����荞��(TEI)�v���̋���
	    }
	}
	else {				// �}�X�^���M�A�}�X�^��M�̏ꍇ
	       if ( (iic_sd_data[0] & 0x01) == 1 ) {  // �}�X�^��M�̏ꍇ(MLX90640 �ł͎g�p���Ȃ�)
			// �X���[�u�A�h���X(�ǂݏo���p)�̑��M��ɁAICCR2.TRS = 0(��M���[�h)�ƂȂ�A
			// ICSR2.RDRF (��M�f�[�^�t���t���O)�͎����I�Ɂg1�h(ICDRR���W�X�^�Ɏ�M�f�[�^����)�ɂȂ�B
			// �S�f�[�^�̑��M��́A��M���荞�݂ŁA�_�~�[���[�h���邽�߂̃t���O��ݒ�
			 
			 dummy_read_fg = 1;    // �_�~�[���[�h�L��
	       }
	       else {					// �}�X�^���M�̏ꍇ
	         if ( iic_sd_pt == iic_sd_num ) {	// �S�f�[�^�̑��M���� 
	             RIIC0.ICIER.BIT.TIE = 0;	// ���M�f�[�^�G���v�e�B���荞��(TXI)�v���̋֎~
		     RIIC0.ICIER.BIT.TEIE = 1;	// ���M�I�����荞��(TEI)�v���̋���
	         }
	      }
	}
}

// RIIC0 TEI0
// ���M�I�����荞��
//  ICSR2.BIT.TEND = 1�Ŕ��� ( ICSR2.BIT.TDRE = 1 �̏�ԂŁASCL �N���b�N��9 �N���b�N�ڂ̗����オ��Ŕ���)
#pragma interrupt (Excep_RIIC0_TEI0(vect=249))
void Excep_RIIC0_TEI0(void){
	
	
         RIIC0.ICSR2.BIT.TEND = 0;		//  ���M�����t���O�̃N���A
	
	 if ( iic_sd_rcv_fg == 1 ) {		// �}�X�^����M�̏ꍇ(= ���X�^�[�g������ꍇ)
		RIIC0.ICCR2.BIT.RS = 1;		// ���X�^�[�g�R���f�B�V�����̔��s 
	 }
	 
	 else {					// �}�X�^���M�ŁA�S�f�[�^�̑��M������
	  
	 	RIIC0.ICIER.BIT.TEIE = 0;	// ���M�I�����荞��(TEI)�v���̋֎~
		RIIC0.ICCR2.BIT.SP = 1;	       // �X�g�b�v�R���f�B�V�����̔��s�v���̐ݒ�
	
	 }	    
	 

}



//  RIIC ���M�J�n
void riic_sd_start(void)
{
	iic_sd_pt = 0;				 // ���M�f�[�^�ʒu�@�N���A
	iic_rcv_pt = 0;                          // ��M�f�[�^�ʒu

	iic_com_over_fg = 0;			// �ʐM�����t���O�̃N���A
	
	RIIC0.ICIER.BIT.TIE = 1;		// ���M�f�[�^�G���v�e�B���荞��(TXI)�v���̋���
	
	while(RIIC0.ICCR2.BIT.BBSY == 1){ 	// I2C�o�X�r�W�[��Ԃ̏ꍇ�A���[�v
	}
	
	RIIC0.ICCR2.BIT.ST = 1;		// �X�^�[�g�R���f�B�V�����̔��s  (�}�X�^���M�̊J�n)
					// �X�^�[�g�R���f�B�V�������s��AICSR2.TDRE(���M�f�[�^�G���v�e�B�t���O)=1�ƂȂ�A
					//  TXI(���M�f�[�^�G���v�e�B)���荞�݁A����
}


//  I2C(SMBus)�C���^�[�t�F�C�X �̏����� 
// 
//   	PORT16 = SCL
//      PORT17 = SDA
//
//      PCLKB = 32MHz:
//
//      �]�����x= 1 / { ( (ICBRH + 1) + (ICBRL + 1) ) / (IIC ��) + SCLn ���C�������オ�莞��(tr) + SCLn ���C�����������莞��(tf) }
//
//       (������  29.2.14 I2C �o�X�r�b�g���[�gHigh ���W�X�^(ICBRH)�@���)
//
//     ( ����:�u RX23E-A�O���[�v ���[�U�[�Y�}�j���A���@�n�[�h�E�F�A�ҁv (R01UH0801JJ0120 Rev.1.20)�j 
//


void RIIC0_Init(void)
{
	RIIC0.ICCR1.BIT.ICE = 0;    // RIIC�͋@�\��~(SCL,SDA�[�q��쓮���)
	RIIC0.ICCR1.BIT.IICRST = 1; // RIIC���Z�b�g�A
	RIIC0.ICCR1.BIT.ICE = 1;    // �������Z�b�g��� �ASCL0�ASDA0�[�q�쓮���
		
	RIIC0.ICSER.BYTE = 0x00;    // I2C�o�X�X�e�[�^�X�����W�X�^ �i�}�X�^����̂��߃X���[�u�ݒ�͖���)	
	
	
				     // �ʐM���x = 400 kbps (�I�V������l 348 kbps)
  	RIIC0.ICMR1.BIT.CKS = 1;    // RIIC�̓�����N���b�N = 32/2 = 16 MHz�@
  	RIIC0.ICBRH.BIT.BRH = 0xF4; // 
	RIIC0.ICBRL.BIT.BRL = 0xF4; // 
	 
	
	RIIC0.ICMR3.BIT.ACKWP = 1;	// ACKBT�r�b�g�ւ̏������݋���		
						
					
					
	RIIC0.ICMR3.BIT.RDRFS = 1;	// RDRF�t���O(��M�f�[�^�t��)�Z�b�g�^�C�~���O
					// 1�FRDRF �t���O��8 �N���b�N�ڂ̗����オ��Łg1�h �ɂ��A8 �N���b�N�ڂ̗����������SCL0 ���C����Low �Ƀz�[���h���܂��B
					// ����SCL0 ���C����Low �z�[���h��ACKBT �r�b�g�ւ̏������݂ɂ���������܂��B
					//���̐ݒ�̂Ƃ��A�f�[�^��M��A�N�m���b�W�r�b�g���o�O��SCL0 ���C���������I��Low �Ƀz�[���h���邽�߁A
					// ��M�f�[�^�̓��e�ɉ�����ACK (ACKBT �r�b�g���g0�h) �܂���NACK (ACKBT �r�b�g���g1�h) �𑗏o���鏈�����\�ł��B
			
					
	RIIC0.ICMR3.BIT.WAIT = 0;	// WAIT�Ȃ� (9�N���b�N�ڂ�1�N���b�N�ڂ̊Ԃ�Low�Ƀz�[���h���Ȃ�)	
	
	RIIC0.ICMR3.BIT.SMBS = 0;       // I2C�o�X�I�� 				
	
	 
	RIIC0.ICCR1.BIT.IICRST = 0;	 // RIIC���Z�b�g����
}




//
//
//  I2C(SMBus)�C���^�[�t�F�C�X�p�̃|�[�g��ݒ�
// 
//   	PORT16 = SCL
//      PORT17 = SDA
//

void RIIC0_Port_Set(void)
{
	
	MPC.PWPR.BIT.B0WI = 0;      // �}���`�t�@���N�V�����s���R���g���[���@�v���e�N�g����
    	MPC.PWPR.BIT.PFSWE = 1;     // PmnPFS ���C�g�v���e�N�g����
    
    	MPC.P16PFS.BYTE = 0x0f;     // PORT16 = SCL0
    	MPC.P17PFS.BYTE = 0x0f;     // PORT17 = SDA0
          
    	MPC.PWPR.BYTE = 0x80;      //  PmnPFS ���C�g�v���e�N�g �ݒ�
  
    	PORT1.PMR.BIT.B6 = 1;     // PORT16:���Ӄ��W���[���Ƃ��Ďg�p
    	PORT1.PMR.BIT.B7 = 1;     // PORT17:      :
}



// RIIC �̊��荞�ݗp�A���荞�݃R���g���[���̐ݒ�
// �ȉ����A���荞�ݏ����ōs��
//   EEI: �ʐM�G���[/�ʐM�C�x���g (NACK ���o�A�X�^�[�g�R���f�B�V�������o�A�X�g�b�v�R���f�B�V�������o)
//�@ RXI:�@��M�f�[�^�t��
//   TXI:  ���M�f�[�^�G���v�e�B
//   TEI:  ���M�I��

void RIIC0_Init_interrupt(void)
{
					// �ʐM�G���[/�ʐM�C�x���g ���荞��
	IPR(RIIC0,EEI0) = 10;		// ���荞�݃��x�� = 10�@�@�i15���ō����x��)
	IR(RIIC0,EEI0) = 0;		// ���荞�ݗv���̃N���A
	IEN(RIIC0,EEI0) = 1;		// ���荞�݋���	
	
					// ��M�f�[�^�t��
	IPR(RIIC0,RXI0) = 10;		// ���荞�݃��x�� = 10�@�@�i15���ō����x��)
	IR(RIIC0,RXI0) = 0;		// ���荞�ݗv���̃N���A
	IEN(RIIC0,RXI0) = 1;		// ���荞�݋���	
	
					// ���M�f�[�^�G���v�e�B
	IPR(RIIC0,TXI0) = 10;		// ���荞�݃��x�� = 10�@�@�i15���ō����x��)
	IR(RIIC0,TXI0) = 0;		// ���荞�ݗv���̃N���A
	IEN(RIIC0,TXI0) = 1;		// ���荞�݋���	
	
					// ���M�I��
	IPR(RIIC0,TEI0) = 10;		// ���荞�݃��x�� = 10�@�@�i15���ō����x��)
	IR(RIIC0,TEI0) = 0;		// ���荞�ݗv���̃N���A
	IEN(RIIC0,TEI0) = 1;		// ���荞�݋���	
	
	
	
	RIIC0.ICIER.BIT.TMOIE = 0;	// �^�C���A�E�g���荞��(TMOI)�v���̋֎~
	RIIC0.ICIER.BIT.ALIE  = 0;   	// �A�[�r�g���[�V�������X�g���荞��(ALI)�v���̋֎~
	
	RIIC0.ICIER.BIT.STIE  = 1;	// �X�^�[�g�R���f�B�V�������o���荞��(STI)�v���̋���
	RIIC0.ICIER.BIT.SPIE  = 1;      // �X�g�b�v�R���f�B�V�������o���荞��(SPI)�v���̋���
	RIIC0.ICIER.BIT.NAKIE  = 1;	// NACK��M���荞��(NAKI)�v���̋���

	RIIC0.ICIER.BIT.RIE = 1;	// ��M�f�[�^�t�����荞��(RXI)�v���̋���
	RIIC0.ICIER.BIT.TIE = 0;	// ���M�f�[�^�G���v�e�B���荞��(TXI)�v���̋֎~
	RIIC0.ICIER.BIT.TEIE = 0;	// ���M�I�����荞��(TEI)�v���̋֎~
	
}
