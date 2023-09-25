
#include "iodefine.h"
#include "misratypes.h"
#include "sci.h"
#include "crc_16.h"
#include "mlx90640_iic.h"
#include "thermocouple.h"

//
//  SCI1 シリアル通信(調歩同期)処理
//
// パソコンとの通信　受信用
volatile uint8_t  rcv_data[8];
volatile uint8_t rxdata;
volatile uint8_t rcv_over;
volatile uint8_t  rcv_cnt;

// パソコンとの通信 送信用
volatile uint8_t sd_data[1687];
volatile uint8_t  send_cnt;
volatile uint8_t  send_pt;


//
// SCI1 送信終了割り込み(パソコンとの通信用)
//  (送信はDMAで実施)
//
#pragma interrupt (Excep_SCI1_TEI1(vect=221))
void Excep_SCI1_TEI1(void)
{	 
	SCI1.SCR.BIT.TE = 0;            // 送信禁止
        SCI1.SCR.BIT.TIE = 0;           // 送信割り込み禁止	        
	SCI1.SCR.BIT.TEIE = 0;  	// TEI割り込み(送信終了割り込み)禁止

	LED_TX_PODR = 0;	        // 送信 LEDの消灯
	
 }
 




// コマンド受信の対する、コマンド処理とレスポンス作成処理 
//   ペルチェコントローラー用、MLX90640用
//

void comm_cmd(void){
   
	uint8_t  cmd;
	
	uint32_t sd_cnt;

	if ( crc_16_err == 1 ) {	// 受信文のCRC不一致の場合、パソコン側へ返送しない。パソコン側はタイムアウトする。
				        // アラーム処理
	    return;
	}
	
	
	cmd = rcv_data[0];
	
	sd_cnt = 0;
				    
	if ( cmd == 0x30 ) {	     // MLX EEPROM (0x2400-0x273F) 読出し
	   sd_cnt = mlx_sci_read_eeprom();	
	}	
	
	else if ( cmd == 0x31 ) {     // MLX RAM (0x400-0x73F),EM, 熱電対の温度 読出し
	   sd_cnt = mlx_sci_read_ram();	
	}
	 
	
	DMAC1.DMSAR = (void *)&sd_data[0];	 // 転送元アドレス		
	DMAC1.DMDAR = (void *)&SCI1.TDR;	 // 転送先アドレス TXD12 送信データ

	DMAC1.DMCRA = sd_cnt; 	 	// 転送回数 (送信バイト数)	
	    
	DMAC1.DMCNT.BIT.DTE = 1;    // DMAC1 (DMAC チャンネル1) 転送許可
	
	    			   // 一番最初の送信割り込み(TXI)を発生させる処理。 ( RX23E-A ユーザーズマニュアル　ハードウェア編　28.3.7 シリアルデータの送信（調歩同期式モード）)　
	SCI1.SCR.BIT.TIE = 1;      // 送信割り込み許可
	SCI1.SCR.BIT.TE = 1;	   // 送信許可
	
	LED_TX_PODR = 1;	   // 送信 LEDの点灯
}





// MLX90640の EEPROM　(0x2400-0x273f)読み出しコマンド(0x030)
//
// 受信データ:
//  rcv_data[0];　0x30 (コマンド)
//  rcv_data[1]:  dummy 0
//  rcv_data[2]:  dummy 0
//  rcv_data[3]:  dummy 0
//  rcv_data[4]:  dummy 0
//  rcv_data[5]:  dummy 0
//  rcv_data[6]: CRC(上位バイト側)
//  rcv_data[7]: CRC(下位バイト側)

// 送信データ :
//     sd_data[0] : 0xb0 (コマンドに対するレスポンス)
//     se_data[1] : アドレス(0x2400)のデータ(上位バイト側)
//     sd_data[2] : アドレス(0x2400)のデータ(下位バイト側)
//     sd_data[3] : アドレス(0x2401)のデータ(上位バイト側)
//     sd_data[4] : アドレス(0x2401)のデータ(下位バイト側)
//          :                  :
//          :                  :
//     sd_data[1663]: アドレス(0x273F)のデータ(上位バイト側)
//     sd_data[1664]: アドレス(0x273F)のデータ(下位バイト側)
//     sd_data[1665]: CRC(上位バイト側)
//     sd_data[1666]: CRC(下位バイト側

