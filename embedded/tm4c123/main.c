#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/systick.h"
#include "crossline.h"

#include <rt_sys.h>
#include <rt_misc.h>
#include <stdio.h>
#include <string.h>

/* IO device file handles. */
#define FH_STDIN  0x8001
#define FH_STDOUT 0x8002
#define FH_STDERR 0x8003
// User defined ...


/* Standard IO device name defines. */
const char __stdin_name[]  = ":STDIN";
const char __stdout_name[] = ":STDOUT";
const char __stderr_name[] = ":STDERR";

extern void $Super$$_sys_open(void);
FILEHANDLE  $Sub$$_sys_open(const char* name, int openmode)
{
    if (name == NULL)
    {
        return (-1);
    }

    if (name[0] == ':')
    {
        if (strcmp(name, ":STDIN") == 0)
        {
            return (FH_STDIN);
        }
        if (strcmp(name, ":STDOUT") == 0)
        {
            return (FH_STDOUT);
        }
        if (strcmp(name, ":STDERR") == 0)
        {
            return (FH_STDERR);
        }
    }

    return -1;
}

extern void $Super$$_sys_close(void);
int         $Sub$$_sys_close(FILEHANDLE fh)
{
    return 0;
}

extern void $Super$$_sys_write(void);
int         $Sub$$_sys_write(FILEHANDLE fh, const unsigned char* buf, unsigned len, int mode)
{
    int ch;
    switch (fh)
    {
        case FH_STDIN:
            return (-1);

        case FH_STDOUT:
            for (; len; len--)
            {
              ch = *buf++;
							if(ch == '\n')
							{
								MAP_UARTCharPut(UART0_BASE, '\n');
								MAP_UARTCharPut(UART0_BASE, '\r');
							}
							else
							{
							  MAP_UARTCharPut(UART0_BASE, ch);
							}
            }
            return (0);

        case FH_STDERR:
            for (; len; len--)
            {
                ch = *buf++;
							  if(ch == '\n')
							  {
							  	MAP_UARTCharPut(UART0_BASE, '\n');
							  	MAP_UARTCharPut(UART0_BASE, '\r');
							  }
							  else
							  {
							    MAP_UARTCharPut(UART0_BASE, ch);
							  }
            }
            return (0);
    }
    return (-1);
}

extern void $Super$$_sys_read(void);
int         $Sub$$_sys_read(FILEHANDLE fh, unsigned char* buf, unsigned len, int mode)
{
    int ch;

    switch (fh)
    {
        case FH_STDIN:
            ch = MAP_UARTCharGet(UART0_BASE);
            if (ch < 0)
            {
                return ((int)(len | 0x80000000U));
            }
            *buf++ = (uint8_t)ch;
            len--;
            return ((int)(len));
        case FH_STDOUT:
            return (-1);
        case FH_STDERR:
            return (-1);
    }

    return (-1);
}

extern void $Super$$_ttywrch(void);
void        $Sub$$_ttywrch(int ch)
{    
	MAP_UARTCharPut(UART0_BASE, ch);
}

extern void $Super$$_sys_istty(FILEHANDLE fh);
int         $Sub$$_sys_istty(FILEHANDLE fh)
{
    return 1; /* buffered output */
}

uint32_t systick = 0;

void SysTickHandler(void)
{
	systick++;
}

int crossline_getch_timeout (uint32_t timeout_ms)
{
	int32_t ch          = 0;
	uint32_t start      = systick;
	uint32_t now        = systick;
	uint32_t elapsed    = now - start;
	
	while(elapsed < timeout_ms)
	{
		if(UARTCharsAvail(UART0_BASE))
		{
			ch = MAP_UARTCharGetNonBlocking(UART0_BASE);
			return ch;
		}
		
		elapsed = systick - start;
	}

	return 0;
}

int crossline_getch()
{
	int ch;
	ch = MAP_UARTCharGet(UART0_BASE);
  return ch;
}

static void completion_hook (char const *buf, crossline_completions_t *pCompletion)
{
    int i;
    static const char *cmd[] = {"insert", "select", "update", "delete", "create", "drop", "show", "describe", "help", "exit", "history", NULL};

    for (i = 0; NULL != cmd[i]; ++i) {
        if (0 == strncmp(buf, cmd[i], strlen(buf))) {
            crossline_completion_add (pCompletion, cmd[i], NULL);
        }
    }

}

int main(void)
{
    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    MAP_FPUEnable();
    MAP_FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    MAP_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);
	
    //
    // Enable the peripherals used by this example.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable processor interrupts.
    //
    MAP_IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    MAP_UARTConfigSetExpClk(UART0_BASE, MAP_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
		
		UARTFIFOEnable(UART0_BASE);


    MAP_SysTickPeriodSet(MAP_SysCtlClockGet() / 1000);
    MAP_SysTickEnable();
		MAP_SysTickIntEnable();

		static char buf[20];
		crossline_completion_register (completion_hook);
    while (NULL != crossline_readline ("> ", buf, sizeof(buf))) 
    {
        printf ("Read line: \"%s\"\n", buf);
    } 

		return 0;
}

