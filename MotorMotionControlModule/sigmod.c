#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	电机S形加减速，直接修改启动和停止模式，使用sigmod函数建模
	S形加减速核心：建立开启脉冲和关闭脉冲之间的时间常数，加入对应Y值的频率变化控制
*/

Sigmod_Parameter asp, dsp;

//参数初始化
void sigmodPara_Init (void)
{
	/*
		参数freq_max，设置最高达到频率，但要注意抑制机械振动
		参数freq_min，设置最小换向频率
		参数para_a，越小曲线越平滑
		参数para_b，越大曲线上升下降越缓慢
		参数ratio，S形加减速阶段分化比例
		加减速参数设置相同运行会更加平稳(没有突变)
	*/
	
	u8 i;
	
	//加速段
	asp.freq_max = 2800u;
	asp.freq_min = 1200u;
	asp.para_a = 0.03f;
	asp.para_b = 200.f;
	asp.ratio = 0.4f;
	for (i = 0u; i < Num_Range; i++)
		asp.disp_table[i] = 0u;
	//减速段
	dsp.freq_max = 2800u;
	dsp.freq_min = 1200u;
	dsp.para_a = 0.03f;
	dsp.para_b = 200.f;
	dsp.ratio = 0.1f;
	for (i = 0u; i < Num_Range; i++)
		dsp.disp_table[i] = 0u;
}

//创建离散值数组 程序初始化时调用一次，系统运行时全局保存
void FreqDisperseTable_Create (MotorMotionSetting mc)
{
	u16 num, step_x;																					

	sigmodPara_Init();									//对参数初始化
	
	for (num = 0u, step_x = 0u; num < Num_Range; num++, step_x += X_Range / Num_Range)				
	{
		//加速表
		asp.disp_table[num] = sigmodAlgo(	asp.freq_max, 
											asp.freq_min, 
											asp.para_a, 
											asp.para_b, 
											step_x);	
		//减速表
		dsp.disp_table[num] = sigmodAlgo(	dsp.freq_max, 
											dsp.freq_min, 
											dsp.para_a, 
											dsp.para_b, 
											step_x);	
	}
}

//S形加减速
void SigmodAcceDvalSpeed (void) 		
{
	u8 fp;													//只取0-39
	
	//S曲线加速启动
	for (fp = 0u; fp < Num_Range; fp++)						//遍历频率表	
		st_motorAcfg.SpeedFrequency = asp.disp_table[fp];		
	//匀速
	st_motorAcfg.SpeedFrequency = asp.disp_table[Num_Range - 1];	

	//S曲线减速停止
	for (fp = Num_Range; fp > 0; fp--)		
		st_motorAcfg.SpeedFrequency = dsp.disp_table[fp - 1];				
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
