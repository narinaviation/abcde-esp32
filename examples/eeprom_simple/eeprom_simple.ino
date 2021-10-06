#include <ABCDE.h>
#include <EEPROM.h>
#include <BluetoothSerial.h>

ABCDESerial     ABCDE;
BluetoothSerial SerialBT;

#define RFS_PIN     13    // INPUT : Restore Factory Settings (RFS) Jumper.
#define DIO_PIN     12    // INPUT : Button (Digital)
#define ADC_PIN     36    // INPUT : Float (Analog)

#define LED_PIN     2     // OUTPUT : Module LED.
#define RELAY_PIN   4     // OUTPUT : Board Relay.

/* display variables */
int gpio_state = 0;
int adc_reading = 0;
int relay_status = 0;
int blink_status = 0;

/* config variables */
typedef struct {
  char  ident[32];
  int   start_relay;
  int   start_blink;
  int   blink_ms;
} config_t;

config_t cfg;

/* other global variables */
static int blink_state = 0;

/* execute functions */
void relay_on(char* reply_str, uint16_t* reply_len) {
  relay_status = 1;
  sprintf(reply_str, "RELAY:ON");
  *reply_len = strlen(reply_str);
}
void relay_off(char* reply_str, uint16_t* reply_len) {
  relay_status = 0;
  sprintf(reply_str, "RELAY:OFF");
  *reply_len = strlen(reply_str);
}
void blink_start(char* reply_str, uint16_t* reply_len) {
  blink_status = 1;
  sprintf(reply_str, "RELAY:ON");
  *reply_len = strlen(reply_str);
}
void blink_stop(char* reply_str, uint16_t* reply_len) {
  blink_status = 0;
  sprintf(reply_str, "RELAY:OFF");
  *reply_len = strlen(reply_str);
}

/* callback functions */
void get_data() {
  adc_reading = analogRead(ADC_PIN);
  gpio_state = digitalRead(DIO_PIN);
}
void save_config() {
  for (int i = 0; i < sizeof(config_t); i++) {
    EEPROM.write(i, *((uint8_t*)&cfg + i));
  }
  EEPROM.commit();
  delay(500);
  ESP.restart();
}

/* initialization function */
void load_config() {
  pinMode(RFS_PIN, INPUT_PULLUP);
  if (EEPROM.begin(sizeof(config_t))) {
    if (digitalRead(RFS_PIN) == LOW) {
      // Restore defaults
      strcpy(cfg.ident, "ANUNDA_ABCDE");
      cfg.start_relay = 0;
      cfg.start_blink = 1;
      cfg.blink_ms   = 500;
      for (int i = 0; i < sizeof(config_t); i++) {
        EEPROM.write(i, *((uint8_t*)&cfg + i));
      }
      EEPROM.commit();
      Serial.println("FACTORY config restored.");
    }
    for (int i = 0; i < sizeof(config_t); i++) {
      *((uint8_t*)&cfg + i) = EEPROM.read(i);
    }
    Serial.println("CONFIG loaded successfully");
  } else {
    Serial.println("EEPROM init failed.");
  }
}

void setup() {
  Serial.begin(115200);
  load_config();
  pinMode(DIO_PIN, INPUT_PULLUP);
  pinMode(ADC_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  blink_status = cfg.start_blink;
  relay_status = cfg.start_relay;

  ABCDEItem ConfigItems[4];
  ConfigItems[0].set("Identifier"   , ABCDE_TYPE_STR, 0, 31 , (void*)&cfg.ident[0]);
  ConfigItems[1].set("Start Relay"  , ABCDE_TYPE_INT, 0, 1  , (void*)&cfg.start_relay);
  ConfigItems[2].set("Start Blink"  , ABCDE_TYPE_INT, 0, 1  , (void*)&cfg.start_blink);
  ConfigItems[3].set("Blink (ms)"   , ABCDE_TYPE_INT, 0, 255, (void*)&cfg.blink_ms);
  ABCDEItem DisplayItems[4];
  DisplayItems[0].set("Button State", ABCDE_TYPE_INT, 0, 1    , (void*)&gpio_state);
  DisplayItems[1].set("ADC Reading" , ABCDE_TYPE_INT, 0, 4095 , (void*)&adc_reading);
  DisplayItems[2].set("Relay Status", ABCDE_TYPE_INT, 0, 1    , (void*)&relay_status);
  DisplayItems[3].set("Blink Status", ABCDE_TYPE_INT, 0, 1    , (void*)&blink_status);
  ABCDEItem ExecuteItems[4];
  ExecuteItems[0].set("Relay ON"    , ABCDE_TYPE_FUN, 0, 255 , (void*)&relay_on);
  ExecuteItems[1].set("Relay OFF"   , ABCDE_TYPE_FUN, 0, 255 , (void*)&relay_off);
  ExecuteItems[2].set("Blink START" , ABCDE_TYPE_FUN, 0, 255 , (void*)&blink_start);
  ExecuteItems[3].set("Blink STOP"  , ABCDE_TYPE_FUN, 0, 255 , (void*)&blink_stop);
  
  ABCDE.setConfigItems(ConfigItems, sizeof(ConfigItems) / sizeof(ABCDEItem));
  ABCDE.setDisplayItems(DisplayItems, sizeof(DisplayItems) / sizeof(ABCDEItem));
  ABCDE.setExecuteItems(ExecuteItems, sizeof(ExecuteItems) / sizeof(ABCDEItem));

  ABCDE.onDataRequest(&get_data);
  ABCDE.onConfigComplete(&save_config);

  SerialBT.begin(cfg.ident);
  ABCDE.init(SerialBT);
}

void loop() {
  ABCDE.loop();
  if (blink_status == 1) {
    for (static uint32_t lastMillis;
    (millis() - lastMillis) > cfg.blink_ms;
    lastMillis += cfg.blink_ms) {
      blink_state = !blink_state;
      digitalWrite(LED_PIN, blink_state);
    }
  } else {
    digitalWrite(LED_PIN,LOW);
  }
  if (relay_status == 1) {
    digitalWrite(RELAY_PIN,HIGH);
  } else {
    digitalWrite(RELAY_PIN,LOW);
  }
}
