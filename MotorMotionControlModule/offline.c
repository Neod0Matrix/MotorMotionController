#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//专业综合训练脱机控制协议

Bool_ClassType offline_JumpOutWhileLoop = False;
u16 AnologPWM_DC = 0;

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
	
	offline_JumpOutWhileLoop = False;										
	
	//进入脱机-无限循环栈
	while (Return_Error_Type == Error_Clear && offline_JumpOutWhileLoop == False)
	{
		LEDGroupCtrl(led_2, On);									//脱机指示灯
		
		//第一组动作
		if (RadG1 != 0)
		{
			MotorMotionController(OLCP_StableFreq, RadG1, dirG1, LimitRun, RadUnit, mcstr);
			delay_ms(1000 + OLCP_45degreeDelayConst * RadG1);
			//while (mcstr -> MotorStatusFlag == Run);
		}
		//第二组动作
		if (RadG2 != 0)
		{
			MotorMotionController(OLCP_StableFreq, RadG2, dirG2, LimitRun, RadUnit, mcstr);
			delay_ms(1000 + OLCP_45degreeDelayConst * RadG2);
			//while (mcstr -> MotorStatusFlag == Run);
		}
		//第三组动作
		if (RadG3 != 0)
		{
			MotorMotionController(OLCP_StableFreq, RadG3, dirG3, LimitRun, RadUnit, mcstr);
			delay_ms(1000 + OLCP_45degreeDelayConst * RadG3);
			//while (mcstr -> MotorStatusFlag == Run);
		}
		//第四组动作
		if (RadG4 != 0)
		{
			MotorMotionController(OLCP_StableFreq, RadG4, dirG4, LimitRun, RadUnit, mcstr);
			delay_ms(1000 + OLCP_45degreeDelayConst * RadG4);
			//while (mcstr -> MotorStatusFlag == Run);
		}
		//第五组动作
		if (RadG5 != 0)
		{
			MotorMotionController(OLCP_StableFreq, RadG5, dirG5, LimitRun, RadUnit, mcstr);
			delay_ms(1000 + OLCP_45degreeDelayConst * RadG5);
			//while (mcstr -> MotorStatusFlag == Run);
		}
	}
}

//开关量模拟量示例IO
void AnologDigitalVal_IO_Init (void)
{
	//PA11 开关量IO
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOA,										
						GPIO_Mode_Out_PP,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_11,							
						GPIOA,					
						IHL,				
						EBO_Enable);
	
	//PA5 模拟量IO
	ucGPIO_Config_Init (RCC_APB2Periph_GPIOA,										
						GPIO_Mode_Out_PP,					
						GPIO_Speed_50MHz,						
						GPIORemapSettingNULL,			
						GPIO_Pin_12,							
						GPIOA,					
						IHL,				
						EBO_Enable);					
}

//模拟量IO
void AnologPWMProduce (void)
{
	static u8 dutyRange = 100u;
	
	//初始化过程关闭
	if (pwsf == JBoot || Return_Error_Type != Error_Clear)
		IO_AnologVal = MD_IO_Reset;
	else if (Return_Error_Type == Error_Clear && pwsf != JBoot 
		&& globalSleepflag == SysOrdWork) 
	{
		//产生固定占空比的PWM波
		if (AnologPWM_DC == dutyRange)
			IO_AnologVal = MD_IO_Reset;
		if (dutyRange == 0u)
		{
			IO_AnologVal = !MD_IO_Reset;
			dutyRange = 100u;
		}
		dutyRange--;
	}
}

//开关量/模拟量控制
void Anolog_Digital_Controller (void)
{
	float ano_val = (*(USART1_RX_BUF + ADCP_ABit_1) 
		+ *(USART1_RX_BUF + ADCP_ABit_2) * 0.1f 
		+ *(USART1_RX_BUF + ADCP_ABit_3) * 0.01f);
	
	float anoDutyCycle = ano_val / 5.f;						//计算占空比
	
	LED_Status dl_status = (LED_Status)(*(USART1_RX_BUF + ADCP_DLogic_Bit));
	
	__ShellHeadSymbol__; U1SD("Anolog Value: %4.2fV Digital Value: %d\r\n", ano_val, dl_status);
	
	AnologPWM_DC = anoDutyCycle * 100;						//全局传参，得到百分制占空比
	
#ifdef use_ULN2003A
	IO_DigitalVal = !dl_status;
#else
	IO_DigitalVal = dl_status;
#endif
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
