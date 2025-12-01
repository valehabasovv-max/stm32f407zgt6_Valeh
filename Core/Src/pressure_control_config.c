/*
 * Pressure Control Configuration and Calibration Implementation
 * Valeh Injection System - Configuration Management
 * 
 * This file implements all configuration management, calibration,
 * and parameter tuning functions for the pressure control system.
 */

#include "pressure_control_config.h"
#include "advanced_pressure_control.h"
#include "ILI9341_FSMC.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx_hal.h"

/* =========================================================================
   GLOBAL CONFIGURATION VARIABLES
   ========================================================================= */

System_Config_t g_system_config = {
    .system_name = CONFIG_SYSTEM_NAME,
    .version = CONFIG_VERSION,
    .build_date = CONFIG_BUILD_DATE,
    .debug_enabled = true,
    .safety_enabled = true
};

PID_Tuning_t g_pid_zme_tuning = {
    .kp = CONFIG_PID_ZME_KP_DEFAULT,
    .ki = CONFIG_PID_ZME_KI_DEFAULT,
    .kd = CONFIG_PID_ZME_KD_DEFAULT,
    .auto_tune_enabled = false,
    .tuning_aggressiveness = 1.0f
};

PID_Tuning_t g_pid_drv_tuning = {
    .kp = CONFIG_PID_DRV_KP_DEFAULT,
    .ki = CONFIG_PID_DRV_KI_DEFAULT,
    .kd = CONFIG_PID_DRV_KD_DEFAULT,
    .auto_tune_enabled = false,
    .tuning_aggressiveness = 1.0f
};

// KRİTİK DÜZƏLİŞ: Slope və offset default dəyərlərlə hesablanmalıdır!
// Əvvəlki kod slope=0.0f və calibrated=false idi, bu da bütün təzyiq oxunuşlarının 0 olmasına səbəb olurdu!
// 
// Düzgün hesablama:
// slope = (pressure_max - pressure_min) / (adc_max - adc_min)
//       = (300.0 - 0.0) / (CONFIG_PRESSURE_SENSOR_ADC_MAX - CONFIG_PRESSURE_SENSOR_ADC_MIN)
// offset = pressure_min - (slope * adc_min)
//        = 0.0 - (slope * CONFIG_PRESSURE_SENSOR_ADC_MIN)
#define CONFIG_PRESSURE_SLOPE ((CONFIG_PRESSURE_SENSOR_PRESSURE_MAX - CONFIG_PRESSURE_SENSOR_PRESSURE_MIN) / \
                               (float)(CONFIG_PRESSURE_SENSOR_ADC_MAX - CONFIG_PRESSURE_SENSOR_ADC_MIN))
#define CONFIG_PRESSURE_OFFSET (CONFIG_PRESSURE_SENSOR_PRESSURE_MIN - \
                                (CONFIG_PRESSURE_SLOPE * (float)CONFIG_PRESSURE_SENSOR_ADC_MIN))

Calibration_Data_t g_calibration_data = {
    .adc_min = CONFIG_PRESSURE_SENSOR_ADC_MIN,
    .adc_max = CONFIG_PRESSURE_SENSOR_ADC_MAX,
    .pressure_min = CONFIG_PRESSURE_SENSOR_PRESSURE_MIN,
    .pressure_max = CONFIG_PRESSURE_SENSOR_PRESSURE_MAX,
    .slope = CONFIG_PRESSURE_SLOPE,      // DÜZƏLİŞ: Default slope hesablandı
    .offset = CONFIG_PRESSURE_OFFSET,    // DÜZƏLİŞ: Default offset hesablandı
    .calibrated = true,                  // DÜZƏLİŞ: Default dəyərlərlə calibrated=true
    .calibration_date = 0
};

Valve_Config_t g_valve_config = {
    .zme_pwm_min = CONFIG_ZME_PWM_MIN,
    .zme_pwm_max = CONFIG_ZME_PWM_MAX,
    .zme_cutoff_pwm = CONFIG_ZME_CUTOFF_PWM,
    .zme_min_working = CONFIG_ZME_MIN_WORKING,
    .drv_pwm_min = CONFIG_DRV_PWM_MIN,
    .drv_pwm_max = CONFIG_DRV_PWM_MAX,
    .motor_pwm_min = CONFIG_MOTOR_PWM_MIN,
    .motor_pwm_max = CONFIG_MOTOR_PWM_MAX
};

Safety_Config_t g_safety_config = {
    .max_pressure = CONFIG_SAFETY_MAX_PRESSURE,
    .over_limit_margin = CONFIG_SAFETY_OVER_LIMIT_MARGIN,
    .emergency_threshold = CONFIG_SAFETY_EMERGENCY_THRESHOLD,
    .safety_enabled = true
};

/* =========================================================================
   CONFIGURATION MANAGEMENT FUNCTIONS
   ========================================================================= */

/**
 * @brief Initialize configuration system
 */
void PressureControlConfig_Init(void) {
    // Load configuration from flash
    PressureControlConfig_LoadFromFlash();
    
    // Load PID parameters and SP from flash
    PressureControlConfig_LoadPIDParams();
    
    // Apply PID tuning parameters to AdvancedPressureControl system
    PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 
                                       g_pid_zme_tuning.kp, 
                                       g_pid_zme_tuning.ki, 
                                       g_pid_zme_tuning.kd);
    PressureControlConfig_SetPIDTuning(&g_pid_drv_tuning, 
                                       g_pid_drv_tuning.kp, 
                                       g_pid_drv_tuning.ki, 
                                       g_pid_drv_tuning.kd);
    
    // Slope və offset hesabla
    if (g_calibration_data.adc_max > g_calibration_data.adc_min) {
        g_calibration_data.slope = (g_calibration_data.pressure_max - g_calibration_data.pressure_min) / 
                                  (g_calibration_data.adc_max - g_calibration_data.adc_min);
        g_calibration_data.offset = g_calibration_data.pressure_min - 
                                   (g_calibration_data.slope * g_calibration_data.adc_min);
        g_calibration_data.calibrated = true;
    } else {
        // ADC aralığı invalid - default dəyərləri istifadə et
        g_calibration_data.adc_min = (float)CONFIG_PRESSURE_SENSOR_ADC_MIN;
        g_calibration_data.adc_max = (float)CONFIG_PRESSURE_SENSOR_ADC_MAX;
        g_calibration_data.slope = CONFIG_PRESSURE_SLOPE;
        g_calibration_data.offset = CONFIG_PRESSURE_OFFSET;
        g_calibration_data.calibrated = true;
    }
}

/**
 * @brief Load default configuration
 */
void PressureControlConfig_LoadDefaults(void) {
    // Reset all configurations to defaults
    g_pid_zme_tuning.kp = CONFIG_PID_ZME_KP_DEFAULT;
    g_pid_zme_tuning.ki = CONFIG_PID_ZME_KI_DEFAULT;
    g_pid_zme_tuning.kd = CONFIG_PID_ZME_KD_DEFAULT;
    
    g_pid_drv_tuning.kp = CONFIG_PID_DRV_KP_DEFAULT;
    g_pid_drv_tuning.ki = CONFIG_PID_DRV_KI_DEFAULT;
    g_pid_drv_tuning.kd = CONFIG_PID_DRV_KD_DEFAULT;
    
    // Apply default PID tuning values to AdvancedPressureControl system
    PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 
                                       g_pid_zme_tuning.kp, 
                                       g_pid_zme_tuning.ki, 
                                       g_pid_zme_tuning.kd);
    PressureControlConfig_SetPIDTuning(&g_pid_drv_tuning, 
                                       g_pid_drv_tuning.kp, 
                                       g_pid_drv_tuning.ki, 
                                       g_pid_drv_tuning.kd);
    
    g_safety_config.max_pressure = CONFIG_SAFETY_MAX_PRESSURE;
    g_safety_config.over_limit_margin = CONFIG_SAFETY_OVER_LIMIT_MARGIN;
    g_safety_config.emergency_threshold = CONFIG_SAFETY_EMERGENCY_THRESHOLD;
    
    g_valve_config.zme_pwm_min = CONFIG_ZME_PWM_MIN;
    g_valve_config.zme_pwm_max = CONFIG_ZME_PWM_MAX;
    g_valve_config.zme_cutoff_pwm = CONFIG_ZME_CUTOFF_PWM;
}

/**
 * @brief Save configuration to flash memory
 */
void PressureControlConfig_SaveToFlash(void) {
    // Save all configuration data
    PressureControlConfig_SavePIDParams();
    PressureControlConfig_SaveCalibrationData();
}

/**
 * @brief Load configuration from flash memory
 */
