
//  ���M����LED�p�@�o�̓|�[�g P27�@(�p�\�R���Ƃ̒ʐM�p)
#define LED_TX_PDR      (PORT2.PDR.BIT.B7)   // 1: �o�̓|�[�g�Ɏw��
#define LED_TX_PODR     (PORT2.PODR.BIT.B7)  //   �o�̓f�[�^


//  ��M����LED�p�@�o�̓|�[�g P31 �@(�p�\�R���Ƃ̒ʐM�p)
#define LED_RX_PDR      (PORT3.PDR.BIT.B1)   // 1: �o�̓|�[�g�Ɏw��
#define LED_RX_PODR     (PORT3.PODR.BIT.B1)  //   �o�̓f�[�^


extern volatile uint8_t  rcv_data[8];
extern  volatile uint8_t  rcv_cnt;
extern	volatile uint8_t rcv_over;


void comm_cmd(void);

uint32_t mlx_sci_read_eeprom(void);
uint32_t mlx_sci_read_ram(void);


uint16_t cal_crc_sd_data( uint16_t num );

void initSCI_1(void);

void LED_comm_port_set(void);




