#ifndef F_CPU
#define F_CPU             1000000
#endif

// =========== TIMINGS ==============

#define RESET_PULSE       (485)
#define SLOT_START_W      (10)
#define SLOT_START_R      (5)
#define SLOT_SAMPLE       (50)
#define SLOT_RECOVER      (5)

// ==================================


#define _PORT             (PORTD)
#define _DDR              (DDRD)
#define _PIN              (PD7)

// ROM Commands :
#define SEARCH_ROM        (0xF0)
#define READ_ROM          (0x33)
#define MATCH_ROM         (0x55)
#define SKIP_ROM          (0xCC)
#define ALARM_SEARCH      (0xEC)

// DS18B20 Function Commands :
#define CONVERT_T         (0x44)
#define WRITE_SCRATCHPAD  (0x4E)
#define READ_SCRATCHPAD   (0xBE)
#define COPY_SCRATCHPAD   (0x48)
#define RECALL_E          (0xB8)
#define READ_POWER_SUPPLY (0xB4)


void ds_convert_temp();
