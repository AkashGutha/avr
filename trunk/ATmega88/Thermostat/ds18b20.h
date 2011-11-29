#include <avr/io.h>

/* Thermometer Connections (At your choice) */
#define THERM_PORT              PORTB
#define THERM_DDR               DDRB
#define THERM_PIN               PINB
#define THERM_DQ                PB0

/* Utils */
#define THERM_INPUT_MODE()      THERM_DDR&=~(1<<THERM_DQ)
#define THERM_OUTPUT_MODE()     THERM_DDR|=(1<<THERM_DQ)
#define THERM_LOW()             THERM_PORT&=~(1<<THERM_DQ)
#define THERM_HIGH()            THERM_PORT|=(1<<THERM_DQ

/* DS18B20 commands */
#define THERM_CMD_CONVERTTEMP   0x44
#define THERM_CMD_RSCRATCHPAD   0xbe
#define THERM_CMD_WSCRATCHPAD   0x4e
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM     0xb8
#define THERM_CMD_RPWRSUPPLY    0xb4
#define THERM_CMD_SEARCHROM     0xf0
#define THERM_CMD_READROM       0x33
#define THERM_CMD_MATCHROM      0x55
#define THERM_CMD_SKIPROM       0xcc
#define THERM_CMD_ALARMSEARCH   0xec

#define THERM_DECIMAL_STEPS_12BIT 625 //.0625
#define THERM_RES_9BIT          0
#define THERM_RES_10BIT         1
#define THERM_RES_11BIT         2
#define THERM_RES_12BIT         3

void therm_set_resolution(char res);

short therm_read_temperature(char* buffer);