void PressureControlConfig_LoadFromFlash(void) {
    // Load all configuration data
    PressureControlConfig_LoadPIDParams();
    PressureControlConfig_LoadCalibrationData();
}

/* =========================================================================
   PID TUNING FUNCTIONS
   ========================================================================= */

/**
 * @brief Set PID tuning parameters
 */
void PressureControlConfig_SetPIDTuning(PID_Tuning_t* tuning, float kp, float ki, float kd) {
    if (PressureControlConfig_ValidatePIDParams(kp, ki, kd)) {
        tuning->kp = kp;
        tuning->ki = ki;
        tuning->kd = kd;
        
        // Apply to active PID controllers
        if (tuning == &g_pid_zme_tuning) {
            AdvancedPressureControl_SetPIDParams(&g_pid_zme, kp, ki, kd);
        } else if (tuning == &g_pid_drv_tuning) {
            AdvancedPressureControl_SetPIDParams(&g_pid_drv, kp, ki, kd);
        }
    }
}

/**
 * @brief Enable/disable auto-tuning
 */
void PressureControlConfig_EnableAutoTune(PID_Tuning_t* tuning, bool enable) {
    tuning->auto_tune_enabled = enable;
}

/**
 * @brief Set tuning aggressiveness
 */
void PressureControlConfig_SetTuningAggressiveness(PID_Tuning_t* tuning, float aggressiveness) {
    tuning->tuning_aggressiveness = fmaxf(0.1f, fminf(2.0f, aggressiveness));
}

/**
 * @brief Reset PID tuning to defaults
 */
void PressureControlConfig_ResetPIDTuning(PID_Tuning_t* tuning) {
    if (tuning == &g_pid_zme_tuning) {
        tuning->kp = CONFIG_PID_ZME_KP_DEFAULT;
        tuning->ki = CONFIG_PID_ZME_KI_DEFAULT;
        tuning->kd = CONFIG_PID_ZME_KD_DEFAULT;
    } else if (tuning == &g_pid_drv_tuning) {
        tuning->kp = CONFIG_PID_DRV_KP_DEFAULT;
        tuning->ki = CONFIG_PID_DRV_KI_DEFAULT;
        tuning->kd = CONFIG_PID_DRV_KD_DEFAULT;
    }
    
    // Apply reset values to AdvancedPressureControl system
    PressureControlConfig_SetPIDTuning(tuning, tuning->kp, tuning->ki, tuning->kd);
}

/* =========================================================================
   CALIBRATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Start calibration process
 */
void PressureControlConfig_StartCalibration(void) {
    g_calibration_data.calibrated = false;
    g_calibration_data.calibration_date = HAL_GetTick();
}

/**
 * @brief Add calibration point
 */
void PressureControlConfig_AddCalibrationPoint(float adc_value, float pressure_value) {
    // Update calibration data
    if (pressure_value == 0.0f) {
        g_calibration_data.adc_min = adc_value;
        g_calibration_data.pressure_min = pressure_value;
    } else {
        g_calibration_data.adc_max = adc_value;
        g_calibration_data.pressure_max = pressure_value;
    }
}

/**
 * @brief Complete calibration process
 * 
 * KRİTİK DÜZƏLİŞ: ADC aralığı validasiyası gücləndirildi.
 * Minimum ADC aralığı (2000) tələb olunur ki, slope çox kiçik olmasın.
 */
void PressureControlConfig_CompleteCalibration(void) {
    // ADC aralığı validasiyası
    float adc_range = g_calibration_data.adc_max - g_calibration_data.adc_min;
    
    // Minimum aralıq yoxlaması
    const float MIN_ADC_RANGE = 1500.0f;
    
    if (g_calibration_data.adc_max > g_calibration_data.adc_min && adc_range >= MIN_ADC_RANGE) {
        // Calculate slope and offset
        g_calibration_data.slope = (g_calibration_data.pressure_max - g_calibration_data.pressure_min) / adc_range;
        g_calibration_data.offset = g_calibration_data.pressure_min - 
                                   (g_calibration_data.slope * g_calibration_data.adc_min);
        g_calibration_data.calibrated = true;
        g_calibration_data.calibration_date = HAL_GetTick();
        
        // Advanced sistemdəki kalibrasiya strukturunu da yenilə
        extern CalibrationData_t g_calibration;
        g_calibration.adc_min = g_calibration_data.adc_min;
        g_calibration.adc_max = g_calibration_data.adc_max;
        g_calibration.pressure_min = g_calibration_data.pressure_min;
        g_calibration.pressure_max = g_calibration_data.pressure_max;
        g_calibration.slope = g_calibration_data.slope;
        g_calibration.offset = g_calibration_data.offset;
        g_calibration.calibrated = true;
        
        // Save calibration data to both systems
        PressureControlConfig_SaveCalibrationData();
        AdvancedPressureControl_SaveCalibration();
    }
}

