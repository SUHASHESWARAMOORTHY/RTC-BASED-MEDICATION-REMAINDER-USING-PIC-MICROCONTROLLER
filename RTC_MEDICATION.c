#include <16F877A.h>
#device ADC=16
#FUSES NOWDT                    // No Watch Dog Timer
#FUSES PUT                      // Power Up Timer
#FUSES NOBROWNOUT               // No brownout reset
#FUSES NOLVP                    // No low voltage programming, B3(PIC16) or B5(PIC18) used for I/O
#use delay(crystal=4MHz)
#use i2c(Master,Fast,sda=PIN_C4,scl=PIN_C3)
// Define pins for LCD
#define LCD_ENABLE_PIN PIN_D0
#define LCD_RS_PIN PIN_D1
#define LCD_RW_PIN PIN_D2
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7

// Define buzzer pin
#define BUZZER_PIN PIN_A5  // Buzzer on PIN_A5

#include <lcd.c>

// Variables for time and alarm
int sec = 0x00, min = 0x00, hrs = 0x71, day = 0x00, hr, AMPM, date = 0x13, month = 0x09;
int alarm_hr = 0;   // Alarm hour
int alarm_min = 0;  // Alarm minute
int alarm_AMPM = 0; // AM = 0, PM = 1
int set_alarm = 0;        // Alarm set mode flag
int position = 0;         // Cursor position for setting alarm
int alarm_confirmed = 0;  // Flag to indicate if the alarm is confirmed (via button B5)
int alarm_triggered = 0;  // Flag to indicate if the alarm has been triggered
unsigned int16 alarm_timer = 0; // Timer for buzzer duration

// Variables for storing confirmed alarm time
int stored_alarm_hr = 0;   // Stored alarm hour
int stored_alarm_min = 0;  // Stored alarm minute
int stored_alarm_AMPM = 0; // Stored alarm AM/PM (0 for AM, 1 for PM)

// Convert BCD to decimal
int bcd_to_decimal(int bcd_value) {
    return ((bcd_value / 16) * 10) + (bcd_value % 16);
}

// Convert decimal to BCD
int decimal_to_bcd(int decimal_value) {
    return ((decimal_value / 10) * 16) + (decimal_value % 10);
}

// Function to read data from RTC using I2C
byte read(byte add) {
    int data;
    i2c_start();
    i2c_write(0xd0);  // I2C address of RTC
    i2c_write(add);   // Register address
    i2c_stop();
    i2c_start();
    i2c_write(0xd1);  // Read mode
    data = i2c_read(0);
    i2c_stop();
    return data;
}

// Function to write data to RTC using I2C
void write() {
    i2c_start();
    i2c_write(0xd0);  // I2C address of RTC
    i2c_write(0x00);  // Starting word address
    i2c_write(decimal_to_bcd(sec));   // Seconds in BCD
    i2c_write(decimal_to_bcd(min));   // Minutes in BCD
    i2c_write(decimal_to_bcd(hrs));   // Hours in BCD
    i2c_write(day);                      // Day of the week
    i2c_write(decimal_to_bcd(date));   // Date in BCD
    i2c_write(decimal_to_bcd(month));  // Month in BCD
    i2c_stop();
}

// Function to store alarm settings
void store_alarm() {
    stored_alarm_hr = alarm_hr;
    stored_alarm_min = alarm_min;
    stored_alarm_AMPM = alarm_AMPM;
}

// Function to check if current time matches stored alarm time
void check_alarm() {
    if (alarm_confirmed) {  // Check if alarm has been confirmed by the user
        int current_AMPM = (hrs & 0x20) ? 1 : 0; // Determine AM/PM
        int current_hr = hrs & 0x1F;  // Mask off the AM/PM bit

        // Adjust 12-hour format
        if (current_hr == 0) current_hr = 12;
        else if (current_hr > 12) current_hr -= 12;

        // Check if RTC time matches stored alarm time
        if ((min == decimal_to_bcd(stored_alarm_min)) &&
            (current_hr == decimal_to_bcd(stored_alarm_hr)) &&
            (current_AMPM == stored_alarm_AMPM)) {
            
            if (!alarm_triggered) { // Only trigger if not already triggered
                alarm_triggered = 1; // Set alarm triggered flag
                output_high(BUZZER_PIN);
                lcd_putc('\f');           // Clear the LCD screen
                printf(lcd_putc, "TABLET TIME"); // Display "TABLET TIME" on LCD
                delay_ms(10000);
            }
        } else {
            // Reset alarm trigger if conditions are not met
            alarm_triggered = 0;
            output_low(BUZZER_PIN);
        }
    }
}


// Function to display alarm settings on the LCD
void display_alarm() {
    lcd_gotoxy(1, 2);
    if (alarm_AMPM == 1) {
        printf(lcd_putc, "ALARM %02d:%02d PM", alarm_hr, alarm_min); // If PM
    } else {
        printf(lcd_putc, "ALARM %02d:%02d AM", alarm_hr, alarm_min); // If AM
    }
}

// Function to debounce buttons with reduced time
void debounce() {
    delay_ms(20);  // Reduced debounce time for quicker button response
}

