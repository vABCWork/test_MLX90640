#include	<machine.h>
#include	 "iodefine.h"
#include	 "misratypes.h"
#include	"delay.h"
#include 	"dma.h"
#include	"sci.h"
#include	"timer.h"
#include	"riic_mlx.h"

#include	"mlx90640_iic.h"
#include	"mlx90640_api.h"

#include	 "rspi_9bit.h"
#include	 "ILI9488_9bit_dma.h"
#include	"dsad.h"



void clear_module_stop(void);

void main(void)
{
	uint32_t i;
	
	clear_module_stop();	//  ���W���[���X�g�b�v�̉���
	
	DMA0_ini();		//  PC�ւ̃V���A���f�[�^��M�p��DMA�����@�����ݒ�
	
	DMA0_SCI_RCV_SET();	// ���̓d����M�̂��ߍĐݒ�
	
	DMA1_ini();           	// PC�ւ̃V���A���f�[�^���M�p��DMA�����@�����ݒ�	
	
	initSCI_1();		// SCI1(�p�\�R���Ƃ̒ʐM�p) 307.2Kbps  
	Init_CRC();		//  CRC���Z��̐ݒ� 16�r�b�gCRC�iX^16 + X^12 + X^5 + 1 �j
	LED_comm_port_set();	//  ���M���Ǝ�M����LED�o�̓|�[�g�ݒ�
	
	DMA2_ini();           	// DMA �`�����l��2( LCD�ւ̃f�[�^���M�p)�@�����ݒ�
	RSPI_Init_Port();	// RSPI �|�[�g�̏�����  (LCD�R���g���[���p)   
     	RSPI_Init_Reg();        // SPI ���W�X�^�̐ݒ�  

     	RSPI_SPCMD_0();	        // SPI �]���t�H�[�}�b�g��ݒ�, SSLA0�g�p	
	
	ILI9488_Reset();	// LCD �̃��Z�b�g	
	 
	ILI9488_Init();		// LCD�̏�����
	
	delay_msec(10);		// LCD(ILI9488)�����������҂�
	
//	while(1) {
//	 //   pixel_write_test_rgb666();
	    
//	    color_bar_rgb666();		// 364 msec
	    
//	    delay_msec(10);		// LCD(ILI9488)�����������҂�
//	}
	
	
	disp_black_rgb666();		// LCD��� �S�̂���
	color_map_turbo();		// �J���[�}�b�v(turbo)�̕\�� 256Wx10H  
	
	
	RIIC0_Port_Set();	//  I2C(SMBus)�C���^�[�t�F�C�X�p�̃|�[�g�ݒ�	
	RIIC0_Init();		//  I2C(SMBus)�C���^�[�t�F�C�X �̏�����
					
	RIIC0_Init_interrupt();	// RIIC0 ���荞�݋��� 
	
	afe_ini();		// AFE(�A�i���O�t�����g�G���h)�ݒ�
	
	dsad0_ini();		// DASD0�̐ݒ�@(�M�d�Ηp 4�`�����l��)
	dsad1_ini();            // DASD1�̐ݒ� (��ړ_�⏞ RTD 100 ohm)
	
	ad_index = 0;		// �e�`�����l���̃f�[�^�i�[�ʒu�̏�����
	ad1_index = 0;
	
	Timer10msec_Set();      // �^�C�}(10msec)�쐬(CMT0)
     	Timer10msec_Start();    // �^�C�}(10msec)�J�n�@( DSAD0,DSAD1 �I�[�g�X�L�����J�n)
	
	iic_slave_adrs = 0x33;    	//  �X���[�u�A�h���X = 0x33 (MLX 90640)
	
	Read_MLX_EEPROM();	// MLX90640  EEPRM(0x2400-0x273F)����f�[�^��ǂݏo��  mlx_eeprom[832]�֊i�[ �@
	
	
	mlx_broke_pixel = MLX90640_ExtractParameters(&mlx_eeprom[0], &mlx_para);	// mlx_para�֓W�J
	
	mlx_emissivity = 0.95;		// ���˗� = 0.95 �Œ�
	
	MLX_Set_Refresh_rate();		// MLX90640 ���t���b�V�����[�g 4[MHz]�ݒ�
	
	
	disp_temp_label();		// ��,Em,Ta,CH1..���̃��x���\��
	
	while(1) {   //  �S�̂̏������� 537 msec
		
		 IWDT_Refresh();		// �E�I�b�`�h�b�N�^�C�}���t���b�V��
	
		 MLX_Get_FrameData();	// RAM(0x0400-0x073F)�̃f�[�^��(Control register1)�� (statusRegister & 0x0001)�̓ǂݏo��(146 [msec], clock=400[kHz])
	     
	         mlx_frame_data_err = ValidateFrameData(&mlx_ram[0]);  // MLX90640 RAM 0x0400����̃f�[�^���`�F�b�N
	     
	         mlx_aux_data_err =  ValidateAuxData(&mlx_ram[768]);   // MLX90640 RAM 0x0700(Ta_Vbe)����̃f�[�^���`�F�b�N
	     
		 mlx_tr = MLX90640_GetTa(&mlx_ram[0], &mlx_para);      // reflected temperature: �����Ŏg�p����ꍇ�́A�Z���T���͉��x(Ta)���g�p
		 
		 //mlx_tr = MLX90640_GetTa(&mlx_ram[0], &mlx_para) - 8.0;   // ���O�Ŏg�p����ꍇ�́A�Z���T���͉��x(Ta) - 8.0
		 						       // MLX90640 32x24 IR array driver (Rev.1 - October 31,2022) (page 15)
								       // mlx_tr : reflected temperature defined by the user. 
								       //  If the object emissivity is less than 1, there might be some temperature reflected from the object. 
								       // In order for this to be compensated the user should input this reflected temperature. 
								       // The sensor ambient temperature could be used, but some shift depending on the enclosure might be needed. 
								       // For a MLX90640 in the open air the shift is -8��C.
	 	 
	     
	         MLX90640_CalculateTo(&mlx_ram[0], &mlx_para, mlx_emissivity, mlx_tr, &mlx_to[0]);  // ���x�̌v�Z 4.4[msec]
		 
		 
		 mlx_to_min_max();			// ���艷�x�̍ő�l�ƍŏ��l�𓾂� 0.4 msec
		 
		 mlx_to_min_max_disp();			// �ő�l�ƍŏ��l�̕\�� 11 msec
		 mlx_to_center_disp();		        // �����̒l��\�� 5.5 msec
		
		 color_map_mlx_to_interpolate();	// ���艷�x�̃J���[�}�b�v�\�� (320Wx240H) (�o���`���)	326[msec]
	       
		// color_map_mlx_to();			// ���艷�x�̃J���[�}�b�v�\�� (320Wx240H) (��ԂȂ�) 211 [msec] 
		
		//color_map_mlx_to_1px();		// ���艷�x�̃J���[�}�b�v��\�� (32Wx24H)
		
		Cal_ad_avg();		   // dsad0 �e�`�����l���̕��ϒl�𓾂�
		Cal_ad1_avg();		   // dsad1 �e�`�����l���̕��ϒl�𓾂�
		tc_temp_cal();		   // ���x�v�Z

			
		disp_em_temp();		   // Em(���˗�)�Ǝ��͉��x(Ta) �y�� �M�d�΂̊e�`�����l���̉��x�\�� 44[msec]
		 
		 if ( rcv_over == 1 ) {		// �ʐM���� �R�}���h��M�̏ꍇ
		    LED_RX_PODR = 0;		// ��M LED�̏���  
  		    comm_cmd();			// ���X�|���X�쐬�A���M
	   	    rcv_over = 0;		// �R�}���h��M�t���O�̃N���A
		    rcv_cnt  = 0;		//  ��M�o�C�g���̏���
		    Init_CRC();			// CRC���Z��̏�����
		 }
		 
	     
		// delay_msec(1);		// 1msec�҂� 
	}  // while
	

}




