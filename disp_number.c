#include "iodefine.h"
#include  "misratypes.h"

#include "mlx90640_iic.h"
#include "ILI9488_9bit_dma.h"
#include "font_16_32_mlx.h"
#include "font_48_24_mlx.h"
#include "disp_number.h"
#include "thermocouple.h"

//
// �����̕\���e�X�g
//
// ����: index : �\�������������C���f�b�N�X (0�`21) 
//
//  index  �\������
//    0       0
//    1       1 
//    2       2 
//    3       3
//    4       4
//    5       5
//    6       6
//    7       7
//    8       8
//    9       9
//   10       0.  (0�̃s���I�h�t��)
//   11       1.
//   12       2.
//   13       3.
//   14       4.
//   15       5.
//   16       6.
//   17       7.
//   18       8.
//   19       9.
//   20      blank
//   21       -
//
// 
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
//             �B �B�@       �B �B�@       
//�@�@�@�@�@�@VCC GND          T_IRQ
//
//

void disp_num_test(uint8_t index )
{
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	
	st_col = 16;		// �J�n�J����
	st_row = 260;		// �J�n�s
	
				// 16(W)x32(H)
	end_col = st_col + 15;	// �I���J����
	end_row = st_row + 31;	// �I���s
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	spi_data_send_id(index);  	// �f�[�^���M�@�����_��1�ʂ̐���	
	
}

//  MLX90640�� Em(���˗�)�Ǝ��͉��x(Ta) �y��
//  �M�d�΂̊e�`�����l��(CH1�`CH4)�̉��x�Ɗ�ړ_���x�̕\��
//
//  

void disp_em_temp(void)	
{

	disp_float_data_em( mlx_emissivity, 64, 316);  // Em ���˗��̕\��
	
	disp_float_data( mlx_ta , 64, 358);         // Ta MLX90640�̎��͉��x�̕\��
	disp_float_data( mlx_tr , 64, 400);         // reflected temperature�̕\�� 
	
	disp_float_data( cj_temp , 64, 442);         //�@ ��ړ_���x�̕\�� 

	disp_float_data( tc_temp[0],256,316);  	     // CH1 ���x�\��
	disp_float_data( tc_temp[1],256,358);  	     // CH2 ���x�\��
	disp_float_data( tc_temp[2],256,400);  	     // CH3 ���x�\��
	disp_float_data( tc_temp[3],256,442);  	     // CH4 ���x�\��

}



//	
// ��,Em,Ta,CH1..���̃��x���\��
//
void disp_temp_label(void)
{
	disp_celsius();      // ���̕\��
	
	disp_s48_24_font( INDEX_FONT_EM, 16, 316); // ���x��( Em: )�̕\��
	disp_s48_24_font( INDEX_FONT_TA, 16, 358); // ���x��( Ta: )�̕\��
	disp_s48_24_font( INDEX_FONT_TR, 16, 400); // ���x��( Tr: )�̕\��
	disp_s48_24_font( INDEX_FONT_CJT, 16, 442); // ���x��( CJT: )�̕\��
	
	disp_s48_24_font( INDEX_FONT_CH1,208, 316); // ���x��( CH1: )�̕\�� 
	disp_s48_24_font( INDEX_FONT_CH2,208, 358); // ���x��( CH2: )�̕\�� 
	disp_s48_24_font( INDEX_FONT_CH3,208, 400); // ���x��( CH3: )�̕\�� 
	disp_s48_24_font( INDEX_FONT_CH4,208, 442); // ���x��( CH4: )�̕\�� 
}


// ���̕\�� (48x96)
 void disp_celsius(void)
{

	lcd_adrs_set(184, 260, 199, 291 );	 // �������ݔ͈͎w�� 
	
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	spi_data_send_id(INDEX_FONT_CELSIUS);  	// �f�[�^���M�@�����_��1�ʂ̐���	
	
}



