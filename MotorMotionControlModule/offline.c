#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//专业综合训练脱机控制协议

//上位机脱机控制
void UpperMonitorOfflineControl (MotorMotionSetting *mcstr)
{	
	//转向与转角倍数解析
	RevDirection dirG1 = (RevDirection)(*(USART1_RX_BUF + OLP_G1_Dir));
	u16 RadG1 = (*(USART1_RX_BUF + OLP_G1_Multi) * 10u + *(USART1_RX_BUF + (OLP_G1_Multi + 1))) * 45;
	
	RevDirection dirG2 = (RevDirection)(*(USART1_RX_BUF + OLP_G2_Dir));
	u16 RadG2 = (*(USART1_RX_BUF + OLP_G2_Multi) * 10u + *(USART1_RX_BUF + (OLP_G2_Multi + 1))) * 45;
	
	RevDirection dirG3 = (RevDirection)(*(USART1_RX_BUF + OLP_G3_Dir));
	u16 RadG3 = (*(USART1_RX_BUF + OLP_G3_Multi) * 10u + *(USART1_RX_BUF + (OLP_G3_Multi + 1))) * 45;
	
	RevDirection dirG4 = (RevDirection)(*(USART1_RX_BUF + OLP_G4_Dir));
	u16 RadG4 = (*(USART1_RX_BUF + OLP_G4_Multi) * 10u + *(USART1_RX_BUF + (OLP_G4_Multi + 1))) * 45;
	
	RevDirection dirG5 = (RevDirection)(*(USART1_RX_BUF + OLP_G5_Dir));
	u16 RadG5 = (*(USART1_RX_BUF + OLP_G5_Multi) * 10u + *(USART1_RX_BUF + (OLP_G5_Multi + 1))) * 45;
	
	//模式标识
	__ShellHeadSymbol__; U1SD("Upper Monitor Offline Control Mode\r\n");	
	//打印每组动态信息
	__ShellHeadSymbol__; U1SD("[Group1] Dir: %d Rad: %d\r\n", dirG1, RadG1);
	__ShellHeadSymbol__; U1SD("[Group2] Dir: %d Rad: %d\r\n", dirG2, RadG2);
	__ShellHeadSymbol__; U1SD("[Group3] Dir: %d Rad: %d\r\n", dirG3, RadG3);
	__ShellHeadSymbol__; U1SD("[Group4] Dir: %d Rad: %d\r\n", dirG4, RadG4);
	__ShellHeadSymbol__; U1SD("[Group5] Dir: %d Rad: %d\r\n", dirG5, RadG5);
	
	//进入脱机-无限循环栈
	while (Return_Error_Type == Error_Clear)
	{
		//第一组动作
		MotorMotionController(OLCP_StableFreq, RadG1, dirG1, LimitRun, RadUnit, mcstr);
		delay_ms(1000 + OLCP_45degreeDelayConst * RadG1);
		//第二组动作
		MotorMotionController(OLCP_StableFreq, RadG2, dirG2, LimitRun, RadUnit, mcstr);
		delay_ms(1000 + OLCP_45degreeDelayConst * RadG2);
		//第三组动作
		MotorMotionController(OLCP_StableFreq, RadG3, dirG3, LimitRun, RadUnit, mcstr);
		delay_ms(1000 + OLCP_45degreeDelayConst * RadG3);
		//第四组动作
		MotorMotionController(OLCP_StableFreq, RadG4, dirG4, LimitRun, RadUnit, mcstr);
		delay_ms(1000 + OLCP_45degreeDelayConst * RadG4);
		//第五组动作
		MotorMotionController(OLCP_StableFreq, RadG5, dirG5, LimitRun, RadUnit, mcstr);
		delay_ms(1000 + OLCP_45degreeDelayConst * RadG5);
	}
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
