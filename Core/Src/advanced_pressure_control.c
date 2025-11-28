/*
 * Advanced PID-based Pressure Control System for STM32F407ZGT6
 * Valeh Injection System - Complete Control Solution
 * 
 * Implementation of the complete PID control system with:
 * - ZME and DRV valve control
 * - Motor speed management
 * - Safety systems
 * - Calibration support
 */

#include "advanced_pressure_control.h"
#include "pressure_control_config.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* External handles from main.c */
extern ADC_HandleTypeDef hadc3;
extern TIM_HandleTypeDef htim3;

/* External functions from pressure_control_config.c */
extern System_Config_t g_system_config;
extern PID_Tuning_t g_pid_zme_tuning;
extern PID_Tuning_t g_pid_drv_tuning;
extern Calibration_Data_t g_calibration_data;
extern Valve_Config_t g_valve_config;
extern Safety_Config_t g_safety_config;

/* =========================================================================
   V. GLOBAL DƏYİŞƏNLƏR
   ========================================================================= */

// Main system status
SystemStatus_t g_system_status = {0};

// PID Controllers
PID_Controller_t g_pid_zme = {0};
PID_Controller_t g_pid_drv = {0};

// Calibration data
// DÜZƏLİŞ: Offset düzgün hesablanmalıdır
CalibrationData_t g_calibration = {
    .adc_min = (float)ADC_MIN,
    .adc_max = (float)ADC_MAX,
    .pressure_min = PRESSURE_MIN,
    .pressure_max = PRESSURE_MAX,
    .slope = PRESSURE_SLOPE,
    .offset = PRESSURE_MIN - (PRESSURE_SLOPE * (float)ADC_MIN),  // DÜZƏLİŞ: Offset hesablanır
    .calibrated = true,
    .calibration_date = 0
};

// Safety limits
SafetyLimits_t g_safety_limits = {
    .max_pressure = 300.0f,
    .over_limit_margin = PRESSURE_OVER_LIMIT_MARGIN,
    .safety_enabled = true
};

// Control timing
static bool g_control_initialized = false;

// Təkmilləşdirilmiş PID Parametrləri (Kalibrasiya Ekranından dəyişdirilə bilər)
static float g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;
static float g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;

/* =========================================================================
   HELPER VALIDATION FUNCTIONS
   ========================================================================= */

static bool AdvancedPressureControl_IsCalibrationRangeValid(uint16_t adc_min_val,
                                                            uint16_t adc_max_val,
                                                            float pressure_min_val,
                                                            float pressure_max_val)
{
    if (adc_min_val >= adc_max_val) {
        return false;
    }

    if (adc_min_val < 50U || adc_max_val > 4095U) {
        return false;
    }

    if ((adc_max_val - adc_min_val) < 200U) {
        return false;
    }

    if (!isfinite(pressure_min_val) || !isfinite(pressure_max_val)) {
        return false;
    }

    if (pressure_max_val <= pressure_min_val) {
        return false;
    }

    return true;
}

/* =========================================================================
   VI. KÖMƏKÇİ FUNKSİYALAR (Hardware Abstraction Layer - HAL)
   ========================================================================= */

/**
 * @brief ADC-dən xam dəyəri oxu
 * @retval Raw ADC value (0-4095)
 *
 * Continuous mode-da ADC davamlı işləyir, ona görə də dəyəri ən son tamamlanmış
 * konversiyadan birbaşa oxumaq kifayətdir. EOC flaqına güvənmək əvəzinə dəyəri
 * həmişə oxuyuruq və yalnız konversiya baş verməyibsə əvvəlki etibarlı dəyəri
 * qaytarırıq.
 */
uint16_t AdvancedPressureControl_ReadADC(void) {
    static uint16_t last_valid_adc = ADC_MIN;
    static uint32_t error_count = 0;
    static uint32_t debug_count = 0;
    static uint16_t last_read_value = 0;
    static uint32_t same_value_count = 0;
    debug_count++;

    /* KRİTİK DÜZƏLİŞ: Continuous mode-da ADC davamlı konversiya edir
     * Əsas problem: Dəyəri oxuyub EOC flag-i təmizlədikdən sonra, yeni konversiyanın
     * tamamlanmasını gözləmək lazımdır. Əks halda eyni dəyər oxuna bilər.
     * 
     * Düzgün məntiq:
     * 1. EOC flag-inin qalxmasını gözlə (yeni konversiya tamamlanıb)
     * 2. Dəyəri oxu
     * 3. EOC flag-i təmizlə
     * 4. Növbəti konversiyanın başlaması üçün qısa gözlə (continuous mode avtomatik başlayır)
     */
    
    // Overrun baş veribsə flaqı təmizlə ki, növbəti konversiya bloklanmasın
    if (__HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_OVR) != RESET) {
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_OVR);
        error_count++;
        if (error_count < 10 || (error_count % 100 == 0)) {
            printf("WARNING[%lu]: ADC Overrun detected and cleared\r\n", debug_count);
        }
    }

    /* KRİTİK DÜZƏLİŞ: Continuous mode-da yeni konversiyanın tamamlanmasını gözlə
     * EOC flag-inin qalxması yeni dəyərin hazır olduğunu göstərir */
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout_ms = 50;  // DÜZƏLİŞ: Continuous mode üçün daha uzun timeout (50ms)
    
    // EOC flag-inin qalxmasını gözlə (yeni konversiya tamamlanıb)
    while (__HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_EOC) == RESET) {
        if ((HAL_GetTick() - start_time) >= timeout_ms) {
            error_count++;
            uint32_t adc_state = HAL_ADC_GetState(&hadc3);
            
            // ADC-nin işlədiyini yoxla
            if ((adc_state & HAL_ADC_STATE_REG_BUSY) == 0U) {
                // ADC dayanıb - yenidən başlat
                if (error_count < 10 || (error_count % 100 == 0)) {
                    printf("ERROR[%lu]: ADC stopped (state=0x%08lX), restarting...\r\n", 
                           debug_count, adc_state);
                }
                HAL_ADC_Stop(&hadc3);
                HAL_Delay(5);
                if (HAL_ADC_Start(&hadc3) != HAL_OK) {
                    if (error_count < 10 || (error_count % 100 == 0)) {
                        printf("ERROR[%lu]: ADC Start failed after timeout\r\n", debug_count);
                    }
                    return last_valid_adc;
                }
                // İlk konversiyanın başlaması üçün gözlə
                HAL_Delay(10);
                // Yenidən yoxla
                start_time = HAL_GetTick();
                continue;
            }
            
            if (error_count < 10 || (error_count % 100 == 0)) {
                printf("ERROR[%lu]: ADC EOC timeout (waited %lu ms), state=0x%08lX\r\n", 
                       debug_count, timeout_ms, adc_state);
            }
            return last_valid_adc;
        }
        // CPU-nu bloklamamaq üçün qısa gecikmə
        for(volatile uint32_t i = 0; i < 1000; i++);  // ~1-2μs delay
    }

    /* EOC flag qalxıb - dəyəri oxu və flag-i təmizlə */
    uint16_t adc_value = (uint16_t)HAL_ADC_GetValue(&hadc3);
    __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_EOC);
    
    /* KRİTİK DÜZƏLİŞ: Continuous mode-da növbəti konversiyanın başlaması üçün
     * qısa gözlə. Bu, ADC-nin yeni konversiyaya başlamasına imkan verir.
     * Continuous mode avtomatik başlayır, amma qısa gecikmə daha etibarlıdır. */
    // DÜZƏLİŞ: 100-dən 1000-ə artırıldı - ADC-nin yeni konversiyaya başlaması üçün daha çox vaxt
    for(volatile uint32_t i = 0; i < 1000; i++);  // ~1-2μs delay
    
    // KRİTİK DÜZƏLİŞ: Eyni dəyərin ardıcıl oxunmasını yoxla
    // Continuous mode-da düzgün gözləmə ilə bu nadir hallarda baş verməlidir
    // Amma sensor həqiqətən dəyişmirsə (məsələn, təzyiq sabitdirsə), bu normaldır
    if (adc_value == last_read_value && debug_count > 3) {
        same_value_count++;
        // DÜZƏLİŞ: Həddindən artıq stuck value zamanı ADC-ni restart et
        if (same_value_count > 500) {  // DÜZƏLİŞ: 100-dən 500-ə artırıldı
            printf("CRITICAL[%lu]: ADC value %u stuck for %lu reads - RESTARTING ADC!\r\n", 
                   debug_count, adc_value, same_value_count);
            // ADC-ni restart et
            HAL_ADC_Stop(&hadc3);
            HAL_Delay(10);
            if (HAL_ADC_Start(&hadc3) == HAL_OK) {
                printf("INFO[%lu]: ADC restarted successfully\r\n", debug_count);
            } else {
                printf("ERROR[%lu]: ADC restart failed!\r\n", debug_count);
            }
            HAL_Delay(20);  // ADC-nin hazır olması üçün
            same_value_count = 0;  // Counter sıfırla
        } else if (same_value_count == 100 || same_value_count == 200 || same_value_count == 300 || same_value_count == 400) {
            // Hər 100 oxunuşda bir xəbərdarlıq ver
            printf("WARNING[%lu]: ADC value %u unchanged for %lu reads (checking if stuck...)\r\n", 
                   debug_count, adc_value, same_value_count);
        }
    } else {
        // Dəyər dəyişdi, counter-i sıfırla
        if (same_value_count > 10) {  // Yalnız əhəmiyyətli dəyişiklik zamanı məlumat ver
            printf("INFO[%lu]: ADC value changed from %u to %u (was stable for %lu reads)\r\n", 
                   debug_count, last_read_value, adc_value, same_value_count);
        }
        same_value_count = 0;
    }
    last_read_value = adc_value;
    
    /* KRİTİK DÜZƏLİŞ: 12-bit ADC maksimum dəyəri 4095-dir (2^12 - 1)
     * Bəzən HAL_ADC_GetValue() qeyri-etibarlı dəyərlər qaytara bilər
     * ADC dəyərini 0-4095 diapazonunda clamp et */
    if (adc_value > 4095U) {
        // Debug: Qeyri-etibarlı ADC dəyəri aşkarlandı
        error_count++;
        if (error_count < 10 || (error_count % 100 == 0)) {
            printf("WARNING[%lu]: Invalid ADC value detected: %u (> 4095), clamping to 4095\r\n", 
                   debug_count, adc_value);
        }
        adc_value = 4095U;  // Maksimum 12-bit ADC dəyəri
    }

    /* KRİTİK DÜZƏLİŞ: ADC 0 dəyəri yoxlaması - sensor spesifikasiyasına görə minimum ADC_MIN (620) olmalıdır
     * Amma əgər ADC həqiqətən 0 oxuyursa, bu hardware problemi ola bilər
     * Bu halda, yalnız ardıcıl 0 oxunuşları zamanı last_valid_adc istifadə et */
    static uint32_t zero_count = 0;
    if (adc_value == 0U) {
        zero_count++;
        error_count++;
        if (zero_count < 10 || (zero_count % 100 == 0)) {
            printf("WARNING[%lu]: ADC reading is 0 (count=%lu, sensor disconnected?), state=0x%08lX, last_valid=%u\r\n", 
                   debug_count, zero_count, HAL_ADC_GetState(&hadc3), last_valid_adc);
        }
        // KRİTİK: Əgər ardıcıl çoxlu 0 oxunuşu varsa, ADC-ni yenidən başlat
        if (zero_count > 50) {
            printf("ERROR[%lu]: Too many consecutive 0 readings, restarting ADC\r\n", debug_count);
            HAL_ADC_Stop(&hadc3);
            HAL_Delay(5);
            HAL_ADC_Start(&hadc3);
            zero_count = 0;
        }
        return last_valid_adc;
    } else {
        // Uğurlu oxunuş - zero_count-u sıfırla
        if (zero_count > 0) {
            printf("INFO[%lu]: ADC recovered from 0 readings (was stuck for %lu cycles)\r\n", 
                   debug_count, zero_count);
        }
        zero_count = 0;
    }

    /* Uğurlu oxunuş - error_count-u sıfırla */
    if (adc_value > 0U && adc_value <= 4095U) {
        if (error_count > 0) {
            error_count = 0;  // Uğurlu oxunuş - error_count-u sıfırla
        }
    }

    // DEBUG: İlk bir neçə oxunuşda və hər 100 oxunuşda bir dəfə göstər (daha tez-tez)
    // Həmçinin stuck value aşkarlandıqda da göstər
    if (debug_count <= 20 || (debug_count % 100 == 0) || same_value_count > 0) {
        printf("DEBUG ADC[%lu]: value=%u, state=0x%08lX, last_valid=%u, same_count=%lu\r\n", 
               debug_count, adc_value, HAL_ADC_GetState(&hadc3), last_valid_adc, same_value_count);
    }

    last_valid_adc = adc_value;
    return adc_value;
}

/**
 * @brief ZME PWM çıxışını təyin et
 * @param percent: Duty cycle 0-30%
 */
void AdvancedPressureControl_SetZME_PWM(float percent) {
    percent = AdvancedPressureControl_ClampValue(percent, ZME_PWM_MIN, ZME_PWM_MAX);
    uint32_t compare_value = (uint32_t)((percent / 100.0f) * __HAL_TIM_GET_AUTORELOAD(&htim3));
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare_value);
}

