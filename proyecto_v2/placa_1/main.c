/*
 * Tiva 2: a4 en el esquema del ejercicio
 */

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

// sincronizacion
tCANMsgObject rxSyncMsg;
tCANMsgObject txAckMsg;
uint32_t ackData = 0x11223344;
volatile bool sincronizado = false;

// ejercicio diapos
uint32_t tiempo_computo_tarea = 5; // en ms
uint32_t periodo_tarea = 4000; // en ms
tCANMsgObject rxMsg_e1234;
uint8_t rxData_e1234[8];
volatile bool ejecutar_a3 = false;
uint32_t tiempo_computo_tarea3 = 20; // en ms

void CANIntHandler(void) {
    uint32_t status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    if (status == 0) {
        // no hay interrupciones pendientes
        return;
    }

    if (status == 1) {
        // SINCRONIZACION
        rxSyncMsg.pui8MsgData = rxData;
        CANMessageGet(CAN0_BASE, 1, &rxSyncMsg, true);
        if (rxSyncMsg.ui32MsgID == 0x10) {
            UARTprintf("Sync recibido, enviando ACK\n");
            CANMessageSet(CAN0_BASE, 2, &txAckMsg, MSG_OBJ_TYPE_TX);
            sincronizado = true;
        }
        CANMessageSet(CAN0_BASE, 1, &rxSyncMsg, MSG_OBJ_TYPE_RX);
    }
    if (status == 2) {
        // MENSAJES RECIBIDOS DE TIVA 1
        rxMsg_e1234.pui8MsgData = rxData_e1234;
        CANMessageGet(CAN0_BASE, 2, &rxMsg_e1234, true);
        if (rxMsg_e1234.ui32MsgID == 0x12) {
            //UARTprintf("Tiva 2: e1234 recibido. Contenido: 0x%02X\n", rxData_e1234[0]);
            ejecutar_a3 = true;
        }
        CANMessageSet(CAN0_BASE, 2, &rxMsg_e1234, MSG_OBJ_TYPE_RX);
    }

    else if (status == CAN_INT_INTID_STATUS) {
        // Interrupción de estado (error)
        /*uint32_t err = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        UARTprintf("Estado de error del CAN: 0x%08X\n", err);*/
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
    rxMessage.ui32MsgIDMask = 0x7FF;
    rxMessage.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;;
    rxMessage.ui32MsgLen = 4;
    rxMessage.pui8MsgData = rxData;

    CANMessageSet(CAN0_BASE, 1, &rxMessage, MSG_OBJ_TYPE_RX);
}

// sincronizacion
void SetupRxSync(void) {
    rxSyncMsg.ui32MsgID = 0x10;
    rxSyncMsg.ui32MsgIDMask = 0x7FF;
    rxSyncMsg.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    rxSyncMsg.ui32MsgLen = 4;
    rxSyncMsg.pui8MsgData = rxData;
    CANMessageSet(CAN0_BASE, 1, &rxSyncMsg, MSG_OBJ_TYPE_RX);
}

void SetupTxAck(void) {
    txAckMsg.ui32MsgID = 0x11;
    txAckMsg.ui32MsgIDMask = 0;
    txAckMsg.ui32Flags = 0;
    txAckMsg.ui32MsgLen = 4;
    txAckMsg.pui8MsgData = (uint8_t*)&ackData;
}

void EjecutarTarea4(uint32_t tiempo_ms) {
    UARTprintf("Tarea a4 se ejecuta durante su tiempo de computo %u ms\n", tiempo_ms);
    SysCtlDelay((SysCtlClockGet() / 3000) * tiempo_ms);
}

void EjecutarTarea3(uint32_t tiempo_ms) {
    UARTprintf("Tarea a3 se ejecuta durante su tiempo de computo %u ms\n", tiempo_ms);
    SysCtlDelay((SysCtlClockGet() / 3000) * tiempo_ms);
}

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    InitUART();
    InitCAN();

    SysCtlDelay(SysCtlClockGet() * 2);
    UARTprintf("Inicio de receptor\n");

    SetupTxAck();
    SetupRxSync();

    while (!sincronizado) {
        UARTprintf("Esperando sincronizacion...\n");
        SysCtlDelay(SysCtlClockGet() * 2);
    }
    //SetupRxObject();
    UARTprintf("Sincronizacion completa. Iniciando tarea periodica\n");

    rxMsg_e1234.ui32MsgID = 0x12;
    rxMsg_e1234.ui32MsgIDMask = 0x7FF;
    rxMsg_e1234.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    rxMsg_e1234.ui32MsgLen = 1;
    rxMsg_e1234.pui8MsgData = rxData_e1234;

    CANMessageSet(CAN0_BASE, 2, &rxMsg_e1234, MSG_OBJ_TYPE_RX);


    while (1) {
        EjecutarTarea4(tiempo_computo_tarea);
        if (ejecutar_a3) {
            ejecutar_a3 = false;
            EjecutarTarea3(tiempo_computo_tarea3);
        }
        SysCtlDelay((SysCtlClockGet() / 3000) * (periodo_tarea - tiempo_computo_tarea));
    }
}
