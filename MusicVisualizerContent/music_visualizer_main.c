#include <xc.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define _XTAL_FREQ 64000000UL

#define FAN         LATAbits.LATA5
#define PEAK_LED    LATCbits.LATC3

// RGB LED #1
#define RGB_RED     LATEbits.LATE0
#define RGB_GREEN   LATEbits.LATE1
#define RGB_BLUE    LATCbits.LATC2

// RGB LED #2
#define RGB2_RED    LATCbits.LATC4
#define RGB2_GREEN  LATCbits.LATC5
#define RGB2_BLUE   LATCbits.LATC6

// RGB LED #3
#define RGB3_RED    LATAbits.LATA4
#define RGB3_GREEN  LATAbits.LATA6
#define RGB3_BLUE   LATAbits.LATA3

#define LCD_RS LATDbits.LATD0
#define LCD_E  LATDbits.LATD1
#define LCD_DATA LATB

void OSC_Init(void);
void GPIO_Init(void);
void ADC_Init(void);

uint16_t ADC_Read_Channel(uint8_t channel);
uint16_t ADC_Read_RA0(void);
uint16_t ADC_Read_RA1(void);
uint16_t calibrate_baseline(void);
uint16_t get_sound_level(uint16_t baseline);
uint16_t apply_sensitivity(uint16_t level, uint16_t pot);
uint8_t level_to_percent(uint16_t level);

void RGB_Off(void);
void RGB_Set_Level(uint16_t level);
uint8_t get_fan_duty(uint16_t level);
void software_pwm_fan(uint8_t duty);

void LCD_Pulse(void);
void LCD_Command(uint8_t cmd);
void LCD_Char(char data);
void LCD_Init(void);
void LCD_String(const char *str);
void LCD_Set_Cursor(uint8_t row, uint8_t col);
void LCD_Update(uint16_t level, uint8_t duty, uint16_t pot);

void main(void)
{
    OSC_Init();
    GPIO_Init();
    ADC_Init();
    LCD_Init();

    uint16_t baseline = calibrate_baseline();

    LCD_Command(0x01);
    __delay_ms(2);

    LCD_Update(0, 0, ADC_Read_RA1());

    uint8_t lcd_counter = 0;

    while(1)
    {
        uint16_t raw_level = get_sound_level(baseline);
        uint16_t pot = ADC_Read_RA1();
        uint16_t level = apply_sensitivity(raw_level, pot);
        uint8_t duty = get_fan_duty(level);

        RGB_Set_Level(level);

        lcd_counter++;
        if(lcd_counter >= 15)
        {
            LCD_Update(level, duty, pot);
            lcd_counter = 0;
        }

        software_pwm_fan(duty);
    }
}

void OSC_Init(void)
{
    OSCCON1 = 0x60;
    OSCFRQ = 0x08;
}

void GPIO_Init(void)
{
    // RA0 = mic analog, RA1 = pot analog
    // RA3/RA4/RA6 = RGB #3, RA5 = fan
    ANSELA = 0x03;
    TRISA  = 0x03;
    LATA = 0x00;

    ANSELB = 0x00;
    TRISB = 0x00;
    LATB = 0x00;

    ANSELCbits.ANSELC2 = 0;
    ANSELCbits.ANSELC3 = 0;
    ANSELCbits.ANSELC4 = 0;
    ANSELCbits.ANSELC5 = 0;
    ANSELCbits.ANSELC6 = 0;

    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC3 = 0;
    TRISCbits.TRISC4 = 0;
    TRISCbits.TRISC5 = 0;
    TRISCbits.TRISC6 = 0;

    LATCbits.LATC2 = 0;
    LATCbits.LATC3 = 0;
    LATCbits.LATC4 = 0;
    LATCbits.LATC5 = 0;
    LATCbits.LATC6 = 0;

    ANSELD = 0x00;
    TRISD = 0x00;
    LATD = 0x00;

    ANSELEbits.ANSELE0 = 0;
    ANSELEbits.ANSELE1 = 0;
    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    LATEbits.LATE0 = 0;
    LATEbits.LATE1 = 0;
}

void ADC_Init(void)
{
    ADREF = 0x00;
    ADCLK = 0x3F;
    ADACQ = 0x20;

    ADCON0bits.FM = 1;
    ADCON0bits.CS = 0;
    ADCON0bits.ON = 1;
}

