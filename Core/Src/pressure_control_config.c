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

Calibration_Data_t g_calibration_data = {
    .adc_min = CONFIG_PRESSURE_SENSOR_ADC_MIN,
    .adc_max = CONFIG_PRESSURE_SENSOR_ADC_MAX,
    .pressure_min = CONFIG_PRESSURE_SENSOR_PRESSURE_MIN,
    .pressure_max = CONFIG_PRESSURE_SENSOR_PRESSURE_MAX,
    .slope = 0.0f,
    .offset = 0.0f,
    .calibrated = false,
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
    .safety_enabled = true
};

/* =========================================================================
   CONFIGURATION MANAGEMENT FUNCTIONS
   ========================================================================= */

/**
 * @brief Initialize configuration system
 */
void PressureControlConfig_Init(void) {
    printf("Pressure Control Configuration - Initializing...\r\n");
    
    // Load configuration from flash
    PressureControlConfig_LoadFromFlash();
    
    // Load PID parameters and SP from flash (important - must be called early)
    PressureControlConfig_LoadPIDParams();
    
    // Apply PID tuning parameters to AdvancedPressureControl system
    // This ensures that g_pid_zme_tuning and g_pid_drv_tuning values are applied
    // even if flash loading didn't happen or returned early
    PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 
                                       g_pid_zme_tuning.kp, 
                                       g_pid_zme_tuning.ki, 
                                       g_pid_zme_tuning.kd);
    PressureControlConfig_SetPIDTuning(&g_pid_drv_tuning, 
                                       g_pid_drv_tuning.kp, 
                                       g_pid_drv_tuning.ki, 
                                       g_pid_drv_tuning.kd);
    
    // Calculate calibration slope
    if (g_calibration_data.calibrated) {
        g_calibration_data.slope = (g_calibration_data.pressure_max - g_calibration_data.pressure_min) / 
                                  (g_calibration_data.adc_max - g_calibration_data.adc_min);
        g_calibration_data.offset = g_calibration_data.pressure_min - 
                                   (g_calibration_data.slope * g_calibration_data.adc_min);
    }
    
    printf("Configuration initialized successfully\r\n");
    PressureControlConfig_PrintSystemInfo();
}

/**
 * @brief Load default configuration
 */
void PressureControlConfig_LoadDefaults(void) {
    printf("Loading default configuration...\r\n");
    
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
    
    g_valve_config.zme_pwm_min = CONFIG_ZME_PWM_MIN;
    g_valve_config.zme_pwm_max = CONFIG_ZME_PWM_MAX;
    g_valve_config.zme_cutoff_pwm = CONFIG_ZME_CUTOFF_PWM;
    
    printf("Default configuration loaded\r\n");
}

/**
 * @brief Save configuration to flash memory
 */
void PressureControlConfig_SaveToFlash(void) {
    printf("Saving system configuration to flash...\r\n");
    
    // Save all configuration data
    PressureControlConfig_SavePIDParams();
    PressureControlConfig_SaveCalibrationData();
    
    printf("System configuration saved to flash memory\r\n");
}

/**
 * @brief Load configuration from flash memory
 */
void PressureControlConfig_LoadFromFlash(void) {
    printf("Loading system configuration from flash...\r\n");
    
    // Load all configuration data
    PressureControlConfig_LoadPIDParams();
    PressureControlConfig_LoadCalibrationData();
    
    printf("System configuration loaded from flash memory\r\n");
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
        
        printf("PID tuning updated: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", kp, ki, kd);
    } else {
        printf("Invalid PID parameters: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", kp, ki, kd);
    }
}

/**
 * @brief Enable/disable auto-tuning
 */
void PressureControlConfig_EnableAutoTune(PID_Tuning_t* tuning, bool enable) {
    tuning->auto_tune_enabled = enable;
    printf("Auto-tuning %s for PID controller\r\n", enable ? "enabled" : "disabled");
}

/**
 * @brief Set tuning aggressiveness
 */
