#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//两相(ABZ)5线编码器速度位置反馈

//初始化定时器8输入比较模式记录编码器的反馈脉冲
#define Encoder_Timerx				TIM8
#define	Encoder_RCC_APBxPeriph_TIMx	RCC_APB2Periph_TIM8
//编码器模式设定模式3，上升下降都计数
#define TIM_EncoderMode_TIWhat		TIM_EncoderMode_TI12
#define TIM_ICPolarity_What			TIM_ICPolarity_BothEdge
//输入滤波器
#define TIM_ICFilter_Number			6

//相序输入IO初始化
void EncoderPhase_IO_Init (void)
{
	//PC6 A, PC7 B
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,										
						GPIO_Mode_IN_FLOATING,					
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
    TIM_TimeBaseStructure.TIM_Period = (EncoderLineValue - 1) * 4; 	//编码器线数代入
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
}

//读取编码器输出值
u16 EncoderCount_ReadValue (void)
{
	u16 encoder_cnt;
	
	encoder_cnt = TIM_GetCounter(Encoder_Timerx) / 4;				//获取计数值
	//TODO: 接下来做数据转换为可读值
	__ShellHeadSymbol__; U1SD("Encoder Counter Value: %d\r\n", encoder_cnt);

	return encoder_cnt;
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