/**
 * @brief DRV PWM çıxışını təyin et
 * @param percent: Duty cycle 0-40%
 */
void AdvancedPressureControl_SetDRV_PWM(float percent) {
    percent = AdvancedPressureControl_ClampValue(percent, DRV_PWM_MIN, DRV_PWM_MAX);
    uint32_t compare_value = (uint32_t)((percent / 100.0f) * __HAL_TIM_GET_AUTORELOAD(&htim3));
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compare_value);
}

/**
 * @brief Motor PWM çıxışını təyin et
 * @param percent: Duty cycle 0-100%
 */
void AdvancedPressureControl_SetMotor_PWM(float percent) {
    percent = AdvancedPressureControl_ClampValue(percent, 0.0f, MOTOR_MAX_PWM);
    uint32_t compare_value = (uint32_t)((percent / 100.0f) * __HAL_TIM_GET_AUTORELOAD(&htim3));
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_value);
}

/**
 * @brief Map funksiyası (dəyəri bir diapazondan digərinə xəritələyir)
 */
float AdvancedPressureControl_MapValue(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Dəyəri məhdudlaşdır
 */
float AdvancedPressureControl_ClampValue(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/**
 * @brief Xam ADC dəyərini kalibrlənmiş təzyiqə çevir
 *
 * @param adc_raw Xam ADC dəyəri (0-4095)
 * @retval Filtrlənmiş təzyiq (bar)
 */
static float AdvancedPressureControl_ConvertAdcToPressure(uint16_t adc_raw) {
    // DEBUG: İlk çağırışda kalibrləmə məlumatlarını göstər
    static bool first_call = true;
    if (first_call) {
        printf("DEBUG: Calibration values - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar, Slope: %.6f, Offset: %.2f\r\n",
               g_calibration.adc_min, g_calibration.adc_max,
               g_calibration.pressure_min, g_calibration.pressure_max,
               g_calibration.slope, g_calibration.offset);
        first_call = false;
    }

    // DÜZƏLİŞ: Lineyar çevirmə düsturu - offset istifadə edilir
    float pressure = g_calibration.offset + ((float)adc_raw * g_calibration.slope);

    // KRİTİK DÜZƏLİŞ: ADC filtrləmə - Moving Average Filter
    static float pressure_history[8] = {0.0f};  // 8 nümunə üçün tarixçə
    static uint8_t history_index = 0;
    static bool history_filled = false;

    // Tarixçəni yenilə
    pressure_history[history_index] = pressure;
    
    // DÜZƏLİŞ: history_index-i artırmadan əvvəl count hesabla
    // Çünki history_index artırıldıqdan sonra 0 ola bilər və count yanlış hesablanar
    uint8_t count = history_filled ? 8U : (history_index + 1U);
    if (count == 0U) {
        count = 1U;
    }
    
    history_index = (history_index + 1U) % 8U;
    if (history_index == 0U) {
        history_filled = true;
    }

    // Moving Average hesabla
    float filtered_pressure = 0.0f;
    for (uint8_t i = 0U; i < count; i++) {
        filtered_pressure += pressure_history[i];
    }
    filtered_pressure /= (float)count;

    // Filtrlənmiş dəyəri istifadə et
    pressure = filtered_pressure;

    // DEBUG: Hər 100 çağırışda bir dəfə debug məlumatı göstər
    static uint32_t call_count = 0;
    call_count++;
    if (call_count <= 20 || (call_count % 100U == 0U)) {
        printf("DEBUG Convert[%lu]: ADC=%u, RawPressure=%.2f, FilteredPressure=%.2f bar (Offset=%.2f, Slope=%.6f)\r\n",
               call_count, adc_raw, pressure_history[history_index], filtered_pressure, 
               g_calibration.offset, g_calibration.slope);
    }

    // DÜZƏLİŞ: Həm minimum, həm də maksimum limit clamp edilir
    // Minimum clamp: mənfi təzyiqin qarşısını alır
    float min_pressure_clamp = (g_calibration.pressure_min < 0.0f) ? 0.0f : g_calibration.pressure_min;
    if (pressure < min_pressure_clamp) {
        // KRİTİK DÜZƏLİŞ: Mənfi təzyiq clamp olunur - debug məlumatı
        static uint32_t clamp_count = 0;
        clamp_count++;
        if (clamp_count <= 10 || (clamp_count % 100 == 0)) {
            printf("WARNING Convert[%lu]: Pressure clamped from %.2f to %.2f bar (ADC=%u)\r\n",
                   call_count, pressure, min_pressure_clamp, adc_raw);
        }
        pressure = min_pressure_clamp;
    }
    
    // Maksimum clamp: sensor diapazonunu aşan dəyərlərin qarşısını alır
    if (pressure > g_calibration.pressure_max) {
        static uint32_t max_clamp_count = 0;
        max_clamp_count++;
        if (max_clamp_count <= 10 || (max_clamp_count % 100 == 0)) {
            printf("WARNING Convert[%lu]: Pressure clamped from %.2f to %.2f bar (ADC=%u)\r\n",
                   call_count, pressure, g_calibration.pressure_max, adc_raw);
        }
        pressure = g_calibration.pressure_max;
    }

    return pressure;
}

/* =========================================================================
   VII. ƏSAS İDARƏETMƏ VƏ ÖLÇMƏ FUNKSİYALARI
   ========================================================================= */

/**
 * @brief Təzyiqi Bara Çevir
 * @retval Pressure in bar
 * 
 * KRİTİK DÜZƏLİŞ: ADC dəyəri clamp edilir ki, mənfi təzyiq dəyərləri yaranmasın.
 * Əgər ADC < ADC_MIN (620) olarsa, təzyiq PRESSURE_MIN (0.0 bar) olacaq.
 * Əgər ADC > ADC_MAX (4095) olarsa, təzyiq PRESSURE_MAX (300.0 bar) olacaq.
 */
float AdvancedPressureControl_ReadPressure(void) {
    uint16_t adc_raw = AdvancedPressureControl_ReadADC();
    
    // DÜZƏLİŞ: ADC dəyərini çevirmədən əvvəl clamp et
    // Bu, mənfi təzyiq dəyərlərinin qarşısını alır
    if (adc_raw < ADC_MIN) {
        adc_raw = ADC_MIN;
    }
    if (adc_raw > ADC_MAX) {
        adc_raw = ADC_MAX;
    }
    
    return AdvancedPressureControl_ConvertAdcToPressure(adc_raw);
}

/**
 * @brief Motor Sürətinin Tənzimlənməsi (Təzyiq Limitindən Asılı olaraq)
 */
void AdvancedPressureControl_ControlMotorSpeed(void) {
    // DÜZƏLİŞ: Motor yalnız over limit olduqda dayanmalıdır
    // Motor idarəetməsi yalnız SP-yə görə olmalıdır, cari təzyiqə görə deyil
    // Təzyiq SP-dən yüksək olduqda, klapanlar təzyiqi azaltmalıdır, motor deyil
    
    // DÜZƏLİŞ: Motor sürəti - 50 bar üçün 10%
    // 0 bar limit -> 5% Motor sürəti (minimum)
    // 50 bar limit -> 10% Motor sürəti (DÜZƏLİŞ: 8.5%-dən 10%-ə artırıldı)
    // 300.0 bar limit -> 25% Motor sürəti (maksimum)
    // Bu, PID məntiqinin daha yaxşı işləməsinə kömək edəcək
    
    const float M_MIN = 5.0f;      // Motorun Minimum işləmə sürəti
    const float M_MAX = 25.0f;     // Motorun Maksimum işləmə sürəti
    
    // KRİTİK YOXLAMA: Motorun 0% olmasının səbəblərini təhlil et
    static uint32_t debug_count = 0;
    debug_count++;
    
    // KRİTİK DÜZƏLİŞ: Safety vəziyyətində motor 0% təyin olunmalıdır
    if (g_system_status.safety_triggered) {
        g_system_status.motor_pwm_percent = 0.0f;
        AdvancedPressureControl_SetMotor_PWM(0.0f);
        if (debug_count <= 20 || debug_count % 50 == 0) {
            printf("MOTOR STOPPED[%lu]: safety_triggered=%d\r\n",
                   debug_count, g_system_status.safety_triggered);
        }
        return;  // Safety vəziyyətində motor işləməsin
    }
    
    // Əgər motor 0% və ya çox kiçikdirsə, səbəbini tap
    if (g_system_status.motor_pwm_percent < 0.1f) {
        if (debug_count <= 20 || debug_count % 50 == 0) {
            printf("MOTOR 0%% DIAGNOSTIC[%lu]:\r\n", debug_count);
            printf("  -> control_enabled = %d\r\n", g_system_status.control_enabled);
            printf("  -> g_control_initialized = %d\r\n", g_control_initialized);
            printf("  -> target_pressure = %.2f bar\r\n", g_system_status.target_pressure);
            printf("  -> current_pressure = %.2f bar\r\n", g_system_status.current_pressure);
            printf("  -> safety_triggered = %d\r\n", g_system_status.safety_triggered);
            
            // Səbəb təhlili
            if (!g_control_initialized) {
                printf("  *** SƏBƏB: g_control_initialized = false (Init() çağırılmamış)\r\n");
            } else if (!g_system_status.control_enabled) {
                printf("  *** SƏBƏB: control_enabled = false (Nəzarət deaktivdir)\r\n");
            } else if (g_system_status.target_pressure <= 0.1f) {
                printf("  *** SƏBƏB: target_pressure = %.2f bar (çox kiçik, minimum 5%% istifadə olunmalıdır)\r\n", 
                       g_system_status.target_pressure);
            } else {
                printf("  *** SƏBƏB: Naməlum (funksiya çağırılmır və ya başqa səbəb)\r\n");
            }
        }
    }
    
    // DEBUG: Target pressure dəyərini yoxla (hər çağırışda - problem aşkarlamaq üçün)
    if (debug_count <= 20 || debug_count % 50 == 0) {  // İlk 20 çağırışda və hər 50 çağırışda
        printf("DEBUG MotorSpeed[%lu]: target=%.2f bar, enabled=%d, motor_pwm=%.2f%%\r\n",
               debug_count, g_system_status.target_pressure, g_system_status.control_enabled,
               g_system_status.motor_pwm_percent);
    }
    
    // KRİTİK DÜZƏLİŞ: Motor sürəti hesablaması - Feedforward Xəritəsi (Lookup Table)
    // Düz xəttli interpolyasiya motor/nasosun qeyri-xəttiliyini nəzərə almır
    // Feedforward xəritəsi motorun real xüsusiyyətlərini əks etdirir
    // Təzyiq-Sürət Xəritəsi: [Təzyiq (bar), Motor PWM (%)]
    typedef struct {
        float pressure;  // Təzyiq (bar)
        float motor_pwm; // Motor PWM (%)
    } MotorFeedforwardMap_t;
    
    // Feedforward xəritəsi - motorun qeyri-xəttiliyini nəzərə alır
    static const MotorFeedforwardMap_t motor_map[] = {
        {0.0f,   5.0f},   // 0 bar -> 5% (minimum)
        {25.0f,  7.0f},   // 25 bar -> 7%
        {50.0f,  10.0f},  // 50 bar -> 10%
        {100.0f, 15.0f},  // 100 bar -> 15%
        {150.0f, 18.0f},  // 150 bar -> 18%
        {200.0f, 21.0f},  // 200 bar -> 21%
        {250.0f, 23.0f},  // 250 bar -> 23%
        {300.0f, 25.0f}   // 300 bar -> 25% (maksimum)
    };
    const uint8_t map_size = sizeof(motor_map) / sizeof(motor_map[0]);
    
    // DÜZƏLİŞ: target_pressure 0.0-dan böyük olmalıdır, amma PRESSURE_MIN (0.0) də ola bilər
    // KRİTİK DÜZƏLİŞ: Əgər target_pressure 0.0 və ya çox kiçikdirsə, minimum motor sürətini istifadə et
    // DEBUG: target_pressure dəyərini yoxla
    if (debug_count <= 20) {
        printf("DEBUG MotorSpeed[%lu]: Checking target_pressure=%.2f bar, condition=%s\r\n", 
               debug_count, g_system_status.target_pressure,
               (g_system_status.target_pressure > 0.1f) ? ">0.1" : "<=0.1");
    }
    
    if (g_system_status.target_pressure > 0.1f) {
        float effective_pressure = g_system_status.target_pressure;
        if (effective_pressure < PRESSURE_MIN) {
            effective_pressure = PRESSURE_MIN;
        }
        if (effective_pressure > PRESSURE_MAX) {
            effective_pressure = PRESSURE_MAX;
        }
        
        // Feedforward xəritəsindən motor sürətini tap
        // Xəritəni interpolyasiya ilə istifadə et
        float motor_pwm = M_MIN;  // Default: minimum
        
        // Xəritədə dəqiq uyğunluq tap
        if (effective_pressure <= motor_map[0].pressure) {
            motor_pwm = motor_map[0].motor_pwm;
        } else if (effective_pressure >= motor_map[map_size - 1].pressure) {
            motor_pwm = motor_map[map_size - 1].motor_pwm;
        } else {
            // İnterpolyasiya: iki nöqtə arasında düz xəttli interpolyasiya
            for (uint8_t i = 0; i < map_size - 1; i++) {
                if (effective_pressure >= motor_map[i].pressure && 
                    effective_pressure <= motor_map[i + 1].pressure) {
                    // İnterpolyasiya: y = y1 + (x - x1) * (y2 - y1) / (x2 - x1)
                    float x1 = motor_map[i].pressure;
                    float x2 = motor_map[i + 1].pressure;
                    float y1 = motor_map[i].motor_pwm;
                    float y2 = motor_map[i + 1].motor_pwm;
                    
                    if (x2 > x1) {  // Təhlükəsizlik yoxlaması
                        motor_pwm = y1 + (effective_pressure - x1) * (y2 - y1) / (x2 - x1);
                    } else {
                        motor_pwm = y1;
                    }
                    break;
                }
            }
        }
        
        // Məhdudlaşdırma
        g_system_status.motor_pwm_percent = AdvancedPressureControl_ClampValue(
            motor_pwm, M_MIN, M_MAX);
        
        AdvancedPressureControl_SetMotor_PWM(g_system_status.motor_pwm_percent);
        
        // DEBUG: Motor sürəti təyin olunduqda
        if (debug_count <= 20 || debug_count % 50 == 0) {
            printf("DEBUG MotorSpeed[%lu]: Motor PWM set to %.2f%% (target=%.2f bar, Feedforward map)\r\n",
                   debug_count, g_system_status.motor_pwm_percent, g_system_status.target_pressure);
        }
    } else {
        // MÜVƏQQƏTİ HƏLL: target_pressure < 0.1 bar olduqda motor minimum sürətdə işləyir
        // Bu, problemin target_pressure dəyərinin düzgün yüklənməməsi ilə bağlı olduğunu təsdiqləyir
        // Əgər motor 5% sürətlə işləyirsə, bu, target_pressure dəyərinin düzgün yüklənmədiyini göstərir
        // QEYD: M_MIN artıq funksiyanın əvvəlində təyin olunub (sətir 256)
        g_system_status.motor_pwm_percent = M_MIN;  // 5.0% minimuma təyin et
        AdvancedPressureControl_SetMotor_PWM(M_MIN);
        
        // DEBUG: Motor minimum sürətdə işləyir
        if (debug_count <= 20 || debug_count % 50 == 0) {
            printf("DEBUG MotorSpeed[%lu]: Target too low (%.2f bar <= 0.1), using MIN PWM=%.1f%%\r\n",
                   debug_count, g_system_status.target_pressure, M_MIN);
        }
    }
}

/**
 * @brief ZME İdarəetməsi (TƏRS Məntiq - Baza + PID Təshisi)
 * @param control_output: PID çıxışı (-PWM_TRIM_LIMIT to +PWM_TRIM_LIMIT)
 */
void AdvancedPressureControl_ControlZME(float control_output) {
    // ZME İdarəetməsi (TƏRS MƏNTİQ - Normally Open: 0% tam açıq, 30% tam bağlı)
    // 
    // KRİTİK DÜZƏLİŞ: ZME-nin davranışı əksdir!
    // ZME Normally Open-dir:
    // - 0% = tam açıq (təzyiq artır) ← ƏKS MƏNTİQ!
    // - 30% = tam bağlı (təzyiq azalır) ← ƏKS MƏNTİQ!
    // 
    // Bizim sistemdə:
    // Error = target - current
    // Error müsbət (təzyiq aşağı) → pid_output müsbət → təzyiq artırmaq lazımdır → ZME açılmalıdır (PWM azalır)
    // Error mənfi (təzyiq yüksək) → pid_output mənfi → təzyiq azaltmaq lazımdır → ZME bağlanmalıdır (PWM artır)
    // 
    // Düzgün məntiq (ƏKS):
    // - control_output müsbət (təzyiq artırmaq) → ZME açılır → zme_pwm azalır → zme_pwm = base - control_output
    // - control_output mənfi (təzyiq azaltmaq) → ZME bağlanır → zme_pwm artır → zme_pwm = base - control_output
    // 
    // Yoxlama:
    // - base = 20%, control_output = +15 (təzyiq artırmaq) → zme_pwm = 5% → ZME açıq (təzyiq artır) ✓
    // - base = 20%, control_output = -15 (təzyiq azaltmaq) → zme_pwm = 35% (clamp to 30%) → ZME bağlı (təzyiq azalır) ✓
    float zme_pwm = g_zme_base_pwm - control_output;  // DÜZƏLİŞ: ƏKS məntiq üçün çıxmaq lazımdır
    
    // ZME Limitləməsi (0-30%)
    zme_pwm = AdvancedPressureControl_ClampValue(zme_pwm, ZME_PWM_MIN, ZME_PWM_MAX);
    
    // DÜZƏLİŞ: ZME kompensasiyası ləğv edildi - PID dəyərlərini birbaşa istifadə edirik
    // AdvancedPressureControl_CompensateZME_Nonlinearity() funksiyası artıq çağırılmır
    // Bu, PID-nin incə tənzimləmə işini pozmur və klapanın sıçrayışını aradan qaldırır
    
    g_system_status.zme_pwm_percent = zme_pwm;
}

/**
 * @brief SP-yə görə ZME baza dəyərini hesabla
 * @param sp: Setpoint (bar)
 * @retval ZME baza dəyəri (%)
 * 
 * Məntiq:
 * - SP = 0 bar → ZME baza = 0% (tam açıq, təzyiq artırır)
 * - SP = 50 bar → ZME baza = 26% (orta mövqe, təzyiq sabit saxlayır)
 * - Linear interpolation
 */
float AdvancedPressureControl_CalculateZMEBaseFromSP(float sp) {
    // SP = 0 bar → ZME baza = 0% (tam açıq, təzyiq artırır)
    // SP = 50 bar → ZME baza = 26% (orta mövqe, təzyiq sabit saxlayır)
    // Linear interpolation
    
    if (sp <= 0.0f) {
        return 0.0f;  // Minimum SP üçün minimum ZME (tam açıq)
    }
    
    if (sp >= 50.0f) {
        return 26.0f;  // 50 bar və yuxarı üçün 26% (orta mövqe)
    }
    
    // Linear interpolation: 0 bar → 0%, 50 bar → 26%
    float zme_base = (sp / 50.0f) * 26.0f;
    return zme_base;
}

/**
 * @brief SP-yə görə DRV baza dəyərini hesabla
 * @param sp: Setpoint (bar)
 * @retval DRV baza dəyəri (%)
 * 
 * Məntiq:
 * - SP = 0 bar → DRV baza = 26% (bağlı, təzyiq artırır)
 * - SP = 50 bar → DRV baza = 13% (açıq, təzyiq boşalda bilir)
 * - Linear interpolation
 */
float AdvancedPressureControl_CalculateDRVBaseFromSP(float sp) {
    // SP = 0 bar → DRV baza = 26%
    // SP = 50 bar → DRV baza = 13%
    // Linear interpolation
    
    if (sp <= 0.0f) {
        return 26.0f;  // Minimum SP üçün DRV baza dəyəri
    }
    
    if (sp >= 50.0f) {
        return 13.0f;  // 50 bar və yuxarı üçün 13% (açıq)
    }
    
    // Linear interpolation: 0 bar → 26%, 50 bar → 13%
    float drv_base = 26.0f - (sp / 50.0f) * (26.0f - 13.0f);
    return drv_base;
}

/**
 * @brief DRV İdarəetməsi (DÜZ Məntiq - Baza + PID Təshisi)
 * @param control_output: PID çıxışı (-PWM_TRIM_LIMIT to +PWM_TRIM_LIMIT)
 */
void AdvancedPressureControl_ControlDRV(float control_output) {
    // DRV İdarəetməsi (DÜZ: +Control Output DRV-ni bağlayır)
    // Baza mövqe + PID təshisi
    // control_output müsbət → təzyiq artırmaq lazımdır → DRV bağlanmalıdır (PWM artır)
    // control_output mənfi → təzyiq azaltmaq lazımdır → DRV açılmalıdır (PWM azalır)
    float drv_pwm = g_drv_base_pwm + control_output;

    // DRV Limitləməsi (0-40%)
    drv_pwm = AdvancedPressureControl_ClampValue(drv_pwm, DRV_PWM_MIN, DRV_PWM_MAX);
    
    g_system_status.drv_pwm_percent = drv_pwm;
}

/**
 * @brief Klapan çıxışlarını tətbiq et
 */
void AdvancedPressureControl_ApplyValveOutputs(void) {
    AdvancedPressureControl_SetZME_PWM(g_system_status.zme_pwm_percent);
    AdvancedPressureControl_SetDRV_PWM(g_system_status.drv_pwm_percent);
}

/* =========================================================================
   VIII. PID İDARƏETMƏ FUNKSİYALARI
   ========================================================================= */

/**
 * @brief PID nəzarətçisini başlat
 */
void AdvancedPressureControl_InitPID(PID_Controller_t* pid, float kp, float ki, float kd) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->integral_sum = 0.0f;
    pid->previous_error = 0.0f;
    pid->output_min = PID_OUTPUT_MIN;
    pid->output_max = PID_OUTPUT_MAX;
}

/**
 * @brief PID hesablaması
 * @param pid: PID nəzarətçisi
 * @param error: Xəta dəyəri
 * @param dt: Zaman addımı (saniyə)
 * @retval PID çıxışı
 */
float AdvancedPressureControl_CalculatePID(PID_Controller_t* pid, float error, float dt) {
    // Proportional term
    float P_term = pid->Kp * error;

    // Integral term (Anti-windup ilə - Clamping/Tracking metodu)
    // DÜZƏLİŞ: Ki termi HƏMİŞƏ işləməlidir - steady-state error aradan qaldırılması üçün
    // Dead Band yoxlaması SİLİNDİ (Çünki Ki hədəfə dəqiq çatmalıdır)
    // Dead Band daxilində də Ki termi işləməyə davam etməlidir, çünki:
    // 1. Dead Band daxilində PID çıxışı sıfırlanır (Step() funksiyasında), amma Ki inteqrasiyası davam edir
    // 2. Ki inteqrasiyası dayandırılsa, sistem hədəf təzyiqdən kiçik bir məsafədə sabitləşəcək (steady-state error)
    // 3. Ki həmişə inteqrasiya edərsə, sistem xətanı 0-a endirə bilər
    // 4. Dead Band yalnız PID çıxışını sıfırlamalıdır, Ki inteqrasiyasını deyil
    // 5. Anti-windup mexanizmi Ki-nin həddindən artıq böyüməsinin qarşısını alır
    if (pid->Ki > 0.0f) {
        pid->integral_sum += error * dt; // Ki həmişə inteqrasiya edir
    }
    // Ki=0-dırsa, inteqratoru dondur (windup-a qarşı qoruma)
    
    float I_term = pid->Ki * pid->integral_sum;

    // Derivative term (səs-küyü azaltmaq üçün yalnız Kd > 0 olduqda)
    // KRİTİK DÜZƏLİŞ: dt sabit 0.01f-dir (CONTROL_LOOP_TIME_S), ona görə də dt > 0 yoxlaması lazımsızdır
    // Amma təhlükəsizlik üçün NaN yoxlaması saxlanılır
    float D_term = 0.0f;
    if (pid->Kd > 0.0f) {  // dt sabit 0.01f-dir, ona görə də dt > 0.0001f yoxlaması lazımsızdır
        float derivative = (error - pid->previous_error) / dt;  // dt = CONTROL_LOOP_TIME_S = 0.01f
        // NaN yoxlaması (təhlükəsizlik üçün)
        if (!isnan(derivative) && !isinf(derivative)) {
            pid->previous_error = error;
            D_term = pid->Kd * derivative;
        } else {
            pid->previous_error = error;
            D_term = 0.0f;
        }
    } else {
        pid->previous_error = error;
    }

    // Yekun PID Çıxışı
    float pid_output = P_term + I_term + D_term;
    
    // DÜZƏLİŞ: PID çıxışını yoxla
    if (isnan(pid_output) || isinf(pid_output)) {
        printf("PID ERROR: Invalid PID output: %.2f (P=%.2f, I=%.2f, D=%.2f)\r\n", 
               pid_output, P_term, I_term, D_term);
        pid_output = 0.0f;
    }
    
    // PID çıxışını limitlə (Anti-windup üçün limitlənmədən əvvəl dəyəri saxla)
    float pid_output_before_clamp = pid_output;
    pid_output = AdvancedPressureControl_ClampValue(pid_output, pid->output_min, pid->output_max);
    
    // KRİTİK DÜZƏLİŞ: Anti-windup (Sadə Clamping/Tracking metodu)
    // Sadə yanaşma:
    // 1. İnteqratoru yenilə (yuxarıda edilib, yalnız Dead Band xaricində)
    // 2. Yekun PID çıxışını limitlə (yuxarıda edilib)
    // 3. Əgər çıxış limitlənibsə, inteqratoru limitdən kənara çıxmaması üçün yenilə (Tracking)
    // KRİTİK: Ki > 0.0001f yoxlaması bütün bölmə əməliyyatlarından ƏVVƏL olmalıdır
    // YALNIZ DÜZƏLİŞ EHTİYACI OLAN HİSSƏ: pid->Ki > 0 yoxlamasını təmin etmək
    if (pid->Ki > 0.0001f) {
        // Yoxlama: Bu kod bloku yalnız Ki > 0.0001f olduğu halda işləməlidir
        
        // İnteqratoru həddindən artıq böyüməkdən qorumaq üçün limitlə
        // KRİTİK: Ki > 0.0001f yoxlaması artıq aparılıb - bölmə təhlükəsizdir
        float max_integral = pid->output_max / pid->Ki;
        float min_integral = pid->output_min / pid->Ki;
        
        // İnteqratoru limitlə (sadə Clamping)
        if (pid->integral_sum > max_integral) {
            pid->integral_sum = max_integral;
        } else if (pid->integral_sum < min_integral) {
            pid->integral_sum = min_integral;
        }
        
        // Tracking metodu: Əgər PID çıxışı limitləndisə, inteqratoru yenilə
        // Bu, daha sürətli bərpa təmin edir və overshoot-u azaldır
        if (pid_output_before_clamp != pid_output) {
            // PID çıxışı limitləndi - inteqratoru limitlənmə dəyərinə təyin et
            // KRİTİK: Ki > 0.0001f yoxlaması artıq yuxarıda aparılıb - bölmə təhlükəsizdir
            // BÖLMƏ BURADADIR - Ki > 0.0001f yoxlamasından sonra təhlükəsizdir
            float desired_integral = (pid_output - P_term - D_term) / pid->Ki;
            
            // NaN yoxlaması və limitləmə
            if (!isnan(desired_integral) && !isinf(desired_integral)) {
                // İnteqratoru düzgün dəyərə təyin et, amma limitləri aşmamalıdır
                // Bu, ikili limitləməni aradan qaldırır - yalnız bir dəfə limitləmə
                if (desired_integral > max_integral) {
                    pid->integral_sum = max_integral;
                } else if (desired_integral < min_integral) {
                    pid->integral_sum = min_integral;
                } else {
                    pid->integral_sum = desired_integral;
                }
            }
        }
    }
    
    return pid_output;
}

/**
 * @brief PID parametrlərini təyin et
 */
void AdvancedPressureControl_SetPIDParams(PID_Controller_t* pid, float kp, float ki, float kd) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->integral_sum = 0.0f;
    pid->previous_error = 0.0f;
}

