
extern  uint16_t mlx_status_reg;
extern  uint16_t mlx_control_reg_1;

extern  uint16_t mlx_ram[834];
extern	uint16_t mlx_eeprom[832];

extern float mlx_emissivity;
extern float mlx_ta;
extern float mlx_tr;
extern float mlx_to[768];
extern float mlx_to_interpolate[100];

extern float mlx_to_max;
extern float mlx_to_min;


void mlx_bilinear_interpolate( uint32_t col, uint32_t row );

void mlx_to_min_max(void);

void MLX_Get_FrameData(void);

void MLX_Set_Refresh_rate(void);

void Read_MLX_Status_Register(void);
void Read_MLX_Control_Register_1(void);

void Write_MLX_Control_Register_1(void);

void Read_MLX_EEPROM(void);
void Read_MLX_RAM(void);