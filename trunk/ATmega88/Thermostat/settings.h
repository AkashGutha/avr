// Thermostat functional settings
// 1°C = 16 units

#define TEMP_MAX      (1592) // The maximum temperature the user can set
#define TEMP_MIN      (-152)   // The minimum temperature the user can set
#define TEMP_STEP     (8)  // The increment in setting the temperature
#define TEMP_DELTA    (8)   // The deviation from the desired temperature at which the relay turns on / off
#define TEMP_DEFAULT  (-8) // Default temperature

// Thermostat behaviour settings

//#define FRIDGE
#define TEMP_READ_PERIOD  4000

// Display settings

#define CHAR_DEGREE   10
#define CHAR_C        11
#define CHAR_MINUS    12

#define MODE_SET      0
#define MODE_DISPLAY  1

#define KEY_LOOP_SHORT  10
#define KEY_LOOP_LONG   20
#define KEY_LOOP_WAIT   15

#define BLINK_MODE_SET    35
#define BLINK_MODE_NORMAL 0
