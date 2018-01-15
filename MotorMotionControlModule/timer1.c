#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	启用高级定时器1对脉冲/PWM进行规划
	这里配给电机脉冲的都是高级定时器的比较输出模式
*/

//定时器配置
//注意高级定时器18挂载在APB2总线上，通用定时器2345挂载在APB1总线上
#define TIMERx_Number 					TIM1					//设置定时器编号，对应电机编号
#define RCC_APBxPeriph_TIMERx 			RCC_APB2Periph_TIM1		//设置定时器挂载总线
#define TIMERx_IRQn						TIM1_UP_IRQn			//通道中断编号，配置为更新中断
#define MotorChnx						TIM_IT_CC1				//电机通道编号

//声明电机参数结构体
Motorx_CfgPara motorx_cfg;						
//声明电机定时器参数结构体
TimCalcul_FreqPara 	mAx_Timer;

//电机驱动参数结构体初始化
void MotorConfigStrParaInit (Motorx_CfgPara *mcstr)
{
	mcstr -> pulse_counter 	= 0u;
	mcstr -> distance 		= 0u;
	mcstr -> Frequency 		= 0u;
	mcstr -> freqlive_stage = 0u;
}

//TIM1作为电机驱动定时器初始化
void TIM1_MecMotorDriver_Init (void)
{
	RCC_Configuration();										//这里时钟选择为APB2的2倍，而APB2为36M，系统设置2倍频，TIM输入频率72Mhz
	ucTimerx_InitSetting(	TIMERx_Number, 
							TIMERx_IRQn, 
							RCC_APBxPeriph_TIMERx,
							//重映射GPIO，避免与USART1和OLED I2C冲突(PA9 PA10 PB13 PB15)
							GPIO_FullRemap_TIM1,
							TIMarrPeriod, 
							TIMPrescaler, 
							TIM_CKD_DIV1, 
							TIM_CounterMode_Up, 
							irq_Use, 						
							0x03, 
							0x05, 
							ENABLE); 
}	

//定时器1输出比较模式
//传参：电机翻转周期结构体，电机对应定时器通道，使能开关
void Motor_TIM1_ITConfig (uint16_t tp, uint16_t Motorx_CCx, FunctionalState control)
{
	TIM_OCInitTypeDef TIM_OCInitStructure; 
	TIM_OCStructInit(&TIM_OCInitStructure);
	
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;         //管脚输出模式：翻转
    TIM_OCInitStructure.TIM_Pulse = tp;   						//翻转周期，频率调速器
	
#ifdef PosLogicOperation										//正逻辑
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//使能正向通道  
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	//高有效 输出为正逻辑
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;//空闲状态下的非工作状态
#else															//负逻辑
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;//使能反向通道	 
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;	//平时为高，脉冲为低 输出为正逻辑
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset; 
#endif
	
	//通道选配
	switch (Motorx_CCx)
	{
	case MotorChnx:
		TIM_OC1Init(TIMERx_Number, &TIM_OCInitStructure);       //写入配置 
		TIM_OC1PreloadConfig(TIMERx_Number, TIM_OCPreload_Disable);
#ifdef UseTimerPWMorOCChannel									//使能TIMx在CCRx上的预装载寄存器(与通道IO挂钩)
		TIM_OC1PreloadConfig(TIMERx_Number, TIM_OCPreload_Enable);
#endif
		TIM_SetCompare1(TIMERx_Number, TimerInitCounterValue);
		break;
	//以下可扩展
	}
  
    TIM_ClearFlag(TIMERx_Number, Motorx_CCx);					//清中断
	TIM_ARRPreloadConfig(TIMERx_Number, ENABLE);				//使能TIMx在ARR上的预装载寄存器
    TIM_ITConfig(TIMERx_Number, Motorx_CCx, control);			//TIMx中断源设置，开启相应通道的捕捉比较中断
}

//电机中断
void motorAxisx_IRQHandler (void)
{
    if (TIM_GetITStatus(TIMERx_Number, MotorChnx) == SET)		//电机中断标志置位
    {	
		TIM_ClearITPendingBit(TIMERx_Number, MotorChnx);
		
		//电机IO口翻转，产生脉冲(如果使用原配IO口就不需要这一步)
		mAx_Timer.togglePeriod = TimTPArr500k(motorx_cfg.Frequency);
		IO_MainPulse = !IO_MainPulse;										
		motorx_cfg.pulse_counter++;						
		TIM_SetCompare1(TIMERx_Number, TIM_GetCapture1(TIMERx_Number) + mAx_Timer.togglePeriod);
		
		if (motorx_cfg.pulse_counter >= PulseSumCalicus(Pulse_per_Loop, motorx_cfg.distance))//这里直接取全局变量
		{
			IO_MainPulse = MD_IO_Reset;
			
			TIM_CtrlPWMOutputs(TIMERx_Number, DISABLE);			//通道输出关闭
			TIM_Cmd(TIMERx_Number, DISABLE);					//TIM1关闭
			
			MotorConfigStrParaInit(&motorx_cfg);
			
			return;												//函数遇到return将结束
		}
    }
}

//定时器1输出比较模式中断服务
void TIM1_CC_IRQHandler (void)
{
#if SYSTEM_SUPPORT_OS
	OSIntEnter();
#endif
	
	/*
		使用高级定时器比较输出模式发脉冲，在脉冲发送过程中
		程序会一直卡在中断服务函数中直到发送结束
	*/
	if (Return_Error_Type == Error_Clear)						//仅在无错误状态下使能
		motorAxisx_IRQHandler();
	
#if SYSTEM_SUPPORT_OS
	OSIntExit();    
#endif
}

//电机驱动
//传参：电机编号，结构体频率，结构体距离，使能开关
FunctionalState MotorMotionDriver (	u16 			frequency, 
									float 			Distance, 
									FunctionalState control)
{
	//截止状态
	if (frequency == 0u || Return_Error_Type != Error_Clear)	//报警状态不可驱动电机运转									
	{		
		//更新配置，实际是更新翻转周期	
												
		Motor_TIM1_ITConfig(mAx_Timer.togglePeriod, MotorChnx, DISABLE);
		IO_MainPulse = MD_IO_Reset;
		
		//翻转计数清0
		mAx_Timer.toggleCount = TimerInitCounterValue;
		
		return DISABLE;											//到此跳出函数
	}
	
	TIM_SetCounter(TIMERx_Number, TimerInitCounterValue);		//计数清0				
	
	//更新配置
	mAx_Timer.toggleCount = PulseSumCalicus(Pulse_per_Loop, Distance);
	mAx_Timer.togglePeriod = TimTPArr500k(motorx_cfg.Frequency);	
	Motor_TIM1_ITConfig(mAx_Timer.togglePeriod, MotorChnx, control);
		
	//开关使能
	TIM_CtrlPWMOutputs(TIMERx_Number, control);					//通道输出
	TIM_Cmd(TIMERx_Number, control);							//TIMER使能选择
	
	return ENABLE;												//通过返回值判断电机状态
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
