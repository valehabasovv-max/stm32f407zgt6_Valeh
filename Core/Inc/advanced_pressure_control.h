/*
 * Advanced PID-based Pressure Control System for STM32F407ZGT6
 * Valeh Injection System - Complete Control Solution
 * 
 * Features:
 * - Precise PID control for ZME and DRV valves
 * - Motor speed control based on pressure limits
 * - Safety systems with emergency stop
 * - Calibration and parameter tuning
 * - Real-time monitoring and control
 */

#ifndef __ADVANCED_PRESSURE_CONTROL_H
#define __ADVANCED_PRESSURE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* =========================================================================
   I. KALİBRƏLƏMƏ SABİTLƏRİ (Öz Dəyərlərinizlə Doldurun)
   ========================================================================= */

// Təzyiq Sensoru (ADC)
// KRİTİK: 0.5V (0 bar) -> 410, 5.0V (300 bar) -> 4096 (5.0V Vref fərziyyəsi ilə)
#define ADC_MIN 410
#define ADC_MAX 4096
#define PRESSURE_MIN 0.0f
#define PRESSURE_MAX 300.0f
#define PRESSURE_SLOPE ((PRESSURE_MAX - PRESSURE_MIN) / (float)(ADC_MAX - ADC_MIN))
// PRESSURE_SLOPE təxminən 0.08139f olacaq

// ZME Klapanı (Normally Open - Tərs Məntiq)
#define ZME_PWM_MIN 0.0f     // Təzyiq Maksimum (Açıq)
#define ZME_PWM_MAX 30.0f    // Təzyiq Minimum (Bağlı)
#define ZME_CUTOFF_PWM 2.0f  // *** KRİTİK: ZME-nin yanacağı kəsdiyi nöqtə

// DRV Klapanı (Normally Open - Düz Məntiq)
#define DRV_PWM_MIN 0.0f     // Təzyiq Minimum (Açıq)
#define DRV_PWM_MAX 40.0f    // Təzyiq Maksimum (Bağlı)

// PID İdarəetmə Limitləri (klapan trim limitləri ilə eyni saxlanılır)
#define PID_OUTPUT_MIN (-PWM_TRIM_LIMIT)
#define PID_OUTPUT_MAX (PWM_TRIM_LIMIT)

// Motor İdarəetmə Limitləri (0-100% PWM)
#define MOTOR_MAX_PWM 100.0f

// Təhlükəsizlik Marjası (over limit problemi üçün aşağı salındı)
// DÜZƏLİŞ: 10 bar çox böyükdür - təzyiq artıq aşmış olur
#define PRESSURE_OVER_LIMIT_MARGIN 3.0f  // 3 bar margin (əvvəl 10 bar idi)

// Control Loop Timing
#define CONTROL_LOOP_TIME_MS 10  // 10ms control loop
#define CONTROL_LOOP_TIME_S 0.01f

// =========================================================================
// TƏKMİLLƏŞDİRİLMİŞ FINAL PID PARAMETRLƏRİ (Optimal Tuning Dəyərləri)
// =========================================================================
// Bu parametrlər Common Rail sistemləri üçün optimal tənzimləmə dəyərləridir.
// PID-nin gücünü Dead Band qoruyucusu ilə birləşdirir və titrämə problemini həll edir.
//
// Bu struktur PID-nin gücünü Dead Band qoruyucusu ilə birləşdirir və 
// titrämə problemini həll etməlidir. Proporsional İdarəetmənin ən optimal formasıdır.
//
// Tuning Qaydası (Təhlükəsiz Tuning):
// 1. Kp-ni yavaş-yavaş artırın (0.5-dən başlayaraq). DRV-nin hərəkəti hamar olmalıdır.
// 2. Ki-ni yavaş-yavaş artırın (0.0010-dan başlayaraq). Bu, təzyiqin dəqiq limitində dayanmasını təmin edəcək.
// 3. Kd-ni yalnız sensor səs-küyü varsa və filtrləmə kifayət etmirsə əlavə edin.
//
// Niyə Bu Parametrlər?
// - Həddindən Artıq Güclü Motor: Motorun cəmi 15.8% sürətlə çox böyük təzyiq yaratması,
//   Kp əmsalının çox kiçik bir dəyişikliyə belə kəskin reaksiya verməsinə səbəb olur.
// - Kəskin azaldılmış Kp: Titräməni (Hunting) aradan qaldırır.
// - Ki: Qalan xətanı aradan qaldırmaq üçün.
// - Kd: Sıfır (filtrləmə tətbiq olunarsa əlavə edilə bilər).
#define PID_KP_DEFAULT 0.8f      // Kp dəyəri
#define PID_KI_DEFAULT 0.05f     // Ki dəyəri
#define PID_KD_DEFAULT 0.01f     // Kd dəyəri

