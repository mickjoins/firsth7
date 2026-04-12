/*
 * @Author: �Ǳس�Sguan
 * @Date: 2025-04-27 21:49:12
 * @LastEditors: �Ǳس�Sguan|3464647102@qq.com
 * @LastEditTime: 2025-04-29 21:51:28
 * @FilePath: \test_SPIscreen\Hardware\touch.c
 * @Description: [�����]�����������ı�д
 * 
 * Copyright (c) 2025 by $JUST, All Rights Reserved. 
 */
#include "touch.h" 
#include "lcd.h"
#include "i2c.h"
#include <stdlib.h>
#include <math.h>

_m_tp_dev tp_dev = {
	TP_Init,
	NULL,
	{ 0 },
	{ 0 },
 	0,
};					
//Ĭ��Ϊtouchtype=0������.
 

uint8_t TP_Init(void)
{
	ft6336_init(&hi2c1);
	tp_dev.scan = ft6336_scan;
	return 0;
}

