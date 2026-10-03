#include <lpc21xx.h>
#include "types.h"
#include "delay.h"
#include "LCD.h"
#include "ADC.h"
#include "ADC_defines.h"
#include "LM35.h"
#include "KPM.h"
#include "LCD_defines.h"
#include "interrupts_Mini.h"
#include "MQ2.h"

// Variable of threshold value
f32 temp_threshold = 45.0;
u32 gas_threshold = 600;

// Define pin configurations here
// Alert LED
#define ALLERT_LED 0

// SW2 clear buzzer and led
#define SW2 1

// Buzzer pin
#define BUZ_PIN 27

// RTC_Peripheral_defines
//#define FOSC 12000000
//#define CCLK (5 * FOSC)
//#define PCLK (CCLK / 4)

#define PREINT_VAL  ((int)(PCLK / 32768) - 1)
#define PREFRAC_VAL (PCLK - ((PREINT_VAL + 1) * 32768))

#define RTC_ENABLE  (1 << 0)
#define RTC_RESET   (1 << 1)
#define RTC_CLKSRC  (1 << 4)

// To silence buzzer and turn off LED
extern volatile u32 flag;
volatile u32 alert_silenced = 0;

// To edit RTC, Set values and password
extern volatile u32 edit_flag;
s32 hour=11, min=11, sec, date=3, month=10, year=26, day;

volatile u8 rtc_edit, rtc_edit_time, rtc_edit_date;

u32 current_pass = 111; // You can change this default password

// Temperature and gas values
f32 currentTemp = 0.0, recordedTemp = 0.0;
u32 gasValue = 0, recordedGasValue = 0;

// Initialization of rtc
void RTC_Init(void)
{
    // Disable and reset the RTC
    CCR = RTC_RESET;
#ifndef CPU_LPC2148
    // Set prescaler integer and fractional parts
    PREINT = PREINT_VAL;
    PREFRAC = PREFRAC_VAL;

    // Enable the RTC
    CCR = RTC_ENABLE; // LPC_2129
#else
    // Enable the RTC with external clock source
    CCR = RTC_ENABLE | RTC_CLKSRC; // LPC_2148
#endif
}
void SetRTCDateInfo(u32 date_val, u32 month_val, u32 year_val);
void SetRTCTimeInfo(u32 hour_val, u32 minute_val, u32 second_val);
// System Initialization
void system_init(void)
{
    Init_LCD();
    Init_ADC();
    Init_KPM();
    RTC_Init();
		SetRTCDateInfo(date, month, year);
    SetRTCTimeInfo(hour, min, sec);
    // Initialize buzzer, LEDs, and external interrupts for SW1/SW2 here
    eint0_enable();
    eint1_enable();
		PINSEL0 &= ~(3 << 0);
    IODIR0 |= 1 << ALLERT_LED;
    IODIR1 |= 1 << BUZ_PIN;
}

u32 i, j;

void displayMSG(void)
{
    // Display welcome banner
    u8 msg[] = "KITCHEN SAFETY HEAT AND GAS MONITORING SYSTEM    "; // Add trailing spaces for smooth wrapping
    int len;
    int start, idx;
    
    len = 48; // Length of string including spaces
    WRITE_LCD_CMD(GOTO_LINE1_POS0);
    StrLCD((u8*)"VECTOR ID:");
    WRITE_LCD_CMD(GOTO_LINE2_POS0);
    StrLCD((u8*)"V25HE11G9");
    delay_ms(1000);
    
    WRITE_LCD_CMD(CLEAR_LCD);
    WRITE_LCD_CMD(GOTO_LINE1_POS0);
    StrLCD((u8*)"PROJECT TITTLE");

    for (start = 0; start < len; start++) {
        WRITE_LCD_CMD(GOTO_LINE2_POS0);

        // Print 16 characters starting from the current offset
        for (idx = 0; idx < 16; idx++) {
            WRITE_LCD_DATA(msg[(start + idx) % len]);
        }
        delay_ms(100);
    }
    WRITE_LCD_CMD(CLEAR_LCD);
}

