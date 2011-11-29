#include <ioavr.h>
#include <iom8.h>

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

