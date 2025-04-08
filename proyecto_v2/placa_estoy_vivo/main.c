#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/can.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

// buffers
tCANMsgObject txMessage;
uint32_t txData = 0xABCD1234;
uint8_t* pui8TxData = (uint8_t*)&txData;


// Configurar consola UART0
void InitCAN(void) {
    // PE4 = RX y PE5 = TX
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlDelay(10);
    GPIOPinConfigure(GPIO_PE4_CAN0RX);
    GPIOPinConfigure(GPIO_PE5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // para habilitar y configurar el can
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    SysCtlDelay(10);
    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);
    CANEnable(CAN0_BASE);
}

void InitUART(void) {
    // uart temporal
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlDelay(10);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlDelay(10);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTStdioConfig(0, 115200, 16000000);
}

int main(void) {

    // para configurar el reloj
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    InitCAN();

    InitUART();

    SysCtlDelay(SysCtlClockGet() * 2);

    UARTprintf("Emisor vivo\n");

    // mensaje
    txMessage.ui32MsgID = 0x01;
    txMessage.ui32MsgIDMask = 0;
    txMessage.ui32Flags = 0;
    txMessage.ui32MsgLen = 4;
    txMessage.pui8MsgData = pui8TxData;


    while(1) {
        UARTprintf("CAN enviado: 0x%08X\n", txData);
        CANMessageSet(CAN0_BASE, 1, &txMessage, MSG_OBJ_TYPE_TX);
        uint32_t status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        UARTprintf("Estado CAN: 0x%08X\n", status);
        txData++;
        //txMessage.ui32MsgID++;
        SysCtlDelay(SysCtlClockGet() * 5);
    }
}
