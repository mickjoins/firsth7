/*
 * @Author: 星必尘Sguan
 * @Date: 2025-04-27 21:49:12
 * @LastEditors: 星必尘Sguan|3464647102@qq.com
 * @LastEditTime: 2025-04-29 21:51:28
 * @FilePath: \test_SPIscreen\Hardware\touch.c
 * @Description: [已完成]触控主函数的编写
 * 
 * Copyright (c) 2025 by $JUST, All Rights Reserved. 
 */
#include "touch.h" 
#include "lcd.h"
#include "delay.h"
#include "stdlib.h"
#include "math.h"

_m_tp_dev tp_dev=
{
	TP_Init,
	NULL,
	0,
	0,
 	0,	
};					
//默认为touchtype=0的数据.
 

uint8_t TP_Init(void)
{			    		   
	if(FT6336_Init())
	{
		return 1;
	}
	tp_dev.scan=FT6336_Scan;	//扫描函数指向GT911触摸屏扫描
	return 0;
}