//  �����̕\��  (�t�H���g�T�C�Y: 48(W)x24(H) �p)
//
// ����:  dt_index      : �\���������index
//        st_col        : �������ݐ擪 �J���� (x)
//        st_row        : �������ݐ擪 �y�[�W (y)
//
//  �t�H���g�f�[�^�̃T�C�Y(byte):  144 = 48/8 * 24
//  ���M�f�[�^�� :�@3456 = 144 x 8 x 3      (1pixel�̕\����3byte�v) 
//


void disp_s48_24_font( uint8_t dt_index,  uint32_t st_col, uint32_t st_row)
{
	uint8_t *pt;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t len;
	uint32_t num;
	
	 
	pt = (uint8_t *)&font_48w_24h_seg[dt_index][0];  // �t�H���g�f�[�^�̊i�[�A�h���X
	
	len = 144;				// �t�H���g�f�[�^�̃o�C�g��
	num = 3456;				// ���M�f�[�^��   RGB666
 	
	unpack_font_data_rgb666 ( len, pt );    // �t�H���g�f�[�^�� rgb666_data_bur�֓W�J 
	
	
	end_col = st_col + 47;	     		 // �����̏������ݏI���J����
	end_row = st_row + 23;       		 // �@�@�@�@�@�@:    �@�y�[�W
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w��
	
	spi_cmd_2C_send();	  		// Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		

	rspi_data_send( num, (uint16_t *)&rgb666_data_buf[0] );   //  �f�[�^���M
	
}





//
//  float�^�f�[�^��4���̕����Ƃ��ĕ\������B
//  16Hx32W�t�H���g �����̐F�͔��A�w�i�F�͍�        
//  ����: 
//        t : �\������ float �^�f�[�^
//    start_col: �\������ŏ��̕����̃J�����ʒu  (x)      
//    start_page:�\������ŏ��̕����̃y�[�W�ʒu  (y)
//
// �o��: 
//       �\���͈� -99.9 �` 999.9�@  , 4���ځi���l�܂��̓}�C�i�X),3����,2����+�����_, 1����
//�@   
//
//   
// ��1:  ���x 0.9��
//   4����  3����  2����  1����
//    �󔒁@�@��    0.    9 
//
// ��2:  ���x 12.3�� 
//  4����  3����  2����  1����
//�@  ��  �@1     2.    3 
//
// ��3:  ���x 213.4�� 
// 4����  3����  2����  1����
//    2�@  �@1     3.      4 
//
// ��4:  ���x -5.2��
//  4����  3����  2����  1����
//    - �@�@��    5.    2 
//
// ��5:  ���x -12.3�� 
// 4����  3����  2����  1����
//  -�@   �@1     2.    3 
//

void disp_float_data(float t, uint32_t start_column, uint32_t start_row )
{
	uint8_t dig1;    // 1���� �����_��1��
	uint8_t dig2;    // 2���� 1�̈ʁ@
	uint8_t dig3;    // 3���� 10�̈�
	uint8_t dig4;    // 4���� 100�̈�
	
	uint8_t para[1];
	
	int16_t  temp;
	
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t col_offset;
	
	temp = t * 10.0;	// ���x��10�{����

	
	st_col = start_column;
	st_row = start_row;

	         	// �t�H���g�T�C�Y 16(W)x32(H)
	col_offset = 15;
	end_col = st_col + col_offset;
        end_row = st_row + 31;				
	
	
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
		
	if ( temp >= 0 ) {            // ���x�����̏ꍇ  0�`999.9 �\��					   
		dig4 = temp / 1000;
		
		if ( dig4 == 0 ) {
			 spi_data_send_id(INDEX_FONT_BLANK);  // �f�[�^���M " "(��)
		}
		
		else{
			spi_data_send_id(dig4);  // �f�[�^���M 100�̈ʂ̐���
		}
	}
	else {				// ���x�����̏ꍇ -99.9�` - 0.1
		spi_data_send_id(INDEX_FONT_MINUS);  // �f�[�^���M "-"(�}�C�i�X����)
		
		temp = - temp;		// ���̒l�Ƃ��ĕ\������	
		
		dig4 = 0;
	}
	
	
					// 10�̈� �\��
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;		
	
						// 10�̈� �\��
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	dig3  = ( temp - (dig4 * 1000)) / 100;
	
	if (( dig4 == 0 ) && ( dig3  == 0 )) { // 100�̈ʂ��󔒂ŁA10�̈ʂ��󔒂̏ꍇ (��:�@5.1) 
	  	spi_data_send_id(INDEX_FONT_BLANK);  // �f�[�^���M " "(��)	
	}
	else{
	       spi_data_send_id(dig3);  // �f�[�^���M 10�̈ʂ̐���
	}
	

	
					// 1�̈ʂ̕\��(�����_�t��)
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
					   
	dig2 = ( temp - (dig4 * 1000) - (dig3 * 100) ) / 10;
	
	spi_data_send_id( dig2 + 10 );		  // �f�[�^���M 1�̈ʂ̐���(�����_�t��) (�����_�t���̃f�[�^�́A���� + 10 )

	
							// �����_��1�ʂ̕\��
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;						
					
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	dig1 = temp - (dig4 * 1000) - (dig3 * 100) - (dig2 * 10);
	
	spi_data_send_id(dig1);  	// �f�[�^���M�@�����_��1�ʂ̐���	
	
	
}



