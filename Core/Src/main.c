/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "ILI9341_FSMC.h"
#include "XPT2046.h"
#include <stdio.h>
#include <string.h>
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
ADC_HandleTypeDef hadc3;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

HCD_HandleTypeDef hhcd_USB_OTG_FS;

SRAM_HandleTypeDef hsram1;

/* USER CODE BEGIN PV */

/* =============== SİSTEM DƏYİŞƏNLƏRİ =============== */
// Təzyiq dəyişənləri
float main_pressure = 0.0f;        // Hazırki təzyiq (bar)
float main_setpoint = 100.0f;      // Hədəf təzyiq (bar)
float main_tolerance = 5.0f;       // Tolerans (bar)

// Sistem rejimi
typedef enum {
    MODE_SAFE,      // Təhlükəsiz - hər şey bağlı
    MODE_FILLING,   // Dolduruluр - inlet açıq
    MODE_HOLDING,   // Saxlanılır - hər şey bağlı
    MODE_RELEASING  // Boşaldılır - outlet açıq
} SystemMode;
SystemMode system_mode = MODE_SAFE;

// Ekran rejimi
typedef enum {
    SCREEN_MAIN,    // Əsas ekran
    SCREEN_MENU,    // Menyu
    SCREEN_SETPOINT // Setpoint ayarı
} ScreenMode;
ScreenMode screen_mode = SCREEN_MAIN;

// Touch dəyişənləri
uint8_t touch_pressed = 0;
uint8_t last_touch = 0;
uint16_t touch_x = 0, touch_y = 0;