/* =========================================================================
   IX. TƏHLÜKƏSİZLİK FUNKSİYALARI
   ========================================================================= */

/**
 * @brief Təhlükəsizlik yoxlaması
 * @retval true if safe, false if dangerous
 */
bool AdvancedPressureControl_SafetyCheck(void) {
    if (!g_safety_limits.safety_enabled) {
        return true;
    }
    
    // DEBUG: Təhlükəsizlik yoxlaması (motor 0.00% problemi üçün)
    static uint32_t safety_debug_count = 0;
    safety_debug_count++;
    bool safety_ok = true;
    const char* failure_reason = NULL;
    
    // Təzyiq çox yüksək olduqda təhlükəsizlik tədbiri
    if (g_system_status.current_pressure > 
        (g_system_status.target_pressure + g_safety_limits.over_limit_margin)) {
        failure_reason = "Over limit";
        safety_ok = false;
    }
    
    // Maksimum təhlükəsizlik limiti
    if (g_system_status.current_pressure > g_safety_limits.max_pressure) {
        failure_reason = "Max pressure exceeded";
        safety_ok = false;
    }
    
    
    // DEBUG: Təhlükəsizlik yoxlaması uğursuz olduqda
    if (!safety_ok && (safety_debug_count <= 20 || safety_debug_count % 100 == 0)) {
        printf("SAFETY CHECK[%lu]: %s (current=%.2f bar, target=%.2f bar, margin=%.2f, max=%.2f)\r\n",
               safety_debug_count, failure_reason, g_system_status.current_pressure,
               g_system_status.target_pressure, g_safety_limits.over_limit_margin,
               g_safety_limits.max_pressure);
    }
    
    if (!safety_ok) {
        return false;
    }
    
    return true;
}

