/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
 
#define MLX90640_NO_ERROR 0
#define MLX90640_FRAME_DATA_ERROR 8
#define MLX90640_BROKEN_PIXELS_NUM_ERROR 3
#define MLX90640_OUTLIER_PIXELS_NUM_ERROR 4
#define MLX90640_BAD_PIXELS_NUM_ERROR 5
#define MLX90640_ADJACENT_BAD_PIXELS_ERROR 6

#define SCALEALPHA 0.000001

typedef struct {
	
  int16_t kVdd;
  int16_t vdd25;
  float KvPTAT;
  float KtPTAT;
  uint16_t vPTAT25;
  float alphaPTAT;
  int16_t gainEE;
  float tgc;
  float cpKv;
  float cpKta;
  uint8_t resolutionEE;
  uint8_t calibrationModeEE;
  float KsTa;
  float ksTo[5];
  int16_t ct[5];
  uint16_t alpha[768];
  uint8_t alphaScale;
  int16_t offset[768];
  int8_t kta[768];
  uint8_t ktaScale;
  int8_t kv[768];
  uint8_t kvScale;
  float cpAlpha[2];
  int16_t cpOffset[2];
  float ilChessC[3];
  uint16_t brokenPixels[5];
  uint16_t outlierPixels[5];
  
} paramsMLX90640;


extern paramsMLX90640 mlx_para;

extern int16_t mlx_broke_pixel;	
extern int16_t mlx_frame_data_err;
extern int16_t mlx_aux_data_err;


void MLX90640_CalculateTo(uint16_t *frameData, const paramsMLX90640 *params, float emissivity, float tr, float *result);

float MLX90640_GetVdd(uint16_t *frameData, const paramsMLX90640 *params);
float MLX90640_GetTa(uint16_t *frameData, const paramsMLX90640 *params);

int ValidateFrameData(uint16_t *frameData);
int ValidateAuxData(uint16_t *auxData);