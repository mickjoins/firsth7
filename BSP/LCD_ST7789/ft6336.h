#ifndef __FT6336_H
#define __FT6336_H	

#include "i2c.h"

#define FT_CMD_WR 				0X70    	
#define FT_CMD_RD 				0X71		
  
#define FT_DEVIDE_MODE 			0x00   		
#define FT_REG_NUM_FINGER       0x02		  

#define FT_TP1_REG 				0X03	  	
#define FT_TP2_REG 				0X09		  

#define FT_ID_G_CIPHER_MID      0x9F      
#define FT_ID_G_CIPHER_LOW      0xA0      
#define	FT_ID_G_LIB_VERSION		0xA1		  
#define FT_ID_G_CIPHER_HIGH     0xA3      
#define FT_ID_G_MODE 			0xA4   		
#define FT_ID_G_FOCALTECH_ID    0xA8      
#define FT_ID_G_THGROUP			0x80   		
#define FT_ID_G_PERIODACTIVE	0x88   		

extern uint8_t pressed;
extern uint16_t x_pos;
extern uint16_t y_pos;

void ft6336_init(I2C_HandleTypeDef* p);
void ft6336_scan(void);

#endif
