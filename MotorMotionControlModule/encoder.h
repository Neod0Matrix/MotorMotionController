#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	通用4-5线编码器速度位置反馈
	本API支持4线、5线编码器，若接入4线编码器(自带Z相)则忽略Z相配置
	若接入5线编码器，则需配置Z相脉冲的外部中断和定时器中断
*/

//编码器结构体，假设一个步进电机对应一个编码器
typedef __packed struct 						
{
	TIM_TypeDef* 	timx;													//编码器读取脉冲使用的定时器
	uint32_t		rcc_apbxperiph_timx;									//该定时器RCC总线
	u16 			encoderLine;											//编码器线数
	u16 			absolutePos;											//编码器绝对位置
	float 			encodeMesSpeed;											//编码器当前测速
} EncoderDataCache;
extern EncoderDataCache st_encoderAcfg;										//测试编码器A

#define Encoder_LTR_Const(line)			(float)(360.f / line)				//线数转化为角度系数
#define Encoder_Zphase_EXTI_Line		EXTI_Line2							//编码器Z相外部中断总线	

//测速添加一阶卡尔曼滤波
extern kf_1deriv_factor ecstr;

void EncoderStructure_Init (EncoderDataCache *erstr);
void EncoderPhase_IO_Init (void);
void TIM8_EncoderCounter_Config (EncoderDataCache *erstr);
void EncoderCount_SetZero (EncoderDataCache *erstr);
void EncoderCount_ReadValue (EncoderDataCache *erstr);
void Encoder_MeasureAxisSpeed (MotorMotionSetting *mcstr, EncoderDataCache *erstr);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
