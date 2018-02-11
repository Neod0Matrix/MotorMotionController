#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	模块对框架EmbeddBreakerCore的链接
	该文件写入对框架的函数调用支持
*/

//模块声明
#define _Modules_Type_			"PMC"						//模块类型
#define _Modules_Name_			"MotorMotionController"		//模块名称
#define _Modules_Version_ 		"v0p3_LTE"					//长期演进版

//以下仿照config.h写法配置该模块的本地URC接口
#define PosLogicOperation									//脉冲信号配置为正逻辑式，如果采用负逻辑则注释
//#define UseTimerPWMorOCChannel							//定时器PWM或者OC通道是否使用原设置通道

//是否启用步进电机S形加减速
typedef enum {SAD_Enable = 1, SAD_Disable = !SAD_Enable} 				Sigmod_Acce_Dval_Switch;
extern Sigmod_Acce_Dval_Switch 		SAD_Switch;
//是否启用开机机械臂复位
typedef enum {Reset_Enable = 1, Reset_Disable = !Reset_Enable} 			Init_ARM_Reset_Switch;
extern Init_ARM_Reset_Switch 		Init_Reset_Switch;
//是否将机械臂传感器设置为外部中断
typedef enum {ASES_Enable = 1, ASES_Disable = !ASES_Enable}				ARM_Sensor_EXTI_Setting;
extern ARM_Sensor_EXTI_Setting		ASES_Switch;

//模块使用的协议链接，尽量整合到一条
/*
	电机单步调试协议
	协议18位，除去字头字尾有15位
	后一位表示算列类型，前5位表示行距(1位指示单位，4位指示长度
	再后面5位表示速度，单位hz，再后一位表示模式
	空2位
*/
#define MDLS					0x1A
#define Modules_Protocol 		{DH, MDLS, DMAX, DMAX, LineUnit, DMAX, DMAX, DMAX, DMAX, DMAX, DMAX, DMAX, DMAX, UnlimitRun, NB, NB, NB, DT}
#define SSD_MoNum_1st			2u							//单步调试算例编号第一位，共2位
#define SSD_DisUnit_1st			4u							//单步调试行距单位第一位，共1位
#define SSD_GetDis_1st			5u							//单步调试行距第一位，共4位
#define SSD_SpFq_1st			9u							//单步调试速度第一位，共4位
#define SSD_Mode_1st			13u							//单步调试运行模式第一位，共1位

//对13个运动算例进行串口查询编号
typedef enum
{
    Stew_All 	= 0,									//急停
    UpMove		= 1,									//机械臂上行
    DownMove	= 2,									//机械臂下行
	Repeat		= 3,									//反复测试
} Motion_Select;										//算例选择	

//urc开源链接编号
typedef enum
{
	urc_sad 	= 15,
	urc_areset 	= 16,
	urc_ases 	= 17,
} Modules_SwitchNbr;

#define Max_Option_Value		17u							//裁去protocol.h中的定义放到这里来重新定义urc协议长度
#define ScreenPageCount			5u							//裁去ui.h中定义的总切屏数到这里来重新定义

//对外API接口
void Modules_UniResConfig (void);							//选项设置，链接到Universal_Resource_Config函数的模块库
void Modules_URCMap (void);									//模块选项映射表，链接到urcMapTable_Print函数
void Modules_urcDebugHandler (u8 ed_status, Modules_SwitchNbr sw_type);//选项处理，链接到pclURC_DebugHandler函数
void Modules_ProtocolTask (void);							//协议调用指令响应，链接到OrderResponse_Handler函数
void OLED_ScreenModules_Const (void);						//OLED常量显示屏，链接到OLED_DisplayInitConst和UIScreen_DisplayHandler函数
void OLED_DisplayModules (void);							//OLED模块调用数据显示，链接到UIScreen_DisplayHandler函数
void Modules_HardwareInit (void);							//硬件底层初始化任务，链接到bspPeriSysCalls函数
void Modules_ExternInterruptInit (void);					//硬件底层外部中断初始化，链接到EXTI_Config_Init函数
void Modules_NonInterruptTask (void);						//模块非中断任务，链接到local_taskmgr.c，默认添加到第二任务
void Modules_InterruptTask (void);							//模块中断任务，链接到time_base.c TIM2_IRQHandler函数中

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