// ���W���[���X�g�b�v�̉���
//
//�@ �R���y�A�}�b�`�^�C�}(CMT) ���j�b�g0(CMT0, CMT1) 
//   �A�i���O�t�����g�G���h(AFE)
//   24�r�b�g��-�� A/D �R���o�[�^(DSAD0) ���j�b�g0
//   24�r�b�g��-�� A/D �R���o�[�^(DSAD1) ���j�b�g1
//   I2C �o�X�C���^�t�F�[�X(RIICa)
//   �V���A���y���t�F�����C���^�t�F�[�X0(RSPI)
//   DMA �R���g���[��(DMACA)
//   �V���A���R�~���j�P�[�V�����C���^�t�F�[�X1(SCI1)(�p�\�R���Ƃ̒ʐM�p)
//   CRC���Z��  (�p�\�R���Ƃ̒ʐM�f�[�^�m�F�p)
//

void clear_module_stop(void)
{
	SYSTEM.PRCR.WORD = 0xA50F;	// �N���b�N�����A����d�͒ጸ�@�\�֘A���W�X�^�̏������݋���	
	
	MSTP(CMT0) = 0;			// �R���y�A�}�b�`�^�C�}(CMT) ���j�b�g0(CMT0, CMT1) ���W���[���X�g�b�v�̉���
	
	MSTP(AFE) = 0;			// �A�i���O�t�����g�G���h(AFE) ���W���[���X�g�b�v�̉���
	MSTP(DSAD0) = 0;		// 24 �r�b�g��-�� A/D �R���o�[�^(DSAD0) ���j�b�g0 ���W���[���X�g�b�v�̉���
	MSTP(DSAD1) = 0;		//             :                        ���j�b�g1 
	
	MSTP(RIIC0) = 0;                //  RIIC0���W���[���X�g�b�v���� (I2C�ʐM)
	
	MSTP(RSPI0) = 0;		// �V���A���y���t�F�����C���^�t�F�[�X0 ���W���[���X�g�b�v�̉���
	MSTP(DMAC) = 0;                //  DMA ���W���[���X�g�b�v����
	
	MSTP(SCI1) = 0;	        	// SCI1 ���W���[���X�g�b�v�̉���
	MSTP(CRC) = 0;			// CRC ���W���[���X�g�b�v�̉���	
	
	
	SYSTEM.PRCR.WORD = 0xA500;	// �N���b�N�����A����d�͒ጸ�@�\�֘A���W�X�^�������݋֎~
}