/**
 * @brief Təhlükəsizlik pozuntusunu idarə et
 */
void AdvancedPressureControl_HandleSafetyViolation(void) {
    printf("TƏHLÜKƏSİZLİK REJİMİ: Təzyiq çox yüksək! P=%.1f > SP+%.1f=%.1f\r\n", 
           g_system_status.current_pressure, 
           g_safety_limits.over_limit_margin,
           g_system_status.target_pressure + g_safety_limits.over_limit_margin);
    
    // PID-ni sıfırla
    g_pid_zme.integral_sum = 0.0f;
    g_pid_zme.previous_error = 0.0f;
    g_pid_drv.integral_sum = 0.0f;
    g_pid_drv.previous_error = 0.0f;
    
    // DRV-ni aç (0.0%) və ZME-ni bağla (30.0%) - Kəskin Təzyiq Azaltma
    g_system_status.drv_pwm_percent = DRV_PWM_MIN; // 0.0%
    g_system_status.zme_pwm_percent = ZME_PWM_MAX; // 30.0%
    g_system_status.motor_pwm_percent = 0.0f; // Motor dayandır
    
    // Təhlükəsizlik rejimini aktivləşdir
    g_system_status.safety_triggered = true;
    
    // Klapanları təyin et
    AdvancedPressureControl_ApplyValveOutputs();
    AdvancedPressureControl_SetMotor_PWM(0.0f);
}

/* =========================================================================
   X. ƏSAS İDARƏETMƏ DÖVRÜ
   ========================================================================= */

/**
 * @brief Əsas idarəetmə dövrü - Təkmilləşdirilmiş Final PID + Dead Band + Baza Mövqe
 * 
 * Bu funksiya Common Rail sistemləri üçün optimal PID tənzimləmə metodunu tətbiq edir.
 * PID-nin gücünü Dead Band qoruyucusu ilə birləşdirir və titrämə problemini həll edir.
 * 
 * Məntiq Strukturu:
 * 1. Ölü Zona (Dead Band): Təzyiq ±1.0 bar diapazonunda olduqda, PID hesablamaları atlanır
 *    və klapanlar hazırkı mövqeydə qalır (titräməyə qarşı qoruma).
 * 
 * 2. PID Hesablaması: Kp=0.8, Ki=0.05, Kd=0.01 parametrləri ilə proporsional idarəetmə.
 *    PID çıxışı PWM_TRIM_LIMIT (±20%) ilə məhdudlaşdırılır.
 * 
 * 3. Baza Mövqe + PID Təshisi: 
 *    - ZME: zme_pwm = ZME_BASE_PWM + control_output (TƏRS məntiq)
 *    - DRV: drv_pwm = DRV_BASE_PWM + control_output (DÜZ məntiq)
 * 
 * Tuning Qaydası:
 * - Kp-ni yavaş-yavaş artırın (məsələn, 0.5 → 0.6). DRV-nin hərəkəti hamar olmalıdır.
 * - Ki-ni yavaş-yavaş artırın (0.005-dən başlayaraq). Bu, təzyiqin dəqiq limitində dayanmasını təmin edəcək.
 * - Kd-ni yalnız sensor səs-küyü varsa və filtrləmə kifayət etmirsə əlavə edin.
 * 
 * Timer kəsilməsində çağırılmalıdır (10ms interval)
 */
