#include <WiFi.h>
#include <WiFiUdp.h>
#include "driver/adc.h" 
#include "esp_adc_cal.h"

const int pin1 = A4;
const int pin2 = A5;
const int ledpin1 = 12;
const int ledpin2 = 14;
#define LEDC_BASE_FREQ 33000.0
#define LEDC_TIMER_BIT 11

unsigned long prev;
int pin5;
int pin18;
int pin23;

const char ssid[] = "ESP32_power"; // SSID should be change
const char pass[] = "passpower";  // password should be change
const int localPort = 10000;      // portnum

const IPAddress ip(192, 168, 4, 1);       // IP address
const IPAddress subnet(255, 255, 255, 0); // subnet mask

WiFiUDP udp;

esp_adc_cal_characteristics_t adcChar;
uint32_t adjzero; //throttle midpoint

void setup() {
  Serial.begin(115200);

  // timer channel and frequency
  ledcSetup(0, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  ledcSetup(1, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  // 
  ledcAttachPin(pin1, 0);
  // 
  ledcAttachPin(pin2, 1);

  pinMode(5, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
  pinMode(19, OUTPUT);
  pinMode(22, OUTPUT);

  WiFi.softAP(ssid, pass);           // Set SSID and Password
  delay(100);                        // Do not remove this delay, if remove sometimes softAPConfig fail.
  WiFi.softAPConfig(ip, ip, subnet); // IP Adress etc.

  Serial.print("AP IP address: ");
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  Serial.println("Starting UDP");
  udp.begin(localPort);  // Start UDP communication

  Serial.print("Local port: ");
  Serial.println(localPort);
  // Initialize ADC1_CH6
  adc_gpio_init(ADC_UNIT_1, ADC_CHANNEL_6);
  // Set ADC1 resolution 12bit（0~4095）
  adc1_config_width(ADC_WIDTH_BIT_12);
  // Set ADC1 attenuation 11dB
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  // Set ADC info to addChar
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adcChar);
  // Set current Vol position as virtual midpoint
  esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adcChar, &adjzero);
}

void loop() {
  int j;
  int th1;
  int th2;
  unsigned long curr;
  uint8_t cstr[6];
  uint32_t voltage;
  
  j = udp.parsePacket();

  //Serial.println("j=" + String(j)); // display coming value for debug
  if (j > 0) {
    for(int i=0; i < 6; i++){
      cstr[i] = udp.read();
    }
    udp.flush();
    if (cstr[0] == 0x41){ //th1 "A"
      th1 = (cstr[2] - 0x30) * 1000;
      th1 = th1 + (cstr[3] - 0x30) * 100;
      th1 = th1 + (cstr[4] - 0x30) * 10;
      th1 = th1 + (cstr[5] - 0x30);
      if (cstr[1] == 0x2D){ // minus
        th1 = th1 * -1;
      }
      if (pin23 == LOW){
        nettoth(th1);
      } else {
        nettotha(th1);
      }
      //Serial.println("th1=" + String(th1)); // display coming value for debug
      prev = millis();
    }
  } else {
    curr = millis() - prev;
      //Serial.println("mls=" + String(curr)); // millis() - prev
    if (curr > 1000){
      if (th1 >= 10){
        th1 = th1 - 10;
      } else if (th1 <= -10) {
        th1 = th1 + 10;
      } else {
        th1 = 0;
      }
      if (pin23 == LOW){
        nettoth(th1);
      } else {
        nettotha(th1);
      }
    }
  }

  pin5 = digitalRead(5);
  pin18 = digitalRead(18);
  pin23 = digitalRead(23);
      //Serial.println("18:" + String(pin18) + " 23:" + String(pin23)); // millis() - prev

  while(digitalRead(5) == LOW){
    // Get voltage ADC1_CH6
    esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adcChar, &voltage);

    th1 = voltoth(voltage);
    if (digitalRead(23) == LOW){
      nettoth(th1);
    } else {
      nettotha(th1);
    }
    delay(50);
    udp.flush();
  }
}

void nettoth(int ret){
  if (ret < 0) {
    if (pin18 == LOW){
      ledcWrite(0, 2047 + ret);
      ledcWrite(1, 2047);
    } else {
      ledcWrite(0, 0);
      ledcWrite(1, ret * -1);
    }
    digitalWrite(19, HIGH);
    digitalWrite(22, LOW);
  } else if (ret > 0){
    if (pin18 == LOW){
      ledcWrite(0, 2047);
      ledcWrite(1, 2047 - ret);
    } else {
      ledcWrite(0, ret);
      ledcWrite(1, 0);
    }
    digitalWrite(19, LOW);
    digitalWrite(22, HIGH);
  } else {
    ledcWrite(0, 2047);
    ledcWrite(1, 2047);
    digitalWrite(19, LOW);
    digitalWrite(22, LOW);
  }
}

void nettotha(int ret){
  int ret2;

  ret2 = abs(ret);
  ret2 = ret2 * ret2 / 2047;
//  ret2 = map2(ret2, 0, 800, 0, 200);
//  ret2 = map2(ret2, 801, 1400, 200, 800);
//  ret2 = map2(ret2, 1401, 2047, 800, 2047);

  if (ret < 0){
    ret = ret2 * -1;
  } else {
    ret = ret2;
  }

  nettoth(ret);
}

int voltoth(uint32_t vol){
  long ret;
  long ret2;
  int refvol;
  //refvol = (int)adjzero;

  ret = map2(vol, 142, adjzero - 16, -2047, - 1);
  ret = map2(ret, adjzero -15, adjzero + 15, 0, 0);
  ret = map2(ret, adjzero + 16, 3150, 1, 2047);
 
  return ret;
}

long map2(long x, long in_min, long in_max, long out_min, long out_max) {
  if ((x >= in_min) && (x<= in_max)){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return x;
  }
}
