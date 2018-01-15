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
#define TIMPrescaler					71u								//psc 时钟预分频数，通用71分频
#define TIMarrPeriod					9								//arr 自动重装值，最大捕获范围0xFFFF
#define FreqMaxThreshold				500000L							//频率计数器上限阈值
//分频数计算
#ifndef DivFreqConst					
#define DivFreqConst(fre) 				(uint32_t)((FreqMaxThreshold / TargetTimeBase) / fre)
#endif

//带参宏定义翻转周期设置
#ifndef TimTPArr500k
#define TimTPArr500k(fre)				(uint16_t)((FreqMaxThreshold / fre) - 1u)//频率计数器
#endif	

//脉冲发送结束后电机驱动IO口的复位状态
#ifdef use_ULN2003A														//ULN2003A反相设置
#define MD_IO_Reset						lvl								//反相拉低
#else 
#define MD_IO_Reset						hvl								//正相拉高
#endif

//定时器技术频率结构体
typedef __packed struct 											
{
	//编译不优化
    volatile uint16_t togglePeriod;										//翻转周期
	volatile uint16_t toggleCount;										//翻转次数
} TimCalcul_FreqPara;
//每个电机对应的定时器通道都必须有相应的配置
extern TimCalcul_FreqPara mAx_Timer;			

//脉冲总数计算公式
#ifndef PulseSumCalicus
#define PulseSumCalicus(perloop, dis) 	(2u * perloop * dis - 1u)		
#endif

//行距逆向算法，用于机械臂绝对坐标构建
#ifndef DistanceFeedback
#define DistanceFeedback(pulc, perloop) ((((pulc + 1u) / 2u) / perloop) * OneLoopHeight)
#endif

//高级定时器初始化函数声明					
void TIM1_MecMotorDriver_Init (void);								

//电机输出比较中断
void motorAxisx_IRQHandler (void);

//结构体成员初始化
void MotorConfigStrParaInit (Motorx_CfgPara *mcstr);

//定时器输出比较模式配置
void Motor_TIM1_ITConfig (
									uint16_t 		tp, 				//翻转周期
									uint16_t 		Motorx_CCx, 		//电机对应定时器通道
									FunctionalState control				//控制开关
						);
	
//电机动作调用函数
//传参：结构体频率，结构体圈数，使能开关
FunctionalState MotorMotionDriver (	u16 			frequency, 			//结构体频率，即电机运行速度
									float 			Distance, 			//运行距离
									FunctionalState control				//控制开关
									);
									
//电机运动开
#define MotorAxisx_Switch_On 			MotorMotionDriver(motorPID_DebugFreq(motorx_cfg.Frequency), motorPID_DebugDis(motorx_cfg.distance), ENABLE)
//电机运动关
#define MotorAxisx_Switch_Off 			MotorMotionDriver(motorPID_DebugFreq(motorx_cfg.Frequency), motorPID_DebugDis(motorx_cfg.distance), DISABLE)

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon

