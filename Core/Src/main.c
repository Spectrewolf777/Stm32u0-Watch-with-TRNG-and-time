/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "rv_3028_c7.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

LCD_HandleTypeDef hlcd;

RNG_HandleTypeDef hrng;

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */
#define DIGIT_BLANK          10   // setNumber() draws nothing for values > 9
#define BTN1_PIN             GPIO_PIN_12  // PB12: random 1..35
#define BTN2_PIN             GPIO_PIN_13  // PB13: random 1..20
#define BTN_DEBOUNCE_MS      30
#define RANDOM_SHOW_SECONDS  10   // how long a random number stays before the clock returns

uint32_t lcd_shadow[16] = {0}; // Fast RAM buffer to track LCD states
/* Colon ("1:00") segment. Spare LCD bits are (6,0) (6,2) (6,4) (6,6);
 * if the wrong dot lights up, try another COLON_BIT from that list. */
#define COLON_REG            6
#define COLON_BIT            0   // (6,2) = dot left of digit 2, so (6,0) should be the colon
bool lcd_colon = false;        // setDisplayValue() adds the colon when true
rv3028_handle_t rtc_handle;        /* RV-3028-C7 RTC handle */
volatile bool rtc_interrupt_fired = true; // Set true initially so it updates on first boot
volatile uint16_t button_flags = 0;       // set by EXTI on PB12/PB13 falling edge
volatile uint8_t  btn_irq_count[2] = {0}; // diagnostic: EXTI hits on PB12 / PB13
#define POWER_TEST 0 // 1 = everything off + STOP2 forever, 2 = MCU in SHUTDOWN (~30 nA): rest = board
#define BUTTONS_ENABLED 1 // 0 = PB12/PB13 analog, no pull-up (exactly like the old 8 uA build)
#define BTN_DIAG 0  // 1 = button test mode (no sleep, live pin view), 0 = normal

uint8_t secondsToFireInt = 2;

void TestIndiviual(){
	// Change these values to map your specific hardware
	uint32_t testRegister = 6; // Try 0, 2, 4, 6
	uint32_t testPinBit = 3;    // Try 0, 1, 2, 3, 4, 5, 6, 7  0 , note 0 starts at 5 and now is 0 2 4 6 of test register

	// This will turn on ONE segment at a time so you can identify it
	HAL_LCD_Write(&hlcd, testRegister, (1UL << testPinBit), (1UL << testPinBit));
	HAL_LCD_UpdateDisplayRequest(&hlcd);
}


void setvalue(uint8_t testRegister, uint8_t testPinBit){
    // ONLY update our fast RAM buffer here. Do NOT write to hardware yet!
    lcd_shadow[testRegister] |= (1UL << testPinBit);
}



