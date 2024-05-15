/*
 * 这个程序是一个简单的心跳速率监测和警告系统，使用了LCD显示心率数值以及高低警告值。
 * 通过按键可以循环切换显示当前心率、低警告值和高警告值界面，并且可以对警告值进行设置。
 */
#include <REGX52.H>
#include <string.h> 
#include <intrins.h>

#define uint            unsigned int
#define uchar           unsigned char
#define ulong           unsigned long
#define LCD_DATA        P0				// 定义LCD的数据端口

// 定义特殊功能寄存器位
sbit LCD_RS =P2^5;	// LCD命令/数据选择位
sbit LCD_RW =P2^6;	// LCD读/写选择位
sbit LCD_E  =P2^7;	// LCD使能信号

sbit heartrate =P1^0;	// 心跳输入信号
sbit buzzer =P2^4;		// 蜂鸣器输出信号

// 延时函数声明
void delay5ms(void);
// LCD操作函数声明
void LCD_WriteData(uchar LCD_1602_DATA);
void LCD_WriteCom(uchar LCD_1602_COM);
void lcd_1602_word(uchar Adress_Com,uchar Num_Adat,uchar *Adress_Data);
void InitLcd();	
// 定时器初始化函数声明
void Tim_Init();
// UART串口通信函数声明
void Uart_Init();
void Uart_SendString(char *str);

// 全局变量声明
uchar heartrate_Change=0;	// 心跳计数状态标志
uint  heartrate_count;		// 心跳周期计数值
uchar stop;					// 心跳检测停止标志
uchar View_Data[3];			// 显示数据缓冲区
uchar View_L[3];			// 低警告值显示缓冲区
uchar View_H[3];			// 高警告值显示缓冲区
uchar heartrate_H=100;		// 高警告阈值
uchar heartrate_L=40;		// 低警告阈值
uchar sentence[51];

uchar Key_Change;		// 按键状态改变标志
uchar Key_Value;		// 按键值
uchar View_Con;			// 当前显示界面索引
uchar View_Change;		// 显示更新标志

// 主函数：初始化LCD和定时器，进入主循环监控按键输入与心跳速率，并根据状态更新显示
void main()
{
	Uart_Init();
	InitLcd();
	Tim_Init();		// 初始化LCD和定时器
	lcd_1602_word(0x80,16,"Heart Rate:     ");	// 在LCD上显示初始界面
	TR0=1;
	TR2=1;		// 开启定时器0和定时器1，进入主循环监控按键输入与心跳速率，并根据状态更新显示
	while(1)
	{
		// 检测按键状态变化，并根据按键值更新显示界面或修改警告值
		if(Key_Change)
		{
			Key_Change = 0;				// 复位按键状态改变标志
			View_Change = 1;			// 触发显示更新标志
			switch(Key_Value)
			{
				case 1:					//设置键按下
				{
					View_Con++;			//设置的位加
					if(View_Con == 3)	// 如果当前界面是高警告值设置界面，则循环回到低警告值设置界面
						View_Con = 0;
					break;				//跳出，下同
				}
				case 2:					//加键按下
				{
					if(View_Con == 2)	// 在高警告值设置界面时，增加高警告值
					{
						if(heartrate_H < 150)
							heartrate_H++;
					}
					if(View_Con == 1)	// 在低警告值设置界面时，增加低警告值
					{
						if(heartrate_L < heartrate_H-1)	//下限值小于上限-1
							heartrate_L++;
					}
					break;
				}
				case 3:					//减键按下
				{
					if(View_Con == 2)	// 在高警告值设置界面时，减少高警告值
					{
						if(heartrate_H > heartrate_L+1)	//上限数据大于下限+1
							heartrate_H--;
					}
					if(View_Con == 1)	 // 在低警告值设置界面时，减少低警告值
					{
						if(heartrate_L > 30)
							heartrate_L--;
					}
					break;
				} 
			}
		}
		/* 根据显示更新标志更新LCD显示内容 */
		if(View_Change)
		{
			View_Change = 0;				// 复位显示更新标志
			if(stop == 0)					// 如果心跳检测正常，更新显示数据
			{
				if(View_Data[0] == 0x30)	// 最高位为0时不显示
					View_Data[0] = ' ';		// 将空数据字符替换为空格
			}
			else	//计数超过5000,也就是两次信号时间超过5s,不显示数据
			{
				View_Data[0] = ' ';
				View_Data[1] = ' ';
				View_Data[2] = ' ';
			}

			switch(View_Con)	// 根据当前显示界面，更新相应的显示内容
			{
				case 0:	// 正常显示当前心率界面
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");	//显示一行数据
					lcd_1602_word(0xc0,16,"                ");	//显示第二行数据
					lcd_1602_word(0xcd,3,View_Data);			//第二行显示心率
					break;
				}
				case 1:	 // 显示低警告值设置界面时
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");	//第一行显示心率
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_L[0] = heartrate_L / 100 + 0x30;		// 将低警告值转换为字符形式
					View_L[1] = heartrate_L % 100 / 10 + 0x30;
					View_L[2] = heartrate_L % 10 + 0x30;

					if(View_L[0] == 0x30)		// 最高位为0时，不显示
						View_L[0] = ' ';		// 处理低警告值显示字符前的空字符
			   
					lcd_1602_word(0xC0,16,"Warning L :     ");	//第二行显示下限数据
					lcd_1602_word(0xCd,3,View_L);
					break;
				}
				case 2:	// 当前显示高警告值设置界面
				{
					lcd_1602_word(0x80,16,"Heart Rate:     ");
					lcd_1602_word(0x8d,3,View_Data);
			   
					View_H[0] = heartrate_H / 100 + 0x30;		// 将高警告值转换为字符形式
					View_H[1] = heartrate_H % 100 / 10 + 0x30;
					View_H[2] = heartrate_H % 10 + 0x30;

					if(View_H[0] == 0x30)		// 最高位为0时，不显示
						View_H[0] = ' ';		// 处理高警告值显示字符前的空字符
			   
					lcd_1602_word(0xC0,16,"Warning H :     ");
					lcd_1602_word(0xCd,3,View_H);
					break;
				}
			}
		}
	}
}

