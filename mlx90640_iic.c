#include "iodefine.h"
#include "misratypes.h"
#include "mlx90640_iic.h"
#include "riic_mlx.h"


volatile uint8_t iic_comm_fg;	// 通信中フラグ　STOP発行でクリア
volatile uint8_t mlx_status_read_req;  // ステータスレジスタ読み出し要求フラグ

uint16_t mlx_status_reg; // ステータスレジスタ RAM(0x8000)の内容
uint16_t mlx_control_reg_1;  // コントロールレジスタ 1 (0x800D)の内容


uint16_t mlx_ram[834];		// MLX90640 RAM(0x0400-0x073F) (0x33F = 831)のデータと(Control register1)と (statusRegister & 0x0001)
uint16_t mlx_eeprom[832];	// MLX90640 EEPROM(0x2400-0x273F) (0x33F = 831) 


uint8_t mlx_subpage0_ready;      // 1:Subpage0　読みだし済み
uint8_t mlx_subpage1_ready;      //  1:Subpage1 読み出し済み


float mlx_emissivity;		// 放射率

float mlx_ta;      		// 周囲温度[℃]
float mlx_tr;			// 室温補正
float mlx_to[768];		//　各ピクセル(1～768)の計算した温度




// MLX90640 RAM(0x0400-0x073F)のデータと(Control register1)と (statusRegister & 0x0001)の読み出し
//
void MLX_Get_FrameData(void)
{
	Read_MLX_RAM();		//  RAM(0x400～0x73F)読み出し、mlx_ram[0]～mlx_ram[831] に格納する。
	
	Read_MLX_Control_Register_1(); // コントロールレジスタ1(0x800D)読み出し、mlx_control_reg_1に格納
	
	mlx_ram[832] = mlx_control_reg_1;
	
	Read_MLX_Status_Register();	// ステータスレジスタ(0x8000)読み出し、mlx_status_regに格納
	
	mlx_ram[833] = mlx_status_reg & 0x0001;  // 0=subpage 0 measured , 1=subpage 1 measured.
	

}



// Control register 1: 0x800D
//   デフォルト = 0x1901 
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



// リフレッシュレートの　4[Hz]設定
void MLX_Set_Refresh_rate(void)
{
				
	iic_sd_data[3] = 0x19;             // IR refrsh rate 4 [Hz]設定
	iic_sd_data[4] = 0x81;

	 
    	 Write_MLX_Control_Register_1(); 	// MLX90640 コントロールレジスタ1(0x800D)書き込み
	 
	 while( iic_com_over_fg != 1 ) {     // 通信完了待ち(受信完了待ち)
	 }
}


//
// MLX90640 ステータスレジスタ(0x8000)読み出し
//	mlx_status_regに格納
//
//   IIC 送信バッファ
//   　sci_iic_sd_data[0] : スレーブアドレス(7bit) + Wrビット(0)
//     sci_iic_sd_data[1] : 読み出しアドレス(上位バイト側)
//     sci_iic_sd_data[2] : 読み出しアドレス(下位バイト側)
//     sci_iic_sd_data[3] : スレーブアドレス(7bit) + Rdビット(1)
//
void Read_MLX_Status_Register(void)
{
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // 書き込み用 スレーブアドレス
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // 読み出し用　スレーブアドレス 
	
	iic_sd_data[1] = 0x80;             // Status register アドレス = 0x8000
	iic_sd_data[2] = 0x00;
	
	iic_sd_rcv_fg = 1;			// マスタ送受信処理
	
	riic_sd_start();			// SCI IIC 送信開始
	
	
	while( iic_com_over_fg != 1 ) {     // 通信完了待ち(受信完了待ち)
	}
	 
	mlx_status_reg = iic_rcv_data[0];	// 上位バイト

	mlx_status_reg = ( mlx_status_reg << 8 ) | iic_rcv_data[1];
	
	
}

