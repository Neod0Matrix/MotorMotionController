#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//模块MotorMotionControl对框架EmbeddBreakerCore的链接
//该文件写入对框架的函数调用支持

Sigmod_Acce_Dval_Switch 	SAD_Switch;
Init_ARM_Reset_Switch 		Init_Reset_Switch;
ARM_Sensor_EXTI_Setting		ASES_Switch;
Stew_EXTI_Setting			StewEXTI_Switch;

//链接到Universal_Resource_Config函数的模块库
void ModuleMMC_UniResConfig (void)
{
	/*
		电机柔性启停有多种积极意义
		本工程主要是为了在步进电机相对高速运转时带动更重负载
	*/
    SAD_Switch 			= SAD_Enable;					//SAD_Enable		SAD_Disable
	
	/*
		机器上电完全复位的重要部分
		实际运用建议开启，建立绝对坐标系
		调试时建议关闭，以免损伤机械臂部件
	*/
    Init_Reset_Switch 	= Reset_Disable;				//Reset_Enable		Reset_Disable
	
	/*
		由于机械臂传感器触发判断多种多样
		可能不适合放到外部中断
		但如果普通检测可能响应不够快
	*/
	ASES_Switch			= ASES_Enable;					//ASES_Enable		ASES_Disable
	
	/*
		急停状态判断复杂，不适合外部中断
		但也有可能普通监测不够快
	*/
	StewEXTI_Switch 	= StewEXTI_Enable;				//StewEXTI_Enable	StewEXTI_Disable
}

//模块选项映射表，链接到urcMapTable_Print函数
void ModuleMMC_URCMap (void)
{
	printf("\r\n%02d	S-Accel/Dvalue Speed", urc_sad);
	usart1WaitForDataTransfer();
	printf("\r\n%02d	Arm Position Reset", urc_areset);
	usart1WaitForDataTransfer();
	printf("\r\n%02d 	Arm Sensor EXTI Setting", urc_ases);
	usart1WaitForDataTransfer();
	printf("\r\n%02d 	Stew EXTI Setting", urc_stew);
	usart1WaitForDataTransfer();
}

//选项处理，链接到pclURC_DebugHandler函数
void ModuleMMC_urcDebugHandler (u8 ed_status, Module_SwitchNbr sw_type)
{
	switch (sw_type)
	{
	case urc_sad: 		SAD_Switch 		= (Sigmod_Acce_Dval_Switch)ed_status; 		break;
	case urc_areset: 	ASES_Switch 	= (ARM_Sensor_EXTI_Setting)ed_status; 		break;
	case urc_ases: 		ASES_Switch		= (ARM_Sensor_EXTI_Setting)ed_status;		break;	
	case urc_stew: 		StewEXTI_Switch	= (Stew_EXTI_Setting)ed_status;				break;	
	}
}

//OLED常量第四屏，链接到OLED_DisplayInitConst和UIScreen_DisplayHandler函数
void OLED_ScreenP4_Const (void)
{	
	OLED_ShowString(strPos(1u), ROW1, (const u8*)" MotorMotion  ", Font_Size);	
	OLED_ShowString(strPos(1u), ROW2, (const u8*)"ControlModule ", Font_Size);	
	OLED_Refresh_Gram();
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
