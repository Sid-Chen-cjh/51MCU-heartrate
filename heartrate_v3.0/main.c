#include <REGX52.H>
#include <intrins.h>

#define uint            unsigned int
#define uchar           unsigned char
#define ulong           unsigned long
#define LCD_DATA        P0				// 定义LCD的数据端口

// 定义特殊功能寄存器位
sbit LCD_RS =P2^5;	// LCD命令/数据选择位
sbit LCD_RW =P2^6;	// LCD读/写选择位
sbit LCD_E  =P2^7;	// LCD使能信号

sbit Xintiao =P1^0;	// 心跳输入信号
sbit speaker =P2^4;	// 蜂鸣器输出信号

// 延时函数声明
void delay5ms(void);
// LCD操作函数声明
void LCD_WriteData(uchar LCD_1602_DATA);
void LCD_WriteCom(uchar LCD_1602_COM);
void lcd_1602_word(uchar Adress_Com,uchar Num_Adat,uchar *Adress_Data);
void InitLcd();	// LCD初始化
// 定时器初始化函数声明
void Tim_Init();

// 全局变量声明
uchar Xintiao_Change=0;		// 心跳计数状态标志
uint  Xintiao_Jishu;		// 心跳周期计数值
uchar stop;		// 心跳检测停止标志
uchar View_Data[3];		// 显示数据缓冲区
uchar View_L[3];		// 低警告值显示缓冲区
uchar View_H[3];		// 高警告值显示缓冲区
uchar Xintiao_H=100;		// 高警告阈值
uchar Xintiao_L=40;		// 低警告阈值


uchar Key_Change;		// 按键状态改变标志
uchar Key_Value;		// 按键值
uchar View_Con;		// 当前显示界面索引
uchar View_Change;		// 显示更新标志

void main()
{
	InitLcd();
	Tim_Init();		// 初始化LCD和定时器
	lcd_1602_word(0x80,16,"Heart Rate:     ");
	TR0=1;
	TR1=1;		// 开启定时器0和定时器1，进入主循环监控按键输入与心跳速率，并根据状态更新显示
	while(1)
	{
		if(Key_Change)
		{
			Key_Change = 0;		//复位
			View_Change = 1;
			switch(Key_Value)
			{
				case 1:
				{
					View_Con++;
					if(View_Con == 3)
						View_Con = 0;
					break;		//0-1-2循环
				}
				case 2:
				{
					if(View_Con == 2)
					{
						if(Xintiao_H < 150)
							Xintiao_H++;
					}
					if(View_Con == 1)
					{
						if(Xintiao_L < Xintiao_H-1)
							Xintiao_L++;
					}
					break;
				}
				case 3:
				{
					if(View_Con == 2)
					{
						if(Xintiao_H > Xintiao_L+1)
							Xintiao_H--;
					}
					if(View_Con == 1)
					{
						if(Xintiao_L > 30)
							Xintiao_L--;
					}
					break;
				} 
			}
		}
		if(View_Change)
		{
			View_Change = 0;
			if(stop == 0)
			{
				if(View_Data[0] == 0x30)
					View_Data[0] = ' ';
			}
			else
			{
				View_Data[0] = ' ';
				View_Data[1] = ' ';
				View_Data[2] = ' ';
			}

			switch(View_Con)
			{
				case 0:
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0xc0,16,"                ");
					lcd_1602_word(0xcd,3,View_Data);
					break;
				}
				case 1:
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_L[0] = Xintiao_L / 100 + 0x30;
					View_L[1] = Xintiao_L % 100 / 10 + 0x30;
					View_L[2] = Xintiao_L % 10 + 0x30;

					if(View_L[0] == 0x30)
						View_L[0] = ' ';
			   
					lcd_1602_word(0xC0,16,"Warning L :     ");
					lcd_1602_word(0xCd,3,View_L);
					break;
				}
				case 2:
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_H[0] = Xintiao_H / 100 + 0x30;
					View_H[1] = Xintiao_H % 100 / 10 + 0x30;
					View_H[2] = Xintiao_H % 10 + 0x30;

					if(View_H[0] == 0x30)
						View_H[0] = ' ';
			   
					lcd_1602_word(0xC0,16,"Warning H :     ");
					lcd_1602_word(0xCd,3,View_H);
					break;
				}
			}
		}
	}
}

