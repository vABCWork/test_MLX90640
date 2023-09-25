#include "iodefine.h"
#include "misratypes.h"
#include "riic_mlx.h"



uint8_t iic_slave_adrs;  // IIC スレーブアドレス  00: 7bitアドレス( 例:100 0000 = 0x40 )

volatile uint8_t iic_rcv_data[16];   // IIC受信データ
volatile uint8_t iic_sd_data[32];    // 送信データ
volatile uint8_t iic_sd_pt;	    // 送信データ位置
volatile uint8_t iic_rcv_pt;         // 受信データ位置

volatile uint8_t  dummy_read_fg;    // 受信割り込みで、ダミーリードするためフラグ

volatile uint8_t  iic_sd_rcv_fg;    // 0:送信のみまたは受信のみの場合,  1:マスタ送受信の場合(= リスタートがある場合)
volatile uint8_t  iic_sd_num;	    // 送信データ数(スレーブアドレスを含む)
volatile uint8_t  iic_rcv_num;      // 受信データ数
volatile uint8_t  iic_com_over_fg;  // 1:STOPコンディションの検出時

				

// RIIC0 EEI0
// 通信エラー/通信イベント発生
//( アービトレーションロスト検出、NACK 検出、タイムアウト検出、スタートコンディション検出、ストップコンディション検出)
//
//   アービトレーションロスト検出、とタイムアウト検出は、使用していない。
//

#pragma interrupt (Excep_RIIC0_EEI0(vect=246))
void Excep_RIIC0_EEI0(void){
	
	if( RIIC0.ICSR2.BIT.START == 1 ) {      // スタート(リスタート)コンディション検出
		RIIC0.ICSR2.BIT.START = 0;	// スタートコンディション検出フラグのクリア
		
	     if ( iic_sd_rcv_fg == 1 ) {	// マスタ送受信の場合(= リスタートがある場合)
		if ( iic_sd_pt == 3) {      //  	コマンド(読み出しレジスタ)送信完了後の、リスタートコンディション発行
			
			RIIC0.ICDRT = iic_sd_data[iic_sd_pt];  // 送信 ( スレーブアドレス(読み出し用)の送信 )
			
			iic_sd_pt++;
			
			// スレーブアドレス(読み出し用)の送信後に、ICCR2.TRS = 0(受信モード)となり、
			// ICSR2.RDRF (受信データフルフラグ)は自動的に“1”(ICDRRレジスタに受信データあり)になる。
			// スレーブアドレス(読み出し用)送信後の、受信割り込みで、ダミーリードするためのフラグを設定
			 
			 dummy_read_fg = 1;    // ダミーリード有効
		  
		 	 RIIC0.ICIER.BIT.TEIE = 0;	// 送信終了割り込み(TEI)要求の禁止
		}
	     }
		
	}
	
	else if ( RIIC0.ICSR2.BIT.STOP == 1 ) {      // STOP 検出
	
	      RIIC0.ICSR2.BIT.STOP = 0;	 //  STOP 検出フラグのクリア	
	      
	     iic_com_over_fg = 1;		// 通信完了
	      
	}
	
	else if ( RIIC0.ICSR2.BIT.NACKF == 1 ) {      // NACK 検出
	        
		RIIC0.ICSR2.BIT.NACKF = 0;	  // NACK 検出フラグのクリア
	        
		RIIC0.ICCR2.BIT.SP = 1;		   // ストップコンディションの発行要求の設定
	}
	
}

// RIIC0 RXI0
// 受信データフル　割り込み
// ICDRRレジスタに受信データあり
#pragma interrupt (Excep_RIIC0_RXI0(vect=247))
void Excep_RIIC0_RXI0(void){
	
	uint8_t dummy;
	
	if ( dummy_read_fg == 1 ) {		// スレーブアドレス(読み出し用)送信後のダミーリード
	
		dummy = RIIC0.ICDRR;		// ダミーリード　(SCLクロックを出力して、受信動作開始)
		dummy_read_fg = 0;
	}
	else { 
		
		iic_rcv_data[iic_rcv_pt] = RIIC0.ICDRR;    // 受信データ読み出し

		iic_rcv_pt++;
		
		
		 if ( iic_rcv_pt < 2 ) {
		     RIIC0.ICMR3.BIT.ACKBT = 0;		// ACK 送信	
		 }
		 else {					// 最終バイトの受信
		     RIIC0.ICMR3.BIT.ACKBT = 1;		// NACK 送信	
		      
		     RIIC0.ICCR2.BIT.SP = 1;		// ストップコンディションの発行要求の設定
		}
	}
	
}

// RIIC0 TXI0
// 送信データエンプティ	割り込み
// ICDRTレジスタに送信データなしの時に、発生
//
//    