// =========================================================================
// BAZA VƏZİYYƏTLƏRİ (Təcrübi Tapılmalıdır)
// =========================================================================
// Bu dəyərlər sisteminizə xas olmalıdır və təcrübi yolla tapılmalıdır.
// Növbəti addım: 50 bar limitində ZME_BASE_PWM və DRV_BASE_PWM üçün təxmini təcrübi dəyərləri tapın.
// Bu, PID-nin işini yüngülləşdirəcək.
//
// Təcrübi tapmaq üçün:
// 1. Motoru sabit sürətə təyin edin (SP-yə görə)
// 2. ZME-ni müxtəlif PWM dəyərlərində test edin (0-30%)
// 3. Təzyiqin 50 bar-ı yaratdığı ZME PWM dəyərini tapın → ZME_BASE_PWM
// 4. DRV üçün də eyni prosesi təkrarlayın → DRV_BASE_PWM
#define ZME_BASE_PWM_DEFAULT 26.0f  // ZME-nin 50 barı yaratmaq üçün ideal mövqeyi (0-30%) - DÜZƏLİŞ: 20%-dən 26%-ə artırıldı
#define DRV_BASE_PWM_DEFAULT 26.0f  // DRV-nin sabit dayanmaq üçün ideal mövqeyi (0-40%)

// =========================================================================
// KLAPANLARIN QORUNMASI VƏ STABİLLƏŞDİRİLMƏSİ
// =========================================================================
// Dead Band: Titräməni dayandıran ölü zona. Təzyiq bu diapazonunda olduqda, 
//            PID hesablamaları atlanır və klapanlar hazırkı mövqeydə qalır.
//            Bu, titräməyə son qoyur və klapanların fasiləsiz açılıb-bağlanmasını qarşısını alır.
// PWM Trim Limit: PID-nin baza mövqeyindən nə qədər kənara çıxa biləcəyini məhdudlaşdırır.
//                 Bu, klapanın həddindən artıq sürətlə hərəkət etməsinin qarşısını alır
//                 və baza mövqedən nə qədər kənara çıxa biləcəyini məhdudlaşdırır.
#define DEAD_BAND_BAR 1.0f       // Titräməni dayandıran Ölü Zona (+/- 1.0 bar)
#define PWM_TRIM_LIMIT 20.0f     // PID-nin baza mövqeyindən nə qədər kənara çıxa biləcəyi (+/- 20%)

/* =========================================================================
   II. DATA STRUCTURES
   ========================================================================= */

// PID Parameters Structure
typedef struct {
    float Kp;           // Proportional gain
    float Ki;           // Integral gain  
    float Kd;           // Derivative gain
    float integral_sum; // Integral accumulator
    float previous_error; // Previous error for derivative
    float output_min;   // Output minimum limit
    float output_max;   // Output maximum limit
} PID_Controller_t;

// System Status Structure
typedef struct {
    float target_pressure;      // Setpoint pressure (bar)
    float current_pressure;     // Current pressure (bar)
    uint16_t raw_adc_value;     // KRİTİK: Xam ADC dəyəri (0-4095) - UI üçün lazımdır
    float motor_pwm_percent;    // Motor PWM duty cycle (%)
    float zme_pwm_percent;      // ZME PWM duty cycle (%)
    float drv_pwm_percent;      // DRV PWM duty cycle (%)
    float pid_output;           // PID controller output
    float error;                // Current error (setpoint - current)
    bool control_enabled;       // Control system enabled
    bool safety_triggered;      // Safety system triggered
} SystemStatus_t;

// Calibration Data Structure
typedef struct {
    float adc_min;
    float adc_max;
    float pressure_min;
    float pressure_max;
    float slope;
    float offset;
    bool calibrated;
    uint32_t calibration_date;
} CalibrationData_t;

// Safety Limits Structure
typedef struct {
    float max_pressure;         // Maximum safe pressure
    float over_limit_margin;    // Over-limit margin
    float emergency_threshold;  // Emergency stop threshold
    bool safety_enabled;        // Safety system enabled
} SafetyLimits_t;

