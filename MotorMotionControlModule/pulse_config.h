#ifndef __TIM_CONFIG_H__
#define __TIM_CONFIG_H__
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

//定时器设置参数
#define TargetTimeBase					10								//定时器单个目标定时时基，单位us
#define TIMPrescaler					71								//psc 时钟预分频数，通用71分频
#define TIMarrPeriod					9								//arr 自动重装值，最大捕获范围0xFFFF
#define FreqMaxThreshold				500000L							//频率计数器上限阈值
//分频数计算
#ifndef DivFreqConst					
#define DivFreqConst(fre) 				(uint32_t)((FreqMaxThreshold / TargetTimeBase) / fre)
#endif

//脉冲发送结束后电机驱动IO口的复位状态
#ifdef use_ULN2003A														//ULN2003A反相设置
#define MD_IO_Reset						lvl								//反相拉低
#else 
#define MD_IO_Reset						hvl								//正相拉高
#endif

//行距逆向算法，用于机械臂绝对坐标构建
#ifndef DistanceFeedback
#define DistanceFeedback(pulc, perloop) ((((pulc + 1u) / 2u) / perloop) * OneLoopHeight)
#endif

//结构体成员初始化
void MotorConfigStrParaInit (MotorMotionSetting *mcstr);

//高级定时器初始化函数声明					
void TIM1_MecMotorDriver_Init (void);			

//更新行距
void DistanceAlgoUpdate (MotorMotionSetting *mcstr);

//电机脉冲产生中断
void MotorPulseProduceHandler (MotorMotionSetting *mcstr);



//定时器输出比较模式配置
void TIM1_MotorMotionTimeBase (			uint16_t 		Motorx_CCx, 		//电机对应定时器通道
									FunctionalState control				//控制开关
						);
	
//电机动作调用函数
//传参：结构体频率，结构体圈数，使能开关
void MotorMotionDriver (	MotorMotionSetting *mcstr,				//结构体传参
									FunctionalState control				//控制开关
									);

//机械臂运动算例
extern void MotorBaseMotion (u16 spfq, u16 mvdis, RevDirection dir, MotorRunMode mrm, LineRadSelect lrs);

//测试算例
extern void PeriodUpDnMotion (u16 count);											//滑轨上下测试
extern void RepeatTestMotion (void);												//传感器机械臂反复测试
extern void Axis_Pos_Reset (void);													//开机机械臂复位到零点

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon

