/*
 * ADC Diagnostic Tool
 * 
 * This file contains diagnostic functions to help identify why:
 * 1. ADC value is stuck at 632
 * 2. Pressure reads 0.00 despite system having pressure
 */

#include "main.h"
#include "advanced_pressure_control.h"
#include <stdio.h>

extern ADC_HandleTypeDef hadc3;
extern CalibrationData_t g_calibration;

/**
 * @brief Comprehensive ADC diagnostic - Run this to identify the issue
 */
void ADC_RunDiagnostic(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("           ADC VƏ TƏZYIQ DİAQNOSTİKASI\n");
    printf("=================================================================\n\n");
    
    // 1. Check ADC state
    printf("--- 1. ADC STATE ---\n");
    uint32_t adc_state = HAL_ADC_GetState(&hadc3);
    printf("ADC State: 0x%08lX\n", adc_state);
    printf("  - Ready: %s\n", (adc_state & HAL_ADC_STATE_READY) ? "YES" : "NO");
    printf("  - Busy (Regular): %s\n", (adc_state & HAL_ADC_STATE_REG_BUSY) ? "YES" : "NO");
    printf("  - EOC Flag: %s\n", __HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_EOC) ? "SET" : "CLEAR");
    printf("  - OVR Flag: %s\n", __HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_OVR) ? "SET" : "CLEAR");
    printf("\n");
    
    // 2. Read raw ADC values multiple times
    printf("--- 2. RAW ADC READINGS (10 consecutive reads) ---\n");
    for (int i = 0; i < 10; i++) {
        uint16_t adc_raw = AdvancedPressureControl_ReadADC();
        printf("  Read %d: ADC = %u\n", i+1, adc_raw);
        HAL_Delay(10);
    }
    printf("\n");
    
    // 3. Check calibration data
    printf("--- 3. CALIBRATION DATA ---\n");
    printf("Calibrated: %s\n", g_calibration.calibrated ? "YES" : "NO");
    printf("ADC Range: %.0f - %.0f\n", g_calibration.adc_min, g_calibration.adc_max);
    printf("Pressure Range: %.2f - %.2f bar\n", 
           g_calibration.pressure_min, g_calibration.pressure_max);
    printf("Slope: %.6f bar/ADC\n", g_calibration.slope);
    printf("Offset: %.2f bar\n", g_calibration.offset);
    printf("\n");
    
    // 4. Test pressure conversion for different ADC values
    printf("--- 4. PRESSURE CONVERSION TEST ---\n");
    uint16_t test_adc_values[] = {620, 632, 700, 1000, 2000, 3000, 4095};
    for (int i = 0; i < 7; i++) {
        uint16_t test_adc = test_adc_values[i];
        float raw_pressure = g_calibration.offset + ((float)test_adc * g_calibration.slope);
        printf("  ADC=%u → Raw Pressure=%.2f bar", test_adc, raw_pressure);
        
        // Check if it gets clamped
        float min_clamp = (g_calibration.pressure_min < 0.0f) ? 0.0f : g_calibration.pressure_min;
        if (raw_pressure < min_clamp) {
            printf(" → CLAMPED to %.2f bar", min_clamp);
        }
        printf("\n");
    }
    printf("\n");
    
    // 5. Check actual current pressure reading
    printf("--- 5. CURRENT SYSTEM STATUS ---\n");
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    printf("Raw ADC Value: %u\n", status->raw_adc_value);
    printf("Current Pressure: %.2f bar\n", status->current_pressure);
    printf("Target Pressure: %.2f bar\n", status->target_pressure);
    printf("Control Enabled: %s\n", status->control_enabled ? "YES" : "NO");
    printf("\n");
    
    // 6. Hardware check
    printf("--- 6. HARDWARE CHECK ---\n");
    printf("ADC3 Instance: 0x%08lX\n", (uint32_t)hadc3.Instance);
    printf("ADC3 Channel: ADC_CHANNEL_3 (PA3)\n");
    printf("Continuous Mode: %s\n", 
           hadc3.Init.ContinuousConvMode == ENABLE ? "ENABLED" : "DISABLED");
    printf("Sampling Time: %lu cycles\n", hadc3.Init.ScanConvMode);
    printf("\n");
    
    // 7. Diagnosis
    printf("--- 7. DIAGNOSIS ---\n");
    uint16_t current_adc = AdvancedPressureControl_ReadADC();
    float current_pressure = status->current_pressure;
    
    // ADC_MIN dəyərini yoxla (voltage divider-ə görə fərqli ola bilər)
    printf("Expected ADC range: %u - %u\n", ADC_MIN, ADC_MAX);
    printf("Current ADC: %u\n", current_adc);
    printf("Current Pressure: %.2f bar\n\n", current_pressure);
    
