#include "sys.h"
#include "delay.h"
#include "usart.h"      
#include "malloc.h"
#include "w25qxx.h"    
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wdg.h"
#include "stmflash.h"
#include "key.h"

#define Flash_ADDR    0x1000		//存入flash起始位置
#define Read_SIZE    2048

typedef  void (*iapfun)(void);				//定义一个函数类型的参数.

#define FLASH_APP1_ADDR		0x08005000  	//第一个应用程序起始地址(存放在FLASH)
											//保留0X08000000~0X08008FFF的空间为IAP使用
#define FLASH_APP2_ADDR		0x08070000  	//第二个应用程序起始地址(存放备用程序)

iapfun jump2app; 
	
int main(void)
{	 
	u32 Size;
	u16 l,read_num;	
	u8 pack,*crc32_file,write_buf2[4] = {0},key;
	
	u16 *iapbuf; 
	
	u32 fwaddr=FLASH_APP1_ADDR;//当前写入的地址
	u32 readaddr=Flash_ADDR+4;//当前写入的地址
	u16 t,tum;
	u16 temp;
	
//	IWDG_Init(6,3125);
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200

	W25QXX_Init();				//初始化W25Q128
	KEY_Init();
 	my_mem_init(SRAMIN);		//初始化内部内存池
//	IWDG_Feed();//喂狗
	delay_ms(10);
	printf("Init ok!");

	while(1)
	{
		IWDG_Feed();//喂狗
		iapbuf = mymalloc(SRAMIN,2300);
		crc32_file = mymalloc(SRAMIN,2300);     //申请内存
		W25QXX_Read(crc32_file,Flash_ADDR,4);
		Size = crc32_file[1]*0x10000+crc32_file[2]*0x100+crc32_file[3];
		if(crc32_file[0] == 0xF6)//判断文件CRC32
		{	
			printf("开始更新固件\r\n");
			if(Size%Read_SIZE != 0) 
				pack = Size/Read_SIZE + 1;
			else pack = Size/Read_SIZE;
			printf("pack：%d\r\n",pack);
			for(l=0;l<pack;l++)
			{
				if(l == (pack-1))
					read_num = Size - (Read_SIZE*l);
				else read_num = Read_SIZE;
				W25QXX_Read(crc32_file,readaddr,read_num);
				readaddr += read_num;
				for(t=0;t<read_num;t+=2)
				{						    
					temp=(u16)crc32_file[t+1]<<8;
					temp+=(u16)crc32_file[t];	  
					iapbuf[tum++]=temp;	    
					if(tum==(1024))
					{
						tum=0;
						STMFLASH_Write(fwaddr,iapbuf,1024);	
						fwaddr+=2048;//偏移2048  16=2*8.所以要乘以2.
					}
				}	
				if(tum)STMFLASH_Write(fwaddr,iapbuf,tum);//将最后的一些内容字节写进去. 
				delay_ms(10);	
			}
			W25QXX_Write((u8*)write_buf2,Flash_ADDR,4);
			printf("固件更新完成!\r\n");	
		}	
		myfree(SRAMIN,iapbuf);
		myfree(SRAMIN,crc32_file);   //释放内存	
		printf("有固件\r\n");
		
		key=KEY_Scan(0);
		if(key == WKUP_PRES)
		{
			if(((*(vu32*)(FLASH_APP2_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
			{	
				if(((*(vu32*)FLASH_APP2_ADDR)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
				{ 
					jump2app=(iapfun)*(vu32*)(FLASH_APP2_ADDR+4);		//用户代码区第二个字为程序开始地址(复位地址)		
					MSR_MSP(*(vu32*)FLASH_APP2_ADDR);					//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
					jump2app();									//跳转到APP.
				}	
			}	
		}
		else
		{
			if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
			{	
				if(((*(vu32*)FLASH_APP1_ADDR)&0x2FFE0000)==0x20000000)	//检查栈顶地址是否合法.
				{ 
					jump2app=(iapfun)*(vu32*)(FLASH_APP1_ADDR+4);		//用户代码区第二个字为程序开始地址(复位地址)		
					MSR_MSP(*(vu32*)FLASH_APP1_ADDR);					//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
					jump2app();									//跳转到APP.
				}	
			}	
		}			
	}
}




