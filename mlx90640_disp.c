#include "iodefine.h"
#include  "misratypes.h"

#include "ILI9488_9bit_dma.h"

#include "mlx90640_iic.h"
#include "mlx90640_disp.h"
#include "colormap_turbo.h"
#include "disp_number.h"

float mlx_to_interpolate[100];  // �o���`��Ԃ������x( 1pixel�� ) ( MLX 1pixel �� LCD 10x10 dot ) 

float mlx_to_max;		// �Ώە��̉��x �ő�l
float mlx_to_min;		// �Ώە��̉��x �ŏ��l
float mlx_to_center;            //  (�ő�l - �ŏ��l)/2


// �o���`��Ԃ̌v�Z
// �@�Q�l: https://x-engineer.org/bilinear-interpolation/
//
//  �w�肵���s�N�Z���̉��x���A�o���`��Ԃ��āAmlx_to_interpolate[]�֊i�[����
// ����: col :  0 - 30    ( col�̍ő�=30 (Q2,Q4��col=31�̂���))
//       row :  0 - 22    ( row�̍ő�=22 (Q3,Q4��row=23�̂���))
//   
//  Q�̒l�́Ax��y�̒l�����߂��Ă��܂��B
//  4�_(a,b),(c,b),(a,d),(c,d)�ƁA����Q�l���������Ă���ꍇ�ɁA
//  P�_(x,y)�ł� �l��o���`��Ԃɂ�苁�߂܂��B
//
//  y(row)
//   |
// d-|-- Q3         Q4
//   |
//   |    �@�@P
// b-|---Q1         Q2
//   |   |
//   +---+-----------+------------- x(col)
//       a           c 
//  
// x,y�̒l�� Q�̑Ή� : (a,b) = Q1 , (c,b) = Q2 , (a,d) = Q3,  (c,d) = Q4
//
// �v�Z�菇:
//  1) x�������ɐ��`��Ԃ��s���AR1,R2�����߂܂��B
//    R1 = ((Q2 - Q1)/( c - a ))*( x - a ) + Q1 
//    R2 = ((Q4 - Q3)/( c - a ))*( x - a ) + Q3 
//  2)R1,R2��y�������ɐ��`��Ԃ��s�� P�_(x,y)�ł̒l�����߂܂��B
//    P(x,y) = ((R2 - R1)/( d - b))*( y - b ) + R1 
//
// �E�v���O�����ł́Aa=0,b=0, c=10,d=10 �Ƃ��Ă��܂��B
//
void mlx_bilinear_interpolate( uint32_t col, uint32_t row )
{
	uint32_t i;
	uint32_t j;
	
	float q1;
	float q2;
	float q3;
	float q4;
	
	float r1;
	float r2;
	float p;
	
	
	q1 = mlx_to[col + row*32];		// 4���̉��x�𓾂�
	q2 = mlx_to[col+1 + row*32];
	q3 = mlx_to[col + (row+1)*32];
	q4 = mlx_to[col+1 +(row+1)*32];
	
	for ( j = 0 ; j < 10 ; j++ ) {
	  for ( i = 0 ; i < 10 ; i++ ) {
	    r1 = (( q2 - q1 ) / 10.0) * i + q1;
	    r2 = (( q4 - q3 ) / 10.0) * i + q3;
	
	    p = (( r2 - r1 ) / 10.0 ) * j + r1;
	  
	    mlx_to_interpolate[i + 10*j] = p;	// �o���`��Ԃ����l�̊i�[
	  }
	}
	
}


// 
//   �Ώە��̉��x(�ő�l�ƍŏ��l)�𓾂�
//
void mlx_to_min_max(void)
{
	uint32_t i;
	float val;
	
	mlx_to_min = mlx_to[0];
	mlx_to_max = mlx_to[0];
	
	for ( i = 1 ; i < 768; i++ ) {
	   
	   val = mlx_to[i];
		
	   if ( val  < mlx_to_min )  {
	   	mlx_to_min = val;
	   }
	   
	   if ( val > mlx_to_max ) {
	   	mlx_to_max = val;
	   }
	}
	
	
	
}


// �J���[�}�b�v�̗��[�̉����ɁA���艷�x�̍ŏ��l�ƍő�l��\��
//
 void mlx_to_min_max_disp(void)
 {
	disp_float_data(mlx_to_min,0,260);	// �ŏ��l�\�� �t�H���g�T�C�Y 16Wx32H
	
	disp_float_data(mlx_to_max,256,260);  // �ŏ��l�\��
}

 //
