#include <WiFi.h>
#include <WiFiUdp.h>
#include "driver/adc.h" 
#include "esp_adc_cal.h"

const int pin1 = A4;
const int pin2 = A5;

const char ssid[] = "ESP32_power"; // SSID should be change(same at wpower_p.ino)
const char pass[] = "passpower"";  // password should be change(same at wpower_p.ino)

static WiFiUDP wifiUdp; 
static const char *kRemoteIpadr = "192.168.4.1";  // destination IP address
static const int kRmoteUdpPort = 10000; // destination port
esp_adc_cal_characteristics_t adcChar;
uint32_t adjzero; //midpoint

static void WiFi_setup()
{
  static const int kLocalPort = 5000;  // port itself

  WiFi.begin(ssid, pass);
//  WiFi.setTxPower(WIFI_POWER_11dBm);
  while( WiFi.status() != WL_CONNECTED) {
    delay(500);  
  }  
  wifiUdp.begin(kLocalPort);
  Serial.println("Wifi begin");   
}

static void Serial_setup()
{
  Serial.begin(115200);
  Serial.println("Serial begin");   
}

void setup() {
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial_setup();
  WiFi_setup();
  // Initialize ADC1_CH6
  adc_gpio_init(ADC_UNIT_1, ADC_CHANNEL_6);
  // Set ADC1 resolution 12bit（0~4095）
  adc1_config_width(ADC_WIDTH_BIT_12);
  // Set ADC1_CH6 attenuation 11dB
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  // Set ADC info to addChar
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adcChar);

  // Initialize ADC1_CH4
  adc_gpio_init(ADC_UNIT_1, ADC_CHANNEL_4);
  // Set ADC1_CH4 attenuation 11dB
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);

  // Set current Vol position as virtual midpoint
  esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adcChar, &adjzero);
}

void loop() 
{
  uint32_t voltage;
  uint32_t voltage2;
  int realth;
  uint8_t cstr[6];
  
  // Get voltage ADC1_CH6
  esp_adc_cal_get_voltage(ADC_CHANNEL_6, &adcChar, &voltage);
  // Get voltage ADC1_CH4
  esp_adc_cal_get_voltage(ADC_CHANNEL_4, &adcChar, &voltage2);
//    Serial.println("Voltage: " + String(voltage) + "[mV]");

  realth = voltoth(voltage, voltage2);
  cstr[0] = 0x41;
  if (realth >= 0){
    cstr[1] = 0x2B; //plus
  } else {
    cstr[1] = 0x2D; //minus
    realth = realth * -1;
  }
  cstr[2] = (realth / 1000) + 0x30;
  cstr[3] = ((realth % 1000) / 100) + 0x30;
  cstr[4] = ((realth % 100) / 10) + 0x30;
  cstr[5] = (realth % 10) + 0x30;
    Serial.println("Throtl: " + String(realth) + "%%");
  wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
//  wifiUdp.write('A');  //th1
//  std::string str = cstr;
//  uint8_t arr[sizeof(cstr)];
//  std::copy(str.begin(), str.end(), std::begin(arr));

  wifiUdp.write(cstr, sizeof(cstr));
  wifiUdp.endPacket();  

  delay(50);
}

int voltoth(uint32_t vol, uint32_t vol2){
  long ret;
  long ret2;
  int refvol;
  //refvol = (int)adjzero;

  // adjust minimum power
  ret2 = map2(vol2, 0, 309, 0, 0);
  ret2 = map2(ret2, 310, 3080, 0, 1000);

  // calcurate throttle from vol 
  ret = map2(vol, 0, 249, -2047, -2047);
  ret = map2(ret, 250, adjzero - 201, -2047, ret2 * -1 - 1);
  ret = map2(ret, adjzero - 200, adjzero - 16, ret2 * -1, ret2 * -1);
  ret = map2(ret, adjzero - 15, adjzero + 15, 0, 0);
  ret = map2(ret, adjzero + 16, adjzero + 200, ret2, ret2);
  ret = map2(ret, adjzero + 201, 3080, ret2 + 1, 2047);
  ret = map2(ret, 3081, 3300, 2047, 2047);
//    Serial.println("ret4: " + String(ret) + "%");

/*  if (ret < 0) {
    //ret = (ret * (2047 - ret2) / 2047) - ret2;
    ledcWrite(0, 2047 - (ret * -1));
    ledcWrite(1, 2047);
  } else if (ret > 0){
    //ret = (ret * (2047 - ret2) / 2047) + ret2;
    ledcWrite(0, 2047);
    ledcWrite(1, 2047 - ret);
  } else {
    ledcWrite(0, 2047);
    ledcWrite(1, 2047);
  }*/

  return ret;
}

long map2(long x, long in_min, long in_max, long out_min, long out_max) {
  if ((x >= in_min) && (x<= in_max)){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return x;
  }
}
