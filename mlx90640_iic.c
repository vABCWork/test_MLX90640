#include "iodefine.h"
#include "misratypes.h"
#include "mlx90640_iic.h"
#include "riic_mlx.h"


volatile uint8_t iic_comm_fg;	// �ʐM���t���O�@STOP���s�ŃN���A
volatile uint8_t mlx_status_read_req;  // �X�e�[�^�X���W�X�^�ǂݏo���v���t���O

uint16_t mlx_status_reg; // �X�e�[�^�X���W�X�^ RAM(0x8000)�̓��e
uint16_t mlx_control_reg_1;  // �R���g���[�����W�X�^ 1 (0x800D)�̓��e


uint16_t mlx_ram[834];		// MLX90640 RAM(0x0400-0x073F) (0x33F = 831)�̃f�[�^��(Control register1)�� (statusRegister & 0x0001)
uint16_t mlx_eeprom[832];	// MLX90640 EEPROM(0x2400-0x273F) (0x33F = 831) 


uint8_t mlx_subpage0_ready;      // 1:Subpage0�@�ǂ݂����ς�
uint8_t mlx_subpage1_ready;      //  1:Subpage1 �ǂݏo���ς�


float mlx_emissivity;		// ���˗�

float mlx_ta;      		// ���͉��x[��]
float mlx_tr;			// �����␳
float mlx_to[768];		//�@�e�s�N�Z��(1�`768)�̌v�Z�������x




// MLX90640 RAM(0x0400-0x073F)�̃f�[�^��(Control register1)�� (statusRegister & 0x0001)�̓ǂݏo��
//
void MLX_Get_FrameData(void)
{
	Read_MLX_RAM();		//  RAM(0x400�`0x73F)�ǂݏo���Amlx_ram[0]�`mlx_ram[831] �Ɋi�[����B
	
	Read_MLX_Control_Register_1(); // �R���g���[�����W�X�^1(0x800D)�ǂݏo���Amlx_control_reg_1�Ɋi�[
	
	mlx_ram[832] = mlx_control_reg_1;
	
	Read_MLX_Status_Register();	// �X�e�[�^�X���W�X�^(0x8000)�ǂݏo���Amlx_status_reg�Ɋi�[
	
	mlx_ram[833] = mlx_status_reg & 0x0001;  // 0=subpage 0 measured , 1=subpage 1 measured.
	

}



// Control register 1: 0x800D
//   �f�t�H���g = 0x1901 
//
//  b0 : Enable Subpage mode 
//  0 = No Subpages, only one page will be measured
//  1 = Subpage mode is activeted (default)
//  
//  b1 : Melexis reserved
//  0 = (default)
//
//  b2 : Enable data hold
//  0 = Transfer the data into storage RAM at each measured frame (default)
//  1 = Transfer the data into storage RAM only if en_overwrite = 1 (check 0x8000)
//
//  b3 : Enable subpage repeat
//  0 = Toggles between subpage "0" and subpage "1" if Enable subpages mode = "1" (default) , 
//  1 = Select subpage determines which subpage to be measured if Enable subpages mode = "1"
//
//  b6,b5,b4 : Select subpate
//  0, 0, 0   = Subpage 0 is selected (default)
//  0, 0, 1   = Subpage 1 is selected
//   others   = Not Applicable
//
//  b9,b8,b7 : Refresh rate control
//  0, 0, 0   = IR refrsh rate  0.5 [Hz]
//  0, 0, 1   = IR refrsh rate  1 [Hz]
//  0, 1, 0   = IR refrsh rate  2 [Hz] (default)
//  0, 1, 1   = IR refrsh rate 4 [Hz]
//  1, 0, 0   = IR refrsh rate 8 [Hz]
//  1, 0, 1   = IR refrsh rate 16 [Hz]
//  1, 1, 0   = IR refrsh rate 32 [Hz]
//  1, 1, 1   = IR refrsh rate 64 [Hz]
//
//  b11,b10  : Resolution control
//  0, 0      = ADC set to 16 bit resolution
//  0, 1      = ADC set to 17 bit resolution
//  1, 0      = ADC set to 18 bit resolution (default)
//  1, 1      = ADC set to 19 bit resolution
//
//  b12      : Reading pattern
//  0 = Interleaved(TV) mode
//  1 = Chess pattern(default)
//
//  b15 b14 b13 : Melexis reserved