void AdvancedPressureControl_Step(void) {
    // DÜZƏLİŞ: Debug mesajları əlavə et (KRİTİK - motor 0.00% problemi üçün)
    static uint32_t step_debug_count = 0;
    step_debug_count++;
    
    if (!g_control_initialized) {
        // DEBUG: İlk 20 çağırışda hər dəfə göstər (problemin səbəbini tapmaq üçün)
        if (step_debug_count <= 20 || step_debug_count % 100 == 0) {
            printf("PID ERROR[%lu]: g_control_initialized = false\r\n", step_debug_count);
            printf("  -> Call AdvancedPressureControl_Init() first!\r\n");
        }
        // KRİTİK: Sistem başlanmayıbsa, PWM-ləri sıfırla və çıx
        AdvancedPressureControl_SetZME_PWM(0.0f);
        AdvancedPressureControl_SetDRV_PWM(0.0f);
        AdvancedPressureControl_SetMotor_PWM(0.0f);
        return;
    }
    if (!g_system_status.control_enabled) {
        // DEBUG: İlk 20 çağırışda hər dəfə göstər (problemin səbəbini tapmaq üçün)
        if (step_debug_count <= 20 || step_debug_count % 100 == 0) {
            printf("PID WARNING[%lu]: control_enabled = false (target=%.2f bar, current=%.2f bar)\r\n",
                   step_debug_count, g_system_status.target_pressure, g_system_status.current_pressure);
            printf("  -> Enable control: pid_status->control_enabled = true\r\n");
        }
        // KRİTİK DÜZƏLİŞ: Nəzarət disabled-dirsə, motor sürətini təyin et (target_pressure-a görə)
        // Amma PID hesablamalarını atla (klapanlar və PID işləməsin)
        // Motor həmişə target_pressure-a görə işləməlidir, nəzarət disabled olsa belə
        AdvancedPressureControl_ControlMotorSpeed();
        // Klapanları təhlükəsiz vəziyyətə gətir
        AdvancedPressureControl_SetZME_PWM(ZME_PWM_MAX);  // ZME bağlı
        AdvancedPressureControl_SetDRV_PWM(DRV_PWM_MIN);  // DRV açıq
        return;  // PID hesablamalarını atla
    }
    
    // DEBUG: İlk bir neçə çağırışda status göstər
    static uint32_t step_count = 0;
    step_count++;
    if (step_count <= 5 || step_count % 1000 == 0) {
        printf("PID Step #%lu: target=%.1f, current=%.1f, error=%.1f, enabled=%d\r\n",
               step_count, g_system_status.target_pressure, g_system_status.current_pressure,
               g_system_status.error, g_system_status.control_enabled);
    }
    
    
    // 0. Təzyiqi Oxu (Safety yoxlaması üçün lazımdır)
    // DÜZƏLİŞ: AdvancedPressureControl_ReadPressure() istifadə et ki, ADC clamp düzgün işləsin
    // KRİTİK DÜZƏLİŞ: Xam ADC dəyərini də Status strukturuna yaz (UI üçün)
    uint16_t raw_adc = AdvancedPressureControl_ReadADC();
    g_system_status.raw_adc_value = raw_adc;
    // DÜZƏLİŞ: ReadPressure() istifadə et ki, ADC clamp və filtrləmə düzgün işləsin
    g_system_status.current_pressure = AdvancedPressureControl_ReadPressure();
    
    // Validate pressure reading without hiding genuine over-pressure conditions
    if (!isfinite(g_system_status.current_pressure) || g_system_status.current_pressure < 0.0f) {
        printf("PID ERROR: Invalid pressure reading: %.2f bar\r\n", g_system_status.current_pressure);
        g_system_status.current_pressure = 0.0f;
    } else {
        if (g_system_status.current_pressure > g_safety_limits.max_pressure) {
            static uint32_t over_limit_log_count = 0;
            if (over_limit_log_count < 10U || (over_limit_log_count % 50U) == 0U) {
                printf("PID WARNING: Pressure %.2f bar exceeds configured max %.1f bar\r\n",
                       g_system_status.current_pressure, g_safety_limits.max_pressure);
            }
            over_limit_log_count++;
        }
        float sensor_fault_limit = g_safety_limits.max_pressure + 50.0f;
        if (g_system_status.current_pressure > sensor_fault_limit) {
            float fault_value = g_system_status.current_pressure;
            g_system_status.current_pressure = sensor_fault_limit;
            printf("PID WARNING: Pressure reading %.2f bar exceeds sensor range, clamping to %.1f bar\r\n",
                   fault_value, sensor_fault_limit);
        }
    }
    
    // 1. TƏHLÜKƏSİZLİK MÜDAXİLƏSİ (Motor təyin olunmadan ƏVVƏL)
    // KRİTİK DÜZƏLİŞ: Safety yoxlaması motor təyin olunmadan əvvəl aparılmalıdır
    // Çünki safety uğursuz olarsa, motor 0% təyin olunur və return edilir
    if (!AdvancedPressureControl_SafetyCheck()) {
        // DEBUG: Təhlükəsizlik yoxlaması uğursuz olduqda (motor 0.0f-ə təyin olunur)
        if (step_debug_count <= 20 || step_debug_count % 100 == 0) {
            printf("PID SAFETY[%lu]: Safety check FAILED (current=%.2f bar, target=%.2f bar)\r\n",
                   step_debug_count, g_system_status.current_pressure, g_system_status.target_pressure);
        }
        AdvancedPressureControl_HandleSafetyViolation();
        return; // Normal PID-yə keçmə
    }
    
    // 2. SP-yə görə ZME və DRV baza dəyərlərini yenilə (YENİ MƏNTİQ)
    // ZME və DRV baza dəyərləri SP-yə görə təyin olunur, PID error-a görə təshis edir
    g_zme_base_pwm = AdvancedPressureControl_CalculateZMEBaseFromSP(g_system_status.target_pressure);
    g_drv_base_pwm = AdvancedPressureControl_CalculateDRVBaseFromSP(g_system_status.target_pressure);
    
    // 3. Motor sürətini tənzimlə (Hədəf təzyiqə görə)
    // KRİTİK DÜZƏLİŞ: Safety yoxlaması keçdikdən sonra motor sürəti təyin olunur
    // Bu, motorun safety vəziyyətində 0% qalmasını təmin edir
    if (step_debug_count <= 10) {
        printf("STEP[%lu]: Calling ControlMotorSpeed() - target=%.2f bar\r\n", 
               step_debug_count, g_system_status.target_pressure);
    }
    AdvancedPressureControl_ControlMotorSpeed();
    if (step_debug_count <= 10) {
        printf("STEP[%lu]: After ControlMotorSpeed() - motor_pwm=%.2f%%\r\n", 
               step_debug_count, g_system_status.motor_pwm_percent);
    }
    
    // 4. Xətanı Hesabla
    g_system_status.error = g_system_status.target_pressure - g_system_status.current_pressure;
    
    // DÜZƏLİŞ: Xəta dəyərini yoxla
    if (isnan(g_system_status.error) || isinf(g_system_status.error)) {
        printf("PID ERROR: Invalid error value: %.2f\r\n", g_system_status.error);
        g_system_status.error = 0.0f;
    }
    
    // Təhlükəsizlik rejimindən çıx
    // KRİTİK DÜZƏLİŞ: Safety vəziyyətindən çıxmaq üçün təzyiq target-dan aşağı olmalıdır
    // və safety yoxlaması uğurlu olmalıdır
    if (g_system_status.safety_triggered) {
        if (g_system_status.current_pressure < g_system_status.target_pressure && 
            AdvancedPressureControl_SafetyCheck()) {
            g_system_status.safety_triggered = false;
            printf("TƏHLÜKƏSİZLİK REJİMİNDƏN ÇIXILDI (current=%.2f bar < target=%.2f bar)\r\n",
                   g_system_status.current_pressure, g_system_status.target_pressure);
        } else {
            // Safety vəziyyəti aktivdir, motor 0% qalmalıdır
            if (step_debug_count <= 20 || step_debug_count % 100 == 0) {
                printf("SAFETY ACTIVE[%lu]: current=%.2f bar, target=%.2f bar (motor stopped)\r\n",
                       step_debug_count, g_system_status.current_pressure, g_system_status.target_pressure);
            }
        }
    }
    
    // 6. PID Hesablamaları (HƏMİŞƏ İŞLƏYİR - Steady-State Error Aradan Qaldırılması Üçün)
    // KRİTİK DÜZƏLİŞ: error_multiplier = 2.0f məntiqi silindi - PID simmetriyasını pozurdu
    // PID-nin simmetrik işləməsi üçün error dəyərini dəyişdirməmək lazımdır
    // Əgər təzyiq SP-dən yüksəkdirsə, PID-nin özü bunu düzəltməlidir (Kp, Ki, Kd parametrləri ilə)
    
    // KRİTİK DÜZƏLİŞ: dt-ni sabit dəyərə təyin et (Timer 6 10ms-də bir çağırır)
    // HAL_GetTick() gecikmələri və qeyri-dəqiqliyi aradan qaldırmaq üçün sabit dt istifadə edirik
    // Timer 6 (10ms) istifadə edildiyi üçün dt = 0.01f (CONTROL_LOOP_TIME_S)
    float dt = CONTROL_LOOP_TIME_S;  // dt = 0.01f (10ms = 0.01 saniyə)
    
    // DÜZƏLİŞ: ZME və DRV üçün AYRI PID hesablamaları (HƏMİŞƏ İŞLƏYİR)
    // PID hesablamaları hər zaman aparılır - Dead Band daxilində də inteqrator işləməyə davam edir
    // Bu, steady-state error-u aradan qaldırır və sistemin hədəf təzyiqə dəqiq çatmasını təmin edir
    float zme_pid_output = AdvancedPressureControl_CalculatePID(
        &g_pid_zme, g_system_status.error, dt);
    
    // DRV üçün PID hesablaması (ayrı PID nəzarətçisi)
    float drv_pid_output = AdvancedPressureControl_CalculatePID(
        &g_pid_drv, g_system_status.error, dt);
    
    // PID çıxışlarını limitlə (PWM_TRIM_LIMIT)
    float zme_control_output = zme_pid_output;
    if (zme_control_output > PWM_TRIM_LIMIT) zme_control_output = PWM_TRIM_LIMIT;
    if (zme_control_output < -PWM_TRIM_LIMIT) zme_control_output = -PWM_TRIM_LIMIT;
    
    float drv_control_output = drv_pid_output;
    if (drv_control_output > PWM_TRIM_LIMIT) drv_control_output = PWM_TRIM_LIMIT;
    if (drv_control_output < -PWM_TRIM_LIMIT) drv_control_output = -PWM_TRIM_LIMIT;
    
    // 7. ÖLÜ ZONA (DEAD BAND) TƏTBİQİ - Yalnız Çıxışı Sıfırla, İnteqratoru Dondurma
    // Dead band: ±1.0 bar - Təzyiq bu diapazonunda olduqda PID çıxışı sıfırlanır
    // KRİTİK DÜZƏLİŞ: Dead Band daxilində yalnız PID çıxışı sıfırlanır, inteqrator işləməyə davam edir
    // Bu, titrəməni (hunting) azaldır, amma steady-state error-u aradan qaldırır
    // İnteqratorun işləməyə davam etməsi sayəsində sistem hədəf təzyiqə dəqiq çata bilər
    bool in_dead_band = (fabsf(g_system_status.error) < DEAD_BAND_BAR);
    
    if (in_dead_band) {
        // Dead Band daxilində yalnız çıxışı sıfırla (inteqrator işləməyə davam edir)
        zme_control_output = 0.0f;
        drv_control_output = 0.0f;
        g_system_status.pid_output = 0.0f;
        // QEYD: previous_error yenilənməlidir ki, derivative term düzgün işləsin
        // Amma bu, CalculatePID() funksiyasında artıq edilir
    } else {
        // Dead Band xaricində normal PID çıxışı istifadə olunur
        g_system_status.pid_output = zme_pid_output;
    }
    
    // 8. KLAPAN İDARƏETMƏSİ (Baza Mövqe + PID Təshisi)
    // YENİ MƏNTİQ: ZME və DRV baza dəyərləri SP-yə görə, PID error-a görə təshis edir
    // PID çıxışı error-a görədir:
    // Error müsbət (təzyiq aşağı) → pid_output müsbət → təzyiq artırmaq lazımdır
    // Error mənfi (təzyiq yüksək) → pid_output mənfi → təzyiq azaltmaq lazımdır
    // 
    // ZME (ƏKS məntiq - Normally Open: 0% tam açıq (təzyiq artır), 30% tam bağlı (təzyiq azalır)):
    // YENİ MƏNTİQ: ZME baza dəyəri SP-yə görə təyin olunub (yuxarıda)
    // Təzyiq artırmaq lazımdır → ZME açılmalıdır (PWM azalır) → zme_control_output müsbət → zme_pwm = base(SP) - control_output
    // Təzyiq azaltmaq lazımdır → ZME bağlanmalıdır (PWM artır) → zme_control_output mənfi → zme_pwm = base(SP) - control_output
    AdvancedPressureControl_ControlZME(zme_control_output);
    
    // DRV (DÜZ məntiq - Normally Closed):
    // YENİ MƏNTİQ: DRV baza dəyəri SP-yə görə təyin olunub (yuxarıda)
    // Təzyiq artırmaq lazımdır → DRV bağlanmalıdır (PWM artır) → drv_control_output müsbət → drv_pwm = base(SP) + control_output
    // Təzyiq azaltmaq lazımdır → DRV açılmalıdır (PWM azalır) → drv_control_output mənfi → drv_pwm = base(SP) + control_output
    AdvancedPressureControl_ControlDRV(drv_control_output);
    
    // 9. PWM Siqnallarını Tətbiq et
    // DÜZƏLİŞ: PWM dəyərlərini yoxla
    if (isnan(g_system_status.zme_pwm_percent) || isinf(g_system_status.zme_pwm_percent)) {
        printf("PID ERROR: Invalid ZME PWM: %.2f%%\r\n", g_system_status.zme_pwm_percent);
        g_system_status.zme_pwm_percent = g_zme_base_pwm;
    }
    if (isnan(g_system_status.drv_pwm_percent) || isinf(g_system_status.drv_pwm_percent)) {
        printf("PID ERROR: Invalid DRV PWM: %.2f%%\r\n", g_system_status.drv_pwm_percent);
        g_system_status.drv_pwm_percent = g_drv_base_pwm;
    }
    
    AdvancedPressureControl_ApplyValveOutputs();
    
    // 10. Status yenilə
    AdvancedPressureControl_UpdateStatus();
    
    // Debug çıxışı (hər 10 dövrədə bir - çox mesaj olmasın)
    static uint16_t debug_counter = 0;
    if (++debug_counter >= 10) {
        debug_counter = 0;
        printf("SP:%.1f  P:%.1f  ERR:%.2f  Kp:%.3f  Ki:%.4f  ZME_PID:%.2f  DRV_PID:%.2f  ZME:%.1f%%(B:%.1f%%)  DRV:%.1f%%(B:%.1f%%)  MOTOR:%.1f%%\r\n",
               g_system_status.target_pressure, g_system_status.current_pressure, 
               g_system_status.error, g_pid_zme.Kp, g_pid_zme.Ki,
               zme_control_output, drv_control_output,
               g_system_status.zme_pwm_percent, g_zme_base_pwm,
               g_system_status.drv_pwm_percent, g_drv_base_pwm,
               g_system_status.motor_pwm_percent);
    }
}

/* =========================================================================
   XI. SİSTEM BAŞLATMA VƏ KONFİQURASİYA
   ========================================================================= */

/**
 * @brief Sistem başlatma
 */
