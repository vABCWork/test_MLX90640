
#include "iodefine.h"
#include "misratypes.h"
#include "sci.h"
#include "crc_16.h"
#include "mlx90640_iic.h"
#include "thermocouple.h"

//
//  SCI1 �V���A���ʐM(��������)����
//
// �p�\�R���Ƃ̒ʐM�@��M�p
volatile uint8_t  rcv_data[8];
volatile uint8_t rxdata;
volatile uint8_t rcv_over;
volatile uint8_t  rcv_cnt;

// �p�\�R���Ƃ̒ʐM ���M�p
volatile uint8_t sd_data[1687];
volatile uint8_t  send_cnt;
volatile uint8_t  send_pt;


//
// SCI1 ���M�I�����荞��(�p�\�R���Ƃ̒ʐM�p)
//  (���M��DMA�Ŏ��{)
//
#pragma interrupt (Excep_SCI1_TEI1(vect=221))
void Excep_SCI1_TEI1(void)
{	 
	SCI1.SCR.BIT.TE = 0;            // ���M�֎~
        SCI1.SCR.BIT.TIE = 0;           // ���M���荞�݋֎~	        
	SCI1.SCR.BIT.TEIE = 0;  	// TEI���荞��(���M�I�����荞��)�֎~

	LED_TX_PODR = 0;	        // ���M LED�̏���
	
 }
 




// �R�}���h��M�̑΂���A�R�}���h�����ƃ��X�|���X�쐬���� 
//   �y���`�F�R���g���[���[�p�AMLX90640�p
//

void comm_cmd(void){
   
	uint8_t  cmd;
	
	uint32_t sd_cnt;

	if ( crc_16_err == 1 ) {	// ��M����CRC�s��v�̏ꍇ�A�p�\�R�����֕ԑ����Ȃ��B�p�\�R�����̓^�C���A�E�g����B
				        // �A���[������
	    return;
	}
	
	
	cmd = rcv_data[0];
	
	sd_cnt = 0;
				    
	if ( cmd == 0x30 ) {	     // MLX EEPROM (0x2400-0x273F) �Ǐo��
	   sd_cnt = mlx_sci_read_eeprom();	
	}	
	
	else if ( cmd == 0x31 ) {     // MLX RAM (0x400-0x73F),EM, �M�d�΂̉��x �Ǐo��
	   sd_cnt = mlx_sci_read_ram();	
	}
	 
	
	DMAC1.DMSAR = (void *)&sd_data[0];	 // �]�����A�h���X		
	DMAC1.DMDAR = (void *)&SCI1.TDR;	 // �]����A�h���X TXD12 ���M�f�[�^

	DMAC1.DMCRA = sd_cnt; 	 	// �]���� (���M�o�C�g��)	
	    
	DMAC1.DMCNT.BIT.DTE = 1;    // DMAC1 (DMAC �`�����l��1) �]������
	
	    			   // ��ԍŏ��̑��M���荞��(TXI)�𔭐������鏈���B ( RX23E-A ���[�U�[�Y�}�j���A���@�n�[�h�E�F�A�ҁ@28.3.7 �V���A���f�[�^�̑��M�i�������������[�h�j)�@
	SCI1.SCR.BIT.TIE = 1;      // ���M���荞�݋���
	SCI1.SCR.BIT.TE = 1;	   // ���M����
	
	LED_TX_PODR = 1;	   // ���M LED�̓_��
}





// MLX90640�� EEPROM�@(0x2400-0x273f)�ǂݏo���R�}���h(0x030)
//
// ��M�f�[�^:
//  rcv_data[0];�@0x30 (�R�}���h)
//  rcv_data[1]:  dummy 0
//  rcv_data[2]:  dummy 0
//  rcv_data[3]:  dummy 0
//  rcv_data[4]:  dummy 0
//  rcv_data[5]:  dummy 0
//  rcv_data[6]: CRC(��ʃo�C�g��)
//  rcv_data[7]: CRC(���ʃo�C�g��)

// ���M�f�[�^ :
//     sd_data[0] : 0xb0 (�R�}���h�ɑ΂��郌�X�|���X)
//     se_data[1] : �A�h���X(0x2400)�̃f�[�^(��ʃo�C�g��)
//     sd_data[2] : �A�h���X(0x2400)�̃f�[�^(���ʃo�C�g��)
//     sd_data[3] : �A�h���X(0x2401)�̃f�[�^(��ʃo�C�g��)
//     sd_data[4] : �A�h���X(0x2401)�̃f�[�^(���ʃo�C�g��)
//          :                  :
//          :                  :
//     sd_data[1663]: �A�h���X(0x273F)�̃f�[�^(��ʃo�C�g��)
//     sd_data[1664]: �A�h���X(0x273F)�̃f�[�^(���ʃo�C�g��)
//     sd_data[1665]: CRC(��ʃo�C�g��)
//     sd_data[1666]: CRC(���ʃo�C�g��

uint32_t mlx_sci_read_eeprom(void)
{
	uint16_t crc_cd;
	
	uint32_t i;
	uint32_t cnt;
	
	cnt = 1667;			// ���M�o�C�g��
	
	sd_data[0] = 0xb0;	 	// �R�}���h�ɑ΂��郌�X�|���X	
	
	for ( i = 0; i < 832 ; i++ ){   // EEPROM 0x2400-0x273F, 832 word = 1664 byte

	   sd_data[i*2 + 1] = (mlx_eeprom[i] >> 8); // ��ʃo�C�g��
	   
	   sd_data[i*2 + 2] = mlx_eeprom[i];    // ���ʃo�C�g��
	}
	
	
	crc_cd = cal_crc_sd_data( cnt - 2 );   // CRC�̌v�Z
	
	sd_data[1665] = crc_cd >> 8;	// CRC��ʃo�C�g
	sd_data[1666] = crc_cd;		// CRC���ʃo�C�g
	
	return cnt;
	
}


// MLX90640�� RAM�@(0x400-0x73f), Em, �M�d�΂̉��x �ǂݏo���R�}���h(0x031)
//
// ��M�f�[�^
//  rcv_data[0];�@0x31 (�R�}���h)
//  rcv_data[1]:  dummy 0
//  rcv_data[2]:  dummy 0
//  rcv_data[3]:  dummy 0
//  rcv_data[4]:  dummy 0
//  rcv_data[5]:  dummy 0
//  rcv_data[6]: CRC(��ʃo�C�g��)
//  rcv_data[7]: CRC(���ʃo�C�g��)

// ���M�f�[�^ :
//     sd_data[0] : 0xb1 (�R�}���h�ɑ΂��郌�X�|���X)
//     se_data[1] : �A�h���X(0x400)�̃f�[�^(��ʃo�C�g��)
//     sd_data[2] : �A�h���X(0x400)�̃f�[�^(���ʃo�C�g��)
//     sd_data[3] : �A�h���X(0x401)�̃f�[�^(��ʃo�C�g��)
//     sd_data[4] : �A�h���X(0x401)�̃f�[�^(���ʃo�C�g��)
//          :                  :
//          :                  :
//     sd_data[1663]: �A�h���X(0x73F)�̃f�[�^(��ʃo�C�g��)
//     sd_data[1664]: �A�h���X(0x73F)�̃f�[�^(���ʃo�C�g��)
//
//     sd_data[1665]:Control register 1 (0x800d)(��ʃo�C�g��)
//     sd_data[1666]:Control register 1 (0x800d)(���ʃo�C�g��)
//     sd_data[1667]:Status register (0x8000)(��ʃo�C�g��)
//     sd_data[1668]:Status register (0x8000)(���ʃo�C�g��)
//
//     sd_data[1669] : ���˗�(Em)  (���ʃo�C�g��) 100�{�����l (��:Em=0.95�Ȃ��950��Ԃ�)
//     sd_data[1670] :   :         (��ʃo�C�g��)
//     sd_data[1671] : ���͉��x(Ta)(���ʃo�C�g��) 10�{�����l (��:Ta=23.5�Ȃ��235��Ԃ�)
//     se_data[1672] :   :         (��ʃo�C�g��)
//     sd_data[1673] : ���ˉ��x(Tr)(���ʃo�C�g��) 10�{�����l
//     sd_data[1674] :   :         (��ʃo�C�g��)
//     sd_data[1675] : �M�d�� CH1���x  (���ʃo�C�g��)
//     se_data[1676] :    :            (��ʃo�C�g��)
//     sd_data[1677] : �M�d�� CH2���x (���ʃo�C�g��)  
//     sd_data[1678] :    :	    (��ʃo�C�g��)
//     sd_data[1679] : �M�d�� CH3���x (���ʃo�C�g��) 
//     se_data[1680] :    :           (��ʃo�C�g��)
//     sd_data[1681] : �M�d�� CH4���x (���ʃo�C�g��)  
//     sd_data[1682] :    :�@�@�@�@�@ (��ʃo�C�g��)
//     sd_data[1683] : ��ړ_���x(CJT)  (���ʃo�C�g��)  
//     se_data[1684] :    :               (��ʃo�C�g��)
//
//     sd_data[1685]: CRC(��ʃo�C�g��)
//     sd_data[1686]: CRC(���ʃo�C�g��



uint32_t mlx_sci_read_ram(void)
{
	int16_t   x_ch1, x_ch2, x_ch3, x_ch4, x_cjt;
	int16_t   x_ta,x_tr;

	uint16_t  x_em;
	uint32_t i;
	uint32_t cnt;
	
	uint16_t crc_cd;
	
	cnt = 1687;			// ���M�o�C�g��
	
	sd_data[0] = 0xb1;	 	// �p�����[�^�������݃R�}���h�ɑ΂��郌�X�|���X	
	
	for ( i = 0; i < 832 ; i++ ){   // EEPROM 0x2400-0x273F, 832 word = 1664 byte
	
	   sd_data[i*2 + 1] = (mlx_ram[i] >> 8); // ��ʃo�C�g��
	   
	   sd_data[i*2 + 2] = mlx_ram[i];    // ���ʃo�C�g��
	}
	
	sd_data[1665] =  (mlx_control_reg_1 >> 8);  // ��ʃo�C�g��
	sd_data[1666] = mlx_control_reg_1;	    // ���ʃo�C�g��
	
	sd_data[1667] =  (mlx_status_reg >> 8);  // ��ʃo�C�g��
	sd_data[1668] = mlx_status_reg;	         // ���ʃo�C�g��
	
	x_em = mlx_emissivity * 100.0;  // ���˗�(Em)��100�{
	sd_data[1669] = x_em;		// Low�o�C�g��
	sd_data[1670] = x_em >> 8;		// High�o�C�g��
	
	x_ta = mlx_ta * 10.0;		// ���͉��x(Ta)��10�{
	sd_data[1671] = x_ta;		// Low�o�C�g��
	sd_data[1672] = x_ta >> 8;		// High�o�C�g��
	
	x_tr = mlx_tr * 10.0;		// �����␳�p ���ˉ��x(Tr)��10�{
	sd_data[1673] = x_tr;		// Low�o�C�g��
	sd_data[1674] = x_tr >> 8;		// High�o�C�g��
	
	x_ch1 = tc_temp[0] * 10.0;	// ch1��10�{
	sd_data[1675] = x_ch1;		// Low�o�C�g��
	sd_data[1676] = x_ch1 >> 8;	// High�o�C�g��
	
        x_ch2 = tc_temp[1] * 10.0;	// ch2��10�{
	sd_data[1677] = x_ch2;		// Low�o�C�g��
	sd_data[1678] = x_ch2 >> 8;	// High�o�C�g��
	
	x_ch3 = tc_temp[2] * 10.0;	// ch3��10�{
	sd_data[1679] = x_ch3;		// Low�o�C�g��
	sd_data[1680] = x_ch3 >> 8;	// High�o�C�g��
	
        x_ch4 = tc_temp[3] * 10.0;	// ch4��10�{
	sd_data[1681] = x_ch4;		// Low�o�C�g��
	sd_data[1682] = x_ch4 >> 8;	// High�o�C�g��
	
	x_cjt = cj_temp * 10.0;		// ��ړ_���x��10�{
	sd_data[1683] = x_cjt;
	sd_data[1684] = x_cjt >> 8;
	
	crc_cd = cal_crc_sd_data( cnt - 2 );   // CRC�̌v�Z
	
	sd_data[1685] = crc_cd >> 8;	// CRC��ʃo�C�g
	sd_data[1686] = crc_cd;		// CRC���ʃo�C�g
	
	return cnt;
	
}





// �@���M�f�[�^��CRC�v�Z
//   sd_data[0]����num�̃f�[�^��CRC���v�Z����B
//
uint16_t    cal_crc_sd_data( uint16_t num )
{
	uint16_t  crc;
	
	uint32_t i;

	Init_CRC();			// CRC���Z��̏�����
	
	for ( i = 0 ; i < num ; i++ ) {	// CRC�v�Z
	
		CRC.CRCDIR = sd_data[i];
	}
	
	crc = CRC.CRCDOR;        // CRC�̉��Z����
	
	return crc;
}



