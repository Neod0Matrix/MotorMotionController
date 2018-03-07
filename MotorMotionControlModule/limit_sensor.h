#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//====================================================================================================
//装载在机械臂上的定位传感器设置初始化
/*
	初始化IO口映射:
		A2U		A2D
		|		|
		PB4		PB3
*/

//定义硬件：传感器输入端口

//#define ARM2_USR 				PBin(4)	 
//#define ARM2_DSR 				PBin(3)   	

#define ULSR_IO  				GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4)	//读取顶部传感器
#define DLSR_IO  				GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)	//读取底部传感器

//限位传感器外部中断总线
#define ARM2Dn_EXTI_Line  		EXTI_Line3									//下方传感器					
#define ARM2Up_EXTI_Line		EXTI_Line4									//上方传感器						

//定义传感器映射键值，必须大于未响应的0值
typedef enum 
{
	Null_TR	= 0u,
	ULSR_TR	= 1u,
	DLSR_TR	= 2u,
} Sensor_MapTable;

//定义enum型传感器编号
typedef enum
{
	ULSR = 1u,
	DLSR = 2u,
} Sensor_Number;

//传感器低电平触发
typedef enum {Trigger = 0, No_Trigger = !Trigger} Sensor_Status;	
//普遍输入检测连续取值
typedef enum {NSLT = 0, SLT = 1} Input_LoogTrigger;

void Sensor_IO_Init (void);													//GPIO初始化
Sensor_MapTable Sensor_Scan (Input_LoogTrigger mode);						//检测触发

//传感器条件判断
#define USrNLTri				(Sensor_Scan(NSLT) == ULSR_TR)
#define USrLTri					(Sensor_Scan(SLT) == ULSR_TR)

#define DSrNLTri				(Sensor_Scan(NSLT) == DLSR_IO)
#define DSrLTri					(Sensor_Scan(SLT) == DLSR_IO)

//判断等待触发封装
extern void WaitForSR_Trigger (Sensor_Number sr_nbr);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
