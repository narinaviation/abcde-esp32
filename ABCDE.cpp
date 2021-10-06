#include "ABCDE.h"
#include "./src/jsmn.h"

#define TAG "ABCDE"

/**
 * @brief 
 * 
 * @param name 
 * @param type 
 * @param min 
 * @param max 
 * @param vptr 
 * @return true 
 * @return false 
 */

bool ABCDEItem::set(const char* name, abcde_type_t type, int min, int max, void* vptr) {
   strcpy(this->name,name);
   this->type = type;
   this->min = min;
   this->max = max;
   this->vptr = vptr;
}

bool ABCDEItem::getName(char* dest) {
   if(dest==NULL) {
      return false;
   }
   strcpy(dest,this->name);
   return true;
}
int ABCDEItem::getType() {
   return this->type;
}
int ABCDEItem::getMin() {
   return this->min;
}
int ABCDEItem::getMax() {
   return this->max;
}
void* ABCDEItem:: getValuePointer(){
   return this->vptr;
};


ABCDESerial::ABCDESerial() {
   _config_items        = NULL;
   _display_items       = NULL;
   _execute_items       = NULL;
   _on_data_request     = NULL;
   _on_config_complete  = NULL;
}

ABCDESerial::~ABCDESerial() {
}

void ABCDESerial::setConfigItems(ABCDEItem items[], int items_num) {
   _config_items_num = items_num;
   _config_items = (abcde_item_t*)malloc(items_num*sizeof(abcde_item_t));
   for(int i = 0; i < items_num; i++) {
      items[i].getName(_config_items[i].name);
      _config_items[i].type = items[i].getType();
      _config_items[i].min  = items[i].getMin();
      _config_items[i].max  = items[i].getMax();
      _config_items[i].vptr = items[i].getValuePointer();  
   }
}

void ABCDESerial::setDisplayItems(ABCDEItem items[], int items_num) {
   _display_items_num = items_num;
   _display_items = (abcde_item_t*)malloc(items_num*sizeof(abcde_item_t));
   for(int i = 0; i < items_num; i++) {
      items[i].getName(_display_items[i].name);
      _display_items[i].type = items[i].getType();
      _display_items[i].min  = items[i].getMin();
      _display_items[i].max  = items[i].getMax();
      _display_items[i].vptr = items[i].getValuePointer();  
   }
}

void ABCDESerial::setExecuteItems(ABCDEItem items[], int items_num) {
   _execute_items_num = items_num;
   _execute_items = (abcde_item_t*)malloc(items_num*sizeof(abcde_item_t));
   for(int i = 0; i < items_num; i++) {
      items[i].getName(_execute_items[i].name);
      _execute_items[i].type = items[i].getType();
      _execute_items[i].min  = items[i].getMin();
      _execute_items[i].max  = items[i].getMax();
      _execute_items[i].vptr = items[i].getValuePointer();  
   }
}

void ABCDESerial::onDataRequest(abcde_callback_t callback) {
   _on_data_request = callback;
}

void ABCDESerial::onConfigComplete(abcde_callback_t callback) {
   _on_config_complete = callback;
}

void ABCDESerial::init(Stream &serial) {
   // check item null here
   _BTSerial = &serial;
   /* State Control*/
   _recv_state = ABCDE_RECV_STATE_IDLE;
   _recv_index = 0;
   /* Timer Controls */
   _timer_started = false;
   _timer_millis = 0;
}

/**
 * @brief ABCDE Loop function, Put this inside Arduino's void loop() or RTOS task.
 * This will be valid only when .....
 */
