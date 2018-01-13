#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//装载在机械臂上的定位传感器设置初始化
/*
	初始化IO口映射:
		A2U		A2D
		|		|
		PB4		PB3
*/

//初始化传感器外设接口
void Sensor_IO_Init (void)
{
	//一般设置成上拉输入
	//U PB4		
	//D PB3	
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,			
						GPIO_Mode_IPU,					
						GPIO_Input_Speed,					//无效参数						
						GPIO_Remap_SWJ_JTAGDisable,			//关闭jtag，使能SWD，可以用SWD模式调试		
						GPIO_Pin_4 | GPIO_Pin_3,					
						GPIOB,					
						NI,				
						EBO_Disable);
}

/*
	传感器处理函数
	返回值：0，没有任何传感器使能
	留神传感器是否支持长期检测这个问题
*/
Sensor_MapTable Sensor_Scan (Input_LoogTrigger mode)
{	 
	u8 sensor_detect = 1;									//初始化传感器未响应
	
	if (mode) sensor_detect = 1; 									  
	
	if (sensor_detect && (		ULSR_IO == Trigger 
							|| 	DLSR_IO == Trigger)
		)
	{
		//传感器不消抖
		
		sensor_detect = 0;									//已触发，复位
		
		//映射表
		if (ULSR_IO == Trigger) 		return ULSR_TR;							
		else if (DLSR_IO == Trigger) 	return DLSR_TR;
	} 
	
	//无映射
	else if (	ULSR_IO == No_Trigger 
			&& 	DLSR_IO == No_Trigger 
						) 
		sensor_detect = 1; 	     
	
	return Null_TR;											//未响应
}

//等待传感器检测到遮挡，封装判断是否触发
void WaitForSR_Trigger (Sensor_Number sr_nbr)
{
	//选择编号
	switch (sr_nbr)
	{
		//这里可以添加更多的传感器
	case ULSR: while (!USrNLTri){} break;
	case DLSR: while (!DSrNLTri){} break;
	}
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
