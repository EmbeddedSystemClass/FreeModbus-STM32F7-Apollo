/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "stm32f7xx_hal.h"

UART_HandleTypeDef UART1_Handler;

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	 if(xRxEnable)
	 {
		SET_BIT(UART1_Handler.Instance->CR1, (USART_CR1_PEIE | USART_CR1_RXNEIE));
	 }
	 else
	 {
		CLEAR_BIT(UART1_Handler.Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
	 }
	 
	 if(xTxEnable)
	 {
		SET_BIT(UART1_Handler.Instance->CR1, USART_CR1_TXEIE);
	 }
	 else
	 {
		CLEAR_BIT(UART1_Handler.Instance->CR1, USART_CR1_TXEIE);
	 }
}

uint8_t aRxBuffer[16] = {0};
BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	(void)ucPORT;
	(void)ucDataBits;
	(void)eParity;
	
	/* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  
    __HAL_RCC_GPIOA_CLK_ENABLE();
	
	/**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	UART1_Handler.Instance=USART1;
	UART1_Handler.Init.BaudRate=115200;
	UART1_Handler.Init.WordLength=UART_WORDLENGTH_8B;
	UART1_Handler.Init.StopBits=UART_STOPBITS_1;
	UART1_Handler.Init.Parity=UART_PARITY_NONE;
	UART1_Handler.Init.HwFlowCtl=UART_HWCONTROL_NONE;
	UART1_Handler.Init.Mode=UART_MODE_TX_RX;
	HAL_UART_Init(&UART1_Handler);
	
	/* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn,3,3);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
	
	return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	UART1_Handler.Instance->TDR = ucByte;
    
	return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
	*pucByte = (UART1_Handler.Instance->RDR & 0xFF);
    
	return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}

void USART1_IRQHandler(void)
{
	uint32_t isrflags   = READ_REG(UART1_Handler.Instance->ISR);
	uint32_t cr1its     = READ_REG(UART1_Handler.Instance->CR1);
	uint32_t cr3its     = READ_REG(UART1_Handler.Instance->CR3);

	if(((isrflags & USART_ISR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
		prvvUARTRxISR();
	}
	
	if(((isrflags & USART_ISR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
	{
		prvvUARTTxReadyISR();
	}
	
	/* UART parity error interrupt occurred -------------------------------------*/
    if(((isrflags & USART_ISR_PE) != RESET) && ((cr1its & USART_CR1_PEIE) != RESET))
    {
        __HAL_UART_CLEAR_IT(&UART1_Handler, UART_CLEAR_PEF);
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if(((isrflags & USART_ISR_FE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
        __HAL_UART_CLEAR_IT(&UART1_Handler, UART_CLEAR_FEF);
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if(((isrflags & USART_ISR_NE) != RESET) && ((cr3its & USART_CR3_EIE) != RESET))
    {
        __HAL_UART_CLEAR_IT(&UART1_Handler, UART_CLEAR_NEF);
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if(((isrflags & USART_ISR_ORE) != RESET) &&
       (((cr1its & USART_CR1_RXNEIE) != RESET) || ((cr3its & USART_CR3_EIE) != RESET)))
    {
        __HAL_UART_CLEAR_IT(&UART1_Handler, UART_CLEAR_OREF);
    }
}
