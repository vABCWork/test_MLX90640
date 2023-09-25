#include "iodefine.h"
#include  "misratypes.h"

#include "ILI9488_9bit_dma.h"

#include "mlx90640_iic.h"
#include "mlx90640_disp.h"
#include "colormap_turbo.h"
#include "disp_number.h"

float mlx_to_interpolate[100];  // 双線形補間した温度( 1pixel分 ) ( MLX 1pixel → LCD 10x10 dot ) 

float mlx_to_max;		// 対象物の温度 最大値
float mlx_to_min;		// 対象物の温度 最小値
float mlx_to_center;            //  (最大値 - 最小値)/2


// 双線形補間の計算
// 　参考: https://x-engineer.org/bilinear-interpolation/
//
//  指定したピクセルの温度を、双線形補間して、mlx_to_interpolate[]へ格納する
// 入力: col :  0 - 30    ( colの最大=30 (Q2,Q4のcol=31のため))
//       row :  0 - 22    ( rowの最大=22 (Q3,Q4のrow=23のため))
//   
//  Qの値は、xとyの値から定められています。
//  4点(a,b),(c,b),(a,d),(c,d)と、そのQ値が分かっている場合に、
//  P点(x,y)での 値を双線形補間により求めます。
//
//  y(row)
//   |
// d-|-- Q3         Q4
//   |
//   |    　　P
// b-|---Q1         Q2
//   |   |
//   +---+-----------+------------- x(col)
//       a           c 
//  
// x,yの値と Qの対応 : (a,b) = Q1 , (c,b) = Q2 , (a,d) = Q3,  (c,d) = Q4
//
// 計算手順:
//  1) x軸方向に線形補間を行い、R1,R2を求めます。
//    R1 = ((Q2 - Q1)/( c - a ))*( x - a ) + Q1 
//    R2 = ((Q4 - Q3)/( c - a ))*( x - a ) + Q3 
//  2)R1,R2をy軸方向に線形補間を行い P点(x,y)での値を求めます。
//    P(x,y) = ((R2 - R1)/( d - b))*( y - b ) + R1 
//
// ・プログラムでは、a=0,b=0, c=10,d=10 としています。
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
	
	
	q1 = mlx_to[col + row*32];		// 4隅の温度を得る
	q2 = mlx_to[col+1 + row*32];
	q3 = mlx_to[col + (row+1)*32];
	q4 = mlx_to[col+1 +(row+1)*32];
	
	for ( j = 0 ; j < 10 ; j++ ) {
	  for ( i = 0 ; i < 10 ; i++ ) {
	    r1 = (( q2 - q1 ) / 10.0) * i + q1;
	    r2 = (( q4 - q3 ) / 10.0) * i + q3;
	
	    p = (( r2 - r1 ) / 10.0 ) * j + r1;
	  
	    mlx_to_interpolate[i + 10*j] = p;	// 双線形補間した値の格納
	  }
	}
	
}


// 
//   対象物の温度(最大値と最小値)を得る
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


// カラーマップの両端の下側に、測定温度の最小値と最大値を表示
//
 void mlx_to_min_max_disp(void)
 {
	disp_float_data(mlx_to_min,0,260);	// 最小値表示 フォントサイズ 16Wx32H
	
	disp_float_data(mlx_to_max,256,260);  // 最小値表示
}

 //
// 中央の値を表示
// 
 void mlx_to_center_disp(void)
 {
	mlx_to_center = (( mlx_to_max - mlx_to_min ) / 2.0) + mlx_to_min;
	
	disp_float_data( mlx_to_center,120,260);	// 最小値表示 フォントサイズ 16Wx32H
	
}
 

