#include "iodefine.h"
#include  "misratypes.h"

#include "mlx90640_iic.h"
#include "ILI9488_9bit_dma.h"
#include "font_16_32_mlx.h"
#include "font_48_24_mlx.h"
#include "disp_number.h"
#include "thermocouple.h"

//
// 数字の表示テスト
//
// 入力: index : 表示文字を示すインデックス (0～21) 
//
//  index  表示文字
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
//   10       0.  (0のピリオド付き)
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
//             。 。　       。 。　       
//　　　　　　VCC GND          T_IRQ
//
//

void disp_num_test(uint8_t index )
{
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	
	st_col = 16;		// 開始カラム
	st_row = 260;		// 開始行
	
				// 16(W)x32(H)
	end_col = st_col + 15;	// 終了カラム
	end_row = st_row + 31;	// 終了行
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	spi_data_send_id(index);  	// データ送信　小数点第1位の数字	
	
}

//  MLX90640の Em(放射率)と周囲温度(Ta) 及び
//  熱電対の各チャンネル(CH1～CH4)の温度と基準接点温度の表示
//
//  

void disp_em_temp(void)	
{

	disp_float_data_em( mlx_emissivity, 64, 316);  // Em 放射率の表示
	
	disp_float_data( mlx_ta , 64, 358);         // Ta MLX90640の周囲温度の表示
	disp_float_data( mlx_tr , 64, 400);         // reflected temperatureの表示 
	
	disp_float_data( cj_temp , 64, 442);         //　 基準接点温度の表示 

	disp_float_data( tc_temp[0],256,316);  	     // CH1 温度表示
	disp_float_data( tc_temp[1],256,358);  	     // CH2 温度表示
	disp_float_data( tc_temp[2],256,400);  	     // CH3 温度表示
	disp_float_data( tc_temp[3],256,442);  	     // CH4 温度表示

}



//	
// ℃,Em,Ta,CH1..等のラベル表示
//
void disp_temp_label(void)
{
	disp_celsius();      // ℃の表示
	
	disp_s48_24_font( INDEX_FONT_EM, 16, 316); // ラベル( Em: )の表示
	disp_s48_24_font( INDEX_FONT_TA, 16, 358); // ラベル( Ta: )の表示
	disp_s48_24_font( INDEX_FONT_TR, 16, 400); // ラベル( Tr: )の表示
	disp_s48_24_font( INDEX_FONT_CJT, 16, 442); // ラベル( CJT: )の表示
	
	disp_s48_24_font( INDEX_FONT_CH1,208, 316); // ラベル( CH1: )の表示 
	disp_s48_24_font( INDEX_FONT_CH2,208, 358); // ラベル( CH2: )の表示 
	disp_s48_24_font( INDEX_FONT_CH3,208, 400); // ラベル( CH3: )の表示 
	disp_s48_24_font( INDEX_FONT_CH4,208, 442); // ラベル( CH4: )の表示 
}


// ℃の表示 (48x96)
 void disp_celsius(void)
{

	lcd_adrs_set(184, 260, 199, 291 );	 // 書き込み範囲指定 
	
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	spi_data_send_id(INDEX_FONT_CELSIUS);  	// データ送信　小数点第1位の数字	
	
}



//  文字の表示  (フォントサイズ: 48(W)x24(H) 用)
//
// 入力:  dt_index      : 表示文字列のindex
//        st_col        : 書き込み先頭 カラム (x)
//        st_row        : 書き込み先頭 ページ (y)
//
//  フォントデータのサイズ(byte):  144 = 48/8 * 24
//  送信データ数 :　3456 = 144 x 8 x 3      (1pixelの表示に3byte要) 
//


void disp_s48_24_font( uint8_t dt_index,  uint32_t st_col, uint32_t st_row)
{
	uint8_t *pt;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t len;
	uint32_t num;
	
	 
	pt = (uint8_t *)&font_48w_24h_seg[dt_index][0];  // フォントデータの格納アドレス
	
	len = 144;				// フォントデータのバイト数
	num = 3456;				// 送信データ数   RGB666
 	
	unpack_font_data_rgb666 ( len, pt );    // フォントデータを rgb666_data_burへ展開 
	
	
	end_col = st_col + 47;	     		 // 文字の書き込み終了カラム
	end_row = st_row + 23;       		 // 　　　　　　:    　ページ
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定
	
	spi_cmd_2C_send();	  		// Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		

	rspi_data_send( num, (uint16_t *)&rgb666_data_buf[0] );   //  データ送信
	
}





