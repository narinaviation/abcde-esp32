#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <Arduino.h>

#define DEVICE_VENDOR_PREFIX "ANUNDA"

#ifndef DEVICE_VENDOR_PREFIX
#define DEVICE_VENDOR_PREFIX "ESP32"
#endif

#define DEVICE_GPIO_RED 2
#define DEVICE_GPIO_GRN 4
#define DEVICE_GPIO_BLU 5


#define DEVICE_GPIO_ADC0 12
#define DEVICE_GPIO_ADC1 13
#define DEVICE_GPIO_BTN0 25
#define DEVICE_GPIO_BTN1 26

#define DEVICE_GPIO_FRST 21

#define TAG "DEVICE"

typedef struct {
  char  identifier[32];
  int  rgb_red;
  int  rgb_green;
  int  rgb_blue;
} __attribute__ ((packed)) device_config_t;

typedef struct {
  int analog0_value;
  int analog1_value;
  int digital0_state;
  int digital1_state;
} __attribute__ ((packed)) program_data_t;

void device_init_gpios() {
  pinMode(DEVICE_GPIO_FRST, INPUT_PULLUP);
  pinMode(DEVICE_GPIO_BTN0, INPUT_PULLUP);
  pinMode(DEVICE_GPIO_BTN1, INPUT_PULLUP);
  pinMode(DEVICE_GPIO_RED, OUTPUT);
  pinMode(DEVICE_GPIO_GRN, OUTPUT);
  pinMode(DEVICE_GPIO_BLU, OUTPUT);
}

/**
   @brief Implements the factory reset function.
          For example, by checking one of a GPIO pin state of a button.
   @return true When the reset condition is satisfied.
   @return false When in normal operation. (the reset operation is skipped).
*/
bool device_factory_reset() {
  return !digitalRead(DEVICE_GPIO_FRST);
}

/**
   @brief Gets the "Device ID" from MAC address in eFuse in the format "PREFIX_0123456789AB".

   @param prefix A string literal containing prefix for the device identifier.
   @param dest_str Pointer to a string buffer where the "Device ID" will be stored.
*/
void device_get_id(const char* prefix, char* dest_str) {
  // The eFuse MAC address will be in little-endian,i reverse it into the big-endian.
  uint64_t chip_id_little_endian = ESP.getEfuseMac();
  uint8_t* chip_id_little_endian_ptr = (uint8_t*)&chip_id_little_endian;
  uint64_t chip_id_big_endian = 0x00;
  uint8_t* chip_id_big_endian_ptr = (uint8_t*)&chip_id_big_endian;

  for (uint8_t byte_index = 0; byte_index < 8; byte_index++)
    *(chip_id_big_endian_ptr + byte_index) = *(chip_id_little_endian_ptr + (7 - byte_index));
  chip_id_big_endian >>= 8;

  sprintf(dest_str, "%s_%06X", prefix , (uint32_t)((chip_id_big_endian >> 8) & 0x00FFFFFF));
  //sprintf(dest_str, "%s_%06X%06X",prefix ,(uint32_t)(chip_id_big_endian >> 32), (uint32_t)((chip_id_big_endian >> 8) & 0x00FFFFFF));
}

/**
   @brief Implements the process of loading the start-up configuration.
          For normal operation, The device loads current configuration from the memory.
          And for the factory reset operation, the device overwrites the configuration
          to default values and loads it to device configuration global variables.
   @param storage_handle Handle of the memory driver object.
   @return esp_err_t ESP_OK When success, ESP_FAIL When the memory read/write failed.
*/
esp_err_t device_load_config(EEPROMClass &mem_handle, device_config_t* device_config_ptr) {
  if (mem_handle.begin(sizeof(device_config_t)) == false) {
    ESP_LOGE(TAG, "(checked) failure initializing EEPROM!");
    return ESP_FAIL;
  }
  if (device_factory_reset() == true) {
    ESP_LOGW(TAG, "factory reset condition satisfied, restoring defaults...");
    // restore the factory settings, which is hard-coded in the firmware source below.
    device_get_id(DEVICE_VENDOR_PREFIX, device_config_ptr->identifier);
    device_config_ptr->rgb_red   = 127;
    device_config_ptr->rgb_green = 127;
    device_config_ptr->rgb_blue  = 127;
    // write the device configuration structure to the memory starting from the EEPROM address 0x00.
    for (int i = 0; i < sizeof(device_config_t); i++) {
      mem_handle.write(i, *((uint8_t*)device_config_ptr + i));
    }
    mem_handle.commit();
    ESP_LOGW(TAG, "default factory configurations restored and written to EEPROM.");
  }
  for (int i = 0; i < sizeof(device_config_t); i++) {
    *((uint8_t*)device_config_ptr + i) = mem_handle.read(i);
  }
  ESP_LOGE(TAG, "configurations loaded successfully");
}

/**
   @brief Implements the process of loading the start-up configuration.
          For normal operation, The device loads current configuration from the memory.
          And for the factory reset operation, the device overwrites the configuration
          to default values and loads it to device configuration global variables.
   @param storage_handle Handle of the memory driver object.
   @return esp_err_t ESP_OK When success, ESP_FAIL When the memory read/write failed.
*/
void device_save_config(EEPROMClass &mem_handle, device_config_t* device_config_ptr) {
  if(device_config_ptr != NULL) {
    for (int i = 0; i < sizeof(device_config_t); i++) {
      mem_handle.write(i, *((uint8_t*)device_config_ptr + i));
    }
    mem_handle.commit();
    ESP_LOGW(TAG, "new configurations are written to EEPROM.");
  } else {
    ESP_LOGE(TAG, "The device config pointer is NULL");
  }
}
#endif // __DEVICE_H__
