/*
 * uart.c
 *
 *  Created on: Jun 5, 2017
 *      Author: Han-Chung Tseng
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "uart.h"
//========================================================================
#define UART_DEV        "/dev/ttyAMA0"
//------------------------------------------------------------------------
int g_UartFd = -1;
//========================================================================
_UartErr UART_Open(char *dev)
{
    int ret = UART_ERROR;

    ret = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if(ret < 0)
    {
        printf("UART: Open uart failed\n");
        return UART_OPEN_ERR;
    }



    return ret;
}
