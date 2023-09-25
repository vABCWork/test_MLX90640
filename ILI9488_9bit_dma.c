#include "iodefine.h"
#include  "misratypes.h"
#include "rspi_9bit.h"
#include  "delay.h"
#include "ILI9488_9bit_dma.h"


uint16_t rspi_snd_buf[8];	// ILI9588�֑��M����R�}���h�A�p�����[�^���i�[

uint16_t rgb666_data_buf[3456];  // �\���p�o�b�t�@ (1152x3)  
				// 1�������̕\���p�f�[�^		
				// �t�H���g24Wx48H �Ȃ�� 144 byte= 144x8 =1152bit,  1bit�̕\����3�o�C�g�K�v(RGB666)	
 				//  DMA�]����RSPI��9bit���M���邽�߁Aword �f�[�^�Ƃ��Ă���B

// ILI9488�̏�����
//
// 1) RGB 6-6-6 �Ŏg�p :  Interface Pixel Format (3Ah)
// 2) �c�^, �������֏�������: Memory Access Control (36h) 
//
// (ILI)488 Datasheet 6.3. MCU to Memory Write/Read Direction )
//
// 
// Memory Access Control (36h)
// �p�����[�^ (MADCTL)
//  b7 : MY (Row address order)    0=Top to bottomm, 1= Bottom to top	
//  b6 : MX (Column address order) 0=Left to right,  1= Right to Left
//  b5 : MV (Row/Column exchange)
//  b4 : Vertical Refresh Order
//  b3 : RGB-BGR Order 0=RGB color filter panel, 1=BGR color filter panel
//  b2 : Horizontal Refresh ORDER
//  b1 : Reserved
//  b0 : Reserved
//	

