#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/can.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"
#include "inc/hw_can.h"

//volatile bool g_bRXFlag = 0;
tCANMsgObject rxMessage;
uint8_t rxData[4];

void CANIntHandler(void) {
    uint32_t status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    /*if (status == 1) {
        g_bRXFlag = 1;
        CANIntClear(CAN0_BASE, 1);
    } else if (status == CAN_INT_INTID_STATUS) {
        uint32_t err = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        UARTprintf("Estado de error del CAN: 0x%08X\n", err);
    }*/
    if (status == 0) {
        // no hay interrupciones pendientes
        return;
    }

    if (status == 1) {
        CANMessageGet(CAN0_BASE, 1, &rxMessage, true);  // 'true' limpia automáticamente
        UARTprintf("Mensaje CAN recibido: ID=0x%X, DATA=", rxMessage.ui32MsgID);
        int i;
        for (i = 0; i < rxMessage.ui32MsgLen; i++) {
            UARTprintf("%02X ", rxMessage.pui8MsgData[i]);
        }
        UARTprintf("\n\n");
        SetupRxObject();
    }

    else if (status == CAN_INT_INTID_STATUS) {
        // Interrupción de estado (error)
        uint32_t err = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        UARTprintf("Estado de error del CAN: 0x%08X\n", err);
    }

    // Siempre limpiar la interrupción
    CANIntClear(CAN0_BASE, status);
}

void InitUART(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlDelay(10);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlDelay(10);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);
}

void InitCAN(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlDelay(10);
    GPIOPinConfigure(GPIO_PE4_CAN0RX);
    GPIOPinConfigure(GPIO_PE5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
    SysCtlDelay(10);
    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);
    CANEnable(CAN0_BASE);

    // interrupciones
    CANIntRegister(CAN0_BASE, CANIntHandler);
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    IntEnable(INT_CAN0);

    CANEnable(CAN0_BASE);
}

void SetupRxObject(void) {
    rxMessage.ui32MsgID = 0x01;
    rxMessage.ui32MsgIDMask = 0;
    rxMessage.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;;
    rxMessage.ui32MsgLen = 4;
    rxMessage.pui8MsgData = rxData;

    CANMessageSet(CAN0_BASE, 1, &rxMessage, MSG_OBJ_TYPE_RX);
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    InitUART();
    InitCAN();

    SysCtlDelay(SysCtlClockGet() * 2);
    UARTprintf("Inicio de receptor\n");

    SetupRxObject();

    while (1) {
        /*uint32_t status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        UARTprintf("Estado CAN: 0x%08X\n", status);
        SysCtlDelay(SysCtlClockGet() / 10);*/

        /*if (CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT) & 0x0002) {
            rxMessage.pui8MsgData = rxData;
            CANMessageGet(CAN0_BASE, 1, &rxMessage, true);

            //UARTprintf("CAN recibido: 0x%08X\n", rxData);

            UARTprintf("Mensaje CAN recibido: ID=0x%X, DATA=", rxMessage.ui32MsgID);
            int i;
            for (i = 0; i < rxMessage.ui32MsgLen; i++) {
                UARTprintf("%02X ", rxData[i]);
            }
            UARTprintf("\n\n");
            SetupRxObject();
        }*/


        /*SysCtlDelay(SysCtlClockGet() / 100);  // espera 1 segundo entre lecturas
        UARTprintf("Estoy vivo desde el receptor");
        SysCtlDelay(SysCtlClockGet());*/

        /*if (g_bRXFlag) {
            g_bRXFlag = 0;

            rxMessage.pui8MsgData = rxData;
            CANMessageGet(CAN0_BASE, 1, &rxMessage, 0);

            /*UARTprintf("Mensaje CAN recibido: ID=0x%X, DATA=", rxMessage.ui32MsgID);
            int i;
            for (i = 0; i < rxMessage.ui32MsgLen; i++) {
               UARTprintf("%02X ", rxData[i]);
            }*/
            /*UARTprintf("CAN recibido: 0x%08X\n", rxData);
            UARTprintf("\n\n");

            CANMessageSet(CAN0_BASE, 1, &rxMessage, MSG_OBJ_TYPE_RX);
        }*/
        SysCtlDelay(SysCtlClockGet() / 3);
    }
}