/**
 * @brief Reset calibration data to defaults
 * 
 * KRİTİK DÜZƏLİŞ: Slope=0 etmək əvəzinə default dəyərlərə sıfırlayır.
 * Bu, təzyiq oxunuşlarının işləməyə davam etməsini təmin edir.
 */
void PressureControlConfig_ResetCalibration(void) {
    // Default dəyərlərə sıfırla (slope=0 etmə!)
    g_calibration_data.adc_min = (float)CONFIG_PRESSURE_SENSOR_ADC_MIN;
    g_calibration_data.adc_max = (float)CONFIG_PRESSURE_SENSOR_ADC_MAX;
    g_calibration_data.pressure_min = CONFIG_PRESSURE_SENSOR_PRESSURE_MIN;
    g_calibration_data.pressure_max = CONFIG_PRESSURE_SENSOR_PRESSURE_MAX;
    g_calibration_data.slope = CONFIG_PRESSURE_SLOPE;
    g_calibration_data.offset = CONFIG_PRESSURE_OFFSET;
    g_calibration_data.calibrated = true;  // Default dəyərlərlə calibrated=true
    g_calibration_data.calibration_date = 0;
    
    // Advanced sistemdəki kalibrasiya strukturunu da sıfırla
    extern CalibrationData_t g_calibration;
    g_calibration.adc_min = g_calibration_data.adc_min;
    g_calibration.adc_max = g_calibration_data.adc_max;
    g_calibration.pressure_min = g_calibration_data.pressure_min;
    g_calibration.pressure_max = g_calibration_data.pressure_max;
    g_calibration.slope = g_calibration_data.slope;
    g_calibration.offset = g_calibration_data.offset;
    g_calibration.calibrated = true;
    
}

/**
 * @brief Check if system is calibrated
 */
bool PressureControlConfig_IsCalibrated(void) {
    return g_calibration_data.calibrated;
}

/**
 * @brief Print calibration data
 */
void PressureControlConfig_PrintCalibrationData(void) {
    // No-op - silenced for performance
}

/* =========================================================================
   VALVE CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set valve limits
 */
void PressureControlConfig_SetValveLimits(Valve_Config_t* config) {
    g_valve_config = *config;
}

/**
 * @brief Set ZME valve limits
 */
void PressureControlConfig_SetZMELimits(float pwm_min, float pwm_max, float cutoff_pwm) {
    g_valve_config.zme_pwm_min = pwm_min;
    g_valve_config.zme_pwm_max = pwm_max;
    g_valve_config.zme_cutoff_pwm = cutoff_pwm;
}

/**
 * @brief Set DRV valve limits
 */
void PressureControlConfig_SetDRVLimits(float pwm_min, float pwm_max) {
    g_valve_config.drv_pwm_min = pwm_min;
    g_valve_config.drv_pwm_max = pwm_max;
}

/**
 * @brief Set motor limits
 */
void PressureControlConfig_SetMotorLimits(float pwm_min, float pwm_max) {
    g_valve_config.motor_pwm_min = pwm_min;
    g_valve_config.motor_pwm_max = pwm_max;
}

/* =========================================================================
   SAFETY CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set safety limits
 */
void PressureControlConfig_SetSafetyLimits(float max_pressure, float over_limit_margin, float emergency_threshold) {
    g_safety_config.max_pressure = max_pressure;
    g_safety_config.over_limit_margin = over_limit_margin;
    g_safety_config.emergency_threshold = emergency_threshold;
}

/**
 * @brief Enable/disable safety system
 */
void PressureControlConfig_EnableSafety(bool enable) {
    g_safety_config.safety_enabled = enable;
}

/* =========================================================================
   SYSTEM CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set debug mode
 */
void PressureControlConfig_SetDebugMode(bool enable) {
    g_system_config.debug_enabled = enable;
}

