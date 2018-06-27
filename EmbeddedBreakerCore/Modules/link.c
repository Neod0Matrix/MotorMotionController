#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
/*
	模块对框架EmbeddBreakerCore的链接
	该文件写入对框架的函数调用支持
*/

Sigmod_Acce_Dval_Switch 	SAD_Switch;
Init_ARM_Reset_Switch 		Init_Reset_Switch;
ARM_Sensor_EXTI_Setting		ASES_Switch;

//选项设置，链接到Universal_Resource_Config函数的模块库
void Modules_UniResConfig (void)
{
	//该函数设置内容可以更新Universal_Resource_Config函数原设置
	/*
		对框架而言，不显示模块的OLED部分
		对应用的模块而言，不显示框架的常量字符
		且需要使自己本身的显示生效
		框架设置为失能，模块设置为使能
	*/
	MOE_Switch			= MOE_Enable;					//MOE_Enable		MOE_Disable
	
	/*
		电机柔性启停有多种积极意义
		本工程主要是为了在步进电机相对高速运转时带动更重负载
	*/
    SAD_Switch 			= SAD_Disable;					//SAD_Enable		SAD_Disable
	
	/*
		机器上电完全复位的重要部分
		实际运用建议开启，建立绝对坐标系
		调试时建议关闭，以免损伤机械臂部件
	*/
    Init_Reset_Switch 	= Reset_Disable;				//Reset_Enable		Reset_Disable
	
	/*
		由于机械臂传感器触发判断多种多样
		可能不适合放到外部中断
		但如果普通检测可能响应不够快
	*/
	ASES_Switch			= ASES_Enable;					//ASES_Enable		ASES_Disable
}

//模块选项映射表，链接到urcMapTable_Print函数
void Modules_URCMap (void)
{
	U1SD("\r\n%02d	S-Accel/Dvalue Speed", urc_sad);
	U1SD("\r\n%02d	Arm Position Reset", urc_areset);
	U1SD("\r\n%02d 	Arm Sensor EXTI Setting", urc_ases);
}

//选项处理，链接到pclURC_DebugHandler函数
void Modules_urcDebugHandler (u8 ed_status, Modules_SwitchNbr sw_type)
{
   //使用前请先更新Modules_SwitchNbr内容
	switch (sw_type)
	{
	case urc_sad: 		SAD_Switch 		= (Sigmod_Acce_Dval_Switch)ed_status; 		break;
	case urc_areset: 	ASES_Switch 	= (ARM_Sensor_EXTI_Setting)ed_status; 		break;
	case urc_ases: 		ASES_Switch		= (ARM_Sensor_EXTI_Setting)ed_status;		break;	
	}
}