// ���t���b�V�����[�g�́@4[Hz]�ݒ�
void MLX_Set_Refresh_rate(void)
{
				
	iic_sd_data[3] = 0x19;             // IR refrsh rate 4 [Hz]�ݒ�
	iic_sd_data[4] = 0x81;

	 
    	 Write_MLX_Control_Register_1(); 	// MLX90640 �R���g���[�����W�X�^1(0x800D)��������
	 
	 while( iic_com_over_fg != 1 ) {     // �ʐM�����҂�(��M�����҂�)
	 }
}


//
// MLX90640 �X�e�[�^�X���W�X�^(0x8000)�ǂݏo��
//	mlx_status_reg�Ɋi�[
//
//   IIC ���M�o�b�t�@
//   �@sci_iic_sd_data[0] : �X���[�u�A�h���X(7bit) + Wr�r�b�g(0)
//     sci_iic_sd_data[1] : �ǂݏo���A�h���X(��ʃo�C�g��)
//     sci_iic_sd_data[2] : �ǂݏo���A�h���X(���ʃo�C�g��)
//     sci_iic_sd_data[3] : �X���[�u�A�h���X(7bit) + Rd�r�b�g(1)
//
void Read_MLX_Status_Register(void)
{
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // �������ݗp �X���[�u�A�h���X
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // �ǂݏo���p�@�X���[�u�A�h���X 
	
	iic_sd_data[1] = 0x80;             // Status register �A�h���X = 0x8000
	iic_sd_data[2] = 0x00;
	
	iic_sd_rcv_fg = 1;			// �}�X�^����M����
	
	riic_sd_start();			// SCI IIC ���M�J�n
	
	
	while( iic_com_over_fg != 1 ) {     // �ʐM�����҂�(��M�����҂�)
	}
	 
	mlx_status_reg = iic_rcv_data[0];	// ��ʃo�C�g

	mlx_status_reg = ( mlx_status_reg << 8 ) | iic_rcv_data[1];
	
	
}

//
// MLX90640 �R���g���[�����W�X�^1(0x800D)�ǂݏo��
//	mlx_control_reg_1�Ɋi�[
//
//   IIC ���M�o�b�t�@
//   �@sci_iic_sd_data[0] : �X���[�u�A�h���X(7bit) + Wr�r�b�g(0)
//     sci_iic_sd_data[1] : �ǂݏo���A�h���X(��ʃo�C�g��)
//     sci_iic_sd_data[2] : �ǂݏo���A�h���X(���ʃo�C�g��)
//     sci_iic_sd_data[3] : �X���[�u�A�h���X(7bit) + Rd�r�b�g(1)
//
void Read_MLX_Control_Register_1(void)
{
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // �������ݗp �X���[�u�A�h���X
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // �ǂݏo���p�@�X���[�u�A�h���X 
	
	iic_sd_data[1] = 0x80;             // Cotrl register 1 �A�h���X = 0x800d
	iic_sd_data[2] = 0x0d;
	
	iic_sd_rcv_fg = 1;			// �}�X�^����M����
	
	riic_sd_start();			// SCI IIC ���M�J�n
	
	
	while( iic_com_over_fg != 1 ) {     // �ʐM�����҂�(��M�����҂�)
	}
	 
	mlx_control_reg_1 = iic_rcv_data[0];	// ��ʃo�C�g

	mlx_control_reg_1 = ( mlx_control_reg_1 << 8 ) | iic_rcv_data[1];
	
	
}