// To Clear Last digit
void lcd_clear_last_digit(void)
{
    WRITE_LCD_CMD(0x10); // Left shift the cursor
    WRITE_LCD_DATA(' '); // Overwriting the character with space
    WRITE_LCD_CMD(0x10); // Left shift the cursor
}

// Numeric Input with Real-Time Screen Echo
u32 kpm_read_numeric_echo(void)
{
    u32 num = 0;
    u8 key;

    while (1) {
        key = keyscan(); // Wait for key press

        if (key >= '0' && key <= '9') {
            num = (num * 10) + (key - '0');
            WRITE_LCD_DATA(key); // Echo digit immediately to the LCD
        }
        else if (key == 'c') {
            num = num / 10;
            lcd_clear_last_digit();
        }
        else {
            break; // Terminate on non-numeric key (e.g., '=')
        }
    }
    return num;
}

// RTC CONFIGURATION
#define CPU_LPC2148


char week[][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

#define SUN 0
#define MON 1
#define TUE 2
#define WED 3
#define THU 4
#define FRI 5
#define SAT 6

s32 keyValue;

void SetRTCTimeInfo(u32 hour_val, u32 minute_val, u32 second_val)
{
    HOUR = hour_val;
    MIN = minute_val;
    SEC = second_val;
}

void GetRTCTimeInfo(s32 *hour_ptr, s32 *minute_ptr, s32 *second_ptr)
{
    *hour_ptr = HOUR;
    *minute_ptr = MIN;
    *second_ptr = SEC;
}

void SetRTCDateInfo(u32 date_val, u32 month_val, u32 year_val)
{
    DOM = date_val;
    MONTH = month_val;
    YEAR = year_val;
}

void GetRTCDateInfo(s32 *date_ptr, s32 *month_ptr, s32 *year_ptr)
{
    *date_ptr = DOM;
    *month_ptr = MONTH;
    *year_ptr = YEAR;
}

// To Configure RTC Time
void rtc_time_configure(void)
{
    s32 cur_h, cur_m, cur_s;
    u32 rtc_time_menu = 1;
    
    GetRTCTimeInfo(&cur_h, &cur_m, &cur_s);
    hour = cur_h; 
    min = cur_m; 
    sec = cur_s;

    while (rtc_time_menu) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"1.HOUR 2.MIN 3.SEC");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        StrLCD((u8*)"4.EXIT");
        
        rtc_edit_time = keyscan();
        switch (rtc_edit_time) {
            case '1': // 1. Set Hour
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set Hour:(0-23)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo(); 
                
                if (keyValue >= 0 && keyValue <= 23) {
                    hour = keyValue;
                } else {
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"                ");
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"INVALID HOUR");
                    delay_ms(1000);
                }
                break;

            case '2': // 2. Set Minute
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set Minute:(0-59)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo();
                
                if (keyValue >= 0 && keyValue <= 59) {
                    min = keyValue;
                } else {
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"                ");
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"INVALID MIN");
                    delay_ms(1000);
                }
                break;

            case '3': // 3. Set Seconds
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set Seconds:(0-59)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo();
                
                if (keyValue >= 0 && keyValue <= 59) {
                    sec = keyValue;
                } else {
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"                ");
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"INVALID SEC");
                    delay_ms(1000);
                }
                break;

            case '4': // EXIT
                rtc_time_menu = 0;
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"EXIT");
                break;

            default:
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"INVALID RTC TIME");
                return; // Return without updating if invalid key pressed
        }
        
        delay_ms(1000); // Pause so user can read screen
        
        // Apply only after a valid choice
        SetRTCTimeInfo(hour, min, sec);
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"RTC UPDATED TIME");
        delay_ms(1500);
    }
}