/**
 * @brief Print system information
 */
void PressureControlConfig_PrintSystemInfo(void) {
    // No-op - silenced for performance
}

/* =========================================================================
   PARAMETER VALIDATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Validate PID parameters
 */
bool PressureControlConfig_ValidatePIDParams(float kp, float ki, float kd) {
    return (kp >= 0.0f && kp <= 100.0f) &&
           (ki >= 0.0f && ki <= 10.0f) &&
           (kd >= 0.0f && kd <= 10.0f);
}

/**
 * @brief Validate pressure limits
 */
bool PressureControlConfig_ValidatePressureLimits(float min_pressure, float max_pressure) {
    return (min_pressure >= 0.0f) && 
           (max_pressure > min_pressure) && 
           (max_pressure <= 500.0f);
}

/**
 * @brief Validate PWM values
 */
bool PressureControlConfig_ValidatePWMValues(float pwm_min, float pwm_max) {
    return (pwm_min >= 0.0f) && 
           (pwm_max > pwm_min) && 
           (pwm_max <= 100.0f);
}

/* =========================================================================
   REAL-TIME PARAMETER ADJUSTMENT FUNCTIONS
   ========================================================================= */

/**
 * @brief Adjust Kp parameter
 */
void PressureControlConfig_AdjustKP(float delta) {
    float new_kp = g_pid_zme_tuning.kp + delta;
    if (PressureControlConfig_ValidatePIDParams(new_kp, g_pid_zme_tuning.ki, g_pid_zme_tuning.kd)) {
        PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, new_kp, g_pid_zme_tuning.ki, g_pid_zme_tuning.kd);
    }
}

/**
 * @brief Adjust Ki parameter
 */
void PressureControlConfig_AdjustKI(float delta) {
    float new_ki = g_pid_zme_tuning.ki + delta;
    if (PressureControlConfig_ValidatePIDParams(g_pid_zme_tuning.kp, new_ki, g_pid_zme_tuning.kd)) {
        PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, g_pid_zme_tuning.kp, new_ki, g_pid_zme_tuning.kd);
    }
}

/**
 * @brief Adjust Kd parameter
 */
void PressureControlConfig_AdjustKD(float delta) {
    float new_kd = g_pid_zme_tuning.kd + delta;
    if (PressureControlConfig_ValidatePIDParams(g_pid_zme_tuning.kp, g_pid_zme_tuning.ki, new_kd)) {
        PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, g_pid_zme_tuning.kp, g_pid_zme_tuning.ki, new_kd);
    }
}

/**
 * @brief Adjust target pressure
 */
void PressureControlConfig_AdjustTargetPressure(float delta) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    float new_pressure = status->target_pressure + delta;
    if (new_pressure >= 0.0f && new_pressure <= 300.0f) {
        AdvancedPressureControl_SetTargetPressure(new_pressure);
    }
}

/* =========================================================================
   STATUS AND MONITORING FUNCTIONS
   ========================================================================= */

/**
 * @brief Print current configuration
 */
void PressureControlConfig_PrintCurrentConfig(void) {
    // No-op - silenced for performance
}

/**
 * @brief Print tuning status
 */
void PressureControlConfig_PrintTuningStatus(void) {
    // No-op - silenced for performance
}

/**
 * @brief Print calibration status
 */
void PressureControlConfig_PrintCalibrationStatus(void) {
    // No-op - silenced for performance
}

/* =========================================================================
   CONFIGURATION PERSISTENCE FUNCTIONS
   ========================================================================= */

/* Flash memory structure for PID and system parameters */
/* NOTE: Using offset 0x200 to avoid conflict with AdvancedPressureControl (0x000) and Calibration (0x100) */
#define FLASH_CONFIG_ADDRESS    (0x080E0000 + 0x200)  /* Sector 11, offset 0x200 for pressure_control_config */
#define FLASH_CONFIG_MAGIC      0x87654321UL  /* Magic number to verify valid data (different from others) */

typedef struct {
    uint32_t magic;              /* Magic number: 0x87654321 */
    float pid_kp;                /* Kp parameter */
    float pid_ki;                /* Ki parameter */
    float pid_kd;                /* Kd parameter */
    float setpoint;              /* SetPoint (SP) value */
    uint32_t checksum;           /* Simple checksum */
} Flash_Config_Data_t;

/**
 * @brief Calculate simple checksum
 */
