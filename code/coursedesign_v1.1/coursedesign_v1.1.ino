#include <AD9833.h>   // 导入AD9833的库
#include <Wire.h> // 导入Wire库给OLED使用
#include <Adafruit_GFX.h> // 导入支持OLED的库
#include <Adafruit_SSD1306.h> // 导入OLED库
#include <math.h> // 导入math库
#define SCREEN_WIDATA_PINH 128 // 设置OLED的像素宽度
#define SCREEN_HEIGHT 64 // 设置OLED的像素高度
#define SET_SWEEP_FREQUENCY A1 //切换成扫频功能
#define INFO_DISPLAY A2 // 设置显示学生信息
#define SET_FREQUENCY_KHZ A3 // 设置KHz的按钮引脚
#define SET_FREQUENCY_MHZ A6 // 设置MHz的按钮引脚
#define ENABLE_DISABLE_OUTPUT_PIN A7 // 设置On/Off的按钮引脚
#define FNC_PIN 4 // 设置AD9533的Fsync引脚
#define CLK_PIN 8 // 设置编码器的Clk引脚
#define DATA_PIN 7 // 设置编码器的Data引脚
#define BTN_PIN 9 // 设置编码器的Btn引脚
int counter = 1; // 用于记录编码器旋转的计数器
int clockPin; // Placeholder por pin status used by the rotary encoder
int clockPinState; // Placeholder por pin status used by the rotary encoder
unsigned long time = 0; // 用于消抖
unsigned long moduleFrequency; // 输出频率
unsigned long sweepStep = 1000; // 扫频步进
unsigned long sweepFrequencyH = 10000000; // 扫频范围H
unsigned long sweepFrequencyL = 1000; // 扫频范围L
unsigned long sweepCurrentFrequency; // 扫频频率
long debounce = 220; // 消抖延迟
bool btn_state = 0; // On/Off AD9833的按钮状态
bool sweep_state = 0; // 扫频模式按钮状态
bool set_frequency_khz = 1; // AD9833的默认频率
bool set_frequency_mhz;
String waveSelect = "SIN"; // AD9833的初始波形类型
int encoder_btn_count = 0; // used to check encoder button press
Adafruit_SSD1306 display(SCREEN_WIDATA_PINH, SCREEN_HEIGHT, &Wire, -1);
AD9833 gen(FNC_PIN);
void setup() {
  Serial.begin(9600);
  gen.Begin(); // This MUST be the first command after declaring the AD9833 object
  pinMode(CLK_PIN, INPUT);
  pinMode(DATA_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  clockPinState = digitalRead(CLK_PIN);
  pinMode(SET_FREQUENCY_KHZ, INPUT);
  pinMode(SET_FREQUENCY_MHZ, INPUT);
  pinMode(ENABLE_DISABLE_OUTPUT_PIN, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay(); // 清屏
  display.setTextSize(2); // 设置文本大小
  display.setTextColor(WHITE); // 设置屏幕的颜色
  display.setCursor(17, 0);  // 设置游标坐标
  display.println("Function"); // 显示
  display.setCursor(13, 20); // 设置游标坐标
  display.println("Generator"); // 显示
  display.setTextSize(2); // 设置文本大小
  display.setCursor(33, 40); // 设置游标坐标
  display.println("^ _ ^"); // 显示
  display.display(); // 更新屏幕
  delay(3000); // 延迟3秒
  update_display();
}
void loop()
{
  clockPin = digitalRead(CLK_PIN);
  if (clockPin != clockPinState  && clockPin == 1) {
    if (digitalRead(DATA_PIN) != clockPin) {
      counter --;
    }
    else {
      counter ++;// Encoder is rotating CW so increment
    }
    if (counter < 1 ) counter = 1;
    Serial.println(counter);
    update_display();
  }
  clockPinState = clockPin; // 记录上一个CLK_PIN的值
  //检测到低电平信号，则表示按钮按下
  if (digitalRead(BTN_PIN) == LOW && millis() - time > debounce) {
    encoder_btn_count++; // Increment the values
    if (encoder_btn_count > 2) // 如果encoder_btn_count值大于2，则将其置为0
    {
      encoder_btn_count = 0;
    }
    if (encoder_btn_count == 0) { // 如果encoder_btn_count值为0，则切换成SIN（正弦波）
      waveSelect = "SIN"; // 修改字符串
      update_display(); // 更新屏幕
    }
    if (encoder_btn_count == 1) { // 如果encoder_btn_count值为1，则切换成SQR（方波）
      waveSelect = "SQR"; // 修改字符串
      update_display(); // 更新屏幕
    }
    if (encoder_btn_count == 2) { // 如果encoder_btn_count值为2，则切换成SQR（三角波）
      waveSelect = "TRI";  // 修改字符串
      update_display();// 更新屏幕
    }
    time = millis(); // 更新时间
  }
  // Check buttton press action with analogread method
  // Put in a slight delay to help debounce the reading
  if (analogRead(SET_FREQUENCY_KHZ) < 30 && millis() - time > debounce) { // 单位更新为KHz
    set_frequency_khz = 1;
    set_frequency_mhz = 0;
    moduleFrequency = counter * 1000;
    update_display();// 更新屏幕
    time = millis();// 更新时间
  }
  if (analogRead(SET_FREQUENCY_MHZ) < 30 && millis() - time > debounce ) { // 单位更新为MHz
    set_frequency_khz = 0;
    set_frequency_mhz = 1;
    moduleFrequency = counter * 1000000;
    update_display();// 更新屏幕
    time = millis();// 更新时间
  }
  if (analogRead(ENABLE_DISABLE_OUTPUT_PIN) < 30 && millis() - time > debounce ) { // On/Off按钮
    btn_state = ! btn_state; // 反转按钮状态
    gen.EnableOutput(btn_state); // 通过按钮状态控制On/Off
    update_display();// 更新屏幕
    time = millis();// 更新时间
  }
  if (analogRead(INFO_DISPLAY) < 30 && millis() - time > debounce ) { // 显示学生信息按钮
    display.clearDisplay();
    display.setTextSize(2); // 设置文本大小
    display.setCursor(0, 0); // 设置游标坐标
    display.println("HHJ:20192333075"); // 显示
    display.setCursor(0, 32); // 设置游标坐标
    display.println("LYJ:20192333014"); // 显示
    display.display(); // 更新屏幕
    delay(5000); // 延迟5秒
    update_display();
  }
  if (analogRead(SET_SWEEP_FREQUENCY) < 30 && millis() - time > debounce ) { // 扫频按钮
    sweep_state = ! sweep_state;
    display.clearDisplay(); // 清屏
    display.setTextSize(2); // 设置文本大小
    display.setCursor(32, 0);  // 设置游标坐标
    display.println("Sweep"); // 显示
    display.setCursor(13, 20); // 设置游标坐标
    display.println("Frequency"); // 显示
    display.setTextSize(2); // 设置文本大小
    display.setCursor(40, 40); // 设置游标坐标
    display.println("Mode"); // 显示
    display.display(); // 更新屏幕
    delay(2000);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0); // 设置游标坐标
    display.println("Sweep Frequency"); //显示
    display.setTextSize(2);
    display.setCursor(0, 20);// 设置游标坐标
    display.println("SIN");
    display.setCursor(45, 20);
    display.println(sweepStep / 1000);
    display.setCursor(90, 20);
    display.println("Khz");
    display.setCursor(0, 45);
    display.print("Sweeping..");
    display.display();
    gen.EnableOutput(1);
    btn_state = 1;
    for(sweepCurrentFrequency = sweepFrequencyL; sweepCurrentFrequency != sweepFrequencyH; sweepCurrentFrequency = sweepCurrentFrequency + sweepStep) {
      gen.ApplySignal(SINE_WAVE, REG0, sweepCurrentFrequency); // 更新AD9833模块
      Serial.println(sweepCurrentFrequency);
      delay(10);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0); // 设置游标坐标
    display.println("Sweep Frequency"); //显示
    display.setTextSize(2);
    display.setCursor(0, 20);//设置游标坐标
    display.println("SIN");
    display.setCursor(45, 20);
    display.println(sweepStep / 1000);
    display.setCursor(90, 20);
    display.println("Khz");
    display.setCursor(0, 45);
    display.print("Sweep Done");
    display.display();
    gen.EnableOutput(0);
    btn_state = 0;
    delay(5000);
    update_display();    
  }
}
void update_display()
{
  display.clearDisplay(); // 清屏
  display.setTextSize(1); //设置文本大小
  display.setCursor(10, 0); // 设置游标坐标
  display.println("Function Generator"); //显示
  display.setTextSize(2);//设置文本大小
  display.setCursor(0, 20);//设置游标坐标
  if (set_frequency_khz == 1 && set_frequency_mhz == 0 ) { // 检测设置为KHz的按钮是否被按下
    moduleFrequency = counter * 1000;//更新设置频率为计数值*1000
  }
  if (set_frequency_khz == 0 && set_frequency_mhz == 1) { // 检测设置为KHz的按钮是否被按下
    moduleFrequency = counter * 1000000;//更新设置频率为计数值*1000000
    if (moduleFrequency > 12000000)
    {
      moduleFrequency = 12000000; // 频率最大值为12MHz
      counter = 12;
    }
  }
  if (waveSelect == "SIN") { // 当选择SIN（正弦波）
    display.println("SIN");
    gen.ApplySignal(SINE_WAVE, REG0, moduleFrequency); // 更新AD9833模块
    Serial.println(moduleFrequency);
  }
  if (waveSelect == "SQR") {// 当选择SQR（方波）
    display.println("SQR");
    gen.ApplySignal(SQUARE_WAVE, REG0, moduleFrequency); // 更新AD9833模块
    Serial.println(moduleFrequency);
  }
  if (waveSelect == "TRI" ) {// 当选择TRI（三角波）
    display.println("TRI");
    gen.ApplySignal(TRIANGLE_WAVE, REG0, moduleFrequency); // 更新AD9833模块
    Serial.println(moduleFrequency);
  }
  display.setCursor(45, 20);
  display.println(counter); // 显示计数值（即当前频率）
  if (set_frequency_khz == 1 && set_frequency_mhz == 0 ) {
    display.setCursor(90, 20);
    display.println("Khz");// 显示单位KHz
    display.display(); // 更新屏幕
  }
  if (set_frequency_khz == 0 && set_frequency_mhz == 1) {
    display.setCursor(90, 20);
    display.println("Mhz");// 显示单位MHz
    display.display(); // 更新屏幕
  }
  if (btn_state) {
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print("Output ON"); // 显示On
    display.display();
  }
  else {
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print("Output OFF"); // 显示Off
    display.display();
  }
}
