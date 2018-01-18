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
#define TIMERx_Number 			TIM1					//设置定时器编号，对应电机编号
#define RCC_APBxPeriph_TIMERx 	RCC_APB2Periph_TIM1		//设置定时器挂载总线
#define TIMERx_IRQn				TIM1_CC_IRQn			//通道中断编号
#define MotorChnx				TIM_IT_CC1				//电机通道编号

//声明电机参数结构体
MotorMotionSetting st_motorAcfg;						

//主脉冲IO口初始化
void PulseDriver_IO_Init (void)
{
	//PB0
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOB,
	//如果IO口对应定时器通道，则配置成复用推挽输出，如果是任意IO口则配置成推挽输出
#ifdef UseTimerPWMorOCChannel	
						GPIO_Mode_AF_PP,
#else
						GPIO_Mode_Out_PP,
#endif	
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_0,					
						GPIOB,					
						IHL,				
						EBO_Enable);		
}

//方向线IO初始化
void Direction_IO_Init (void)
{
	//PA6
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOA,										
						GPIO_Mode_Out_PP,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_6,							
						GPIOA,					
						IHL,				
						EBO_Enable);
}

//电机驱动参数结构体初始化
void MotorConfigStrParaInit (MotorMotionSetting *mcstr)
{
	mcstr -> ReversalCnt 		= 0u;					//脉冲计数器
	mcstr -> ReversalRange 		= 0u;					//脉冲回收系数
	mcstr -> RotationDistance 	= 0u;					//行距
	mcstr -> SpeedFrequency 	= 0u;					//设定频率
	mcstr -> divFreqCnt			= 0u;					//分频计数器	
	mcstr -> CalDivFreqConst 	= 0u;					//分频系数
	mcstr -> MotorStatusFlag	= Stew;
	mcstr -> MotorModeFlag		= LimitRun;
	mcstr -> DistanceUnitLS		= RadUnit;	
	mcstr -> RevDirectionFlag	= Pos_Rev;
}

//TIM1作为电机驱动定时器初始化
void TIM1_MecMotorDriver_Init (void)
{
	RCC_Configuration();										
	ucTimerx_InitSetting(	TIMERx_Number, 
							TIMERx_IRQn, 
							RCC_APBxPeriph_TIMERx,
							//重映射GPIO，避免与USART1和OLED I2C冲突(PA9 PA10 PB13 PB15)
							GPIO_FullRemap_TIM1,
							//此处写入定时器的定时值
							TIMarrPeriod, 
							TIMPrescaler, 
							TIM_CKD_DIV1, 
							TIM_CounterMode_Up, 
							irq_Use, 						
							0x03, 
							0x05, 
							ENABLE);
	TIM1_OutputChannelConfig(MotorChnx, ENABLE);		//配置TIM1通道
	MotorConfigStrParaInit(&st_motorAcfg);				//参数初始化
}	

//电机驱动库初始化函数合并，对main函数接口
void MotorDriverLib_Init (void)
{
	PulseDriver_IO_Init();								//脉冲IO口
	Direction_IO_Init();								//方向IO口
	TIM1_MecMotorDriver_Init();							//脉冲发生定时器
	FreqDisperseTable_Create(st_motorAcfg);				//加减速表生成
}

/*
	定时器1作为电机控制定时器配置
	传参：电机对应定时器通道，使能开关
*/
void TIM1_OutputChannelConfig (uint16_t Motorx_CCx, FunctionalState control)
{
	TIM_OCInitTypeDef TIM_OCInitStructure; 
	TIM_OCStructInit(&TIM_OCInitStructure);
	
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle; //管脚输出模式：翻转
	//TIM_OCInitStructure.TIM_Pulse = (TIMarrPeriod - 1) / 2;
	
#ifdef PosLogicOperation								//正逻辑
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//使能正向通道  
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;	//高有效 输出为正逻辑
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;//空闲状态下的非工作状态
#else													//负逻辑
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;//使能反向通道	 
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;	//平时为高，脉冲为低 输出为正逻辑
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset; 
#endif
	
	//通道选配
	switch (Motorx_CCx)
	{
	case MotorChnx:
		TIM_OC1Init(TIMERx_Number, &TIM_OCInitStructure);  //写入配置 
		TIM_OC1PreloadConfig(TIMERx_Number, TIM_OCPreload_Disable);
#ifdef UseTimerPWMorOCChannel							//使能TIMx在CCRx上的预装载寄存器(与通道IO挂钩)
		TIM_OC1PreloadConfig(TIMERx_Number, TIM_OCPreload_Enable);
#endif
		TIM_SetCompare1(TIMERx_Number, TimerInitCounterValue);
		break;
	//以下可扩展
	}
  
    TIM_ClearFlag(TIMERx_Number, Motorx_CCx);			//清中断
	TIM_ARRPreloadConfig(TIMERx_Number, ENABLE);		//使能TIMx在ARR上的预装载寄存器，如果中断中不修改ARR值则此函数无影响
    TIM_ITConfig(TIMERx_Number, Motorx_CCx, control);	//TIMx中断源设置，开启相应通道的捕捉比较中断
}

