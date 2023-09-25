#include "iodefine.h"
#include  "misratypes.h"
#include "rspi_9bit.h"
#include  "delay.h"
#include "ILI9488_9bit_dma.h"


uint16_t rspi_snd_buf[8];	// ILI9588へ送信するコマンド、パラメータを格納

uint16_t rgb666_data_buf[3456];  // 表示用バッファ (1152x3)  
				// 1文字分の表示用データ		
				// フォント24Wx48H ならば 144 byte= 144x8 =1152bit,  1bitの表示に3バイト必要(RGB666)	
 				//  DMA転送でRSPIに9bit送信するため、word データとしている。

// ILI9488の初期化
//
// 1) RGB 6-6-6 で使用 :  Interface Pixel Format (3Ah)
// 2) 縦型, →方向へ書き込み: Memory Access Control (36h) 
//
// (ILI)488 Datasheet 6.3. MCU to Memory Write/Read Direction )
//
// 
// Memory Access Control (36h)
// パラメータ (MADCTL)
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
	rspi_snd_buf[1] = 0x0106;	      // 262K色 18 bits/pixel RGB666 
	rspi_data_send(2, (uint16_t *)&rspi_snd_buf[0]);  // コマンド送信 
	
	
	rspi_snd_buf[0] = 0x0036;	      // Memory Access Control (36h)
	
					      // LCDからのケーブルを下から引き出す場合	
	rspi_snd_buf[1] = 0x0188;	      // コマンドレジスタ 0x36用パラメータ (MY=1,MX=0,MV=0) 
	
					      // LCDからのケーブルを上から引き出す場合	
	//rspi_snd_buf[1] = 0x0148;	      // コマンドレジスタ 0x36用パラメータ (MY=0,MX=1,MV=0)
	
	rspi_data_send(2, (uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	
	
	rspi_snd_buf[0] = 0x0011;             // Sleep OUT (11h),パラメータ数=0		
	rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	
	 
	rspi_snd_buf[0] = 0x0029;             // Display ON (29h), パラメータ数=0		
	rspi_data_send(1,(uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	 
	
	delay_msec(5);	    	    	      //  5msec待ち
}



// LCD のリセット
// 
void ILI9488_Reset(void)
{

	LCD_RESET_PODR = 0;              // LCD リセット状態にする
	delay_msec(1);		        // 1[msec] 待つ 
	
	LCD_RESET_PODR = 1;             // LCD 非リセット状態
	delay_msec(2);	        	// 2[msec] 待つ 
}


//  表示範囲の設定
// 入力:
//  col: 開始カラム(x), page(row):開始ページ(y)
//  col2:終了カラム, page2(row2): 終了ページ
//
void lcd_adrs_set( uint16_t col, uint16_t page, uint16_t col2, uint16_t page2)
{
 	 rspi_snd_buf[0] = 0x002a;  			       // Column Address Set コマンドレジスタ 0x2a , パラメータ数=4
	 rspi_snd_buf[1] = 0x0100 | ((0xff00 & col) >> 8);     //  SC[15:8]　スタートカラム(16bit)の上位バイ
	 rspi_snd_buf[2] = 0x0100 | (0x00ff & col);            //  SC[7:0]         :　　　　　　　　下位バイト 
	 rspi_snd_buf[3] = 0x0100 | ((0xff00 & col2) >> 8);    //  EC[15:8]　終了カラム(16bit)の上位バイト
	 rspi_snd_buf[4] = 0x0100 | (0x00ff & col2);           //  EC[7:0]         :　　　　　　　　下位バイト 
	 
	 rspi_data_send(5,(uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	
	 rspi_snd_buf[0] = 0x002b;				       //  Page Address Set コマンドレジスタ 0x2b , パラメータ数=4
	 rspi_snd_buf[1] = 0x0100 | ((0xff00 & page ) >> 8);    //  SP[15:8]　スタートページ(16bit)の上位バイト
	 rspi_snd_buf[2] = 0x0100 | (0x00ff & page);            //  SP[7:0]         :　　　　　　　　下位バイト 
	 rspi_snd_buf[3] = 0x0100 | ((0xff00 & page2 ) >> 8);    // EP[15:8]　終了ページ(16bit)の上位バイト
	 rspi_snd_buf[4] = 0x0100 | (0x00ff & page2);            // EP[7:0]         :　　　　　　　　下位バイト 
	 
	rspi_data_send(5,(uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
		
}


//
//  Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み
// 
void spi_cmd_2C_send( void )
{
	 rspi_snd_buf[0] = 0x002c;		       // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み
	
	 rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	
}


//
// 　ピクセルデータの書き込みテスト (RGB 6-6-6用)
//    Memory Access Control (36h) の設定により、データの表示位置が変わる事を確認
//   
//   ILI9488 data sheet:
//   4.7.1.2. SPI Data for 18-bit/pixel (RGB 6-6-6 Bits Input), 262K-color
//
// 色          R         G         B  
// 白       11111100  11111100  11111100
// 黄色     1iiiii00  11111100  00000000
// シアン   00000000  11111100  11111100
// 緑       00000000  11111100  00000000
// マゼンタ 11111100  00000000  11111100
// 赤       11111100  00000000  00000000
// 青       00000000  00000000  11111100
// 黒       00000000  00000000  00000000
//

void pixel_write_test_rgb666()
{
	uint16_t pix_num;
	uint16_t wr_num;
	
	pix_num = 1;		// 書き込みピクセル数
	wr_num = pix_num * 3;	// 送信バイト数 ( 3 byteで 1ピクセル分の情報　)
	
	rgb666_data_buf[0] = 0x01fc;   // 書き込みデータのセット (黄色)
	rgb666_data_buf[1] = 0x01fc;
	rgb666_data_buf[2] = 0x0100;
	
	lcd_adrs_set(0,0, pix_num, 0);	  // Column Address Set(2Ah), Page Address Set(2Bh) (開始カラム=0, 開始ページ=0, 終了カラム=pix_num, 終了ページ=0)
	
	 
	 spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 
	
	 rspi_data_send(wr_num,(uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信

	
}



//  ILI9488  LCD カラーバー(8色) (RGB 6-6-6)
//   (320x480)
//
// 色          R         G         B  
// 白       11111100  11111100  11111100
// 黄色     1iiiii00  11111100  00000000
// シアン   00000000  11111100  11111100
// 緑       00000000  11111100  00000000
// マゼンタ 11111100  00000000  11111100
// 赤       11111100  00000000  00000000
// 青       00000000  00000000  11111100
// 黒       00000000  00000000  00000000
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
		 
         for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (白) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,0, 319,59);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=0, 終了カラム=319, 終了ページ=59)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (黄色) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,60, 319,119);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=60, 終了カラム=319, 終了ページ=119)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (シアン) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,120, 319,179);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=120, 終了カラム=319, 終了ページ=179)
	rgb666_data_send();
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (緑) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x01fc;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,180, 319,239);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=180, 終了カラム=319, 終了ページ=239)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (マゼンタ) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,240, 319,299);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=240, 終了カラム=319, 終了ページ=299)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (赤) 	
         {
		rgb666_data_buf[i*3] = 0x01fc;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,300, 319,359);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=300, 終了カラム=319, 終了ページ=359)
	rgb666_data_send();
	 
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (青) 	
         {
		rgb666_data_buf[i*3] = 0x0100;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x01fc; 
         }
	 
	lcd_adrs_set(0,360, 319,419);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=360, 終了カラム=319, 終了ページ=419)
	rgb666_data_send();
	
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (黒)
         {
		rgb666_data_buf[i*3] = 0x0100;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	lcd_adrs_set(0,420, 319,479);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ=360, 終了カラム=319, 終了ページ=419)
	rgb666_data_send();
	
}



