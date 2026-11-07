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

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */
uint32_t lcd_shadow[16] = {0}; // Fast RAM buffer to track LCD states
rv3028_handle_t rtc_handle;        /* RV-3028-C7 RTC handle */
volatile bool rtc_interrupt_fired = true; // Set true initially so it updates on first boot

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
	    case 1:
	    	setvalue(2,0);   // f
	    	setvalue(4,0);   // e
	        break;
	    case 2:
	    	setvalue(2,2);
	    	setvalue(4,2);
	        break;
	    case 3:
	    	setvalue(2,4);
	    	setvalue(4,4);
	        break;
	    case 4:
	    	setvalue(2,6);
	    	setvalue(4,6);
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

    // 3. Write the assembled registers to the hardware
    HAL_LCD_Write(&hlcd, 0, 0xFFFFFFFF, lcd_shadow[0]);
    HAL_LCD_Write(&hlcd, 2, 0xFFFFFFFF, lcd_shadow[2]);
    HAL_LCD_Write(&hlcd, 4, 0xFFFFFFFF, lcd_shadow[4]);
    HAL_LCD_Write(&hlcd, 6, 0xFFFFFFFF, lcd_shadow[6]);

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
    do {
        rv3028_read_register(&rtc_handle, 0x0E, &status);
    } while (status & (1 << 7)); // Poll STATUS_EEBUSY (bit 7)
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

void Display_RTC_Time(bool use_12hr_format) {
	clearScreem();
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

        // 4. Send to display
        setDisplayValue(d1, d2, d3, d4);
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
  // 1. Enable Battery Backup Mode (Level Switchover)
  // This writes to EEPROM, you only strictly need to do this once,
  // but running it on boot is fine as long as you aren't resetting constantly.
  //save_backup_config(3);
  // Clear Timer Flag (TF) in RTC

  //setup_rtc_minute_interrupt();
  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);


  clearScreem();




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (rtc_interrupt_fired) {
	            rtc_interrupt_fired = false;

	            // No need to clear the RV-3028 timer flags anymore!
	            // Just fetch the time and update your LCD.
	            Display_RTC_Time(true);
	        }

	        // ==========================================
	        // Deep Sleep (STOP Mode) Sequence
	        // ==========================================
	        HAL_SuspendTick();

	        // Enter STOP mode. The Internal RTC will wake the MCU up.
	        HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

	        // ---> MCU WAKES UP HERE <---

	        // 1. Reconfigure the main system clock immediately after wake
	        SystemClock_Config();

	        // 2. Resume SysTick
	        HAL_ResumeTick();


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

  /*Configure GPIO pins : PB10 PB11 PB12 PB13
                           PB14 PB15 PB4 PB5
                           PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // This fires when the internal STM32 RTC wakes the MCU from STOP mode
    rtc_interrupt_fired = true;
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