// rtc_date_configuration
void rtc_date_configuration(void)
{
    s32 cur_d, cur_m, cur_y;
    u32 rtc_date_menu = 1;
    
    // Fetch current date first
    GetRTCDateInfo(&cur_d, &cur_m, &cur_y);
    date = cur_d; 
    month = cur_m; 
    year = cur_y;
    
    while (rtc_date_menu) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"1.DD 2.MM 3.YY");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        StrLCD((u8*)"4.EXIT  (c=Del)");

        rtc_edit_date = keyscan();
        switch (rtc_edit_date) {
            case '1': // 1. Set Date
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set date:(0-31)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo(); 
                
                if (keyValue >= 0 && keyValue <= 31) {
                    date = keyValue;
                } else {
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"                ");
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"INVALID DATE");
                    delay_ms(1000);
                }
                break;

            case '2': // 2. Set Month
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set Month:(0-12)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo(); 
                
                if (keyValue >= 0 && keyValue <= 12) {
                    month = keyValue;
                } else {
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"                ");
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"INVALID MONTH");
                    delay_ms(1000);
                }
                break;

            case '3': // 3. Set Year
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Set Year:");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                year = kpm_read_numeric_echo(); 
                delay_ms(1000);
                break;

            case '4': // EXIT
                rtc_date_menu = 0;
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"EXIT");
                break;

            default:
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"INVALID RTC DATE");
                return;
        }
        delay_ms(1000);
    }
    
    // Apply only after a valid choice
    SetRTCDateInfo(date, month, year);
    WRITE_LCD_CMD(CLEAR_LCD);
    StrLCD((u8*)"RTC UPDATED DATE");
    delay_ms(1500);
}

void rtc_configuration(void)
{
    u32 rtc_menu = 1;
    
    while (rtc_menu) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"1.TIME 2.DATE");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        StrLCD((u8*)"3.EXIT");
        
        rtc_edit = keyscan();
        switch (rtc_edit) {
            case '1':
                rtc_time_configure();
                break;
            case '2':
                rtc_date_configuration();
                break;
            case '3': // EXIT
                rtc_menu = 0;
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"EXIT");
                break;
            default:
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"INVALID RTC");
        }
        delay_ms(1000);
    }
}

// To Configure Threshold Setpoints
void setpoint_configuration(void)
{
    u32 setpoint_edit, setpoint_menu = 1;
    
    while (setpoint_menu) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"1.TEMP 2.GAS");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        StrLCD((u8*)"3.sVal 4.EXIT");
        
        setpoint_edit = keyscan();
        switch (setpoint_edit) {
            case '1': // Set Temperature Threshold
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Temp:  (c=DEL)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
								keyValue = kpm_read_numeric_echo(); 	
								if (keyValue >= 0 && keyValue <= 200) {
										temp_threshold = keyValue;
								} else {
										WRITE_LCD_CMD(GOTO_LINE2_POS0);
										StrLCD((u8*)"                ");
										WRITE_LCD_CMD(GOTO_LINE2_POS0);
										StrLCD((u8*)"LIMIT EXCEEDED");
										delay_ms(1000);
								}
                delay_ms(1000);
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"TEMP UPDATED");
                break;

            case '2': // Set Gas Threshold
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Gas:  (c=DEL)");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                keyValue = kpm_read_numeric_echo(); 	
								if (keyValue >= 0 && keyValue <= 1000) {
										gas_threshold = keyValue;
								} else {
										WRITE_LCD_CMD(GOTO_LINE2_POS0);
										StrLCD((u8*)"                ");
										WRITE_LCD_CMD(GOTO_LINE2_POS0);
										StrLCD((u8*)"LIMIT EXCEEDED");
										delay_ms(1000);
								}
                delay_ms(1000);
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"GAS UPDATED");
                break;

            case '3':
                WRITE_LCD_CMD(GOTO_LINE1_POS0);
                StrLCD((u8*)"SET VALUES:");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                StrLCD((u8*)"                ");
                WRITE_LCD_CMD(GOTO_LINE2_POS0);
                StrLCD((u8*)"T:");
                F32LCD(temp_threshold, 1);
                WRITE_LCD_DATA(0xDF);
                StrLCD((u8*)"C G:");
                U32LCD(gas_threshold);
                break;

            case '4': // EXIT
                setpoint_menu = 0;
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"EXIT");
                break;

            default:
                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"INVALID SETPOINT");
        }
        delay_ms(1000);
    }
}

