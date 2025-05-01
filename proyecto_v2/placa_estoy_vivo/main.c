/*
 * Tiva 1: a1 en el esquema del ejercicio
 */

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
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"

// buffers
tCANMsgObject txMessage_1;
tCANMsgObject txMessage_2;
uint32_t txData_1 = 0xABCD1234;
uint32_t txData_2 = 0x00000000;
uint8_t* pui8TxData_1 = (uint8_t*)&txData_1;
uint8_t* pui8TxData_2 = (uint8_t*)&txData_2;

// sincronizacion
volatile bool g_bSyncReceived = false;
tCANMsgObject txSyncMsg;
tCANMsgObject rxAckMsg;
uint32_t txData = 0xAABBCCDD;
uint8_t rxData[4];

// ejercicio diapos
uint32_t tiempo_computo_tarea = 5; // en ms
uint32_t periodo_tarea = 3000; // en ms


void CANIntHandler(void) {
    uint32_t status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    if (status == 0 || status == 0xFFFFFFFF) {
        CANIntClear(CAN0_BASE, status);
        return;
    }
    if (status == 1) {
        rxAckMsg.pui8MsgData = rxData;
        CANMessageGet(CAN0_BASE, 1, &rxAckMsg, true);
        if (rxAckMsg.ui32MsgID == 0x11) {
            g_bSyncReceived = true;
            UARTprintf("ACK recibido.\n");
        }
        CANMessageSet(CAN0_BASE, 1, &rxAckMsg, MSG_OBJ_TYPE_RX);
    }

    else if (status == CAN_INT_INTID_STATUS) {
        uint32_t error = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        //UARTprintf("CAN error state: 0x08X\n", error);
    }

    CANIntClear(CAN0_BASE, status);
}

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

    // interrupciones
    CANIntRegister(CAN0_BASE, CANIntHandler);
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    IntEnable(INT_CAN0);
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

void SetupRxAck(void) {
    rxAckMsg.ui32MsgID = 0x11;
    rxAckMsg.ui32MsgIDMask = 0x7FF;
    rxAckMsg.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    rxAckMsg.ui32MsgLen = 4;
    rxAckMsg.pui8MsgData = rxData;
    CANMessageSet(CAN0_BASE, 1, &rxAckMsg, MSG_OBJ_TYPE_RX);
}

void EjecutarTarea1(uint32_t tiempo_ms) {
    UARTprintf("Tarea a1 se ejecuta durante su tiempo de computo %u ms\n", tiempo_ms);
    // enviar mensaje a tiva 2
    uint8_t evento_e1234 = 0xE1;
    tCANMsgObject msg_e1234;

    msg_e1234.ui32MsgID = 0x12;
    msg_e1234.ui32Flags = 0;
    msg_e1234.ui32MsgLen = 1;
    msg_e1234.pui8MsgData = &evento_e1234;

    CANMessageSet(CAN0_BASE, 2, &msg_e1234, MSG_OBJ_TYPE_TX);


    SysCtlDelay((SysCtlClockGet() / 3000) * tiempo_ms);
}

int main(void) {

    // para configurar el reloj
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    InitCAN();

    InitUART();

    SysCtlDelay(SysCtlClockGet() * 2);

    UARTprintf("Inicio de emisor\n");

    // configuracion mensaje SYNC
    txSyncMsg.ui32MsgID = 0x10;
    txSyncMsg.ui32MsgIDMask = 0;
    txSyncMsg.ui32Flags = 0;
    txSyncMsg.ui32MsgLen = 4;
    txSyncMsg.pui8MsgData = (uint8_t*)&txData;

    SetupRxAck();

    while (!g_bSyncReceived) {
        UARTprintf("Enviando SYNC...\n");
        CANMessageSet(CAN0_BASE, 2, &txSyncMsg, MSG_OBJ_TYPE_TX);
        SysCtlDelay(SysCtlClockGet() * 2);
    }
    UARTprintf("Sistema sincronizado \n");

    // mensaje 1 - ID 0x01
    /*txMessage_1.ui32MsgID = 0x01;
    txMessage_1.ui32MsgIDMask = 0;
    txMessage_1.ui32Flags = 0;
    txMessage_1.ui32MsgLen = 4;
    txMessage_1.pui8MsgData = pui8TxData_1;

    // mensaje 2 - ID 0x02
    txMessage_2.ui32MsgID = 0x02;
    txMessage_2.ui32MsgIDMask = 0;
    txMessage_2.ui32Flags = 0;
    txMessage_2.ui32MsgLen = 4;
    txMessage_2.pui8MsgData = pui8TxData_2;*/


    while(1) {
        EjecutarTarea1(tiempo_computo_tarea);
        SysCtlDelay((SysCtlClockGet() / 3000) * (periodo_tarea - tiempo_computo_tarea));
        /*UARTprintf("CAN ID 0x01 enviado: 0x%08X\n", txData_1);
        CANMessageSet(CAN0_BASE, 1, &txMessage_1, MSG_OBJ_TYPE_TX);
        //uint32_t status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        //UARTprintf("Estado CAN: 0x%08X\n", status);
        txData_1++;
        SysCtlDelay(SysCtlClockGet());
        //txMessage.ui32MsgID++;
        UARTprintf("CAN ID 0x02 enviado: 0x%08X\n", txData_2);
        CANMessageSet(CAN0_BASE, 2, &txMessage_2, MSG_OBJ_TYPE_TX);
        txData_2++;

        SysCtlDelay(SysCtlClockGet() * 5);*/
    }
}