uint16_t ADC_Read_Channel(uint8_t channel)
{
    ADPCH = channel;
    __delay_us(10);

    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);

    return ((uint16_t)ADRESH << 8) | ADRESL;
}

uint16_t ADC_Read_RA0(void)
{
    return ADC_Read_Channel(0x00);
}

uint16_t ADC_Read_RA1(void)
{
    return ADC_Read_Channel(0x01);
}

uint16_t calibrate_baseline(void)
{
    uint32_t sum = 0;

    for(uint8_t i = 0; i < 64; i++)
    {
        sum += ADC_Read_RA0();
        __delay_ms(5);
    }

    return sum / 64;
}

uint16_t get_sound_level(uint16_t baseline)
{
    uint16_t max_level = 0;

    for(uint8_t i = 0; i < 64; i++)
    {
        uint16_t adc = ADC_Read_RA0();
        uint16_t diff;

        if(adc > baseline)
            diff = adc - baseline;
        else
            diff = baseline - adc;

        if(diff > max_level)
            max_level = diff;
    }

    return max_level;
}

uint16_t apply_sensitivity(uint16_t level, uint16_t pot)
{
    uint16_t gain = 1 + (pot / 341);

    level = level * gain;

    if(level > 999)
        level = 999;

    return level;
}

uint8_t level_to_percent(uint16_t level)
{
    if(level >= 100)
        return 100;

    return (uint8_t)level;
}

void RGB_Off(void)
{
    RGB_RED = 0;
    RGB_GREEN = 0;
    RGB_BLUE = 0;

    RGB2_RED = 0;
    RGB2_GREEN = 0;
    RGB2_BLUE = 0;

    RGB3_RED = 0;
    RGB3_GREEN = 0;
    RGB3_BLUE = 0;
}

void RGB_Set_Level(uint16_t level)
{
    static uint8_t peak_hold = 0;
    static uint8_t wave_step = 0;
    static uint8_t wave_timer = 0;

    RGB_Off();
    PEAK_LED = 0;

    // White peak trigger
    if(level >= 125)
    {
        peak_hold = 6;
    }

    // White flash on strongest peaks
    if(peak_hold > 0)
    {
        RGB_RED = 1;
        RGB_GREEN = 1;
        RGB_BLUE = 1;

        RGB2_RED = 1;
        RGB2_GREEN = 1;
        RGB2_BLUE = 1;

        RGB3_RED = 1;
        RGB3_GREEN = 1;
        RGB3_BLUE = 1;

        PEAK_LED = 1;
        peak_hold--;
        return;
    }

    // Wave speed based on volume range
    wave_timer++;

    if(level < 25)
    {
        if(wave_timer >= 12)
        {
            wave_step++;
            wave_timer = 0;
        }
    }
    else if(level < 45)
    {
        if(wave_timer >= 9)
        {
            wave_step++;
            wave_timer = 0;
        }
    }
    else if(level < 70)
    {
        if(wave_timer >= 7)
        {
            wave_step++;
            wave_timer = 0;
        }
    }
    else if(level < 90)
    {
        if(wave_timer >= 5)
        {
            wave_step++;
            wave_timer = 0;
        }
    }
    else
    {
        if(wave_timer >= 3)
        {
            wave_step++;
            wave_timer = 0;
        }
    }

    if(wave_step > 2)
        wave_step = 0;

    // Very low: blue wave
    if(level < 25)
    {
        if(wave_step == 0)
        {
            RGB_BLUE = 1;
            RGB2_BLUE = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_BLUE = 1;
            RGB3_BLUE = 1;
        }
        else
        {
            RGB3_BLUE = 1;
            RGB_BLUE = 1;
        }
    }

    // Low-medium: cyan wave
    else if(level < 45)
    {
        if(wave_step == 0)
        {
            RGB_GREEN = 1;
            RGB_BLUE = 1;

            RGB2_GREEN = 1;
            RGB2_BLUE = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_GREEN = 1;
            RGB2_BLUE = 1;

            RGB3_GREEN = 1;
            RGB3_BLUE = 1;
        }
        else
        {
            RGB3_GREEN = 1;
            RGB3_BLUE = 1;

            RGB_GREEN = 1;
            RGB_BLUE = 1;
        }
    }

    // Medium: green wave
    else if(level < 70)
    {
        if(wave_step == 0)
        {
            RGB_GREEN = 1;
            RGB2_GREEN = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_GREEN = 1;
            RGB3_GREEN = 1;
        }
        else
        {
            RGB3_GREEN = 1;
            RGB_GREEN = 1;
        }
    }

    // Medium-loud: yellow wave
    else if(level < 90)
    {
        if(wave_step == 0)
        {
            RGB_RED = 1;
            RGB_GREEN = 1;

            RGB2_RED = 1;
            RGB2_GREEN = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_RED = 1;
            RGB2_GREEN = 1;

            RGB3_RED = 1;
            RGB3_GREEN = 1;
        }
        else
        {
            RGB3_RED = 1;
            RGB3_GREEN = 1;

            RGB_RED = 1;
            RGB_GREEN = 1;
        }
    }

    // High energy: magenta/purple wave
    else if(level < 110)
    {
        if(wave_step == 0)
        {
            RGB_RED = 1;
            RGB_BLUE = 1;

            RGB2_RED = 1;
            RGB2_BLUE = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_RED = 1;
            RGB2_BLUE = 1;

            RGB3_RED = 1;
            RGB3_BLUE = 1;
        }
        else
        {
            RGB3_RED = 1;
            RGB3_BLUE = 1;

            RGB_RED = 1;
            RGB_BLUE = 1;
        }
    }

    // Very loud: red chase before white peak
    else
    {
        if(wave_step == 0)
        {
            RGB_RED = 1;
        }
        else if(wave_step == 1)
        {
            RGB2_RED = 1;
        }
        else
        {
            RGB3_RED = 1;
        }
    }
}
uint8_t get_fan_duty(uint16_t level)
{
    static uint8_t burst_hold = 0;
    static uint8_t current_duty = 0;
    uint8_t target_duty;

    // Trigger fan burst on strong peaks
    if(level >= 105)
    {
        burst_hold = 20;   // longer burst so you can hear throttle kick
    }

    if(burst_hold > 0)
    {
        burst_hold--;
        target_duty = 100;
    }
    else if(level < 25)
    {
        target_duty = 0;
    }
    else if(level < 55)
    {
        target_duty = 31;
    }
    else if(level < 75)
    {
        target_duty = 55;
    }
    else
    {
        target_duty = 80;
    }

    // Smooth ramping
    if(current_duty < target_duty)
    {
        current_duty += 10;   // faster ramp-up for audible burst

        if(current_duty > target_duty)
            current_duty = target_duty;
    }
    else if(current_duty > target_duty)
    {
        current_duty -= 3;    // slower ramp-down

        if(current_duty < target_duty)
            current_duty = target_duty;
    }

    return current_duty;
}

