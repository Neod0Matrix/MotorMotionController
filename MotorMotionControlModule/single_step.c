#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//单步测试串口发送算例及圈数

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