void ILI9488_Init(void)
{
	
	rspi_snd_buf[0] = 0x003A;	      //  Interface Pixel Format (3Ah)
	rspi_snd_buf[1] = 0x0106;	      // 262K�F 18 bits/pixel RGB666 
	rspi_data_send(2, (uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M 
	
	
	rspi_snd_buf[0] = 0x0036;	      // Memory Access Control (36h)
	
					      // LCD����̃P�[�u��������������o���ꍇ	
	rspi_snd_buf[1] = 0x0188;	      // �R�}���h���W�X�^ 0x36�p�p�����[�^ (MY=1,MX=0,MV=0) 
	
					      // LCD����̃P�[�u�����ォ������o���ꍇ	
	//rspi_snd_buf[1] = 0x0148;	      // �R�}���h���W�X�^ 0x36�p�p�����[�^ (MY=0,MX=1,MV=0)
	
	rspi_data_send(2, (uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	
	
	rspi_snd_buf[0] = 0x0011;             // Sleep OUT (11h),�p�����[�^��=0		
	rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	
	 
	rspi_snd_buf[0] = 0x0029;             // Display ON (29h), �p�����[�^��=0		
	rspi_data_send(1,(uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	 
	
	delay_msec(5);	    	    	      //  5msec�҂�
}



// LCD �̃��Z�b�g
// 
void ILI9488_Reset(void)
{

	LCD_RESET_PODR = 0;              // LCD ���Z�b�g��Ԃɂ���
	delay_msec(1);		        // 1[msec] �҂� 
	
	LCD_RESET_PODR = 1;             // LCD �񃊃Z�b�g���
	delay_msec(2);	        	// 2[msec] �҂� 
}


//  �\���͈͂̐ݒ�
// ����:
//  col: �J�n�J����(x), page(row):�J�n�y�[�W(y)
//  col2:�I���J����, page2(row2): �I���y�[�W
//
void lcd_adrs_set( uint16_t col, uint16_t page, uint16_t col2, uint16_t page2)
{
 	 rspi_snd_buf[0] = 0x002a;  			       // Column Address Set �R�}���h���W�X�^ 0x2a , �p�����[�^��=4
	 rspi_snd_buf[1] = 0x0100 | ((0xff00 & col) >> 8);     //  SC[15:8]�@�X�^�[�g�J����(16bit)�̏�ʃo�C
	 rspi_snd_buf[2] = 0x0100 | (0x00ff & col);            //  SC[7:0]         :�@�@�@�@�@�@�@�@���ʃo�C�g 
	 rspi_snd_buf[3] = 0x0100 | ((0xff00 & col2) >> 8);    //  EC[15:8]�@�I���J����(16bit)�̏�ʃo�C�g
	 rspi_snd_buf[4] = 0x0100 | (0x00ff & col2);           //  EC[7:0]         :�@�@�@�@�@�@�@�@���ʃo�C�g 
	 
	 rspi_data_send(5,(uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	
	 rspi_snd_buf[0] = 0x002b;				       //  Page Address Set �R�}���h���W�X�^ 0x2b , �p�����[�^��=4
	 rspi_snd_buf[1] = 0x0100 | ((0xff00 & page ) >> 8);    //  SP[15:8]�@�X�^�[�g�y�[�W(16bit)�̏�ʃo�C�g
	 rspi_snd_buf[2] = 0x0100 | (0x00ff & page);            //  SP[7:0]         :�@�@�@�@�@�@�@�@���ʃo�C�g 
	 rspi_snd_buf[3] = 0x0100 | ((0xff00 & page2 ) >> 8);    // EP[15:8]�@�I���y�[�W(16bit)�̏�ʃo�C�g
	 rspi_snd_buf[4] = 0x0100 | (0x00ff & page2);            // EP[7:0]         :�@�@�@�@�@�@�@�@���ʃo�C�g 
	 
	rspi_data_send(5,(uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
		
}


//
//  Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������
// 
void spi_cmd_2C_send( void )
{
	 rspi_snd_buf[0] = 0x002c;		       // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������
	
	 rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	
}


//
// �@�s�N�Z���f�[�^�̏������݃e�X�g (RGB 6-6-6�p)
//    Memory Access Control (36h) �̐ݒ�ɂ��A�f�[�^�̕\���ʒu���ς�鎖���m�F
//   
//   ILI9488 data sheet:
//   4.7.1.2. SPI Data for 18-bit/pixel (RGB 6-6-6 Bits Input), 262K-color
//
// �F          R         G         B  
// ��       11111100  11111100  11111100
// ���F     1iiiii00  11111100  00000000
// �V�A��   00000000  11111100  11111100
// ��       00000000  11111100  00000000
// �}�[���^ 11111100  00000000  11111100
// ��       11111100  00000000  00000000
// ��       00000000  00000000  11111100
// ��       00000000  00000000  00000000
//

void pixel_write_test_rgb666()
{
	uint16_t pix_num;
	uint16_t wr_num;
	
	pix_num = 1;		// �������݃s�N�Z����
	wr_num = pix_num * 3;	// ���M�o�C�g�� ( 3 byte�� 1�s�N�Z�����̏��@)
	
	rgb666_data_buf[0] = 0x01fc;   // �������݃f�[�^�̃Z�b�g (���F)
	rgb666_data_buf[1] = 0x01fc;
	rgb666_data_buf[2] = 0x0100;
	
	lcd_adrs_set(0,0, pix_num, 0);	  // Column Address Set(2Ah), Page Address Set(2Bh) (�J�n�J����=0, �J�n�y�[�W=0, �I���J����=pix_num, �I���y�[�W=0)
	
	 
	 spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	
	 rspi_data_send(wr_num,(uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M

	
}



//  ILI9488  LCD �J���[�o�[(8�F) (RGB 6-6-6)
//   (320x480)
//
// �F          R         G         B  
// ��       11111100  11111100  11111100
// ���F     1iiiii00  11111100  00000000
// �V�A��   00000000  11111100  11111100
// ��       00000000  11111100  00000000
// �}�[���^ 11111100  00000000  11111100
// ��       11111100  00000000  00000000
// ��       00000000  00000000  11111100
// ��       00000000  00000000  00000000
//
//                  Clolumn
//            0                 319
//          0 +------------------+
// Page(Row)  |                  |
//            |                  |
//            |                  |
//            |                  |
//            |                  |
//            |                  |
//            |                  |    
//            |                  |
//            |                  |
//            |                  |
//            |                  |
//       479  |                  |
//            +------------------+
//                          Z400IT002
//

void color_bar_rgb666(void)
{
	uint32_t i;
		 
         for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,0, 319,59);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=0, �I���J����=319, �I���y�[�W=59)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (���F) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,60, 319,119);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=60, �I���J����=319, �I���y�[�W=119)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (�V�A��) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,120, 319,179);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=120, �I���J����=319, �I���y�[�W=179)
	rgb666_data_send();
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,180, 319,239);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=180, �I���J����=319, �I���y�[�W=239)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (�}�[���^) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,240, 319,299);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=240, �I���J����=319, �I���y�[�W=299)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,300, 319,359);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=300, �I���J����=319, �I���y�[�W=359)
	rgb666_data_send();
	 
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,360, 319,419);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=360, �I���J����=319, �I���y�[�W=419)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��)
         {
		rgb666_data_buf[i*3] = 0x0100;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,420, 319,479);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=360, �I���J����=319, �I���y�[�W=419)
	rgb666_data_send();
	
}



//�@�J���[�o�[�p�@�f�[�^���M (RGB 6-6-6�p)
//  60�s���\��
void rgb666_data_send(void)
{
	uint32_t i;
	uint32_t num;
	
	 num = 960;		 // ���M�o�C�g��

	 spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 rspi_data_send(num, (uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M
	 
	 
	 for ( i = 0; i < 59; i++ ) {
	   rspi_snd_buf[0] = 0x003c;		 // Memory Write Continue (3Ch)
	   rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // �R�}���h���M
	   
	   rspi_data_send(num,(uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M
	 
	 }
	 
	
}



//
// ��ʂ����ɂ���
//
void disp_black_rgb666(void)
{
	uint32_t i;
	
	uint32_t  j;
	
	 for ( i = 0; i < 320 ; i++)	// �s�N�Z���f�[�^�𗬂����� (1 �s��) (��)
         {
		rgb666_data_buf[i*3] = 0x0100;   // �������݃f�[�^�̃Z�b�g 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	
	for ( j = 0 ; j < 8 ; j++ ) {
		
	  lcd_adrs_set(0,j*60, 319, j*60+59);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W= J*60, �I���J����=319, �I���y�[�W= J*60+59)
        
	  rgb666_data_send();			  // 60�s���̕\��
	}
	
	
}


	


