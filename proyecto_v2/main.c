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

//buffers
tCANMsgObject txMessage, rxMessage;
uint32_t txData = 0x12345678;
uint8_t* pui8TxData = (uint8_t*)&txData;
uint8_t rxData[8];

// Configurar consola UART0
void InitConsole(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);
}

int main(void) {

    // para configurar el reloj
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    InitConsole();

    SysCtlDelay(SysCtlClockGet() / 3); // delay para que no se buguee


    UARTprintf("Inicio de test\n");

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB); // CAMBIAR
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    CANInit(CAN0_BASE);

    // estas dos lineas activan el modo loopback interno, el periferico
    // CAN dentro de la tiva se habla a si mismo
    HWREG(CAN0_BASE + CAN_O_CTL) |= CAN_CTL_TEST;
    HWREG(CAN0_BASE + CAN_O_TST) |= CAN_TST_LBACK;

    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);
    CANEnable(CAN0_BASE);

    rxMessage.ui32MsgID = 0;
    rxMessage.ui32MsgIDMask = 0;
    rxMessage.ui32Flags = MSG_OBJ_USE_ID_FILTER;
    rxMessage.ui32MsgLen = 8;
    CANMessageSet(CAN0_BASE, 2, &rxMessage, MSG_OBJ_TYPE_RX);

    txMessage.ui32MsgID = 1;
    txMessage.ui32MsgIDMask = 0;
    txMessage.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    txMessage.ui32MsgLen = 4;
    txMessage.pui8MsgData = pui8TxData;

    while(1) {


        UARTprintf("TX -> 0x%08X\n", txData);
        CANMessageSet(CAN0_BASE, 1, &txMessage, MSG_OBJ_TYPE_TX);

        // delay
        SysCtlDelay(SysCtlClockGet() / 30);

        // leer mensaje recibido
        rxMessage.pui8MsgData = rxData;
        CANMessageGet(CAN0_BASE, 2, &rxMessage, 0);

        // mostramos por la uart el mensaje recibido
        UARTprintf("RX <- ID=0x%X, DATA=", rxMessage.ui32MsgID);
        int i;
        for (i = 0; i < rxMessage.ui32MsgLen; i++) {
            UARTprintf("%02X ", rxData[i]);
        }
        UARTprintf("\n\n");

        // esperamos 1 segundo
        txData++;
        txMessage.ui32MsgID++;
        if (txMessage.ui32MsgID == 20) {
            UARTprintf("Fin de test\n");
            break;
        }
        SysCtlDelay(SysCtlClockGet());
    }
}