#pragma interrupt (Excep_RIIC0_TXI0(vect=248))
void Excep_RIIC0_TXI0(void){
	
	RIIC0.ICDRT = iic_sd_data[iic_sd_pt];  // 送信
	
	iic_sd_pt++;		// 送信位置の更新
	
	if ( iic_sd_rcv_fg == 1 ) {	// マスタ送受信の場合(= リスタートがある場合)
	    if ( iic_sd_pt == 3) {      //  コマンド(読み出しアドレス)送信開始後
		
		RIIC0.ICIER.BIT.TIE = 0;	// 送信データエンプティ割り込み(TXI)要求の禁止
		RIIC0.ICIER.BIT.TEIE = 1;	// 送信終了割り込み(TEI)要求の許可
	    }
	}
	else {				// マスタ送信、マスタ受信の場合
	       if ( (iic_sd_data[0] & 0x01) == 1 ) {  // マスタ受信の場合(MLX90640 では使用しない)
			// スレーブアドレス(読み出し用)の送信後に、ICCR2.TRS = 0(受信モード)となり、
			// ICSR2.RDRF (受信データフルフラグ)は自動的に“1”(ICDRRレジスタに受信データあり)になる。
			// 全データの送信後の、受信割り込みで、ダミーリードするためのフラグを設定
			 
			 dummy_read_fg = 1;    // ダミーリード有効
	       }
	       else {					// マスタ送信の場合
	         if ( iic_sd_pt == iic_sd_num ) {	// 全データの送信完了 
	             RIIC0.ICIER.BIT.TIE = 0;	// 送信データエンプティ割り込み(TXI)要求の禁止
		     RIIC0.ICIER.BIT.TEIE = 1;	// 送信終了割り込み(TEI)要求の許可
	         }
	      }
	}
}

// RIIC0 TEI0
// 送信終了割り込み
//  ICSR2.BIT.TEND = 1で発生 ( ICSR2.BIT.TDRE = 1 の状態で、SCL クロックの9 クロック目の立ち上がりで発生)
#pragma interrupt (Excep_RIIC0_TEI0(vect=249))
void Excep_RIIC0_TEI0(void){
	
	
         RIIC0.ICSR2.BIT.TEND = 0;		//  送信完了フラグのクリア
	
	 if ( iic_sd_rcv_fg == 1 ) {		// マスタ送受信の場合(= リスタートがある場合)
		RIIC0.ICCR2.BIT.RS = 1;		// リスタートコンディションの発行 
	 }
	 
	 else {					// マスタ送信で、全データの送信完了時
	  
	 	RIIC0.ICIER.BIT.TEIE = 0;	// 送信終了割り込み(TEI)要求の禁止
		RIIC0.ICCR2.BIT.SP = 1;	       // ストップコンディションの発行要求の設定
	
	 }	    
	 

}



//  RIIC 送信開始
void riic_sd_start(void)
{
	iic_sd_pt = 0;				 // 送信データ位置　クリア
	iic_rcv_pt = 0;                          // 受信データ位置

	iic_com_over_fg = 0;			// 通信完了フラグのクリア
	
	RIIC0.ICIER.BIT.TIE = 1;		// 送信データエンプティ割り込み(TXI)要求の許可
	
	while(RIIC0.ICCR2.BIT.BBSY == 1){ 	// I2Cバスビジー状態の場合、ループ
	}
	
	RIIC0.ICCR2.BIT.ST = 1;		// スタートコンディションの発行  (マスタ送信の開始)
					// スタートコンディション発行後、ICSR2.TDRE(送信データエンプティフラグ)=1となり、
					//  TXI(送信データエンプティ)割り込み、発生
}


//  I2C(SMBus)インターフェイス の初期化 
// 
//   	PORT16 = SCL
//      PORT17 = SDA
//
//      PCLKB = 32MHz:
//
//      転送速度= 1 / { ( (ICBRH + 1) + (ICBRL + 1) ) / (IIC Φ) + SCLn ライン立ち上がり時間(tr) + SCLn ライン立ち下がり時間(tf) }
//
//       (資料の  29.2.14 I2C バスビットレートHigh レジスタ(ICBRH)　より)
//
//     ( 資料:「 RX23E-Aグループ ユーザーズマニュアル　ハードウェア編」 (R01UH0801JJ0120 Rev.1.20)） 
//


void RIIC0_Init(void)
{
	RIIC0.ICCR1.BIT.ICE = 0;    // RIICは機能停止(SCL,SDA端子非駆動状態)
	RIIC0.ICCR1.BIT.IICRST = 1; // RIICリセット、
	RIIC0.ICCR1.BIT.ICE = 1;    // 内部リセット状態 、SCL0、SDA0端子駆動状態
		
	RIIC0.ICSER.BYTE = 0x00;    // I2Cバスステータス許可レジスタ （マスタ動作のためスレーブ設定は無効)	
	
	
				     // 通信速度 = 400 kbps (オシロ測定値 348 kbps)
  	RIIC0.ICMR1.BIT.CKS = 1;    // RIICの内部基準クロック = 32/2 = 16 MHz　
  	RIIC0.ICBRH.BIT.BRH = 0xF4; // 
	RIIC0.ICBRL.BIT.BRL = 0xF4; // 
	 
	
	RIIC0.ICMR3.BIT.ACKWP = 1;	// ACKBTビットへの書き込み許可		
						
					
					
	RIIC0.ICMR3.BIT.RDRFS = 1;	// RDRFフラグ(受信データフル)セットタイミング
					// 1：RDRF フラグは8 クロック目の立ち上がりで“1” にし、8 クロック目の立ち下がりでSCL0 ラインをLow にホールドします。
					// このSCL0 ラインのLow ホールドはACKBT ビットへの書き込みにより解除されます。
					//この設定のとき、データ受信後アクノリッジビット送出前にSCL0 ラインを自動的にLow にホールドするため、
					// 受信データの内容に応じてACK (ACKBT ビットが“0”) またはNACK (ACKBT ビットが“1”) を送出する処理が可能です。
			
					
	RIIC0.ICMR3.BIT.WAIT = 0;	// WAITなし (9クロック目と1クロック目の間をLowにホールドしない)	
	
	RIIC0.ICMR3.BIT.SMBS = 0;       // I2Cバス選択 				
	
	 
	RIIC0.ICCR1.BIT.IICRST = 0;	 // RIICリセット解除
}




//
//
//  I2C(SMBus)インターフェイス用のポートを設定
// 
//   	PORT16 = SCL
//      PORT17 = SDA
//

void RIIC0_Port_Set(void)
{
	
	MPC.PWPR.BIT.B0WI = 0;      // マルチファンクションピンコントローラ　プロテクト解除
    	MPC.PWPR.BIT.PFSWE = 1;     // PmnPFS ライトプロテクト解除
    
    	MPC.P16PFS.BYTE = 0x0f;     // PORT16 = SCL0
    	MPC.P17PFS.BYTE = 0x0f;     // PORT17 = SDA0
          
    	MPC.PWPR.BYTE = 0x80;      //  PmnPFS ライトプロテクト 設定
  
    	PORT1.PMR.BIT.B6 = 1;     // PORT16:周辺モジュールとして使用
    	PORT1.PMR.BIT.B7 = 1;     // PORT17:      :
}



// RIIC の割り込み用、割り込みコントローラの設定
// 以下を、割り込み処理で行う
//   EEI: 通信エラー/通信イベント (NACK 検出、スタートコンディション検出、ストップコンディション検出)
//　 RXI:　受信データフル
//   TXI:  送信データエンプティ
//   TEI:  送信終了

void RIIC0_Init_interrupt(void)
{
					// 通信エラー/通信イベント 割り込み
	IPR(RIIC0,EEI0) = 10;		// 割り込みレベル = 10　　（15が最高レベル)
	IR(RIIC0,EEI0) = 0;		// 割り込み要求のクリア
	IEN(RIIC0,EEI0) = 1;		// 割り込み許可	
	
					// 受信データフル
	IPR(RIIC0,RXI0) = 10;		// 割り込みレベル = 10　　（15が最高レベル)
	IR(RIIC0,RXI0) = 0;		// 割り込み要求のクリア
	IEN(RIIC0,RXI0) = 1;		// 割り込み許可	
	
					// 送信データエンプティ
	IPR(RIIC0,TXI0) = 10;		// 割り込みレベル = 10　　（15が最高レベル)
	IR(RIIC0,TXI0) = 0;		// 割り込み要求のクリア
	IEN(RIIC0,TXI0) = 1;		// 割り込み許可	
	
					// 送信終了
	IPR(RIIC0,TEI0) = 10;		// 割り込みレベル = 10　　（15が最高レベル)
	IR(RIIC0,TEI0) = 0;		// 割り込み要求のクリア
	IEN(RIIC0,TEI0) = 1;		// 割り込み許可	
	
	
	
	RIIC0.ICIER.BIT.TMOIE = 0;	// タイムアウト割り込み(TMOI)要求の禁止
	RIIC0.ICIER.BIT.ALIE  = 0;   	// アービトレーションロスト割り込み(ALI)要求の禁止
	
	RIIC0.ICIER.BIT.STIE  = 1;	// スタートコンディション検出割り込み(STI)要求の許可
	RIIC0.ICIER.BIT.SPIE  = 1;      // ストップコンディション検出割り込み(SPI)要求の許可
	RIIC0.ICIER.BIT.NAKIE  = 1;	// NACK受信割り込み(NAKI)要求の許可

	RIIC0.ICIER.BIT.RIE = 1;	// 受信データフル割り込み(RXI)要求の許可
	RIIC0.ICIER.BIT.TIE = 0;	// 送信データエンプティ割り込み(TXI)要求の禁止
	RIIC0.ICIER.BIT.TEIE = 0;	// 送信終了割り込み(TEI)要求の禁止
	
}
