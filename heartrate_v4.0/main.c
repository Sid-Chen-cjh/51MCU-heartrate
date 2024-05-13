/*
 * 这个程序是一个简单的心跳速率监测和警告系统，使用了LCD显示心率数值以及高低警告值。
 * 通过按键可以循环切换显示当前心率、低警告值和高警告值界面，并且可以对警告值进行设置。
 */
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

// 主函数：初始化LCD和定时器，进入主循环监控按键输入与心跳速率，并根据状态更新显示
void main()
{
	InitLcd();
	Tim_Init();		// 初始化LCD和定时器
	lcd_1602_word(0x80,16,"Heart Rate:     ");	// 在LCD上显示初始界面
	TR0=1;
	TR1=1;		// 开启定时器0和定时器1，进入主循环监控按键输入与心跳速率，并根据状态更新显示
	while(1)
	{
		// 检测按键状态变化，并根据按键值更新显示界面或修改警告值
		if(Key_Change)
		{
			Key_Change = 0;		//复位按键状态改变标志
			View_Change = 1;	// 触发显示更新标志
			switch(Key_Value)
			{
				case 1:
				{
					View_Con++;
					if(View_Con == 3)	// 如果当前界面是高警告值设置界面，则循环回到低警告值设置界面
						View_Con = 0;
					break;		//0-1-2循环
				}
				case 2:
				{
					if(View_Con == 2)	// 在高警告值设置界面时，增加高警告值
					{
						if(Xintiao_H < 150)
							Xintiao_H++;
					}
					if(View_Con == 1)	// 在低警告值设置界面时，增加低警告值
					{
						if(Xintiao_L < Xintiao_H-1)
							Xintiao_L++;
					}
					break;
				}
				case 3:
				{
					if(View_Con == 2)	// 在高警告值设置界面时，减少高警告值
					{
						if(Xintiao_H > Xintiao_L+1)
							Xintiao_H--;
					}
					if(View_Con == 1)	 // 在低警告值设置界面时，减少低警告值
					{
						if(Xintiao_L > 30)
							Xintiao_L--;
					}
					break;
				} 
			}
		}
		// 根据显示更新标志更新LCD显示内容
		if(View_Change)
		{
			View_Change = 0;	// 复位显示更新标志
			if(stop == 0)	// 如果心跳检测未停止，更新显示数据
			{
				if(View_Data[0] == 0x30)
					View_Data[0] = ' ';	// 将空数据字符替换为空格
			}
			else	// 如果心跳检测已停止，清空显示数据
			{
				View_Data[0] = ' ';
				View_Data[1] = ' ';
				View_Data[2] = ' ';
			}

			switch(View_Con)	// 根据当前显示界面，更新相应的显示内容
			{
				case 0:	// 当前显示心率界面
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0xc0,16,"                ");
					lcd_1602_word(0xcd,3,View_Data);
					break;
				}
				case 1:	 // 当前显示低警告值设置界面
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_L[0] = Xintiao_L / 100 + 0x30;	// 将低警告值转换为字符形式
					View_L[1] = Xintiao_L % 100 / 10 + 0x30;
					View_L[2] = Xintiao_L % 10 + 0x30;

					if(View_L[0] == 0x30)
						View_L[0] = ' ';	// 处理低警告值显示字符前的空字符
			   
					lcd_1602_word(0xC0,16,"Warning L :     ");
					lcd_1602_word(0xCd,3,View_L);
					break;
				}
				case 2:	// 当前显示高警告值设置界面
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_H[0] = Xintiao_H / 100 + 0x30;	// 将高警告值转换为字符形式
					View_H[1] = Xintiao_H % 100 / 10 + 0x30;
					View_H[2] = Xintiao_H % 10 + 0x30;

					if(View_H[0] == 0x30)
						View_H[0] = ' ';	// 处理高警告值显示字符前的空字符
			   
					lcd_1602_word(0xC0,16,"Warning H :     ");
					lcd_1602_word(0xCd,3,View_H);
					break;
				}
			}
		}
	}
}

