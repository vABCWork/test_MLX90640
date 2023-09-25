

// LCD�R���g���[�� Reset�p �o�̓|�[�g(PC7)�̒�`
#define LCD_RESET_PMR     (PORTC.PMR.BIT.B7)   //  �ėp���o�̓|�[�g
#define LCD_RESET_PDR     (PORTC.PDR.BIT.B7)   //  �o�̓|�[�g�Ɏw��
#define LCD_RESET_PODR    (PORTC.PODR.BIT.B7)  //  �o�̓f�[�^

extern  volatile uint8_t spi_sending_fg;	// ���M�� = 1

void Excep_RSPI0_SPTI0(void);

void RSPI_Init_Port(void);

void RSPI_Init_Reg(void);

void RSPI_SPCMD_0(void);

void rspi_data_send( uint32_t sd_num, uint16_t *data_pt);

void  rspi_data_send_wait(void);