
void disp_num_test(uint8_t index);

void disp_em_temp(void);	
void disp_temp_label(void);
void disp_celsius(void);
 
void disp_s48_24_font( uint8_t dt_index,  uint32_t st_col, uint32_t st_row);

void disp_float_data(float t, uint32_t start_column, uint32_t start_row );
void disp_float_data_em(float t, uint32_t start_column, uint32_t start_row );

void spi_data_send_id(uint8_t index);

void unpack_font_data_rgb666 ( uint32_t len, uint8_t * src_ptr );