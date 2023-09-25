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
 
#include <mathf.h>
#include "iodefine.h"
#include "misratypes.h"
#include "mlx90640_api.h"
#include "mlx90640_iic.h"



int MLX90640_ExtractParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractVDDParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractPTATParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractGainParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractTgcParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractResolutionParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractKsTaParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractKsToParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractAlphaParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractOffsetParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractKtaPixelParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractKvPixelParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractCPParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
void ExtractCILCParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
int ExtractDeviatingPixels(uint16_t *eeData, paramsMLX90640 *mlx90640);
int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2);  
float GetMedian(float *values, int n);
int IsPixelBad(uint16_t pixel,paramsMLX90640 *params);



// ƒpƒ‰ƒ[ƒ^‚Ì“WŠJ—Ìˆæ
paramsMLX90640 mlx_para;

float mlx_vdd;     // ‹Ÿ‹‹“dˆ³


int16_t mlx_broke_pixel;	//  0 : broken pixel‚ª4‚Â–¢–A‚©‚ÂOutlier pixel‚ª‚S‚Â–¢–
int16_t mlx_frame_data_err;
int16_t mlx_aux_data_err;


float mlx_ptatArt; // for debug  

float alphaCorrR_0;
float alphaCorrR_1;
float alphaCorrR_2;
float alphaCorrR_3;

float mlx_gain;

float  mlx_Sx_0;
float  mlx_Sx_sqrt4;
float  mlx_To_before; 
float  mlx_To_range;

// EEPROM‚Ìƒf[ƒ^‚ğA\‘¢‘Ì‚ÌŠeƒpƒ‰ƒ[ƒ^‚Ö“WŠJ
int MLX90640_ExtractParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int error = 0;
    
    ExtractVDDParameters(eeData, mlx90640);
    ExtractPTATParameters(eeData, mlx90640);
    ExtractGainParameters(eeData, mlx90640);
    ExtractTgcParameters(eeData, mlx90640);
    ExtractResolutionParameters(eeData, mlx90640);
    ExtractKsTaParameters(eeData, mlx90640);
    ExtractKsToParameters(eeData, mlx90640);
    ExtractCPParameters(eeData, mlx90640);
    ExtractAlphaParameters(eeData, mlx90640);
    ExtractOffsetParameters(eeData, mlx90640);
    ExtractKtaPixelParameters(eeData, mlx90640);
    ExtractKvPixelParameters(eeData, mlx90640);
    ExtractCILCParameters(eeData, mlx90640);
    
    error = ExtractDeviatingPixels(eeData, mlx90640);  
    
    return error;

}

//
// ‘ª’è‘ÎÛ•¨‚Ì‰·“xŒvZ@(Chesspattern)
//  
// *framData : ƒZƒ“ƒT‚©‚ç“Ç‚İo‚µ‚½ƒf[ƒ^‚Ìæ“ªƒAƒhƒŒƒX
// *params: paramsMLX90640@‚Ìƒpƒ‰ƒ[ƒ^@:
// emissive: •úË—¦
// tr: º‰·•â³ ( = Ta - 8.0 )
// *result: ŒvZjŒ‹‰Ê‚ÌŠi”[ˆÊ’u
//
// tr: reflected temperature defined by the user. If the object emissivity is less than 1, there might be some
//     temperature reflected from the object. In order for this to be compensated the user should input this
//     reflected temperature. The sensor ambient temperature could be used, but some shift depending on the
//     enclosure might be needed. For a MLX90640 in the open air the shift is -8‹C
//
//     ( MLX90640 32x24 IR array Driver(REVISION 1 - OCTOBER 31, 2022). Page 15 of 21 )
//