// Yeniləmə sayğacı
uint32_t last_update = 0;
uint8_t need_full_redraw = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FSMC_Init(void);
static void MX_USB_OTG_FS_HCD_Init(void);
static void MX_ADC3_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
void Draw_MainScreen(void);
void Draw_MenuScreen(void);
void Draw_SetpointScreen(void);
void Handle_Touch(void);
void Update_Pressure(void);
void Control_Valves(void);
void Draw_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint16_t color);
uint8_t Button_Pressed(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =============== DÜYMƏ FUNKSİYALARI =============== */
void Draw_Button(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* text, uint16_t color) {
    // Düymə çərçivəsi
    ILI9341_DrawRect(x, y, w, h, color);
    ILI9341_DrawRect(x+1, y+1, w-2, h-2, color);
    // Mətn (ortada)
    uint16_t text_x = x + (w - strlen(text) * 12) / 2;
    uint16_t text_y = y + (h - 16) / 2;
    ILI9341_DrawString(text_x, text_y, text, color, ILI9341_COLOR_BLACK, 2);
}

uint8_t Button_Pressed(uint16_t bx, uint16_t by, uint16_t bw, uint16_t bh) {
    if (!touch_pressed || last_touch) return 0;  // Yalnız yeni basış
    if (touch_x >= bx && touch_x <= bx + bw &&
        touch_y >= by && touch_y <= by + bh) {
        return 1;
    }
    return 0;
}

/* =============== ƏSAS EKRAN =============== */
void Draw_MainScreen(void) {
    if (need_full_redraw) {
        ILI9341_FillScreen(ILI9341_COLOR_BLACK);
        
        // Başlıq
        ILI9341_DrawString(20, 10, "HIGH PRESSURE CONTROL", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
        
        // Aşağıdakı düymələr
        Draw_Button(10, 190, 100, 45, "MENU", ILI9341_COLOR_WHITE);
        Draw_Button(120, 190, 100, 45, "START", ILI9341_COLOR_GREEN);
        Draw_Button(230, 190, 80, 45, "STOP", ILI9341_COLOR_RED);
        
        need_full_redraw = 0;
    }
    
    // Təzyiq dəyəri (böyük)
    char buf[20];
    sprintf(buf, "%5.1f", main_pressure);
    ILI9341_DrawString(60, 55, buf, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 4);
    ILI9341_DrawString(200, 70, "BAR", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    
    // Setpoint
    sprintf(buf, "SP: %3.0f bar", main_setpoint);
    ILI9341_DrawString(20, 120, buf, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 2);
    
    // Status
    const char* status_text;
    uint16_t status_color;
    switch (system_mode) {
        case MODE_SAFE:
            status_text = "SAFE    ";
            status_color = ILI9341_COLOR_GREEN;
            break;
        case MODE_FILLING:
            status_text = "FILLING ";
            status_color = ILI9341_COLOR_YELLOW;
            break;
        case MODE_HOLDING:
            status_text = "HOLDING ";
            status_color = ILI9341_COLOR_CYAN;
            break;
        case MODE_RELEASING:
            status_text = "RELEASE ";
            status_color = ILI9341_COLOR_RED;
            break;
        default:
            status_text = "UNKNOWN ";
            status_color = ILI9341_COLOR_WHITE;
    }
    ILI9341_DrawString(180, 120, status_text, status_color, ILI9341_COLOR_BLACK, 2);
    
    // Proqres bar
    uint16_t bar_width = (uint16_t)((main_pressure / 300.0f) * 280);
    if (bar_width > 280) bar_width = 280;
    ILI9341_FillRect(20, 155, bar_width, 20, ILI9341_COLOR_GREEN);
    ILI9341_FillRect(20 + bar_width, 155, 280 - bar_width, 20, ILI9341_COLOR_DARKGREY);
    ILI9341_DrawRect(20, 155, 280, 20, ILI9341_COLOR_WHITE);
}

/* =============== MENYU EKRANI =============== */
void Draw_MenuScreen(void) {
    if (need_full_redraw) {
        ILI9341_FillScreen(ILI9341_COLOR_BLACK);
        ILI9341_DrawString(100, 10, "MENU", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 3);
        
        Draw_Button(40, 60, 240, 45, "SET PRESSURE", ILI9341_COLOR_CYAN);
        Draw_Button(40, 115, 240, 45, "CALIBRATE", ILI9341_COLOR_YELLOW);
        Draw_Button(40, 170, 240, 45, "BACK", ILI9341_COLOR_WHITE);
        
        need_full_redraw = 0;
    }
}

/* =============== SETPOINT EKRANI =============== */
void Draw_SetpointScreen(void) {
    if (need_full_redraw) {
        ILI9341_FillScreen(ILI9341_COLOR_BLACK);
        ILI9341_DrawString(50, 10, "SET PRESSURE", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
        
        // +/- düymələri
        Draw_Button(30, 60, 60, 50, "-10", ILI9341_COLOR_RED);
        Draw_Button(100, 60, 60, 50, "-1", ILI9341_COLOR_RED);
        Draw_Button(170, 60, 60, 50, "+1", ILI9341_COLOR_GREEN);
        Draw_Button(240, 60, 60, 50, "+10", ILI9341_COLOR_GREEN);
        
        Draw_Button(80, 180, 160, 45, "OK", ILI9341_COLOR_CYAN);
        
        need_full_redraw = 0;
    }
    
    // Setpoint dəyəri
    char buf[20];
    sprintf(buf, "%3.0f BAR  ", main_setpoint);
    ILI9341_DrawString(90, 130, buf, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 3);
}

/* =============== TOUCH İDARƏETMƏSİ =============== */
void Handle_Touch(void) {
    // Touch oxu
    last_touch = touch_pressed;
    
    if (XPT2046_IsTouched()) {
        uint16_t raw_x, raw_y;
        XPT2046_GetTouchPoint(&raw_x, &raw_y);
        
        // Koordinat çevrilməsi (kalibrasiyaya görə)
        touch_x = raw_x;
        touch_y = raw_y;
        touch_pressed = 1;
    } else {
        touch_pressed = 0;
    }
    
    // Ekrana görə touch idarəetməsi
    switch (screen_mode) {
        case SCREEN_MAIN:
            // MENU düyməsi
            if (Button_Pressed(10, 190, 100, 45)) {
                screen_mode = SCREEN_MENU;
                need_full_redraw = 1;
            }
            // START düyməsi
            else if (Button_Pressed(120, 190, 100, 45)) {
                if (system_mode == MODE_SAFE) {
                    system_mode = MODE_FILLING;
                }
            }
            // STOP düyməsi
            else if (Button_Pressed(230, 190, 80, 45)) {
                system_mode = MODE_SAFE;
            }
            break;
            
        case SCREEN_MENU:
            // SET PRESSURE
            if (Button_Pressed(40, 60, 240, 45)) {
                screen_mode = SCREEN_SETPOINT;
                need_full_redraw = 1;
            }
            // BACK
            else if (Button_Pressed(40, 170, 240, 45)) {
                screen_mode = SCREEN_MAIN;
                need_full_redraw = 1;
            }
            break;
            
        case SCREEN_SETPOINT:
            // -10
            if (Button_Pressed(30, 60, 60, 50)) {
                main_setpoint -= 10;
                if (main_setpoint < 0) main_setpoint = 0;
            }
            // -1
            else if (Button_Pressed(100, 60, 60, 50)) {
                main_setpoint -= 1;
                if (main_setpoint < 0) main_setpoint = 0;
            }
            // +1
            else if (Button_Pressed(170, 60, 60, 50)) {
                main_setpoint += 1;
                if (main_setpoint > 300) main_setpoint = 300;
            }
            // +10
            else if (Button_Pressed(240, 60, 60, 50)) {
                main_setpoint += 10;
                if (main_setpoint > 300) main_setpoint = 300;
            }
            // OK
            else if (Button_Pressed(80, 180, 160, 45)) {
                screen_mode = SCREEN_MAIN;
                need_full_redraw = 1;
            }
            break;
    }
}

/* =============== TƏZYİQ YENİLƏMƏSİ =============== */
void Update_Pressure(void) {
    uint16_t adc = 0;
    if (__HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_EOC)) {
        adc = HAL_ADC_GetValue(&hadc3);
    }
    
    // Təzyiq hesabla (0.5-4.5V -> 0-300 bar)
    // ADC: 620 = 0.5V (0 bar), 4095 = 4.5V (300 bar)
    if (adc > 620) {
        main_pressure = (float)(adc - 620) * 300.0f / 3475.0f;
    } else {
        main_pressure = 0.0f;
    }
    if (main_pressure > 300.0f) main_pressure = 300.0f;
    if (main_pressure < 0.0f) main_pressure = 0.0f;
}

/* =============== KLAPAN İDARƏETMƏSİ =============== */
void Control_Valves(void) {
    // TIM3 PWM: CH1=Inlet, CH2=Outlet (0-65535)
    
    switch (system_mode) {
        case MODE_SAFE:
            // Hər şey bağlı
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);      // Inlet bağlı
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);      // Outlet bağlı
            break;
            
        case MODE_FILLING:
            // Inlet açıq, outlet bağlı
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 65535);  // Inlet tam açıq
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);      // Outlet bağlı
            
            // Hədəfə çatdıqda holding rejiminə keç
            if (main_pressure >= main_setpoint) {
                system_mode = MODE_HOLDING;
            }
            break;
            
        case MODE_HOLDING:
            // Hər şey bağlı, təzyiq saxlanılır
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
            
            // Əgər təzyiq düşübsə, yenidən doldur
            if (main_pressure < main_setpoint - main_tolerance) {
                system_mode = MODE_FILLING;
            }
            // Əgər təzyiq çox yüksəkdirsə, boşalt
            if (main_pressure > main_setpoint + main_tolerance * 2) {
                system_mode = MODE_RELEASING;
            }
            break;
            
        case MODE_RELEASING:
            // Outlet açıq, inlet bağlı
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);      // Inlet bağlı
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 65535);  // Outlet tam açıq
            
            // Hədəfə çatdıqda holding rejiminə keç
            if (main_pressure <= main_setpoint) {
                system_mode = MODE_HOLDING;
            }
            break;
    }
}

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
  MX_FSMC_Init();
  MX_USB_OTG_FS_HCD_Init();
  MX_ADC3_Init();
  
  /* KRİTİK DÜZƏLİŞ: ADC-ni Continuous Mode-da başlat */
  /* Continuous mode-da ADC davamlı konversiya edir, Start/Stop lazım deyil */
  HAL_ADC_Start(&hadc3);  // ADC-ni bir dəfə başlat, davamlı işləyəcək
  
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  
  /* LCD başlat */
  HAL_Delay(50);
  ILI9341_Init();
  HAL_Delay(100);
  
  /* AÇILIŞ EKRANI */
  ILI9341_FillScreen(ILI9341_COLOR_BLACK);
  ILI9341_DrawString(40, 60, "HIGH PRESSURE", ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 3);
  ILI9341_DrawString(70, 100, "CONTROL v3.0", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
  ILI9341_DrawString(60, 160, "Touch to start...", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
  HAL_Delay(2000);
  
  /* PWM başlat */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  
  /* Touch başlat */
  XPT2046_Init();
  
  /* Əsas ekranı çək */
  need_full_redraw = 1;
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    /* 1. Təzyiq oxu */
    Update_Pressure();
    
    /* 2. Touch idarəetməsi */
    Handle_Touch();
    
    /* 3. Klapan idarəetməsi */
    Control_Valves();
    
    /* 4. Ekran yenilə */
    switch (screen_mode) {
        case SCREEN_MAIN:
            Draw_MainScreen();
            break;
        case SCREEN_MENU:
            Draw_MenuScreen();
            break;
        case SCREEN_SETPOINT:
            Draw_SetpointScreen();
            break;
    }
    
    HAL_Delay(50);  // 20 FPS
    
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = DISABLE;
  // KRİTİK DÜZƏLİŞ: ADC-ni Continuous Mode-a keçir - ADC bloklanmasının qarşısını almaq üçün
  // Continuous mode-da ADC davamlı konversiya edir, Start/Stop lazım deyil
  hadc3.Init.ContinuousConvMode = ENABLE;  // DÜZƏLİŞ: DISABLE-dan ENABLE-a dəyişdirildi
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 1;
  /* Pressure sensor front-end is high impedance -> need long acquisition time */
  /* QEYD: .ioc faylında ADC_SAMPLETIME_3CYCLES göstərilir, amma kodda 480 cycles istifadə olunur.
   * Bu, sensorun yüksək impedansına görə lazımdır. STM32CubeMX-dən kod yenidən generate edilsə,
   * bu dəyər 3 cycles olacaq və manual olaraq 480 cycles-a dəyişdirilməlidir. */
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;  // 84MHz / 8400 = 10kHz
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;       // 10kHz / 100 = 100Hz (10ms period)
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */
  /* Enable TIM6 interrupt so the 10ms control loop can run */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_HCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hhcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hhcd_USB_OTG_FS.Init.Host_channels = 8;
  hhcd_USB_OTG_FS.Init.speed = HCD_SPEED_FULL;
  hhcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hhcd_USB_OTG_FS.Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  if (HAL_HCD_Init(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Lcd_RST_GPIO_Port, Lcd_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Lcd_LIG_GPIO_Port, Lcd_LIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Lcd_RST_Pin */
  GPIO_InitStruct.Pin = Lcd_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Lcd_LIG_Pin */
  GPIO_InitStruct.Pin = Lcd_LIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_LIG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : XPT2046 SPI pins */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_12, GPIO_PIN_SET);  /* SCK, TCS */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_SET);  /* MISO, MOSI, TIRQ */

  /*Configure GPIO pins : XPT2046 SPI pins */
  GPIO_InitStruct.Pin = GPIO_PIN_1;  /* SCK */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_12;  /* TCS */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8;  /* MISO */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9;  /* MOSI */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10;  /* TIRQ */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */
  
  /* FSMC Clock Enable - ƏN ƏVVƏL! */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */
  
  /* DÜZƏLİŞ: FSMC GPIO pinlərini manual konfiqurasiya etməyə ehtiyac yoxdur!
   * HAL_SRAM_MspInit() funksiyası stm32f4xx_hal_msp.c faylında 
   * bütün FSMC GPIO pinlərini avtomatik konfiqurasiya edir.
   * Dublikat konfiqurasiya problemlərə səbəb ola bilər.
   * 
   * Əgər manual konfiqurasiya lazımdırsa, yalnız .ioc faylında 
   * göstərilən pinləri konfiqurasiya edin (PD11 istifadə olunmur!)
   */

  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK4;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hsram1.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 5;
  Timing.BusTurnAroundDuration = 1;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  /* ExtTiming */

  if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FSMC_Init 2 */

  /* USER CODE END FSMC_Init 2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief Timer Period Elapsed Callback
 * @param htim: Timer handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        /* DÜZƏLİŞ: PID Step deaktiv - sadə mode üçün */
        /* AdvancedPressureControl_Step(); */
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
