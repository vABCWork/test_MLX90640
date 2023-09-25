
//  送信時のLED用　出力ポート P27　(パソコンとの通信用)
#define LED_TX_PDR      (PORT2.PDR.BIT.B7)   // 1: 出力ポートに指定
#define LED_TX_PODR     (PORT2.PODR.BIT.B7)  //   出力データ


//  受信時のLED用　出力ポート P31 　(パソコンとの通信用)
#define LED_RX_PDR      (PORT3.PDR.BIT.B1)   // 1: 出力ポートに指定
#define LED_RX_PODR     (PORT3.PODR.BIT.B1)  //   出力データ


extern volatile uint8_t  rcv_data[8];
extern  volatile uint8_t  rcv_cnt;
extern	volatile uint8_t rcv_over;


void comm_cmd(void);

uint32_t mlx_sci_read_eeprom(void);
uint32_t mlx_sci_read_ram(void);


uint16_t cal_crc_sd_data( uint16_t num );

void initSCI_1(void);

void LED_comm_port_set(void);