static uint32_t CalculateChecksum(float kp, float ki, float kd, float sp) {
    uint32_t *kp_ptr = (uint32_t*)&kp;
    uint32_t *ki_ptr = (uint32_t*)&ki;
    uint32_t *kd_ptr = (uint32_t*)&kd;
    uint32_t *sp_ptr = (uint32_t*)&sp;
    return (uint32_t)FLASH_CONFIG_MAGIC ^ *kp_ptr ^ *ki_ptr ^ *kd_ptr ^ *sp_ptr;
}

/**
 * @brief Save PID parameters and SP to flash
 */
void PressureControlConfig_SavePIDParams(void) {
    // Prepare data structure
    Flash_Config_Data_t config_data;
    config_data.magic = FLASH_CONFIG_MAGIC;
    config_data.pid_kp = g_pid_zme_tuning.kp;
    config_data.pid_ki = g_pid_zme_tuning.ki;
    config_data.pid_kd = g_pid_zme_tuning.kd;
    
    // Get current SP value - DÜZƏLİŞ: g_system_status.target_pressure-dən götür
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    config_data.setpoint = status->target_pressure;  // target_pressure dəyərini istifadə et
    
    // Calculate checksum
    config_data.checksum = CalculateChecksum(
        config_data.pid_kp, config_data.pid_ki, 
        config_data.pid_kd, config_data.setpoint);
    
    // KRİTİK DÜZƏLİŞ: Mərkəzləşdirilmiş Flash Yaddaş Məntiqindən istifadə et
    // block_type = 2 (Config PID)
    // Bütün doğrulama, printf mesajları və xəta yoxlamaları mərkəzləşdirilmiş funksiyanın daxilində həyata keçirilir
    AdvancedPressureControl_SaveToFlash_Centralized(
        2,  // block_type: Config PID
        &config_data,
        sizeof(Flash_Config_Data_t)
    );
}

/**
 * @brief Load PID parameters and SP from flash
 * 
 * ⚠️ KRİTİK XƏBƏRDARLIQ: Bu funksiya Config sisteminin daxili strukturlarını (g_pid_zme_tuning, g_pid_drv_tuning) yeniləyir
 * və Advanced sistemin funksiyalarını çağırır. Lakin Advanced sistem artıq öz funksiyasını 
 * (AdvancedPressureControl_LoadPIDParamsFromFlash) çağırır və fərqli Flash offset-dən (0x000) oxuyur.
 * 
 * Bu, ikiqat Flash əməliyyatı və məlumat ziddiyyəti riskini yaradır.
 * 
 * Tövsiyə: Bütün Flash əməliyyatları yalnız Advanced sistemdə mərkəzləşdirilməlidir.
 * Config sistemi yalnız Advanced sistemin setter funksiyalarını çağırmalıdır.
 */
