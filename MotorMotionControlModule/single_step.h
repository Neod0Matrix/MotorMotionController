#ifndef __SINGLE_STEP_H__
#define __SINGLE_STEP_H__
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//单步调试

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

extern void U1RSD_example (void);				//串口处理例程封装
extern Motion_Select SingleStepDebug_linker (void);//上层封装单步调试调用链接库

#endif

//====================================================================================================
//code by </MATRIX>@Neod Anderjon