// 
// SCI1 �����ݒ�
//  8bit-non parity-1stop
//  PCLKB = 32MHz
//  TXD1= P16,  RXD1 = P15
//
//  (�\28.10 BRR���W�X�^�̐ݒ�lN�ƃr�b�g���[�gB�̊֌W)���
// 
// 1) BDGM=0,ABCS=0 �̏ꍇ (SCI1.SEMR.BIT.BGDM = 0 , SCI1.SEMR.BIT.ABCS = 0;)      
//   N = {(32 x 1000000)/((64/2) x B)} - 1
//    N: BRR���W�X�^�̒l
//�@�@B: �{�[���[�g bps
//    
//    B= 1000000 = 1[Mbps] �Ƃ���ƁAN = 0
//
//  �덷:
//  �덷 =  { (32 x 1000000) / ( B x (64/2) x (N+1)) - 1 } x 100
//       = 0 %
//
//  2) BDGM= 1,ABCS=0 �̏ꍇ (�{�����[�h)      
//   N = (32 x 1000000/(32/2)xB)-1
//    
//    N = 0 �Ƃ���ƁAB= 2 [Mbps]
//
//
//

void initSCI_1(void)
{
	
	MPC.PWPR.BIT.B0WI = 0;   // �}���`�t�@���N�V�����s���R���g���[���@�v���e�N�g����
	MPC.PWPR.BIT.PFSWE = 1;  // PmnPFS ���C�g�v���e�N�g����
	
	MPC.P30PFS.BYTE = 0x0A;  // P30 = RXD1
	MPC.P26PFS.BYTE = 0x0A;  // P26 = TXD1
	
	
	MPC.PWPR.BYTE = 0x80;    //  PmnPFS ���C�g�v���e�N�g �ݒ�
			
	PORT3.PMR.BIT.B0 = 1;	// P30 ���Ӄ��W���[���Ƃ��Ďg�p
	PORT2.PMR.BIT.B6 = 1;   // P26 ���Ӄ��W���[���Ƃ��Ďg�p
		
	
	SCI1.SCR.BYTE = 0;	// �����{�[���[�g�W�F�l���[�^�A����M�֎~
	SCI1.SMR.BYTE = 0;	// PCLKB(=32MHz), ��������,8bit,parity �Ȃ�,1stop
	
	
	SCI1.BRR = 0;			// 1 [Mbps] 
	SCI1.SEMR.BIT.BGDM = 0;         // 0= �{�[���[�g�W�F�l���[�^����ʏ�̎��g���̃N���b�N���o��
	SCI1.SEMR.BIT.ABCS = 0;         // 0= ��{�N���b�N16�T�C�N���̊��Ԃ�1�r�b�g���Ԃ̓]�����[�g�ɂȂ�܂�
	
//	SCI1.BRR = 0;			// 2 [Mbps] 
//	SCI1.SEMR.BIT.BGDM = 1;         // 1= �{�[���[�g�W�F�l���[�^����2�{�̎��g���̃N���b�N���o��
//	SCI1.SEMR.BIT.ABCS = 0;         // 0= ��{�N���b�N16�T�C�N���̊��Ԃ�1�r�b�g���Ԃ̓]�����[�g�ɂȂ�܂�
	
	
	
	SCI1.SCR.BIT.TIE = 0;		// TXI���荞�ݗv���� �֎~
	SCI1.SCR.BIT.RIE = 1;		// RXI�����ERI���荞�ݗv���� ����
	SCI1.SCR.BIT.TE = 0;		// �V���A�����M����� �֎~�@�i������ TE=1�ɂ���ƁA��ԍŏ��̑��M���荞�݂��������Ȃ�)
	SCI1.SCR.BIT.RE = 1;		// �V���A����M����� ����
	
	SCI1.SCR.BIT.MPIE = 0;         // (�������������[�h�ŁASMR.MP�r�b�g= 1�̂Ƃ��L��)
	SCI1.SCR.BIT.TEIE = 0;         // TEI���荞�ݗv�����֎~
	SCI1.SCR.BIT.CKE = 0;          // �����{�[���[�g�W�F�l���[�^
	
	
	IEN(SCI1,RXI1) = 1;		// ��M���荞�݋���
	
	IEN(SCI1,TXI1) = 1;		// ���M���荞�݋���
	
	IPR(SCI1,TEI1) = 12;		// ���M���� ���荞�݃��x�� = 12 �i15���ō����x��)
	IEN(SCI1,TEI1) = 1;		// ���M�������荞�݋���
	
	rcv_cnt = 0;			// ��M�o�C�g���̏�����
	Init_CRC();			// CRC���Z��̏�����
	
	
}


//  ���M���Ǝ�M����LED�@�o�̓|�[�g�ݒ� (�p�\�R���Ƃ̒ʐM�p)
 void LED_comm_port_set(void)	
 {
					// ���M�@�\���pLED
	  LED_TX_PDR = 1;		// �o�̓|�[�g�Ɏw��
	  
	 				// ��M�@�\���pLED
	  LED_RX_PDR = 1;		// �o�̓|�[�g�Ɏw��
 }