void MLX90640_CalculateTo(uint16_t *frameData, const paramsMLX90640 *params, float emissivity, float tr, float *result)
{
    float vdd;
    float ta;
    float ta4;
    float tr4;
    float taTr;
    float gain;
    float irData;
    float alphaCompensated;
    int8_t ilPattern;
    int8_t chessPattern;
    int8_t pattern;
    float Sx;
    float To;
    float alphaCorrR[4];
    int8_t range;
    float ktaScale;
    float kvScale;
    float alphaScale;
    float kta;
    float kv;
    
    int16_t pixelNumber;
    
    
    vdd = MLX90640_GetVdd(frameData, params);
    
    mlx_vdd = vdd;
    
    ta = MLX90640_GetTa(frameData, params);
    mlx_ta = ta;			//  ta(üˆÍ‰·“x)
    
    ta4 = (ta + 273.15);
    ta4 = ta4 * ta4;
    ta4 = ta4 * ta4;
    tr4 = (tr + 273.15);
    tr4 = tr4 * tr4;
    tr4 = tr4 * tr4;
    taTr = tr4 - (tr4-ta4)/emissivity;
    
    ktaScale = powf(2.0, (float)params->ktaScale);
    kvScale = powf(2.0, (float)params->kvScale);
    alphaScale = powf(2.0,(float)params->alphaScale);
    
    alphaCorrR[0] = 1 / (1 + params->ksTo[0] * 40);
    alphaCorrR[1] = 1 ;
    alphaCorrR[2] = (1 + params->ksTo[1] * params->ct[2]);
    alphaCorrR[3] = alphaCorrR[2] * (1 + params->ksTo[2] * (params->ct[3] - params->ct[2]));
    
    alphaCorrR_0 =  alphaCorrR[0];  // for debug
    alphaCorrR_1 =  alphaCorrR[1];
    alphaCorrR_2 =  alphaCorrR[2];
    alphaCorrR_3 =  alphaCorrR[3];
  
    
    gain = (float)params->gainEE / (int16_t)frameData[778];   // ƒQƒCƒ“
    mlx_gain = gain;		  // for debug
    
  
	
    for( pixelNumber = 0; pixelNumber < 768; pixelNumber++)
    {
        ilPattern = pixelNumber / 32 - (pixelNumber / 64) * 2; 
        chessPattern = ilPattern ^ (pixelNumber - (pixelNumber/2)*2); 
      
        pattern = chessPattern;
					  // ‰·“x‚ÍA’¼‹ß‚É‘ª’è‚µ‚½Subpage(0‚Ü‚½‚Í1)‚ÌƒsƒNƒZƒ‹”Ô†‚Ì‰·“x‚¾‚¯ŒvZ‚·‚éB
        if(pattern == frameData[833])     //  frameData[833]: status register b0 : 0=Subpage0, 1=Subpage1
        {    
            irData = (int16_t)frameData[pixelNumber] * gain;	// ƒQƒCƒ“•â³ (11.2.2.5.1)	
            
            kta = params->kta[pixelNumber]/ktaScale;
	    
            kv = params->kv[pixelNumber]/kvScale;
            
	    irData = irData - params->offset[pixelNumber]*(1 + kta*(ta - 25))*(1 + kv*(vdd - 3.3));
            
            irData = irData / emissivity;
            
            alphaCompensated = SCALEALPHA*alphaScale/params->alpha[pixelNumber];
            alphaCompensated = alphaCompensated*(1 + params->KsTa * (ta - 25));
                        
            Sx = alphaCompensated * alphaCompensated * alphaCompensated * (irData + alphaCompensated * taTr);
	    
	    if ( pixelNumber == 0 ) {
	       mlx_Sx_0 = Sx;    // for debug
	    }
	    
            Sx = sqrtf(sqrtf(Sx)) * params->ksTo[1];            
            
	    if ( pixelNumber == 0 ) {
	       mlx_Sx_sqrt4 = Sx;   // for debug
	    }
	    
            To = sqrtf(sqrtf(irData/(alphaCompensated * (1 - params->ksTo[1] * 273.15) + Sx) + taTr)) - 273.15;                     
	     
	    if ( pixelNumber == 0 ) {
	      mlx_To_before = To;   // for debug
	    }
	    
            if(To < params->ct[1])     // To < 0 []
            {
                range = 0;
            }
            else if(To < params->ct[2])   // To < 300 []
            {
                range = 1;            
            }   
            else if(To < params->ct[3])  // To < 500 []
            {
                range = 2;            
            }
            else
            {
                range = 3;            
            }      
            
            To = sqrtf(sqrtf(irData / (alphaCompensated * alphaCorrR[range] * (1 + params->ksTo[range] * (To - params->ct[range]))) + taTr)) - 273.15;
                 
	     
	    if ( pixelNumber == 0 ) {
	      mlx_To_range = To;
	    }
	    
            result[pixelNumber] = To;
        }
    }
}

// •ª‰ğ”\•â³‚Æ‹Ÿ‹‹“dˆ³(Vdd)‚ğ“¾‚é
// 11.2.2.1. Resolution restore
// 11.2.2.2. Supply voltage value calculation (common for all pixels)@
//  
// ‰‰Z®:
//   1) resolutionCorrection = 2^(resolutionEE) / 2^(resolutionRAM)
//         resolutionEE ‚ÍAmlx90640.resolutonEE (Extract_Resolution()‚Å“ü‚ê‚Ä‚¢‚é)
//         resoltionRAM‚ÍA Control register1 ‚Ì b11,b10
//         frameData[832] = Control register1 ( GetFrameData()‚Å“ü‚ê‚Ä‚¢‚é)
//      ADC‚Ìresolution‚ªƒfƒtƒHƒ‹ƒg(18bit)‚Å‚ ‚ê‚ÎAresolutonCorrection = 1;
//
//   2) Vdd = (resolutionCorrection * VDDpix - Vdd25) / Kvdd + Vdd0
//
// ERAM
//  id  adrs  b15 b14 b13 b12 b11 b10  b9 b8 b7 b6 b5 b4 b3 b2 b1 b0
// 810  0x72a < S ---------------  VDDpix  ------------------------->    
//
// EEEPROM
// id  adrs     b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 51 [0x2433]:< S -------- kVdd_EE  --------><S --- vdd25_EE ------> 

float MLX90640_GetVdd(uint16_t *frameData, const paramsMLX90640 *params)
{
    float vdd;
    float resolutionCorrection;

    int16_t resolutionRAM;    
    
    vdd = frameData[810];
    if(vdd > 32767)
    {
        vdd = vdd - 65536;
    }
    resolutionRAM = (frameData[832] & 0x0C00) >> 10;
    
    resolutionCorrection = powf(2.0, (float)params->resolutionEE) / powf(2.0, (float)resolutionRAM);
  
    vdd = (resolutionCorrection * vdd - params->vdd25) / params->kVdd + 3.3;
    
    return vdd;
}



// 
//  MLX90640 ƒZƒ“ƒT‚ÌüˆÍ‰·“x(Ta)
//  (11.2.2.3. Ambient temperature calculation (common for all pixels))
//
//  Ta = (Vptat_art/(1 + Kvptat * (mlx_vdd - 3.3)) - Vptat25) / Ktptat  + 25 []
//
// ERAM
// id   adrs  b15 b14 b13 b12 b11 b10  b9 b8 b7 b6 b5 b4 b3 b2 b1 b0
// 768 0x700 < S ---------------  Ta_Vbe (Vbe) -------------------->
// 800 0x720 < S ---------------  Ta_PTAT(Vptat)-T------------------>
//
// EEEPROM
// id  adrs  b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 16 0x2410:<-alphaPTAT_EE -><------------><-----------><--------->
//  
// 49 0x2431:<-S ------------ vPTAT25_EE -------------------------->
//  
// 50 0x2432:<-S ----- KvPTAT_EE---><-S -------------KtPTAT_EE ----->
//
// EVptat_art = Vptat / (Vptat * Alpha_ptat + Vbe ) * 2^18
//             = Ta_PTAT / ( Ta_PTAT * Alpha_ptat + Ta_Vbe) * 2^18
// 
//              Alpha_ptat = (EEPROM(0x2410) & 0xf000)/16384 + 8;
//
// EKvptat  = Kvptat_EE / 2^12
//            Kvptat_EE:EEPROM(0x2432)‚Ìb15-b10   (b15:•„†bit) 
//
// Emlx_vdd :vdd_cal()‚æ‚è 
//      
// EVptat25 : EEPROM(0x2431)‚Ìb15-b0   (b15:•„†bit)
// 
// EKtptat =  Ktptat_EE / 2^3
//           Ktptat_EE:EEPROM(0x2432)‚Ìb9-b0   (b9:•„†bit) 
//@
 