//更新行距计算
void DistanceAlgoUpdate (MotorMotionSetting *mcstr)
{
	switch (mcstr -> DistanceUnitLS)
	{
	case RadUnit: 	mcstr -> ReversalRange = 2 * RadUnitConst * mcstr -> RotationDistance - 1u; 	break;
	case LineUnit: 	mcstr -> ReversalRange = 2 * LineUnitConst * mcstr -> RotationDistance - 1u; 	break;
	}
}

//电机中断
void MotorPulseProduceHandler (MotorMotionSetting *mcstr)
{
    if (TIM_GetITStatus(TIMERx_Number, MotorChnx) == SET)	//电机中断标志置位
    {	
		TIM_ClearITPendingBit(TIMERx_Number, MotorChnx);
		
		//脉冲自动完成
		if (mcstr -> ReversalCnt == mcstr -> ReversalRange && mcstr -> MotorModeFlag != UnlimitRun)		
		{
			TIM_CtrlPWMOutputs(TIMERx_Number, DISABLE);	//通道输出关闭
			TIM_Cmd(TIMERx_Number, DISABLE);			//TIM1关闭
			IO_MainPulse = MD_IO_Reset;
			mcstr -> MotorStatusFlag = Stew;			//标志复位
			__ShellHeadSymbol__; U1SD("MotorDriver Has Finished Work\r\n");
			return;										//函数遇到return将结束
		}
		//分频产生对应的脉冲频率
		if (++mcstr -> divFreqCnt == mcstr -> CalDivFreqConst)
		{
			mcstr -> divFreqCnt = 0;
			IO_MainPulse = !IO_MainPulse;			
			mcstr -> ReversalCnt++;						//对脉冲计数
		}
    }
}

//定时器1中断服务
void TIM1_CC_IRQHandler (void)
{
#if SYSTEM_SUPPORT_OS
	OSIntEnter();
#endif
	
	//仅在无错误状态下使能
	if (Return_Error_Type == Error_Clear)						
		MotorPulseProduceHandler(&st_motorAcfg);
	
#if SYSTEM_SUPPORT_OS
	OSIntExit();    
#endif
}

//电机驱动
//传参：电机编号，结构体频率，结构体距离，使能开关
void MotorMotionDriver (MotorMotionSetting *mcstr, FunctionalState control)
{	
	if (control == ENABLE)
	{
		//外部完成计算转储
		if (mcstr -> SpeedFrequency != 0)
			mcstr -> CalDivFreqConst = DivFreqConst(mcstr -> SpeedFrequency);
		if (mcstr -> RotationDistance != 0)
			DistanceAlgoUpdate(mcstr);
		
		//报警状态不可驱动电机运转		
		if (mcstr -> CalDivFreqConst != 0 && mcstr -> ReversalRange != 0 && Return_Error_Type == Error_Clear)								
		{					
			//计数器初始化
			mcstr -> ReversalCnt = 0;			
			mcstr -> divFreqCnt	= 0;
			
			//开关使能
			TIM_CtrlPWMOutputs(TIMERx_Number, control);	//通道输出
			TIM_Cmd(TIMERx_Number, control);			//TIMER使能选择
			
			mcstr -> MotorStatusFlag = Run;
		}
	}
	else
	{
		//开关使能
		TIM_CtrlPWMOutputs(TIMERx_Number, control);		//通道输出
		TIM_Cmd(TIMERx_Number, control);				//TIMER使能选择
		
		IO_MainPulse = MD_IO_Reset;
		
		//参量更新
		mcstr -> ReversalCnt = mcstr -> ReversalRange;	
		mcstr -> MotorStatusFlag = Stew;		
		mcstr -> divFreqCnt	= 0u;	
		__ShellHeadSymbol__; U1SD("MotorDriver Emergency Stop\r\n");
	}
}