uint32_t mlx_sci_read_eeprom(void)
{
	uint16_t crc_cd;
	
	uint32_t i;
	uint32_t cnt;
	
	cnt = 1667;			// 送信バイト数
	
	sd_data[0] = 0xb0;	 	// コマンドに対するレスポンス	
	
	for ( i = 0; i < 832 ; i++ ){   // EEPROM 0x2400-0x273F, 832 word = 1664 byte

	   sd_data[i*2 + 1] = (mlx_eeprom[i] >> 8); // 上位バイト側
	   
	   sd_data[i*2 + 2] = mlx_eeprom[i];    // 下位バイト側
	}
	
	
	crc_cd = cal_crc_sd_data( cnt - 2 );   // CRCの計算
	
	sd_data[1665] = crc_cd >> 8;	// CRC上位バイト
	sd_data[1666] = crc_cd;		// CRC下位バイト
	
	return cnt;
	
}


// MLX90640の RAM　(0x400-0x73f), Em, 熱電対の温度 読み出しコマンド(0x031)
//
// 受信データ
//  rcv_data[0];　0x31 (コマンド)
//  rcv_data[1]:  dummy 0
//  rcv_data[2]:  dummy 0
//  rcv_data[3]:  dummy 0
//  rcv_data[4]:  dummy 0
//  rcv_data[5]:  dummy 0
//  rcv_data[6]: CRC(上位バイト側)
//  rcv_data[7]: CRC(下位バイト側)

// 送信データ :
//     sd_data[0] : 0xb1 (コマンドに対するレスポンス)
//     se_data[1] : アドレス(0x400)のデータ(上位バイト側)
//     sd_data[2] : アドレス(0x400)のデータ(下位バイト側)
//     sd_data[3] : アドレス(0x401)のデータ(上位バイト側)
//     sd_data[4] : アドレス(0x401)のデータ(下位バイト側)
//          :                  :
//          :                  :
//     sd_data[1663]: アドレス(0x73F)のデータ(上位バイト側)
//     sd_data[1664]: アドレス(0x73F)のデータ(下位バイト側)
//
//     sd_data[1665]:Control register 1 (0x800d)(上位バイト側)
//     sd_data[1666]:Control register 1 (0x800d)(下位バイト側)
//     sd_data[1667]:Status register (0x8000)(上位バイト側)
//     sd_data[1668]:Status register (0x8000)(下位バイト側)
//
//     sd_data[1669] : 放射率(Em)  (下位バイト側) 100倍した値 (例:Em=0.95ならば950を返す)
//     sd_data[1670] :   :         (上位バイト側)
//     sd_data[1671] : 周囲温度(Ta)(下位バイト側) 10倍した値 (例:Ta=23.5ならば235を返す)
//     se_data[1672] :   :         (上位バイト側)
//     sd_data[1673] : 反射温度(Tr)(下位バイト側) 10倍した値
//     sd_data[1674] :   :         (上位バイト側)
//     sd_data[1675] : 熱電対 CH1温度  (下位バイト側)
//     se_data[1676] :    :            (上位バイト側)
//     sd_data[1677] : 熱電対 CH2温度 (下位バイト側)  
//     sd_data[1678] :    :	    (上位バイト側)
//     sd_data[1679] : 熱電対 CH3温度 (下位バイト側) 
//     se_data[1680] :    :           (上位バイト側)
//     sd_data[1681] : 熱電対 CH4温度 (下位バイト側)  
//     sd_data[1682] :    :　　　　　 (上位バイト側)
//     sd_data[1683] : 基準接点温度(CJT)  (下位バイト側)  
//     se_data[1684] :    :               (上位バイト側)
//
//     sd_data[1685]: CRC(上位バイト側)
//     sd_data[1686]: CRC(下位バイト側



