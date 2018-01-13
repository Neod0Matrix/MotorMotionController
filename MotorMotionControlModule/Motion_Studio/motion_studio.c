#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//所有使用到的基础运动算例和高级运动算例					

//机械臂单独急停
void MotorAxisEmgStew (void)
{	
	//定时器配置关闭
	MotorAxisx_Switch_Off;
	
	//脉冲总线配置关闭
	MotorConfigStrParaInit(&motorx_cfg);
}

//该运动算例包含对步进电机S形加减速的设置，可以在config.c中选择是否使用这一功能
void MotorBaseMotion (	u16 			mvdis, 
						RevDirection 	dir)
{	
	//初始化参数设置，分割设置防止启动干扰
	
	MotorConfigStrParaInit(&motorx_cfg);				//参数清0
	IO_Direction = dir;									//电机转向初始化
	
	//将步进距离转换成圈数，添加软件限位
	if (mvdis < Z_Max)				
		motorx_cfg.distance = mvdis / OneLoopHeight;
	else
		motorx_cfg.distance = Z_Max / OneLoopHeight;
	
	//S形加减速法
	if (SAD_Switch == SAD_Enable)
	{
		if (motorx_cfg.distance != 0
			//传感器初始限位
			&& (	(dir == Pos_Rev && !USrNLTri) 
				|| 	(dir == Nav_Rev && !DSrNLTri))
		)	
			SigmodAcceDvalSpeed(motorx_cfg); 					//调用S形加减速频率-时间-脉冲数控制	
		else 
			MotorAxisEmgStew();
	}
	//匀速法
	else
	{
		//代替S形加减速使用固有换向频率
		//motorx_cfg.Frequency = AutoSettingSpeed;				
		
		if (motorx_cfg.distance != 0 
			//传感器初始限位
			&& (	(dir == Pos_Rev && !USrNLTri) 
				|| 	(dir == Nav_Rev && !DSrNLTri))
		)	
			MotorAxisx_Switch_On;
		else 
			MotorAxisEmgStew();
	}
}

/*
	机械臂上下测试
	传送参数：计数变量(偶数上升，奇数下降)
*/
void PeriodUpDnMotion (u16 count)
{
	//滑轨上下测试，通用传感器长时间触发检测配置
	if (count % 2u == 0u)								//偶数上升
	{
		if (!USrNLTri)								//划定条件范围				
		{
			MotorBaseMotion(MaxLimit_Dis * Distance_Ratio, Pos_Rev);
			WaitForSR_Trigger(ULSR);					//等待传感器长期检测	
			MotorAxisEmgStew();
		}
	}
	else if (count % 2u != 0u)							//奇数下降
	{
		if (!DSrNLTri)								//划定条件范围	
		{
			MotorBaseMotion(MaxLimit_Dis * Distance_Ratio, Nav_Rev);
			WaitForSR_Trigger(DLSR);					//等待传感器长期检测
			MotorAxisEmgStew();
		}
	}
}

//传感器反复测试运动算例
void RepeatTestMotion (void)
{
	u16 repeatCnt = 0u;
	__ShellHeadSymbol__; U1SD("Repeate Test Motion\r\n");//动作类型标志

	//除非产生警报否则一直循环
	while (Return_Error_Type == Error_Clear)							
	{		
		PeriodUpDnMotion(repeatCnt);
		
		//打印循环次数
		__ShellHeadSymbol__; 
		if (SendDataCondition)
		{
			printf("No.%04d Times Repeat Test\r\n", repeatCnt);
			usart1WaitForDataTransfer();		
		}
		
		displaySystemInfo();							//打印系统状态信息
		
		if (repeatCnt >= 1000) repeatCnt = 0;			//计数复位，防止溢出
		repeatCnt++;									//从0计到999
		
		if (STEW_LTrigger) break;						//长按检测急停
			
	}
	
    //总动作完成
	__ShellHeadSymbol__; U1SD("Test Repeat Stop\r\n");
}

/*
	开机自动机械臂复位到零点
	完成这一步机械臂坐标系就建立完成，确定零点，以绝对坐标运动
*/
void Axis_Pos_Reset (void)
{
	//检测是否开启复位功能，且是否处于允许复位的运行状态
	if (Init_Reset_Switch == Reset_Enable && pwsf == JBoot) 
	{
		if (!DSrNLTri)								//起始时判断是否在原位置
		{
			MotorBaseMotion(MaxLimit_Dis * Distance_Ratio, Nav_Rev);//默认以最大运动距离降下，适当调节Distance_Ratio
			WaitForSR_Trigger(DLSR);				//等待传感器长期检测
			MotorAxisEmgStew();						//完成复位立即停止动作
		}
	}		
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
