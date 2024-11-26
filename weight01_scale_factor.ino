#include "HX711.h"

// HX711接线设定
const int DT_PIN = A1;
const int SCK_PIN = A0;

const int sample_weight = 171; // 用来校正的质量（公克）

HX711 scale;

void setup() {
  Serial.begin(9600);
  scale.begin(DT_PIN, SCK_PIN);
  scale.set_scale(); // 开始取得比例参数
  scale.tare();
  Serial.println("Nothing on it.");
  Serial.println(scale.get_units(10));
  Serial.println("Please put sample..."); // 提示放上基准物品
}

void loop() {
  float current_weight = scale.get_units(10); // 取得10次数值的平均
  float scale_factor = (current_weight / sample_weight);
  Serial.print("Scale number: ");
  Serial.println(scale_factor, 0); // 显示比例参数，记起来，以便用在正式的程式中
}
