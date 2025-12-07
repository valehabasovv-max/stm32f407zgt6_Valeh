/**
 * @file uart_redirect_helper.c
 * @brief UART redirect helper for printf output in emulator
 * 
 * This file provides a simple UART redirect for printf() output.
 * Add this to your project if you want printf to work in the emulator.
 * 
 * To use:
 * 1. Configure UART2 in STM32CubeMX
 * 2. Include this file in your build
 * 3. Call UART_Redirect_Init() after HAL_Init()
 */

#include "stm32f4xx_hal.h"
#include <stdio.h>

// UART handle (you need to configure this in CubeMX)
extern UART_HandleTypeDef huart2;

/**
 * @brief Redirect printf to UART2
 * This function is called by syscalls.c _write()
 */
int __io_putchar(int ch) {
    // Send character via UART2
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief Get character from UART2 (for scanf, etc.)
 */
int __io_getchar(void) {
    uint8_t ch;
    HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

/**
 * @brief Initialize UART redirect
 * Call this after HAL_Init() and UART initialization
 */
void UART_Redirect_Init(void) {
    // UART should already be initialized by MX_USART2_UART_Init()
    // This is just a placeholder for any additional setup
}




