
extern uint16_t rgb666_data_buf[3456];  

void ILI9488_Init(void);

void ILI9488_Reset(void);

void lcd_adrs_set( uint16_t col, uint16_t page, uint16_t col2, uint16_t page2);

void spi_cmd_2C_send( void );

void pixel_write_test_rgb666();
void color_bar_rgb666(void);
void rgb666_data_send(void);

void disp_black_rgb666(void);


