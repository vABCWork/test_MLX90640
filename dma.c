
#include "iodefine.h"
#include "misratypes.h"
#include "dma.h"
#include "crc_16.h"
#include "sci.h"

// DMA
// チャンネル   起動要因　　 
//   0        RXI1  (割り込みベクタ番号=219）  SCI1 受信データフル　
//   1        TXI1  (割り込みベクタ番号=220）  SCI1 送信データエンプティ
//   2        SPTI0  (割り込みベクタ番号=46）  RSPI0 SPTI0 送信データエンプティ
//


// DMA(チャンネル1) 転送終了割り込み  
// DMAC DMAC0I
//  SCI1 1つの電文の受信終了で発生 (8 byteの受信終了で発生)
#pragma interrupt (Excep_DMAC_DMAC0I(vect=198))
void Excep_DMAC_DMAC0I(void)
{
	uint32_t i;
	
	Init_CRC();		// CRC演算器の初期化
	
	for ( i = 0 ; i < DMA0_SCI_RCV_DATA_NUM ; i++ ) {	// CRC計算
	
		CRC.CRCDIR = rcv_data[i];
	}
	
	if ( CRC.CRCDOR == 0 ) {   // CRCの演算結果 OKの場合
		
		rcv_over = 1;      // 受信完了フラグのセット
		crc_16_err = 0;
		
		LED_RX_PODR = 1;   // 受信 LEDの点灯
	}
	else {   		   // CRCの演算結果 NGの場合
		crc_16_err = 1;
	}
	
	
	IEN(SCI1,RXI1) = 0;			// 受信割り込み禁止
	
	DMA0_SCI_RCV_SET();			// 次の電文受信のため再設定
	
	IR(SCI1,RXI1) = 0;			// RXI 割り込み要求をクリア
	IEN(SCI1,RXI1) = 1;			// 受信割り込み許可
	
}


// DMA転送終了割り込み  
// DMAC DMAC1I
#pragma interrupt (Excep_DMAC_DMAC1I(vect=199))
void Excep_DMAC_DMAC1I(void)
{
     
	SCI1.SCR.BIT.TEIE = 1;  	// TEI割り込み(送信終了割り込み)許可 (全データ送信完了で発生)
}



// DMA(チャンネル2) 転送終了割り込み  
// DMAC DMAC2I

#pragma interrupt (Excep_DMAC_DMAC2I(vect=200))

void Excep_DMAC_DMAC2I(void)
{	
 	RSPI0.SPCR.BIT.SPTIE = 0;       //  RSPI 送信バッファエンプティ割り込み要求の発生を禁止
        
	RSPI0.SPCR2.BIT.SPIIE = 1;	// アイドル割り込み要求の生成を許可
}



//
//  DMA チャンネル0  シリアルデータ受信のための設定
//
void DMA0_SCI_RCV_SET(void)
{
	DMAC0.DMSAR = (void *)&SCI1.RDR;	 // 転送元アドレス SCI1 受信データレジスタ		
        DMAC0.DMDAR = (void *)&rcv_data[0];	 // 転送先アドレス 受信バッファ
        DMAC0.DMCRA = DMA0_SCI_RCV_DATA_NUM; 	  // 転送回数 (受信バイト数　8byte固定)	
	DMAC0.DMCNT.BIT.DTE = 1;    	         // DMAC0 (DMAC チャンネル0) 転送許可
}


//
// DMA チャンネル0 初期設定 (パソコンからのシリアルデータ受信用)
//

void DMA0_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC 停止
	
	DMAC0.DMCNT.BIT.DTE = 0;    // DMAC1 (DMAC チャンネル0) 転送禁止
	
	ICU.DMRSR0 = 219;           // DMA起動要因　受信データフル RXI（割り込みベクタ番号=219）  SCI1 受信割り込みは、DMA0で処理
	
	
	DMAC0.DMAMD.WORD = 0x0080;  // 転送元=アドレス固定、転送先=インクリメント
	DMAC0.DMTMD.WORD = 0x2001;  // ノーマル転送、リピート、ブロック領域なし、8bit転送、周辺モジュールからの割り込みにより開始
	
	DMAC0.DMINT.BIT.DTIE = 1;   // 指定した回数のデータ転送が終了したときの転送終了割り込み要求を許可
	
	IPR(DMAC,DMAC0I) = 9;		// 転送終了割り込みレベル = 9
	IEN(DMAC,DMAC0I) = 1;		// 転送終了割り込み許可
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC 起動

}


//
// DMA チャンネル1 初期設定 (パソコンへのシリアルデータ送信用)
//
void DMA1_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC 停止
	
	DMAC1.DMCNT.BIT.DTE = 0;    // DMAC1 (DMAC チャンネル1) 転送禁止
	
	ICU.DMRSR1 = 220;           // DMA起動要因　TXI1（割り込みベクタ番号=220）  SCI1 送信割り込みは、DMA1で処理
	
	
	DMAC1.DMAMD.WORD = 0x8000;  // 転送元=インクリメント、転送先=アドレス固定
	DMAC1.DMTMD.WORD = 0x2001;  // ノーマル転送、リピート、ブロック領域なし、8bit転送、周辺モジュールからの割り込みにより開始
	
	DMAC1.DMINT.BIT.DTIE = 1;   // 指定した回数のデータ転送が終了したときの転送終了割り込み要求を許可
	
	IPR(DMAC,DMAC1I) = 9;		// 転送終了割り込みレベル = 9
	IEN(DMAC,DMAC1I) = 1;		// 転送終了割り込み許可
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC 起動

	
}

//
// DMA チャンネル2 初期設定 (LCDコントローラ ILI9488へのデータ送信用)
//

void DMA2_ini(void) {
	
	DMAC.DMAST.BIT.DMST =0;     // DMAC 停止
	
	DMAC2.DMCNT.BIT.DTE = 0;    // DMAC2 (DMAC チャンネル2) 転送禁止
			           
				   // 14.2.7 DMAC 起動要因選択レジスタm (DMRSRm) (m = DMAC チャネル番号)
	ICU.DMRSR2 = 46;           // DMA起動要因　SPTI0（割り込みベクタ番号=46） RSPI0 送信バッファエンプティ割り込みは、DMA0で処理
	
	
	DMAC2.DMAMD.WORD = 0x8000;  // 転送元=インクリメント、転送先=アドレス固定
	DMAC2.DMTMD.WORD = 0x2101;  // ノーマル転送、リピート、ブロック領域なし、16bit転送、周辺モジュールからの割り込みにより開始
	
	DMAC2.DMINT.BIT.DTIE = 1;   // 指定した回数のデータ転送が終了したときの転送終了割り込み要求を許可
	
	IPR(DMAC,DMAC2I) = 8;	    // 転送終了割り込みレベル = 8
	IEN(DMAC,DMAC2I) = 1;	    // 転送終了割り込み許可
	
	DMAC.DMAST.BIT.DMST =1;     // DMAC 起動

}

