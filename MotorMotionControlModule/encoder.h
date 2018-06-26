#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//两相(ABZ)5线编码器速度位置反馈

#define EncoderLineValue			600u		//编码器线数
#define Encoder_Zphase_EXTI_Line	EXTI_Line2	//编码器Z相外部中断总线		

void EncoderPhase_IO_Init (void);
void TIM4_EncoderCounter_Config (void);
u16 EncoderCount_ReadValue (void);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