void software_pwm_fan(uint8_t duty)
{
    for(uint8_t cycle = 0; cycle < 4; cycle++)
    {
        for(uint8_t step = 0; step < 100; step++)
        {
            if(step < duty)
                FAN = 1;
            else
                FAN = 0;

            __delay_us(50);
        }
    }
}

void LCD_Pulse(void)
{
    LCD_E = 1;
    __delay_us(5);
    LCD_E = 0;
    __delay_ms(1);
}

void LCD_Command(uint8_t cmd)
{
    LCD_RS = 0;
    LCD_DATA = cmd;
    LCD_Pulse();

    __delay_ms(3);
}

void LCD_Char(char data)
{
    LCD_RS = 1;
    LCD_DATA = data;
    LCD_Pulse();

    __delay_ms(1);
}

void LCD_Init(void)
{
    __delay_ms(20);

    LCD_Command(0x38);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    __delay_ms(2);
}

void LCD_String(const char *str)
{
    while(*str)
    {
        LCD_Char(*str++);
    }
}

void LCD_Set_Cursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if(row == 1)
        address = 0x80 + (col - 1);
    else
        address = 0xC0 + (col - 1);

    LCD_Command(address);
}

void LCD_Update(uint16_t level, uint8_t duty, uint16_t pot)
{
    char buffer[17];
    uint8_t vol_percent = level_to_percent(level);
    uint8_t sens_percent = (uint8_t)((pot * 100UL) / 1023UL);

    LCD_Set_Cursor(1, 1);
    sprintf(buffer, "VOL:%03u%% FAN:%03u%%", vol_percent, duty);
    LCD_String(buffer);

    LCD_Set_Cursor(2, 1);
    sprintf(buffer, "SENS:%03u%%       ", sens_percent);
    LCD_String(buffer);
}