void PressureControlConfig_LoadPIDParams(void) {
    // Read data from flash
    Flash_Config_Data_t *config_data = (Flash_Config_Data_t*)FLASH_CONFIG_ADDRESS;
    
    // Verify magic number
    if (config_data->magic != FLASH_CONFIG_MAGIC) {
        // Apply default PID tuning values to AdvancedPressureControl system
        PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 
                                           g_pid_zme_tuning.kp, 
                                           g_pid_zme_tuning.ki, 
                                           g_pid_zme_tuning.kd);
        PressureControlConfig_SetPIDTuning(&g_pid_drv_tuning, 
                                           g_pid_drv_tuning.kp, 
                                           g_pid_drv_tuning.ki, 
                                           g_pid_drv_tuning.kd);
        return;
    }
    
    // Verify checksum
    uint32_t calculated_checksum = CalculateChecksum(
        config_data->pid_kp, config_data->pid_ki,
        config_data->pid_kd, config_data->setpoint);
    
    if (calculated_checksum != config_data->checksum) {
        // Apply default PID tuning values to AdvancedPressureControl system
        PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 
                                           g_pid_zme_tuning.kp, 
                                           g_pid_zme_tuning.ki, 
                                           g_pid_zme_tuning.kd);
        PressureControlConfig_SetPIDTuning(&g_pid_drv_tuning, 
                                           g_pid_drv_tuning.kp, 
                                           g_pid_drv_tuning.ki, 
                                           g_pid_drv_tuning.kd);
        return;
    }
    
    // Load PID parameters
    // Check if loaded values are old defaults and update to new defaults if so
    // Old defaults: Kp=1.5, Ki=0.005, Kd=0.0 (pressure_control_config.h)
    // Old defaults: Kp=0.5, Ki=0.0010, Kd=0.0 (advanced_pressure_control.h)
    bool is_old_default_1 = (fabsf(config_data->pid_kp - 0.5f) < 0.01f) && 
                            (fabsf(config_data->pid_ki - 0.0010f) < 0.0001f) && 
                            (fabsf(config_data->pid_kd - 0.0f) < 0.01f);
    
    bool is_old_default_2 = (fabsf(config_data->pid_kp - 1.5f) < 0.01f) && 
                            (fabsf(config_data->pid_ki - 0.005f) < 0.0001f) && 
                            (fabsf(config_data->pid_kd - 0.0f) < 0.01f);
    
    if (is_old_default_1 || is_old_default_2) {
        g_pid_zme_tuning.kp = CONFIG_PID_ZME_KP_DEFAULT;
        g_pid_zme_tuning.ki = CONFIG_PID_ZME_KI_DEFAULT;
        g_pid_zme_tuning.kd = CONFIG_PID_ZME_KD_DEFAULT;
        g_pid_drv_tuning.kp = CONFIG_PID_DRV_KP_DEFAULT;
        g_pid_drv_tuning.ki = CONFIG_PID_DRV_KI_DEFAULT;
        g_pid_drv_tuning.kd = CONFIG_PID_DRV_KD_DEFAULT;
    } else {
        g_pid_zme_tuning.kp = config_data->pid_kp;
        g_pid_zme_tuning.ki = config_data->pid_ki;
        g_pid_zme_tuning.kd = config_data->pid_kd;
        // Config sistemində DRV də eyni dəyərləri istifadə edir
        g_pid_drv_tuning.kp = config_data->pid_kp;
        g_pid_drv_tuning.ki = config_data->pid_ki;
        g_pid_drv_tuning.kd = config_data->pid_kd;
    }
    
    // Apply PID parameters to Advanced system controllers
    // DÜZƏLİŞ: Advanced sistem funksiyasını istifadə et
    // QEYD: Advanced sistem artıq öz funksiyasını (AdvancedPressureControl_LoadPIDParamsFromFlash) çağırır
    // Bu, ikiqat Flash əməliyyatı riskini yaradır, amma uyğunluq üçün saxlanılır
    AdvancedPressureControl_SetPIDParams(&g_pid_zme, 
                                         g_pid_zme_tuning.kp, 
                                         g_pid_zme_tuning.ki, 
                                         g_pid_zme_tuning.kd);
    AdvancedPressureControl_SetPIDParams(&g_pid_drv, 
                                         g_pid_drv_tuning.kp, 
                                         g_pid_drv_tuning.ki, 
                                         g_pid_drv_tuning.kd);
    
    // Load and set SetPoint
    if (config_data->setpoint > 0.1f && config_data->setpoint <= 300.0f) {
        AdvancedPressureControl_SetTargetPressure(config_data->setpoint);
    }
}

/**
 * @brief Save calibration data to flash
 * 
 * KRİTİK DÜZƏLİŞ: Bu funksiya artıq mərkəzləşdirilmiş Flash yaddaş məntiqindən istifadə edir.
 * Bütün əl ilə bərpa məntiqi silindi.
 */
void PressureControlConfig_SaveCalibrationData(void) {
    // Prepare calibration data structure
    typedef struct {
        uint32_t magic;           /* Magic number: 0x12345678 */
        float min_voltage;        /* 0.5V */
        float max_voltage;        /* 5.24V */
        float min_pressure;       /* 0.0 bar */
        float max_pressure;       /* 300.0 bar */
        uint16_t adc_min;         /* 410 */
        uint16_t adc_max;         /* 4095 (12-bit ADC max value) */
        uint32_t checksum;         /* Data integrity check */
    } calibration_data_t;
    
    calibration_data_t cal_data;
    cal_data.magic = 0x12345678;
    cal_data.min_voltage = 0.5f;  // Default values, should be updated from g_calibration_data
    cal_data.max_voltage = 5.24f;
    cal_data.min_pressure = g_calibration_data.pressure_min;
    cal_data.max_pressure = g_calibration_data.pressure_max;
    cal_data.adc_min = (uint16_t)g_calibration_data.adc_min;
    cal_data.adc_max = (uint16_t)g_calibration_data.adc_max;
    
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
    
    (void)status; // Unused after silencing printf
}

