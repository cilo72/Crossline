/*
  Copyright (c) 2024 Daniel Zwirner
  SPDX-License-Identifier: MIT-0
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "crossline.h"
#include <string.h>

int crossline_getch_timeout (uint32_t timeout_ms)
{
	char ch = 0;
	ch =  getchar_timeout_us (timeout_ms * 1000);
	if(ch == PICO_ERROR_TIMEOUT)
	{
		return 0;
	}
	else
	{
		return ch;
	}
}

int crossline_getch ()
{
	return getchar();
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

int main()
{
  stdio_init_all();

    static char buf[20];
    
    crossline_completion_register (completion_hook);

    while (NULL != crossline_readline ("> ", buf, sizeof(buf))) {
        printf ("Read line: \"%s\"\n", buf);
    } 
}
