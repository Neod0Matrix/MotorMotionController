#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	步进电机S形加减速，直接修改启动和停止模式，使用sigmod函数建模
	S形加减速核心：建立开启脉冲和关闭脉冲之间的时间常数，加入对应Y值的频率变化控制
	初始化定时器3用于管理S形加减速时域
	需要考虑到资源占用的问题
*/

//注意高级定时器18挂载在APB2总线上，通用定时器2345挂载在APB1总线上
#define Sigmod_Timerx 			TIM3						//用于计算S形加减速时间段的定时器
#define RCC_APBxPeriph_TIMERx 	RCC_APB1Periph_TIM3			//设置定时器挂载总线
#define Timerx_IRQn				TIM3_IRQn					//终端编号
//1ms定时
#define Timerx_TogglePeriod		((uint16_t)71)				//定时器自动重装翻转周期
#define Timerx_Prescaler		999u						//定时器分频器		

void PulseDriver_IO_Init (void)
{
	//PC7
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,
	//如果IO口对应定时器通道，则配置成复用推挽输出，如果是任意IO口则配置成推挽输出
#ifdef UseTimerPWMorOCChannel	
						GPIO_Mode_AF_PP,
#else
						GPIO_Mode_Out_PP,
#endif	
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_7,					
						GPIOC,					
						IHL,				
						EBO_Enable);		
}

//方向线 IO初始化
void Direction_IO_Init (void)
{
	//PC9
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,										
						GPIO_Mode_Out_PP,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_9,							
						GPIOC,					
						IHL,				
						EBO_Enable);
}

//电机驱动库初始化函数合并
void MotorDriverLib_Init (void)
{
	TIM1_MecMotorDriver_Init();								//脉冲发生定时器
	TIM3_SigmodSysTick_Init(DISABLE);						//间隔定时器
	PulseDriver_IO_Init();									//脉冲IO口
	Direction_IO_Init();									//方向IO口
	FreqDisperseTable_Create(motorx_cfg);					//加减速表生成
}

//初始化定时器3获取时间(初始化完成后关闭)
void TIM3_SigmodSysTick_Init (FunctionalState control)  
{  
	ucTimerx_InitSetting(	Sigmod_Timerx, 
							Timerx_IRQn, 
							RCC_APBxPeriph_TIMERx, 
							TIMx_GPIO_Remap_NULL,
							Timerx_TogglePeriod, 
							Timerx_Prescaler, 
							TIM_CKD_DIV1, 
							TIM_CounterMode_Up, 
							irq_Use, 						
							0x04, 
							0x02, 
							control);
}  

//定时器3中断服务，记录频率点的生存时间
void TIM3_IRQHandler (void)
{
#if SYSTEM_SUPPORT_OS										//OS支持
	OSIntEnter();
#endif
	
	if (TIM_GetITStatus(Sigmod_Timerx, TIM_IT_Update) != RESET)//检查指定的TIM中断发生与否
	{
		TIM_ClearITPendingBit(Sigmod_Timerx, TIM_IT_Update);//清除TIMx的中断待处理位
		
		motorx_cfg.freqlive_stage++;						//频率变换计数
	}
	
#if SYSTEM_SUPPORT_OS										//OS支持
	OSIntExit();    
#endif
}

//定时器时间获取
u16 TIMx_GetNowTime (void)
{
	//通过取中断计数得到总的运行时间，单位ms
#define BasicTimerResult	TimeCalcusofucTimer(Timerx_TogglePeriod, Timerx_Prescaler) * 1000u

	return (u16)motorx_cfg.freqlive_stage * BasicTimerResult;
}

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
	motorx_cfg.asp.freq_max = 2800u;
	motorx_cfg.asp.freq_min = 1200u;
	motorx_cfg.asp.para_a = 0.03f;
	motorx_cfg.asp.para_b = 200.f;
	motorx_cfg.asp.ratio = 0.4f;
	for (i = 0u; i < Num_Range; i++)
		motorx_cfg.asp.disp_table[i] = 0u;
	//减速段
	motorx_cfg.dsp.freq_max = 2800u;
	motorx_cfg.dsp.freq_min = 1200u;
	motorx_cfg.dsp.para_a = 0.03f;
	motorx_cfg.dsp.para_b = 200.f;
	motorx_cfg.dsp.ratio = 0.1f;
	for (i = 0u; i < Num_Range; i++)
		motorx_cfg.dsp.disp_table[i] = 0u;
}

//创建离散值数组 程序初始化时调用一次，系统运行时全局保存
void FreqDisperseTable_Create (MotorMotionSetting mc)
{
	u16 num, step_x;																					

	sigmodPara_Init();									//对参数初始化
	
	for (num = 0u, step_x = 0u; num < Num_Range; num++, step_x += X_Range / Num_Range)				
	{
		//加速表
		mc.asp.disp_table[num] = sigmodAlgo(mc.asp.freq_max, 
											mc.asp.freq_min, 
											mc.asp.para_a, 
											mc.asp.para_b, 
											step_x);	
		//减速表
		mc.dsp.disp_table[num] = sigmodAlgo(mc.dsp.freq_max, 
											mc.dsp.freq_min, 
											mc.dsp.para_a, 
											mc.dsp.para_b, 
											step_x);	
	}
	//动态内存调用
	stackOverFlow(mc.asp.disp_table);
	stackOverFlow(mc.dsp.disp_table);
}

/*
	运行阶段
	传参：运行阶段，控制结构体
*/
void WaitPulseRespond (MotorRunStage rs, MotorMotionSetting mc)
{
	TIM_Cmd(Sigmod_Timerx, DISABLE);
	
	mc.freqlive_stage = 0u;						//频率变换清0	
	MotorAxisx_Switch_On;
	TIM_Cmd(Sigmod_Timerx, ENABLE);	
	
	switch (rs)
	{
	case as: while (TIMx_GetNowTime() < StageTimeDelay(PulseWholeNbr(mc.distance, 
				mc.asp.ratio) / Num_Range, mc.Frequency)){} break;
	case us: while (TIMx_GetNowTime() < StageTimeDelay(PulseWholeNbr(mc.distance, 
				(1.f - (mc.asp.ratio + mc.dsp.ratio))), mc.Frequency)){} break;
	case ds: while (TIMx_GetNowTime() < StageTimeDelay(PulseWholeNbr(mc.distance, 
				mc.dsp.ratio) / Num_Range, mc.Frequency)){} break;
	}
}

//S形加减速
void SigmodAcceDvalSpeed (MotorMotionSetting mc) 		
{
	u8 fp;													//只取0-39
	
	//S曲线加速启动
	for (fp = 0u; fp < Num_Range; fp++)						//遍历频率表	
	{
		mc.Frequency = mc.asp.disp_table[fp];		
		WaitPulseRespond(as, mc);
	}
	//匀速
	mc.Frequency = mc.asp.disp_table[Num_Range - 1];	
	WaitPulseRespond(us, mc);
	//S曲线减速停止
	for (fp = Num_Range; fp > 0; fp--)		
	{
		mc.Frequency = mc.dsp.disp_table[fp - 1];				
		WaitPulseRespond(ds, mc);
	}
	
	TIM_Cmd(Sigmod_Timerx, DISABLE);						//关闭定时器
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
