#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	通用4-5线编码器速度位置反馈
	本API支持4线、5线编码器，若接入4线编码器(自带Z相)则忽略Z相配置
	若接入5线编码器，则需配置Z相脉冲的外部中断和定时器中断
*/

//初始化定时器8输入比较模式记录编码器的反馈脉冲
#define Encoder_Timerx				TIM8
#define	Encoder_RCC_APBxPeriph_TIMx	RCC_APB2Periph_TIM8
//编码器模式设定模式3，上升下降都计数，即输出四倍频信号
#define TIM_EncoderMode_TIWhat		TIM_EncoderMode_TI12
#define TIM_ICPolarity_What			TIM_ICPolarity_BothEdge
//输入滤波器
#define TIM_ICFilter_Number			6
//编码器测速，采样率分频设置，单位us
#define Encoder_SampleTime			20000							
#define EncoderSpeedPrintTime		1000000							

float encodeMesSpeed;
kf_1deriv_factor ecstr;

//相序输入IO初始化
void EncoderPhase_IO_Init (void)
{
	/*
		PC6 -- A phase
		PC7 -- B phase
	*/
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,										
						GPIO_Mode_IN_FLOATING,						//浮空输入模式		
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_6 | GPIO_Pin_7,							
						GPIOC,					
						NI,				
						EBO_Disable);
}

//编码器定时器8输入比较模式初始化配置
void TIM8_EncoderCounter_Config (void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

	RCC_APB2PeriphClockCmd(Encoder_RCC_APBxPeriph_TIMx, ENABLE);	//使能TIM时钟
    TIM_DeInit(Encoder_Timerx);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = ((u16)EncoderLineValue - 1) * 4; 	//编码器线数代入
    TIM_TimeBaseStructure.TIM_Prescaler = 0; 						//时钟预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision =TIM_CKD_DIV1;			//设置时钟分割 
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 	//向上计数
    TIM_TimeBaseInit(Encoder_Timerx, &TIM_TimeBaseStructure);

	//编码器触发模式
    TIM_EncoderInterfaceConfig(	Encoder_Timerx, 
								TIM_EncoderMode_TIWhat, 
								TIM_ICPolarity_What,
								TIM_ICPolarity_What);
    TIM_ICStructInit(&TIM_ICInitStructure);							//将结构体中的内容缺省输入
    TIM_ICInitStructure.TIM_ICFilter = TIM_ICFilter_Number;  		//选择输入比较滤波器
    TIM_ICInit(Encoder_Timerx, &TIM_ICInitStructure);				//将TIM_ICInitStructure中的指定参数初始化

	//TIM_ARRPreloadConfig(Encoder_Timerx, ENABLE);					//使能预装载
    TIM_ClearFlag(Encoder_Timerx, TIM_FLAG_Update);					//清除TIM的更新标志位
    TIM_ITConfig(Encoder_Timerx, TIM_IT_Update, ENABLE);			//运行更新中断 
	TIM_SetCounter(Encoder_Timerx, 0);								//定时器计数器初始
    										
    TIM_Cmd(Encoder_Timerx, ENABLE);   			

	KF_1DerivFactor_Init(&ecstr);									//编码器测速一阶卡尔曼滤波初始化
}

//定时器8中断处理编码器接口模式
void TIM8_IRQHandler (void)
{   
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntEnter();    
#endif
	
    if (TIM_GetFlagStatus(Encoder_Timerx, TIM_FLAG_Update))			
    {
		TIM_ClearITPendingBit(Encoder_Timerx, TIM_IT_Update);
		//若无Z相线接入，则该步骤不起作用
    } 
	
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntEnter();    
#endif
}

//编码器Z相--PC2
void EXTI2_IRQHandler (void)										
{
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntEnter();    
#endif
	
	if (EXTI_GetITStatus(Encoder_Zphase_EXTI_Line) != RESET)  
	{		
		TIM_SetCounter(Encoder_Timerx, 0);							//遇到相对0，计数归零
		TIM_Cmd(Encoder_Timerx, ENABLE);
	}
	EXTI_ClearITPendingBit(Encoder_Zphase_EXTI_Line);				//清除EXTI线路挂起位
	
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntExit();  											 
#endif
}

//读取编码器输出值
u16 EncoderCount_ReadValue (void)
{
	u16 encoder_cnt;
	
	encoder_cnt = TIM_GetCounter(Encoder_Timerx) / 4;				//获取计数值
	__ShellHeadSymbol__; U1SD("Encoder Counter Value: %d\r\n", encoder_cnt);

	return encoder_cnt;
}

//利用算法定时器100us时基源生成20ms采样周期(50Hz采样率)进行测速
float Encoder_MeasureAxisSpeed (MotorMotionSetting *mcstr)
{
	static u16 divFreqSem = 0u, odd_even_flag = 0u;
	//虚拟测速时间点对应的编码器线数x1，x2
	static u16 encoder_cnt_x1 = 0u, encoder_cnt_x2 = 0u, encoder_cnt_dx = 0u;
	static float mes_speed = 0.f;
	
	if (Return_Error_Type == Error_Clear 							//无错误状态
		&& pwsf != JBoot 											//初始化完成状态
		&& globalSleepflag == SysOrdWork 							//非睡眠状态
		&& mcstr -> MotorStatusFlag == Run) 						//电机处于运动状态
	{
		if (divFreqSem++ == TickDivsIntervalus(Encoder_SampleTime) - 1)
		{
			divFreqSem = 0u;
					
			//利用奇偶相隔获取一对计数值，前后相差Encoder_SampleTime
			if (odd_even_flag % 2 == 0)
				encoder_cnt_x2 = TIM_GetCounter(Encoder_Timerx) / 4;							
			else
				encoder_cnt_x1 = TIM_GetCounter(Encoder_Timerx) / 4;
			if (odd_even_flag > 1000u)
				odd_even_flag = 0u;
			odd_even_flag++;
			
			//计算编码器线数差值并取绝对值，与最大线数比较，取最小(以此来排除零位置逆差)
			encoder_cnt_dx = abs(encoder_cnt_x1 - encoder_cnt_x2);
			encoder_cnt_dx = ((u16)EncoderLineValue - encoder_cnt_dx > 
				encoder_cnt_dx)? encoder_cnt_dx:(u16)EncoderLineValue - encoder_cnt_dx;
			
			//抛弃零值缺失
			if (encoder_cnt_dx != 0)
			{
				/*
					计算速度(理想时间假设)，并配置一阶卡尔曼滤波对输出进行数字滤波
					速度的串口输出调试切换到中断服务函数外部进行，避免电机卡顿
				*/
				mes_speed = (encoder_cnt_dx * Encoder_LineTransferRad_Const) 
					/ (float)(Encoder_SampleTime / 1000000.f);		
				mes_speed = Kalman_1DerivFilter(mes_speed, &ecstr);	
			}
		}
	}
	//错误状态和电机静置状态速度清零
	else if (Return_Error_Type != Error_Clear || mcstr -> MotorStatusFlag != Run)
	{
		encoder_cnt_dx = 0;
		mes_speed = 0.f;
	}
	
	return mes_speed;
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
