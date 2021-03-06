#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	启用高级定时器1对脉冲/PWM进行规划
	这里配给电机脉冲的都是高级定时器的比较输出模式
	关于分频的特性，分频数越大，频率越小，反过来分频数越小频率越大
	定时时基越小，高频越准，定时时基越大，低频越准
*/

//定时器配置
//注意高级定时器18挂载在APB2总线上，通用定时器2345挂载在APB1总线上
#define TIMERx_Number 				TIM1				//设置定时器编号，对应电机编号
#define RCC_APBxPeriph_TIMERx 		RCC_APB2Periph_TIM1	//设置定时器挂载总线
#define TIMERx_IRQn					TIM1_CC_IRQn		//通道中断编号
#define MotorChnx					TIM_IT_CC1			//电机通道编号
//定时器设置参数
#define DriverDivision				8					//细分数
/*
	arr 自动重装值，最大捕获范围0xFFFF
	根据TB6560步进电机驱动器特性，细分数越大，可以接受的频率上限就越大
*/
#if DriverDivision >= 8									//8个细分及其以上设置到5us
#define TIMarrPeriod				9					
#elif DriverDivision <= 4								//4个细分及其以下设置到50us
#define TIMarrPeriod				99
#endif
#define TIMPrescaler				35					//psc 时钟预分频数
#define TargetTimeBase				TimeCalcusofucTimer(TIMarrPeriod, TIMPrescaler)//定时器单个目标定时时基，单位us
#define FreqMaxThreshold			500000L				//频率计数器上限阈值

//声明电机参数结构体
MotorMotionSetting st_motorAcfg;						