//
// MLX90640 コントロールレジスタ1(0x800D)読み出し
//	mlx_control_reg_1に格納
//
//   IIC 送信バッファ
//   　sci_iic_sd_data[0] : スレーブアドレス(7bit) + Wrビット(0)
//     sci_iic_sd_data[1] : 読み出しアドレス(上位バイト側)
//     sci_iic_sd_data[2] : 読み出しアドレス(下位バイト側)
//     sci_iic_sd_data[3] : スレーブアドレス(7bit) + Rdビット(1)
//
void Read_MLX_Control_Register_1(void)
{
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // 書き込み用 スレーブアドレス
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // 読み出し用　スレーブアドレス 
	
	iic_sd_data[1] = 0x80;             // Cotrl register 1 アドレス = 0x800d
	iic_sd_data[2] = 0x0d;
	
	iic_sd_rcv_fg = 1;			// マスタ送受信処理
	
	riic_sd_start();			// SCI IIC 送信開始
	
	
	while( iic_com_over_fg != 1 ) {     // 通信完了待ち(受信完了待ち)
	}
	 
	mlx_control_reg_1 = iic_rcv_data[0];	// 上位バイト

	mlx_control_reg_1 = ( mlx_control_reg_1 << 8 ) | iic_rcv_data[1];
	
	
}


//
// MLX90640 コントロールレジスタ 1(0x800D)書き込み
//
//   IIC 送信バッファ
//   　sci_iic_sd_data[0] : スレーブアドレス(7bit) + Wrビット(0)
//     sci_iic_sd_data[1] : 書き込みアドレス(上位バイト側)
//     sci_iic_sd_data[2] : 書き込みアドレス(下位バイト側)
//     sci_iic_sd_data[3] : 書き込みデータ(上位バイト側)
//     sci_iic_sd_data[4] : 書き込みデータ(下位バイト側)
//

void Write_MLX_Control_Register_1(void)
{
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // 書き込み用 スレーブアドレス
	
	iic_sd_data[1] = 0x80;             // Cotrl register 1 アドレス = 0x800d
	iic_sd_data[2] = 0x0d;
	
	iic_sd_rcv_fg = 0;			// マスタ送信処理
	iic_sd_num = 5;				// 送信データ数
	
	riic_sd_start();			// SCI IIC 送信開始
	
}



//
// MLX90640 RAM(0x400～0x73F)読み出し、
//　uint16_t mlx_ram[832] に格納する。
//
void Read_MLX_RAM(void)
{
	uint16_t i;
	uint16_t adrs;
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // 書き込み用 スレーブアドレス
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // 読み出し用　スレーブアドレス 
	
	for ( i = 0 ; i < 832 ; i++ ) {    // RAM 0x400-0x73F　の読み出し 832 word
	
	  adrs = 0x400 + i;	
		
	  iic_sd_data[1] = (adrs >> 8 );    //  読み出しアドレスの上位バイト
	  iic_sd_data[2] = adrs & 0xff;	//  読み出しアドレスの下位バイト
	
	  iic_sd_rcv_fg = 1;		// マスタ送受信処理
	
	  riic_sd_start();			// SCI IIC 送信開始
	
	  while( iic_com_over_fg != 1 ) {     // 通信完了待ち(受信完了待ち)

	  }
	
	  mlx_ram[i] = (( iic_rcv_data[0] << 8 ) | iic_rcv_data[1] );    // 読み出しデータの格納
	
	}
	
	
	
	
}



//   MLX90640 EEPROM(0x2400-0x273F)からデータを読み出し、
//   int mlx_eeprom[832]へ格納する。  0x340 ( = 832 )
//   142[msec]かかる。(at I2C CLK= 400[KHz])　Aug 23
void Read_MLX_EEPROM(void)
{
	uint16_t i;
	uint16_t adrs;
	
	iic_sd_data[0] = ( iic_slave_adrs << 1 );    // 書き込み用 スレーブアドレス
	
	iic_sd_data[3] = ( iic_sd_data[0] | 0x01);   // 読み出し用　スレーブアドレス 
	
	
	for ( i = 0 ; i < 832 ; i++ ) {    // EEPROM 0x2400-0x273F　の読み出し 832 word
	
	  adrs = 0x2400 + i;	
		
	  iic_sd_data[1] = (adrs >> 8 );    //  読み出しアドレスの上位バイト
	  iic_sd_data[2] = adrs & 0xff;	//  読み出しアドレスの下位バイト
	
	  iic_sd_rcv_fg = 1;		// マスタ送受信処理
	
	  riic_sd_start();			// SCI IIC 送信開始
	
	  while( iic_com_over_fg != 1 ) {     // 通信完了待ち(受信完了待ち)

	  }
	
	  mlx_eeprom[i] = (( iic_rcv_data[0] << 8 ) | iic_rcv_data[1] );    // 読み出しデータの格納
	
	}
}

