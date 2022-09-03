#include <AD9833.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#define SCREEN_WIDATA_PINH 128 // 设置OLED的像素宽度
#define SCREEN_HEIGHT 64 // 设置OLED的像素高度
#define SET_SWEEP_FREQUENCY A1 //按钮5：切换成扫频功能/退出扫频
#define INFO_DISPLAY A2 // 按钮2：设置显示学生信息
#define SET_FREQUENCY_KHZ A3 // 按钮3：设置KHz的按钮引脚/设置扫频范围L
#define SET_FREQUENCY_MHZ A6 // 按钮4：设置MHz的按钮引脚/设置扫频范围H
#define ENABLE_DISABLE_OUTPUT_PIN A7 // 按钮1：设置On/Off的按钮引脚/开始扫频
#define FNC_PIN 4 // 设置AD9533的Fsync引脚
#define CLK_PIN 8 // 设置编码器的Clk引脚
#define DATA_PIN 7 // 设置编码器的Data引脚
#define BTN_PIN 9 // 设置编码器的Btn引脚
int counter = 1; // 用于记录编码器旋转的计数器
int clockPin;
int clockPinState;
unsigned long time = 0; // 用于消抖
unsigned long moduleFrequency; // 输出频率
unsigned long sweepStep = 1000; // 扫频步进
unsigned long sweepFrequencyH = 10000000; // 扫频范围H
unsigned long sweepFrequencyL = 1000; // 扫频范围L
unsigned long sweepCurrentFrequency; // 扫频频率
long debounce = 220; // 消抖延迟
bool btn_state = 0; // On/Off AD9833的按钮状态
bool sweep_state = 0; // 扫频模式按钮状态
bool sweep_frequency_L = 1; // 扫频范围（低）
bool sweep_frequency_H = 0; // 扫频范围（高）
bool set_frequency_khz = 1; // AD9833的默认频率
bool set_frequency_mhz;
String waveSelect = "SIN"; // AD9833的初始波形类型
int encoder_btn_count = 0; // 按下旋钮计数
Adafruit_SSD1306 display(SCREEN_WIDATA_PINH, SCREEN_HEIGHT, &Wire, -1);
AD9833 gen(FNC_PIN);
void setup() {
  Serial.begin(9600);
  gen.Begin();
  pinMode(CLK_PIN, INPUT);
  pinMode(DATA_PIN, INPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  clockPinState = digitalRead(CLK_PIN);
  pinMode(INFO_DISPLAY, INPUT);
  pinMode(SET_FREQUENCY_KHZ, INPUT);
  pinMode(SET_FREQUENCY_MHZ, INPUT);
  pinMode(ENABLE_DISABLE_OUTPUT_PIN, INPUT);
  pinMode(SET_SWEEP_FREQUENCY, INPUT);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // OLED的地址
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay(); // 清屏
  display.setTextSize(2); // 设置文本大小
  display.setTextColor(WHITE); // 设置文本颜色
  display.setCursor(17, 0); // 设置游标坐标
  display.println("Function"); // 打印
  display.setCursor(13, 20); // 设置游标坐标
  display.println("Generator"); // 打印
  display.setTextSize(1); // 设置文本大小
  display.setCursor(27, 45); // 设置游标坐标
  display.println("Course Design"); // 打印
  display.display(); // 更新屏幕显示
  delay(3000);
  update_display();
}
void loop()
{
  clockPin = digitalRead(CLK_PIN);
  if (clockPin != clockPinState  && clockPin == 1) {
    if (digitalRead(DATA_PIN) != clockPin) {
      counter --; // 旋钮逆时针旋转
    }
    else {
      counter ++; // 旋钮顺时针旋转
    }
    if (counter < 1 ) counter = 1;
    Serial.println(counter);
    update_display();
  }
  clockPinState = clockPin; // 记录上一个CLK_PIN的值
  //检测到低电平信号，则表示按钮按下
  if (digitalRead(BTN_PIN) == LOW && millis() - time > debounce) {
    encoder_btn_count++;
    if (encoder_btn_count > 2) // 如果encoder_btn_count值大于2，则将其置为0
    {
      encoder_btn_count = 0;
    }
    if (encoder_btn_count == 0) { // 如果encoder_btn_count值为0，则切换成SIN（正弦波）
      waveSelect = "SIN";
      update_display();
    }
    if (encoder_btn_count == 1) { // 如果encoder_btn_count值为1，则切换成SQR（方波）
      waveSelect = "SQR";
      update_display();
    }
    if (encoder_btn_count == 2) { // 如果encoder_btn_count值为2，则切换成SQR（三角波）
      waveSelect = "TRI";
      update_display();
    }
    time = millis();
  }
  if (analogRead(SET_FREQUENCY_KHZ) < 30 && millis() - time > debounce) { // 单位更新为KHz
    set_frequency_khz = 1;
    set_frequency_mhz = 0;
    moduleFrequency = counter * 1000;
    update_display();
    time = millis();
  }
  if (analogRead(SET_FREQUENCY_MHZ) < 30 && millis() - time > debounce ) { // 单位更新为MHz
    set_frequency_khz = 0;
    set_frequency_mhz = 1;
    moduleFrequency = counter * 1000000;
    update_display();
    time = millis();
  }
  if (analogRead(ENABLE_DISABLE_OUTPUT_PIN) < 30 && millis() - time > debounce ) { // On/Off按钮
    btn_state = ! btn_state; // 反转按钮状态
    gen.EnableOutput(btn_state); // 通过按钮状态控制On/Off
    update_display();
    time = millis();
  }
  if (analogRead(INFO_DISPLAY) < 30 && millis() - time > debounce ) { // 按钮2：显示学生信息
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("HHJ:20192333075");
    display.setCursor(0, 32);
    display.println("LYJ:20192333014");
    display.display();
    delay(5000);
    update_display();
  }
  if (analogRead(SET_SWEEP_FREQUENCY) < 30 && millis() - time > debounce ) { // 按钮5：切换扫频功能
    sweep_state = ! sweep_state;
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(35, 0);
    display.println("Sweep");
    display.setCursor(13, 20);
    display.println("Frequency");
    display.setTextSize(2);
    display.setCursor(40, 40);
    display.println("Mode");
    display.display();
    delay(2000);
    update_display_sweepStep();
    for(;;) { // 设置扫频步进
      clockPin = digitalRead(CLK_PIN);
      if (clockPin != clockPinState  && clockPin == 1) {
        if (digitalRead(DATA_PIN) != clockPin) {
          sweepStep = (sweepStep - 1000);
        }
        else {
          sweepStep = (sweepStep + 1000);
        }
        if (sweepStep < 1000 ) sweepStep = 1000;
        Serial.println(sweepStep);
        update_display_sweepStep();
      }
      clockPinState = clockPin; // 记录上一个CLK_PIN的值
      if (analogRead(ENABLE_DISABLE_OUTPUT_PIN) < 30 && millis() - time > debounce ) { // 按钮1：进入下一步设置，设置扫频范围
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(18, 0);
        display.println("Sweep Frequency"); 
        display.setTextSize(2);
        display.setCursor(0,20);
        display.println("Set Step");
        display.setCursor(80,40);
        display.println("Done");
        display.display();
        delay(1000);
        break;
      }
    }
    update_display_sweepRange();
    for(;;) { // 设置扫频范围
      if (analogRead(SET_FREQUENCY_KHZ) < 30 && millis() - time > debounce) { // 按钮3：设置扫频范围（低）
        sweep_frequency_L = 1;
        sweep_frequency_H = 0;
        update_display_sweepRange();
      }
      if (analogRead(SET_FREQUENCY_MHZ) < 30 && millis() - time > debounce) { // 按钮4：设置扫频范围（高）
        sweep_frequency_L = 0;
        sweep_frequency_H = 1;
        update_display_sweepRange();   
      }
      if (sweep_frequency_L == 1 && sweep_frequency_H == 0) { // 设置扫频范围（低）
        clockPin = digitalRead(CLK_PIN);
        if (clockPin != clockPinState  && clockPin == 1) {
          if (digitalRead(DATA_PIN) != clockPin) {
            sweepFrequencyL = (sweepFrequencyL - 1000);
          }
          else {
            sweepFrequencyL = (sweepFrequencyL + 1000);
          }
          if (sweepFrequencyL < 1000 ) sweepFrequencyL = 1000;
          update_display_sweepRange();
        }
        clockPinState = clockPin; // 记录上一个CLK_PIN的值   
      }
      if (sweep_frequency_L == 0 && sweep_frequency_H == 1) { // 设置扫频范围（高）
        clockPin = digitalRead(CLK_PIN);
        if (clockPin != clockPinState  && clockPin == 1) {
          if (digitalRead(DATA_PIN) != clockPin) {
            sweepFrequencyH = (sweepFrequencyH - 1000000);
          }
          else {
            sweepFrequencyH = (sweepFrequencyH + 1000000);
          }
          if (sweepFrequencyH > 12000000 ) {
            sweepFrequencyH = 12000000;
          }
          else if (sweepFrequencyH < 1000000 ) {
            sweepFrequencyH = 1000000;
          }
          update_display_sweepRange();
        }
        clockPinState = clockPin; // 记录上一个CLK_PIN的值   
      }
      if (analogRead(ENABLE_DISABLE_OUTPUT_PIN) < 30 && millis() - time > debounce ) { // 按钮1：进入下一步，开始扫频
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(18, 0);
        display.println("Sweep Frequency"); 
        display.setTextSize(2);
        display.setCursor(0,20);
        display.println("Set Range");
        display.setCursor(80,40);
        display.println("Done");
        display.display();
        delay(1000);
        sweep_frequency_L = 1;
        sweep_frequency_H = 0;
        break;
      }    
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0);
    display.println("Sweep Frequency");
    display.setTextSize(2);
    display.setCursor(0, 20);
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
    for(sweepCurrentFrequency = sweepFrequencyL; sweepCurrentFrequency < sweepFrequencyH; sweepCurrentFrequency = sweepCurrentFrequency + sweepStep) {
      gen.ApplySignal(SINE_WAVE, REG0, sweepCurrentFrequency); // 更新AD9833模块
      Serial.println(sweepCurrentFrequency);
      if (analogRead(SET_SWEEP_FREQUENCY) < 30 && millis() - time > debounce ) { // 按钮5：退出扫频
        break;
      }
      delay(10);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0);
    display.println("Sweep Frequency");
    display.setTextSize(2);
    display.setCursor(0, 20);
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
    delay(3000);
    update_display();    
  }
}
// 更新屏幕：发生器
void update_display()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.println("Function Generator");
  display.setTextSize(2);
  display.setCursor(0, 20);
  if (set_frequency_khz == 1 && set_frequency_mhz == 0 ) {
    moduleFrequency = counter * 1000; //更新设置频率为计数值*1000
  }
  if (set_frequency_khz == 0 && set_frequency_mhz == 1) {
    moduleFrequency = counter * 1000000; //更新设置频率为计数值*1000000
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
  display.println(counter);
  if (set_frequency_khz == 1 && set_frequency_mhz == 0 ) {
    display.setCursor(90, 20);
    display.println("Khz");
    display.display();
  }
  if (set_frequency_khz == 0 && set_frequency_mhz == 1) {
    display.setCursor(90, 20);
    display.println("Mhz");
    display.display();
  }
  if (btn_state) {
    display.setTextSize(1);
    display.setCursor(65, 45);
    display.print("Output ON");
    display.display();
  }
  else {
    display.setTextSize(1);
    display.setCursor(65, 45);
    display.print("Output OFF");
    display.display();
  }
}
// 更新屏幕：扫频步进
void update_display_sweepStep()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(18, 0);
  display.println("Sweep Frequency");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("SIN");
  display.setCursor(45, 20);
  display.println(sweepStep / 1000);
  display.setCursor(90, 20);
  display.println("Khz");
  display.setTextSize(1);
  display.setCursor(65, 45);
  display.print("Set Step..");
  display.display();
}
// 更新屏幕：扫频范围
void update_display_sweepRange()
{
  if (sweep_frequency_L == 1 && sweep_frequency_H == 0) { // 设置扫频范围（低）
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0);
    display.println("Sweep Frequency");
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("SIN");
    display.setCursor(45, 20);
    display.println(sweepFrequencyL / 1000);
    display.setCursor(90, 20);
    display.println("Khz");
    display.setTextSize(2);
    display.setCursor(20,40);
    display.print("L");
    display.setTextSize(1);
    display.setCursor(60, 45);
    display.print("Set Range..");
    display.display();
  }
  if (sweep_frequency_L == 0 && sweep_frequency_H == 1) { // 设置扫频范围（高）
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(18, 0);
    display.println("Sweep Frequency");
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("SIN");
    display.setCursor(45, 20);
    display.println(sweepFrequencyH / 1000000);
    display.setCursor(90, 20);
    display.println("Mhz");
    display.setTextSize(2);
    display.setCursor(20,40);
    display.print("H");
    display.setTextSize(1);
    display.setCursor(60, 45);
    display.print("Set Range..");
    display.display();
  }
}