uint32_t mlx_sci_read_ram(void)
{
	int16_t   x_ch1, x_ch2, x_ch3, x_ch4, x_cjt;
	int16_t   x_ta,x_tr;

	uint16_t  x_em;
	uint32_t i;
	uint32_t cnt;
	
	uint16_t crc_cd;
	
	cnt = 1687;			// 送信バイト数
	
	sd_data[0] = 0xb1;	 	// パラメータ書き込みコマンドに対するレスポンス	
	
	for ( i = 0; i < 832 ; i++ ){   // EEPROM 0x2400-0x273F, 832 word = 1664 byte
	
	   sd_data[i*2 + 1] = (mlx_ram[i] >> 8); // 上位バイト側
	   
	   sd_data[i*2 + 2] = mlx_ram[i];    // 下位バイト側
	}
	
	sd_data[1665] =  (mlx_control_reg_1 >> 8);  // 上位バイト側
	sd_data[1666] = mlx_control_reg_1;	    // 下位バイト側
	
	sd_data[1667] =  (mlx_status_reg >> 8);  // 上位バイト側
	sd_data[1668] = mlx_status_reg;	         // 下位バイト側
	
	x_em = mlx_emissivity * 100.0;  // 放射率(Em)の100倍
	sd_data[1669] = x_em;		// Lowバイト側
	sd_data[1670] = x_em >> 8;		// Highバイト側
	
	x_ta = mlx_ta * 10.0;		// 周囲温度(Ta)の10倍
	sd_data[1671] = x_ta;		// Lowバイト側
	sd_data[1672] = x_ta >> 8;		// Highバイト側
	
	x_tr = mlx_tr * 10.0;		// 室内補正用 反射温度(Tr)の10倍
	sd_data[1673] = x_tr;		// Lowバイト側
	sd_data[1674] = x_tr >> 8;		// Highバイト側
	
	x_ch1 = tc_temp[0] * 10.0;	// ch1の10倍
	sd_data[1675] = x_ch1;		// Lowバイト側
	sd_data[1676] = x_ch1 >> 8;	// Highバイト側
	
        x_ch2 = tc_temp[1] * 10.0;	// ch2の10倍
	sd_data[1677] = x_ch2;		// Lowバイト側
	sd_data[1678] = x_ch2 >> 8;	// Highバイト側
	
	x_ch3 = tc_temp[2] * 10.0;	// ch3の10倍
	sd_data[1679] = x_ch3;		// Lowバイト側
	sd_data[1680] = x_ch3 >> 8;	// Highバイト側
	
        x_ch4 = tc_temp[3] * 10.0;	// ch4の10倍
	sd_data[1681] = x_ch4;		// Lowバイト側
	sd_data[1682] = x_ch4 >> 8;	// Highバイト側
	
	x_cjt = cj_temp * 10.0;		// 基準接点温度の10倍
	sd_data[1683] = x_cjt;
	sd_data[1684] = x_cjt >> 8;
	
	crc_cd = cal_crc_sd_data( cnt - 2 );   // CRCの計算
	
	sd_data[1685] = crc_cd >> 8;	// CRC上位バイト
	sd_data[1686] = crc_cd;		// CRC下位バイト
	
	return cnt;
	
}





// 　送信データのCRC計算
//   sd_data[0]からnum個のデータのCRCを計算する。
//
uint16_t    cal_crc_sd_data( uint16_t num )
{
	uint16_t  crc;
	
	uint32_t i;

	Init_CRC();			// CRC演算器の初期化
	
	for ( i = 0 ; i < num ; i++ ) {	// CRC計算
	
		CRC.CRCDIR = sd_data[i];
	}
	
	crc = CRC.CRCDOR;        // CRCの演算結果
	
	return crc;
}