float MLX90640_GetTa(uint16_t *frameData, const paramsMLX90640 *params)
{
    int16_t ptat;
    float ptatArt;
    float vdd;
    float ta;
    
    vdd = MLX90640_GetVdd(frameData, params);
    
    ptat = (int16_t)frameData[800];
    
    ptatArt = (ptat / (ptat * params->alphaPTAT + (int16_t)frameData[768])) * powf(2.0,18.0);
    
    mlx_ptatArt = ptatArt;	// for debug
    
    ta = (ptatArt / (1 + params->KvPTAT * (vdd - 3.3)) - params->vPTAT25);
    
    ta = ta / params->KtPTAT + 25.0f;
    
    return ta;
}




//
// ƒtƒŒ[ƒ€ƒf[ƒ^‚ª 0x7fff ‚©‚Â
// MLX90640_LINE_SIZE 32
// frameData[833] = 0: subpage 0 measured.
//                = 1: subpage 1 measured.

int ValidateFrameData(uint16_t *frameData)
{
    int i;
    uint8_t line = 0;
    
    for( i = 0; i < 768; i+=32)
    {
        if((frameData[i] == 0x7FFF) && (line%2 == frameData[833])) return -MLX90640_FRAME_DATA_ERROR;
        
	line = line + 1;
    }    
        
    return MLX90640_NO_ERROR;    
}


// w’è‚µ‚½ƒAƒhƒŒƒX‚Ì’l‚ªA0x7fff‚Ìê‡AƒGƒ‰[‚Æ‚·‚éB
// auxData[0] : ƒAƒhƒŒƒX 0x0700‚Ì’l
// 
int ValidateAuxData(uint16_t *auxData)
{
     int i;
	
    if(auxData[0] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;    
    
    for(i=8; i<19; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    for(i=20; i<23; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    for(i=24; i<33; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    for(i=40; i<51; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    for(i=52; i<55; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    for(i=56; i<64; i++)
    {
        if(auxData[i] == 0x7FFF) return -MLX90640_FRAME_DATA_ERROR;
    }
    
    return MLX90640_NO_ERROR;
    
}

// ‹Ÿ‹‹“dˆ³ Vdd‚ÌŒvZ—pƒpƒ‰ƒ[ƒ^‚Ìæ‚èo‚µ‚ÆA‰‰ZB(11.1.1. Restoring the VDD sensor parameters)
//@Kvdd ‚Æ vdd25‚ğ“¾‚éB
//@‰‰Z®:
//   Kvdd  =  Kvdd_EE * 2^(5)
//         =  Kvdd_EE * 32
//
//   vdd25 = (vdd25_EE - 256 )*2^(5) - 2^(13) 
//         = (vdd25_EE - 256 )*32 - 8192 
//
//@EEPROM:
//   Kvdd_EE: EEPROM(0x2433)‚Ì b15-b8 (b15:•„†bit) 
//  vdd25_EE: EEPROM(0x2433)‚Ì b7-b1 (b7:•„†bit)
//
// id  adrs     b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 51 [0x2433]:< S -------- Kvdd_EE  --------><S --- vdd25_EE ------>               

void ExtractVDDParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int8_t kVdd;
    int16_t vdd25;
    
    kVdd = (eeData[51] & 0xff00) >> 8;

    vdd25 = (eeData[51]) & 0x00ff;
    vdd25 = ((vdd25 - 256) << 5) - 8192;
    
    mlx90640->kVdd = 32 * kVdd;
    mlx90640->vdd25 = vdd25; 
}


//
//  KvPTAT, KtPTAT, vPTAT25, alphaPTAT ‚ğ“¾‚é
// ‰‰Z®:
//   KvPTAT = KvPTAT_EE / 2^12 = KvPTAT_EE / 4096
//   KtPTAT = KtPTAT_EE / 2^3  = KtPTAT_EE / 8
//
//@EEPROM:
//  vPTAT25_EE :EEPROM(0x2431)‚Ìb15-b0  (b15:•„†bit)
//  KvPTAT_EE  :EEPROM(0x2432)‚Ìb15-b10 (b15:•„†bit) 
//  KtPTAT_EE  :EEPROM(0x2432)‚Ìb9-b0   (b9:•„†bit) 
// alphaPTAT_EE:EEPROM(0x2410)‚Ìb15-b12    
//
// id  adrs  b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 16 0x2410:<-alphaPTAT_EE -><------------><-----------><--------->
//  
// 49 0x2431:<-S ------------ vPTAT25_EE -------------------------->
//  
// 50 0x2432:<-S ----- KvPTAT_EE--><-S -------------KtPTAT_EE ----->
//

void ExtractPTATParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    float KvPTAT;
    float KtPTAT;
    int16_t vPTAT25;
    float alphaPTAT;
    
    KvPTAT = (eeData[50] & 0xfc00) >> 10;
    
    if(KvPTAT > 31)
    {
        KvPTAT = KvPTAT - 64;
    }
    KvPTAT = KvPTAT/4096;
    
    KtPTAT = eeData[50] & 0x03ff;
    
    if(KtPTAT > 511)
    {
        KtPTAT = KtPTAT - 1024;
    }
    KtPTAT = KtPTAT/8;
    
    vPTAT25 = eeData[49];
    
    alphaPTAT = (eeData[16] & 0xf000) / 16384 + 8.0f;
    
    mlx90640->KvPTAT = KvPTAT;
    mlx90640->KtPTAT = KtPTAT;    
    mlx90640->vPTAT25 = vPTAT25;
    mlx90640->alphaPTAT = alphaPTAT;   
}



// ƒQƒCƒ“‚Ìæ‚èo‚µ (11.1.7. Restoring the GAIN coefficient (common for all pixels)
//
//@EEPROM:
//  Gain_EE :EEPROM(0x2430)‚Ìb15-b0  (b15:•„†bit)
//
// id  adrs  b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 48 0x2430:<-S ------------------Gain_EE ---------------------->
//

void ExtractGainParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    mlx90640->gainEE =  eeData[48];;    
}



// tgc‚Ìæ‚èo‚µ ( 11.1.16. Restoring the TGC coefficient)
//
// ‰‰Z®:
//  float tgc = tgc_EE / 2^5 = tgc_EE/32
//@EEPROM:
//  tgc_EE :EEPROM(0x243c)‚Ìb7-b0  (b7:•„†bit)
//
// id adrs   b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 60 0x243c:<-S  -------- KsTa_EE  ------><-S ------tgc_EE ------>
//
//  MLX90640ESF-BAx-000-TU ‚Ìê‡‚Íí‚É 0
// 11.1.16. Restoring the TGC coefficient)
//  NOTE 1: In a MLX90640ESF?BAx?000-TU device, the TGC coefficient is set to 0 and must not be changed.
//

void ExtractTgcParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
	
   mlx90640->tgc = (int8_t)( eeData[60] & 0x00ff ) / 32.0f;
	

}



// ResolutionEEæ‚èo‚µ ( 11.1.17. Restoring the resolution control coefficient)
//
// ‰‰Z®:
//     resolutionEE =  ResoEE / 2^12 = ResoEE/ 4096
//@EEPROM:
//       ResoEE  :EEPROM(0x2438)‚Ìb13-b12 (*1)
//
// id  adrs   b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 56 0x2438:<--MLX--><ResoEE><-Kv_scale->< Kta_scale1>< Kta_scale2>
//                      

void ExtractResolutionParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    uint8_t resolutionEE;
    
    resolutionEE = (eeData[56] & 0x3000) >> 12;    
    
    mlx90640->resolutionEE = resolutionEE;
}


// KsTaæ‚èo‚µ ( 11.2.2.8. Normalizing to sensitivity )
//
//@EEPROM:
//       Ksta_EE  :EEPROM(0x243C)‚Ìb15-b8 (*1)
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 60 0x243c:<-S  -------- KsTa_EE  ------><-S ------tgc_EE ------>
//
void ExtractKsTaParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{   
    mlx90640->KsTa = (int8_t)((eeData[60] & 0xff00) >> 8) / 8192.0f;
}



// ct[],KsTo[] æ‚èo‚µ (11.1.10. Restoring the KsTo coefficient (common for all pixels) )
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3 b2 b1 b0   
// 61 0x243d: <---- KsTo range2 ---------><---- KsTo range1 ------->
// 62 0x243e: <---- KsTo range4 ---------><---- KsTo range3 ------->
// 63 0x243f: <-MLX-><-step-><----CT4 ---><-----CT3---><-KsToScale->

void ExtractKsToParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int32_t KsToScale;
    int8_t step;
    
    step = ((eeData[63] & 0x3000) >> 12) * 10;
    
    mlx90640->ct[0] = -40;
    mlx90640->ct[1] = 0;
    mlx90640->ct[2] = (eeData[63] & 0x00f0) >> 4;
    mlx90640->ct[3] = (eeData[63] & 0x0f00) >> 8;
    
    mlx90640->ct[2] = mlx90640->ct[2]*step;
    mlx90640->ct[3] = mlx90640->ct[2] + mlx90640->ct[3]*step;
    mlx90640->ct[4] = 400;
    
    KsToScale = (eeData[63] & 0x000f) + 8;
    KsToScale = 1 << KsToScale;
    
    mlx90640->ksTo[0] = (int8_t)(eeData[61] & 0x00ff) / (float)KsToScale;
    mlx90640->ksTo[1] = (int8_t)((eeData[61] & 0xff00) >> 8 ) / (float)KsToScale;
    mlx90640->ksTo[2] = (int8_t)(eeData[62] & 0x00ff) / (float)KsToScale;
    mlx90640->ksTo[3] = (int8_t)((eeData[62] & 0xff00) >> 8 ) / (float)KsToScale;
    mlx90640->ksTo[4] = -0.0002;
}