#if VOLTAGE_DIVIDER_ENABLED
    // Voltage divider ilə: ADC_MIN ≈ 310, ADC_MAX ≈ 3103
    if (current_adc < 250 || current_adc > 3500) {
        printf("⚠ ADC qiymət gözlənilən aralıqdan kənardır!\n");
        printf("  Gözlənilən: %u - %u\n", ADC_MIN, ADC_MAX);
        printf("  Oxunan: %u\n", current_adc);
        printf("  Səbəblər:\n");
        printf("  1. Voltage divider düzgün qoşulmayıb\n");
        printf("  2. Sensor problemlidir\n");
        printf("  3. Kabel qoşulması yoxlanılmalıdır\n");
    }
#else
    // Voltage divider olmadan: ADC_MIN ≈ 620, ADC_MAX ≈ 4095
    if (current_adc < 500 || current_adc > 4095) {
        printf("⚠ ADC qiymət gözlənilən aralıqdan kənardır!\n");
        printf("  Gözlənilən: %u - %u\n", ADC_MIN, ADC_MAX);
        printf("  Oxunan: %u\n", current_adc);
        printf("  Səbəblər:\n");
        printf("  1. Sensor problemlidir\n");
        printf("  2. Kabel qoşulması yoxlanılmalıdır\n");
        printf("  3. Sensor gərginliyi 5V-dan yüksək ola bilməz (STM32 zədələnə bilər!)\n");
    }
#endif
    
    // Təzyiq 0.00 bar problemi yoxlaması
    if (current_pressure < 0.5f && current_adc > ADC_MIN) {
        printf("⚠ Təzyiq 0.00 göstərir, amma ADC > ADC_MIN (%u)!\n", ADC_MIN);
        printf("  Oxunan ADC: %u\n", current_adc);
        printf("  Gözlənilən təzyiq: %.2f bar\n", 
               g_calibration.offset + ((float)current_adc * g_calibration.slope));
        printf("\n  Potensial səbəblər:\n");
        printf("  1. Kalibrasiya məlumatları düzgün yüklənməyib\n");
        printf("  2. Offset (%.2f) çox böyük/kiçikdir\n", g_calibration.offset);
        printf("  3. Slope (%.6f) səhvdir\n", g_calibration.slope);
        printf("\n  TÖVSİYƏ: ADC_ForceRecalibration() çağırın\n");
    }
    
    // ADC dəyəri ADC_MIN-dən aşağıdırsa
    if (current_adc < ADC_MIN) {
        printf("⚠ ADC dəyəri ADC_MIN-dən (%u) aşağıdır: %u\n", ADC_MIN, current_adc);
        printf("  Bu, sensor gərginliyinin 0 bar-dan da aşağı olduğunu göstərir.\n");
        printf("  Səbəblər:\n");
        printf("  1. Sensor düzgün qoşulmayıb\n");
        printf("  2. Sensor qida gərginliyi (5V/12V) yoxlanılmalıdır\n");
        printf("  3. GND bağlantısı yoxlanılmalıdır\n");
    }
    
    printf("\n");
    printf("=================================================================\n");
    printf("           DİAQNOSTİKA TAM\n");
    printf("=================================================================\n\n");
}

/**
 * @brief Test ADC hardware directly (bypass all filtering/clamping)
 */
void ADC_TestHardwareDirectly(void) {
    printf("\n--- DIRECT HARDWARE TEST ---\n");
    
    // Clear any pending flags
    __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_EOC | ADC_FLAG_OVR);
    
    // Stop and restart ADC
    HAL_ADC_Stop(&hadc3);
    HAL_Delay(10);
    HAL_ADC_Start(&hadc3);
    HAL_Delay(20);
    
    printf("Reading ADC directly 20 times:\n");
    for (int i = 0; i < 20; i++) {
        // Wait for EOC
        uint32_t timeout = 100;
        uint32_t start = HAL_GetTick();
        while (__HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_EOC) == RESET) {
            if ((HAL_GetTick() - start) > timeout) {
                printf("  Read %d: TIMEOUT\n", i+1);
                break;
            }
        }
        
        if (__HAL_ADC_GET_FLAG(&hadc3, ADC_FLAG_EOC) != RESET) {
            uint16_t value = (uint16_t)HAL_ADC_GetValue(&hadc3);
            __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_EOC);
            printf("  Read %d: %u\n", i+1, value);
        }
        
        HAL_Delay(50);
    }
    printf("\n");
}