//
//  float型データを4桁の文字として表示する。
//  16Hx32Wフォント 文字の色は白、背景色は黒        
//  入力: 
//        t : 表示する float 型データ
//    start_col: 表示する最初の文字のカラム位置  (x)      
//    start_page:表示する最初の文字のページ位置  (y)
//
// 出力: 
//       表示範囲 -99.9 ～ 999.9　  , 4桁目（数値またはマイナス),3桁目,2桁目+小数点, 1桁目
//　   
//
//   
// 例1:  温度 0.9℃
//   4桁目  3桁目  2桁目  1桁目
//    空白　　空白    0.    9 
//
// 例2:  温度 12.3℃ 
//  4桁目  3桁目  2桁目  1桁目
//　  空白  　1     2.    3 
//
// 例3:  温度 213.4℃ 
// 4桁目  3桁目  2桁目  1桁目
//    2　  　1     3.      4 
//
// 例4:  温度 -5.2℃
//  4桁目  3桁目  2桁目  1桁目
//    - 　　空白    5.    2 
//
// 例5:  温度 -12.3℃ 
// 4桁目  3桁目  2桁目  1桁目
//  -　   　1     2.    3 
//

void disp_float_data(float t, uint32_t start_column, uint32_t start_row )
{
	uint8_t dig1;    // 1桁目 小数点第1位
	uint8_t dig2;    // 2桁目 1の位　
	uint8_t dig3;    // 3桁目 10の位
	uint8_t dig4;    // 4桁目 100の位
	
	uint8_t para[1];
	
	int16_t  temp;
	
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t col_offset;
	
	temp = t * 10.0;	// 温度を10倍する

	
	st_col = start_column;
	st_row = start_row;

	         	// フォントサイズ 16(W)x32(H)
	col_offset = 15;
	end_col = st_col + col_offset;
        end_row = st_row + 31;				
	
	
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
		
	if ( temp >= 0 ) {            // 温度が正の場合  0～999.9 表示					   
		dig4 = temp / 1000;
		
		if ( dig4 == 0 ) {
			 spi_data_send_id(INDEX_FONT_BLANK);  // データ送信 " "(空白)
		}
		
		else{
			spi_data_send_id(dig4);  // データ送信 100の位の数字
		}
	}
	else {				// 温度が負の場合 -99.9～ - 0.1
		spi_data_send_id(INDEX_FONT_MINUS);  // データ送信 "-"(マイナス符号)
		
		temp = - temp;		// 正の値として表示処理	
		
		dig4 = 0;
	}
	
	
					// 10の位 表示
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;		
	
						// 10の位 表示
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	dig3  = ( temp - (dig4 * 1000)) / 100;
	
	if (( dig4 == 0 ) && ( dig3  == 0 )) { // 100の位が空白で、10の位も空白の場合 (例:　5.1) 
	  	spi_data_send_id(INDEX_FONT_BLANK);  // データ送信 " "(空白)	
	}
	else{
	       spi_data_send_id(dig3);  // データ送信 10の位の数字
	}
	

	
					// 1の位の表示(小数点付き)
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
					   
	dig2 = ( temp - (dig4 * 1000) - (dig3 * 100) ) / 10;
	
	spi_data_send_id( dig2 + 10 );		  // データ送信 1の位の数字(小数点付き) (小数点付きのデータは、数字 + 10 )

	
							// 小数点第1位の表示
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;						
					
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	dig1 = temp - (dig4 * 1000) - (dig3 * 100) - (dig2 * 10);
	
	spi_data_send_id(dig1);  	// データ送信　小数点第1位の数字	
	
	
}



//
//  放射率(Em)　専用
//  float型データを4桁の文字として表示する。
//  16Hx32Wフォント 文字の色は白、背景色は黒        
//  入力: 
//        t : 表示する float 型データ
//    start_col: 表示する最初の文字のカラム位置  (x)      
//    start_page:表示する最初の文字のページ位置  (y)
//
// 出力: 
//       表示範囲 0.01 ～ 1.00　  , 4桁目,3桁目+小数点,2桁目, 1桁目
//　   
//
//   
// 例1:  Em = 0.95
//   4桁目  3桁目  2桁目  1桁目
//    空白    0.    9      5
//
//

