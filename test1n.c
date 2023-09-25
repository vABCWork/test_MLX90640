#include	<machine.h>
#include	 "iodefine.h"
#include	 "misratypes.h"
#include	"delay.h"
#include 	"dma.h"
#include	"sci.h"
#include	"timer.h"
#include	"riic_mlx.h"

#include	"mlx90640_iic.h"
#include	"mlx90640_api.h"

#include	 "rspi_9bit.h"
#include	 "ILI9488_9bit_dma.h"
#include	"dsad.h"



void clear_module_stop(void);

void main(void)
{
	uint32_t i;
	
	clear_module_stop();	//  モジュールストップの解除
	
	DMA0_ini();		//  PCへのシリアルデータ受信用のDMA処理　初期設定
	
	DMA0_SCI_RCV_SET();	// 次の電文受信のため再設定
	
	DMA1_ini();           	// PCへのシリアルデータ送信用のDMA処理　初期設定	
	
	initSCI_1();		// SCI1(パソコンとの通信用) 307.2Kbps  
	Init_CRC();		//  CRC演算器の設定 16ビットCRC（X^16 + X^12 + X^5 + 1 ）
	LED_comm_port_set();	//  送信時と受信時のLED出力ポート設定
	
	DMA2_ini();           	// DMA チャンネル2( LCDへのデータ送信用)　初期設定
	RSPI_Init_Port();	// RSPI ポートの初期化  (LCDコントローラ用)   
     	RSPI_Init_Reg();        // SPI レジスタの設定  

     	RSPI_SPCMD_0();	        // SPI 転送フォーマットを設定, SSLA0使用	
	
	ILI9488_Reset();	// LCD のリセット	
	 
	ILI9488_Init();		// LCDの初期化
	
	delay_msec(10);		// LCD(ILI9488)初期化完了待ち
	
//	while(1) {
//	 //   pixel_write_test_rgb666();
	    
//	    color_bar_rgb666();		// 364 msec
	    
//	    delay_msec(10);		// LCD(ILI9488)初期化完了待ち
//	}
	
	
	disp_black_rgb666();		// LCD画面 全体を黒
	color_map_turbo();		// カラーマップ(turbo)の表示 256Wx10H  
	
	
	RIIC0_Port_Set();	//  I2C(SMBus)インターフェイス用のポート設定	
	RIIC0_Init();		//  I2C(SMBus)インターフェイス の初期化
					
	RIIC0_Init_interrupt();	// RIIC0 割り込み許可 
	
	afe_ini();		// AFE(アナログフロントエンド)設定
	
	dsad0_ini();		// DASD0の設定　(熱電対用 4チャンネル)
	dsad1_ini();            // DASD1の設定 (基準接点補償 RTD 100 ohm)
	
	ad_index = 0;		// 各チャンネルのデータ格納位置の初期化
	ad1_index = 0;
	
	Timer10msec_Set();      // タイマ(10msec)作成(CMT0)
     	Timer10msec_Start();    // タイマ(10msec)開始　( DSAD0,DSAD1 オートスキャン開始)
	
	iic_slave_adrs = 0x33;    	//  スレーブアドレス = 0x33 (MLX 90640)
	
	Read_MLX_EEPROM();	// MLX90640  EEPRM(0x2400-0x273F)からデータを読み出し  mlx_eeprom[832]へ格納 　
	
	
	mlx_broke_pixel = MLX90640_ExtractParameters(&mlx_eeprom[0], &mlx_para);	// mlx_paraへ展開
	
	mlx_emissivity = 0.95;		// 放射率 = 0.95 固定
	
	MLX_Set_Refresh_rate();		// MLX90640 リフレッシュレート 4[MHz]設定
	
	
	disp_temp_label();		// ℃,Em,Ta,CH1..等のラベル表示
	
	while(1) {   //  全体の処理時間 537 msec
		
		 IWDT_Refresh();		// ウオッチドックタイマリフレッシュ
	
		 MLX_Get_FrameData();	// RAM(0x0400-0x073F)のデータと(Control register1)と (statusRegister & 0x0001)の読み出し(146 [msec], clock=400[kHz])
	     
	         mlx_frame_data_err = ValidateFrameData(&mlx_ram[0]);  // MLX90640 RAM 0x0400からのデータをチェック
	     
	         mlx_aux_data_err =  ValidateAuxData(&mlx_ram[768]);   // MLX90640 RAM 0x0700(Ta_Vbe)からのデータをチェック
	     
		 mlx_tr = MLX90640_GetTa(&mlx_ram[0], &mlx_para);      // reflected temperature: 屋内で使用する場合は、センサ周囲温度(Ta)を使用
		 
		 //mlx_tr = MLX90640_GetTa(&mlx_ram[0], &mlx_para) - 8.0;   // 屋外で使用する場合は、センサ周囲温度(Ta) - 8.0
		 						       // MLX90640 32x24 IR array driver (Rev.1 - October 31,2022) (page 15)
								       // mlx_tr : reflected temperature defined by the user. 
								       //  If the object emissivity is less than 1, there might be some temperature reflected from the object. 
								       // In order for this to be compensated the user should input this reflected temperature. 
								       // The sensor ambient temperature could be used, but some shift depending on the enclosure might be needed. 
								       // For a MLX90640 in the open air the shift is -8°C.
	 	 
	     
	         MLX90640_CalculateTo(&mlx_ram[0], &mlx_para, mlx_emissivity, mlx_tr, &mlx_to[0]);  // 温度の計算 4.4[msec]
		 
		 
		 mlx_to_min_max();			// 測定温度の最大値と最小値を得る 0.4 msec
		 
		 mlx_to_min_max_disp();			// 最大値と最小値の表示 11 msec
		 mlx_to_center_disp();		        // 中央の値を表示 5.5 msec
		
		 color_map_mlx_to_interpolate();	// 測定温度のカラーマップ表示 (320Wx240H) (双線形補間)	326[msec]
	       
		// color_map_mlx_to();			// 測定温度のカラーマップ表示 (320Wx240H) (補間なし) 211 [msec] 
		
		//color_map_mlx_to_1px();		// 測定温度のカラーマップを表示 (32Wx24H)
		
		Cal_ad_avg();		   // dsad0 各チャンネルの平均値を得る
		Cal_ad1_avg();		   // dsad1 各チャンネルの平均値を得る
		tc_temp_cal();		   // 温度計算

			
		disp_em_temp();		   // Em(放射率)と周囲温度(Ta) 及び 熱電対の各チャンネルの温度表示 44[msec]
		 
		 if ( rcv_over == 1 ) {		// 通信処理 コマンド受信の場合
		    LED_RX_PODR = 0;		// 受信 LEDの消灯  
  		    comm_cmd();			// レスポンス作成、送信
	   	    rcv_over = 0;		// コマンド受信フラグのクリア
		    rcv_cnt  = 0;		//  受信バイト数の初期
		    Init_CRC();			// CRC演算器の初期化
		 }
		 
	     
		// delay_msec(1);		// 1msec待ち 
	}  // while
	

}




