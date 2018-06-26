#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//两相(ABZ)5线编码器速度位置反馈

//初始化定时器4输入比较模式记录编码器的反馈脉冲

//相序输入IO初始化
void EncoderPhase_IO_Init (void)
{
	//PC0 A, PC1 B
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,										
						GPIO_Mode_IN_FLOATING,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_0 | GPIO_Pin_1,							
						GPIOC,					
						NI,				
						EBO_Disable);
}

//定时器4输入比较模式初始化配置
void TIM4_EncoderCounter_Config (void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);		//使能TIM4时钟
    TIM_DeInit(TIM4);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = EncoderLineValue * 4;  	//编码器线数四倍
    TIM_TimeBaseStructure.TIM_Prescaler = 0; 					//TIM4时钟预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision =TIM_CKD_DIV1;		//设置时钟分割 T_dts = T_ck_int
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_BothEdge ,TIM_ICPolarity_BothEdge);//使用编码器模式3，上升下降都计数
    TIM_ICStructInit(&TIM_ICInitStructure);						//将结构体中的内容缺省输入
    TIM_ICInitStructure.TIM_ICFilter = 6;  						//选择输入比较滤波器
    TIM_ICInit(TIM4, &TIM_ICInitStructure);						//将TIM_ICInitStructure中的指定参数初始化TIM3

//	TIM_ARRPreloadConfig(TIM4, ENABLE);							//使能预装载
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);						//清除TIM4的更新标志位
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);					//运行更新中断
 
    TIM4 -> CNT = 0;											//定时器计数器初始
    TIM_Cmd(TIM4, ENABLE);   									//启动TIM4定时器
}

//读取编码器输出值
u16 EncoderCount_ReadValue (void)
{
	u16 encoder_cnt;
	
	encoder_cnt = TIM4 -> CNT / 4;								//获取计数值
	//TODO: 接下来做数据转换为可读值
	__ShellHeadSymbol__; U1SD("[!!TEST!!]Encoder Counter Value: %d\r\n", encoder_cnt);

	return encoder_cnt;
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