/* 定时器2中断服务函数：处理按键输入和心跳检测 */
void Time2() interrupt 5
{
	static uchar Key_Con,heartrate_Con;
	TH2=0xd8;			//重新赋初值,使得间隔为10ms
	TL2=0xf0;
	switch(Key_Con)		//无按键按下时此值为0
	{
		case 0:			//每10ms扫描此处
		{
			if((P3 & 0x07) != 0x07)	//扫描按键是否有按下
			{
				Key_Con++;			//有按下此值加1，值为1
			}
			break;
		}
		case 1:			//10ms后二次进入中断后扫描此处（Key_Con为1）
		{
			if((P3 & 0x07) != 0x07)		//第二次进入中断时，按键仍然是按下（起到按键延时去抖的作用）
			{
				Key_Con++;				//变量加1，值为2
				switch(P3 & 0x07)		//确定是哪个按键按下
				{
					case 0x06: Key_Value = 1; break;	//判断好按键后将键值赋值给变量Key_Value
					case 0x05: Key_Value = 2; break;
					case 0x03: Key_Value = 3; break;
				}
			}
			else						//如果10ms时没有检测到按键按下（按下时间过短）
			{
				Key_Con = 0;			//变量清零，重新检测按键
			}
			break;
		}
		case 2:			//20ms后检测按键
		{
			if((P3&0x07) == 0x07)		//检测按键是否还是按下状态
			{
				Key_Change = 1;			//有按键按下使能变量，（此变量为1时才会处理键值数据）
				Key_Con = 0;			//变量清零，等待下次有按键按下
			}
			break;
		}
	}
	
	switch (heartrate_Con)	// 处理心跳信号输入
	{
		case 0:	 	//默认heartrate_Con是为0的
		{
			if(!heartrate)			//每10ms（上面的定时器）检测一次脉搏是否有信号
			{
				heartrate_Con++;	//如果有信号，变量加一，程序就会往下走了
			}
			break;
		}
		case 1:		//确认心跳信号未收到
		{
			if(!heartrate)			//每过10ms检测一下信号是否还存在
			{
				heartrate_Con++;	//存在就加一
			}
			else
			{
				heartrate_Con = 0;	//如果不存在了，检测时间很短，说明检测到的不是脉搏信号，可能是其他干扰，将变量清零，跳出此次检测
			}
			break;
		}
		case 2:		//等待下一个心跳周期
		{
			if(!heartrate)
			{
				heartrate_Con++;	//存在就加一
			}
			else
			{
				heartrate_Con = 0;	//如果不存在了，检测时间很短，说明检测到的不是脉搏信号，可能是其他干扰，将变量清零，跳出此次检测
			} 
			break;
		}
		case 3:		//等待确认心跳周期
		{
			if(!heartrate)
			{
				heartrate_Con++;	//存在就加一
			}
			else
			{
				heartrate_Con = 0;	//如果不存在了，检测时间很短，说明检测到的不是脉搏信号，可能是其他干扰，将变量清零，跳出此次检测
			} 
			break;
		}
		case 4:	 	//处理心跳周期计数和更新显示
		{
			if(heartrate)//超过30ms有信号，判定此次是脉搏信号，然后当信号消失后，执行以下程序
			{
				if(heartrate_Change == 1)//心率计原理为检测两次脉冲间隔时间计算心率，变量heartrate_Change第一次脉冲时为0的，所有走下面的else，第二次走这里
				{
					View_Data[0] = (60000 / heartrate_count) / 100 + 0x30;//计算心跳并拆字显示：心跳计时是以1ms为单位，两次心跳中间计数如果是1000次，也就是1000*1ms=1000ms=1s
					View_Data[1] = (60000 / heartrate_count) % 100 / 10 + 0x30;//那么计算出的一分钟（60s）心跳数就是：60*1000/（1000*1ms）=60次	  其中60是一分钟60s，1000是一秒有1000ms，1000是计数值，1是一次计数对应 的时间是1ms
					View_Data[2] = (60000 / heartrate_count) % 10 + 0x30;//计算出的心跳数/100得到心跳的百位，%100是取余的，就是除以100的余数，再除以10就得到十位了，以此类推
					//拆字后的单个数据+0x30的目的是得到对应数字的液晶显示码，数字0对应的液晶显示码是0x30，1是0x30+1，以此类推							  
					if(((60000/heartrate_count)>=heartrate_H)||((60000/heartrate_count)<=heartrate_L))//心率不在范围内报警
					{
						buzzer = 0;		// 蜂鸣器响
						strcpy(sentence, "Abnormal heart rate! Current heart rate value is:");
						Uart_SendString(sentence);
						Uart_SendString(View_Data);
					}
					else
					{
						buzzer = 1;		//不响
						strcpy(sentence, "Heart rate is normal! Current heart rate value is:");
						Uart_SendString(sentence);
						Uart_SendString(View_Data);
					}
					
					View_Change = 1; 		//计算出心率后启动显示
					heartrate_count = 0;	//心跳计数清零
					heartrate_Change = 0;	//计算出心率后该变量清零，准备下次检测心率
					stop = 0;				//计算出心率后stop清零
				}
				else	//第一次脉冲时heartrate_Change为0
				{
					heartrate_count = 0;	//脉冲计时变量清零，开始计时
					heartrate_Change = 1;	//heartrate_Change置1，准备第二次检测到脉冲时计算心率
				}
				heartrate_Con = 0;		//清零，准备检测下一次脉冲
				break;
			}
		}
	}
}

