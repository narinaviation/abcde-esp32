#include <EEPROM.h>
#include <BluetoothSerial.h>
#include <ABCDE.h>

#include "Device.h"

device_config_t device_config;
program_data_t  program_data;

ABCDESerial     ABCDE;
BluetoothSerial SerialBT;

/* ABCDE Execute functions */
void all_leds_on(char* reply_str, uint16_t* reply_len) {
   digitalWrite(DEVICE_GPIO_RED, HIGH);
   digitalWrite(DEVICE_GPIO_GRN, HIGH); 
   digitalWrite(DEVICE_GPIO_BLU, HIGH); 
   sprintf(reply_str, "LED:ON");
   *reply_len = strlen(reply_str);
}
void all_leds_off(char* reply_str, uint16_t* reply_len) {
   digitalWrite(DEVICE_GPIO_RED, LOW); 
   digitalWrite(DEVICE_GPIO_GRN, LOW); 
   digitalWrite(DEVICE_GPIO_BLU, LOW); 
   sprintf(reply_str, "LED:OFF");
   *reply_len = strlen(reply_str);
}
void led_only_red(char* reply_str, uint16_t* reply_len) {
   digitalWrite(DEVICE_GPIO_RED, HIGH);
   digitalWrite(DEVICE_GPIO_GRN, LOW);
   digitalWrite(DEVICE_GPIO_BLU, LOW); 
   sprintf(reply_str, "LED:RED");
   *reply_len = strlen(reply_str);
}
void led_only_grn(char* reply_str, uint16_t* reply_len) {
   digitalWrite(DEVICE_GPIO_RED, LOW);
   digitalWrite(DEVICE_GPIO_GRN, HIGH);
   digitalWrite(DEVICE_GPIO_BLU, LOW); 
   sprintf(reply_str, "LED:GRN");  
   *reply_len = strlen(reply_str);
}
void led_only_blue(char* reply_str, uint16_t* reply_len) {
   digitalWrite(DEVICE_GPIO_RED, LOW);
   digitalWrite(DEVICE_GPIO_GRN, LOW);
   digitalWrite(DEVICE_GPIO_BLU, HIGH); 
   sprintf(reply_str, "LED:BLU");  
   *reply_len = strlen(reply_str);
}

void abcde_data_request() {
  ESP_LOGW("APP","data request occured.");
  program_data.analog0_value = analogRead(DEVICE_GPIO_ADC0);
  program_data.analog0_value = analogRead(DEVICE_GPIO_ADC1);
  program_data.digital0_state = digitalRead(DEVICE_GPIO_BTN0);
  program_data.digital1_state = digitalRead(DEVICE_GPIO_BTN1);
}

void abcde_config_complete() {
   ESP_LOGW("APP","saving configurations.");
   device_save_config(EEPROM,&device_config);
}

void app_init_abcde(Stream& serial) {
   ABCDEItem ConfigItems[4];
   ConfigItems[0].set("Identifier" , ABCDE_TYPE_STR, 0, 32  , (void*)device_config.identifier);
   ConfigItems[1].set("Color RED"  , ABCDE_TYPE_INT, 0, 255 , (void*)&device_config.rgb_red);
   ConfigItems[2].set("Color GREEN", ABCDE_TYPE_INT, 0, 255 , (void*)&device_config.rgb_green);
   ConfigItems[3].set("Color BLUE" , ABCDE_TYPE_INT, 0, 255 , (void*)&device_config.rgb_blue);
   ABCDEItem DisplayItems[5];
   DisplayItems[0].set("Identifier" , ABCDE_TYPE_STR, 0, 32, (void*)device_config.identifier);
   DisplayItems[1].set("Analog0"    , ABCDE_TYPE_INT, 0, 4095 , (void*)&program_data.analog0_value);
   DisplayItems[2].set("Analog1"    , ABCDE_TYPE_INT, 0, 4095 , (void*)&program_data.analog1_value);
   DisplayItems[3].set("Digital0"   , ABCDE_TYPE_INT, 0, 1 , (void*)&program_data.digital0_state);
   DisplayItems[4].set("Digital1"   , ABCDE_TYPE_INT, 0, 1 , (void*)&program_data.digital1_state);
   ABCDEItem ExecuteItems[5];
   ExecuteItems[0].set("ALL ON"    , ABCDE_TYPE_FUN, 0, 255 , (void*)&all_leds_on);
   ExecuteItems[1].set("ALL OFF"   , ABCDE_TYPE_FUN, 0, 255 , (void*)&all_leds_off);
   ExecuteItems[2].set("ONLY RED"  , ABCDE_TYPE_FUN, 0, 255 , (void*)&led_only_red);
   ExecuteItems[3].set("ONLY GREEN", ABCDE_TYPE_FUN, 0, 255 , (void*)&led_only_grn);
   ExecuteItems[4].set("ONLY BLUE" , ABCDE_TYPE_FUN, 0, 255 , (void*)&led_only_blue);
   ABCDE.setConfigItems(ConfigItems,sizeof(ConfigItems)/sizeof(ABCDEItem));
   ABCDE.setDisplayItems(DisplayItems,sizeof(DisplayItems)/sizeof(ABCDEItem));
   ABCDE.setExecuteItems(ExecuteItems,sizeof(ExecuteItems)/sizeof(ABCDEItem));
   ABCDE.onDataRequest(&abcde_data_request);
   ABCDE.onConfigComplete(&abcde_config_complete);
   ABCDE.init(serial);
}

void setup() {
   Serial.begin(115200);
   device_init_gpios();
   device_load_config(EEPROM,&device_config);
   SerialBT.begin(device_config.identifier);
   app_init_abcde(SerialBT);
}

void loop() {
   ABCDE.loop();
}