u32 readPassword(void)
{
    u32 pass = 0;
    u8 key;

    while (1) {
        key = keyscan(); // Wait for key press

        if (key >= '0' && key <= '9') {
            pass = (pass * 10) + (key - '0');
            WRITE_LCD_DATA(key); // Display the actual digit
            delay_ms(100);       // Wait for 10 ms so user can see it briefly
            WRITE_LCD_CMD(0x10); // Move LCD cursor back 1 position
            WRITE_LCD_DATA('*'); // Overwrite digit position with asterisk
        }
        else if (key == 'c') {
            pass = pass / 10;
            lcd_clear_last_digit();
        }
        else {
            break;
        }
    }
    return pass;
}

// To Change Password via Case 3
void change_password(void)
{
    u32 old_pass, new_pass;

    WRITE_LCD_CMD(CLEAR_LCD);
    StrLCD((u8*)"Enter Old Pass:");
    WRITE_LCD_CMD(GOTO_LINE2_POS0);
    old_pass = readPassword();

    if (old_pass == current_pass) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"Enter New Pass:");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        new_pass = readPassword();

        current_pass = new_pass;

        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"PASS UPDATED!");
    } else {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"WRONG OLD PASS");
    }
    delay_ms(1500);
}

// Password Verification Function with 3-Attempt Lockout
u32 verify_password(void)
{
    u32 entered_pass;
    u8 attempts = 0;
    u32 sec_count = 0;

    while (attempts < 3) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"Enter Password:");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);

        entered_pass = readPassword();

        if (entered_pass == current_pass) {
            WRITE_LCD_CMD(CLEAR_LCD);
            StrLCD((u8*)"Access Granted");
            delay_ms(1200);
            return 1; // Success
        } else {
            attempts++;
            WRITE_LCD_CMD(CLEAR_LCD);
            StrLCD((u8*)"Wrong Password!");
            WRITE_LCD_CMD(GOTO_LINE2_POS0);

            if (attempts < 3) {
                StrLCD((u8*)"Try Again");
                delay_ms(1500);
            }
        }
    }

    // If 3 attempts fail, trigger 10-second lockout with live countdown
    for (sec_count = 10; sec_count > 0; sec_count--) {
        WRITE_LCD_CMD(CLEAR_LCD);
        StrLCD((u8*)"Locked Out!");
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        StrLCD((u8*)"Wait: ");
        U32LCD(sec_count);
        StrLCD((u8*)" sec");
        delay_ms(1000);
    }

    return 0; // Failure: Locked out
}

void monitor_safety_sensors(void)
{
    currentTemp = LM35tC();
    gasValue = MQ2();

    if ((currentTemp > temp_threshold || gasValue > gas_threshold) && !alert_silenced) {
        recordedTemp = currentTemp;
        recordedGasValue = gasValue;
        IOSET0 = 1 << ALLERT_LED;
        IOSET1 = 1 << BUZ_PIN;
    }
    if (flag == 1) {
        alert_silenced = 1;
        IOCLR0 = 1 << ALLERT_LED;
        IOCLR1 = 1 << BUZ_PIN;
        flag = 0;
    }
    if (currentTemp <= temp_threshold && gasValue <= gas_threshold) {
        alert_silenced = 0;
    }
}

