#ifndef __SIGMOD_H__
#define __SIGMOD_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	机械臂步进电机S形加减速算法
	调参方法：优化曲线A，B值
*/		
				
//脉冲IO口
#define IO_MainPulse 			PBout(0)		//PB0
#define IO_Direction 			PAout(6)		//方向线输出
//测试用宏定义
#define StepMotorZero			200u			//正常一圈(步距角)
#define Subdivision				2u				//细分数
#define Pulse_per_Loop 			(StepMotorZero * Subdivision)//实际脉冲个数/圈	
#define MaxLimit_Dis			315				//滑轨限位



//方向选择
#ifdef use_ULN2003A								//ULN2003A反相设置
typedef enum {Pos_Rev = 1, Nav_Rev = !Pos_Rev} RevDirection;		
#else
typedef enum {Pos_Rev = 0, Nav_Rev = !Pos_Rev} RevDirection;	
#endif

//电机控制IO初始化
void PulseDriver_IO_Init (void);
void Direction_IO_Init (void);					

/*
	sigmod函数原型
	matlab建模原型方程：
	x取10为间隔，从0取到400
	y在1500到2500之间摆动
	测试参数a=0.1 b=50
	>>	x = [0:10:400];
	>>	y = (2500-1500)./(1+exp(-0.1*(x-50)))+1500;
	>>	plot(x, y)
	只取整数进行演算
*/	
#ifndef sigmodAlgo
#define sigmodAlgo(ymax, ymin, a, b, x)	(u16)((ymax - ymin) / (1 + exp((double)(-a * (x - b)))) + ymin)
#endif

//X_Range / Num_Range即x取值间隔，最好为整数			
#define Num_Range				40u				//x取值个数	
#define X_Range					400u			//x取值范围，越大曲线越平滑(通常并不需要多平滑)

//sigmod函数参数结构体
typedef __packed struct 
{
	u16 	freq_max;							//参数freq_max，设置最高达到频率，但要注意抑制机械振动
	u16 	freq_min;							//参数freq_min，设置最小换向频率
	float 	para_a;								//参数para_a，越小曲线越平滑
	float 	para_b;								//参数para_b，越大曲线上升下降越缓慢
	float 	ratio;								//参数ratio，S形加减速阶段分化比例
	u16 	disp_table[Num_Range];				//整型离散表
} Sigmod_Parameter;		

//结构体声明
typedef __packed struct 						
{
    volatile u16 		pulse_counter;			//配置定时器18计数器		
    float 				distance;				//最小旋转圈数
    u16 				Frequency;				//设定频率
	volatile u16 		freqlive_stage;			//频率存活阶段		
	Sigmod_Parameter 	asp;					//S加速
	Sigmod_Parameter 	dsp;					//S减速
} Motorx_CfgPara;
//初始化两个轴
extern Motorx_CfgPara motorx_cfg;	

//区间脉冲总数计算
#ifndef PulseWholeNbr
#define PulseWholeNbr(d, r)				(d * Pulse_per_Loop * r)
#endif

//均匀时间分配，单位ms
#ifndef StageTimeDelay	
#define StageTimeDelay(pn, fq) 			(u16)((pn * 1000u) / fq)
#endif			

//定义电机运行速度阶段
typedef enum
{
	as = 0,
	us = 1,
	ds = 2,
} MotorRunStage;

extern void MotorDriverLib_Init (void);							//总初始化封装库

//相关函数声明
u16 TIMx_GetNowTime (void);										//对应轴的时间获取
void sigmodPara_Init (void);									//参数初始化
extern void FreqDisperseTable_Create (Motorx_CfgPara mc);		//得到加减速表
extern void TIM3_SigmodSysTick_Init (FunctionalState control);	//定时器3初始化
//S型加减速电机控制实现
void WaitPulseRespond (MotorRunStage rs, Motorx_CfgPara mc);
extern void SigmodAcceDvalSpeed (Motorx_CfgPara mc);

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