//主脉冲IO口初始化
void PulseDriver_IO_Init (void)
{
	//PA7
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOA,
	//如果IO口对应定时器通道，则配置成复用推挽输出，如果是任意IO口则配置成推挽输出
#ifdef UseTimerPWMorOCChannel	
						GPIO_Mode_AF_PP,
#else
						GPIO_Mode_Out_PP,
#endif	
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_7,					
						GPIOA,					
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

//TIM1作为电机驱动定时器初始化
void TIM1_MecMotorDriver_Init (void)
{				
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
							0x04, 
							ENABLE);
	TIM1_OutputChannelConfig(MotorChnx, ENABLE);		//配置TIM1通道
	MotorConfigStrParaInit(&st_motorAcfg);				//参数初始化
	//IO初始化拉高
	IO_MainPulse = MD_IO_Reset;
	IO_Direction = MD_IO_Reset;
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
	TIM_OCInitStructure.TIM_Pulse = (TIMarrPeriod - 1) / 2;
	
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
		//通道OC1
		TIM_OC1Init(TIMERx_Number, &TIM_OCInitStructure);//为定时器写入通道配置 
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

//S型加减速参数初始化，每次电机启动时调用更新
void SigmoidParam_Init (MotorMotionSetting *mcstr)
{
	u16 i;
	
	float step_x, x_interval;	
	
	/*
		对参数初始化:
		freq_max	设置最高换向频率，必须大于freq_min
		freq_min	设置最小换向频率
		para_a		sigmoid参数A，越小曲线越平滑
		para_b		sigmoid参数B，越大曲线上升下降越缓慢
		ratio		S形加减速分段区间比例(加减速段占比越大行程误差越大)
		table_index 离散表指向序列
		table_size	离散表大小，x取值个数，根据不同的行程取值个数会影响精度
		x_range		x取值范围，越大曲线越平滑，不会影响精度
	*/
	//加速段
	mcstr -> asp -> freq_max 	= mcstr -> SpeedFrequency;
	mcstr -> asp -> freq_min 	= mcstr -> asp -> freq_max / 10;	
	mcstr -> asp -> para_a 		= 0.02f;						//重复测试后发现第140组数据会在这里溢出
	mcstr -> asp -> para_b 		= 250.f;
	mcstr -> asp -> ratio 		= 0.05f;
	mcstr -> asp -> table_index = 0u;
	
	//根据行距切换X取值个数(玄学测量统计分析)
	if (mcstr -> RotationDistance > 0 && mcstr -> RotationDistance <= 360)
		mcstr -> asp -> table_size = R360;
	else if (mcstr -> RotationDistance > 360 && mcstr -> RotationDistance <= 1080)
		mcstr -> asp -> table_size = R1080;
	else if (mcstr -> RotationDistance > 1080 && mcstr -> RotationDistance <= 1800)
		mcstr -> asp -> table_size = R1800;
	else if (mcstr -> RotationDistance > 1800 && mcstr -> RotationDistance <= 3600)
		mcstr -> asp -> table_size = R3600;
	else
		mcstr -> asp -> table_size = R3600;						//大于3600度没有测出合适值
	__ShellHeadSymbol__; U1SD("Setting S-AD Function X Count: %d\r\n", mcstr -> asp -> table_size);
	
	mcstr -> asp -> x_range		= 1600.f;
	mcstr -> asp -> disp_table 	= (u16 *)mymalloc(sizeof(u16) * mcstr -> asp -> table_size);
	
	//减速段
	mcstr -> dsp -> freq_max 	= mcstr -> asp -> freq_max;		//加减速最大频率相同，即匀速频率
	mcstr -> dsp -> freq_min 	= mcstr -> dsp -> freq_max / 10;		
	mcstr -> dsp -> para_a 		= 0.02f;
	mcstr -> dsp -> para_b 		= 250.f;
	mcstr -> dsp -> ratio 		= 0.05f;
	mcstr -> dsp -> table_index = 0u;
	mcstr -> dsp -> table_size 	= mcstr -> asp -> table_size;
	mcstr -> dsp -> x_range		= mcstr -> asp -> x_range;
	mcstr -> dsp -> disp_table 	= (u16 *)mymalloc(sizeof(u16) * mcstr -> dsp -> table_size);
	
	//依次塞入y值(离散频率点)
	x_interval = mcstr -> asp -> x_range / mcstr -> asp -> table_size;
	for (i = 0u, step_x = 0.f; i < mcstr -> asp -> table_size; ++i, step_x += x_interval)				
	{
		//加速表
		mcstr -> asp -> disp_table[i] = SIGMOID_FUNCTION(
			mcstr -> asp -> freq_max, 
			mcstr -> asp -> freq_min, 
			mcstr -> asp -> para_a, 
			mcstr -> asp -> para_b, 
			step_x);	
	}
	x_interval = mcstr -> dsp -> x_range / mcstr -> dsp -> table_size;
	for (i = 0u, step_x = 0.f; i < mcstr -> dsp -> table_size; ++i, step_x += x_interval)				
	{
		//减速表
		mcstr -> dsp -> disp_table[mcstr -> dsp -> table_size - i - 1] = SIGMOID_FUNCTION(
			mcstr -> dsp -> freq_max, 
			mcstr -> dsp -> freq_min, 
			mcstr -> dsp -> para_a, 
			mcstr -> dsp -> para_b, 
			step_x);	
	}
	
	/*
	//打印加减速表测试，用于确认频率点结果
	__ShellHeadSymbol__; U1SD("Print Test [Accel] Sigmoid Value: \r\n");
	for (i = 0u; i < mcstr -> asp -> table_size; ++i)
		U1SD("%dHz\t", mcstr -> asp -> disp_table[i]);
	U1SD("\r\n");
	__ShellHeadSymbol__; U1SD("Print Test [D-value] Sigmoid Value: \r\n");
	for (i = 0u; i < mcstr -> dsp -> table_size; ++i)
		U1SD("%dHz\t", mcstr -> dsp -> disp_table[i]);
	U1SD("\r\n");
	*/
}

//电机驱动参数结构体初始化
//电机停止时会调用一次初始化复位所有参数
void MotorConfigStrParaInit (MotorMotionSetting *mcstr)
{
	TIM_CtrlPWMOutputs(TIMERx_Number, DISABLE);			//通道输出关闭
	TIM_Cmd(TIMERx_Number, DISABLE);					//TIM关闭
	IO_MainPulse = MD_IO_Reset;							//IO置位
	
	mcstr -> ReversalCnt 		= 0u;					//脉冲计数器
	mcstr -> IndexCnt			= 0u;					//序列计数器
	mcstr -> ReversalRange 		= 0u;					//脉冲回收系数				
	memset((void *)mcstr -> IndexRange, 0u, sizeof(mcstr -> IndexRange));//序列回收系数
	mcstr -> RotationDistance 	= 0u;					//行距
	mcstr -> SpeedFrequency 	= 0u;					//设定频率
	mcstr -> divFreqCnt			= 0u;					//分频计数器	
	mcstr -> CalDivFreqConst 	= 0u;					//分频系数
	//频率矫正系数(此处仅提供标准主频和最高主频对应的矫正系数)
	if (rcc_main_freq == 72000000UL)
		mcstr -> DivCorrectConst = 0.9f;
	else if (rcc_main_freq == 128000000UL)
		mcstr -> DivCorrectConst = 0.64f;
	mcstr -> MotorStatusFlag	= Stew;					//开启状态停止
	mcstr -> MotorModeFlag		= PosiCtrl;				//位置控制模式
	mcstr -> DistanceUnitLS		= RadUnit;				//默认角度制
	mcstr -> RevDirectionFlag	= Pos_Rev;				//默认正转
	mcstr -> aud_sym			= asym;					//加减匀速阶段标识
}

//电机运行启动
void MotorWorkBooter (MotorMotionSetting *mcstr)
{	
	static MotorRunStatus local_status_flag;
	
	//在电机运行初始化参数时赋予结构体指针空间，运行停止后释放空间
	mcstr -> asp = (Sigmoid_Parameter *)mymalloc(sizeof(Sigmoid_Parameter));
	mcstr -> dsp = (Sigmoid_Parameter *)mymalloc(sizeof(Sigmoid_Parameter));
	
	DivFreqAlgoUpdate(mcstr);							//更新频率
	SigmoidParam_Init(mcstr);							//更新S型加减速参数
	DistanceAlgoUpdate(mcstr);							//更新行距

	//对用户输入结果判定
	if (mcstr -> CalDivFreqConst != 0 && mcstr -> ReversalRange != 0 
		&& Return_Error_Type == Error_Clear)								
	{	
		if (ui_oled.ui_confirm_alter == 5 && UIRef_ModeFlag == Quick_Ref)
			OLED_DisplayMotorStatus(mcstr);
		
		//开关使能
		TIM_CtrlPWMOutputs(TIMERx_Number, ENABLE);	
		TIM_Cmd(TIMERx_Number, ENABLE);				
		
		mcstr -> MotorStatusFlag = Run;			
	}
	
	local_status_flag = mcstr -> MotorStatusFlag;
	//不允许打断当前工作
	while (local_status_flag == Run)
	{
		if (local_status_flag != mcstr -> MotorStatusFlag)
		{
			local_status_flag = mcstr -> MotorStatusFlag;
			//这里必须打印一些东西才能正常工作
			__ShellHeadSymbol__; U1SD("Wait For Pulse Produce Finish\r\n");
		}
	}
}

//电机运行停止(错误发生或者脉冲执行结束)
void MotorWorkStopFinish (MotorMotionSetting *mcstr)
{	
	//从运行状态停止
	if (mcstr -> MotorStatusFlag == Run)
	{
		//重新初始化参数
		MotorConfigStrParaInit(mcstr);
		
		//释放离散表指针空间
		if (!mcstr -> asp -> disp_table)
		{
			myfree((void *)mcstr -> asp -> disp_table);
			mcstr -> asp -> disp_table = NULL;
		}
		if (!mcstr -> dsp -> disp_table)
		{
			myfree((void *)mcstr -> dsp -> disp_table);
			mcstr -> dsp -> disp_table = NULL;
		}
		//释放结构体指针空间
		if (!mcstr -> asp)
		{
			myfree((void *)mcstr -> asp);
			mcstr -> asp = NULL;
		}
		if (!mcstr -> dsp) 
		{
			myfree((void *)mcstr -> dsp);
			mcstr -> dsp = NULL;
		}
	}
	if (ui_oled.ui_confirm_alter == 5 && UIRef_ModeFlag == Quick_Ref)
		OLED_DisplayMotorStatus(mcstr);
	//为了延长电机寿命让组合运动指令中间间歇一段时间
	delay_ms(500);
}

//模块同名函数，基准调用
//传参：转速(单位Hz)，行距，转向，运行模式(速度/位置)，行距单位，总调用结构体
void MotorMotionController (u16 spfq, u16 mvdis, RevDirection dir, 
	MotorRunMode mrm, LineRadSelect lrs, MotorMotionSetting *mcstr)
{	
	mcstr -> RevDirectionFlag = dir;
	IO_Direction = mcstr -> RevDirectionFlag;			//电机转向初始化
	mcstr -> SpeedFrequency = spfq;							
	mcstr -> MotorModeFlag = mrm;
	mcstr -> DistanceUnitLS = lrs;
	
	//仅在非无限脉冲模式下对线度进行限制
	mcstr -> RotationDistance = (mrm != SpeedCtrl 
		&& lrs == LineUnit)? ((mvdis < MaxLimit_Dis)? mvdis:MaxLimit_Dis):mvdis;
	
	//限位传感器起始信号捕捉(触发则不予启动电机)
	((dir == Pos_Rev && !USrNLTri) || (dir == Nav_Rev && !DSrNLTri))? 
		//调用电机运行启动/停止函数
		MotorWorkBooter(mcstr):MotorWorkStopFinish(mcstr);
}

//分频系数更新计算
void DivFreqAlgoUpdate (MotorMotionSetting *mcstr)
{
	//分频系数 = ((频率上限阈值/目标时基/设定频率)-1)x频率矫正系数
	if (mcstr -> SpeedFrequency != 0)
		mcstr -> CalDivFreqConst = ((((FreqMaxThreshold / TargetTimeBase) 
			/ mcstr -> SpeedFrequency) - 1) * mcstr -> DivCorrectConst);
}

//行距更新计算
void DistanceAlgoUpdate (MotorMotionSetting *mcstr)
{
	static Bool_ClassType local_sad_switch = False;
	
	if (mcstr -> RotationDistance != 0)
	{
		//总脉冲区间回收计算
		mcstr -> ReversalRange = 2.f 
			* ((mcstr -> DistanceUnitLS == RadUnit)? RadUnitConst:LineUnitConst) 
			* mcstr -> RotationDistance - 1.f;
		
		/*
			S型加减速分段脉冲区间回收计算
			在S型加减速中加减速段的偏移会导致整个行程的偏差
			当加减速都减1，长行程更加精确
			当加减速都不减1，短行程更加精确
		*/
		mcstr -> IndexRange[0] = (((mcstr -> ReversalRange + 1) 
			* mcstr -> asp -> ratio) / mcstr -> asp -> table_size) - 1;	//加速段单位
		mcstr -> IndexRange[1] = ((mcstr -> ReversalRange + 1) 
			* (1.f - (mcstr -> asp -> ratio + mcstr -> dsp -> ratio))) - 1;//匀速段区间
		mcstr -> IndexRange[2] = (((mcstr -> ReversalRange + 1) 
			* (mcstr -> dsp -> ratio)) / mcstr -> dsp -> table_size) - 1;//减速段单位
		
		//测试行程区间计算
		__ShellHeadSymbol__; U1SD(
			"Check Calculate Result: [Distance Pulse] %d | [Accel Unit] %d | [Uniform Range] %d | [D-value Unit] %d\r\n", 
			mcstr -> ReversalRange, mcstr -> IndexRange[0], mcstr -> IndexRange[1], mcstr -> IndexRange[2]);
			
		/*
			在URC预设时若开启S型加减速后，在计算阈值时出现0则禁用加减速
			若出现不为0值则恢复加减速
			该功能仅适用于URC预设开启加减速，若预设禁用则不会因为计算结果启用
		*/
		if (SAD_Switch == SAD_Enable && local_sad_switch == False 
			&& (mcstr -> IndexRange[0] == 0 || mcstr -> IndexRange[2] == 0))
		{
			SAD_Switch = SAD_Disable;					//若计算阈值出现0则禁用S型加减速
			local_sad_switch = True;
			__ShellHeadSymbol__; U1SD("Calculate Result Appear 0 value, S-shaped A/D Disabled!\r\n");
		}
		else if (mcstr -> IndexRange[0] != 0 && mcstr -> IndexRange[2] != 0 
			&& local_sad_switch == True)
		{
			SAD_Switch = SAD_Enable;					//计算不为0恢复S型加减速
			local_sad_switch = False;
			__ShellHeadSymbol__; U1SD("Calculate Result Correct, S-shaped A/D Activated!\r\n");
		}
	}
}

//定时器中断调用函数，控制电机脉冲IO口电平变化
void MotorPulseProduceHandler (MotorMotionSetting *mcstr)
{	
	//电机通道中断标志置位
    if (TIM_GetITStatus(TIMERx_Number, MotorChnx) == SET)	
    {	
		TIM_ClearITPendingBit(TIMERx_Number, MotorChnx);
		
		LEDGroupCtrl(led_3, On);						//电机运行指示灯(在PCB上是随机闪烁灯)
		
		//不启用S型加减速全程匀速
		if ((mcstr -> ReversalCnt == mcstr -> ReversalRange 
			&& mcstr -> MotorModeFlag != SpeedCtrl 
			&& SAD_Switch == SAD_Disable) 
			|| (Return_Error_Type != Error_Clear))		
		{	
			LEDGroupCtrl(led_3, Off);					//电机运行指示灯
			
			MotorWorkStopFinish(mcstr);
			EncoderCount_ReadValue(&st_encoderAcfg);	//检测行程偏差
			
			return;
		}
		
		//S型加减速(仅位置控制模式生效)
		if (mcstr -> MotorModeFlag == PosiCtrl 
			&& Return_Error_Type == Error_Clear 
			&& SAD_Switch == SAD_Enable)
		{
			//加速段，结束后标志指向匀速段
			//每一个判断指向一个频率点的脉冲小区间
			if (mcstr -> aud_sym == asym && mcstr -> IndexCnt == mcstr -> IndexRange[0])
			{
				mcstr -> IndexCnt = 0u;
				//加速段结束(遍历频率点数组)
				//频率点达到数组末位有效位
				if (mcstr -> asp -> table_index == mcstr -> asp -> table_size - 1)			
					mcstr -> aud_sym = usym;			//修改标志进入匀速段
				mcstr -> SpeedFrequency = mcstr -> asp -> disp_table[mcstr -> asp -> table_index++];//频率更新
			}
			//匀速段，结束后标志指向减速段
			//每一个判断指向整个匀速段同等频率的脉冲大区间
			//匀速段IndexCnt从1开始自增，所以终止值也加1
			if (mcstr -> aud_sym == usym && mcstr -> IndexCnt == mcstr -> IndexRange[1] + 1)
			{
				mcstr -> IndexCnt = 0u;
				mcstr -> aud_sym = dsym;				//匀速段结束，修改标志进入减速段
			}
			//减速段，结束后标志复位到加速段，等待下一次启动
			//每一个判断指向一个频率点的脉冲小区间
			if (mcstr -> aud_sym == dsym && mcstr -> IndexCnt == mcstr -> IndexRange[2])
			{
				mcstr -> IndexCnt = 0u;
				//全部行程结束，电机停止
				//序列达到末位有效位+1，达到后会退出不再执行
				if (mcstr -> dsp -> table_index == mcstr -> dsp -> table_size)
				{	
					LEDGroupCtrl(led_3, Off);			//电机运行指示灯
					
					MotorWorkStopFinish(mcstr);
					EncoderCount_ReadValue(&st_encoderAcfg);//检测行程偏差
					
					return;
				}
				mcstr -> SpeedFrequency = mcstr -> dsp -> disp_table[mcstr -> dsp -> table_index++];//频率更新
			}
			//频率更新后分频系数更新
			DivFreqAlgoUpdate(&st_motorAcfg);
		}
		
		//分频器，每执行一次完成一次分频计算和IO口翻转
		if (mcstr -> divFreqCnt++ == mcstr -> CalDivFreqConst)
		{
			mcstr -> divFreqCnt = 0;					//分频计数变量复位
			IO_MainPulse = !IO_MainPulse;				
			
			++mcstr -> ReversalCnt;						//总脉冲回收系数自增
			++mcstr -> IndexCnt;						//分段脉冲回收系数自增
		}
    }
}

//定时器1捕获比较中断服务
void TIM1_CC_IRQHandler (void)
{
#if SYSTEM_SUPPORT_OS
	OSIntEnter();
#endif
	
	//仅在无错误状态下使能该过程
	if (Return_Error_Type == Error_Clear)			
	{		
		MotorPulseProduceHandler(&st_motorAcfg);
		//以下可以添加更多的电机中断服务函数
	}
	
#if SYSTEM_SUPPORT_OS
	OSIntExit();    
#endif
}

/*
	滑轨上下测试
	传送参数：计数变量(偶数上升，奇数下降)	
	这个功能必须在传感器安装后使用
*/
void PeriodUpDnMotion (u16 count, MotorMotionSetting *mcstr)
{
	//滑轨上下测试，通用传感器长时间触发检测配置
	if (count % 2u == 0u && !USrNLTri)					//偶数上升
	{
		MotorMotionController(	mcstr -> SpeedFrequency, 
								MaxLimit_Dis, 
								Pos_Rev, 
								PosiCtrl, 
								LineUnit, 
								mcstr);
		WaitForSR_Trigger(ULSR);						//等待传感器长期检测	
	}
	else if (count % 2u != 0u && !DSrNLTri)				//奇数下降
	{
		MotorMotionController(	mcstr -> SpeedFrequency, 
								MaxLimit_Dis, 
								Nav_Rev, 
								PosiCtrl, 
								LineUnit, 
								mcstr);
		WaitForSR_Trigger(DLSR);						//等待传感器长期检测
	}
	MotorWorkStopFinish(&st_motorAcfg);					//完成复位立即停止动作
}

/*
	传感器反复测试运动算例
	这个功能必须在传感器安装后使用，不然会卡死
*/
void RepeatTestMotion (MotorMotionSetting *mcstr)
{
	u16 repeatCnt = 0u;
	__ShellHeadSymbol__; U1SD("Repeate Test Motion\r\n");

	//除非产生警报否则一直循环
	while (Return_Error_Type == Error_Clear)							
	{		
		PeriodUpDnMotion(repeatCnt, mcstr);
		
		//打印循环次数
		__ShellHeadSymbol__; U1SD("No.%04d Times Repeat Test\r\n", repeatCnt);
		displaySystemInfo();							//打印系统状态信息
		
		if (++repeatCnt >= 1000u) 
			repeatCnt = 0u;			
		
		if (STEW_LTrigger) 
			break;										//长按检测急停
	}
    //总动作完成
	__ShellHeadSymbol__; U1SD("Test Repeat Stop\r\n");
}

//正反转重复性测试
void PosNavRepeatMotion (MotorMotionSetting *mcstr, u16 speed, u16 dis)
{
	u16 repeatCnt = 0u;
	__ShellHeadSymbol__; U1SD("Positive And Negative Repeatability Test\r\n");

	//除非产生警报否则一直循环
	while (Return_Error_Type == Error_Clear)							
	{		
		//正转
		MotorMotionController(	speed, 
								dis, 
								Pos_Rev, 
								PosiCtrl, 
								RadUnit, 
								mcstr); 
		//反转
		MotorMotionController(	speed, 
								dis, 
								Nav_Rev, 
								PosiCtrl, 
								RadUnit, 
								mcstr); 
		
		//打印循环次数
		__ShellHeadSymbol__; U1SD("No.%04d Times Repeat Test\r\n", repeatCnt);
		
		if (++repeatCnt >= 1000u) 
			repeatCnt = 0u;			
		
		if (STEW_LTrigger) 
			break;										//长按检测急停
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
			MotorMotionController(mcstr -> SpeedFrequency, MaxLimit_Dis, Nav_Rev, PosiCtrl, LineUnit, mcstr);
			WaitForSR_Trigger(DLSR);					//等待传感器长期检测
			MotorWorkStopFinish(&st_motorAcfg);			//完成复位立即停止动作
		}
	}		
}

//OLED显示电机运行状态值(分时任务，不考虑实时显示)
void OLED_DisplayMotorStatus (MotorMotionSetting *mcstr)
{
	//显示电机运行状态、显示电机转向
	snprintf((char*)oled_dtbuf, OneRowMaxWord, ("MS:%s DN:%s"), 
		((mcstr -> MotorStatusFlag == Run)? "Work":"Stew"), 
		((mcstr -> RevDirectionFlag == Pos_Rev)? "Pos":"Neg"));
	OLED_ShowString(strPos(0u), ROW1, (StringCache*)oled_dtbuf, Font_Size);
	
	//显示电机行距、显示电机转速
	snprintf((char*)oled_dtbuf, OneRowMaxWord, 
		((mcstr -> DistanceUnitLS == LineUnit)? "RM:%04d SF:%04d":"RA:%04d SF:%04d"), 
		mcstr -> RotationDistance, mcstr -> SpeedFrequency);
	OLED_ShowString(strPos(0u), ROW2, (StringCache*)oled_dtbuf, Font_Size);
	OLED_Refresh_Gram();
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
