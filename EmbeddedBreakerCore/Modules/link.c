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
	printf("\r\n%02d	S-Accel/Dvalue Speed", urc_sad);
	usart1WaitForDataTransfer();
	printf("\r\n%02d	Arm Position Reset", urc_areset);
	usart1WaitForDataTransfer();
	printf("\r\n%02d 	Arm Sensor EXTI Setting", urc_ases);
	usart1WaitForDataTransfer();
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
	
	//两字节算列类型
	Motion_Select SSD_MotionNumber	= (Motion_Select)(
											USART1_RX_BUF[SSD_MoNum_1st] 		* 10u 
										+ 	USART1_RX_BUF[SSD_MoNum_1st + 1]);
	//一字节行距单位
	LineRadSelect SSD_Lrsflag		= (LineRadSelect)(
											USART1_RX_BUF[SSD_DisUnit_1st]);
	//四字节行距长度
	u16 SSD_GetDistance 			= (u16)(
											USART1_RX_BUF[SSD_GetDis_1st] 		* 1000u 
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 1] 	* 100u 
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 2] 	* 10u
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 3]);
	//四字节速度
	u16 SSD_Speed					= (u16)(USART1_RX_BUF[SSD_SpFq_1st]			* 1000u
										+	USART1_RX_BUF[SSD_SpFq_1st + 1] 	* 100u 
										+ 	USART1_RX_BUF[SSD_SpFq_1st + 2] 	* 10u 
										+ 	USART1_RX_BUF[SSD_SpFq_1st + 3]);
	//一字节模式位
	MotorRunMode SSD_Mrmflag		= (MotorRunMode)(USART1_RX_BUF[SSD_Mode_1st]);
	
	//打印标志，算例编号，圈数，急停不显示
	if (SendDataCondition && SSD_MotionNumber != Stew_All)
	{
		__ShellHeadSymbol__; 
		printf("Please Confirm Motion Parameter: ");
		//两个flag四种情况
		if (SSD_Lrsflag == RadUnit && SSD_Mrmflag == LimitRun)
			printf("Motion Type: %02d | Speed: %dHz | Distance: %ddegree | Mode: LimitRun\r\n", SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == RadUnit && SSD_Mrmflag == UnlimitRun)
			printf("Motion Type: %02d | Speed: %dHz | Distance: %ddegree | Mode: UnlimitRun\r\n", SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == LineUnit && SSD_Mrmflag == LimitRun)
			printf("Motion Type: %02d | Speed: %dHz | Distance: %dmm | Mode: LimitRun\r\n", SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		else if (SSD_Lrsflag == LineUnit && SSD_Mrmflag == UnlimitRun)
			printf("Motion Type: %02d | Speed: %dHz | Distance: %dmm | Mode: UnlimitRun\r\n", SSD_MotionNumber, SSD_Speed, SSD_GetDistance);
		usart1WaitForDataTransfer();		
	}

	switch (SSD_MotionNumber)				
	{
	//急停
	case Stew_All: 		
		MotorBasicDriver(&st_motorAcfg, StopRun); 
		EMERGENCYSTOP;									//协议通信急停
		EMERGENCYSTOP_16;										
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
	order_bootflag = pcl_error;							//完成工作，协议关闭
}

//OLED常量显示屏，链接到OLED_DisplayInitConst和UIScreen_DisplayHandler函数
void OLED_ScreenModules_Const (void)
{
	OLED_ShowString(strPos(1u), ROW1, (const u8*)" MotorMotion  ", Font_Size);	
	OLED_ShowString(strPos(1u), ROW2, (const u8*)"ControlModule ", Font_Size);	
	OLED_Refresh_Gram();
}

//OLED模块调用数据显示，链接到UIScreen_DisplayHandler函数
void OLED_DisplayModules (void)
{
	OLED_DisplayMotorA(&st_motorAcfg);
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

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
