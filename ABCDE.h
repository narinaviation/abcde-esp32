#ifndef __ABCDE_H__
#define __ABCDE_H__

#include <Arduino.h>

#define ABCDE_RECV_BUFFER_SIZE   1024
#define ABCDE_SEND_BUFFER_SIZE   1024
#define ABCDE_RECV_TIMEOUT_MS    5000
#define ABCDE_PACKET_SOF   0x02
#define ABCDE_PACKET_EOF   0x03
#define ABCDE_MAX_JSMN_TOKENS    127

typedef enum {
    ABCDE_TYPE_INT = 0,
    ABCDE_TYPE_STR = 1,
    ABCDE_TYPE_FUN = 255
} abcde_type_t;

typedef enum {
    ABCDE_CMD_ABCDE_ASK       = 0xAA,
    ABCDE_CMD_ABCDE_CONFIRM   = 0xAC,
    ABCDE_CMD_CONFIG_REQUEST  = 0xC1,
    ABCDE_CMD_CONFIG_REPLY    = 0xC0,
    ABCDE_CMD_CONFIG_APPLY    = 0xCA,
    ABCDE_CMD_CONFIG_COMPLETE = 0xCC,
    ABCDE_CMD_DATA_REQUEST    = 0xD1,
    ABCDE_CMD_DATA_REPLY      = 0xD0,
    ABCDE_CMD_EXECUTE_REQUEST = 0xE1,
    ABCDE_CMD_EXECUTE_REPLY   = 0xE0,
} abcde_cmd_t;

typedef enum {
    ABCDE_RECV_STATE_IDLE       = 0,
    ABCDE_RECV_STATE_START      = 1,
    ABCDE_RECV_STATE_RECIEVE    = 2,
    ABCDE_RECV_STATE_STOP       = 3,
    ABCDE_RECV_STATE_VALIDATE   = 4,
    ABCDE_RECV_STATE_COMPLETE   = 5
} recv_state_t;

typedef enum {
    JSMN_NULL_POINTERS=-4,
    JSMN_NO_TOKENS=-3,
    JSMN_NO_KEY=-2,
    JSMN_FAIL=-1,
    JSMN_OK=0
} jsmn_err_t;

typedef struct {
    char   name[32];
    int    type;
    int    min;
    int    max;
    void*  vptr;
} __attribute__((packed)) abcde_item_t;

typedef void* (*abcde_execute_t)(char* reply_json, uint16_t* reply_len);
typedef void (*abcde_callback_t)(void);

class ABCDEItem {
    private:
        char  name[32];
        int   type;
        int   min;
        int   max;
        void* vptr;
    public:
        bool  set(const char* name, abcde_type_t type, int min, int max, void* vptr);
        bool  getName(char* dest);
        int   getType();
        int   getMin();
        int   getMax();
        void* getValuePointer();
};

class ABCDESerial {
    private:
        Stream*           _BTSerial;
        int               _ConfigItemsSize;
        int               _DisplayItemsSize;
        int               _ExecuteItemsSize;
        recv_state_t      _recv_state;
        uint8_t           _recv_buffer[ABCDE_RECV_BUFFER_SIZE];
        uint8_t           _recv_char;
        uint16_t          _recv_index;
        uint16_t          _recv_length;
        bool              _timer_started;
        uint64_t          _current_millis;
        uint64_t          _timer_millis;
        abcde_item_t*     _config_items;
        abcde_item_t*     _display_items;
        abcde_item_t*     _execute_items;
        int               _config_items_num;
        int               _display_items_num;
        int               _execute_items_num;
        abcde_callback_t  _on_data_request;
        abcde_callback_t  _on_config_complete;
        int generateJson(char* payload_ptr, char payload_size, abcde_cmd_t command, abcde_item_t* items_array, uint8_t items_num);
        int getJsonValue(const char* json_string, const char* lookup_key_ptr, char* dest_value_buff);
    public:
        ABCDESerial();  
        ~ABCDESerial();
        void init(Stream &serial);
        void setConfigItems(ABCDEItem items[], int items_num);
        void setDisplayItems(ABCDEItem items[], int items_num);
        void setExecuteItems(ABCDEItem items[], int items_num);
        void onDataRequest(abcde_callback_t callback);
        void onConfigComplete(abcde_callback_t callback);
        void loop(void);
};

#endif //__ABCDE_H__