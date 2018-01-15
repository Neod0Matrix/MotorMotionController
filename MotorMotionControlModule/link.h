#ifndef __LINK_H__
#define __LINK_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//模块MotorMotionControl对框架EmbeddBreakerCore的链接
//该文件需要添加到stdafx.h内生效

//链接所有MotorMotionControl模块的头文件
#include "limit_sensor.h"
#include "pulse_config.h"
#include "sigmod.h"

//以下仿照config.h写法配置该模块的本地URC接口
#define PosLogicOperation				//脉冲信号配置为正逻辑式，如果采用负逻辑则注释
//#define UseTimerPWMorOCChannel		//定时器PWM或者OC通道是否使用原设置通道

//是否启用步进电机S形加减速
typedef enum {SAD_Enable = 1, SAD_Disable = !SAD_Enable} 				Sigmod_Acce_Dval_Switch;
extern Sigmod_Acce_Dval_Switch 		SAD_Switch;
//是否启用开机机械臂复位
typedef enum {Reset_Enable = 1, Reset_Disable = !Reset_Enable} 			Init_ARM_Reset_Switch;
extern Init_ARM_Reset_Switch 		Init_Reset_Switch;
//是否将机械臂传感器设置为外部中断
typedef enum {ASES_Enable = 1, ASES_Disable = !ASES_Enable}				ARM_Sensor_EXTI_Setting;
extern ARM_Sensor_EXTI_Setting		ASES_Switch;
//是否开启急停外部中断
typedef enum {StewEXTI_Enable = 1, StewEXTI_Disable = !StewEXTI_Enable}	Stew_EXTI_Setting;
extern Stew_EXTI_Setting			StewEXTI_Switch;
//电机运行模式，有限运行(正常)，无限运行(测试脉冲频率使用)
typedef enum {LimitRun = 0, UnlimitRun = 1} MotorRunMode;
//线度角度切换(RA<->RD)
typedef enum {RadUnit = 0, LineUnit = 1} LineRadSelect;

//urc开源链接编号
typedef enum 
{
	urc_sad 	= 15,
	urc_areset 	= 16,
	urc_ases 	= 17,
	urc_stew 	= 18,
} Module_SwitchNbr;

#define Max_Option_Value	18u

#define ModuleMMC_Protocol 	{DH, SSDS, DMAX, DMAX, DMAX, DMAX, DMAX, DMAX, NB, NB, NB, NB, NB, NB, NB, NB, NB, DT}

//协议protocol.c链接
#define SSDS				0x1D						//单步调试
#define SSD_MoNum_1st		2u							//单步调试算例编号第一位
#define SSD_GetDis_1st		4u							//单步调试获得圈数第一位

//对13个运动算例进行串口查询编号
typedef enum
{
    Stew_All 	= 0,							//急停
    UpMove		= 1,							//机械臂上行
    DownMove	= 2,							//机械臂下行
	ERROR_OUT	= 3,							//解除报警
	Repeat		= 4,							//反复测试
} Motion_Select;								//算例选择

#define X_Max				9999					
#define Y_Max				9999

//行距倍数
#define Distance_Ratio		1u			
//测试滑轨定义，长度单位都是mm
#define Z_Max				310														//5轴机械臂Z轴坐标系顶点，可适当延伸
#define OneLoopHeight		5														//步进电机转一圈上升高度	

extern void U1RSD_example (void);				//串口处理例程封装
extern Motion_Select SingleStepDebug_linker (void);//上层封装单步调试调用链接库

void ModuleMMC_UniResConfig (void);
void ModuleMMC_URCMap (void);
void ModuleMMC_urcDebugHandler (u8 ed_status, Module_SwitchNbr sw_type);
void OLED_ScreenP4_Const (void);

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