// Main function
void main() {
    lcd_init();
    write();

    // Enable PORTB pull-up resistors for button inputs
    port_b_pullups(TRUE);
    set_tris_b(0xFF);  // Set PORTB as input (buttons on PORTB pins)

    // Set buzzer pin as output
    output_low(BUZZER_PIN);  // Buzzer off initially

    while(TRUE) {
        // Read current time from RTC
        sec = read(0);
        min = read(1);
        hrs = read(2);
        day = read(3);
        date = read(4);
        month = read(5);

        // Convert BCD values to decimal for display and logic
       // Convert BCD values to decimal for display and logic
sec = bcd_to_decimal(sec);
min = bcd_to_decimal(min);
hrs = bcd_to_decimal(hrs);
day = bcd_to_decimal(day);
date = bcd_to_decimal(date);  // Convert date to decimal
month = bcd_to_decimal(month);  // Convert month to decimal


        // Convert RTC hours to 12-hour format
        hr = hrs & 0x1F;  // Get the hour in 12-hour format
        if (hr == 0) {
            hr = 12; // Convert 0 hour to 12 for display
        } else if (hr > 12) {
            hr -= 12; // Adjust hour greater than 12
        }

        // Display time on LCD
        printf(lcd_putc, "\f%02d:%02d:%02d ", hr, min, sec);  // Use %02d for formatting
        lcd_gotoxy(11, 1);
        AMPM = hrs & 0x20;
        if (AMPM == 0x20) {
            printf(lcd_putc, "PM");
        } else {
            printf(lcd_putc, "AM");
        }

        // Display day of the week
        lcd_gotoxy(1, 2);
        switch(day) {
            case 0: printf(lcd_putc, "SUNDAY"); break;
            case 1: printf(lcd_putc, "MONDAY"); break;
            case 2: printf(lcd_putc, "TUESDAY"); break;
            case 3: printf(lcd_putc, "WEDNESDAY"); break;
            case 4: printf(lcd_putc, "THURSDAY"); break;
            case 5: printf(lcd_putc, "FRIDAY"); break;
            case 6: printf(lcd_putc, "SATURDAY"); break;
        }

        // Display date and month
        // Display date and month
lcd_gotoxy(11, 2);
printf(lcd_putc, "%02d/%02d", date, month);  // Display date and month in decimal format
 // Display date and month

        // Check if middle button (RB1) is pressed to toggle alarm set mode
        if (!input(PIN_B1)) {  
            debounce();  
            while(!input(PIN_B1));  
            set_alarm = !set_alarm;  
        }

        if (set_alarm) {
            // Adjust alarm time with left/right and up/down buttons
            if (!input(PIN_B0)) {  
                debounce();  
                while(!input(PIN_B0));  
                position = (position > 0) ? position - 1 : position; 
            }
            
            if (!input(PIN_B2)) {  
                debounce();  
                while(!input(PIN_B2));  
                position = (position < 2) ? position + 1 : position; 
            }
            
            if (!input(PIN_B3)) {  
                debounce();  
                while(!input(PIN_B3));  
                if (position == 0) {
                    alarm_hr = (alarm_hr + 1) % 12;
                    if (alarm_hr == 0) alarm_hr = 12;
                } else if (position == 1) {
                    alarm_min = (alarm_min + 1) % 60;
                } else {
                    alarm_AMPM = !alarm_AMPM;
                }
            }
            
            if (!input(PIN_B4)) {  
                debounce();  
                while(!input(PIN_B4));  
                if (position == 0) {
                    alarm_hr = (alarm_hr > 1) ? alarm_hr - 1 : 12;
                } else if (position == 1) {
                    alarm_min = (alarm_min > 0) ? alarm_min - 1 : 59;
                } else {
                    alarm_AMPM = !alarm_AMPM;
                }
            }

            display_alarm();  // Display alarm settings on LCD
        }

        // Check if set button (RB5) is pressed to confirm alarm setting
        if (!input(PIN_B5)) {  
            debounce();  
            while(!input(PIN_B5));  
            if (set_alarm) {
                set_alarm = 0;              // Exit alarm setting mode
                alarm_confirmed = 1;        // Confirm the alarm setting
                store_alarm();              // Store the alarm time
            } else if (alarm_triggered) {
                output_low(BUZZER_PIN);     // Disable the buzzer
                alarm_triggered = 0;        // Reset alarm trigger flag
                alarm_timer = 0;            // Reset alarm timer
            }
        }

        // Call check_alarm to compare RTC time with stored alarm
        check_alarm();

        // Handle buzzer timing if alarm is triggered
        if (alarm_triggered) {
            alarm_timer++;
            if (alarm_timer >= 60) { // 1 minute passed
                output_low(BUZZER_PIN);   // Turn off the buzzer
                alarm_triggered = 0;      // Reset alarm trigger flag
                alarm_timer = 0;          // Reset timer
            }
        } else {
            alarm_timer = 0; // Reset timer if alarm is not triggered
        }

        delay_ms(1000); // Main loop delay
    }
}