//
// MLX90640 �R���g���[�����W�X�^ 1(0x800D)��������
//
//   IIC ���M�o�b�t�@
//   �@sci_iic_sd_data[0] : �X���[�u�A�h���X(7bit) + Wr�r�b�g(0)
//     sci_iic_sd_data[1] : �������݃A�h���X(��ʃo�C�g��)
//     sci_iic_sd_data[2] : �������݃A�h���X(���ʃo�C�g��)
//     sci_iic_sd_data[3] : �������݃f�[�^(��ʃo�C�g��)
//     sci_iic_sd_data[4] : �������݃f�[�^(���ʃo�C�g��)
//

void Write_MLX_Control_Register_1(void)
{
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // �������ݗp �X���[�u�A�h���X
	
	iic_sd_data[1] = 0x80;             // Cotrl register 1 �A�h���X = 0x800d
	iic_sd_data[2] = 0x0d;
	
	iic_sd_rcv_fg = 0;			// �}�X�^���M����
	iic_sd_num = 5;				// ���M�f�[�^��
	
	riic_sd_start();			// SCI IIC ���M�J�n
	
}



//
// MLX90640 RAM(0x400�`0x73F)�ǂݏo���A
//�@uint16_t mlx_ram[832] �Ɋi�[����B
//
void Read_MLX_RAM(void)
{
	uint16_t i;
	uint16_t adrs;
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // �������ݗp �X���[�u�A�h���X
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // �ǂݏo���p�@�X���[�u�A�h���X 
	
	for ( i = 0 ; i < 832 ; i++ ) {    // RAM 0x400-0x73F�@�̓ǂݏo�� 832 word
	
	  adrs = 0x400 + i;	
		
	  iic_sd_data[1] = (adrs >> 8 );    //  �ǂݏo���A�h���X�̏�ʃo�C�g
	  iic_sd_data[2] = adrs & 0xff;	//  �ǂݏo���A�h���X�̉��ʃo�C�g
	
	  iic_sd_rcv_fg = 1;		// �}�X�^����M����
	
	  riic_sd_start();			// SCI IIC ���M�J�n
	
	  while( iic_com_over_fg != 1 ) {     // �ʐM�����҂�(��M�����҂�)

	  }
	
	  mlx_ram[i] = (( iic_rcv_data[0] << 8 ) | iic_rcv_data[1] );    // �ǂݏo���f�[�^�̊i�[
	
	}
	
	
	
	
}



//   MLX90640 EEPROM(0x2400-0x273F)����f�[�^��ǂݏo���A
//   int mlx_eeprom[832]�֊i�[����B  0x340 ( = 832 )
//   142[msec]������B(at I2C CLK= 400[KHz])�@Aug 23
void Read_MLX_EEPROM(void)
{
	uint16_t i;
	uint16_t adrs;
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // �������ݗp �X���[�u�A�h���X
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // �ǂݏo���p�@�X���[�u�A�h���X 
	
	
	for ( i = 0 ; i < 832 ; i++ ) {    // EEPROM 0x2400-0x273F�@�̓ǂݏo�� 832 word
	
	  adrs = 0x2400 + i;	
		
	  iic_sd_data[1] = (adrs >> 8 );    //  �ǂݏo���A�h���X�̏�ʃo�C�g
	  iic_sd_data[2] = adrs & 0xff;	//  �ǂݏo���A�h���X�̉��ʃo�C�g
	
	  iic_sd_rcv_fg = 1;		// �}�X�^����M����
	
	  riic_sd_start();			// SCI IIC ���M�J�n
	
	  while( iic_com_over_fg != 1 ) {     // �ʐM�����҂�(��M�����҂�)

	  }
	
	  mlx_eeprom[i] = (( iic_rcv_data[0] << 8 ) | iic_rcv_data[1] );    // �ǂݏo���f�[�^�̊i�[
	
	}
}

