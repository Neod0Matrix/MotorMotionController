#ifndef __SIGMOD_H__
#define __SIGMOD_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//电机S形加减速算法
	
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
	调参方法：优化曲线A，B值
	不建议把最低频率设置到0
*/	
#ifndef sigmodAlgo
#define sigmodAlgo(ymax, ymin, a, b, x)	(u16)((ymax - ymin) / (1 + exp((double)(-a * (x - b)))) + ymin)
#endif

//X_Range / Num_Range即x取值间隔，最好为整数			
#define Num_Range						40u				//x取值个数	
#define X_Range							400u			//x取值范围，越大曲线越平滑(通常并不需要多平滑)

//sigmod函数参数结构体
typedef __packed struct 
{
	u16 	freq_max;									//参数freq_max，设置最高达到频率，但要注意抑制机械振动
	u16 	freq_min;									//参数freq_min，设置最小换向频率
	float 	para_a;										//参数para_a，越小曲线越平滑
	float 	para_b;										//参数para_b，越大曲线上升下降越缓慢
	float 	ratio;										//参数ratio，S形加减速阶段分化比例
	u16 	disp_table[Num_Range];						//整型离散表
} Sigmod_Parameter;		
extern Sigmod_Parameter asp, dsp;

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

//相关函数声明
u16 TIMx_GetNowTime (void);										//对应轴的时间获取
void sigmodPara_Init (void);									//参数初始化
extern void FreqDisperseTable_Create (MotorMotionSetting mc);	//得到加减速表
//S型加减速电机控制实现
extern void SigmodAcceDvalSpeed (void);

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