/* =========================================================================
   III. GLOBAL VARIABLES (External Access)
   ========================================================================= */

// Main system status (extern for external access)
extern SystemStatus_t g_system_status;

// PID Controllers
extern PID_Controller_t g_pid_zme;
extern PID_Controller_t g_pid_drv;

// Calibration data
extern CalibrationData_t g_calibration;

// Safety limits
extern SafetyLimits_t g_safety_limits;

/* =========================================================================
   IV. FUNCTION PROTOTYPES
   ========================================================================= */

/* Hardware Interface Functions */
uint16_t AdvancedPressureControl_ReadADC(void);
void AdvancedPressureControl_SetZME_PWM(float percent);
void AdvancedPressureControl_SetDRV_PWM(float percent);
void AdvancedPressureControl_SetMotor_PWM(float percent);

/* Core Control Functions */
void AdvancedPressureControl_Init(void);
void AdvancedPressureControl_Step(void);
void AdvancedPressureControl_Reset(void);

/* PID Control Functions */
void AdvancedPressureControl_InitPID(PID_Controller_t* pid, float kp, float ki, float kd);
float AdvancedPressureControl_CalculatePID(PID_Controller_t* pid, float error, float dt);
void AdvancedPressureControl_SetPIDParams(PID_Controller_t* pid, float kp, float ki, float kd);

/* Pressure Management Functions */
float AdvancedPressureControl_ReadPressure(void);
void AdvancedPressureControl_SetTargetPressure(float pressure);
void AdvancedPressureControl_ControlMotorSpeed(void);

/* Valve Control Functions */
float AdvancedPressureControl_CalculateZMEBaseFromSP(float sp);  // YENİ: SP-yə görə ZME baza dəyəri
float AdvancedPressureControl_CalculateDRVBaseFromSP(float sp);  // YENİ: SP-yə görə DRV baza dəyəri
void AdvancedPressureControl_ControlZME(float pid_output);
void AdvancedPressureControl_ControlDRV(float pid_output);
void AdvancedPressureControl_ApplyValveOutputs(void);

/* Safety Functions */
bool AdvancedPressureControl_SafetyCheck(void);
void AdvancedPressureControl_HandleSafetyViolation(void);
void AdvancedPressureControl_EnableSafety(bool enable);

/* Calibration Functions */
void AdvancedPressureControl_CalibrateSensor(void);
void AdvancedPressureControl_LoadCalibration(void);
void AdvancedPressureControl_SaveCalibration(void);
bool AdvancedPressureControl_IsCalibrated(void);

/* Utility Functions */
float AdvancedPressureControl_MapValue(float x, float in_min, float in_max, float out_min, float out_max);
float AdvancedPressureControl_ClampValue(float value, float min_val, float max_val);
void AdvancedPressureControl_UpdateStatus(void);

/* Status and Monitoring Functions */
SystemStatus_t* AdvancedPressureControl_GetStatus(void);
void AdvancedPressureControl_PrintStatus(void);
void AdvancedPressureControl_PrintDebugInfo(void);

/* ZME Nonlinearity Compensation */
float AdvancedPressureControl_CompensateZME_Nonlinearity(float desired_pwm);
void AdvancedPressureControl_TestZME_Nonlinearity(void);

/* Base PWM Configuration Functions */
void AdvancedPressureControl_SetZME_BasePWM(float base_pwm);
void AdvancedPressureControl_SetDRV_BasePWM(float base_pwm);
float AdvancedPressureControl_GetZME_BasePWM(void);
float AdvancedPressureControl_GetDRV_BasePWM(void);

/* 
 * REMOVED: AdvancedPressureControl_TimerCallback() funksiya elanı silindi
 * 
 * Bu funksiya artıq lazımsızdır, çünki Timer 6 artıq 10ms tezliyə qurulub.
 * Bunun əvəzinə main.c-də HAL_TIM_PeriodElapsedCallback() funksiyasında
 * birbaşa AdvancedPressureControl_Step() çağırılır.
 */

/* Flash Memory Functions */
void AdvancedPressureControl_SavePIDParamsToFlash(void);
void AdvancedPressureControl_LoadPIDParamsFromFlash(void);
HAL_StatusTypeDef AdvancedPressureControl_SaveToFlash_Centralized(uint8_t block_type, void* data_ptr, uint32_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* __ADVANCED_PRESSURE_CONTROL_H */