//
//  ŠeƒsƒNƒZƒ‹–ˆ‚Ì ƒ¿’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8  b7 b6  b5  b4  b3  b2  b1    b0   
// 32 0x2420:<-Alpha scale -><Scale ACC row><Scale ACC col><Scale ACC remnand>
// 33 0x2421:<----------------  Pix sensitivity average -------------------->
// 34 0x2422:<} Acc row 4  >< }Acc row 3  ><} Acc row 2  >< }Acc row 1  >
// 35 0x2423:<} Acc row 8  >< }Acc row 7  ><} Acc row 6  >< }Acc row 5  >
// 36 0x2424:<} Acc row 12 >< }Acc row 11 ><} Acc row 10 >< }Acc row 9  >
// 37 0x2425:<} Acc row 16 >< }Acc row 15 ><} Acc row 14 >< }Acc row 13  >
// 38 0x2426:<} Acc row 20 >< }Acc row 19 ><} Acc row 18 >< }Acc row 17 >
// 39 0x2427:<} Acc row 24 >< }Acc row 23 ><} Acc row 22 >< }Acc row 21  >
// 40 0x2428:<} Acc col 4  >< }Acc col 3  ><} Acc col 2  >< }Acc col 1  >
// 41 0x2429:<} Acc col 8  >< }Acc col 7  ><} Acc col 6  >< }Acc col 5  >
// 42 0x242A:<} Acc col 12 >< }Acc col 11 ><} Acc col 10 >< }Acc col 9  >
// 43 0x242B:<} Acc col 16 >< }Acc col 15 ><} Acc col 14 >< }Acc col 13  >
// 44 0x242C:<} Acc col 20 >< }Acc col 19 ><} Acc col 18 >< }Acc col 17 >
// 45 0x242D:<} Acc col 24 >< }Acc col 23 ><} Acc col 22 >< }Acc col 21  >
// 46 0x242E:<} Acc col 28 >< }Acc col 27 ><} Acc col 26 >< }Acc col 25 >
// 47 0x242F:<} Acc col 32 >< }Acc col 31 ><} Acc col 30 >< }Acc col 29  >
//
// EŠeƒsƒNƒZƒ‹‚Ìƒ¿’l
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3  b2  b1    b0   
// 64 0x2440: <-}Offset pixel(1,1) -><-ƒ¿ pixel(1,1)-><}Kta(1,1)><Outlier>
// 65 0x2441: <-}Offset pixel(1,2) -><-ƒ¿ pixel(1,2)-><}Kta(1,2)><Outlier>
//                          :
//831 0x273F:<-}Offset pixel(24,32)-><ƒ¿ pixel(24,32)-><}Kta(24,42)><Outlier>
//
// alphaScale ‚ÌÅ‘å’l‚ÍA45
// 2^45 = 3.51843 E+13
void ExtractAlphaParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int i;
    int j;
    int accRow[24];
    int accColumn[32];
    int p = 0;
    int alphaRef;
    uint8_t alphaScale;
    uint8_t accRowScale;
    uint8_t accColumnScale;
    uint8_t accRemScale;
    float alphaTemp[768];
    float temp;
    float f_alphaScale;
    

    accRemScale = eeData[32] & 0x000f;
    accColumnScale = (eeData[32] & 0x00f0) >> 4;
    accRowScale = (eeData[32] & 0x0f00) >> 8;
    alphaScale = ((eeData[32] & 0xf000) >> 12) + 30;
    alphaRef = eeData[33];
    
    for(i = 0; i < 6; i++)
    {
        p = i * 4;
        accRow[p + 0] = (eeData[34 + i] & 0x000f);
        accRow[p + 1] = (eeData[34 + i] & 0x00f0) >> 4;
        accRow[p + 2] = (eeData[34 + i] & 0x0f00) >> 8;
        accRow[p + 3] = (eeData[34 + i] & 0xf000) >> 12;
    }
    
    for(i = 0; i < 24; i++)
    {
        if (accRow[i] > 7)
        {
            accRow[i] = accRow[i] - 16;
        }
    }
    
    for(i = 0; i < 8; i++)
    {
        p = i * 4;
        accColumn[p + 0] = (eeData[40 + i]) & 0x000f;
        accColumn[p + 1] = ((eeData[40 + i]) & 0x00f0) >> 4;
        accColumn[p + 2] = ((eeData[40 + i]) & 0x0f00) >> 8;
        accColumn[p + 3] = ((eeData[40 + i]) & 0xf000) >> 12;
    }
    
    for(i = 0; i < 32; i++)
    {
        if (accColumn[i] > 7)
        {
            accColumn[i] = accColumn[i] - 16;
        }
    }

    f_alphaScale = powf(2.0, (float)alphaScale); 
    
    for(i = 0; i < 24; i++)
    {
        for(j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
	    
            alphaTemp[p] = (eeData[64 + p] & 0x03f0) >> 4;
            
	    if (alphaTemp[p] > 31)
            {
                alphaTemp[p] = alphaTemp[p] - 64;
            }
            alphaTemp[p] = alphaTemp[p]*(1 << accRemScale);
            alphaTemp[p] = (alphaRef + (accRow[i] << accRowScale) + (accColumn[j] << accColumnScale) + alphaTemp[p]);
            
	    alphaTemp[p] = alphaTemp[p] / f_alphaScale; 
	    
            alphaTemp[p] = alphaTemp[p] - mlx90640->tgc * (mlx90640->cpAlpha[0] + mlx90640->cpAlpha[1])/2;
            alphaTemp[p] = SCALEALPHA / alphaTemp[p];
        }
    }
    
    temp = alphaTemp[0];       // alphaTemp[i]‚ÌÅ‘å’l‚ğ temp‚Æ‚·‚éˆ—
    for(i = 1; i < 768; i++)
    {
        if (alphaTemp[i] > temp)
        {
            temp = alphaTemp[i];
        }
    }
    
    alphaScale = 0;
    while(temp < 32768 )
    {
        temp = temp*2;
        alphaScale = alphaScale + 1;
    } 
    
    for(i = 0; i < 768; i++)
    {
        temp = alphaTemp[i] * powf(2.0, (float)alphaScale);        
        mlx90640->alpha[i] = (temp + 0.5);        
        
    } 
    
    mlx90640->alphaScale = alphaScale;      
   
}


