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

#define EncoderLineValue				600.f								//编码器线数
#define Encoder_LineTransferRad_Const	(float)(360.f / EncoderLineValue)	//线数转化为角度系数
#define Encoder_Zphase_EXTI_Line		EXTI_Line2							//编码器Z相外部中断总线	

extern float encodeMesSpeed;
//测速添加一阶卡尔曼滤波
extern kf_1deriv_factor ecstr;

void EncoderPhase_IO_Init (void);
void TIM8_EncoderCounter_Config (void);
void EncoderCount_SetZero (void);
u16 EncoderCount_ReadValue (void);
float Encoder_MeasureAxisSpeed (MotorMotionSetting *mcstr);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