// 測定温度に対応したカラーマップを表示 補間あり(320x240)
//
// MLX90640 (32Wx24H):
//  ・元データ 32x24個のデータ
//    0    　1  　2 ... 30 31
//  0 22℃ 23℃
//  1 32℃　
//  :
// 23
//
//  ・補間データ 320x240個のデータ
//	0    .1   .2  .3    .4   .5   .6   .7   .8   .9  1        2 ... 30 31
//  0 22℃ 22.1 22.2 22.3 22.4 22.5 22.6 22.7 22.8 22.9  23℃
// .1 23
// .2 24
// .3 25
// .4 26
// .5 27
// .6 28
// .7 29
// .8 30
// .9 31
// 1  32℃　
//  :
// 23
//
//  
// LCD 表示範囲:
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
               mlx_bilinear_interpolate( i, j );  // 双線形補間した値を　mlx_to_interpolate[100]へ格納する 100usec
	    }	  
	    else {                             // i = 31 または j = 23の場合、
	       for ( k = 0; k < 100 ; k++ ) {  // 補間無しで同じ値をセット 
		   mlx_to_interpolate[k]  = mlx_to[i + j*32];
	       }    
	    }
	    
            mlx_interpolate_to_rgb();	     //  正方形(10x10)のデータセット	73usec
	    
	    lcd_adrs_set(310-i*10,j*10,319-i*10,j*10+9);    // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=j, 終了カラム=31, 終了ページ=j)
	    
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	    rspi_data_send(300,(uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信 送信バイト数 (300 = 10*10*3 )
	    
	  }
 
	}
}


//
// 双線形補間した値 ( mlx_to_interpolate[100] )から、rgbデータを得る
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
	
           pt = (uint32_t) (( mlx_to_interpolate[k] - mlx_to_min ) * diff_255 ); // カラーバー turbo_rgb[256][3] 左端=最低温度 右端=最高温度
	   
	   if ( pt > 255 ) {
	   	pt = 255;
	   }
	   
	   rpt = (uint8_t *)&turbo_rgb[pt][0];  // R データの格納アドレス
	   gpt = (uint8_t *)&turbo_rgb[pt][1];  // G データの格納アドレス
	   bpt = (uint8_t *)&turbo_rgb[pt][2];  // B データの格納アドレス
		
	   r_data = (*rpt >> 2) << 2;	//  6bitデータ(下位 2bit 切り捨て)後に、書き込みデータ用に、2ビット左シフト
	   g_data = (*gpt >> 2) << 2;
	   b_data = (*bpt >> 2) << 2;
	
	   rgb666_data_buf[k*3] = (0x0100 | r_data);      // R　書き込みデータ
	   rgb666_data_buf[k*3 + 1] = (0x0100 | g_data);  // G
	   rgb666_data_buf[k*3 + 2] = (0x0100 | b_data);  // B  
	
	}
}




// 測定温度に対応したカラーマップを表示 (320x240) （補間なし)(テスト用)
//
//  MLXの1ピクセルが、LCDの10x10に対応
//
//・LCDの表示領域とMLXのピクセル(col1～col32,row1～row24)との対応
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
		  
	    pt = (uint32_t) ( ( mlx_to[i + j*32] - mlx_to_min ) * diff_255 ); // カラーバー turbo_rgb[256][3] 左端=最低温度 右端=最高温度
	    
	    if ( pt > 255 ) {
	   	pt = 255;
	    }
	    
	    rpt = (uint8_t *)&turbo_rgb[pt][0];  // R データの格納アドレス
	    gpt = (uint8_t *)&turbo_rgb[pt][1];  // G データの格納アドレス
	    bpt = (uint8_t *)&turbo_rgb[pt][2];  // B データの格納アドレス
		
	    r_data = (*rpt >> 2) << 2;	//  6bitデータ(下位 2bit 切り捨て)後に、書き込みデータ用に、2ビット左シフト
	    g_data = (*gpt >> 2) << 2;
	    b_data = (*bpt >> 2) << 2;
		
	    rgb666_square_data_set( r_data, g_data, b_data);  // 正方形(10x10)のデータセット
	    
	  
	    lcd_adrs_set(310-i*10,j*10,319-i*10,j*10+9);         // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=j, 終了カラム=31, 終了ページ=j)
								//lcd_adrs_set(i*10,j*10,i*10+9,j*10+9);	// 鏡像
	    
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	  
	    rspi_data_send(300, (uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信
	  
	  }
 
	}
}

