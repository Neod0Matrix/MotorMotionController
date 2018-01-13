#ifndef __MOTION_STUDIO_H__
#define __MOTION_STUDIO_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//全局运动算例

//行距倍数
#define Distance_Ratio		1u			
//测试滑轨定义，长度单位都是mm
#define Z_Max				310														//5轴机械臂Z轴坐标系顶点，可适当延伸
#define OneLoopHeight		5														//步进电机转一圈上升高度	

//机械臂运动算例
extern void MotorBaseMotion (	u16 			mvdis, 
								RevDirection 	dir);
//急停
extern void MotorAxisEmgStew (void);

//测试算例
extern void PeriodUpDnMotion (u16 count);											//滑轨上下测试
extern void RepeatTestMotion (void);												//传感器机械臂反复测试
extern void Axis_Pos_Reset (void);													//开机机械臂复位到零点

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