// モジュールストップの解除
//
//　 コンペアマッチタイマ(CMT) ユニット0(CMT0, CMT1) 
//   アナログフロントエンド(AFE)
//   24ビットΔ-Σ A/D コンバータ(DSAD0) ユニット0
//   24ビットΔ-Σ A/D コンバータ(DSAD1) ユニット1
//   I2C バスインタフェース(RIICa)
//   シリアルペリフェラルインタフェース0(RSPI)
//   DMA コントローラ(DMACA)
//   シリアルコミュニケーションインタフェース1(SCI1)(パソコンとの通信用)
//   CRC演算器  (パソコンとの通信データ確認用)
//

void clear_module_stop(void)
{
	SYSTEM.PRCR.WORD = 0xA50F;	// クロック発生、消費電力低減機能関連レジスタの書き込み許可	
	
	MSTP(CMT0) = 0;			// コンペアマッチタイマ(CMT) ユニット0(CMT0, CMT1) モジュールストップの解除
	
	MSTP(AFE) = 0;			// アナログフロントエンド(AFE) モジュールストップの解除
	MSTP(DSAD0) = 0;		// 24 ビットΔ-Σ A/D コンバータ(DSAD0) ユニット0 モジュールストップの解除
	MSTP(DSAD1) = 0;		//             :                        ユニット1 
	
	MSTP(RIIC0) = 0;                //  RIIC0モジュールストップ解除 (I2C通信)
	
	MSTP(RSPI0) = 0;		// シリアルペリフェラルインタフェース0 モジュールストップの解除
	MSTP(DMAC) = 0;                //  DMA モジュールストップ解除
	
	MSTP(SCI1) = 0;	        	// SCI1 モジュールストップの解除
	MSTP(CRC) = 0;			// CRC モジュールストップの解除	
	
	
	SYSTEM.PRCR.WORD = 0xA500;	// クロック発生、消費電力低減機能関連レジスタ書き込み禁止
}

