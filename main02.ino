/*
    本程序需要安装如下库（括号里是库的作者）：
      - DHT kxn ( by Adafruit ) V3.4.4
      - LiquidCrystal I2C ( by Frank de Brabander )
*/

/*--------------------------------------------------------------------------1--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------本程序所引用库--------------------------------------------------------------------*/
#include <DHT.h>                // 调用库文件DHT.h，允许使用DHT系列传感器（例如DHT11和DHT22）来测量温度和湿度
#include <Wire.h>               // 用于控制I2C通信协议,允许控制其他I2C协议的设备
#include <LiquidCrystal_I2C.h>  // 允许通过I2C总线与LCD显示屏进行通信和控制
#include <Servo.h>              // 提供一系列的与servop控制相关的函数和声明，可以方便的控制servo
#include "HX711.h"

/*--------------------------------------------------------------------------2--------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------声明定义-----------------------------------------------------------------------*/
/*****************************< 常量声明 >*****************************/
// DHT温湿度传感器 型号
#define DHTTYPE DHT22               // 注意型号，有DHT11 DHT22 DHT21等可以替换
const int scale_factor = 427; // 比例参数，从校正程序中取得
#define KEYPAD_ROWS 4               // 4行
#define KEYPAD_COLS 4               // 4列
#define Membrane_Debounce_Time 20  // 薄膜按键消抖时间
// 为键盘上的每个位置赋值
const char Key_Map[KEYPAD_ROWS][KEYPAD_COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

/*****************************< 引脚定义 >*****************************/
const int DHT_Pin = 2;         // DHT传感器的信号脚
const int SCK_Pin = A0;  // 称重模块SCK
const int DT_Pin = A1;   // 称重模块DT
const int Row_Pins[KEYPAD_ROWS] = { 12, 11, 10, 9 };
const int Col_Pins[KEYPAD_COLS] = { 8, 7, 6, 5 };
const int Servo_Pin = 3;  // 舵机信号脚

/*****************************< 变量定义 >*****************************/
float Humidity, Temperature_C;  // 分别是 湿度，温度(℃)，温度(℉)
float HX711_Buffer = 0;          // 存放当前称重的缓冲数据
float Weight_Maopi = 0;          // 存放称重的毛皮数据
float Weight_Shiwu = 0;          // 存放称重的最终得到的重量数据
long Weight;

bool Alarm_FG = true;

String inputPassword = "";  // 用户输入的密码

/*****************************< 对象声明 >*****************************/
DHT dht(DHT_Pin, DHTTYPE);           // 创建一个DHT对象，命名为dht，该对象可以调用DHT库里的函数
LiquidCrystal_I2C lcd(0x27, 16, 2);  // IIC地址，每行几个，共几行
Servo my_Servo;                      // 创建舵机对象
HX711 scale;
/*--------------------------------------------------------------------------3--------------------------------------------------------------------------*/
/*-------------------------------------------------------------------setup()程序初始化------------------------------------------------------------------*/
void setup() {
  /******************************< 启动串口通讯 >******************************/
  Serial.begin(9600);  // 初始化串口通信，并设置波特率为9600

  /******************************设置引脚模式******************************/
  for (int row = 0; row < KEYPAD_ROWS; row++) {
    pinMode(Row_Pins[row], INPUT);      // 设置 行 为 输入模式
    digitalWrite(Row_Pins[row], HIGH);  // 先设定为HIGH
  }
  for (int col = 0; col < KEYPAD_COLS; col++) {
    pinMode(Col_Pins[col], OUTPUT);     // 设置 列 为 输出模式
    digitalWrite(Col_Pins[col], HIGH);  // 先设定为HIGH
  }

  /*******************************初始化CLD********************************/
  lcd.init();       // 初始化LCD
  lcd.backlight();  // 开启背光

  /**********************< 初始化DHT传感器，并确保可用性 >**********************/
  init_DHT(1);

  /******************************获取毛皮重量******************************/
 Serial.begin(9600);
  Serial.println("Initializing the scale");
  scale.begin(DT_Pin, SCK_Pin);
  Serial.println("Before setting up the scale:");
  Serial.println(scale.get_units(5), 0); // 未设定比例参数前的数值
  scale.set_scale(scale_factor);         // 设定比例参数
  scale.tare();                          // 归零
  Serial.println("After setting up the scale:");
  Serial.println(scale.get_units(5), 0); // 设定比例参数后的数值
  Serial.println("Readings:");           // 在这个讯息之前都不要放东西在电子秤上

  /******************************< 设置servo引脚 >******************************/
  my_Servo.attach(Servo_Pin, 500, 2500);  // 转满180°，解决一些开发板只能转90° 的问题
  my_Servo.write(0);
}


/*--------------------------------------------------------------------------4--------------------------------------------------------------------------*/
/*-------------------------------------------------------------------loop()基础循环体-------------------------------------------------------------------*/
void loop() {

  char key = getkey();
  if (key != 0) {  // 这里是ASCII码不为0，不是字符0
    Serial.println(key);
    if (key == '*') {
      int num = inputPassword.toInt();
      constrain(num, 5, 20);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Out of food:");
      lcd.print(num);
      while (scale.get_units(10) < num) {
        my_Servo.write(40);
      }
      my_Servo.write(0);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Release complete");
      delay(1000);
      inputPassword = "";
    } else {
      inputPassword += key;
    }
  }

  Weight = scale.get_units(10);                 // 读取称重数据
    if (Weight < 0) {
    Weight = 0; // 如果重量为负数，则设置为0
  }
  Temperature_C = dht.readTemperature();  // 通过DHT传感器得到温度数值，并赋值给Temperature_C/F
  Humidity = dht.readHumidity();          // 通过DHT传感器得到湿度数值，并赋值给Humidity
  if (Humidity > 60 || Temperature_C > 30) {
    if (Alarm_FG) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Environmental ano");
      lcd.setCursor(0, 1);
      lcd.print("maly, Consume");

      Alarm_FG = false;
    }
  } else {
    refresh_LCD(2000);
    Alarm_FG = true;
  }
}