//  
//  ŠeƒsƒNƒZƒ‹–ˆ‚Ì offset’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
// 11.2.2.5.2 Offset calculation   
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8  b7 b6  b5  b4  b3  b2  b1    b0   
// 16 0x2410:<-Alpha PTAT  -><Scale Occ row><Scale Occ col><Scale Occ remnand>
// 17 0x2411:<---------------- } Pix os average  -------------------------->
// 18 0x2412:<} Occ row 4  >< }Occ row 3  ><} Occ row 2  >< }Occ row 1  >
// 19 0x2413:<} Occ row 8  >< }Occ row 7  ><} Occ row 6  >< }Occ row 5  >
// 20 0x2414:<} Occ row 12 >< }Occ row 11 ><} Occ row 10 >< }Occ row 9  >
// 21 0x2415:<} Occ row 16 >< }Occ row 15 ><} Occ row 14 >< }Occ row 13  >
// 22 0x2416:<} Occ row 20 >< }Occ row 19 ><} Occ row 18 >< }Occ row 17 >
// 23 0x2417:<} Occ row 24 >< }Occ row 23 ><} Occ row 22 >< }Occ row 21  >
// 24 0x2418:<} Occ col 4  >< }Occ col 3  ><} Occ col 2  >< }Occ col 1  >
// 25 0x2419:<} Occ col 8  >< }Occ col 7  ><} Occ col 6  >< }Occ col 5  >
// 26 0x241A:<} Occ col 12 >< }Occ col 11 ><} Occ col 10 >< }Occ col 9  >
// 27 0x241B:<} Occ col 16 >< }Occ col 15 ><} Occ col 14 >< }Occ col 13  >
// 28 0x241C:<} Occ col 20 >< }Occ col 19 ><} Occ col 18 >< }Occ col 17 >
// 29 0x241D:<} Occ col 24 >< }Occ col 23 ><} Occ col 22 >< }Occ col 21  >
// 30 0x241E:<} Occ col 28 >< }Occ col 27 ><} Occ col 26 >< }Occ col 25 >
// 31 0x241F:<} Occ col 32 >< }Occ col 31 ><} Occ col 30 >< }Occ col 29  >
//
// EŠeƒsƒNƒZƒ‹‚Ì Offset’l
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3  b2  b1    b0   
// 64 0x2440: <-}Offset pixel(1,1) -><-ƒ¿ pixel(1,1)-><}Kta(1,1)><Outlier>
// 65 0x2441: <-}Offset pixel(1,2) -><-ƒ¿ pixel(1,2)-><}Kta(1,2)><Outlier>
//                          :
//831 0x273F:<-}Offset pixel(24,32)-><ƒ¿ pixel(24,32)-><}Kta(24,42)><Outlier>
//------------------------------------------------------------------------------

void ExtractOffsetParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{ 
    int i;
    int j;
    int occRow[24];
    int occColumn[32];
    int p = 0;
    int16_t offsetRef;
    uint8_t occRowScale;
    uint8_t occColumnScale;
    uint8_t occRemScale;
    

    occRemScale = eeData[16] & 0x000f;
    occColumnScale = (eeData[16] & 0x00f0) >> 4;
    occRowScale = (eeData[16] & 0x0f00) >> 8;
    offsetRef = (int16_t)eeData[17];
        
    for(i = 0; i < 6; i++)
    {
        p = i * 4;
        occRow[p + 0] = eeData[18 + i] & 0x000f;
        occRow[p + 1] = (eeData[18 + i] & 0x00f0) >> 4;
        occRow[p + 2] = (eeData[18 + i] & 0x0f00) >> 8;
        occRow[p + 3] = (eeData[18 + i] & 0xf000) >> 12;
    }
    
    for(i = 0; i < 24 ; i++)
    {
        if (occRow[i] > 7)
        {
            occRow[i] = occRow[i] - 16;
        }
    }
    
    for(i = 0; i < 8; i++)
    {
        p = i * 4;
        occColumn[p + 0] = eeData[24 + i] & 0x000f;
        occColumn[p + 1] = (eeData[24 + i] & 0x00f0) >> 4;
        occColumn[p + 2] = (eeData[24 + i] & 0x0f00) >> 8;
        occColumn[p + 3] = (eeData[24 + i] & 0xf000) >> 12;
    }
    
    for(i = 0; i < 32; i ++)
    {
        if (occColumn[i] > 7)
        {
            occColumn[i] = occColumn[i] - 16;
        }
    }

    for(i = 0; i < 24; i++)
    {
        for(j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            mlx90640->offset[p] = (eeData[64 + p] & 0xfc00) >> 10;
            if (mlx90640->offset[p] > 31)
            {
                mlx90640->offset[p] = mlx90640->offset[p] - 64;
            }
            mlx90640->offset[p] = mlx90640->offset[p]*(1 << occRemScale);
            mlx90640->offset[p] = (offsetRef + (occRow[i] << occRowScale) + (occColumn[j] << occColumnScale) + mlx90640->offset[p]);
        }
    }
}


//
//  ŠeƒsƒNƒZƒ‹–ˆ‚Ì Kta’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8  b7 b6  b5  b4  b3  b2  b1   b0   
// 54 0x2436:<-- }Kta_avg_RowOdd-ColOdd --><-- }Kta_avg_RowEven-ColOdd  ->
// 55 0x2437:<-- }Kta_avg_RowOdd-ColEven -><-- }Kta_avg_RowEven-ColEven ->                    
// 56 0x2438:< MLX  ><Res cali>< Kv scale  >< Kta_scale_1 >< Kta_scale_2   >
//  
// EŠeƒsƒNƒZƒ‹‚Ì Kta’l
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8 b7 b6 b5 b4 b3  b2  b1    b0   
// 64 0x2440: <-}Offset pixel(1,1) -><-ƒ¿ pixel(1,1)-><}Kta(1,1)><Outlier>
// 65 0x2441: <-}Offset pixel(1,2) -><-ƒ¿ pixel(1,2)-><}Kta(1,2)><Outlier>
//                          :
//831 0x273F:<-}Offset pixel(24,32)-><ƒ¿ pixel(24,32)-><}Kta(24,42)><Outlier>

void ExtractKtaPixelParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int i;
    int j;
    int p = 0;
    int8_t KtaRC[4];
    uint8_t ktaScale1;
    uint8_t ktaScale2;
    uint8_t split;
    float ktaTemp[768];
    float temp;
    
    KtaRC[0] = (int8_t)((eeData[54] & 0xff00) >> 8);
    KtaRC[2] = (int8_t)( eeData[54] & 0x00ff);
    KtaRC[1] = (int8_t)((eeData[55] & 0xff00) >> 8);
    KtaRC[3] = (int8_t)( eeData[55] & 0x00ff);
      
    ktaScale1 = ((eeData[56] & 0x00f0) >> 4) + 8;
    ktaScale2 =  eeData[56] & 0x000f;

    
    
     for( i = 0; i < 24; i++)
    {
        for( j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            split = 2*(p/32 - (p/64)*2) + p%2;
            ktaTemp[p] = (eeData[64 + p] & 0x000E) >> 1;
            if (ktaTemp[p] > 3)
            {
                ktaTemp[p] = ktaTemp[p] - 8;
            }
            ktaTemp[p] = ktaTemp[p] * (1 << ktaScale2);
            
	    ktaTemp[p] = KtaRC[split] + ktaTemp[p];
	    
            ktaTemp[p] = ktaTemp[p] / powf(2.0, (float)ktaScale1);
            
        }
    }
    
    temp = fabsf(ktaTemp[0]); 
        
    for( i = 1; i < 768; i++)
    {
        if (fabsf(ktaTemp[i]) > temp)
        {
            temp = fabsf(ktaTemp[i]);
        }
    }
    
    ktaScale1 = 0;
    while(temp < 64)
    {
        temp = temp*2;
        ktaScale1 = ktaScale1 + 1;
    }    
    
    for( i = 0; i < 768; i++)
    {
        temp = ktaTemp[i] * powf(2.0, (float)ktaScale1);
        if (temp < 0.0)
        {
            mlx90640->kta[i] = (temp - 0.5);
        }
        else
        {
            mlx90640->kta[i] = (temp + 0.5);
        }        
        
    } 
    
    mlx90640->ktaScale = ktaScale1;           
}