void AdvancedPressureControl_Init(void) {
    printf("Advanced Pressure Control System - Initializing...\r\n");
    
    // Status strukturunu sıfırla
    memset(&g_system_status, 0, sizeof(SystemStatus_t));
    
    // İlkin dəyərlər
    g_system_status.target_pressure = 70.0f; // İlkin hədəf təzyiq
    g_system_status.control_enabled = false;  // İlkin olaraq false, sonra true ediləcək
    g_system_status.safety_triggered = false;
    
    // KRİTİK: target_pressure 0.0 və ya çox kiçik olarsa, default dəyər təyin et
    if (g_system_status.target_pressure < 0.1f) {
        g_system_status.target_pressure = 70.0f;  // Default 70 bar
        printf("WARNING: target_pressure was too small, setting to default 70.0 bar\r\n");
    }
    
    // PID nəzarətçilərini başlat (Tənzimlənmiş default dəyərlər)
    // Kp=0.8, Ki=0.05, Kd=0.01 - Tənzimlənmiş default dəyərlər
    AdvancedPressureControl_InitPID(&g_pid_zme, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
    // DRV də eyni PID parametrlərini istifadə edir (baza mövqe + PID təshisi)
    AdvancedPressureControl_InitPID(&g_pid_drv, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
    
    // Baza PWM dəyərlərini təyin et
    g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;
    g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;
    
    // Load PID parameters and settings from flash memory (load first)
    AdvancedPressureControl_LoadPIDParamsFromFlash();
    
    // Apply PID tuning values from pressure_control_config system
    // This ensures that g_pid_zme_tuning and g_pid_drv_tuning values are applied
    // even if flash loading happened with old values
    extern PID_Tuning_t g_pid_zme_tuning;
    extern PID_Tuning_t g_pid_drv_tuning;
    AdvancedPressureControl_SetPIDParams(&g_pid_zme, 
                                         g_pid_zme_tuning.kp, 
                                         g_pid_zme_tuning.ki, 
                                         g_pid_zme_tuning.kd);
    AdvancedPressureControl_SetPIDParams(&g_pid_drv, 
                                         g_pid_drv_tuning.kp, 
                                         g_pid_drv_tuning.ki, 
                                         g_pid_drv_tuning.kd);
    
    // DÜZƏLİŞ: Kalibrasiya məlumatları artıq main.c-də yüklənib
    // AdvancedPressureControl_LoadCalibration() main.c-də çağırılır, burada təkrar yükləmə lazım deyil
    // Əgər kalibrasiya yüklənməyibsə, default dəyərlər istifadə olunur
    
    // Təhlükəsizlik sistemini aktivləşdir
    AdvancedPressureControl_EnableSafety(true);
    
    // İdarəetməni aktivləşdir
    g_system_status.control_enabled = true;
    g_control_initialized = true;
    
    printf("Advanced Pressure Control System - Initialized\r\n");
    printf("Target Pressure: %.1f bar\r\n", g_system_status.target_pressure);
    printf("Safety System: %s\r\n", g_safety_limits.safety_enabled ? "Enabled" : "Disabled");
    printf("PID Parameters: Kp=%.3f, Ki=%.4f, Kd=%.3f\r\n", 
           g_pid_zme.Kp, g_pid_zme.Ki, g_pid_zme.Kd);
}

/**
 * @brief Sistem sıfırlama
 */
void AdvancedPressureControl_Reset(void) {
    printf("Advanced Pressure Control System - Resetting...\r\n");
    
    // PID nəzarətçilərini sıfırla
    g_pid_zme.integral_sum = 0.0f;
    g_pid_zme.previous_error = 0.0f;
    g_pid_drv.integral_sum = 0.0f;
    g_pid_drv.previous_error = 0.0f;
    
    // Status sıfırla
    g_system_status.safety_triggered = false;
    
    // Klapanları təhlükəsiz vəziyyətə gətir
    g_system_status.zme_pwm_percent = ZME_PWM_MAX; // ZME bağlı
    g_system_status.drv_pwm_percent = DRV_PWM_MIN; // DRV açıq
    g_system_status.motor_pwm_percent = 0.0f; // Motor dayandır
    
    AdvancedPressureControl_ApplyValveOutputs();
    AdvancedPressureControl_SetMotor_PWM(0.0f);
    
    printf("Advanced Pressure Control System - Reset Complete\r\n");
}

/* =========================================================================
   XII. KONFİQURASİYA VƏ MONİTORİNQ
   ========================================================================= */

/**
 * @brief Hədəf təzyiq təyin et
 */
void AdvancedPressureControl_SetTargetPressure(float pressure) {
    g_system_status.target_pressure = AdvancedPressureControl_ClampValue(
        pressure, PRESSURE_MIN, PRESSURE_MAX);
    printf("Target Pressure Set: %.1f bar\r\n", g_system_status.target_pressure);
}

/**
 * @brief Status yenilə
 */
void AdvancedPressureControl_UpdateStatus(void) {
    // Status strukturunda bütün dəyərlər artıq yenilənib
    // Bu funksiya gələcək genişlənmələr üçün qalıb
}

/**
 * @brief Status al
 */
SystemStatus_t* AdvancedPressureControl_GetStatus(void) {
    return &g_system_status;
}

/**
 * @brief Status məlumatlarını çap et
 */
void AdvancedPressureControl_PrintStatus(void) {
    printf("SP:%.1f  P:%.1f  ERR:%.2f  PID:%.2f  ZME:%.1f%%  DRV:%.1f%%  MOTOR:%.1f%%\r\n",
           g_system_status.target_pressure, g_system_status.current_pressure, 
           g_system_status.error, g_system_status.pid_output,
           g_system_status.zme_pwm_percent, g_system_status.drv_pwm_percent, 
           g_system_status.motor_pwm_percent);
}

/**
 * @brief Debug məlumatlarını çap et
 */
void AdvancedPressureControl_PrintDebugInfo(void) {
    printf("\n=== Advanced Pressure Control Debug Info ===\r\n");
    printf("Target Pressure: %.1f bar\r\n", g_system_status.target_pressure);
    printf("Current Pressure: %.1f bar\r\n", g_system_status.current_pressure);
    printf("Error: %.1f bar\r\n", g_system_status.error);
    printf("PID Output: %.1f\r\n", g_system_status.pid_output);
    printf("ZME PWM: %.1f%% (Base: %.1f%%)\r\n", g_system_status.zme_pwm_percent, g_zme_base_pwm);
    printf("DRV PWM: %.1f%% (Base: %.1f%%)\r\n", g_system_status.drv_pwm_percent, g_drv_base_pwm);
    printf("Motor PWM: %.1f%%\r\n", g_system_status.motor_pwm_percent);
    printf("PID Params: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", 
           g_pid_zme.Kp, g_pid_zme.Ki, g_pid_zme.Kd);
    printf("Dead Band: ±%.2f bar\r\n", DEAD_BAND_BAR);
    printf("Control Enabled: %s\r\n", g_system_status.control_enabled ? "Yes" : "No");
    printf("Safety Triggered: %s\r\n", g_system_status.safety_triggered ? "Yes" : "No");
    printf("==========================================\r\n\n");
}

/* =========================================================================
   XIII. ZME QEYRİ-XƏTTİLİK KOMPENSASİYASI
   ========================================================================= */

/**
 * @brief ZME-nin qeyri-xətti davranışını kompensasiya et
 * 
 * DÜZƏLİŞ: Kompensasiya funksiyası ləğv edildi
 * 
 * Səbəb: ZME (Normally Open) klapanı üçün 0.0% tam açıq deməkdir.
 * Əgər PID 0.5% (açılmağa doğru) tələb edirsə, bu klapanın açılması deməkdir.
 * Kompensasiya tətbiq edildikdə, 0.5% dəyəri 0.0% qaytarılır, bu da:
 * 1. PID-nin incə tənzimləmə işini pozur
 * 2. Klapanın sıçrayışına (jumping) səbəb olur
 * 3. 0.0% ilə 1.0% arasında kəskin dəyişiklik yaradır
 * 
 * Həll: Kompensasiya funksiyası ləğv edildi - PID dəyərlərini birbaşa istifadə edirik.
 * Əgər ZME minimum işlək eşiyindən aşağıda işləmirsə, PID özü bunu tənzimləyəcək.
 */
float AdvancedPressureControl_CompensateZME_Nonlinearity(float desired_pwm) {
    // Kompensasiya ləğv edildi - PID dəyərlərini birbaşa istifadə edirik
    // Bu, PID-nin incə tənzimləmə işini pozmur və klapanın sıçrayışını aradan qaldırır
    return desired_pwm;
}

/* =========================================================================
   XIV. KALİBRASİYA FUNKSİYALARI
   ========================================================================= */

/**
 * @brief Sensor kalibrasiyası
 */
void AdvancedPressureControl_CalibrateSensor(void) {
    printf("Pressure Sensor Calibration - Starting...\r\n");
    
    // Kalibrasiya məlumatlarını yadda saxla
    g_calibration.calibrated = true;
    
    printf("Pressure Sensor Calibration - Complete\r\n");
}

/**
 * @brief Kalibrasiya məlumatlarını flash yaddaşdan yüklə
 * 
 * DÜZƏLİŞ: Bu funksiya artıq flash yaddaşdan birbaşa oxuyur (PressureSensor_LoadCalibration() əvəzinə)
 */
void AdvancedPressureControl_LoadCalibration(void) {
    printf("Loading pressure sensor calibration from flash...\r\n");
    
    // Flash memory address for calibration data (offset 0x100 for calibration)
    uint32_t flash_address = 0x080E0000 + 0x100;  // Last 128KB sector, offset 0x100
    
    // Read calibration data from flash
    typedef struct {
        uint32_t magic;           // Magic number: 0x12345678
        float min_voltage;        // 0.5V
        float max_voltage;        // 5.0V
        float min_pressure;       // 0.0 bar
        float max_pressure;       // 300.0 bar
        uint16_t adc_min;         // 620 (DÜZƏLİŞ: əvvəl 410 idi)
        uint16_t adc_max;         // 4095 (DÜZƏLİŞ: 12-bit ADC maksimum dəyəri, 4096 deyil!)
        uint32_t checksum;        // Data integrity check
    } calibration_data_t;
    
    calibration_data_t *cal_data = (calibration_data_t*)flash_address;
    
    printf("Flash address: 0x%08lX\r\n", (uint32_t)flash_address);
    printf("Read magic: 0x%08lX (expected: 0x%08lX)\r\n", cal_data->magic, 0x12345678UL);
    
    // Check if calibration data is valid
    if (cal_data->magic == 0x12345678) {
        // Verify checksum
        uint32_t *min_v_ptr = (uint32_t*)&cal_data->min_voltage;
        uint32_t *max_v_ptr = (uint32_t*)&cal_data->max_voltage;
        uint32_t *min_p_ptr = (uint32_t*)&cal_data->min_pressure;
        uint32_t *max_p_ptr = (uint32_t*)&cal_data->max_pressure;
        uint32_t calculated_checksum = 0x12345678 ^ *min_v_ptr ^ *max_v_ptr ^ 
                                      *min_p_ptr ^ *max_p_ptr ^ 
                                      cal_data->adc_min ^ cal_data->adc_max;
        
        printf("Read checksum: 0x%08lX, Calculated: 0x%08lX\r\n",
               cal_data->checksum, calculated_checksum);
        
        if (calculated_checksum == cal_data->checksum) {
            bool cal_ok = AdvancedPressureControl_IsCalibrationRangeValid(
                cal_data->adc_min,
                cal_data->adc_max,
                cal_data->min_pressure,
                cal_data->max_pressure);

            if (cal_ok) {
                // Valid calibration data found - load it
                g_calibration.adc_min = (float)cal_data->adc_min;
                g_calibration.adc_max = (float)cal_data->adc_max;
                g_calibration.pressure_min = cal_data->min_pressure;
                g_calibration.pressure_max = cal_data->max_pressure;
                
                // Calculate slope and offset
                g_calibration.slope = (cal_data->max_pressure - cal_data->min_pressure) / 
                                     (float)(cal_data->adc_max - cal_data->adc_min);
                // Offset hesabla: offset = pressure_min - (slope * adc_min)
                g_calibration.offset = cal_data->min_pressure - 
                                      (g_calibration.slope * (float)cal_data->adc_min);
                
                g_calibration.calibrated = true;
                PressureControlConfig_UpdateCalibrationCache(&g_calibration);
                
                printf("AdvancedPressureControl: Calibration loaded - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar, Slope: %.6f, Offset: %.2f\r\n",
                       g_calibration.adc_min, g_calibration.adc_max, 
                       g_calibration.pressure_min, g_calibration.pressure_max,
                       g_calibration.slope, g_calibration.offset);
                return;  // Successfully loaded calibration
            } else {
                printf("WARNING: Invalid calibration values in flash (ADC: %u-%u, Pressure: %.2f-%.2f), using defaults\r\n",
                       cal_data->adc_min, cal_data->adc_max,
                       cal_data->min_pressure, cal_data->max_pressure);
            }
        } else {
            printf("Checksum mismatch, using defaults\r\n");
        }
    } else {
        printf("No valid calibration data found in flash, using defaults\r\n");
    }
    
    // No valid calibration data found - use defaults
    g_calibration.adc_min = (float)ADC_MIN;
    g_calibration.adc_max = (float)ADC_MAX;
    g_calibration.pressure_min = PRESSURE_MIN;
    g_calibration.pressure_max = PRESSURE_MAX;
    g_calibration.slope = PRESSURE_SLOPE;
    // DÜZƏLİŞ: Offset hesabla: offset = pressure_min - (slope * adc_min)
    g_calibration.offset = PRESSURE_MIN - (PRESSURE_SLOPE * (float)ADC_MIN);
    g_calibration.calibrated = true;
    PressureControlConfig_UpdateCalibrationCache(&g_calibration);
    
    printf("AdvancedPressureControl: Using default calibration - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar, Slope: %.6f, Offset: %.2f\r\n",
           g_calibration.adc_min, g_calibration.adc_max, 
           g_calibration.pressure_min, g_calibration.pressure_max,
           g_calibration.slope, g_calibration.offset);
}

/**
 * @brief Kalibrasiya məlumatlarını flash yaddaşa yaz
 * 
 * DÜZƏLİŞ: Bu funksiya artıq flash yaddaşa birbaşa yazır (PressureSensor_SaveCalibration() əvəzinə)
 */
void AdvancedPressureControl_SaveCalibration(void) {
    printf("Saving pressure sensor calibration to flash...\r\n");
    
    // Prepare calibration data structure
    typedef struct {
        uint32_t magic;           // Magic number: 0x12345678
        float min_voltage;        // 0.5V (not used in Advanced system, but kept for compatibility)
        float max_voltage;        // 5.0V (not used in Advanced system, but kept for compatibility)
        float min_pressure;       // 0.0 bar
        float max_pressure;       // 300.0 bar
        uint16_t adc_min;         // 620 (DÜZƏLİŞ: əvvəl 410 idi)
        uint16_t adc_max;         // 4095 (DÜZƏLİŞ: 12-bit ADC maksimum dəyəri, 4096 deyil!)
        uint32_t checksum;        // Data integrity check
    } calibration_data_t;
    
    calibration_data_t cal_data;
    cal_data.magic = 0x12345678;
    cal_data.min_voltage = 0.5f;   // Default (not used in Advanced system)
    cal_data.max_voltage = 5.0f;   // Default (not used in Advanced system)
    cal_data.min_pressure = g_calibration.pressure_min;
    cal_data.max_pressure = g_calibration.pressure_max;
    // DÜZƏLİŞ: Float dəyərləri uint16_t-ə çevirərkən düzgün yuvarlaqlaşdırma
    cal_data.adc_min = (uint16_t)(g_calibration.adc_min + 0.5f);  // Round to nearest integer
    cal_data.adc_max = (uint16_t)(g_calibration.adc_max + 0.5f);  // Round to nearest integer
    
    printf("DEBUG: Saving calibration - ADC: %d-%d, Pressure: %.2f-%.2f bar\r\n",
           cal_data.adc_min, cal_data.adc_max, cal_data.min_pressure, cal_data.max_pressure);
    PressureControlConfig_UpdateCalibrationCache(&g_calibration);
    
    // Calculate checksum
    uint32_t *min_v_ptr = (uint32_t*)&cal_data.min_voltage;
    uint32_t *max_v_ptr = (uint32_t*)&cal_data.max_voltage;
    uint32_t *min_p_ptr = (uint32_t*)&cal_data.min_pressure;
    uint32_t *max_p_ptr = (uint32_t*)&cal_data.max_pressure;
    cal_data.checksum = 0x12345678 ^ *min_v_ptr ^ *max_v_ptr ^ *min_p_ptr ^ *max_p_ptr ^ 
                       cal_data.adc_min ^ cal_data.adc_max;
    
    // KRİTİK DÜZƏLİŞ: Mərkəzləşdirilmiş Flash Yaddaş Məntiqindən istifadə et
    // block_type = 1 (Calibration)
    HAL_StatusTypeDef status = AdvancedPressureControl_SaveToFlash_Centralized(
        1,  // block_type: Calibration
        &cal_data,
        sizeof(calibration_data_t)
    );
    
    if (status == HAL_OK) {
        // Verify data was written correctly
        uint32_t flash_address = 0x080E0000 + 0x100;
        calibration_data_t *verify_data = (calibration_data_t*)flash_address;
        if (verify_data->magic == 0x12345678 && verify_data->checksum == cal_data.checksum) {
            printf("AdvancedPressureControl: Calibration saved and verified - ADC: %d-%d, Pressure: %.2f-%.2f bar\r\n",
                   cal_data.adc_min, cal_data.adc_max, cal_data.min_pressure, cal_data.max_pressure);
            printf("AdvancedPressureControl: Current calibration - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar, Slope: %.6f, Offset: %.2f\r\n",
                   g_calibration.adc_min, g_calibration.adc_max, 
                   g_calibration.pressure_min, g_calibration.pressure_max,
                   g_calibration.slope, g_calibration.offset);
        } else {
            printf("ERROR: Calibration write verification failed!\r\n");
            printf("ERROR: Magic: 0x%08lX (expected: 0x%08lX), Checksum: 0x%08lX (expected: 0x%08lX)\r\n",
                   verify_data->magic, 0x12345678UL, verify_data->checksum, cal_data.checksum);
        }
    } else {
        printf("ERROR: Failed to save calibration to flash!\r\n");
    }
}

/**
 * @brief Kalibrasiya statusunu yoxla
 */
bool AdvancedPressureControl_IsCalibrated(void) {
    return g_calibration.calibrated;
}

/**
 * @brief Təhlükəsizlik sistemini aktivləşdir/deaktivləşdir
 */
void AdvancedPressureControl_EnableSafety(bool enable) {
    g_safety_limits.safety_enabled = enable;
    printf("Safety System: %s\r\n", enable ? "Enabled" : "Disabled");
}

/**
 * @brief ZME baza PWM dəyərini təyin et (Kalibrasiya üçün)
 * @param base_pwm: Baza PWM dəyəri (0-30%)
 */
void AdvancedPressureControl_SetZME_BasePWM(float base_pwm) {
    g_zme_base_pwm = AdvancedPressureControl_ClampValue(base_pwm, ZME_PWM_MIN, ZME_PWM_MAX);
    printf("ZME Base PWM Set: %.1f%%\r\n", g_zme_base_pwm);
}

/**
 * @brief DRV baza PWM dəyərini təyin et (Kalibrasiya üçün)
 * @param base_pwm: Baza PWM dəyəri (0-40%)
 */
void AdvancedPressureControl_SetDRV_BasePWM(float base_pwm) {
    g_drv_base_pwm = AdvancedPressureControl_ClampValue(base_pwm, DRV_PWM_MIN, DRV_PWM_MAX);
    printf("DRV Base PWM Set: %.1f%%\r\n", g_drv_base_pwm);
}

/**
 * @brief ZME baza PWM dəyərini al
 * @retval Baza PWM dəyəri
 */
float AdvancedPressureControl_GetZME_BasePWM(void) {
    return g_zme_base_pwm;
}

/**
 * @brief DRV baza PWM dəyərini al
 * @retval Baza PWM dəyəri
 */
float AdvancedPressureControl_GetDRV_BasePWM(void) {
    return g_drv_base_pwm;
}

/* =========================================================================
   XV. FLASH YADDAŞ FUNKSİYALARI
   ========================================================================= */

// Flash memory structure for AdvancedPressureControl parameters
// Note: Calibration data is at offset 0x100, PID params are at offset 0x000
#define FLASH_ADV_CONFIG_ADDRESS    (0x080E0000 + 0x000)  /* Sector 11, offset 0x000 for PID params */
#define FLASH_ADV_CONFIG_MAGIC      0xABCDEF12  /* Magic number to verify valid data */

/**
 * @brief Mərkəzləşdirilmiş Flash Yaddaş Məntiqi - Bütün blokları birlikdə idarə edir
 * 
 * KRİTİK: Bu funksiya bütün Flash yazma əməliyyatlarını mərkəzləşdirir
 * Sector 11-də 3 fərqli blok var:
 * - 0x000: Advanced PID params (FLASH_ADV_CONFIG_ADDRESS)
 * - 0x100: Calibration data (FLASH_CALIBRATION_ADDRESS)
 * - 0x200: Config PID params (FLASH_CONFIG_ADDRESS)
 * 
 * Bu funksiya:
 * 1. Bütün 3 bloku yadda saxlayır
 * 2. Sektoru silir
 * 3. Bütün blokları bərpa edir
 * 4. Yeni bloku yazır (əgər təmin edilibsə)
 * 
 * ⚠️ KRİTİK TƏHLÜKƏSİZLİK XƏBƏRDARLIĞI:
 * Sektorun silinməsi və məlumatların bərpası prosesi atomik deyil (bir anda baş vermir).
 * Əgər bu bərpa prosesi zamanı (bir neçə yüz milisaniyə çəkə bilər) enerji kəsilməsi baş verərsə,
 * bütün üç məlumat bloku (PID parametrləri, Kalibrasiya və Konfiqurasiya) tamamilə itiriləcəkdir.
 * 
 * Tövsiyə olunan həllər:
 * - EEPROM Emulyasiyası (Flash üzərində log strukturu)
 * - Çift-Sektor (A/B Swap) metodu (köhnə məlumatlar yeni məlumatlar tam yazılana qədər saxlanılır)
 * - UPS (Uninterruptible Power Supply) istifadəsi
 * 
 * KRİTİK: Bu funksiya bütün Flash yazma əməliyyatlarını mərkəzləşdirir və
 * digər funksiyalardan bütün bərpa məntiqini silir.
 * 
 * @param block_type: Yazılacaq blok növü (0=Advanced PID, 1=Calibration, 2=Config PID)
 * @param data_ptr: Yazılacaq məlumatın pointer-i (NULL ola bilər)
 * @param data_size: Məlumatın ölçüsü (bytes)
 * @retval HAL_OK if successful, HAL_ERROR otherwise
 */
HAL_StatusTypeDef AdvancedPressureControl_SaveToFlash_Centralized(
    uint8_t block_type, void* data_ptr, uint32_t data_size) {
    
    // Flash ünvanları
    uint32_t pid_flash_addr = 0x080E0000 + 0x000;      // Offset 0x000
    uint32_t cal_flash_addr = 0x080E0000 + 0x100;      // Offset 0x100
    uint32_t config_flash_addr = 0x080E0000 + 0x200;   // Offset 0x200
    
    // Data strukturları
    typedef struct {
        uint32_t magic;
        float pid_kp, pid_ki, pid_kd;
        float target_pressure;
        float zme_base_pwm, drv_base_pwm;
        uint32_t checksum;
    } Flash_Adv_Config_Data_t;
    
    typedef struct {
        uint32_t magic;
        float min_voltage, max_voltage;
        float min_pressure, max_pressure;
        uint16_t adc_min, adc_max;
        uint32_t checksum;
    } Calibration_Flash_Data_t;
    
    typedef struct {
        uint32_t magic;
        float pid_kp, pid_ki, pid_kd;
        float setpoint;
        uint32_t checksum;
    } Config_Flash_Data_t;
    
    // Unlock flash
    HAL_StatusTypeDef unlock_status = HAL_FLASH_Unlock();
    if (unlock_status != HAL_OK) {
        printf("Flash unlock failed! Status: %d\r\n", unlock_status);
        return HAL_ERROR;
    }
    
    // 1. Bütün 3 bloku yadda saxla
    Flash_Adv_Config_Data_t *pid_data = (Flash_Adv_Config_Data_t*)pid_flash_addr;
    Calibration_Flash_Data_t *cal_data = (Calibration_Flash_Data_t*)cal_flash_addr;
    Config_Flash_Data_t *config_data = (Config_Flash_Data_t*)config_flash_addr;
    
    Flash_Adv_Config_Data_t saved_pid = {0};
    Calibration_Flash_Data_t saved_cal = {0};
    Config_Flash_Data_t saved_config = {0};
    bool pid_exists = false;
    bool cal_exists = false;
    bool config_exists = false;
    
    if (pid_data->magic == 0xABCDEF12) {
        saved_pid = *pid_data;
        pid_exists = true;
        printf("Centralized Flash: Saving Advanced PID before erase\r\n");
    }
    
    if (cal_data->magic == 0x12345678) {
        saved_cal = *cal_data;
        cal_exists = true;
        printf("Centralized Flash: Saving Calibration before erase\r\n");
    }
    
    if (config_data->magic == 0x87654321) {
        saved_config = *config_data;
        config_exists = true;
        printf("Centralized Flash: Saving Config PID before erase\r\n");
    }
    
    // 2. Sektoru sil
    // KRİTİK: Flash yazma əməliyyatı zamanı elektrik kəsilməsi riski var
    // Bütün məlumatlar RAM-də yadda saxlanılıb, sektor silinir, sonra bərpa edilir
    // Bu proses zamanı elektrik kəsilsə, bütün məlumatlar itirilə bilər
    // QEYD: Bu, Flash yaddaşın təbiəti ilə bağlıdır - sektor silmə əməliyyatı atomik deyil
    // Daha təhlükəsiz həll üçün EEPROM Emulyasiya Layeri və ya TƏK master struktur lazımdır
    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = FLASH_SECTOR_11;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    uint32_t sector_error = 0;
    HAL_StatusTypeDef erase_status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    if (erase_status != HAL_OK) {
        printf("Flash erase failed! Status: %d, Sector Error: %lu\r\n", erase_status, sector_error);
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }
    printf("Centralized Flash: Sector erased successfully\r\n");
    
    // KRİTİK: Bərpa prosesi zamanı elektrik kəsilməsi riski var
    // Əgər bərpa prosesi başa çatmamış elektrik kəsilsə, bütün məlumatlar itiriləcək
    // 3. Bütün blokları bərpa et (yeni blokdan başqa)
    if (pid_exists && block_type != 0) {
        uint32_t *pid_ptr = (uint32_t*)&saved_pid;
        uint32_t pid_size = sizeof(Flash_Adv_Config_Data_t);
        for (uint32_t i = 0; i < (pid_size / 4); i++) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, pid_flash_addr + (i * 4), pid_ptr[i]);
        }
        printf("Centralized Flash: Advanced PID restored\r\n");
    }
    
    if (cal_exists && block_type != 1) {
        uint32_t *cal_ptr = (uint32_t*)&saved_cal;
        uint32_t cal_size = sizeof(Calibration_Flash_Data_t);
        for (uint32_t i = 0; i < (cal_size / 4); i++) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, cal_flash_addr + (i * 4), cal_ptr[i]);
        }
        printf("Centralized Flash: Calibration restored\r\n");
    }
    
    if (config_exists && block_type != 2) {
        uint32_t *config_ptr = (uint32_t*)&saved_config;
        uint32_t config_size = sizeof(Config_Flash_Data_t);
        for (uint32_t i = 0; i < (config_size / 4); i++) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, config_flash_addr + (i * 4), config_ptr[i]);
        }
        printf("Centralized Flash: Config PID restored\r\n");
    }
    
    // 4. Yeni bloku yaz (əgər təmin edilibsə)
    if (data_ptr != NULL && data_size > 0) {
        uint32_t target_addr = 0;
        switch (block_type) {
            case 0: target_addr = pid_flash_addr; break;
            case 1: target_addr = cal_flash_addr; break;
            case 2: target_addr = config_flash_addr; break;
            default:
                HAL_FLASH_Lock();
                return HAL_ERROR;
        }
        
        uint32_t *data_words = (uint32_t*)data_ptr;
        uint32_t word_count = (data_size + 3) / 4;  // Round up to word count
        
        for (uint32_t i = 0; i < word_count; i++) {
            HAL_StatusTypeDef write_status = HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_WORD,
                target_addr + (i * 4),
                data_words[i]);
            
            if (write_status != HAL_OK) {
                printf("Flash write failed at offset %lu!\r\n", i * 4);
                HAL_FLASH_Lock();
                return HAL_ERROR;
            }
        }
        printf("Centralized Flash: New block written (type=%d)\r\n", block_type);
    }
    
    // Lock flash
    HAL_FLASH_Lock();
    return HAL_OK;
}

