
#include "iodefine.h"
#include "misratypes.h"
#include "dma.h"
#include "crc_16.h"
#include "sci.h"

// DMA
// �`�����l��   �N���v���@�@ 
//   0        RXI1  (���荞�݃x�N�^�ԍ�=219�j  SCI1 ��M�f�[�^�t���@
//   1        TXI1  (���荞�݃x�N�^�ԍ�=220�j  SCI1 ���M�f�[�^�G���v�e�B
//   2        SPTI0  (���荞�݃x�N�^�ԍ�=46�j  RSPI0 SPTI0 ���M�f�[�^�G���v�e�B
//


// DMA(�`�����l��1) �]���I�����荞��  
// DMAC DMAC0I
//  SCI1 1�̓d���̎�M�I���Ŕ��� (8 byte�̎�M�I���Ŕ���)
#pragma interrupt (Excep_DMAC_DMAC0I(vect=198))
void Excep_DMAC_DMAC0I(void)
{
	uint32_t i;
	
	Init_CRC();		// CRC���Z��̏�����
	
	for ( i = 0 ; i < DMA0_SCI_RCV_DATA_NUM ; i++ ) {	// CRC�v�Z
	
		CRC.CRCDIR = rcv_data[i];
	}
	
	if ( CRC.CRCDOR == 0 ) {   // CRC�̉��Z���� OK�̏ꍇ
		
		rcv_over = 1;      // ��M�����t���O�̃Z�b�g
		crc_16_err = 0;
		
		LED_RX_PODR = 1;   // ��M LED�̓_��
	}
	else {   		   // CRC�̉��Z���� NG�̏ꍇ
		crc_16_err = 1;
	}
	
	
	IEN(SCI1,RXI1) = 0;			// ��M���荞�݋֎~
	
	DMA0_SCI_RCV_SET();			// ���̓d����M�̂��ߍĐݒ�
	
	IR(SCI1,RXI1) = 0;			// RXI ���荞�ݗv�����N���A
	IEN(SCI1,RXI1) = 1;			// ��M���荞�݋���
	
}


// DMA�]���I�����荞��  
// DMAC DMAC1I
#pragma interrupt (Excep_DMAC_DMAC1I(vect=199))
void Excep_DMAC_DMAC1I(void)
{
     
	SCI1.SCR.BIT.TEIE = 1;  	// TEI���荞��(���M�I�����荞��)���� (�S�f�[�^���M�����Ŕ���)
}



// DMA(�`�����l��2) �]���I�����荞��  
// DMAC DMAC2I

#pragma interrupt (Excep_DMAC_DMAC2I(vect=200))

void Excep_DMAC_DMAC2I(void)
{	
 	RSPI0.SPCR.BIT.SPTIE = 0;       //  RSPI ���M�o�b�t�@�G���v�e�B���荞�ݗv���̔������֎~
        
	RSPI0.SPCR2.BIT.SPIIE = 1;	// �A�C�h�����荞�ݗv���̐���������
}



//
//  DMA �`�����l��0  �V���A���f�[�^��M�̂��߂̐ݒ�
//
void DMA0_SCI_RCV_SET(void)
{
	DMAC0.DMSAR = (void *)&SCI1.RDR;	 // �]�����A�h���X SCI1 ��M�f�[�^���W�X�^		
        DMAC0.DMDAR = (void *)&rcv_data[0];	 // �]����A�h���X ��M�o�b�t�@
        DMAC0.DMCRA = DMA0_SCI_RCV_DATA_NUM; 	  // �]���� (��M�o�C�g���@8byte�Œ�)	
	DMAC0.DMCNT.BIT.DTE = 1;    	         // DMAC0 (DMAC �`�����l��0) �]������
}


//
// DMA �`�����l��0 �����ݒ� (�p�\�R������̃V���A���f�[�^��M�p)
//

