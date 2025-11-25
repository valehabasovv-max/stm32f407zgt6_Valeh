/*
 * Pressure Control Configuration and Calibration
 * Valeh Injection System - Configuration Management
 * 
 * This file contains all configuration parameters, calibration data,
 * and tuning functions for the pressure control system.
 */

#ifndef __PRESSURE_CONTROL_CONFIG_H
#define __PRESSURE_CONTROL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
   CONFIGURATION PARAMETERS
   ========================================================================= */

// System Configuration
#define CONFIG_SYSTEM_NAME "Valeh Injection System"
#define CONFIG_VERSION "1.0.0"
#define CONFIG_BUILD_DATE __DATE__ " " __TIME__

// Control Loop Configuration
#define CONFIG_CONTROL_LOOP_FREQUENCY_HZ 100  // 100Hz = 10ms
#define CONFIG_CONTROL_LOOP_TIME_MS 10
#define CONFIG_CONTROL_LOOP_TIME_S 0.01f

// Control Deadband Configuration
#define CONFIG_CONTROL_DEADBAND_BAR 0.1f  // ±0.1 bar deadband (precise control)

// Valve Slew Rate Configuration (Rate Limiting)
// NOTE: These constants are available for future use if rate limiting is needed
// Advanced system currently uses different control approach
#define CONFIG_ZME_SLEW 3.0f    // ZME slew rate (% per step) - Available for future use
#define CONFIG_DRV_SLEW 3.0f    // DRV slew rate (% per step) - Available for future use

// PID Tuning Parameters
#define CONFIG_PID_ZME_KP_DEFAULT 0.8f
#define CONFIG_PID_ZME_KI_DEFAULT 0.05f
#define CONFIG_PID_ZME_KD_DEFAULT 0.01f

#define CONFIG_PID_DRV_KP_DEFAULT 0.8f
#define CONFIG_PID_DRV_KI_DEFAULT 0.05f
#define CONFIG_PID_DRV_KD_DEFAULT 0.01f

// Safety Configuration
#define CONFIG_SAFETY_MAX_PRESSURE 300.0f
#define CONFIG_SAFETY_OVER_LIMIT_MARGIN 10.0f
#define CONFIG_SAFETY_EMERGENCY_THRESHOLD 350.0f

// Valve Configuration
#define CONFIG_ZME_PWM_MIN 0.0f
#define CONFIG_ZME_PWM_MAX 30.0f
#define CONFIG_ZME_CUTOFF_PWM 2.0f
#define CONFIG_ZME_MIN_WORKING 1.0f

#define CONFIG_DRV_PWM_MIN 0.0f
#define CONFIG_DRV_PWM_MAX 40.0f

#define CONFIG_MOTOR_PWM_MIN 0.0f
#define CONFIG_MOTOR_PWM_MAX 100.0f

// Pressure Sensor Configuration
// KRİTİK DÜZƏLİŞ: 12-bit ADC maksimum dəyəri 4095-dir (2^12 - 1), 4096 deyil!
// 0.5V (0 bar) -> 410, 5.0V (300 bar) -> 4095 (5.0V Vref fərziyyəsi ilə)
// PRESSURE_SLOPE = (300.0 - 0.0) / (4095 - 410) ≈ 0.08137 bar/ADC count
#define CONFIG_PRESSURE_SENSOR_ADC_MIN 410
#define CONFIG_PRESSURE_SENSOR_ADC_MAX 4095
#define CONFIG_PRESSURE_SENSOR_PRESSURE_MIN 0.0f
#define CONFIG_PRESSURE_SENSOR_PRESSURE_MAX 300.0f

// Motor Speed Configuration
#define CONFIG_MOTOR_MIN_SPEED_PERCENT 20.0f
#define CONFIG_MOTOR_MAX_SPEED_PERCENT 100.0f
#define CONFIG_MOTOR_PRESSURE_RANGE 300.0f

/* =========================================================================
   DATA STRUCTURES
   ========================================================================= */

// PID Tuning Parameters
typedef struct {
    float kp;
    float ki;
    float kd;
    bool auto_tune_enabled;
    float tuning_aggressiveness;
} PID_Tuning_t;

// System Configuration
typedef struct {
    char system_name[32];
    char version[16];
    char build_date[32];
    bool debug_enabled;
    bool safety_enabled;
} System_Config_t;

// Calibration Data
typedef struct {
    float adc_min;
    float adc_max;
    float pressure_min;
    float pressure_max;
    float slope;
    float offset;
    bool calibrated;
    uint32_t calibration_date;
} Calibration_Data_t;