void disp_float_data_em(float t, uint32_t start_column, uint32_t start_row )
{
	uint8_t dig1;    // 1桁目 小数点第2位
	uint8_t dig2;    // 2桁目 小数点第1位
	uint8_t dig3;    // 3桁目 1の位
	uint8_t dig4;    // 4桁目 10の位
	
	uint8_t para[1];
	
	int16_t  temp;
	
	uint32_t st_col;
	uint32_t st_row;
	
	uint32_t end_col;
	uint32_t end_row;
	
	uint32_t col_offset;
	
	temp = t * 100.0;	// 100倍する

	
	st_col = start_column;
	st_row = start_row;

	         	// フォントサイズ 16(W)x32(H)
	col_offset = 15;
	end_col = st_col + col_offset;
        end_row = st_row + 31;				
	
	
							// 4桁目 空白表示										
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	spi_data_send_id(INDEX_FONT_BLANK);  // データ送信 " "(空白)	
	
	
	
					// 3桁目の 表示
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;		
	
						
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	dig3  =  temp  / 100;
	spi_data_send_id(dig3+10);  // データ送信 1の位の数字(小数点付き) (小数点付きのデータは、数字 + 10 )
	
	
					// 2桁目の 表示
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;
	
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
					   
	dig2 = ( temp - (dig3 * 100) ) / 10;
	
	spi_data_send_id( dig2 );		  // データ送信 小数点第一位の数字

	
					// 1桁目の表示
	st_col = end_col + 1;		// 前の文字の最終カラム + 1					
	end_col = st_col + col_offset;						
					
	lcd_adrs_set(st_col, st_row, end_col, end_row );	 // 書き込み範囲指定 
	spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	dig1 = temp - (dig3 * 100) - (dig2 * 10);
	
	spi_data_send_id(dig1);  	// データ送信　小数点第2位の数字	
	
}


//  フォント 16Wx32H pixel のデータ送信
//  
//  入力: index:  送信するデータ
//
//  16(W)x32(H) フォントデータのサイズ(byte):  64 = 16/8 * 32
//  送信データ数 (byte):　1536 = 64 x 8 x 3    (1pixelの表示に3byte要) 
//    
//
// ・indexと表示文字の関係 
//  index  表示文字
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
//   10       0.  (0のピリオド付き)
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


					      // フォントサイズ 16(W)x32(H)						
	pt = (uint8_t *)&font_16w_32h_seg[index][0];  // フォントデータの格納アドレス
	
	len = 64;				// フォントデータのバイト数
	num = 1536;				// 送信データ数   RGB666
 	
	unpack_font_data_rgb666 ( len, pt ); // フォントデータを rgb666_data_burへ展開 
	
	rspi_data_send( num, (uint16_t *)&rgb666_data_buf[0] );   //  データ送信
	

}


// ビットマップのフォントデータ(1byte単位)を、RGB666に変換して、
// 表示用のバッファにセット
// 文字色は白、背景色は黒　
//  入力: * ptt: フォントデータの先頭アドレス
//         len:  フォントデータのバイト数
//               (16Wx32H ならば  64byte)    64x8=512bit
//               (32Wx64H ならば　256byte)　256x8=2048bit
// 
//    rgb666_data_buf[6144];  2048x3 byte (2048pixel分)( 6.4行分)
//
void unpack_font_data_rgb666 ( uint32_t len, uint8_t * src_ptr )
{
	
	uint8_t  bd;
	uint8_t  mask;
	uint8_t  cd;
	
	uint32_t i;
	uint32_t j;
	uint32_t pt;
	
	pt = 0;			// 格納位置のクリア
	
	for ( i = 0; i < len  ; i++ ){
		
	  bd = *src_ptr;	// ビットマップデータ取り出し
	
	  mask = 0x80;          // マスクデータのセット
	  
	  for ( j = 0 ; j < 8 ; j++ ) {   // b8から ビットOn/Offの判定	
	    if ( ( bd & mask ) != 0x00 ) {  // ビットOnの場合
		
		rgb666_data_buf[pt] = 0x01fc;  //   R: 1111 1100
		pt = pt + 1;		     // 格納位置のインクリメント
		
		rgb666_data_buf[pt] = 0x01fc;    // G: 1111 1100
		pt = pt + 1;
		
		rgb666_data_buf[pt] = 0x01fc;    // B: 1111 1100
		pt = pt + 1;
	     }
	     else {				// ビットOffの場合
	     
		rgb666_data_buf[pt] = 0x0100;  //   R: 0000 0000
		pt = pt + 1;		     // 格納位置のインクリメント
		
		rgb666_data_buf[pt] = 0x0100;    // G: 0000 0000
		pt = pt + 1;
		
		rgb666_data_buf[pt] = 0x0100;    // B: 0000 0000
		pt = pt + 1;
	     
	     }
	
	     mask = mask >> 1;		// マスクデータを、右 1ビットシフト
	
	  }
	  
	  src_ptr++;			// 取り出し位置インクリメント
		
	}

}