//　カラーバー用　データ送信 (RGB 6-6-6用)
//  60行分表示
void rgb666_data_send(void)
{
	uint32_t i;
	uint32_t num;
	
	 num = 960;		 // 送信バイト数

	 spi_cmd_2C_send();	  // Memory Write (2Ch)  先頭位置(コマンド2a,2bで指定した位置)からデータ書き込み		
	 rspi_data_send(num, (uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信
	 
	 
	 for ( i = 0; i < 59; i++ ) {
	   rspi_snd_buf[0] = 0x003c;		 // Memory Write Continue (3Ch)
	   rspi_data_send(1, (uint16_t *)&rspi_snd_buf[0]);  // コマンド送信
	   
	   rspi_data_send(num,(uint16_t *)&rgb666_data_buf[0]);  // ピクセルデータ送信
	 
	 }
	 
	
}



//
// 画面を黒にする
//
void disp_black_rgb666(void)
{
	uint32_t i;
	
	uint32_t  j;
	
	 for ( i = 0; i < 320 ; i++)	// ピクセルデータを流し込む (1 行分) (黒)
         {
		rgb666_data_buf[i*3] = 0x0100;   // 書き込みデータのセット 
		rgb666_data_buf[i*3 + 1] = 0x0100;
		rgb666_data_buf[i*3 + 2] = 0x0100; 
         }
	 
	
	for ( j = 0 ; j < 8 ; j++ ) {
		
	  lcd_adrs_set(0,j*60, 319, j*60+59);	  // 書き込み範囲指定(コマンド 2aとコマンド 2b) (開始カラム=0, 開始ページ= J*60, 終了カラム=319, 終了ページ= J*60+59)
        
	  rgb666_data_send();			  // 60行分の表示
	}
	
	
}


	