// Valve Configuration
typedef struct {
    float zme_pwm_min;
    float zme_pwm_max;
    float zme_cutoff_pwm;
    float zme_min_working;
    float drv_pwm_min;
    float drv_pwm_max;
    float motor_pwm_min;
    float motor_pwm_max;
} Valve_Config_t;

// Safety Configuration
typedef struct {
    float max_pressure;
    float over_limit_margin;
    float emergency_threshold;
    bool safety_enabled;
} Safety_Config_t;

/* =========================================================================
   GLOBAL CONFIGURATION VARIABLES
   ========================================================================= */

extern System_Config_t g_system_config;
extern PID_Tuning_t g_pid_zme_tuning;
extern PID_Tuning_t g_pid_drv_tuning;
extern Calibration_Data_t g_calibration_data;
extern Valve_Config_t g_valve_config;
extern Safety_Config_t g_safety_config;

/* =========================================================================
   FUNCTION PROTOTYPES
   ========================================================================= */

/* Configuration Management */
void PressureControlConfig_Init(void);
void PressureControlConfig_LoadDefaults(void);
void PressureControlConfig_SaveToFlash(void);
void PressureControlConfig_LoadFromFlash(void);

/* PID Tuning Functions */
void PressureControlConfig_SetPIDTuning(PID_Tuning_t* tuning, float kp, float ki, float kd);
void PressureControlConfig_EnableAutoTune(PID_Tuning_t* tuning, bool enable);
void PressureControlConfig_SetTuningAggressiveness(PID_Tuning_t* tuning, float aggressiveness);
void PressureControlConfig_ResetPIDTuning(PID_Tuning_t* tuning);

/* Calibration Functions */
void PressureControlConfig_StartCalibration(void);
void PressureControlConfig_AddCalibrationPoint(float adc_value, float pressure_value);
void PressureControlConfig_CompleteCalibration(void);
void PressureControlConfig_ResetCalibration(void);
bool PressureControlConfig_IsCalibrated(void);
void PressureControlConfig_PrintCalibrationData(void);

/* Valve Configuration */
void PressureControlConfig_SetValveLimits(Valve_Config_t* config);
void PressureControlConfig_SetZMELimits(float pwm_min, float pwm_max, float cutoff_pwm);
void PressureControlConfig_SetDRVLimits(float pwm_min, float pwm_max);
void PressureControlConfig_SetMotorLimits(float pwm_min, float pwm_max);

/* Safety Configuration */
void PressureControlConfig_SetSafetyLimits(float max_pressure, float over_limit_margin, float emergency_threshold);
void PressureControlConfig_EnableSafety(bool enable);

/* System Configuration */
void PressureControlConfig_SetDebugMode(bool enable);
void PressureControlConfig_PrintSystemInfo(void);

/* Parameter Validation */
bool PressureControlConfig_ValidatePIDParams(float kp, float ki, float kd);
bool PressureControlConfig_ValidatePressureLimits(float min_pressure, float max_pressure);
bool PressureControlConfig_ValidatePWMValues(float pwm_min, float pwm_max);

/* Configuration Backup and Restore */
void PressureControlConfig_BackupConfiguration(void);
void PressureControlConfig_RestoreConfiguration(void);
void PressureControlConfig_ResetToDefaults(void);

/* Real-time Parameter Adjustment */
void PressureControlConfig_AdjustKP(float delta);
void PressureControlConfig_AdjustKI(float delta);
void PressureControlConfig_AdjustKD(float delta);
void PressureControlConfig_AdjustTargetPressure(float delta);

/* Status and Monitoring */
void PressureControlConfig_PrintCurrentConfig(void);
void PressureControlConfig_PrintTuningStatus(void);
void PressureControlConfig_PrintCalibrationStatus(void);

/* Advanced Tuning Functions */
void PressureControlConfig_StartAutoTuning(void);
void PressureControlConfig_StopAutoTuning(void);
bool PressureControlConfig_IsAutoTuningActive(void);
void PressureControlConfig_OptimizePIDParams(void);

/* Configuration Persistence */
void PressureControlConfig_SavePIDParams(void);
void PressureControlConfig_LoadPIDParams(void);
void PressureControlConfig_SaveCalibrationData(void);
void PressureControlConfig_LoadCalibrationData(void);

/* Additional Function Declarations */
void PressureControlConfig_BackupConfiguration(void);
void PressureControlConfig_RestoreConfiguration(void);
void PressureControlConfig_ResetToDefaults(void);

#ifdef __cplusplus
}
#endif

#endif /* __PRESSURE_CONTROL_CONFIG_H */