/*--------------------------------------------------------------------------5--------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------函数定义-----------------------------------------------------------------------*/
void refresh_LCD(int refreshTime) {
  static unsigned long LCD_Clear_Time = 0;  // 在函数内部声明LCD_Clear_Time为静态变量
  // 判断是否到达指定的刷新时间
  if (millis() - LCD_Clear_Time > refreshTime) {
    LCD_Clear_Time = millis();  // 重新记录上一次清屏时间

    lcd.clear();  // 清除屏幕所有内容，并且将游标回到原点
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(Temperature_C);
    lcd.setCursor(8, 0);
    lcd.print("H:");
    lcd.print(Humidity);
    lcd.setCursor(0, 1);
    lcd.print("Weight:");
    lcd.print(Weight);
    lcd.print("g");
  }
}

/*****************************************************************************************************
 * 函数：init_DHT(bool DF)
 * 作用：初始化DHT传感器，检查传感器是否正常工作
 * 参数：
 *   - DF: 标记决定因素（如果为 true，则在传感器初始化失败时进入死循环）
 * 返回值：无
*****************************************************************************************************/
void init_DHT(bool DF) {
  dht.begin();                                                                                          // 初始化对象dht
  if (isnan(dht.readHumidity()) || isnan(dht.readTemperature()) || isnan(dht.readTemperature(true))) {  // 如果没有正确的数据
    Serial.print("DHT Failed!");
    if (DF) {  // 如果标记决定因素，则进入死循环
      while (1)
        ;
    } else {
      delay(1000);  // 如果没有标记决定因素，能正常进入程序
    }
  } else {
    Serial.print("DHT OK!");
    delay(1000);
  }
}
long get_Weight() {
  HX711_Buffer = HX711_Read();  // 读取采样数据
  Weight_Shiwu = HX711_Buffer;
  Weight_Shiwu = Weight_Shiwu - Weight_Maopi;              // 获取实物的AD采样数值。
  //Weight_Shiwu = (float)Weight_Shiwu / GAP_VALUE;  // 转为重量，单位kg
  return Weight_Shiwu*1000;
}
unsigned long HX711_Read() {
  unsigned long count;
  unsigned char i;
  // 准备读取HX711模块的数据
  digitalWrite(DT_Pin, HIGH);
  delayMicroseconds(1);
  digitalWrite(SCK_Pin, LOW);
  delayMicroseconds(1);

  count = 0;
  // 检测数据引脚（HX711_DT_Pin）是否为高电平，如果是则等待，直到数据引脚变为低电平。
  while (digitalRead(DT_Pin))
    ;
    
  for (i = 0; i < 24; i++) {
    digitalWrite(SCK_Pin, HIGH);
    delayMicroseconds(1);
    count = count << 1;  // 将count左移一位，相当于将count乘以2
    digitalWrite(SCK_Pin, LOW);
    delayMicroseconds(1);
    if (digitalRead(DT_Pin))  // 如果数据引脚为高电平，则将count加1，表示该位为1。
      count++;
  }
  digitalWrite(SCK_Pin, HIGH);
  count ^= 0x800000;  // 将count与0x800000进行异或操作，相当于将count的最高位取反
  delayMicroseconds(1);
  digitalWrite(SCK_Pin, LOW);
  delayMicroseconds(1);
  return (count);
}
char getkey() {
  char key = 0;
  for (int col = 0; col < KEYPAD_COLS; col++) {    // 遍历3列
    digitalWrite(Col_Pins[col], LOW);              // 将每一列变成LOW（列视为接地）
    for (int row = 0; row < KEYPAD_ROWS; row++) {  // 每一列遍历4行
      if (digitalRead(Row_Pins[row]) == LOW) {     // 检测每一行是否有按钮按下，按下为LOW
        delay(Membrane_Debounce_Time);             // 防抖
        while (digitalRead(Row_Pins[row]) == LOW)  // 按着就保持为LOW，直到松手
          ;
        key = Key_Map[row][col];  // 找到按钮按下的值
      }
    }
    digitalWrite(Col_Pins[col], HIGH);  // 将列变回HIGH
  }
  return key;
}