//
//  ŠeƒsƒNƒZƒ‹–ˆ‚Ì Kv’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
//
// id adrs    b15   b14  b13    b12   b11  b10    b9    b8   b7     b6    b5      b4   b3   b2   b1     b0   
// 52 0x2434:<}Kvavg_rowOdd_colOdd><}Kvavg_rowEven_colOdd><}Kvavg-rowOdd_colEven><}Kvavg-rowEven_colEven> 
//      :
// 56 0x2438:<-- MLX --><-Res cali-><---- Kv scale --------><------ Kta_scale_1 ----><------ Kta_scale_2 --->
//

void ExtractKvPixelParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    int i;
    int j;
    int p = 0;
    int8_t KvT[4];
    int8_t KvRoCo;
    int8_t KvRoCe;
    int8_t KvReCo;
    int8_t KvReCe;
    uint8_t kvScale;
    uint8_t split;
    float kvTemp[768];
    float temp;

    KvRoCo = (eeData[52] & 0xf000) >> 12;
    if (KvRoCo > 7)
    {
        KvRoCo = KvRoCo - 16;
    }
    KvT[0] = KvRoCo;
    
    KvReCo = (eeData[52] & 0x0f00) >> 8;
    if (KvReCo > 7)
    {
        KvReCo = KvReCo - 16;
    }
    KvT[2] = KvReCo;
      
    KvRoCe = (eeData[52] & 0x00f0) >> 4;
    if (KvRoCe > 7)
    {
        KvRoCe = KvRoCe - 16;
    }
    KvT[1] = KvRoCe;
      
    KvReCe = (eeData[52] & 0x000f);
    if (KvReCe > 7)
    {
        KvReCe = KvReCe - 16;
    }
    KvT[3] = KvReCe;
  
    kvScale = (eeData[56] & 0x0f00) >> 8;


    for( i = 0; i < 24; i++)
    {
        for( j = 0; j < 32; j ++)
        {
            p = 32 * i +j;
            split = 2*(p/32 - (p/64)*2) + p%2;
            kvTemp[p] = KvT[split];
            kvTemp[p] = kvTemp[p] / powf(2.0, (float)kvScale);
        }
    }
    
    temp = fabsf(kvTemp[0]);
    for( i = 1; i < 768; i++)
    {
        if (fabsf(kvTemp[i]) > temp)
        {
            temp = fabsf(kvTemp[i]);
        }
    }
    
    kvScale = 0;
    while(temp < 63.4)
    {
        temp = temp*2;
        kvScale = kvScale + 1;
    }    
     
    for( i = 0; i < 768; i++)
    {
        temp = kvTemp[i] * powf(2.0,(float)kvScale);
        if (temp < 0)
        {
            mlx90640->kv[i] = (temp - 0.5);
        }
        else
        {
            mlx90640->kv[i] = (temp + 0.5);
        }        
        
    } 
    
    mlx90640->kvScale = kvScale;        
}


//  CP’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8  b7 b6  b5  b4  b3  b2  b1    b0   
// 32 0x2420:<-Alpha scale -><Scale ACC row><Scale ACC col><Scale ACC remnand>
//     :
//     :
// 56 0x2438:< MLX  ><Res cali><- Kv scale -><- Kta_scale_1 ><- Kta_scale_2  >  
// 57 0x2439:<}Alpha CP subpage     ><---- Alpha CP subpage_0  ------------ >
// 58 0x243a:<}Offset(CPsubpate1 - 0><----}Offset CP subpage_0 ------------>
// 59 0x243b:<-------- }Kv_CP --------------><----------- }Kta_CP --------->  
// 

void ExtractCPParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    float alphaSP[2];
    int16_t offsetSP[2];
    float cpKv;
    float cpKta;
    uint8_t alphaScale;
    uint8_t ktaScale1;
    uint8_t kvScale;

    alphaScale = ((eeData[32] & 0xf000) >> 12) + 27;
    
    offsetSP[0] = (eeData[58] & 0x03ff);
    if (offsetSP[0] > 511)
    {
        offsetSP[0] = offsetSP[0] - 1024;
    }
    
    offsetSP[1] = (eeData[58] & 0xfc00) >> 10;
    if (offsetSP[1] > 31)
    {
        offsetSP[1] = offsetSP[1] - 64;
    }
    offsetSP[1] = offsetSP[1] + offsetSP[0]; 
    
    alphaSP[0] = (eeData[57] & 0x03ff);
    if (alphaSP[0] > 511)
    {
        alphaSP[0] = alphaSP[0] - 1024;
    }
    alphaSP[0] = alphaSP[0] /  powf(2.0,(float)alphaScale);
    
    alphaSP[1] = (eeData[57] & 0xfc00) >> 10;
    if (alphaSP[1] > 31)
    {
        alphaSP[1] = alphaSP[1] - 64;
    }
    alphaSP[1] = (1 + alphaSP[1]/128) * alphaSP[0];
    
    cpKta = (eeData[59] & 0x00ff);
    if (cpKta > 127)
    {
        cpKta = cpKta - 256;
     }
    
    ktaScale1 = ((eeData[56] & 0x00f0) >> 4) + 8;    
    mlx90640->cpKta = cpKta / powf(2.0,(float)ktaScale1);
    
    cpKv = (eeData[59] & 0xff00) >> 8 ;
    if (cpKv > 127)
    {
        cpKv = cpKv - 256;
    }
    
    kvScale = (eeData[56] & 0x0f00) >> 8;
    
    mlx90640->cpKv = cpKv / powf(2.0,(float)kvScale);
       
    mlx90640->cpAlpha[0] = alphaSP[0];
    mlx90640->cpAlpha[1] = alphaSP[1];
    mlx90640->cpOffset[0] = offsetSP[0];
    mlx90640->cpOffset[1] = offsetSP[1];  
}