/**定时器T0工作函数**/
void Time0() interrupt 1
{
	TH0=0xfc;		// 设置定时器0的初值,使其中断周期为1ms
	TL0=0x18;
	heartrate_count++;
	if(heartrate_count == 5000)	// 每隔5s，进行一次状态更新
	{
		heartrate_count = 0;	// 重置心率计数器
		View_Change = 1;		// 触发视图更新
		heartrate_Change = 0;	// 重置心率变化标志，准备再次检测
		stop = 1;				// 心跳计数超过5000后说明心率不正常或者没有测出，stop置1
		buzzer = 1;				// 关闭蜂鸣器
	}
}

void lcd_1602_word(uchar Adress_Com, uchar Num_Adat, uchar *Adress_Data)
/* * @brief 向LCD 1602显示器发送一个数据流.
 * 该函数通过指定的命令地址和数据地址，将数据流显示到LCD 1602显示器.
 * * @param Adress_Com 命令地址。用于指定LCD要显示的位置。
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
	delay5ms();		//操作前短暂延时，保证信号稳定
	LCD_E = 0;		//使能置为低电平
	LCD_RS = 1;		//寄存器选择置为高电平，表示传输数据字节
	LCD_RW = 0;		//读写置为低电平,表示写入数据
	_nop_();
	LCD_E = 1;
	LCD_DATA = LCD_1602_DATA;	//写入LCD数据
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
	delay5ms();		//操作前短暂延时，保证信号稳定
	LCD_E = 0;		//使能置为低电平
	LCD_RS = 0;		//寄存器选择置为低电平，表示传输命令字节
	LCD_RW = 0;		//读写置为低电平,表示写入数据
	_nop_();
	LCD_E = 1;
	LCD_DATA = LCD_1602_COM;	//写入LCD命令字节
	LCD_E = 0;
	LCD_RS = 0;
}

void InitLcd()
{
	delay5ms();
	delay5ms();
	LCD_WriteCom(0x38); 		//八位数据接口，两行显示，5*7点阵
	LCD_WriteCom(0x38); 
	LCD_WriteCom(0x38); 
	LCD_WriteCom(0x06);			//显示开，光标关，闪烁关
	LCD_WriteCom(0x0c);			//数据读写操作后，光标自动加一，画面不动
	LCD_WriteCom(0x01);			//光标复位，清屏
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

void Uart_Init(void)	//4800bps@12.000MHz
{
	PCON |= 0x80;		//使能波特率倍速位SMOD
	SCON = 0x50;		//8位数据,可变波特率
	TMOD &= 0x0F;		//设置定时器模式
	TMOD |= 0x20;		//设置定时器模式
	TL1 = 0xF3;			//设置定时初始值
	TH1 = 0xF3;			//设置定时重载值
	ET1 = 0;			//禁止定时器中断
	TR1 = 1;			//定时器1开始计时
	EA = 1;				//总中断使能
	ES = 1;				//打开RI,TI中断
}

void Tim_Init()
{
	EA = 1;				//配置中断寄存器
	ET0 = 1;
	ET2 = 1;
	TMOD = 0x11;		//设置定时器模式
	TH0 = 0xfc;			//设置定时初始值
	TL0 = 0x18;
	TH2 = 0xd8;
	TL2 = 0xf0;
}

void Uart_SendString(char *str) 
{
    while(*str != '\0') // 直到遇到字符串结束符'\0'
	{
        SBUF = *str++; 	// 发送当前字符并移动到下一个字符
        while(!TI); 	// 等待发送完成，TI=1表示发送完毕
        TI = 0; 		// 清除发送中断标志，准备下一次发送
    }
}