//
//  ���˗�(Em)�@��p
//  float�^�f�[�^��4���̕����Ƃ��ĕ\������B
//  16Hx32W�t�H���g �����̐F�͔��A�w�i�F�͍�        
//  ����: 
//        t : �\������ float �^�f�[�^
//    start_col: �\������ŏ��̕����̃J�����ʒu  (x)      
//    start_page:�\������ŏ��̕����̃y�[�W�ʒu  (y)
//
// �o��: 
//       �\���͈� 0.01 �` 1.00�@  , 4����,3����+�����_,2����, 1����
//�@   
//
//   
// ��1:  Em = 0.95
//   4����  3����  2����  1����
//    ��    0.    9      5
//
//

void disp_float_data_em(float t, uint32_t start_column, uint32_t start_row )
{
	uint8_t dig1;    // 1���� �����_��2��
	uint8_t dig2;    // 2���� �����_��1��
	uint8_t dig3;    // 3���� 1�̈�
	uint8_t dig4;    // 4���� 10�̈�
	
	uint8_t para[1];
	
	int16_t  temp;
	
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t col_offset;
	
	temp = t * 100.0;	// 100�{����

	
	st_col = start_column;
	st_row = start_row;

	         	// �t�H���g�T�C�Y 16(W)x32(H)
	col_offset = 15;
	end_col = st_col + col_offset;
        end_row = st_row + 31;				
	
	
							// 4���� �󔒕\��										
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	spi_data_send_id(INDEX_FONT_BLANK);  // �f�[�^���M " "(��)	
	
	
	
					// 3���ڂ� �\��
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;		
	
						
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	dig3  =  temp  / 100;
	spi_data_send_id(dig3+10);  // �f�[�^���M 1�̈ʂ̐���(�����_�t��) (�����_�t���̃f�[�^�́A���� + 10 )
	
	
					// 2���ڂ� �\��
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
					   
	dig2 = ( temp - (dig3 * 100) ) / 10;
	
	spi_data_send_id( dig2 );		  // �f�[�^���M �����_���ʂ̐���

	
					// 1���ڂ̕\��
	st_col = end_col + 1;		// �O�̕����̍ŏI�J���� + 1					
	end_col = st_col + col_offset;						
					
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // �������ݔ͈͎w�� 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	dig1 = temp - (dig3 * 100) - (dig2 * 10);
	
	spi_data_send_id(dig1);  	// �f�[�^���M�@�����_��2�ʂ̐���	
	
}