void ABCDESerial::loop() {
    _current_millis = millis();
    /* TIMEOUT EVENT */
    if ((_timer_started == true) && (_current_millis - _timer_millis > ABCDE_RECV_TIMEOUT_MS)) {
        ESP_LOGE("BTCONFIG", "ERROR : RECIEVE TIMED-OUT!");
        _timer_started = false;
        _recv_state = ABCDE_RECV_STATE_IDLE;
    }
    if (_BTSerial->available()) {
        _recv_char =  _BTSerial->read();
        // ESP_LOGI("BTCONFIG", "Recieved : (0x%02X) Index : %d", _recv_char, _recv_index);
        switch (_recv_state) {
            case ABCDE_RECV_STATE_IDLE:
                if (_recv_char == ABCDE_PACKET_SOF) {
                   _timer_started = true;              // --> TIMER START
                    _timer_millis = _current_millis;    // --> TIMER START
                    _recv_index = 0;                              // RESET INDEX
                    _recv_buffer[_recv_index++] = _recv_char;     // STORE DATA (INDEX WILL BE 1);
                    _recv_state = ABCDE_RECV_STATE_START;               // CHANGE STATE
                    ESP_LOGI("BTCONFIG", "PACKET = STARTED");
                }
            break;
            case ABCDE_RECV_STATE_START:                            // BACK TO IDLE IF THE TIMEOUT OCCURS
                if (_recv_index == 1) {
                    _timer_millis = _current_millis;
                    _recv_buffer[_recv_index++] = _recv_char;                                   // STORE DATA (INDEX WILL BE 2);
                } else if (_recv_index == 2) {
                    _timer_millis = _current_millis;
                    _recv_buffer[_recv_index++] = _recv_char;                                   // STORE DATA (INDEX WILL BE 3);
                    _recv_length = (uint16_t)((_recv_buffer[1] << 8) | (_recv_buffer[2]));      // READS BY BIG-ENDIAN FORMAT
                    if (_recv_length < ABCDE_RECV_BUFFER_SIZE - 16) {
                        ESP_LOGI("BTCONFIG", "LENGTH = %d", _recv_length);
                        _recv_state = ABCDE_RECV_STATE_RECIEVE;
                    } else {
                        ESP_LOGE("BTCONFIG", "ERROR : TOO MUCH LENGTH!");
                        _recv_state = ABCDE_RECV_STATE_IDLE;
                    }
                }
            break;
            case ABCDE_RECV_STATE_RECIEVE:                          // BACK TO IDLE IF THE TIMEOUT OCCURS
                if (_recv_index < _recv_length - 2) {
                    _timer_millis = _current_millis;
                    _recv_buffer[_recv_index++] = _recv_char;
                } else {
                    _timer_millis = _current_millis;
                    _recv_buffer[_recv_index++] = _recv_char;
                    _recv_state = ABCDE_RECV_STATE_STOP;
                }
            break;
            case ABCDE_RECV_STATE_STOP:                             // VALIDATE BEFORE GETTING BACK TO IDLE
                _timer_started = false;  // STOP TIMER
                _recv_buffer[_recv_index++] = _recv_char;
                if (_recv_buffer[(_recv_length - 1)] == ABCDE_PACKET_EOF) {
                    ESP_LOGI("BTCONFIG", "PACKET TERMINATED OK");
                    uint16_t checksum = (uint16_t)((_recv_buffer[(_recv_length - 3)] << 8) | (_recv_buffer[(_recv_length - 2)]));
                    ESP_LOGI("BTCONFIG", "TWO'S COMPLEMENT CHECKSUM-16 = 0x%04X", checksum);
                    for (uint16_t index = 3; index < (_recv_length - 3); index++) {
                        checksum += _recv_buffer[index];
                    }
                    ESP_LOGI("BTCONFIG", "LOCAL VALIDATION SUM = 0x%04X", checksum);
                    if (checksum == 0) {
                        ESP_LOGI("BTCONFIG", "VALID PACKET FOUND -> CALLING BACK / PUTING TO QUEUE");
                        _recv_buffer[(_recv_length - 3)] = '\0';  // TERMINATE THE PAYLOAD
                        uint8_t command = _recv_buffer[3];
                        char reply_json[ABCDE_SEND_BUFFER_SIZE];
                        uint16_t reply_len;
                        if(command == ABCDE_CMD_ABCDE_ASK) {
                            sprintf(reply_json,"{\"cmd\":\"0x%02X\"}\0",ABCDE_CMD_ABCDE_CONFIRM);
                            reply_len = strlen(reply_json);
                        } else if(command == ABCDE_CMD_CONFIG_REQUEST) {
                            generateJson(reply_json,sizeof(reply_json),ABCDE_CMD_CONFIG_REPLY,_config_items,_config_items_num);
                            reply_len = strlen(reply_json);
                        } else if(command==ABCDE_CMD_DATA_REQUEST) {
                            _on_data_request();
                            generateJson(reply_json,sizeof(reply_json),ABCDE_CMD_DATA_REPLY,_display_items,_display_items_num);
                            reply_len = strlen(reply_json);
                        } else if(command==ABCDE_CMD_EXECUTE_REQUEST) {
                            generateJson(reply_json,sizeof(reply_json),ABCDE_CMD_EXECUTE_REPLY,_execute_items,_execute_items_num);
                            reply_len = strlen(reply_json);
                        } else if (command == ABCDE_CMD_CONFIG_APPLY) {
                            char* json_payload = (char*)&_recv_buffer[4];   // points to the json payload in the buffer.
                            /* Parsing JSON */
                            char data_obj_ptr[ABCDE_SEND_BUFFER_SIZE];
                            memset(data_obj_ptr,0x00,ABCDE_SEND_BUFFER_SIZE);
                            int num_items = getJsonValue(json_payload,"data",data_obj_ptr);
                            if(num_items > 0) {
                                char item_key[4];   // compare with number
                                char type_buff[4];  // compare with string
                                char item_buff[512];    // may not have to copy
                                char value_buff[64];    // may not have to copy
                                for(int i = 0; i < num_items; i++) {
                                    sprintf(item_key,"%d\0",i);
                                    if(getJsonValue(data_obj_ptr,item_key,item_buff) > 0) {
                                        if(getJsonValue(item_buff,"value",value_buff) == 0) {
                                            if(getJsonValue(item_buff,"type",type_buff) == 0) {
                                                if(strcmp("int",type_buff) == 0) {
                                                    int value_int = atoi(value_buff); // save int
                                                    *((int*)_config_items[i].vptr) = value_int;
                                                } else if (strcmp("str",type_buff) == 0) {
                                                    strcpy((char*)_config_items[i].vptr, value_buff);
                                                } else {
                                                    ESP_LOGE(TAG,"item number %d has invalid item type \"%s\"",i , type_buff);
                                                }
                                            } else {
                                                ESP_LOGE(TAG,"can't get the type of item number %d",i);
                                            }
                                        } else {
                                            ESP_LOGE(TAG,"can't get the value of item number %d",i);
                                        }
                                    } else {
                                        ESP_LOGE(TAG,"the item number %d not found",i);
                                    }
                                }
                                sprintf(reply_json,"{\"cmd\":\"0x%02X\"}\0",ABCDE_CMD_CONFIG_COMPLETE);
                                _BTSerial->write(reply_json,reply_len);
                                _BTSerial->write('\r');
                                _BTSerial->write('\n');
                                delay(1500);
                                // CALLBACK SAVE FUNCTION
                                _on_config_complete();
                            } else {
                                ESP_LOGE(TAG,"The \"data\" object is invalid!");
                            }
                        } else if (command >= 0x00 && command < 0xAA) {
                            abcde_execute_t callback = (abcde_execute_t)(_execute_items[command].vptr);
                            callback(reply_json,&reply_len);
                        } 
                        _BTSerial->write(reply_json,reply_len);
                        _BTSerial->write('\r');
                        _BTSerial->write('\n');
                    } else {
                        ESP_LOGE("BTCONFIG", "ERROR : INVALID PACKET CHECKSUM");
                        _recv_state = ABCDE_RECV_STATE_IDLE;
                    }
                    _recv_state = ABCDE_RECV_STATE_IDLE;
                } else {
                ESP_LOGE("BTCONFIG", "ERROR : PACKET NOT TERMINATED");
                _recv_state = ABCDE_RECV_STATE_IDLE;
            }
            break;
        }
    }
}

