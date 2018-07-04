#pragma once
#include "stdafx.h"
//code by </MATRIX>@Neod Anderjon
//author: Neod Anderjon
//====================================================================================================
//步进电机简易音乐播放器

extern const u8 musicToneFreqArray[];		//音频数组
extern const u8 musicBeatLongArray[];		//节拍数组

extern Bool_ClassType music_JumpOutWhileLoop;

void MusicPlayerDemo (MotorMotionSetting *mcstr, const u8 mtfa[], const u8 mbla[], u8 tac, u8 bac);
void MusicPlayerCallback (MotorMotionSetting *mcstr);

//====================================================================================================
//code by </MATRIX>@Neod Anderjon
