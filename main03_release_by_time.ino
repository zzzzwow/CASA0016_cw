#include <DHT.h>                
#include <Wire.h>               
#include <LiquidCrystal_I2C.h>  
#include <Servo.h>              
#include "HX711.h"

#define DHTTYPE DHT22
const int scale_factor = 427;
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
const char Key_Map[KEYPAD_ROWS][KEYPAD_COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

const int DHT_Pin = 2;
const int SCK_Pin = A0;
const int DT_Pin = A1;
const int Row_Pins[KEYPAD_ROWS] = { 12, 11, 10, 9 };
const int Col_Pins[KEYPAD_COLS] = { 8, 7, 6, 5 };
const int Servo_Pin = 3;

float Humidity, Temperature_C;
float HX711_Buffer = 0;
float Weight_Shiwu = 0;
long Weight;

bool Alarm_FG = true;
bool isReleasing = false;
unsigned long lastWeightCheckTime = 0;
int hoursWithoutFood = 0;

DHT dht(DHT_Pin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo my_Servo;
HX711 scale;
String inputPassword = "";

void setup() {
  Serial.begin(9600);
  for (int row = 0; row < KEYPAD_ROWS; row++) {
    pinMode(Row_Pins[row], INPUT);
    digitalWrite(Row_Pins[row], HIGH);
  }
  for (int col = 0; col < KEYPAD_COLS; col++) {
    pinMode(Col_Pins[col], OUTPUT);
    digitalWrite(Col_Pins[col], HIGH);
  }
  lcd.init();
  lcd.backlight();
  init_DHT(1);
  scale.begin(DT_Pin, SCK_Pin);
  scale.set_scale(scale_factor);
  scale.tare();
  my_Servo.attach(Servo_Pin, 500, 2500);
  my_Servo.write(0);
}

void loop() {
  char key = getkey();
  if (key != 0) {
    Serial.println(key);
    if (key == '*' && !isReleasing) {
      handleInput();
    } else if (key == '#' && !isReleasing) {
      if (inputPassword.length() > 0) {
        inputPassword.remove(inputPassword.length() - 1);
      }
      displayInputPassword();
    } else if (key == 'A') {
      startReleasing();
    } else if (key == 'B' && isReleasing) {
      stopReleasing();
    } else {
      inputPassword += key;
      displayInputPassword();
    }
  }
  updateSensors();
  refresh_LCD(2000);
}

void handleInput() {
  int num = inputPassword.toInt();
  openServo(num);  // 调用openServo函数，并传递num作为参数
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Released");
  delay(1000);
  inputPassword = "";
}

void displayInputPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(inputPassword);
}

void startReleasing() {
  isReleasing = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("A: releasing");
  my_Servo.write(40); // Open the servo
}

void stopReleasing() {
  my_Servo.write(0); // Close the servo
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("B: released");
  delay(2000); // Display for 2 seconds
  isReleasing = false;
  lcd.clear();
  refresh_LCD(0); // Immediately refresh the LCD to show sensor data
}

void updateSensors() {
  Weight = scale.get_units(10);
  if (Weight < 0) {
    Weight = 0;
  }
  Temperature_C = dht.readTemperature();
  Humidity = dht.readHumidity();
  if (Weight == 0) {
    unsigned long currentTime = millis();
    if (currentTime - lastWeightCheckTime >= 600000) { // 1 hour in milliseconds
      lastWeightCheckTime = currentTime;
      hoursWithoutFood++;
      if (hoursWithoutFood > 24) {
        hoursWithoutFood = 0; // Reset after 24 hours
      }
    }
  } else {
    hoursWithoutFood = 0; // Reset if there is weight detected
    lastWeightCheckTime = millis();
  }
}

void refresh_LCD(int refreshTime) {
  static unsigned long LCD_Clear_Time = 0;
  if (millis() - LCD_Clear_Time > refreshTime) {
    LCD_Clear_Time = millis();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(roundf(Temperature_C * 10) / 10); 
    lcd.print("C  H:");
    lcd.print(roundf(Humidity * 10) / 10); 
    lcd.print("%");

    if (hoursWithoutFood > 0 && hoursWithoutFood <= 24) {
      lcd.setCursor(0, 1);
      lcd.print(hoursWithoutFood);
      lcd.print("h without food!");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Weight:");
      lcd.print(Weight);
      lcd.print("g");
    }
  }
}

void init_DHT(bool DF) {
  dht.begin();
  if (isnan(dht.readHumidity()) || isnan(dht.readTemperature()) || isnan(dht.readTemperature(true))) {
    Serial.print("DHT Failed!");
    if (DF) {
      while (1);
    } else {
      delay(1000);
    }
  } else {
    Serial.print("DHT OK!");
    delay(1000);
  }
}

void openServo(float seconds) {
  my_Servo.write(40); // Open the servo
  delay(seconds * 1000);
  my_Servo.write(0); // Close the servo
}

char getkey() {
  static unsigned long lastKeyTime = 0; 
  for (int col = 0; col < KEYPAD_COLS; col++) {
    digitalWrite(Col_Pins[col], LOW);
    for (int row = 0; row < KEYPAD_ROWS; row++) {
      if (digitalRead(Row_Pins[row]) == LOW) {
        unsigned long currentTime = millis();
        if (currentTime - lastKeyTime > 50) { 
          lastKeyTime = currentTime;
          while (digitalRead(Row_Pins[row]) == LOW); 
          digitalWrite(Col_Pins[col], HIGH);
          return Key_Map[row][col];
        }
      }
    }
    digitalWrite(Col_Pins[col], HIGH);
  }
  return 0;
}