//
// 四角形(10Wx10H) 分の rgb666データをセット
//
void rgb666_square_data_set( uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t i;
	
	for ( i = 0 ; i < 100; i++ ){
	
	    rgb666_data_buf[i*3] =  (0x0100 | r);      // R　書き込みデータ
	    rgb666_data_buf[i*3 + 1] = ( 0x0100 | g);  // G
	    rgb666_data_buf[i*3 + 2] = ( 0x0100 | b);  // B  
	}
	
}


// 測定温度に対応したカラーマップを表示 (32Wx24H)(テスト用)
//
//・LCDの表示領域とMLXのピクセル(col1～col32,row1～row24)との対応
//
//
// 表示範囲:
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
			  
            pt = (uint32_t) ( ( mlx_to[i + j*32] - mlx_to_min ) * diff_255 ); // カラーバー turbo_rgb[256][3] 左端=最低温度 右端=最高温度  
	
	    rpt = (uint8_t *)&turbo_rgb[pt][0];  // R データの格納アドレス
	    gpt = (uint8_t *)&turbo_rgb[pt][1];  // G データの格納アドレス
	    bpt = (uint8_t *)&turbo_rgb[pt][2];  // B データの格納アドレス
		
	    r_data = (*rpt >> 2) << 2;	//  6bitデータ(下位 2bit 切り捨て)後に、書き込みデータ用に、2ビット左シフト
	    g_data = (*gpt >> 2) << 2;
	    b_data = (*bpt >> 2) << 2;
	    
	    
	    rgb666_data_buf[0] = (0x0100 | r_data);  // R　書き込みデータ
	    rgb666_data_buf[1] = (0x0100 | g_data);  // G
	    rgb666_data_buf[2] = (0x0100 | b_data);  // B  
	    
	    lcd_adrs_set(318-i,j,319-i,j);      // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=318-i, 開始ページ=j, 終了カラム=319-i, 終了ページ=j)
	  
	    spi_cmd_2C_send();	  		// Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	    rspi_data_send(3,(uint16_t)&rgb666_data_buf[0]);  // ピクセルデータ送信

	  }
	}

}




// カラーマップ turbo の表示
//
//  表示範囲:
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
	
		 
         for ( i = 0; i < 256 ; i++)	// 右端から左端までのカラーバーのデータ 	
         {
		rpt = (uint8_t *)&turbo_rgb[i][0];  // R データの格納アドレス
		gpt = (uint8_t *)&turbo_rgb[i][1];  // G データの格納アドレス
		bpt=  (uint8_t *)&turbo_rgb[i][2];  // B データの格納アドレス
		
		r_data = (*rpt >> 2) << 2;	//  6bitデータ(下位 2bit 切り捨て)後に、書き込みデータ用に、2ビット左シフト
	    	g_data = (*gpt >> 2) << 2;
	    	b_data = (*bpt >> 2) << 2;
	    
	    	rgb666_data_buf[i*3] = (0x0100 | r_data);      // R　書き込みデータ
	    	rgb666_data_buf[i*3 + 1] = (0x0100 | g_data);  // G
	    	rgb666_data_buf[i*3 + 2] = (0x0100 | b_data);  // B  
         }
	 
	 st_col = 32;
	 st_row = 250;
	 
	 num = 768;		 // 送信バイト数 (768 = 256 *3 )
	 
	 for ( j = 0; j < 10; j++ ) {
		 
	    lcd_adrs_set(st_col,st_row + j ,st_col + 255, st_row + j);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=0, 終了カラム=255, 終了ページ=0)
        
	    spi_cmd_2C_send();	  	// Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	    rspi_data_send(num, (uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信
	 
	 }
}