void DMA0_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC ��~
	
	DMAC0.DMCNT.BIT.DTE = 0;    // DMAC1 (DMAC �`�����l��0) �]���֎~
	
	ICU.DMRSR0 = 219;           // DMA�N���v���@��M�f�[�^�t�� RXI�i���荞�݃x�N�^�ԍ�=219�j  SCI1 ��M���荞�݂́ADMA0�ŏ���
	
	
	DMAC0.DMAMD.WORD = 0x0080;  // �]����=�A�h���X�Œ�A�]����=�C���N�������g
	DMAC0.DMTMD.WORD = 0x2001;  // �m�[�}���]���A���s�[�g�A�u���b�N�̈�Ȃ��A8bit�]���A���Ӄ��W���[������̊��荞�݂ɂ��J�n
	
	DMAC0.DMINT.BIT.DTIE = 1;   // �w�肵���񐔂̃f�[�^�]�����I�������Ƃ��̓]���I�����荞�ݗv��������
	
	IPR(DMAC,DMAC0I) = 9;		// �]���I�����荞�݃��x�� = 9
	IEN(DMAC,DMAC0I) = 1;		// �]���I�����荞�݋���
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC �N��

}


//
// DMA �`�����l��1 �����ݒ� (�p�\�R���ւ̃V���A���f�[�^���M�p)
//
void DMA1_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC ��~
	
	DMAC1.DMCNT.BIT.DTE = 0;    // DMAC1 (DMAC �`�����l��1) �]���֎~
	
	ICU.DMRSR1 = 220;           // DMA�N���v���@TXI1�i���荞�݃x�N�^�ԍ�=220�j  SCI1 ���M���荞�݂́ADMA1�ŏ���
	
	
	DMAC1.DMAMD.WORD = 0x8000;  // �]����=�C���N�������g�A�]����=�A�h���X�Œ�
	DMAC1.DMTMD.WORD = 0x2001;  // �m�[�}���]���A���s�[�g�A�u���b�N�̈�Ȃ��A8bit�]���A���Ӄ��W���[������̊��荞�݂ɂ��J�n
	
	DMAC1.DMINT.BIT.DTIE = 1;   // �w�肵���񐔂̃f�[�^�]�����I�������Ƃ��̓]���I�����荞�ݗv��������
	
	IPR(DMAC,DMAC1I) = 9;		// �]���I�����荞�݃��x�� = 9
	IEN(DMAC,DMAC1I) = 1;		// �]���I�����荞�݋���
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC �N��

	
}

//
// DMA �`�����l��2 �����ݒ� (LCD�R���g���[�� ILI9488�ւ̃f�[�^���M�p)
//

void DMA2_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC ��~
	
	DMAC2.DMCNT.BIT.DTE = 0;    // DMAC2 (DMAC �`�����l��2) �]���֎~
			           
				   // 14.2.7 DMAC �N���v���I�����W�X�^m (DMRSRm) (m = DMAC �`���l���ԍ�)
	ICU.DMRSR2 = 46;           // DMA�N���v���@SPTI0�i���荞�݃x�N�^�ԍ�=46�j RSPI0 ���M�o�b�t�@�G���v�e�B���荞�݂́ADMA0�ŏ���
	
	
	DMAC2.DMAMD.WORD = 0x8000;  // �]����=�C���N�������g�A�]����=�A�h���X�Œ�
	DMAC2.DMTMD.WORD = 0x2101;  // �m�[�}���]���A���s�[�g�A�u���b�N�̈�Ȃ��A16bit�]���A���Ӄ��W���[������̊��荞�݂ɂ��J�n
	
	DMAC2.DMINT.BIT.DTIE = 1;   // �w�肵���񐔂̃f�[�^�]�����I�������Ƃ��̓]���I�����荞�ݗv��������
	
	IPR(DMAC,DMAC2I) = 8;	    // �]���I�����荞�݃��x�� = 8
	IEN(DMAC,DMAC2I) = 1;	    // �]���I�����荞�݋���
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC �N��

}

