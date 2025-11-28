/*
 * ADC Diagnostic Tool Header
 */

#ifndef __ADC_DIAGNOSTIC_H
#define __ADC_DIAGNOSTIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run comprehensive ADC diagnostic
 * This will print detailed information about ADC state, calibration, and readings
 */
void ADC_RunDiagnostic(void);

/**
 * @brief Test ADC hardware directly (bypass filtering/clamping)
 */
void ADC_TestHardwareDirectly(void);

/**
 * @brief Force recalibration with default values
 */
void ADC_ForceRecalibration(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_DIAGNOSTIC_H */
