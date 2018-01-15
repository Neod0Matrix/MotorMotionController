#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//模块MotorMotionControl对框架EmbeddBreakerCore的链接
//该文件写入对框架的函数调用支持

Sigmod_Acce_Dval_Switch 	SAD_Switch;
Init_ARM_Reset_Switch 		Init_Reset_Switch;
ARM_Sensor_EXTI_Setting		ASES_Switch;
Stew_EXTI_Setting			StewEXTI_Switch;

//链接到Universal_Resource_Config函数的模块库
void ModuleMMC_UniResConfig (void)
{
	/*
		电机柔性启停有多种积极意义
		本工程主要是为了在步进电机相对高速运转时带动更重负载
	*/
    SAD_Switch 			= SAD_Enable;					//SAD_Enable		SAD_Disable
	
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
	
	/*
		急停状态判断复杂，不适合外部中断
		但也有可能普通监测不够快
	*/
	StewEXTI_Switch 	= StewEXTI_Enable;				//StewEXTI_Enable	StewEXTI_Disable
}

//模块选项映射表，链接到urcMapTable_Print函数
void ModuleMMC_URCMap (void)
{
	printf("\r\n%02d	S-Accel/Dvalue Speed", urc_sad);
	usart1WaitForDataTransfer();
	printf("\r\n%02d	Arm Position Reset", urc_areset);
	usart1WaitForDataTransfer();
	printf("\r\n%02d 	Arm Sensor EXTI Setting", urc_ases);
	usart1WaitForDataTransfer();
	printf("\r\n%02d 	Stew EXTI Setting", urc_stew);
	usart1WaitForDataTransfer();
}

//选项处理，链接到pclURC_DebugHandler函数
void ModuleMMC_urcDebugHandler (u8 ed_status, Module_SwitchNbr sw_type)
{
	switch (sw_type)
	{
	case urc_sad: 		SAD_Switch 		= (Sigmod_Acce_Dval_Switch)ed_status; 		break;
	case urc_areset: 	ASES_Switch 	= (ARM_Sensor_EXTI_Setting)ed_status; 		break;
	case urc_ases: 		ASES_Switch		= (ARM_Sensor_EXTI_Setting)ed_status;		break;	
	case urc_stew: 		StewEXTI_Switch	= (Stew_EXTI_Setting)ed_status;				break;	
	}
}

//OLED常量第四屏，链接到OLED_DisplayInitConst和UIScreen_DisplayHandler函数
void OLED_ScreenP4_Const (void)
{	
	OLED_ShowString(strPos(1u), ROW1, (const u8*)" MotorMotion  ", Font_Size);	
	OLED_ShowString(strPos(1u), ROW2, (const u8*)"ControlModule ", Font_Size);	
	OLED_Refresh_Gram();
}

//串口接收数据示例
void U1RSD_example (void)
{
    u8 t, len;
	
    if (PD_Switch == PD_Enable && Data_Receive_Over)	//接收数据标志
    {
        len = Data_All_Length;							//得到此次接收到的数据长度(字符串个数)
        __ShellHeadSymbol__; U1SD("Controller Get The Data: \r\n");
		if (No_Data_Receive)							//没有数据接收，可以发送
		{
			for (t = 0u; t < len; t++)
			{
				USART_SendData(USART1, USART1_RX_BUF[t]);//将所有数据依次发出	
				usart1WaitForDataTransfer();			//等待发送结束
			}
		}
        U1SD("\r\n");									//插入换行
		
        USART1_RX_STA = 0u;								//接收状态标记
    }
}

//串口控制运动算例
Motion_Select SingleStepDebug_linker (void)
{
	//16进制转10进制，线度单位毫米，角度单位度
	//由于是16进制坐标所以不需要添加0x30转换值
	
	//两字节算列类型，SSD_MoNum_1st协议宏定义位取
	Motion_Select SSD_MotionNumber	= (Motion_Select)(
											USART1_RX_BUF[SSD_MoNum_1st] 		* 10u 
										+ 	USART1_RX_BUF[SSD_MoNum_1st + 1]);
	//四字节距离
	u16 SSD_GetDistance 			= (u16)(
											USART1_RX_BUF[SSD_GetDis_1st] 		* 1000u 
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 1] 	* 100u 
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 2] 	* 10u
										+ 	USART1_RX_BUF[SSD_GetDis_1st + 3]);
	
	//0 <= dis <= 9999
	if (SSD_GetDistance > X_Max)						//unsigned数据必定大于0
	{
		SSD_GetDistance = 0u;							//数据清0
		
		ErrorWarning_Feedback(SSDS);					//16进制传送算列错误标识SSDS
		
		SERIALDATAERROR;								//串口ascii报警
		SERIALDATAERROR_16;								//串口16进制报警							
	}
	
	//打印标志，算例编号，圈数，急停和警报清除不显示
	if (SendDataCondition && (SSD_MotionNumber != ERROR_OUT && SSD_MotionNumber != Stew_All))
	{
		__ShellHeadSymbol__; 
		printf("Please Confirm Motion Parameter: Motion Type: %02d / Distance: %dmm\r\n", SSD_MotionNumber, SSD_GetDistance);
		usart1WaitForDataTransfer();		
		__ShellHeadSymbol__;  U1SD("Controller Loading Pulse, Please Far Away From Motor or Wheel\r\n");
	}
	
	//急停指令
	if (SSD_MotionNumber == Stew_All)
	{
		MotorAxisEmgStew();								//急停停下所有轴
		EMERGENCYSTOP;									//串口急停
		EMERGENCYSTOP_16;
	}
	else												//当算例不为急停时才能继续跑
	{
		switch (SSD_MotionNumber)							
		{
		//急停
		case Stew_All: 													break;//急停	
		//警报解除
		case ERROR_OUT: 	ERROR_CLEAR; 								break;//警报解除
		//机械臂
		case UpMove: 		MotorBaseMotion(SSD_GetDistance, Pos_Rev); 	break;//机械臂2向上运行测试				
		case DownMove: 		MotorBaseMotion(SSD_GetDistance, Nav_Rev); 	break;//机械臂2向下运行测试
		//重复性测试
		case Repeat: 		RepeatTestMotion(); 						break;//反复测试
		}
	}
	
	//急停和警报清除不提示
	if (SSD_MotionNumber != ERROR_OUT && SSD_MotionNumber != Stew_All)
	{
		__ShellHeadSymbol__; U1SD("Pulse Has Been Sent to the MotorDriver\r\n");
	}
	order_bootflag = pcl_error;							//完成工作，协议关闭
	
	return SSD_MotionNumber;							//返回算例号用于其它功能
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