//协议调用指令响应，链接到OrderResponse_Handler函数
void Modules_ProtocolTask (void)
{
	/*
		16进制转10进制，线度单位毫米，角度单位度
		由于是16进制坐标所以不需要添加0x30转换值
	*/
	
	//两字算列类型
	Motion_Select SSD_MotionNumber	= (Motion_Select)(
											*(USART1_RX_BUF + SSD_MoNum_1st) 		* 10u 
										+ 	*(USART1_RX_BUF + (SSD_MoNum_1st + 1)));
	//一字行距单位
	LineRadSelect SSD_Lrsflag		= (LineRadSelect)(
											*(USART1_RX_BUF + SSD_DisUnit_1st));
	//四字行距长度
	u16 SSD_GetDistance 			= (u16)(
											*(USART1_RX_BUF + SSD_GetDis_1st) 		* 1000u 
										+ 	*(USART1_RX_BUF + (SSD_GetDis_1st + 1)) * 100u 
										+ 	*(USART1_RX_BUF + (SSD_GetDis_1st + 2)) * 10u
										+ 	*(USART1_RX_BUF + (SSD_GetDis_1st + 3)));
	//四字速度
	u16 SSD_Speed					= (u16)(*(USART1_RX_BUF + SSD_SpFq_1st)			* 1000u
										+	*(USART1_RX_BUF + (SSD_SpFq_1st + 1)) 	* 100u 
										+ 	*(USART1_RX_BUF + (SSD_SpFq_1st + 2)) 	* 10u 
										+ 	*(USART1_RX_BUF + (SSD_SpFq_1st + 3)));
	//一字模式位
	MotorRunMode SSD_Mrmflag		= (MotorRunMode)(*(USART1_RX_BUF + SSD_Mode_1st));
	
	char* output_cache;
#define OUTPUT_CACHE_SIZE	100
	
	//打印标志，算例编号，圈数，急停不显示
	if (SSD_MotionNumber != Stew_All)
	{
		__ShellHeadSymbol__; U1SD("Please Confirm Motion Parameter: ");
		
		output_cache = (char*)malloc(sizeof(char) * OUTPUT_CACHE_SIZE);
		//两个flag四种情况
		if (SSD_Lrsflag == RadUnit && SSD_Mrmflag == LimitRun)
			snprintf(output_cache, OUTPUT_CACHE_SIZE, "Motion Type: %02d | Speed: %dHz | Distance: %ddegree | Mode: LimitRun\r\n", 
				SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == RadUnit && SSD_Mrmflag == UnlimitRun)
			snprintf(output_cache, OUTPUT_CACHE_SIZE, "Motion Type: %02d | Speed: %dHz | Distance: %ddegree | Mode: UnlimitRun\r\n", 
				SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == LineUnit && SSD_Mrmflag == LimitRun)
			snprintf(output_cache, OUTPUT_CACHE_SIZE, "Motion Type: %02d | Speed: %dHz | Distance: %dmm | Mode: LimitRun\r\n", 
				SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == LineUnit && SSD_Mrmflag == UnlimitRun)
			snprintf(output_cache, OUTPUT_CACHE_SIZE, "Motion Type: %02d | Speed: %dHz | Distance: %dmm | Mode: UnlimitRun\r\n", 
				SSD_MotionNumber, SSD_Speed, SSD_GetDistance);	
		U1SD("%s", output_cache);
		free((void*)output_cache);
	}

	switch (SSD_MotionNumber)				
	{
	//急停
	case Stew_All: 		
		MotorBasicDriver(&st_motorAcfg, StopRun); 
		//EMERGENCYSTOP;									//协议通信急停								
		break;
	//上下行基本算例
	case UpMove: 		
		MotorMotionController(SSD_Speed, SSD_GetDistance, Pos_Rev, SSD_Mrmflag, SSD_Lrsflag, &st_motorAcfg); 
		break;			
	case DownMove: 		
		MotorMotionController(SSD_Speed, SSD_GetDistance, Nav_Rev, SSD_Mrmflag, SSD_Lrsflag, &st_motorAcfg); 
		break;
	//重复性测试
	case Repeat: 		
		RepeatTestMotion(&st_motorAcfg); 						
		break;
	}
	
	if (SSD_MotionNumber != Stew_All)
	{
		__ShellHeadSymbol__; U1SD("Order Has Started to Execute\r\n");
	}
}

//OLED常量显示屏，链接到OLED_DisplayInitConst和UIScreen_DisplayHandler函数
void OLED_ScreenModules_Const (void)
{
	snprintf((char*)oled_dtbuf, OneRowMaxWord, ("  MotorMotion  "));
	OLED_ShowString(strPos(0u), ROW1, (StringCache*)oled_dtbuf, Font_Size);
	snprintf((char*)oled_dtbuf, OneRowMaxWord, (" ControlModule "));
	OLED_ShowString(strPos(0u), ROW2, (StringCache*)oled_dtbuf, Font_Size);
	OLED_Refresh_Gram();
}

//OLED模块调用数据显示，链接到UIScreen_DisplayHandler函数
void OLED_DisplayModules (u8 page)
{
	switch (page)
	{
	case 5:
		OLED_DisplayMotorA(&st_motorAcfg);
		break;
	}
}

//硬件底层初始化任务，链接到bspPeriSysCalls函数
void Modules_HardwareInit (void)
{
	MotorDriverLib_Init();
}