// 
// SCI1 初期設定
//  8bit-non parity-1stop
//  PCLKB = 32MHz
//  TXD1= P16,  RXD1 = P15
//
//  (表28.10 BRRレジスタの設定値NとビットレートBの関係)より
// 
// 1) BDGM=0,ABCS=0 の場合 (SCI1.SEMR.BIT.BGDM = 0 , SCI1.SEMR.BIT.ABCS = 0;)      
//   N = {(32 x 1000000)/((64/2) x B)} - 1
//    N: BRRレジスタの値
//　　B: ボーレート bps
//    
//    B= 1000000 = 1[Mbps] とすると、N = 0
//
//  誤差:
//  誤差 =  { (32 x 1000000) / ( B x (64/2) x (N+1)) - 1 } x 100
//       = 0 %
//
//  2) BDGM= 1,ABCS=0 の場合 (倍速モード)      
//   N = (32 x 1000000/(32/2)xB)-1
//    
//    N = 0 とすると、B= 2 [Mbps]
//
//
//

void initSCI_1(void)
{
	
	MPC.PWPR.BIT.B0WI = 0;   // マルチファンクションピンコントローラ　プロテクト解除
	MPC.PWPR.BIT.PFSWE = 1;  // PmnPFS ライトプロテクト解除
	
	MPC.P30PFS.BYTE = 0x0A;  // P30 = RXD1
	MPC.P26PFS.BYTE = 0x0A;  // P26 = TXD1
	
	
	MPC.PWPR.BYTE = 0x80;    //  PmnPFS ライトプロテクト 設定
			
	PORT3.PMR.BIT.B0 = 1;	// P30 周辺モジュールとして使用
	PORT2.PMR.BIT.B6 = 1;   // P26 周辺モジュールとして使用
		
	
	SCI1.SCR.BYTE = 0;	// 内蔵ボーレートジェネレータ、送受信禁止
	SCI1.SMR.BYTE = 0;	// PCLKB(=32MHz), 調歩同期,8bit,parity なし,1stop
	
	
	SCI1.BRR = 0;			// 1 [Mbps] 
	SCI1.SEMR.BIT.BGDM = 0;         // 0= ボーレートジェネレータから通常の周波数のクロックを出力
	SCI1.SEMR.BIT.ABCS = 0;         // 0= 基本クロック16サイクルの期間が1ビット期間の転送レートになります
	
//	SCI1.BRR = 0;			// 2 [Mbps] 
//	SCI1.SEMR.BIT.BGDM = 1;         // 1= ボーレートジェネレータから2倍の周波数のクロックを出力
//	SCI1.SEMR.BIT.ABCS = 0;         // 0= 基本クロック16サイクルの期間が1ビット期間の転送レートになります
	
	
	
	SCI1.SCR.BIT.TIE = 0;		// TXI割り込み要求を 禁止
	SCI1.SCR.BIT.RIE = 1;		// RXIおよびERI割り込み要求を 許可
	SCI1.SCR.BIT.TE = 0;		// シリアル送信動作を 禁止　（ここで TE=1にすると、一番最初の送信割り込みが発生しない)
	SCI1.SCR.BIT.RE = 1;		// シリアル受信動作を 許可
	
	SCI1.SCR.BIT.MPIE = 0;         // (調歩同期式モードで、SMR.MPビット= 1のとき有効)
	SCI1.SCR.BIT.TEIE = 0;         // TEI割り込み要求を禁止
	SCI1.SCR.BIT.CKE = 0;          // 内蔵ボーレートジェネレータ
	
	
	IEN(SCI1,RXI1) = 1;		// 受信割り込み許可
	
	IEN(SCI1,TXI1) = 1;		// 送信割り込み許可
	
	IPR(SCI1,TEI1) = 12;		// 送信完了 割り込みレベル = 12 （15が最高レベル)
	IEN(SCI1,TEI1) = 1;		// 送信完了割り込み許可
	
	rcv_cnt = 0;			// 受信バイト数の初期化
	Init_CRC();			// CRC演算器の初期化
	
	
}


//  送信時と受信時のLED　出力ポート設定 (パソコンとの通信用)
 void LED_comm_port_set(void)	
 {
					// 送信　表示用LED
	  LED_TX_PDR = 1;		// 出力ポートに指定
	  
	 				// 受信　表示用LED
	  LED_RX_PDR = 1;		// 出力ポートに指定
 }