//模块同名函数，基准调用
void MotorMotionController (u16 spfq, u16 mvdis, RevDirection dir, 
	MotorRunMode mrm, LineRadSelect lrs, MotorMotionSetting *mcstr)
{	
	//电机转向初始化
	mcstr -> RevDirectionFlag = dir;
	IO_Direction = mcstr -> RevDirectionFlag;	
	mcstr -> SpeedFrequency = spfq;
	mcstr -> MotorModeFlag = mrm;
	mcstr -> DistanceUnitLS = lrs;
	
	//仅在非无限脉冲模式下对线度进行限制
	if (mrm != UnlimitRun && lrs == LineUnit)
	{
		if (mvdis < MaxLimit_Dis)				
			mcstr -> RotationDistance = mvdis;
		else
			mcstr -> RotationDistance = MaxLimit_Dis;
	}
	else
		mcstr -> RotationDistance = mvdis;
	
	//S形加减速法
	if (SAD_Switch == SAD_Enable)
	{
		//传感器初始限位
		if ((dir == Pos_Rev && !USrNLTri) || (dir == Nav_Rev && !DSrNLTri))
			//调用S形加减速频率-时间-脉冲数控制	
			SigmodAcceDvalSpeed();						
		else 
			MotorMotionDriver(mcstr, DISABLE);
	}
	//匀速法
	else
	{
		//代替S形加减速使用固有换向频率	
		if ((dir == Pos_Rev && !USrNLTri) || (dir == Nav_Rev && !DSrNLTri))
			MotorMotionDriver(mcstr, ENABLE);
		else 
			MotorMotionDriver(mcstr, DISABLE);
	}
}

/*
	滑轨上下测试
	传送参数：计数变量(偶数上升，奇数下降)
*/
void PeriodUpDnMotion (u16 count, MotorMotionSetting *mcstr)
{
	//滑轨上下测试，通用传感器长时间触发检测配置
	if (count % 2u == 0u && !USrNLTri)					//偶数上升
	{
		MotorMotionController(mcstr -> SpeedFrequency, MaxLimit_Dis, Pos_Rev, LimitRun, LineUnit, mcstr);
		WaitForSR_Trigger(ULSR);						//等待传感器长期检测	
		MotorMotionDriver(&st_motorAcfg, DISABLE);
	}
	else if (count % 2u != 0u && !DSrNLTri)				//奇数下降
	{
		MotorMotionController(mcstr -> SpeedFrequency, MaxLimit_Dis, Nav_Rev, LimitRun, LineUnit, mcstr);
		WaitForSR_Trigger(DLSR);						//等待传感器长期检测
		MotorMotionDriver(&st_motorAcfg, DISABLE);
	}
}

//传感器反复测试运动算例
void RepeatTestMotion (MotorMotionSetting *mcstr)
{
	u16 repeatCnt = 0u;
	__ShellHeadSymbol__; U1SD("Repeate Test Motion\r\n");//动作类型标志

	//除非产生警报否则一直循环
	while (Return_Error_Type == Error_Clear)							
	{		
		PeriodUpDnMotion(repeatCnt, mcstr);
		
		//打印循环次数
		__ShellHeadSymbol__; 
		if (SendDataCondition)
		{
			printf("No.%04d Times Repeat Test\r\n", repeatCnt);
			usart1WaitForDataTransfer();		
		}
		
		displaySystemInfo();							//打印系统状态信息
		
		if (++repeatCnt >= 1000) 
			repeatCnt = 0;			
		
		if (STEW_LTrigger) break;						//长按检测急停
			
	}
    //总动作完成
	__ShellHeadSymbol__; U1SD("Test Repeat Stop\r\n");
}

/*
	开机自动机械臂复位到零点
	完成这一步机械臂坐标系就建立完成，确定零点，以绝对坐标运动
	如果不构建绝对坐标系就没有必要使能该函数
*/
void Axis_Pos_Reset (MotorMotionSetting *mcstr)
{
	//检测是否开启复位功能，且是否处于允许复位的运行状态
	if (Init_Reset_Switch == Reset_Enable && pwsf == JBoot) 
	{
		mcstr -> SpeedFrequency = ResetStartFrequency;	//赋给起始频率
		if (!DSrNLTri)									//起始时判断是否在原位置
		{
			MotorMotionController(mcstr -> SpeedFrequency, MaxLimit_Dis, Nav_Rev, LimitRun, LineUnit, mcstr);
			WaitForSR_Trigger(DLSR);					//等待传感器长期检测
			MotorMotionDriver(&st_motorAcfg, DISABLE);	//完成复位立即停止动作
		}
	}		
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
