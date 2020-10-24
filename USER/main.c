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

#define Flash_ADDR    0x1000		//����flash��ʼλ��
#define Read_SIZE    2048

typedef  void (*iapfun)(void);				//����һ���������͵Ĳ���.

#define FLASH_APP1_ADDR		0x08005000  	//��һ��Ӧ�ó�����ʼ��ַ(�����FLASH)
											//����0X08000000~0X08008FFF�Ŀռ�ΪIAPʹ��
#define FLASH_APP2_ADDR		0x08070000  	//�ڶ���Ӧ�ó�����ʼ��ַ(��ű��ó���)

iapfun jump2app; 
	
int main(void)
{	 
	u32 Size;
	u16 l,read_num;	
	u8 pack,*crc32_file,write_buf2[4] = {0},key;
	
	u16 *iapbuf; 
	
	u32 fwaddr=FLASH_APP1_ADDR;//��ǰд��ĵ�ַ
	u32 readaddr=Flash_ADDR+4;//��ǰд��ĵ�ַ
	u16 t,tum;
	u16 temp;
	
//	IWDG_Init(6,3125);
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200

	W25QXX_Init();				//��ʼ��W25Q128
	KEY_Init();
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
//	IWDG_Feed();//ι��
	delay_ms(10);
	printf("Init ok!");

	while(1)
	{
		IWDG_Feed();//ι��
		iapbuf = mymalloc(SRAMIN,2300);
		crc32_file = mymalloc(SRAMIN,2300);     //�����ڴ�
		W25QXX_Read(crc32_file,Flash_ADDR,4);
		Size = crc32_file[1]*0x10000+crc32_file[2]*0x100+crc32_file[3];
		if(crc32_file[0] == 0xF6)//�ж��ļ�CRC32
		{	
			printf("��ʼ���¹̼�\r\n");
			if(Size%Read_SIZE != 0) 
				pack = Size/Read_SIZE + 1;
			else pack = Size/Read_SIZE;
			printf("pack��%d\r\n",pack);
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
						fwaddr+=2048;//ƫ��2048  16=2*8.����Ҫ����2.
					}
				}	
				if(tum)STMFLASH_Write(fwaddr,iapbuf,tum);//������һЩ�����ֽ�д��ȥ. 
				delay_ms(10);	
			}
			W25QXX_Write((u8*)write_buf2,Flash_ADDR,4);
			printf("�̼��������!\r\n");	
		}	
		myfree(SRAMIN,iapbuf);
		myfree(SRAMIN,crc32_file);   //�ͷ��ڴ�	
		printf("�й̼�\r\n");
		
		key=KEY_Scan(0);
		if(key == WKUP_PRES)
		{
			if(((*(vu32*)(FLASH_APP2_ADDR+4))&0xFF000000)==0x08000000)//�ж��Ƿ�Ϊ0X08XXXXXX.
			{	
				if(((*(vu32*)FLASH_APP2_ADDR)&0x2FFE0000)==0x20000000)	//���ջ����ַ�Ƿ�Ϸ�.
				{ 
					jump2app=(iapfun)*(vu32*)(FLASH_APP2_ADDR+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
					MSR_MSP(*(vu32*)FLASH_APP2_ADDR);					//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
					jump2app();									//��ת��APP.
				}	
			}	
		}
		else
		{
			if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//�ж��Ƿ�Ϊ0X08XXXXXX.
			{	
				if(((*(vu32*)FLASH_APP1_ADDR)&0x2FFE0000)==0x20000000)	//���ջ����ַ�Ƿ�Ϸ�.
				{ 
					jump2app=(iapfun)*(vu32*)(FLASH_APP1_ADDR+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
					MSR_MSP(*(vu32*)FLASH_APP1_ADDR);					//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
					jump2app();									//��ת��APP.
				}	
			}	
		}			
	}
}