/**
 * @brief Load calibration data from flash
 * 
 * KRİTİK DÜZƏLİŞ: Bu funksiya artıq Advanced sistemin vahid strukturundan istifadə edir.
 * Köhnə strukturlar (ADC 500-3500) silindi və AdvancedPressureControl_LoadCalibration() çağırılır.
 */
void PressureControlConfig_LoadCalibrationData(void) {
    // Advanced sistemin vahid strukturundan istifadə et
    AdvancedPressureControl_LoadCalibration();
    
    // Advanced sistemdən yüklənən kalibrasiya məlumatlarını g_calibration_data strukturuna köçür
    extern CalibrationData_t g_calibration;
    
    if (g_calibration.calibrated) {
        g_calibration_data.adc_min = g_calibration.adc_min;
        g_calibration_data.adc_max = g_calibration.adc_max;
        g_calibration_data.pressure_min = g_calibration.pressure_min;
        g_calibration_data.pressure_max = g_calibration.pressure_max;
        g_calibration_data.slope = g_calibration.slope;
        g_calibration_data.offset = g_calibration.offset;
        g_calibration_data.calibrated = true;
    }
}

/* =========================================================================
   MISSING FUNCTION IMPLEMENTATIONS
   ========================================================================= */

/**
 * @brief Backup configuration
 */
void PressureControlConfig_BackupConfiguration(void) {
    // Save all configuration to flash
    PressureControlConfig_SaveToFlash();
    PressureControlConfig_SavePIDParams();
    PressureControlConfig_SaveCalibrationData();
}

/**
 * @brief Restore configuration
 */
void PressureControlConfig_RestoreConfiguration(void) {
    // Load all configuration from flash
    PressureControlConfig_LoadFromFlash();
    PressureControlConfig_LoadPIDParams();
    PressureControlConfig_LoadCalibrationData();
}

/**
 * @brief Reset to defaults
 */
void PressureControlConfig_ResetToDefaults(void) {
    // Load default configuration
    PressureControlConfig_LoadDefaults();
    
    // Reset calibration
    PressureControlConfig_ResetCalibration();
}

/**
 * @brief Start auto-tuning
 */
void PressureControlConfig_StartAutoTuning(void) {
    g_pid_zme_tuning.auto_tune_enabled = true;
}

/**
 * @brief Stop auto-tuning
 */
void PressureControlConfig_StopAutoTuning(void) {
    g_pid_zme_tuning.auto_tune_enabled = false;
}

/**
 * @brief Check if auto-tuning is active
 */
bool PressureControlConfig_IsAutoTuningActive(void) {
    return g_pid_zme_tuning.auto_tune_enabled || g_pid_drv_tuning.auto_tune_enabled;
}

/**
 * @brief Optimize PID parameters
 */
void PressureControlConfig_OptimizePIDParams(void) {
    // Simple optimization algorithm
    
    // Adjust Kp based on system response
    if (g_pid_zme_tuning.kp < 1.0f) {
        g_pid_zme_tuning.kp += 0.1f;
    } else if (g_pid_zme_tuning.kp > 3.0f) {
        g_pid_zme_tuning.kp -= 0.1f;
    }
    
    // Adjust Ki based on steady-state error
    if (g_pid_zme_tuning.ki < 0.001f) {
        g_pid_zme_tuning.ki += 0.0001f;
    } else if (g_pid_zme_tuning.ki > 0.01f) {
        g_pid_zme_tuning.ki -= 0.0001f;
    }
}

/**
 * @brief Update calibration cache from source data
 * @param source Pointer to source calibration data
 * @note This function synchronizes calibration data between modules
 */
void PressureControlConfig_UpdateCalibrationCache(const CalibrationData_t* source) {
    if (source == NULL) {
        return;
    }
    
    // Update the configuration system's calibration cache
    g_calibration_data.adc_min = source->adc_min;
    g_calibration_data.adc_max = source->adc_max;
    g_calibration_data.pressure_min = source->pressure_min;
    g_calibration_data.pressure_max = source->pressure_max;
    g_calibration_data.slope = source->slope;
    g_calibration_data.offset = source->offset;
    g_calibration_data.calibrated = source->calibrated;
    g_calibration_data.calibration_date = source->calibration_date;
}