void set0(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(2,0);
	    	setvalue(4,0);
	    	setvalue(4,1);
	    	setvalue(6,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(2,2);
	    	setvalue(4,2);
	    	setvalue(4,3);
	    	setvalue(6,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	        // Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(2,4);
	    	setvalue(4,4);
	    	setvalue(4,5);
	    	setvalue(6,5);
	        break;
	    case 4:
	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(2,6);
	    	setvalue(4,6);
	    	setvalue(4,7);
	    	setvalue(6,7);
	        // Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}


void set1(uint8_t pos){
	switch (pos) {
	    // A "1" is drawn on the RIGHT segments b + c (like 4, 7, 9 use them).
	    // Segment map per digit (pos p, k = 2*(p-1)):
	    //   a=(0,k) b=(0,k+1) f=(2,k) g=(2,k+1) e=(4,k) c=(4,k+1) d=(6,k+1)
	    case 1:
	    	setvalue(0,1);   // b
	    	setvalue(4,1);   // c
	        break;
	    case 2:
	    	setvalue(0,3);
	    	setvalue(4,3);
	        break;
	    case 3:
	    	setvalue(0,5);
	    	setvalue(4,5);
	        break;
	    case 4:
	    	setvalue(0,7);
	    	setvalue(4,7);
	        break;
	    default:
	        break;
	}
}


void set2(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(2,1);
	    	setvalue(4,0);
	    	setvalue(6,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(2,3);
	    	setvalue(4,2);
	    	setvalue(6,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	        // Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(2,5);
	    	setvalue(4,4);
	    	setvalue(6,5);
	        break;
	    case 4:

	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(2,7);
	    	setvalue(4,6);
	    	setvalue(6,7);
	        // Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}


}


void set3(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(2,1);
	    	setvalue(4,1);
	    	setvalue(6,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(2,3);
	    	setvalue(4,3);
	    	setvalue(6,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	    	// Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(2,5);
	    	setvalue(4,5);
	    	setvalue(6,5);
	    	break;
	    case 4:

	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(2,7);
	    	setvalue(4,7);
	    	setvalue(6,7);
	    	// Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}


void set4(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(2,0);
	    	setvalue(0,1);
	    	setvalue(2,1);
	    	setvalue(4,1);

	        break;
	    case 2:
	    	setvalue(2,2);
	    	setvalue(0,3);
	    	setvalue(2,3);
	    	setvalue(4,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	    	// Handle State 2 (e.g., PLAYING)
	    	setvalue(2,4);
	    	setvalue(0,5);
	    	setvalue(2,5);
	    	setvalue(4,5);
	    	break;
	    case 4:

	    	setvalue(2,6);
	    	setvalue(0,7);
	    	setvalue(2,7);
	    	setvalue(4,7);
	    	// Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}

void set5(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(2,0);
	    	setvalue(2,1);
	    	setvalue(4,1);
	    	setvalue(6,1);

	    	break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(2,2);
	    	setvalue(2,3);
	    	setvalue(4,3);
	    	setvalue(6,3);
	    	// Handle State 1 (e.g., LOADING)
	    	break;
	    case 3:
	    	// Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(2,4);
	    	setvalue(2,5);
	    	setvalue(4,5);
	    	setvalue(6,5);
	    	break;
	    case 4:

	    	setvalue(0,6);
	    	setvalue(2,6);
	    	setvalue(2,7);
	    	setvalue(4,7);
	    	setvalue(6,7);
	    	// Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}

void set6(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(2,0);
	    	setvalue(2,1);
	    	setvalue(4,0);
	    	setvalue(4,1);
	    	setvalue(6,1);

	    	break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(2,2);
	    	setvalue(2,3);
	    	setvalue(4,2);
	    	setvalue(4,3);
	    	setvalue(6,3);
	    	// Handle State 1 (e.g., LOADING)
	    	break;
	    case 3:
	    	// Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(2,4);
	    	setvalue(2,5);
	    	setvalue(4,4);
	    	setvalue(4,5);
	    	setvalue(6,5);
	    	break;
	    case 4:

	    	setvalue(0,6);
	    	setvalue(2,6);
	    	setvalue(2,7);
	    	setvalue(4,6);
	    	setvalue(4,7);
	    	setvalue(6,7);
	    	// Handle State 3 (e.g., PAUSED)
	    	break;
	    default:
	        // Handle any other state
	        break;
	}
}

void set7(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(4,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(4,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	        // Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(4,5);
	        break;
	    case 4:
	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(4,7);
	        // Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}

void set8(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(2,0);
	    	setvalue(2,1);
	    	setvalue(4,0);
	    	setvalue(4,1);
	    	setvalue(6,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(2,2);
	    	setvalue(2,3);
	    	setvalue(4,2);
	    	setvalue(4,3);
	    	setvalue(6,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	        // Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(2,4);
	    	setvalue(2,5);
	    	setvalue(4,4);
	    	setvalue(4,5);
	    	setvalue(6,5);
	        break;
	    case 4:
	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(2,6);
	    	setvalue(2,7);
	    	setvalue(4,6);
	    	setvalue(4,7);
	    	setvalue(6,7);
	        // Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}

void set9(uint8_t pos){
	switch (pos) {
	    case 1:
	    	setvalue(0,0);
	    	setvalue(0,1);
	    	setvalue(2,0);
	    	setvalue(2,1);
	    	setvalue(4,1);
	    	setvalue(6,1);
	        break;
	    case 2:
	    	setvalue(0,2);
	    	setvalue(0,3);
	    	setvalue(2,2);
	    	setvalue(2,3);
	    	setvalue(4,3);
	    	setvalue(6,3);
	        // Handle State 1 (e.g., LOADING)
	        break;
	    case 3:
	        // Handle State 2 (e.g., PLAYING)
	    	setvalue(0,4);
	    	setvalue(0,5);
	    	setvalue(2,4);
	    	setvalue(2,5);
	    	setvalue(4,5);
	    	setvalue(6,5);
	        break;
	    case 4:
	    	setvalue(0,6);
	    	setvalue(0,7);
	    	setvalue(2,6);
	    	setvalue(2,7);
	    	setvalue(4,7);
	    	setvalue(6,7);
	        // Handle State 3 (e.g., PAUSED)
	        break;
	    default:
	        // Handle any other state
	        break;
	}
}



void setNumber(uint8_t number, uint8_t pos){
	switch (number) {
	    case 0:
	        // Handle 0
	    	set0(pos);


	        break;
	    case 1:
	    	set1(pos);
	        // Handle 1
	        break;
	    case 2:
	    	set2(pos);
	        // Handle 2
	        break;
	    case 3:
	        // Handle 3
	    	set3(pos);
	        break;
	    case 4:
	        // Handle 4
	    	set4(pos);
	        break;
	    case 5:
	        // Handle 5
	    	set5(pos);
	        break;
	    case 6:
	        // Handle 6
	    	set6(pos);
	        break;
	    case 7:
	    	set7(pos);
	        // Handle 7
	        break;
	    case 8:
	    	set8(pos);
	        // Handle 8
	        break;
	    case 9:
	        // Handle 9
	    	set9(pos);
	        break;
	    default:
	        // This handles any number that is NOT 0-9
	        break;
	}


}



void setDisplayValue(uint8_t number1, uint8_t number2, uint8_t number3, uint8_t number4) {
    // 1. CRITICAL: Clear the shadow buffer before starting to assemble the new frame
    for(int i = 0; i < 16; i++) {
        lcd_shadow[i] = 0;
    }

    // 2. Assemble the new segments
    setNumber(number1, 1);
    setNumber(number2, 2);
    setNumber(number3, 3);
    setNumber(number4, 4);
    if (lcd_colon) setvalue(COLON_REG, COLON_BIT);

    // 3. Write the assembled registers to the hardware.
    //    HAL mask = bits to KEEP: RAM = (RAM & mask) | data. Mask 0 replaces the
    //    whole register; 0xFFFFFFFF would only OR new segments on top of old ones.
    HAL_LCD_Write(&hlcd, 0, 0x00000000, lcd_shadow[0]);
    HAL_LCD_Write(&hlcd, 2, 0x00000000, lcd_shadow[2]);
    HAL_LCD_Write(&hlcd, 4, 0x00000000, lcd_shadow[4]);
    HAL_LCD_Write(&hlcd, 6, 0x00000000, lcd_shadow[6]);

    // 4. Update the display
    HAL_LCD_UpdateDisplayRequest(&hlcd);
    while(__HAL_LCD_GET_FLAG(&hlcd, LCD_FLAG_UDR) != RESET) {}
}

void clearScreem(){
	HAL_LCD_Clear(&hlcd);
	  // Clear our software shadow buffer to match the blank screen
	  for(int i = 0; i < 16; i++) {
	        lcd_shadow[i] = 0;
	    }


	   // Wait for hardware clear to finish
	   while(__HAL_LCD_GET_FLAG(&hlcd, LCD_FLAG_UDR) != RESET) {}


}

// Helper to poll EEBUSY status
void wait_for_EEBusy_done(void) {
    uint8_t status = 0;
    uint32_t t0 = HAL_GetTick();
    do {   // EEPROM write takes ~10 ms; never spin forever (CPU awake = high current)
        if (rv3028_read_register(&rtc_handle, 0x0E, &status) != HAL_OK) status = 0x80;
    } while ((status & (1 << 7)) && (HAL_GetTick() - t0) < 100U); // STATUS_EEBUSY (bit 7)
}

void save_backup_config(uint8_t bsm_mode) {
    // 1. Enable EEPROM access: Write 0x00 to 0x27 (ensure EERD is 0)
    rv3028_write_register(&rtc_handle, 0x27, 0x00);

    // 2. Set Backup Register (0x37)
    // Read current first, then modify only the BSM bits
    uint8_t reg37;
    rv3028_read_register(&rtc_handle, 0x37, &reg37);
    reg37 &= ~0x0C; // Clear bits 3:2 (assuming BSM is in these bits)
    reg37 |= (bsm_mode << 2);
    rv3028_write_register(&rtc_handle, 0x37, reg37);

    // 3. Trigger EEPROM update: Write 0x11 to 0x27
    rv3028_write_register(&rtc_handle, 0x27, 0x11);

    // 4. Wait for the chip to finish writing
    wait_for_EEBusy_done();
}

/* Shows HH:MM and returns the current seconds (0..59) so the caller can
 * schedule the next wake-up exactly on the next minute change.
 * No clearScreem() here: setDisplayValue() already rebuilds the whole frame,
 * clearing first only adds a visible blank flash and an extra LCD update. */
uint8_t Display_RTC_Time(bool use_12hr_format) {
    rv3028_time_t now;


    // 1. Get time from RTC
    if (rv3028_get_time(&rtc_handle, &now) == HAL_OK) {
        uint8_t hours = now.hours;
        uint8_t minutes = now.minutes;

        // 2. Handle 12-hour format conversion
        if (use_12hr_format) {
            if (hours == 0) hours = 12;         // 00:xx -> 12:xx
            else if (hours > 12) hours -= 12;  // 13:xx -> 01:xx
        }

        // 3. Extract individual digits for display (e.g., 12:34 -> 1, 2, 3, 4)
        uint8_t d1 = hours / 10;
        uint8_t d2 = hours % 10;
        uint8_t d3 = minutes / 10;
        uint8_t d4 = minutes % 10;

        // Blank the leading zero in 12h mode ( " 1:05" instead of "01:05")
        if (use_12hr_format && d1 == 0) d1 = DIGIT_BLANK;

        // 4. Send to display
        lcd_colon = use_12hr_format;   // "1:00" in 12 h mode
        setDisplayValue(d1, d2, d3, d4);
        lcd_colon = false;             // random numbers etc. without colon
        return (now.seconds < 60) ? now.seconds : 0;
    }
    return 0;
}

/* Wake the MCU from STOP2 after 'seconds' (1..65536) using the internal RTC. */
static void schedule_wakeup(uint32_t seconds) {
    if (seconds == 0) seconds = 1;
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, seconds - 1, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);
}

/* True random number in [min, max] from the hardware TRNG only.
 * Returns 0 if the TRNG fails (callers must use min >= 1).
 * HSI48 + RNG are powered only for the few hundred microseconds this takes,
 * so the sleep current is unaffected. Range 1 is selected temporarily because
 * HSI48 / RNG are specified for voltage range 1. */
void SystemClock_Config(void);
static uint8_t rng_fail_stage = 0;  // 1 = clock/HSI48, 2 = RNG init, 3 = RNG generate

static uint32_t random_range(uint32_t min, uint32_t max) {
    uint32_t span = max - min + 1U;               // max-min < 2^32-1 assumed
    uint32_t limit = 0xFFFFFFFFU - (0xFFFFFFFFU % span); // reject to avoid modulo bias
    uint32_t r = 0;
    bool ok = false;

    /* The RNG cannot run reliably from a 100 kHz AHB clock (48 MHz kernel clock
     * vs. 100 kHz bus), so for these few ms: range 1, MSI 16 MHz, HSI48 on. */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (__HAL_PWR_GET_FLAG(PWR_FLAG_VOSF)) {}  // wait for range 1 before speeding up

    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_HSI48;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_8;     // 16 MHz
    osc.HSI48State          = RCC_HSI48_ON;
    osc.PLL.PLLState        = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        rng_fail_stage = 1;                       // clock / HSI48 did not start
    } else {
        hrng.Instance = RNG;
        hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
        if (HAL_RNG_Init(&hrng) != HAL_OK) {      // MspInit: clock src HSI48 + RNG clock on
            rng_fail_stage = 2;
        } else {
            rng_fail_stage = 3;
            for (int tries = 0; tries < 8; tries++) {
                if (HAL_RNG_GenerateRandomNumber(&hrng, &r) != HAL_OK) break;
                if (r < limit) { ok = true; rng_fail_stage = 0; break; }
            }
        }
        HAL_RNG_DeInit(&hrng);                    // MspDeInit: RNG clock off
    }

    __HAL_RCC_HSI48_DISABLE();
    SystemClock_Config();                         // back to MSI 100 kHz, range 2

    if (!ok) return 0;                            // TRNG only: no software fallback

    return min + (r % span);
}

/* Shows 0..9999 right-aligned, leading zeros blanked (last digit always shown). */
static void Display_Number(uint16_t value) {
    uint8_t d[4] = { value / 1000, (value / 100) % 10, (value / 10) % 10, value % 10 };
    for (int i = 0; i < 3 && d[i] == 0; i++) d[i] = DIGIT_BLANK;
    setDisplayValue(d[0], d[1], d[2], d[3]);
}

/* Shows a TRNG result, or "---N" if the TRNG failed (value 0), N = failing step. */
static void Display_Random(uint32_t value) {
    if (value != 0U) { Display_Number((uint16_t)value); return; }
    for (int i = 0; i < 16; i++) lcd_shadow[i] = 0;
    setvalue(2, 1); setvalue(2, 3); setvalue(2, 5);   // g on digits 1-3
    setNumber(rng_fail_stage, 4);                     // failing step on digit 4
    for (int i = 0; i <= 6; i += 2) HAL_LCD_Write(&hlcd, i, 0x00000000, lcd_shadow[i]);
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/* Sets the RV-3028 to the build time (__DATE__/__TIME__ = local time of the
 * build PC, here Melbourne AEDT) once per build. A 16-bit stamp of the build
 * time is kept in the RTC's battery-backed user RAM so normal resets / power
 * cycles do NOT rewind the clock; only flashing a new build sets it again. */
#define RTC_SET_OFFSET_S  15   // build -> flash -> boot delay to compensate

static uint8_t two(const char *p) { return (uint8_t)((p[0] == ' ' ? 0 : p[0] - '0') * 10 + (p[1] - '0')); }

static void rtc_set_from_build_time(void) {
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__;   // "Nov  7 2026"
    const char *t = __TIME__;   // "13:11:04"

    uint16_t stamp = 0;
    for (const char *c = d; *c; c++) stamp = (uint16_t)(stamp * 31U + (uint8_t)*c);
    for (const char *c = t; *c; c++) stamp = (uint16_t)(stamp * 31U + (uint8_t)*c);

    uint8_t ram[2];
    if (rv3028_read_registers(&rtc_handle, RV3028_REG_USER_RAM1, ram, 2) != HAL_OK) return;
    if (((uint16_t)ram[0] << 8 | ram[1]) == stamp) return;   // this build already set it

    int month = 1;
    for (int i = 0; i < 12; i++) if (d[0] == months[i*3] && d[1] == months[i*3+1] && d[2] == months[i*3+2]) month = i + 1;
    int day = two(d + 4), year = 2000 + two(d + 9);
    int hh = two(t), mm = two(t + 3), ss = two(t + 6) + RTC_SET_OFFSET_S;

    // carry the offset through minutes/hours/day
    static const uint8_t mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    mm += ss / 60; ss %= 60;
    hh += mm / 60; mm %= 60;
    if (hh >= 24) {
        hh -= 24;
        int dim = mdays[month - 1] + (month == 2 && (year % 4) == 0);
        if (++day > dim) { day = 1; if (++month > 12) { month = 1; year++; } }
    }

    // weekday, 1 = Monday .. 7 = Sunday (Sakamoto)
    static const uint8_t k[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    int y = year - (month < 3);
    int wd = (y + y/4 - y/100 + y/400 + k[month - 1] + day) % 7;   // 0 = Sunday
    if (wd == 0) wd = 7;

    rv3028_time_t now = { (uint8_t)ss, (uint8_t)mm, (uint8_t)hh, (uint8_t)wd,
                          (uint8_t)day, (uint8_t)month, (uint8_t)(year - 2000) };
    if (rv3028_set_time(&rtc_handle, &now) == HAL_OK) {
        // Melbourne DST: first Sunday of October 2:00 -> first Sunday of April 3:00.
        // First Sunday of a month = 1 + (7 - weekday_of_the_1st) % 7.
        int wd1 = (wd - 1 - (day - 1) % 7 + 7) % 7 + 1;          // weekday of the 1st, 1=Mon
        int first_sun = 1 + (7 - wd1) % 7;
        bool dst = (month > 10 || month < 4) ||
                   (month == 10 && (day > first_sun || (day == first_sun && hh >= 2))) ||
                   (month == 4  && (day < first_sun || (day == first_sun && hh < 3)));
        uint8_t gp = 0;
        rv3028_read_register(&rtc_handle, RV3028_REG_GPBITS, &gp);
        gp = dst ? (gp | 0x01U) : (gp & ~0x01U);
        rv3028_write_register(&rtc_handle, RV3028_REG_GPBITS, gp & 0x7FU);
        ram[0] = (uint8_t)(stamp >> 8); ram[1] = (uint8_t)stamp;
        rv3028_write_registers(&rtc_handle, RV3028_REG_USER_RAM1, ram, 2);
    }
}

/* ---- Daylight saving (Melbourne: AEST <-> AEDT) --------------------------
 * The DST state lives in the RV-3028 GP bits register (battery backed), bit 0.
 * Holding the PB12 button for DST_HOLD_MS toggles it and shifts the clock 1 h. */
#define DST_HOLD_MS  5000U
#define DST_GPBIT    0x01U

static bool dst_get(void) {
    uint8_t gp = 0;
    rv3028_read_register(&rtc_handle, RV3028_REG_GPBITS, &gp);
    return (gp & DST_GPBIT) != 0U;
}

static void dst_set(bool on) {
    uint8_t gp = 0;
    rv3028_read_register(&rtc_handle, RV3028_REG_GPBITS, &gp);
    gp = on ? (gp | DST_GPBIT) : (gp & ~DST_GPBIT);
    rv3028_write_register(&rtc_handle, RV3028_REG_GPBITS, gp & 0x7FU);
}

/* Shift the RTC by +1 or -1 hour, carrying into date/month/year/weekday. */
static void rtc_shift_hour(int delta) {
    static const uint8_t mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    rv3028_time_t t;
    if (rv3028_get_time(&rtc_handle, &t) != HAL_OK) return;

    int h = (int)t.hours + delta;
    int day = t.date, month = t.month, year = 2000 + t.year, wd = t.weekday;
    if (h >= 24) {
        h -= 24;
        int dim = mdays[month - 1] + (month == 2 && (year % 4) == 0);
        if (++day > dim) { day = 1; if (++month > 12) { month = 1; year++; } }
        if (++wd > 7) wd = 1;
    } else if (h < 0) {
        h += 24;
        if (--day < 1) {
            if (--month < 1) { month = 12; year--; }
            day = mdays[month - 1] + (month == 2 && (year % 4) == 0);
        }
        if (--wd < 1) wd = 7;
    }
    t.hours = (uint8_t)h; t.date = (uint8_t)day; t.month = (uint8_t)month;
    t.year = (uint8_t)(year - 2000); t.weekday = (uint8_t)wd;
    rv3028_set_time(&rtc_handle, &t);
}

/* Toggle daylight saving: on -> clock +1 h, off -> clock -1 h. */
static void dst_toggle(void) {
    bool on = !dst_get();
    rtc_shift_hour(on ? +1 : -1);
    dst_set(on);
}

/* Silence the RV-3028. Its registers are battery backed, so settings from old
 * firmware (e.g. setup_rtc_minute_interrupt) survive: a set TF/UF/AF flag
 * holds the open-drain INT pin LOW forever -> continuous current through the
 * INT pull-up. CLKOUT (factory default ON, 32.768 kHz) also costs current.
 * Register bits per the RV-3028-C7 datasheet. */
static void rtc_quiet(void) {
    uint8_t v;

    // Control 1 (0x0F): stop the countdown timer (TE = bit 2), keep EERD = 0
    if (rv3028_read_register(&rtc_handle, 0x0F, &v) == HAL_OK)
        rv3028_write_register(&rtc_handle, 0x0F, v & ~((1U << 2) | (1U << 3)));

    // Control 2 (0x10): all interrupt enables off (TSE CLKIE UIE TIE AIE EIE),
    // bit 1 = 0 keeps 24 h mode, which the clock code expects
    rv3028_write_register(&rtc_handle, 0x10, 0x00);

    // Status (0x0E): clear CLKF BSF UF TF AF EVF PORF -> releases INT pin
    rv3028_write_register(&rtc_handle, 0x0E, 0x00);

    // CLKOUT (0x35): CLKOE = bit 7. Turn off in RAM and in EEPROM (once),
    // otherwise the RTC's daily EEPROM refresh would turn it back on.
    if (rv3028_read_register(&rtc_handle, 0x35, &v) == HAL_OK && (v & 0x80U)) {
        rv3028_read_register(&rtc_handle, 0x0F, &v);
        rv3028_write_register(&rtc_handle, 0x0F, v | (1U << 3));      // EERD = 1 during access
        wait_for_EEBusy_done();
        rv3028_read_register(&rtc_handle, 0x35, &v);
        rv3028_write_register(&rtc_handle, 0x35, v & ~0x80U);         // CLKOE = 0
        rv3028_write_register(&rtc_handle, 0x27, 0x00);
        rv3028_write_register(&rtc_handle, 0x27, 0x11);               // config RAM -> EEPROM
        wait_for_EEBusy_done();
        rv3028_read_register(&rtc_handle, 0x0F, &v);
        rv3028_write_register(&rtc_handle, 0x0F, v & ~(1U << 3));     // EERD = 0
    }
}

void setup_rtc_minute_interrupt(void) {
    uint8_t reg_val;

    // 1. Stop timer first: Clear TE (bit 3) in Control 1 (0x0F)
    rv3028_read_register(&rtc_handle, 0x0F, &reg_val);
    reg_val &= ~(1 << 3);
    rv3028_write_register(&rtc_handle, 0x0F, reg_val);

    // 2. Clear any existing Timer Flag TF (bit 3) in Status (0x0E)
    rv3028_read_register(&rtc_handle, 0x0E, &reg_val);
    reg_val &= ~(1 << 3);
    rv3028_write_register(&rtc_handle, 0x0E, reg_val);

    // 3. Set Timer Value to 60 (1 Hz clock * 60 = 1 minute)
    rv3028_write_register(&rtc_handle, 0x0A, secondsToFireInt); // Timer Value 0
    rv3028_write_register(&rtc_handle, 0x0B, 0);  // Timer Value 1

    // 4. Enable Timer Interrupt: Set TIE (bit 4) in Control 2 (0x10)
    rv3028_read_register(&rtc_handle, 0x10, &reg_val);
    reg_val |= (1 << 4);
    rv3028_write_register(&rtc_handle, 0x10, reg_val);

    // 5. Start timer: Set TRPT (bit 7 for repeat), TE (bit 3 for enable),
    // and TD to 1Hz (bits 1:0 = 10) in Control 1
    rv3028_read_register(&rtc_handle, 0x0F, &reg_val);
    reg_val |= (1 << 7) | (1 << 3); // Set TRPT and TE
    reg_val &= ~(1 << 0);           // TD0 = 0
    reg_val |= (1 << 1);            // TD1 = 1 (TD = 10 binary is 1Hz)
    rv3028_write_register(&rtc_handle, 0x0F, reg_val);
}



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_LCD_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
void LCD_SetDigit(uint8_t pos, uint8_t value, uint8_t dp);
void LCD_ShowNumber(uint16_t num, uint8_t dpPos);
void Static_Show_1234(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_LCD_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  rv3028_init(&rtc_handle, &hi2c1);
  rtc_set_from_build_time();   // Melbourne time from the build PC, once per build
  rtc_quiet();                 // release RTC INT pin, CLKOUT off
  // 1. Enable Battery Backup Mode (Level Switchover)
  // This writes to EEPROM, you only strictly need to do this once,
  // but running it on boot is fine as long as you aren't resetting constantly.
  //save_backup_config(3);
  // Clear Timer Flag (TF) in RTC

  //setup_rtc_minute_interrupt();

  // BOR/PVD in sampled (ultra-low-power) mode while in STOP2
  HAL_PWREx_EnableUltraLowPowerMode();

  // A debug session leaves "debug in STOP/STANDBY" enabled until a power-on
  // reset; that keeps clocks running in STOP2 (~100+ uA). Turn it off.
  __HAL_RCC_DBGMCU_CLK_ENABLE();
  HAL_DBGMCU_DisableDBGStopMode();
  HAL_DBGMCU_DisableDBGStandbyMode();
  __HAL_RCC_DBGMCU_CLK_DISABLE();

  clearScreem();




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
#if POWER_TEST
  /* Baseline: LCD, I2C, buttons and RTC wake-up off, MCU in STOP2 forever.
   * Whatever current remains is the MCU floor + the rest of the board. */
  HAL_LCD_DeInit(&hlcd);
  HAL_I2C_DeInit(&hi2c1);
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
  GPIO_InitTypeDef g = {0};
  g.Pin = BTN1_PIN | BTN2_PIN; g.Mode = GPIO_MODE_ANALOG; g.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &g);
  HAL_SuspendTick();
#if POWER_TEST == 2
  // Shutdown: MCU core, regulator, RAM all off (only a reset/power cycle exits).
  // Any current still measured is NOT the MCU firmware.
  PWR->SCR = PWR_SCR_CWUF;   // clear wake-up pin flags
  HAL_PWR_EnterSHUTDOWNMode();
#endif
  while (1) HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
#endif
#if BTN_DIAG
  /* Button test: digit1 = PB12 interrupt count, digit2 = PB13 interrupt count,
   * digit3 = PB12 level, digit4 = PB13 level (1 = released, 0 = pressed). */
  while (1) {
      uint8_t l12 = HAL_GPIO_ReadPin(GPIOB, BTN1_PIN) == GPIO_PIN_SET;
      uint8_t l13 = HAL_GPIO_ReadPin(GPIOB, BTN2_PIN) == GPIO_PIN_SET;
      setDisplayValue(btn_irq_count[0] % 10, btn_irq_count[1] % 10, l12, l13);
      HAL_Delay(50);
  }
#endif
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (rtc_interrupt_fired) {
	            rtc_interrupt_fired = false;

	            // Show the time, then sleep until exactly the next minute change
	            // (fixed 60 s from boot would lag the real minute by up to 59 s).
	            uint8_t sec = Display_RTC_Time(true);
	            schedule_wakeup(60U - sec);
	        }

	  if (button_flags) {
	            uint16_t flags = button_flags;
	            button_flags = 0;

	            // Act on the falling edge itself. (Requiring the pin to still be
	            // low after the debounce delay dropped quick taps: wake-up from
	            // STOP2 + clock setup at 100 kHz already eats several ms.)
	            if (flags & BTN1_PIN)      Display_Random(random_range(1, 35));
	            else if (flags & BTN2_PIN) Display_Random(random_range(1, 20));

	            if (flags & (BTN1_PIN | BTN2_PIN)) {
	                // Clock comes back automatically after RANDOM_SHOW_SECONDS
	                schedule_wakeup(RANDOM_SHOW_SECONDS);
	                // Debounce: ignore contact bounce on press and release
	                uint32_t pressed_at = HAL_GetTick();
	                bool dst_done = false;
	                HAL_Delay(BTN_DEBOUNCE_MS);
	                // Wait for release of the pressed button only, max 10 s, so a
	                // stuck/shorted button can never keep the MCU awake.
	                while ((((flags & BTN1_PIN) && HAL_GPIO_ReadPin(GPIOB, BTN1_PIN) == GPIO_PIN_RESET) ||
	                        ((flags & BTN2_PIN) && HAL_GPIO_ReadPin(GPIOB, BTN2_PIN) == GPIO_PIN_RESET)) &&
	                       (HAL_GetTick() - pressed_at) < 10000U) {
	                    // Hold PB12 for 5 s -> toggle daylight saving, show new time
	                    if (!dst_done && (flags & BTN1_PIN) &&
	                        HAL_GPIO_ReadPin(GPIOB, BTN1_PIN) == GPIO_PIN_RESET &&
	                        (HAL_GetTick() - pressed_at) >= DST_HOLD_MS) {
	                        dst_toggle();
	                        Display_RTC_Time(true);
	                        dst_done = true;
	                    }
	                }
	                HAL_Delay(BTN_DEBOUNCE_MS);
	                button_flags = 0;
	                if (dst_done) rtc_interrupt_fired = true;   // redraw + resync to minute
	            }
	        }

	        // ==========================================
	        // Deep Sleep (STOP Mode) Sequence
	        // ==========================================
	        // IRQs masked so a button/RTC event arriving right here is not lost:
	        // WFI still wakes on a pending interrupt, it's serviced after __enable_irq().
	        // SysTick must be stopped BEFORE masking IRQs and any already-pending
	        // tick cleared: a pending SysTick makes WFI return at once, so the MCU
	        // never reached STOP2 and looped awake (~120 uA instead of ~4 uA).
	        HAL_SuspendTick();
	        __disable_irq();
	        SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
	        if (!button_flags && !rtc_interrupt_fired) {
	            // Enter STOP mode. Internal RTC or a button (EXTI) wakes the MCU.
	            HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

	            // ---> MCU WAKES UP HERE <---
	            SystemClock_Config();
	        }
	        HAL_ResumeTick();
	        __enable_irq();


  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000000;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LCD Initialization Function
  * @param None
  * @retval None
  */
static void MX_LCD_Init(void)
{

  /* USER CODE BEGIN LCD_Init 0 */

  /* USER CODE END LCD_Init 0 */

  /* USER CODE BEGIN LCD_Init 1 */

  /* USER CODE END LCD_Init 1 */
  hlcd.Instance = LCD;
  hlcd.Init.Prescaler = LCD_PRESCALER_16;
  hlcd.Init.Divider = LCD_DIVIDER_16;
  hlcd.Init.Duty = LCD_DUTY_1_4;
  hlcd.Init.Bias = LCD_BIAS_1_3;
  hlcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;
  hlcd.Init.Contrast = LCD_CONTRASTLEVEL_1;
  hlcd.Init.DeadTime = LCD_DEADTIME_0;
  hlcd.Init.PulseOnDuration = LCD_PULSEONDURATION_0;
  hlcd.Init.BlinkMode = LCD_BLINKMODE_OFF;
  hlcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV8;
  hlcd.Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;
  if (HAL_LCD_Init(&hlcd) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LCD_Init 2 */

  /* USER CODE END LCD_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 PF3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA4 PA5 PA11
                           PA12 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB11 PB14 PB15
                           PB4 PB5 PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 (buttons to GND, wake from STOP2) */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
#if BUTTONS_ENABLED
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
#else
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
#endif
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // This fires when the internal STM32 RTC wakes the MCU from STOP mode
    rtc_interrupt_fired = true;
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN1_PIN || GPIO_Pin == BTN2_PIN) {
        button_flags |= GPIO_Pin;
        btn_irq_count[GPIO_Pin == BTN2_PIN]++;
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