//  CILC’l‚Ìæ‚èo‚µ (‰‰ZŠÜ‚Ş)
//
// id adrs    b15 b14 b13 b12 b11 b10 b9 b8  b7 b6  b5  b4  b3  b2  b1  b0   
// 10 0x2410: <---             Device options                         --->
//     :
//     :
// 53 0x2435: < }IL_CHESS_C3   ->< }IL_CHESS_C2 -><-   }IL_CHESS_C1 ->  
//

void ExtractCILCParameters(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    float ilChessC[3];
    uint8_t calibrationModeEE;
    
    calibrationModeEE = (eeData[10] & 0x0800) >> 4;
    calibrationModeEE = calibrationModeEE ^ 0x80;

    ilChessC[0] = (eeData[53] & 0x003f);
    if (ilChessC[0] > 31)
    {
        ilChessC[0] = ilChessC[0] - 64;
    }
    ilChessC[0] = ilChessC[0] / 16.0f;
    
    ilChessC[1] = (eeData[53] & 0x07c0) >> 6;
    if (ilChessC[1] > 15)
    {
        ilChessC[1] = ilChessC[1] - 32;
    }
    ilChessC[1] = ilChessC[1] / 2.0f;
    
    ilChessC[2] = (eeData[53] & 0xf800) >> 11;
    if (ilChessC[2] > 15)
    {
        ilChessC[2] = ilChessC[2] - 32;
    }
    ilChessC[2] = ilChessC[2] / 8.0f;
    
    mlx90640->calibrationModeEE = calibrationModeEE;
    mlx90640->ilChessC[0] = ilChessC[0];
    mlx90640->ilChessC[1] = ilChessC[1];
    mlx90640->ilChessC[2] = ilChessC[2];
}


//
//  •s—ÇƒsƒNƒZƒ‹(broken , outlier)
//
// ƒŠƒ^[ƒ“’l:
//          =   0 : broken pixel‚ª4‚Â–¢–A‚©‚ÂOutlier pixel‚ª‚S‚Â–¢–
//          =  -3 : broken pixel‚ª4‚ÂˆÈã‚ ‚éB
//          =  -4 : Outlier pixels‚ª4‚ÂˆÈã‚ ‚éB
//          =  -5 : broken‚ÆOutlier ‚Ì‡Œv‚ª4‚ÂˆÈã‚ ‚éB
//          =  -6 :Broken pixel has adjacent broken pixel or
//                 Outlier pixel has adjacent outlier pixel or 
//                 Broken pixel has adjacent outlier pixel
//
 int ExtractDeviatingPixels(uint16_t *eeData, paramsMLX90640 *mlx90640)
{
    uint16_t pixCnt = 0;
    uint16_t brokenPixCnt = 0;
    uint16_t outlierPixCnt = 0;
    int warn = 0;
    int i;
    
    for(pixCnt = 0; pixCnt<5; pixCnt++)
    {
        mlx90640->brokenPixels[pixCnt] = 0xffff;
        mlx90640->outlierPixels[pixCnt] = 0xffff;
    }
        
    pixCnt = 0;    
    while (pixCnt < 768  && brokenPixCnt < 5 && outlierPixCnt < 5)
    {
        if(eeData[pixCnt+64] == 0)
        {
            mlx90640->brokenPixels[brokenPixCnt] = pixCnt;
            brokenPixCnt = brokenPixCnt + 1;
        }    
        else if((eeData[pixCnt+64] & 0x0001) != 0)
        {
            mlx90640->outlierPixels[outlierPixCnt] = pixCnt;
            outlierPixCnt = outlierPixCnt + 1;
        }    
        
        pixCnt = pixCnt + 1;
        
    } 
    
    if(brokenPixCnt > 4)  
    {
        warn = -MLX90640_BROKEN_PIXELS_NUM_ERROR;
    }         
    else if(outlierPixCnt > 4)  
    {
        warn = -MLX90640_OUTLIER_PIXELS_NUM_ERROR;
    }
    else if((brokenPixCnt + outlierPixCnt) > 4)  
    {
        warn = -MLX90640_BAD_PIXELS_NUM_ERROR;
    } 
    else
    {
        for(pixCnt=0; pixCnt<brokenPixCnt; pixCnt++)
        {
            for(i=pixCnt+1; i<brokenPixCnt; i++)
            {
                warn = CheckAdjacentPixels(mlx90640->brokenPixels[pixCnt],mlx90640->brokenPixels[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        }
        
        for(pixCnt=0; pixCnt<outlierPixCnt; pixCnt++)
        {
            for(i=pixCnt+1; i<outlierPixCnt; i++)
            {
                warn = CheckAdjacentPixels(mlx90640->outlierPixels[pixCnt],mlx90640->outlierPixels[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        } 
        
        for(pixCnt=0; pixCnt<brokenPixCnt; pixCnt++)
        {
            for(i=0; i<outlierPixCnt; i++)
            {
                warn = CheckAdjacentPixels(mlx90640->brokenPixels[pixCnt],mlx90640->outlierPixels[i]);
                if(warn != 0)
                {
                    return warn;
                }    
            }    
        }    
        
    }    
    
    
    return warn;
       
}


// Adjacent:—×Ú‚·‚é
// (Extract_DeviatingPixels)‚©‚çŒÄ‚Î‚ê‚éƒ‹[ƒ`ƒ“
//
int CheckAdjacentPixels(uint16_t pix1, uint16_t pix2)
 {
     int pixPosDif;
  
     uint16_t lp1 = pix1 >> 5;
     uint16_t lp2 = pix2 >> 5;
     uint16_t cp1 = pix1 - (lp1 << 5);
     uint16_t cp2 = pix2 - (lp2 << 5);
     
     pixPosDif = lp1 - lp2;
     if(pixPosDif > -2 && pixPosDif < 2)
     {
        pixPosDif = cp1 - cp2;
        if(pixPosDif > -2 && pixPosDif < 2)
        {
            return -6;
        }

     } 
      
     return 0;    
 }
 