int main(void)
{
    u32 sensor_timer = 0;

    system_init();
    displayMSG();

    WRITE_LCD_CMD(CLEAR_LCD);
    while (1) {
        // 1. Read sensors & check safety hazards using helper function
        monitor_safety_sensors();

        // Fetch live time & date from hardware RTC every loop
        GetRTCTimeInfo(&hour, &min, &sec);
        GetRTCDateInfo(&date, &month, &year);

        // 2. Update Live Monitoring Screen
        WRITE_LCD_CMD(GOTO_LINE1_POS0);
        StrLCD((u8*)"T:");
        F32LCD(currentTemp, 1);
        WRITE_LCD_DATA(0xDF);
        StrLCD((u8*)"C G:");
        U32LCD(gasValue);

        // Line 2: HH:MM:SS DD-MM-YY
        WRITE_LCD_CMD(GOTO_LINE2_POS0);
        U32LCD(hour);
        WRITE_LCD_DATA(':');
        U32LCD(min);
        WRITE_LCD_DATA(':');
        U32LCD(sec);
        WRITE_LCD_DATA(' ');
        U32LCD(date);
        WRITE_LCD_DATA('-');
        U32LCD(month);
        WRITE_LCD_DATA('-');
        U32LCD(year);
        WRITE_LCD_DATA(' ');
        StrLCD((u8*)"                ");

        sensor_timer++;
        if (sensor_timer >= 10) {
            sensor_timer = 0;

            WRITE_LCD_CMD(GOTO_LINE1_POS0);
            StrLCD((u8*)"RT:");
            F32LCD(recordedTemp, 1);
            WRITE_LCD_DATA(0xDF);
            StrLCD((u8*)"C RG:");
            U32LCD(recordedGasValue);
            delay_ms(1000);
            WRITE_LCD_CMD(GOTO_LINE1_POS0);
            StrLCD((u8*)"                ");
        }

        if (flag == 1) {
            alert_silenced = 1;
            IOCLR0 = 1 << ALLERT_LED;
            IOCLR1 = 1 << BUZ_PIN;
            flag = 0;
        }
        
        if (currentTemp <= temp_threshold && gasValue <= gas_threshold) {
            alert_silenced = 0;
        }

        // Configuration Menu Triggered via Interrupt
        if (edit_flag == 1) {
            // Verify password before entering any edit block
            if (verify_password() == 1) {
                u32 menu_session_active = 1;
                u32 menu_time = 30; // 30-second countdown timer

                // Draw the main menu header ONCE when entering the session
                WRITE_LCD_CMD(CLEAR_LCD);
                WRITE_LCD_CMD(GOTO_LINE1_POS0);
                StrLCD((u8*)"1.RTC 2.SETPOINT");

                while (menu_session_active && (menu_time > 0)) {
                    u8 choice = 0;

                    // 1. Continuous safety hazard monitoring inside the menu
                    monitor_safety_sensors();

                    // 2. CRITICAL: Always reset cursor to Line 2 before printing the timer
                    WRITE_LCD_CMD(GOTO_LINE2_POS0);
                    StrLCD((u8*)"3.PASS 4.EX T:");
                    U32LCD(menu_time);

                    // Clear trailing characters when transitioning from 10 to single digits
                    if (menu_time < 10) {
                        StrLCD((u8*)"s  ");
                    } else {
                        StrLCD((u8*)"s ");
                    }

                    // 3. Check for key press with 100ms chunked polling for instant response
                    choice = keyscan_nb();

                    if (choice == 0) {
                        u32 poll_idx = 0;
                        while (poll_idx < 10) { // 10 checks * 100ms = 1 second total
                            monitor_safety_sensors(); // Keep safety active while waiting
                            choice = keyscan_nb();
                            if (choice != 0) {
                                break; // Exit polling loop instantly if a key is pressed!
                            }
                            delay_ms(100);
                            poll_idx++;
                        }
                    }

                    // 4. Process the key choice if one was pressed
                    if (choice != 0) {
                        menu_time = 30; // Reset timer back to 30s on valid button press

                        switch (choice) {
                            case '1':
                                rtc_configuration();
                                break;
                            case '2':
                                setpoint_configuration();
                                break;
                            case '3':
                                change_password();
                                break;
                            case '4':
                                menu_session_active = 0;
                                WRITE_LCD_CMD(CLEAR_LCD);
                                StrLCD((u8*)"EXIT");
                                break;
                            default:
                                WRITE_LCD_CMD(CLEAR_LCD);
                                StrLCD((u8*)"INVALID");
                                delay_ms(1000);
                                break;
                        }

                        // Re-draw main menu header after returning from a sub-menu
                        WRITE_LCD_CMD(CLEAR_LCD);
                        WRITE_LCD_CMD(GOTO_LINE1_POS0);
                        StrLCD((u8*)"1.RTC 2.SETPOINT");
                    }
                    else {
                        // If no key was pressed for the full 1 second, decrement countdown timer
                        menu_time--;
                    }
                }

                WRITE_LCD_CMD(CLEAR_LCD);
                StrLCD((u8*)"Session Ended");
                delay_ms(1000);
            }
            edit_flag = 0; // Reset flag when done
        }

        delay_ms(500); // Refresh rate
    }
}