//  �t�H���g 16Wx32H pixel �̃f�[�^���M
//  
//  ����: index:  ���M����f�[�^
//
//  16(W)x32(H) �t�H���g�f�[�^�̃T�C�Y(byte):  64 = 16/8 * 32
//  ���M�f�[�^�� (byte):�@1536 = 64 x 8 x 3    (1pixel�̕\����3byte�v) 
//    
//
// �Eindex�ƕ\�������̊֌W 
//  index  �\������
//    0       0
//    1       1 
//    2       2 
//    3       3
//    4       4
//    5       5
//    6       6
//    7       7
//    8       8
//    9       9
//   10       0.  (0�̃s���I�h�t��)
//   11       1.
//   12       2.
//   13       3.
//   14       4.
//   15       5.
//   16       6.
//   17       7.
//   18       8.
//   19       9.
//   20      blank
//   21       -
//


void spi_data_send_id(uint8_t index ) {
	
	uint8_t *pt;
 	
	uint32_t len;
	uint32_t num;


					      // �t�H���g�T�C�Y 16(W)x32(H)						
	pt = (uint8_t *)&font_16w_32h_seg[index][0];  // �t�H���g�f�[�^�̊i�[�A�h���X
	
	len = 64;				// �t�H���g�f�[�^�̃o�C�g��
	num = 1536;				// ���M�f�[�^��   RGB666
 	
	unpack_font_data_rgb666 ( len, pt ); // �t�H���g�f�[�^�� rgb666_data_bur�֓W�J 
	
	rspi_data_send( num, (uint16_t *)&rgb666_data_buf[0] );   //  �f�[�^���M
	

}


// �r�b�g�}�b�v�̃t�H���g�f�[�^(1byte�P��)���ARGB666�ɕϊ����āA
// �\���p�̃o�b�t�@�ɃZ�b�g
// �����F�͔��A�w�i�F�͍��@
//  ����: * ptt: �t�H���g�f�[�^�̐擪�A�h���X
//         len:  �t�H���g�f�[�^�̃o�C�g��
//               (16Wx32H �Ȃ��  64byte)    64x8=512bit
//               (32Wx64H �Ȃ�΁@256byte)�@256x8=2048bit
// 
//    rgb666_data_buf[6144];  2048x3 byte (2048pixel��)( 6.4�s��)
//
void unpack_font_data_rgb666 ( uint32_t len, uint8_t * src_ptr )
{
	
	uint8_t  bd;
	uint8_t  mask;
	uint8_t  cd;
	
	uint32_t i;
	uint32_t j;
	uint32_t pt;
	
	pt = 0;			// �i�[�ʒu�̃N���A
	
	for ( i = 0; i < len  ; i++ ){
		
	  bd = *src_ptr;	// �r�b�g�}�b�v�f�[�^���o��
	
	  mask = 0x80;          // �}�X�N�f�[�^�̃Z�b�g
	  
	  for ( j = 0 ; j < 8 ; j++ ) {   // b8���� �r�b�gOn/Off�̔���	
	    if ( ( bd & mask ) != 0x00 ) {  // �r�b�gOn�̏ꍇ
		
		rgb666_data_buf[pt] = 0x01fc;  //   R: 1111 1100
		pt = pt + 1;		     // �i�[�ʒu�̃C���N�������g
		
		rgb666_data_buf[pt] = 0x01fc;    // G: 1111 1100
		pt = pt + 1;
		
		rgb666_data_buf[pt] = 0x01fc;    // B: 1111 1100
		pt = pt + 1;
	     }
	     else {				// �r�b�gOff�̏ꍇ
	     
		rgb666_data_buf[pt] = 0x0100;  //   R: 0000 0000
		pt = pt + 1;		     // �i�[�ʒu�̃C���N�������g
		
		rgb666_data_buf[pt] = 0x0100;    // G: 0000 0000
		pt = pt + 1;
		
		rgb666_data_buf[pt] = 0x0100;    // B: 0000 0000
		pt = pt + 1;
	     
	     }
	
	     mask = mask >> 1;		// �}�X�N�f�[�^���A�E 1�r�b�g�V�t�g
	
	  }
	  
	  src_ptr++;			// ���o���ʒu�C���N�������g
		
	}

}

