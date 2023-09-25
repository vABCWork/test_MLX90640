

// LCDコントローラ Reset用 出力ポート(PC7)の定義
#define LCD_RESET_PMR     (PORTC.PMR.BIT.B7)   //  汎用入出力ポート
#define LCD_RESET_PDR     (PORTC.PDR.BIT.B7)   //  出力ポートに指定
#define LCD_RESET_PODR    (PORTC.PODR.BIT.B7)  //  出力データ

extern  volatile uint8_t spi_sending_fg;	// 送信中 = 1

void Excep_RSPI0_SPTI0(void);

void RSPI_Init_Port(void);

void RSPI_Init_Reg(void);

void RSPI_SPCMD_0(void);

void rspi_data_send( uint32_t sd_num, uint16_t *data_pt);

void  rspi_data_send_wait(void);