void PressureControlConfig_SetTuningAggressiveness(PID_Tuning_t* tuning, float aggressiveness) {
    tuning->tuning_aggressiveness = fmaxf(0.1f, fminf(2.0f, aggressiveness));
    printf("Tuning aggressiveness set to %.2f\r\n", tuning->tuning_aggressiveness);
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
    
    printf("PID tuning reset to defaults\r\n");
}

/* =========================================================================
   CALIBRATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Start calibration process
 */
void PressureControlConfig_StartCalibration(void) {
    printf("Starting pressure sensor calibration...\r\n");
    printf("Please ensure pressure sensor is at 0 bar (atmospheric pressure)\r\n");
    
    g_calibration_data.calibrated = false;
    g_calibration_data.calibration_date = HAL_GetTick();
}

/**
 * @brief Add calibration point
 */
void PressureControlConfig_AddCalibrationPoint(float adc_value, float pressure_value) {
    printf("Calibration point added: ADC=%.0f, Pressure=%.2f bar\r\n", adc_value, pressure_value);
    
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
 */
void PressureControlConfig_CompleteCalibration(void) {
    if (g_calibration_data.adc_min > 0 && g_calibration_data.adc_max > g_calibration_data.adc_min) {
        // Calculate slope and offset
        g_calibration_data.slope = (g_calibration_data.pressure_max - g_calibration_data.pressure_min) / 
                                  (g_calibration_data.adc_max - g_calibration_data.adc_min);
        g_calibration_data.offset = g_calibration_data.pressure_min - 
                                   (g_calibration_data.slope * g_calibration_data.adc_min);
        g_calibration_data.calibrated = true;
        
        printf("Calibration completed successfully\r\n");
        PressureControlConfig_PrintCalibrationData();
        
        // Save calibration data
        PressureControlConfig_SaveCalibrationData();
    } else {
        printf("Calibration failed: Invalid data points\r\n");
    }
}

/**
 * @brief Reset calibration data
 */
void PressureControlConfig_ResetCalibration(void) {
    g_calibration_data.calibrated = false;
    g_calibration_data.slope = 0.0f;
    g_calibration_data.offset = 0.0f;
    printf("Calibration data reset\r\n");
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
    printf("\n=== Calibration Data ===\r\n");
    printf("ADC Min: %.0f\r\n", g_calibration_data.adc_min);
    printf("ADC Max: %.0f\r\n", g_calibration_data.adc_max);
    printf("Pressure Min: %.2f bar\r\n", g_calibration_data.pressure_min);
    printf("Pressure Max: %.2f bar\r\n", g_calibration_data.pressure_max);
    printf("Slope: %.6f bar/ADC\r\n", g_calibration_data.slope);
    printf("Offset: %.2f bar\r\n", g_calibration_data.offset);
    printf("Calibrated: %s\r\n", g_calibration_data.calibrated ? "Yes" : "No");
    printf("=======================\r\n\n");
}

/* =========================================================================
   VALVE CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set valve limits
 */
void PressureControlConfig_SetValveLimits(Valve_Config_t* config) {
    g_valve_config = *config;
    printf("Valve limits updated\r\n");
}

/**
 * @brief Set ZME valve limits
 */
void PressureControlConfig_SetZMELimits(float pwm_min, float pwm_max, float cutoff_pwm) {
    g_valve_config.zme_pwm_min = pwm_min;
    g_valve_config.zme_pwm_max = pwm_max;
    g_valve_config.zme_cutoff_pwm = cutoff_pwm;
    printf("ZME limits: Min=%.1f%%, Max=%.1f%%, Cutoff=%.1f%%\r\n", pwm_min, pwm_max, cutoff_pwm);
}

/**
 * @brief Set DRV valve limits
 */
void PressureControlConfig_SetDRVLimits(float pwm_min, float pwm_max) {
    g_valve_config.drv_pwm_min = pwm_min;
    g_valve_config.drv_pwm_max = pwm_max;
    printf("DRV limits: Min=%.1f%%, Max=%.1f%%\r\n", pwm_min, pwm_max);
}

/**
 * @brief Set motor limits
 */
void PressureControlConfig_SetMotorLimits(float pwm_min, float pwm_max) {
    g_valve_config.motor_pwm_min = pwm_min;
    g_valve_config.motor_pwm_max = pwm_max;
    printf("Motor limits: Min=%.1f%%, Max=%.1f%%\r\n", pwm_min, pwm_max);
}

/* =========================================================================
   SAFETY CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set safety limits
 */
void PressureControlConfig_SetSafetyLimits(float max_pressure, float over_limit_margin) {
    g_safety_config.max_pressure = max_pressure;
    g_safety_config.over_limit_margin = over_limit_margin;
    printf("Safety limits: Max=%.1f bar, Over-limit=%.1f bar\r\n", 
           max_pressure, over_limit_margin);
}

/**
 * @brief Enable/disable safety system
 */
void PressureControlConfig_EnableSafety(bool enable) {
    g_safety_config.safety_enabled = enable;
    printf("Safety system %s\r\n", enable ? "enabled" : "disabled");
}

/* =========================================================================
   SYSTEM CONFIGURATION FUNCTIONS
   ========================================================================= */

/**
 * @brief Set debug mode
 */
void PressureControlConfig_SetDebugMode(bool enable) {
    g_system_config.debug_enabled = enable;
    printf("Debug mode %s\r\n", enable ? "enabled" : "disabled");
}

/**
 * @brief Print system information
 */
void PressureControlConfig_PrintSystemInfo(void) {
    printf("\n=== System Information ===\r\n");
    printf("System: %s\r\n", g_system_config.system_name);
    printf("Version: %s\r\n", g_system_config.version);
    printf("Build Date: %s\r\n", g_system_config.build_date);
    printf("Debug Mode: %s\r\n", g_system_config.debug_enabled ? "Enabled" : "Disabled");
    printf("Safety System: %s\r\n", g_system_config.safety_enabled ? "Enabled" : "Disabled");
    printf("==========================\r\n\n");
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
    printf("\n=== Current Configuration ===\r\n");
    printf("ZME PID: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", 
           g_pid_zme_tuning.kp, g_pid_zme_tuning.ki, g_pid_zme_tuning.kd);
    printf("DRV PID: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", 
           g_pid_drv_tuning.kp, g_pid_drv_tuning.ki, g_pid_drv_tuning.kd);
    printf("Safety: Max=%.1f bar, Over-limit=%.1f bar\r\n", 
           g_safety_config.max_pressure, g_safety_config.over_limit_margin);
    printf("ZME Limits: %.1f%% - %.1f%%, Cutoff=%.1f%%\r\n", 
           g_valve_config.zme_pwm_min, g_valve_config.zme_pwm_max, g_valve_config.zme_cutoff_pwm);
    printf("DRV Limits: %.1f%% - %.1f%%\r\n", 
           g_valve_config.drv_pwm_min, g_valve_config.drv_pwm_max);
    printf("Motor Limits: %.1f%% - %.1f%%\r\n", 
           g_valve_config.motor_pwm_min, g_valve_config.motor_pwm_max);
    printf("=============================\r\n\n");
}

/**
 * @brief Print tuning status
 */
void PressureControlConfig_PrintTuningStatus(void) {
    printf("\n=== PID Tuning Status ===\r\n");
    printf("ZME Auto-tune: %s\r\n", g_pid_zme_tuning.auto_tune_enabled ? "Active" : "Inactive");
    printf("DRV Auto-tune: %s\r\n", g_pid_drv_tuning.auto_tune_enabled ? "Active" : "Inactive");
    printf("ZME Aggressiveness: %.2f\r\n", g_pid_zme_tuning.tuning_aggressiveness);
    printf("DRV Aggressiveness: %.2f\r\n", g_pid_drv_tuning.tuning_aggressiveness);
    printf("=========================\r\n\n");
}

/**
 * @brief Print calibration status
 */
void PressureControlConfig_PrintCalibrationStatus(void) {
    printf("\n=== Calibration Status ===\r\n");
    printf("Calibrated: %s\r\n", g_calibration_data.calibrated ? "Yes" : "No");
    if (g_calibration_data.calibrated) {
        printf("ADC Range: %.0f - %.0f\r\n", g_calibration_data.adc_min, g_calibration_data.adc_max);
        printf("Pressure Range: %.2f - %.2f bar\r\n", g_calibration_data.pressure_min, g_calibration_data.pressure_max);
        printf("Slope: %.6f bar/ADC\r\n", g_calibration_data.slope);
    }
    printf("==========================\r\n\n");
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
    printf("Saving PID parameters to flash...\r\n");
    
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
    printf("Loading PID parameters from flash (Config system, offset 0x200)...\r\n");
    
    // Read data from flash
    Flash_Config_Data_t *config_data = (Flash_Config_Data_t*)FLASH_CONFIG_ADDRESS;
    
    printf("Flash address: 0x%08lX\r\n", (uint32_t)FLASH_CONFIG_ADDRESS);
    printf("Read magic: 0x%08lX (expected: 0x%08lX)\r\n", 
           config_data->magic, FLASH_CONFIG_MAGIC);
    
    // Verify magic number
    if (config_data->magic != FLASH_CONFIG_MAGIC) {
        printf("No valid configuration found in flash, using defaults\r\n");
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
    
    printf("Read checksum: 0x%08lX, Calculated: 0x%08lX\r\n",
           config_data->checksum, calculated_checksum);
    
    if (calculated_checksum != config_data->checksum) {
        printf("Checksum mismatch, using defaults\r\n");
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
        printf("Old default PID values detected in flash (Kp=%.3f, Ki=%.4f, Kd=%.3f), updating to new defaults\r\n",
               config_data->pid_kp, config_data->pid_ki, config_data->pid_kd);
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
    
    // Load and set SetPoint - DÜZƏLİŞ: Advanced sistem istifadə et
    if (config_data->setpoint > 0.1f && config_data->setpoint <= 300.0f) {
        AdvancedPressureControl_SetTargetPressure(config_data->setpoint);
        // REMOVED: pressure_limit sinxronizasiyası - artıq pressure_limit yoxdur
        printf("TARGET PRESSURE FLASH-DAN YÜKLƏNDİ: %.1f bar\r\n", config_data->setpoint);
    } else {
        // Flash-da etibarlı dəyər yoxdursa, default dəyəri istifadə et
        SystemStatus_t* status = AdvancedPressureControl_GetStatus();
        printf("FLASH-DA ETİBARLI TARGET PRESSURE YOXDUR, DEFAULT DƏYƏR İSTİFADƏ OLUNUR: %.1f bar\r\n", 
               status->target_pressure);
    }
    
    printf("PID parameters loaded: Kp=%.3f, Ki=%.3f, Kd=%.3f, SP=%.1f\r\n",
           config_data->pid_kp, config_data->pid_ki,
           config_data->pid_kd, config_data->setpoint);
}

/**
 * @brief Save calibration data to flash
 * 
 * KRİTİK DÜZƏLİŞ: Bu funksiya artıq mərkəzləşdirilmiş Flash yaddaş məntiqindən istifadə edir.
 * Bütün əl ilə bərpa məntiqi silindi.
 */
void PressureControlConfig_SaveCalibrationData(void) {
    printf("Saving calibration data to flash...\r\n");
    
    // Ensure cache mirrors the latest Advanced calibration snapshot before persisting
    extern CalibrationData_t g_calibration;
    if (g_calibration.calibrated) {
        PressureControlConfig_UpdateCalibrationCache(&g_calibration);
    }
    
    // Prepare calibration data structure
    typedef struct {
        uint32_t magic;           /* Magic number: 0x12345678 */
        float min_voltage;        /* 0.5V */
        float max_voltage;        /* 5.0V */
        float min_pressure;       /* 0.0 bar */
        float max_pressure;       /* 300.0 bar */
        uint16_t adc_min;         /* 410 */
        uint16_t adc_max;         /* 4096 */
        uint32_t checksum;         /* Data integrity check */
    } calibration_data_t;
    
    float adc_min_value = g_calibration_data.adc_min;
    float adc_max_value = g_calibration_data.adc_max;
    float min_pressure_value = g_calibration_data.pressure_min;
    float max_pressure_value = g_calibration_data.pressure_max;
    bool source_from_advanced = false;

    if (g_calibration.calibrated) {
        adc_min_value = g_calibration.adc_min;
        adc_max_value = g_calibration.adc_max;
        min_pressure_value = g_calibration.pressure_min;
        max_pressure_value = g_calibration.pressure_max;
        source_from_advanced = true;
    } else if (!g_calibration_data.calibrated) {
        // Fallback to hard-coded defaults if no calibration exists anywhere
        adc_min_value = CONFIG_PRESSURE_SENSOR_ADC_MIN;
        adc_max_value = CONFIG_PRESSURE_SENSOR_ADC_MAX;
        min_pressure_value = CONFIG_PRESSURE_SENSOR_PRESSURE_MIN;
        max_pressure_value = CONFIG_PRESSURE_SENSOR_PRESSURE_MAX;
    }

    calibration_data_t cal_data;
    cal_data.magic = 0x12345678;
    cal_data.min_voltage = 0.5f;  // Default values, kept for compatibility
    cal_data.max_voltage = 5.0f;
    cal_data.min_pressure = min_pressure_value;
    cal_data.max_pressure = max_pressure_value;
    cal_data.adc_min = (uint16_t)(adc_min_value + 0.5f);  // Round to nearest integer
    cal_data.adc_max = (uint16_t)(adc_max_value + 0.5f);
    
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
        // Verify data was written correctly by reading it back
        calibration_data_t *verify_data = (calibration_data_t*)(0x080E0000 + 0x100);
        if (verify_data->magic == 0x12345678 && verify_data->checksum == cal_data.checksum) {
            printf("Calibration data saved and verified (%s source): ADC %d-%d, Pressure %.2f-%.2f bar\r\n",
                   source_from_advanced ? "Advanced" : "Config",
                   cal_data.adc_min, cal_data.adc_max, cal_data.min_pressure, cal_data.max_pressure);

            // Keep config-side cache in sync with the authoritative calibration
            g_calibration_data.adc_min = (float)cal_data.adc_min;
            g_calibration_data.adc_max = (float)cal_data.adc_max;
            g_calibration_data.pressure_min = cal_data.min_pressure;
            g_calibration_data.pressure_max = cal_data.max_pressure;
            float adc_range = g_calibration_data.adc_max - g_calibration_data.adc_min;
            if (adc_range <= 0.0f) {
                adc_range = 1.0f;  // Prevent divide-by-zero
            }
            g_calibration_data.slope = (g_calibration_data.pressure_max - g_calibration_data.pressure_min) / adc_range;
            g_calibration_data.offset = g_calibration_data.pressure_min -
                                       (g_calibration_data.slope * g_calibration_data.adc_min);
            g_calibration_data.calibrated = true;
        } else {
            printf("ERROR: Calibration write verification failed!\r\n");
        }
    } else {
        printf("ERROR: Failed to save calibration data to flash!\r\n");
    }
}

/**
 * @brief Load calibration data from flash
 * 
 * KRİTİK DÜZƏLİŞ: Bu funksiya artıq Advanced sistemin vahid strukturundan istifadə edir.
 * Köhnə strukturlar (ADC 500-3500) silindi və AdvancedPressureControl_LoadCalibration() çağırılır.
 */
void PressureControlConfig_LoadCalibrationData(void) {
    printf("Loading calibration data from flash...\r\n");
    
    // KRİTİK DÜZƏLİŞ: Advanced sistemin vahid strukturundan istifadə et
    // Bu, köhnə strukturlar (ADC 500-3500) ilə yeni strukturlar (ADC 410-4096) arasında ziddiyyəti aradan qaldırır
    AdvancedPressureControl_LoadCalibration();
    
    // Advanced sistemdən yüklənən kalibrasiya məlumatlarını g_calibration_data strukturuna köçür
    extern CalibrationData_t g_calibration;  // advanced_pressure_control.c-dən (CalibrationData_t tipi)
    
    if (g_calibration.calibrated) {
        PressureControlConfig_UpdateCalibrationCache(&g_calibration);
        
        printf("PressureControlConfig: Calibration loaded from Advanced system - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar\r\n",
               g_calibration_data.adc_min, g_calibration_data.adc_max, 
               g_calibration_data.pressure_min, g_calibration_data.pressure_max);
    } else {
        printf("No valid calibration data found, using defaults\r\n");
    }
    
    // Köhnə kod silindi - artıq Advanced sistemin vahid strukturundan istifadə edirik
}

void PressureControlConfig_UpdateCalibrationCache(const CalibrationData_t* source) {
    if (source == NULL) {
        return;
    }

    g_calibration_data.adc_min = source->adc_min;
    g_calibration_data.adc_max = source->adc_max;
    g_calibration_data.pressure_min = source->pressure_min;
    g_calibration_data.pressure_max = source->pressure_max;
    g_calibration_data.slope = source->slope;
    g_calibration_data.offset = source->offset;
    g_calibration_data.calibrated = source->calibrated;
    g_calibration_data.calibration_date = source->calibration_date;
}

/* =========================================================================
   MISSING FUNCTION IMPLEMENTATIONS
   ========================================================================= */

/**
 * @brief Backup configuration
 */
void PressureControlConfig_BackupConfiguration(void) {
    printf("Configuration backup - Starting...\r\n");
    
    // Save all configuration to flash
    PressureControlConfig_SaveToFlash();
    PressureControlConfig_SavePIDParams();
    PressureControlConfig_SaveCalibrationData();
    
    printf("Configuration backup - Complete\r\n");
}

/**
 * @brief Restore configuration
 */
void PressureControlConfig_RestoreConfiguration(void) {
    printf("Configuration restore - Starting...\r\n");
    
    // Load all configuration from flash
    PressureControlConfig_LoadFromFlash();
    PressureControlConfig_LoadPIDParams();
    PressureControlConfig_LoadCalibrationData();
    
    printf("Configuration restore - Complete\r\n");
}

/**
 * @brief Reset to defaults
 */
void PressureControlConfig_ResetToDefaults(void) {
    printf("Configuration reset to defaults - Starting...\r\n");
    
    // Load default configuration
    PressureControlConfig_LoadDefaults();
    
    // Reset calibration
    PressureControlConfig_ResetCalibration();
    
    printf("Configuration reset to defaults - Complete\r\n");
}

/**
 * @brief Start auto-tuning
 */
void PressureControlConfig_StartAutoTuning(void) {
    printf("Auto-tuning started for ZME controller\r\n");
    g_pid_zme_tuning.auto_tune_enabled = true;
}

/**
 * @brief Stop auto-tuning
 */
void PressureControlConfig_StopAutoTuning(void) {
    printf("Auto-tuning stopped for ZME controller\r\n");
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
    printf("PID parameter optimization - Starting...\r\n");
    
    // Simple optimization algorithm
    // This is a placeholder - real optimization would be more complex
    
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
    
    printf("PID parameter optimization - Complete\r\n");
    printf("New ZME PID: Kp=%.3f, Ki=%.3f, Kd=%.3f\r\n", 
           g_pid_zme_tuning.kp, g_pid_zme_tuning.ki, g_pid_zme_tuning.kd);
}