// �����̒l��\��
// 
 void mlx_to_center_disp(void)
 {
	mlx_to_center = (( mlx_to_max - mlx_to_min ) / 2.0) + mlx_to_min;
	
	disp_float_data( mlx_to_center,120,260);	// �ŏ��l�\�� �t�H���g�T�C�Y 16Wx32H
	
}
 

// ���艷�x�ɑΉ������J���[�}�b�v��\�� ��Ԃ���(320x240)
//
// MLX90640 (32Wx24H):
//  �E���f�[�^ 32x24�̃f�[�^
//    0    �@1  �@2 ... 30 31
//  0 22�� 23��
//  1 32���@
//  :
// 23
//
//  �E��ԃf�[�^ 320x240�̃f�[�^
//	0    .1   .2  .3    .4   .5   .6   .7   .8   .9  1        2 ... 30 31
//  0 22�� 22.1 22.2 22.3 22.4 22.5 22.6 22.7 22.8 22.9  23��
// .1 23
// .2 24
// .3 25
// .4 26
// .5 27
// .6 28
// .7 29
// .8 30
// .9 31
// 1  32���@
//  :
// 23
//
//  
// LCD �\���͈�:
//    (0,0)                (319,0)
//        ------------------+
//        |                 |
//        |                 |
//        |                 |
//        |                 |
//        |                 |
//        +-----------------+
//    (0,239)              (319.239)
//
//
void color_map_mlx_to_interpolate(void)
{
	
	uint32_t  i;
	uint32_t  j;
	uint32_t  k;
	
	
	for ( j = 0 ; j < 24; j++ ) {
	  
	  for ( i = 0 ; i < 32; i++ ) {
	
            if( ( i < 31 ) && ( j < 23 ) ) { 		  
               mlx_bilinear_interpolate( i, j );  // �o���`��Ԃ����l���@mlx_to_interpolate[100]�֊i�[���� 100usec
	    }	  
	    else {                             // i = 31 �܂��� j = 23�̏ꍇ�A
	       for ( k = 0; k < 100 ; k++ ) {  // ��Ԗ����œ����l���Z�b�g 
		   mlx_to_interpolate[k]  = mlx_to[i + j*32];
	       }    
	    }
	    
            mlx_interpolate_to_rgb();	     //  �����`(10x10)�̃f�[�^�Z�b�g	73usec
	    
	    lcd_adrs_set(310-i*10,j*10,319-i*10,j*10+9);    // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=j, �I���J����=31, �I���y�[�W=j)
	    
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	    rspi_data_send(300,(uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M ���M�o�C�g�� (300 = 10*10*3 )
	    
	  }
 
	}
}


//
// �o���`��Ԃ����l ( mlx_to_interpolate[100] )����Argb�f�[�^�𓾂�
//
void  mlx_interpolate_to_rgb(void)
{
	uint8_t *rpt;
	uint8_t *gpt;
	uint8_t *bpt;
	
	uint8_t r_data;
	uint8_t g_data;
	uint8_t b_data;
	
	uint32_t  k;
	uint32_t  pt;
	
	float diff;
	float diff_255;
	
	diff = mlx_to_max - mlx_to_min;
	
	diff_255 = 255.0 / diff;
	
	for ( k = 0; k < 100; k++ ){
	
           pt = (uint32_t) (( mlx_to_interpolate[k] - mlx_to_min ) * diff_255 ); // �J���[�o�[ turbo_rgb[256][3] ���[=�Œቷ�x �E�[=�ō����x
	   
	   if ( pt > 255 ) {
	   	pt = 255;
	   }
	   
	   rpt = (uint8_t *)&turbo_rgb[pt][0];  // R �f�[�^�̊i�[�A�h���X
	   gpt = (uint8_t *)&turbo_rgb[pt][1];  // G �f�[�^�̊i�[�A�h���X
	   bpt = (uint8_t *)&turbo_rgb[pt][2];  // B �f�[�^�̊i�[�A�h���X
		
	   r_data = (*rpt >> 2) << 2;	//  6bit�f�[�^(���� 2bit �؂�̂�)��ɁA�������݃f�[�^�p�ɁA2�r�b�g���V�t�g
	   g_data = (*gpt >> 2) << 2;
	   b_data = (*bpt >> 2) << 2;
	
	   rgb666_data_buf[k*3] = (0x0100 | r_data);      // R�@�������݃f�[�^
	   rgb666_data_buf[k*3 + 1] = (0x0100 | g_data);  // G
	   rgb666_data_buf[k*3 + 2] = (0x0100 | b_data);  // B  
	
	}
}




// ���艷�x�ɑΉ������J���[�}�b�v��\�� (320x240) �i��ԂȂ�)(�e�X�g�p)
//
//  MLX��1�s�N�Z�����ALCD��10x10�ɑΉ�
//
//�ELCD�̕\���̈��MLX�̃s�N�Z��(col1�`col32,row1�`row24)�Ƃ̑Ή�
//
//    (0,0)                (319,0
//        ------------------+
//   row1 |col32        col1|
//        |                 |
//        |                 |
//        |                 |
//   row24|                 |
//        +-----------------+
//    (0,239)              (319.239)
//
// 
void color_map_mlx_to(void)
{
	uint8_t *rpt;
	uint8_t *gpt;
	uint8_t *bpt;
	
	uint8_t r_data;
	uint8_t g_data;
	uint8_t b_data;
	
	uint32_t  i;
	uint32_t  j;
	uint32_t  pt;
	
	float diff;
	float diff_255;
	
	diff = mlx_to_max - mlx_to_min;
	
	diff_255 = 255.0 / diff;
	
	for ( j = 0 ; j < 24; j++ ) {
	  
	  for ( i = 0 ; i < 32; i++ ) {
		  
	    pt = (uint32_t) ( ( mlx_to[i + j*32] - mlx_to_min ) * diff_255 ); // �J���[�o�[ turbo_rgb[256][3] ���[=�Œቷ�x �E�[=�ō����x
	    
	    if ( pt > 255 ) {
	   	pt = 255;
	    }
	    
	    rpt = (uint8_t *)&turbo_rgb[pt][0];  // R �f�[�^�̊i�[�A�h���X
	    gpt = (uint8_t *)&turbo_rgb[pt][1];  // G �f�[�^�̊i�[�A�h���X
	    bpt = (uint8_t *)&turbo_rgb[pt][2];  // B �f�[�^�̊i�[�A�h���X
		
	    r_data = (*rpt >> 2) << 2;	//  6bit�f�[�^(���� 2bit �؂�̂�)��ɁA�������݃f�[�^�p�ɁA2�r�b�g���V�t�g
	    g_data = (*gpt >> 2) << 2;
	    b_data = (*bpt >> 2) << 2;
		
	    rgb666_square_data_set( r_data, g_data, b_data);  // �����`(10x10)�̃f�[�^�Z�b�g
	    
	  
	    lcd_adrs_set(310-i*10,j*10,319-i*10,j*10+9);         // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=j, �I���J����=31, �I���y�[�W=j)
								//lcd_adrs_set(i*10,j*10,i*10+9,j*10+9);	// ����
	    
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	  
	    rspi_data_send(300, (uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M
	  
	  }
 
	}
}

//
// �l�p�`(10Wx10H) ���� rgb666�f�[�^���Z�b�g
//
void rgb666_square_data_set( uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t i;
	
	for ( i = 0 ; i < 100; i++ ){
	
	    rgb666_data_buf[i*3] =  (0x0100 | r);      // R�@�������݃f�[�^
	    rgb666_data_buf[i*3 + 1] = ( 0x0100 | g);  // G
	    rgb666_data_buf[i*3 + 2] = ( 0x0100 | b);  // B  
	}
	
}


// ���艷�x�ɑΉ������J���[�}�b�v��\�� (32Wx24H)(�e�X�g�p)
//
//�ELCD�̕\���̈��MLX�̃s�N�Z��(col1�`col32,row1�`row24)�Ƃ̑Ή�
//
//
// �\���͈�:
//   MLX90640 :   LCD
//  col1,row1 = (319,0)
//  col32.row1 = (288,0)
//  col1,row24 = (319,23)
//  col23,row24 = (288,23)
//
//
//                  (288,0) (319,0)
//        ---------------------+
//   row1 |         col32  col1|
//        |                    |
//  row24 |                    | 
//                 
//    
//
void color_map_mlx_to_1px(void)
{
	uint8_t *rpt;
	uint8_t *gpt;
	uint8_t *bpt;
	
	uint8_t r_data;
	uint8_t g_data;
	uint8_t b_data;
	
	uint32_t  i;
	uint32_t  j;
	uint32_t  pt;
	
	float diff;
	float diff_255;
	
	diff = mlx_to_max - mlx_to_min;
	diff_255 = 255.0 / diff;
	
	for ( j = 0 ; j < 24; j++ ) {
	  
	  for ( i = 0 ; i < 32; i++ ) {
			  
            pt = (uint32_t) ( ( mlx_to[i + j*32] - mlx_to_min ) * diff_255 ); // �J���[�o�[ turbo_rgb[256][3] ���[=�Œቷ�x �E�[=�ō����x  
	
	    rpt = (uint8_t *)&turbo_rgb[pt][0];  // R �f�[�^�̊i�[�A�h���X
	    gpt = (uint8_t *)&turbo_rgb[pt][1];  // G �f�[�^�̊i�[�A�h���X
	    bpt = (uint8_t *)&turbo_rgb[pt][2];  // B �f�[�^�̊i�[�A�h���X
		
	    r_data = (*rpt >> 2) << 2;	//  6bit�f�[�^(���� 2bit �؂�̂�)��ɁA�������݃f�[�^�p�ɁA2�r�b�g���V�t�g
	    g_data = (*gpt >> 2) << 2;
	    b_data = (*bpt >> 2) << 2;
	    
	    
	    rgb666_data_buf[0] = (0x0100 | r_data);  // R�@�������݃f�[�^
	    rgb666_data_buf[1] = (0x0100 | g_data);  // G
	    rgb666_data_buf[2] = (0x0100 | b_data);  // B  
	    
	    lcd_adrs_set(318-i,j,319-i,j);      // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=318-i, �J�n�y�[�W=j, �I���J����=319-i, �I���y�[�W=j)
	  
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	    rspi_data_send(3,(uint16_t)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M

	  }
	}

}




// �J���[�}�b�v turbo �̕\��
//
//  �\���͈�:
//  st_col,st_row                                st_col+255,st_row
//
//  st_col,st_row+10                             st_col+255,st_row+10
//
void color_map_turbo(void)
{	
	uint8_t *rpt;
	uint8_t *gpt;
	uint8_t  *bpt;
	
	uint8_t r_data;
	uint8_t g_data;
	uint8_t b_data;
	
	uint32_t i;
	uint32_t j;
	uint32_t num;
	
	uint32_t st_col;
	uint32_t st_row;
	
		 
         for ( i = 0; i < 256 ; i++)	// �E�[���獶�[�܂ł̃J���[�o�[�̃f�[�^ 	
         {
		rpt = (uint8_t *)&turbo_rgb[i][0];  // R �f�[�^�̊i�[�A�h���X
		gpt = (uint8_t *)&turbo_rgb[i][1];  // G �f�[�^�̊i�[�A�h���X
		bpt=  (uint8_t *)&turbo_rgb[i][2];  // B �f�[�^�̊i�[�A�h���X
		
		r_data = (*rpt >> 2) << 2;	//  6bit�f�[�^(���� 2bit �؂�̂�)��ɁA�������݃f�[�^�p�ɁA2�r�b�g���V�t�g
	    	g_data = (*gpt >> 2) << 2;
	    	b_data = (*bpt >> 2) << 2;
	    
	    	rgb666_data_buf[i*3] = (0x0100 | r_data);      // R�@�������݃f�[�^
	    	rgb666_data_buf[i*3 + 1] = (0x0100 | g_data);  // G
	    	rgb666_data_buf[i*3 + 2] = (0x0100 | b_data);  // B  
         }
	 
	 st_col = 32;
	 st_row = 250;
	 
	 num = 768;		 // ���M�o�C�g�� (768 = 256 *3 )
	 
	 for ( j = 0; j < 10; j++ ) {
		 
	    lcd_adrs_set(st_col,st_row + j ,st_col + 255, st_row + j);	  // �������ݔ͈͎w��(�R�}���h 2a�ƃR�}���h 2b) (�J�n�J����=0, �J�n�y�[�W=0, �I���J����=255, �I���y�[�W=0)
        
	    spi_cmd_2C_send();	  	// Memory Write (2Ch)  �擪�ʒu(�R�}���h2a,2b�Ŏw�肵���ʒu)����f�[�^��������		
	 
	    rspi_data_send(num, (uint16_t *)&rgb666_data_buf[0]);  // �s�N�Z���f�[�^���M
	 
	 }
}