/**
 * @brief Generates an ABCDE JSON payload.
 * 
 * @param payload_ptr Pointer to the destination payload buffer.
 * @param payload_size Size of the destination payload.
 * @param command An ABCDE command.
 * @param items_array Source ABCDE items array.
 * @param items_num Number of the items in the ABCDE items array.
 * @return 0 on success.
 * @return -1 on failure. 
 */
int ABCDESerial::generateJson(char* payload_ptr, char payload_size, abcde_cmd_t command, abcde_item_t* items_array, uint8_t items_num) {
    if(payload_ptr == NULL || items_array == NULL) {
        ESP_LOGE(TAG,"(unchecked) null pointer(s) detected");
        return ESP_FAIL;
    }
    memset(payload_ptr, 0x00, payload_size);
    sprintf(payload_ptr, "{\"cmd\":\"0x%02X\",\"data\":{", command);
    for (int i = 0; i < items_num; i++) {
        char temp_str[512] = "";
        if (command == ABCDE_CMD_CONFIG_REPLY || command == ABCDE_CMD_DATA_REPLY) {
            if (items_array[i].type == ABCDE_TYPE_INT) {
                sprintf(temp_str, "\"%d\":{\"name\":\"%s\",\"type\":\"int\",\"min\":%d,\"max\":%d,\"value\":\"%d\"}",
                    i,
                    items_array[i].name, 
                    items_array[i].min,
                    items_array[i].max,
                    *((int*)items_array[i].vptr));
            } else if (items_array[i].type == ABCDE_TYPE_STR) {
                sprintf(temp_str, "\"%d\":{\"name\":\"%s\",\"type\":\"str\",\"min\":%d,\"max\":%d,\"value\":\"%s\"}",
                    i,
                    items_array[i].name,
                    items_array[i].min,
                    items_array[i].max,
                    (char*)items_array[i].vptr);
            } else {
                ESP_LOGE(TAG, "(unchecked) serialization failed, invalid item #%d type : %d.", i, items_array[i].type);
                return ESP_FAIL;
            }
            strcat(payload_ptr, temp_str);
        } else if (command == ABCDE_CMD_EXECUTE_REPLY) {
            sprintf(temp_str, "\"%d\":{\"name\":\"%s\",\"value\":\"%d\"}", i, items_array[i].name, i);
            strcat(payload_ptr, temp_str);
        } else {
            ESP_LOGE(TAG, "(unchecked) serialization failed, invalid command : 0x%02X", command);
            return ESP_FAIL;
        }
        if (i < (items_num - 1)) {
            strcat(payload_ptr, ",");   // adds a trailling comma.
        }
    }
    strcat(payload_ptr, "}}\r\n");
    return ESP_OK;
}