/**
 * @brief Force recalibration with manual values
 * 
 * KRİTİK: Bu funksiya kalibrasiya məlumatlarını default dəyərlərə sıfırlayır.
 * Voltage divider konfiqurasiyasına görə ADC_MIN və ADC_MAX dəyərləri istifadə olunur.
 */
void ADC_ForceRecalibration(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("           KALİBRASİYA SIFILAMA (FORCE RECALIBRATION)\n");
    printf("=================================================================\n");
    
#if VOLTAGE_DIVIDER_ENABLED
    printf("Mode: VOLTAGE DIVIDER AKTİV\n");
    printf("  Sensor: 0.5V (0 bar) - 5.0V (300 bar)\n");
    printf("  Divider: 1:1 (5V → 2.5V, 0.5V → 0.25V)\n");
    printf("  ADC: %u (0 bar) - %u (300 bar)\n", ADC_MIN, ADC_MAX);
    printf("  Formula: Pressure = (ADC - %u) * %.4f bar\n", ADC_MIN, 
           (300.0f / ((float)ADC_MAX - (float)ADC_MIN)));
    
    g_calibration.adc_min = (float)ADC_MIN;
    g_calibration.adc_max = (float)ADC_MAX;
#else
    printf("Mode: VOLTAGE DIVIDER YOX (Direct Connection)\n");
    printf("  Sensor: 0.5V (0 bar) - 5.0V (300 bar)\n");
    printf("  ⚠ XƏBƏRDARLIQ: STM32 ADC maksimum 3.3V qəbul edir!\n");
    printf("  ⚠ 230 bar-dan yuxarı təzyiqlər düzgün ölçülməyəcək!\n");
    printf("  ADC: %u (0 bar) - %u (~230 bar max)\n", ADC_MIN, ADC_MAX);
    
    g_calibration.adc_min = (float)ADC_MIN;
    g_calibration.adc_max = (float)ADC_MAX;
#endif
    
    g_calibration.pressure_min = 0.0f;
    g_calibration.pressure_max = 300.0f;
    g_calibration.slope = (300.0f - 0.0f) / ((float)ADC_MAX - (float)ADC_MIN);
    g_calibration.offset = 0.0f - (g_calibration.slope * (float)ADC_MIN);
    g_calibration.calibrated = true;
    
    printf("\nYeni kalibrasiya dəyərləri:\n");
    printf("  ADC Range: %.0f - %.0f\n", g_calibration.adc_min, g_calibration.adc_max);
    printf("  Pressure Range: %.2f - %.2f bar\n", g_calibration.pressure_min, g_calibration.pressure_max);
    printf("  Slope: %.6f bar/ADC\n", g_calibration.slope);
    printf("  Offset: %.2f bar\n", g_calibration.offset);
    
    // Test with current ADC reading
    printf("\nTest nəticəsi:\n");
    uint16_t current_adc = AdvancedPressureControl_ReadADC();
    float test_pressure = g_calibration.offset + ((float)current_adc * g_calibration.slope);
    
    // Clamp negative pressure to 0
    if (test_pressure < 0.0f) {
        printf("  ADC=%u → Raw=%.2f bar → Clamped=0.00 bar (mənfi dəyər 0-a clamp olundu)\n", 
               current_adc, test_pressure);
        test_pressure = 0.0f;
    } else {
        printf("  ADC=%u → Pressure=%.2f bar\n", current_adc, test_pressure);
    }
    
    // Show pressure for different ADC values
    printf("\nADC-Təzyiq cədvəli (yeni kalibrasiya ilə):\n");
    uint16_t test_values[] = {ADC_MIN, (ADC_MIN + ADC_MAX)/4, (ADC_MIN + ADC_MAX)/2, 
                              3*(ADC_MIN + ADC_MAX)/4, ADC_MAX};
    for (int i = 0; i < 5; i++) {
        float p = g_calibration.offset + ((float)test_values[i] * g_calibration.slope);
        if (p < 0.0f) p = 0.0f;
        if (p > 300.0f) p = 300.0f;
        printf("  ADC=%u → %.2f bar\n", test_values[i], p);
    }
    
    // Save to flash
    AdvancedPressureControl_SaveCalibration();
    printf("\nKalibrasiya Flash yaddaşa qeyd edildi.\n");
    printf("=================================================================\n\n");
}