//硬件底层外部中断初始化，链接到EXTI_Config_Init函数
void Modules_ExternInterruptInit (void)
{
	if (ASES_Switch == ASES_Enable)
	{
		//先初始化IO口
		Sensor_IO_Init();
		//PB4 A2U
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);			//外部中断，需要使能AFIO时钟
		ucEXTI_ModeConfig(
							GPIO_PortSourceGPIOB, 
							GPIO_PinSource4, 
							ARM2Up_EXTI_Line, 
							EXTI_Mode_Interrupt, 
							EXTI_Trigger_Falling, 
							EXTI4_IRQn, 
							0x02, 
							0x03);
							
		//PB3 A2D
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);			//外部中断，需要使能AFIO时钟
		ucEXTI_ModeConfig(
							GPIO_PortSourceGPIOB, 
							GPIO_PinSource3, 
							ARM2Dn_EXTI_Line, 
							EXTI_Mode_Interrupt, 
							EXTI_Trigger_Falling, 
							EXTI3_IRQn, 
							0x02, 
							0x02);
	}

	//编码器Z相外部中断PC2
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOC,										
						GPIO_Mode_IPU,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_2,							
						GPIOC,					
						NI,				
						EBO_Disable);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	ucEXTI_ModeConfig(
							GPIO_PortSourceGPIOC, 
							GPIO_PinSource2, 
							Encoder_Zphase_EXTI_Line, 
							EXTI_Mode_Interrupt, 
							EXTI_Trigger_Falling, 
							EXTI3_IRQn, 
							0x03, 
							0x02);
}

//外部中断任务，无需声明，使用时修改函数名
//A2U--PB4
void EXTI4_IRQHandler (void)										//机械臂传感器检测
{
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntEnter();    
#endif
	
	if (ASES_Switch	== ASES_Enable && EXTI_GetITStatus(ARM2Up_EXTI_Line) != RESET)  		
	{
		MotorBasicDriver(&st_motorAcfg, StopRun);
	}
	EXTI_ClearITPendingBit(ARM2Up_EXTI_Line);						//清除EXTI线路挂起位
	
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntExit();  											 
#endif
}

//A2D--PB3
void EXTI3_IRQHandler (void)										//机械臂传感器检测
{
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntEnter();    
#endif
	
	if (ASES_Switch	== ASES_Enable && EXTI_GetITStatus(ARM2Dn_EXTI_Line) != RESET)  
	{		
		MotorBasicDriver(&st_motorAcfg, StopRun);
	}
	EXTI_ClearITPendingBit(ARM2Dn_EXTI_Line);						//清除EXTI线路挂起位
	
#if SYSTEM_SUPPORT_OS 												//如果SYSTEM_SUPPORT_OS为真，则需要支持OS
	OSIntExit();  											 
#endif
}

//模块非中断任务，链接到local_taskmgr.c，默认添加到第二任务
void Modules_NonInterruptTask (void)
{
	
}

//模块中断任务，链接到time_base.c TIM2_IRQHandler函数中
void Modules_InterruptTask (void)
{
	
}

//基于RTC时间的任务计划，链接到local_taskmgr.c，默认添加到第四任务
void Modules_RTC_TaskScheduler (void)
{
	/*
		RTC API:
			*(rtcTotalData + 0): 年份
			*(rtcTotalData + 1): 月份
			*(rtcTotalData + 2): 日
			*(rtcTotalData + 3): 星期
			*(rtcTotalData + 4): 时
			*(rtcTotalData + 5): 分
			*(rtcTotalData + 6): 秒
	*/
	//example: 设置含有灯光效果的外设休眠
	if ((*(rtcTotalData + 4) >= 1 
		&& *(rtcTotalData + 4) <= 6) || *(rtcTotalData + 4) == 0)
	{
		OLED_Clear();
		OLED_Switch = OLED_Disable;
		Light_Switch = Light_Disable;
	}
	else
	{
		OLED_Switch = OLED_Enable;
		Light_Switch = Light_Enable;
	}
}

//模块状态内容打印请求，链接到sta_req.c displaySystemInfo函数中
void Modules_StatusReqHandler (void)
{
	//此项设计可以减少模块指令的多余添加
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
