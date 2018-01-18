#ifndef __PULSE_CONFIG_H__
#define __PULSE_CONFIG_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	电机驱动脉冲底层框架模块
	采用高级定时器1、8对移动平台机械臂分别控制
	ULN2003A具有IO反相驱动增强功能，使用时5V口悬空，IO输出口接上拉电阻拉到电源
	上拉电阻选择：5V -- 3.3k 24V -- 4.7k
	通用分频数=500000/定时器时基
*/

//脉冲IO口
#define IO_MainPulse 					PBout(0)		//主脉冲输出
#define IO_Direction 					PAout(6)		//方向线输出
//测试用宏定义
#define StepMotorZero					200u			//正常一圈(步距角)
#define Subdivision						16u				//细分数
#define Pulse_per_Loop 					(StepMotorZero * Subdivision)//实际脉冲个数/圈	

//角度线度转换
//角度单位：一圈即360度；线度单位：一圈即5mm
#define MaxLimit_Dis					315				//滑轨限位
#define OneLoopHeight					5				//步进电机转一圈上升高度
#define RadUnitConst					(Pulse_per_Loop / 360)
#define LineUnitConst					(Pulse_per_Loop / OneLoopHeight)

//定时器设置参数
#define TIMPrescaler					71				//psc 时钟预分频数，通用71分频
#define TIMarrPeriod					9				//arr 自动重装值，最大捕获范围0xFFFF
#define TargetTimeBase					TimeCalcusofucTimer(TIMarrPeriod, TIMPrescaler)//定时器单个目标定时时基，单位us
#define FreqMaxThreshold				500000L			//频率计数器上限阈值
//分频数计算
#ifndef DivFreqConst					
#define DivFreqConst(targetFreq) 		(uint16_t)(((FreqMaxThreshold / TargetTimeBase) / targetFreq) - 1)
#endif
//复位起始频率
#define ResetStartFrequency				3000		

//行距逆向算法，用于机械臂绝对坐标构建(坐标调试模式使用)
#ifndef DistanceFeedback
#define DistanceFeedback(pulc, perloop) ((((pulc + 1u) / 2u) / perloop) * OneLoopHeight)
#endif

//脉冲发送结束后电机驱动IO口的复位状态
#ifdef use_ULN2003A										//ULN2003A反相设置
#define MD_IO_Reset						lvl				//反相拉低
#else 
#define MD_IO_Reset						hvl				//正相拉高
#endif

//方向选择
#ifdef use_ULN2003A										//ULN2003A反相设置
typedef enum {Pos_Rev = 1, Nav_Rev = !Pos_Rev} RevDirection;		
#else
typedef enum {Pos_Rev = 0, Nav_Rev = !Pos_Rev} RevDirection;	
#endif

//电机运行状态
typedef enum {Run = 1, Stew = !Run} MotorRunStatus;

//电机运行模式，有限运行(正常)，无限运行(测试脉冲频率使用)
typedef enum {LimitRun = 0, UnlimitRun = 1} MotorRunMode;
//线度角度切换(RA<->RD)
typedef enum {RadUnit = 0, LineUnit = 1} LineRadSelect;

//电机启动制动
typedef enum {StopRun = 0, StartRun = !StopRun} MotorStartStop;

//电机调用结构体
typedef __packed struct 						
{
	//基础运动控制
    volatile uint32_t 	ReversalCnt;					//脉冲计数器
	uint32_t			ReversalRange;					//脉冲回收系数
    uint32_t			RotationDistance;				//行距
    uint16_t 			SpeedFrequency;					//设定频率
	volatile uint16_t	divFreqCnt;						//分频计数器
	uint16_t			CalDivFreqConst;				//分频系数
	//电机状态标志
	MotorRunStatus		MotorStatusFlag;				//运行状态
	MotorRunMode		MotorModeFlag;					//运行模式
	LineRadSelect		DistanceUnitLS;					//线度角度切换
	RevDirection		RevDirectionFlag;				//方向标志
} MotorMotionSetting;
extern MotorMotionSetting st_motorAcfg;					//测试步进电机A

//电机控制IO初始化
void PulseDriver_IO_Init (void);
void Direction_IO_Init (void);		

//基于定时器的基本电机驱动
void MotorConfigStrParaInit (MotorMotionSetting *mcstr);							//结构体成员初始化
void TIM1_MecMotorDriver_Init (void);												//高级定时器初始化函数声明		
extern void MotorDriverLib_Init (void);												//总初始化封装库
void TIM1_OutputChannelConfig (uint16_t Motorx_CCx, FunctionalState control);		//定时器输出比较模式通道配置
void DistanceAlgoUpdate (MotorMotionSetting *mcstr);								//更新行距
void MotorBasicDriver (MotorMotionSetting *mcstr, MotorStartStop control);			//电机底层驱动
void MotorPulseProduceHandler (MotorMotionSetting *mcstr);							//电机脉冲产生中断

//运动测试算例
extern void MotorMotionController (u16 spfq, u16 mvdis, RevDirection dir, 
	MotorRunMode mrm, LineRadSelect lrs, MotorMotionSetting *mcstr);				//机械臂运动算例
extern void PeriodUpDnMotion (u16 count, MotorMotionSetting *mcstr);				//滑轨上下测试
extern void RepeatTestMotion (MotorMotionSetting *mcstr);							//传感器限位反复测试
extern void Axis_Pos_Reset (MotorMotionSetting *mcstr);								//开机滑轨复位到零点

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
