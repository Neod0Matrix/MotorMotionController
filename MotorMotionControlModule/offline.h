#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//专业综合训练脱机控制协议

/*
	2018年夏季专业综合训练使用
	脱机控制协议
	该协议涉及的所有参数和编排意义都根据训练需求而定

	由于协议固定长度为18个字节(早期开发遗留)，按照最小运动单元45度角为基准，
	一字节转向，两字节45度倍数，固定速度，一个动作只需要3个字节来完成，
	这样除去算例标识，帧头帧尾，正好可以在18个字节内完成5组动作

	专业综合训练结束后务必删除该部分(一脸嫌弃)
	protocol.c L32 L180
	protocol.h L54
	offline.c offline.h
	link.h L16
	time_base.c L45
	led.c L161 L199
*/
#define OLCP					0x1B						//Offline Control Porotcol
#define	Offline_Protocol		{DH, OLCP, 0x02, DMAX, DMAX, 0x02, DMAX, DMAX, 0x02, DMAX, DMAX, 0x02, DMAX, DMAX, 0x02, DMAX, DMAX, DT}
#define OLP_G1_Dir				2u							//第一组转向
#define OLP_G1_Multi			3u							//第一组倍数第一位
#define OLP_G2_Dir				5u							//第二组转向
#define OLP_G2_Multi			6u							//第二组倍数第一位
#define OLP_G3_Dir				8u							//第三组转向
#define OLP_G3_Multi			9u							//第三组倍数第一位
#define OLP_G4_Dir				11u							//第四组转向
#define OLP_G4_Multi			12u							//第四组倍数第一位
#define OLP_G5_Dir				14u							//第五组转向
#define OLP_G5_Multi			15u							//第五组倍数第一位

//开关量模拟量控制
#define ADCP					0x1C						//开关量/逻辑量
#define Anolog_Digital_Protocol	{DH, ADCP, 0x03, DMAX, DMAX, 0x01, NB, NB, NB, NB, NB, NB, NB, NB, NB, NB, NB, DT}
#define ADCP_ABit_1				2u							//模拟量个位
#define ADCP_ABit_2				3u							//模拟量十分位
#define ADCP_ABit_3				4u							//模拟量百分位
#define ADCP_DLogic_Bit			5u							//开关量一位

#define OLCP_StableFreq			2000						//脱机模式固定速度设定
//延时计算：标准1s停滞+2000Hz转速450度/s，45度需要大概100ms，100/45=2.22
#define OLCP_45degreeDelayConst	2.22f	

extern Bool_ClassType offline_JumpOutWhileLoop;

#define IO_DigitalVal 			PAout(11)					//开关量IO
#define IO_AnologVal 			PAout(12)					//模拟量IO

extern u16 AnologPWM_DC;

void UpperMonitorOfflineControl (MotorMotionSetting *mcstr);
void AnologPWMProduce (void);
void AnologDigitalVal_IO_Init (void);
void Anolog_Digital_Controller (void);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
