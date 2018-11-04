#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//步进电机简易音乐播放器

Bool_ClassType music_JumpOutWhileLoop = False;

//实际播放效果是不明电音
const u8 musicToneFreqArray[] = {	212, 212, 190, 212, 159, 
									169, 212, 212, 190, 212, 
									142, 159, 212, 212, 106, 
									126, 159, 169, 190, 119, 
									119, 126, 159, 142, 159, 212};				//单位：Hz
const u8 musicBeatLongArray[] = {	9, 3, 12, 12, 12,
									24, 9, 3, 12, 12, 
									12, 24,	9, 3, 12,
									12, 12, 12, 12, 9, 
									3, 12, 12, 12, 24, 9};						//单位：度

//播放demo
//传参：电机控制结构体指针，常量音频数组，常量节拍数组，音频放大系数，节拍放大系数
void MusicPlayerDemo (MotorMotionSetting *mcstr, const u8 mtfa[], const u8 mbla[], u8 tac, u8 bac)
{
	u8 i, arrayLength = Get_Array_Size((char *)mtfa);
	
	for (i = 0; i < arrayLength; i++)
	{	
		MotorMotionController (	(tac != 0)? *(mtfa + i) * tac:*(mtfa + i), 
								(bac != 0)? *(mbla + i) * bac:*(mbla + i), 
								(i % 2 == 0)? Pos_Rev:Nav_Rev, 					//正反转交替
								PosiCtrl, 
								RadUnit, 
								mcstr);
		//等待节拍完成
		delay_ms(300);															
	}
}
							
//回调音乐播放器
void MusicPlayerCallback (MotorMotionSetting *mcstr)
{
	//获取有限无限模式位
	Bool_ClassType musicLimitFlag = (Bool_ClassType)(*(USART1_RX_BUF + SSD_Music_Limit));
	//音频节拍放大系数，放大系数为0则不放大，相当于1
	u8 toneAmpConst = (*(USART1_RX_BUF + SSD_Music_Tone));
	u8 beatAmpConst	= (*(USART1_RX_BUF + SSD_Music_Beat));
	
	//模式标识
	__ShellHeadSymbol__; 
	U1SD((musicLimitFlag == True)?
		"StepMotor Unlimit Music Player\r\n":"StepMotor Limit Music Player\r\n");	
	//打印放大系数
	__ShellHeadSymbol__; U1SD("Tone Amplify Const: %d, Beat Amplify Const: %d\r\n", toneAmpConst, beatAmpConst);
	
	music_JumpOutWhileLoop = False;
	
	//无限播放模式
	if (musicLimitFlag == True)
	{
		while (Return_Error_Type == Error_Clear && music_JumpOutWhileLoop == False)	
		{
			MusicPlayerDemo(mcstr, musicToneFreqArray, musicBeatLongArray, toneAmpConst, beatAmpConst);
		}
	}
	//有限播放模式
	else
	{
		MusicPlayerDemo(mcstr, musicToneFreqArray, musicBeatLongArray, toneAmpConst, beatAmpConst);
	}
}

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