typedef struct {
    uint32_t magic;              /* Magic number: 0xABCDEF12 */
    float pid_kp;                /* Kp parameter */
    float pid_ki;                /* Ki parameter */
    float pid_kd;                /* Kd parameter */
    float target_pressure;       /* Target pressure (SP) */
    float zme_base_pwm;          /* ZME base PWM */
    float drv_base_pwm;          /* DRV base PWM */
    uint32_t checksum;           /* Simple checksum */
} Flash_Adv_Config_Data_t;

/**
 * @brief Calculate simple checksum for AdvancedPressureControl config
 */
static uint32_t CalculateAdvChecksum(float kp, float ki, float kd, float sp, float zme_base, float drv_base) {
    uint32_t *kp_ptr = (uint32_t*)&kp;
    uint32_t *ki_ptr = (uint32_t*)&ki;
    uint32_t *kd_ptr = (uint32_t*)&kd;
    uint32_t *sp_ptr = (uint32_t*)&sp;
    uint32_t *zme_ptr = (uint32_t*)&zme_base;
    uint32_t *drv_ptr = (uint32_t*)&drv_base;
    return FLASH_ADV_CONFIG_MAGIC ^ *kp_ptr ^ *ki_ptr ^ *kd_ptr ^ *sp_ptr ^ *zme_ptr ^ *drv_ptr;
}

