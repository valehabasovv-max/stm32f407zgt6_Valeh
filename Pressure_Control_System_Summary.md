# STM32F407ZGT6 Pressure Control System - Implementation Summary

## System Overview

Your STM32F407ZGT6 project now implements a comprehensive hydraulic pressure control system with the following key components:

### 1. ZME Valve Control (Flow Control)
- **Function**: Controls fluid flow into the system
- **Behavior**: 
  - 100% open (0% PWM) when pressure is low (3.5-6 bar input)
  - Gradually closes as pressure increases
  - Fully closed (100% PWM) when target pressure is reached
- **Implementation**: `ZME_ControlBasedOnPressure()` function

### 2. DRV Valve Control (Pressure Relief)
- **Function**: Releases excess pressure when limit is exceeded
- **Behavior**:
  - Closed (100% PWM) when pressure is below limit
  - Opens (0% PWM) when pressure exceeds the set limit
  - Graduated opening based on pressure excess
- **Implementation**: `DRV_ControlBasedOnPressure()` function

### 3. Automatic Pressure Control Mode
- **Function**: Maintains pressure at user-defined limit (e.g., 50 bar)
- **Features**:
  - Configurable pressure limits
  - Automatic valve control based on pressure readings
  - Motor speed adjustment based on pressure difference
  - Safety features and error handling
- **Implementation**: `AutoMode_Process()` function

## Key Functions Implemented

### ZME Valve Functions
```c
void ZME_ControlBasedOnPressure(float current_pressure, float target_pressure);
void ZME_SetFlowControl(float pressure_ratio);
```

### DRV Valve Functions
```c
void DRV_ControlBasedOnPressure(float current_pressure, float pressure_limit);
void DRV_SetPressureRelief(float pressure_ratio);
```

### Auto Mode Functions
```c
void AutoMode_Enable(void);
void AutoMode_Disable(void);
void AutoMode_Process(void);
void AutoMode_SetTargetPressure(float target);
void AutoMode_SetTolerance(float tolerance);
```

## System Behavior

### Normal Operation
1. **Low Pressure (3.5-6 bar)**: ZME valve fully open, DRV valve closed
2. **Building Pressure**: ZME valve gradually closes as pressure increases
3. **Target Pressure**: ZME valve mostly closed, DRV valve closed
4. **Excess Pressure**: DRV valve opens to release pressure

### Auto Mode Operation
- System automatically maintains pressure at set limit
- ZME valve controls flow based on pressure ratio
- DRV valve provides pressure relief when needed
- Motor speed adjusts based on pressure difference

## Hardware Configuration

### PWM Channels (TIM3)
- **Channel 1**: Motor control (PC6)
- **Channel 2**: DRV valve control (PA7)
- **Channel 3**: ZME valve control (PC8)
- **Channel 4**: Extra PWM (PB1)

### Pressure Sensor
- **ADC Channel**: ADC3_CH3
- **Calibration**: Valeh's precise calibration system
- **Reading**: Averaged readings every 100ms

### Display System
- **LCD**: ILI9341 via FSMC
- **Touch**: XPT2046 touch controller
- **Interface**: Interactive pressure control interface

## Safety Features

1. **Pressure Sensor Monitoring**: Detects stuck sensors
2. **Valve Health Monitoring**: Checks for valve failures
3. **Error Handling**: System errors prevent unsafe operation
4. **Pressure Limits**: Configurable maximum pressure limits
5. **Auto Mode Safety**: Stops motor if pressure sensor fails

## Usage Instructions

### Setting Pressure Limit
1. Navigate to pressure limit page
2. Set desired pressure limit (e.g., 50 bar)
3. Enable auto mode
4. System will automatically maintain pressure at set limit

### Manual Control
1. Use test page for manual valve control
2. Adjust PWM percentages directly
3. Monitor pressure readings in real-time

### Calibration
1. Access calibration page
2. Follow calibration procedure
3. Save calibration data to flash memory

## Technical Specifications

- **Pressure Range**: 0-300 bar (configurable)
- **Control Frequency**: 1 kHz PWM
- **Update Rate**: 100ms pressure readings
- **Tolerance**: 0.5 bar (configurable)
- **Memory**: Flash storage for configuration
- **Display**: 320x240 color LCD with touch interface

## System Integration

The pressure control system integrates with:
- Touch screen interface for user interaction
- Flash memory for configuration storage
- Real-time pressure monitoring
- Automatic valve control
- Safety monitoring and error handling

This implementation provides a complete, professional-grade hydraulic pressure control system suitable for industrial applications.