/**
 * @brief Gets the value of the specified JSON key and copy it into buffer.
 * 
 * @param json_string A string containing JSON format.
 * @param lookup_key A String literal for the key to look up for.
 * @param value_ptr_ref The reference of the value buffer.
 * @return int Number of childs for the 'object' type values. Zero for other types. Negative for errors
 */
int ABCDESerial::getJsonValue(const char* json_string, const char* lookup_key, char* dest_value_buff) {
    if(json_string == NULL || dest_value_buff == NULL) {
        ESP_LOGE(TAG,"(unchecked) null pointer execption");
        return JSMN_NULL_POINTERS;
    }
    jsmn_parser json_parser;
    jsmn_init(&json_parser);
    jsmntok_t tokens[ABCDE_MAX_JSMN_TOKENS];
    int num_tokens = jsmn_parse(&json_parser, json_string, strlen(json_string), tokens, ABCDE_MAX_JSMN_TOKENS);
    if(num_tokens < 0) {
        ESP_LOGE(TAG,"(checked) JSON tokens parsing error : %d", num_tokens);
        return JSMN_ERROR_INVAL;
    }
    for(int i = 0; i < (num_tokens-1); i++) {
        if(tokens[i].type == JSMN_STRING) {
            char* token_key_ptr = (char*)json_string + tokens[i].start;
            int token_key_strlen = tokens[i].end - tokens[i].start;
            if(token_key_strlen == strlen(lookup_key)) {
                if(memcmp(lookup_key,token_key_ptr,token_key_strlen) == 0) {
                    const char* value_ptr = json_string + tokens[i+1].start;
                    size_t value_len = tokens[i+1].end - tokens[i+1].start;
                    strncpy(dest_value_buff, value_ptr, value_len);
                    *(dest_value_buff + value_len) = '\0';
                    return tokens[i+1].size;
                } else {
                    ; // the key content doesn't matched.
                }
            } else {
                ; // the key length doesn't matched. 
            }
        } else {
            ; // the current token is not a "string" type.
        }
    }
    return JSMN_NO_KEY; //the key is not found.
}