/**
 * @brief Save AdvancedPressureControl PID parameters and settings to flash
 */
void AdvancedPressureControl_SavePIDParamsToFlash(void) {
    printf("Saving AdvancedPressureControl parameters to flash...\r\n");
    
    // Prepare data structure
    Flash_Adv_Config_Data_t config_data;
    config_data.magic = FLASH_ADV_CONFIG_MAGIC;
    config_data.pid_kp = g_pid_zme.Kp;
    config_data.pid_ki = g_pid_zme.Ki;
    config_data.pid_kd = g_pid_zme.Kd;
    config_data.target_pressure = g_system_status.target_pressure;
    config_data.zme_base_pwm = g_zme_base_pwm;
    config_data.drv_base_pwm = g_drv_base_pwm;
    
    // Calculate checksum
    config_data.checksum = CalculateAdvChecksum(
        config_data.pid_kp, config_data.pid_ki, 
        config_data.pid_kd, config_data.target_pressure,
        config_data.zme_base_pwm, config_data.drv_base_pwm);
    
    // KRİTİK DÜZƏLİŞ: Mərkəzləşdirilmiş Flash Yaddaş Məntiqindən istifadə et
    // block_type = 0 (Advanced PID)
    HAL_StatusTypeDef status = AdvancedPressureControl_SaveToFlash_Centralized(
        0,  // block_type: Advanced PID
        &config_data,
        sizeof(Flash_Adv_Config_Data_t)
    );
    
    if (status == HAL_OK) {
        // Verify data was written correctly by reading it back
        Flash_Adv_Config_Data_t *verify_data = (Flash_Adv_Config_Data_t*)FLASH_ADV_CONFIG_ADDRESS;
        if (verify_data->magic == FLASH_ADV_CONFIG_MAGIC && 
            verify_data->checksum == config_data.checksum) {
            printf("Parameters saved and verified: Kp=%.3f, Ki=%.3f, Kd=%.3f, SP=%.1f, ZME_BASE=%.1f, DRV_BASE=%.1f\r\n",
                   config_data.pid_kp, config_data.pid_ki, 
                   config_data.pid_kd, config_data.target_pressure,
                   config_data.zme_base_pwm, config_data.drv_base_pwm);
        } else {
            printf("ERROR: Flash write verification failed!\r\n");
            printf("Written magic: 0x%08X, Read magic: 0x%08X\r\n", 
                   (unsigned int)config_data.magic, (unsigned int)verify_data->magic);
        }
    } else {
        printf("ERROR: Failed to save PID parameters to flash!\r\n");
    }
}

/**
 * @brief Load AdvancedPressureControl PID parameters and settings from flash
 */
void AdvancedPressureControl_LoadPIDParamsFromFlash(void) {
    printf("Loading AdvancedPressureControl parameters from flash...\r\n");
    
    // Read data from flash
    Flash_Adv_Config_Data_t *config_data = (Flash_Adv_Config_Data_t*)FLASH_ADV_CONFIG_ADDRESS;
    
    printf("Flash address: 0x%08X\r\n", (unsigned int)FLASH_ADV_CONFIG_ADDRESS);
    printf("Read magic: 0x%08X (expected: 0x%08X)\r\n", 
           (unsigned int)config_data->magic, (unsigned int)FLASH_ADV_CONFIG_MAGIC);
    
    // Verify magic number
    if (config_data->magic != FLASH_ADV_CONFIG_MAGIC) {
        printf("No valid AdvancedPressureControl configuration found in flash, using defaults\r\n");
        // DÜZƏLİŞ: Flash-dan dəyər yoxdursa, default dəyərləri təyin et (Kp=0.100, Ki=0.0000)
        AdvancedPressureControl_SetPIDParams(&g_pid_zme, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
        AdvancedPressureControl_SetPIDParams(&g_pid_drv, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
        // KRİTİK DÜZƏLİŞ: ZME base PWM dəyərini də default-a təyin et (26%)
        g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;
        g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;
        printf("Default PID parameters set: Kp=%.3f, Ki=%.4f, Kd=%.3f, ZME_BASE=%.1f%%, DRV_BASE=%.1f%%\r\n", 
               PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT, g_zme_base_pwm, g_drv_base_pwm);
        return;
    }
    
    // Verify checksum
    uint32_t calculated_checksum = CalculateAdvChecksum(
        config_data->pid_kp, config_data->pid_ki,
        config_data->pid_kd, config_data->target_pressure,
        config_data->zme_base_pwm, config_data->drv_base_pwm);
    
    printf("Read checksum: 0x%08lX, Calculated: 0x%08lX\r\n",
           config_data->checksum, calculated_checksum);
    
    if (calculated_checksum != config_data->checksum) {
        printf("Checksum mismatch, using defaults\r\n");
        // DÜZƏLİŞ: Checksum uyğun gəlmirsə, default dəyərləri təyin et (Kp=0.100, Ki=0.0000)
        AdvancedPressureControl_SetPIDParams(&g_pid_zme, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
        AdvancedPressureControl_SetPIDParams(&g_pid_drv, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
        // KRİTİK DÜZƏLİŞ: ZME base PWM dəyərini də default-a təyin et (26%)
        g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;
        g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;
        printf("Default PID parameters set: Kp=%.3f, Ki=%.4f, Kd=%.3f, ZME_BASE=%.1f%%, DRV_BASE=%.1f%%\r\n", 
               PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT, g_zme_base_pwm, g_drv_base_pwm);
        return;
    }
    
    // Load PID parameters
    // Check if loaded values are old defaults and update to new defaults if so
    // Old defaults: Kp=0.5, Ki=0.0010, Kd=0.0 (advanced_pressure_control.h)
    // Old defaults: Kp=1.5, Ki=0.005, Kd=0.0 (pressure_control_config.h)
    bool is_old_default_1 = (fabsf(config_data->pid_kp - 0.5f) < 0.01f) && 
                            (fabsf(config_data->pid_ki - 0.0010f) < 0.0001f) && 
                            (fabsf(config_data->pid_kd - 0.0f) < 0.01f);
    
    bool is_old_default_2 = (fabsf(config_data->pid_kp - 1.5f) < 0.01f) && 
                            (fabsf(config_data->pid_ki - 0.005f) < 0.0001f) && 
                            (fabsf(config_data->pid_kd - 0.0f) < 0.01f);
    
    if (is_old_default_1 || is_old_default_2) {
        printf("Old default PID values detected in flash (Kp=%.3f, Ki=%.4f, Kd=%.3f), updating to new defaults\r\n",
               config_data->pid_kp, config_data->pid_ki, config_data->pid_kd);
        AdvancedPressureControl_SetPIDParams(&g_pid_zme, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
        AdvancedPressureControl_SetPIDParams(&g_pid_drv, PID_KP_DEFAULT, PID_KI_DEFAULT, PID_KD_DEFAULT);
    } else {
        AdvancedPressureControl_SetPIDParams(&g_pid_zme, config_data->pid_kp, config_data->pid_ki, config_data->pid_kd);
        AdvancedPressureControl_SetPIDParams(&g_pid_drv, config_data->pid_kp, config_data->pid_ki, config_data->pid_kd);
    }
    
    // Load target pressure
    if (config_data->target_pressure > 0.1f && config_data->target_pressure <= 300.0f) {
        AdvancedPressureControl_SetTargetPressure(config_data->target_pressure);
        printf("Target pressure loaded from flash: %.1f bar\r\n", config_data->target_pressure);
    } else {
        // Flash-dan etibarlı dəyər yoxdursa, default dəyəri təyin et
        AdvancedPressureControl_SetTargetPressure(70.0f);
        printf("WARNING: Invalid target_pressure in flash (%.1f), using default 70.0 bar\r\n", 
               config_data->target_pressure);
    }
    
    // Load base PWM values
    // KRİTİK DÜZƏLİŞ: Flash-dan köhnə dəyər (20%) yüklənəndə, yeni default dəyəri (26%) istifadə et
    if (config_data->zme_base_pwm >= 0.0f && config_data->zme_base_pwm <= 30.0f) {
        // Flash-dan dəyər var, amma köhnə default dəyərdirsə (20%), yeni default dəyəri (26%) istifadə et
        if (fabsf(config_data->zme_base_pwm - 20.0f) < 0.1f) {
            // Köhnə default dəyərdir (20%), yeni default dəyəri (26%) istifadə et
            g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;  // 26%
            printf("ZME base PWM updated from old default (20%%) to new default (%.1f%%)\r\n", g_zme_base_pwm);
        } else {
            // Flash-dan dəyər etibarlıdır və köhnə default deyil, istifadə et
            g_zme_base_pwm = config_data->zme_base_pwm;
        }
    } else {
        // Flash-dan dəyər etibarsızdırsa, default dəyəri istifadə et (26%)
        g_zme_base_pwm = ZME_BASE_PWM_DEFAULT;
        printf("Invalid ZME base PWM in flash (%.1f%%), using default: %.1f%%\r\n", 
               config_data->zme_base_pwm, g_zme_base_pwm);
    }
    // KRİTİK DÜZƏLİŞ: Flash-dan köhnə dəyər (20%, 26%, 30%, 35% və ya 40%) yüklənəndə, yeni default dəyəri (15%) istifadə et
    if (config_data->drv_base_pwm >= 0.0f && config_data->drv_base_pwm <= 40.0f) {
        // Flash-dan dəyər var, amma köhnə default dəyərdirsə (20%, 26%, 30%, 35% və ya 40%), yeni default dəyəri (15%) istifadə et
        if (fabsf(config_data->drv_base_pwm - 20.0f) < 0.1f || 
            fabsf(config_data->drv_base_pwm - 26.0f) < 0.1f || 
            fabsf(config_data->drv_base_pwm - 30.0f) < 0.1f ||
            fabsf(config_data->drv_base_pwm - 35.0f) < 0.1f ||
            fabsf(config_data->drv_base_pwm - 40.0f) < 0.1f) {
            // Köhnə default dəyərdir (20%, 26%, 30%, 35% və ya 40%), yeni default dəyəri (15%) istifadə et
            g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;  // 15%
            printf("DRV base PWM updated from old default (%.1f%%) to new default (%.1f%%)\r\n", 
                   config_data->drv_base_pwm, g_drv_base_pwm);
        } else {
            // Flash-dan dəyər etibarlıdır və köhnə default deyil, istifadə et
            g_drv_base_pwm = config_data->drv_base_pwm;
        }
    } else {
        // Flash-dan dəyər etibarsızdırsa, default dəyəri istifadə et (15%)
        g_drv_base_pwm = DRV_BASE_PWM_DEFAULT;
        printf("Invalid DRV base PWM in flash (%.1f%%), using default: %.1f%%\r\n", 
               config_data->drv_base_pwm, g_drv_base_pwm);
    }
    
    printf("Parameters loaded: Kp=%.3f, Ki=%.3f, Kd=%.3f, SP=%.1f, ZME_BASE=%.1f, DRV_BASE=%.1f\r\n",
           config_data->pid_kp, config_data->pid_ki,
           config_data->pid_kd, config_data->target_pressure,
           config_data->zme_base_pwm, config_data->drv_base_pwm);
}

/* =========================================================================
   XV. TİMER KƏSİLMƏSİ HANDLER
   ========================================================================= */

/* 
 * QEYD: Timer 6 kəsilməsi main.c-də HAL_TIM_PeriodElapsedCallback() funksiyasında
 * birbaşa AdvancedPressureControl_Step() çağırır. Bu, kodun təmizliyini artırır.
 */

/* =========================================================================
   XVI. MISSING FUNCTION IMPLEMENTATIONS
   ========================================================================= */

/**
 * @brief ZME nonlinearity test function
 */
void AdvancedPressureControl_TestZME_Nonlinearity(void) {
    printf("ZME Nonlinearity Test - Starting...\r\n");
    
    // Test ZME at different PWM values
    float test_values[] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 5.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f};
    int test_count = sizeof(test_values) / sizeof(test_values[0]);
    
    for (int i = 0; i < test_count; i++) {
        float test_pwm = test_values[i];
        printf("Testing ZME at %.1f%% PWM\r\n", test_pwm);
        
        // Set ZME PWM
        AdvancedPressureControl_SetZME_PWM(test_pwm);
        HAL_Delay(1000); // Wait 1 second
        
        // Read pressure
        float pressure = AdvancedPressureControl_ReadPressure();
        printf("ZME=%.1f%%, Pressure=%.2f bar\r\n", test_pwm, pressure);
        
        HAL_Delay(500); // Wait between tests
    }
    
    // Reset to safe position
    AdvancedPressureControl_SetZME_PWM(ZME_PWM_MAX);
    printf("ZME Nonlinearity Test - Complete\r\n");
}
