#include "E:\Work\electro\proiecte\test_18b20\test18b20.h"
#include "E:\Work\electro\proiecte\test_18b20\lcd.c"
#include "E:\Work\electro\proiecte\test_18b20\18B20.c"
#include "E:\Work\electro\proiecte\test_18b20\rtc_ds1307.c"

char fw_string[21] ="Termostat v0.99 beta"; 
const char menu_lines[5][20] = {"Ceas",
                               "Data",
                               "Temperatura",
                               "Exit",
                               "Temporizare"};
                               
const char monthnames[12][4] = {"Ian","Feb","Mar","Apr","Mai","Iun","Iul","Aug","Sep","Oct","Nov","Dec"};     
int g_day;
int g_month;
int g_year;
int g_sec;
int g_min;
int g_hr;

void print_calendar(int x, int y)
{
   g_day = bcd_to_int (read_ds1307 (4));
   g_month = bcd_to_int (read_ds1307 (5));
   g_year =  bcd_to_int (read_ds1307 (6));
   
   lcd_gotoxy (x,y);
   printf (lcd_putc, "%d %s 2%03d      ", g_day, monthnames[g_month-1], g_year);
}

void print_clock()
{
   g_sec = bcd_to_int (read_ds1307 (0));
   g_min = bcd_to_int (read_ds1307 (1));
   g_hr =  bcd_to_int (read_ds1307 (2));//0x3f && 
   lcd_gotoxy (1,1);
   printf (lcd_putc, "%02d:%02d:%02d", g_hr, g_min, g_sec);
}

#define LED PIN_C0
enum Buttons{
   BUTTON_NONE = -1,
   BUTTON_UP = 0,
   BUTTON_DOWN = 1,
   BUTTON_SET = 2
};

int1 led_on = 0;
unsigned int mychar;
void toggle_led ();
void process_button ();
int g_button = BUTTON_NONE;


void main()
{   int16 counter = 0;
   float temperature_c = 0.0;
   
   setup_adc_ports(NO_ANALOGS|VSS_VDD);
   setup_adc(ADC_OFF|ADC_TAD_MUL_0);
   setup_wdt(WDT_OFF);
   setup_timer_0(RTCC_INTERNAL);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
  // setup_oscillator(False);
  
   enable_interrupts (INT_RB);
   enable_interrupts (GLOBAL);
   
   set_tris_a (0);
   lcd_init();
   lcd_gotoxy(0,0);
   printf (lcd_putc, "test DS18b20 v0.2");
   delay_ms (1000);
  /*while (1){
      delay_ms (500);
      process_button ();
      lcd_gotoxy(2,2);
      printf( lcd_putc, "%c - %u", mychar, mychar);
   }*/
   while (1){
      delay_ms (1000);
     // counter++;
      toggle_led ();
      print_clock ();
      print_calendar (10,1);
      if (!ow_reset()) { 
            // set the number of bits of resolution on the ds1822.
            // note that this affects the conversion time,
            // RTFM and see the called function.
           onewire_ds1822_set_temperature_resolution(12);

            // read the temperature from the ds1822.
            // note that the returned value is in deg C,
            // and the "lite" function returns only positive temps.
            temperature_c = onewire_ds1822_read_temp_c_lite();
            lcd_gotoxy(1,2);
            printf (lcd_putc, " test DS18b20 v0.1");
            lcd_gotoxy(1,3);
            printf( lcd_putc, "Temp %.2f%cC \r\n", temperature_c, 223 ); // print temperature C 
      }
      else{
            lcd_clear ();
            lcd_gotoxy(1,3);
            printf( lcd_putc, "Error");
      }
   }
}

void process_button()
{
   if (g_button == BUTTON_UP){
      mychar++;
   }
   else if (g_button == BUTTON_DOWN){
      mychar--;
   }
   g_button = BUTTON_NONE;
}

#int_RB
RB_isr() 
{
   //g_button = BUTTON_NONE;
   if (input(PIN_B5)){
      g_button = BUTTON_UP;
   }
   else if (input(PIN_B6)){
      g_button = BUTTON_DOWN;
   }
   else if (input(PIN_B7)){
      g_button = BUTTON_SET;
   }
   return 0;
}

void toggle_led ()
{
   if (led_on == 0){
      output_high (LED);
      led_on = 1;
   }
   else{
      output_low (LED);
      led_on = 0;
   }
}