/* 定时器1中断服务函数：处理按键输入和心跳检测 */
void Time1() interrupt 3
{
	static uchar Key_Con,Xintiao_Con;
	TH1=0xd8;
	TL1=0xf0;
	switch(Key_Con)
	{
		case 0:	// 按键按下检测
		{
			if((P3 & 0x07) != 0x07)
			{
				Key_Con++;
			}
			break;
		}
		case 1:	// 确认按键按下
		{
			if((P3 & 0x07) != 0x07)
			{
				Key_Con++;
				switch(P3 & 0x07)	// 确定是哪个按键按下
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
		case 2:	// 等待按键释放
		{
			if((P3&0x07) == 0x07)
			{
				Key_Change = 1;	// 触发按键状态改变标志
				Key_Con = 0;
			}
			break;
		}
	}
	
	switch (Xintiao_Con)	// 处理心跳信号输入
	{
		case 0:	// 等待心跳信号
		{
			if(!Xintiao)
			{
				Xintiao_Con++;
			}
			break;
		}
		case 1:	// 确认心跳信号未收到
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
		case 2:	// 等待下一个心跳周期
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
		case 3:	// 等待确认心跳周期
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
		case 4:	 // 处理心跳周期计数和更新显示
		{
			if(Xintiao)
			{
				if(Xintiao_Change == 1)	// 如果是新的心跳周期
				{
					View_Data[0] = (60000 / Xintiao_Jishu) / 100 + 0x30;	// 计算并转换为字符形式
					View_Data[1] = (60000 / Xintiao_Jishu) % 100 / 10 + 0x30;
					View_Data[2] = (60000 / Xintiao_Jishu) % 10 + 0x30;
												  
					if(((60000/Xintiao_Jishu)>=Xintiao_H)||((60000/Xintiao_Jishu)<=Xintiao_L))
						speaker = 0;	// 关闭蜂鸣器
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
 /* * 函数名称：Time0
  * 描述：定时器0中断服务函数，用于实现特定时间间隔的任务。
  */
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
/* * 函数名称：Tim_Init
 * 功能描述：初始化定时器0和定时器1，设置为模式1，启动定时器。
 */
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
/* * @brief 向LCD 1602显示器发送一个字节的数据流.
 * 该函数通过指定的命令地址和数据地址，将一个字节的数据流发送到LCD 1602显示器.
 * * @param Adress_Com 命令地址。用于指定LCD要执行的操作。
 * * @param Num_Adat 要发送的数据字节数。
 * * @param Adress_Data 数据指针。指向要发送的数据的起始位置。
 */
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

void LCD_WriteData(uchar LCD_1602_DATA)
/* * @brief 向LCD写入数据
 * 该函数用于向1602型号的LCD写入一个字节的数据。在写入数据前，会先进行一个5ms的延时，然后设置LCD的相关控制信号线，
 * 包括使能信号(E)，寄存器选择信号(RS)，读写信号(RW)，最后写入数据并恢复控制信号线的状态。
 * * @param LCD_1602_DATA 要写入LCD的数据，类型为uchar（无符号字符）
 */
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
/* * @brief 向LCD写入命令
 * 该函数用于向1602型号的LCD写入指定的命令。
 * 写入命令的过程包括：设置LCD使能信号、设置LCD命令/数据选择信号、
 * 设置LCD读写信号、写入数据并恢复初始状态。
 * * @param LCD_1602_COM 要写入LCD的命令字节
 */
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
/* * 初始化LCD显示器
 * 该函数初始化LCD显示器，设置其显示模式并进行必要的初始化序列。
 */
{
	delay5ms();
	delay5ms();
	LCD_WriteCom(0x38); 
	LCD_WriteCom(0x38); 
	LCD_WriteCom(0x38); 
	LCD_WriteCom(0x06);
	LCD_WriteCom(0x0c);
	LCD_WriteCom(0x01);
	delay5ms();
	delay5ms();
}

void delay5ms(void)	//@12.000MHz
{
	unsigned char data i, j;

	i = 10;
	j = 183;
	do
	{
		while (--j);
	} while (--i);
}