void Time1() interrupt 3
{
	static uchar Key_Con,Xintiao_Con;
	TH1=0xd8;
	TL1=0xf0;
	switch(Key_Con)
	{
		case 0:
		{
			if((P3 & 0x07) != 0x07)
			{
				Key_Con++;
			}
			break;
		}
		case 1:
		{
			if((P3 & 0x07) != 0x07)
			{
				Key_Con++;
				switch(P3 & 0x07)
				{
					case 0x06: Key_Value = 1; break;
					case 0x05: Key_Value = 2; break;
					case 0x03: Key_Value = 3; break;
				}
			}
			else
			{
				Key_Con = 0;
			}
			break;
		}
		case 2:
		{
			if((P3&0x07) == 0x07)
			{
				Key_Change = 1;
				Key_Con = 0;
			}
			break;
		}
	}
	
	switch (Xintiao_Con)
	{
		case 0:
		{
			if(!Xintiao)
			{
				Xintiao_Con++;
			}
			break;
		}
		case 1:
		{
			if(!Xintiao)
			{
				Xintiao_Con++;
			}
			else
			{
				Xintiao_Con = 0;
			}
			break;
		}
		case 2:
		{
			if(!Xintiao)
			{
				Xintiao_Con++;
			}
			else
			{
				Xintiao_Con = 0;
			} 
			break;
		}
		case 3:
		{
			if(!Xintiao)
			{
				Xintiao_Con++;
			}
			else
			{
				Xintiao_Con = 0;
			} 
			break;
		}
		case 4:
		{
			if(Xintiao)
			{
				if(Xintiao_Change == 1)
				{
					View_Data[0] = (60000 / Xintiao_Jishu) / 100 + 0x30;
					View_Data[1] = (60000 / Xintiao_Jishu) % 100 / 10 + 0x30;
					View_Data[2] = (60000 / Xintiao_Jishu) % 10 + 0x30;
												  
					if(((60000/Xintiao_Jishu)>=Xintiao_H)||((60000/Xintiao_Jishu)<=Xintiao_L))
						speaker = 0;
					else
						speaker = 1;
					
					View_Change = 1;
					Xintiao_Jishu = 0;
					Xintiao_Change = 0;
					stop = 0;			}
				else
				{
					Xintiao_Jishu = 0;
					Xintiao_Change = 1;
				}
				Xintiao_Con = 0;
				break;
			}
		}
	}
}

void Time0() interrupt 1
{
	TH0=0xfc;
	TL0=0x18;
	Xintiao_Jishu++;
	if(Xintiao_Jishu == 5000)
	{
		Xintiao_Jishu = 0;
		View_Change = 1;
		Xintiao_Change = 0;
		stop = 1;
		speaker = 1;
	}
}

void Tim_Init()
{
	EA = 1;
	ET0 = 1;
	ET1 = 1;
	TMOD = 0x11;
	TH0 = 0xfc;
	TL0 = 0x18;
 
	TH1 = 0xd8;
	TL1 = 0xf0;
}

void lcd_1602_word(uchar Adress_Com, uchar Num_Adat, uchar *Adress_Data)
{
	uchar a = 0;
	uchar Data_Word;
	LCD_WriteCom(Adress_Com);
	for(a = 0; a < Num_Adat; a++)
	{
		Data_Word = *Adress_Data;
		LCD_WriteData(Data_Word);
		Adress_Data++;
	}
}

/***************1602*******************/
void LCD_WriteData(uchar LCD_1602_DATA)
{
	delay5ms();
	LCD_E = 0;
	LCD_RS = 1;
	LCD_RW = 0;
	_nop_();
	LCD_E = 1;
	LCD_DATA = LCD_1602_DATA;
	LCD_E = 0;
	LCD_RS = 0;
}


void LCD_WriteCom(uchar LCD_1602_COM)
{
	delay5ms();
	LCD_E = 0;
	LCD_RS = 0;
	LCD_RW = 0;
	_nop_();
	LCD_E = 1;
	LCD_DATA = LCD_1602_COM;
	LCD_E = 0;
	LCD_RS = 0;
}


void InitLcd()
{
	delay5ms();
	delay5ms();
	LCD_WriteCom(0x38); //display mode
	LCD_WriteCom(0x38); //display mode
	LCD_WriteCom(0x38); //display mode
	LCD_WriteCom(0x06);
	LCD_WriteCom(0x0c);
	LCD_WriteCom(0x01);
	delay5ms();
	delay5ms();
}

void delay5ms(void)
{
	unsigned char a,b;
	for(b = 185; b > 0; b--)
		for(a = 12; a > 0; a--);
}
