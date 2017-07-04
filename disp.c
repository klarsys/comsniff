#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "disp.h"

#define PROMPT ">> "

static char rx_ln[16*1024], tx_ln[16*1024];
static int nrx = 0, ntx = 0;
static int echo = 1;
static HANDLE mutex;
typedef struct hist_ {
    char *cmd;
    struct hist_ *prev;
    struct hist_ *next;
} hist;
static hist *head = NULL, *last = NULL, *cur = NULL;

static void blank(void)
{
    int i;
    putchar('\r');
    for (i = 0; i < ntx; i++)
        putchar(' ');
    putchar('\r');
}
#define CRITICAL_IN do {                    \
    WaitForSingleObject(mutex, INFINITE);   \
} while (0)
#define CRITICAL_OUT do {                   \
    ReleaseMutex(mutex);                    \
    fflush(stdout);                         \
} while (0)


void disp_rx(char c)
{
    CRITICAL_IN;
    rx_ln[nrx++] = c;
    if (c == '\n') {
        rx_ln[nrx++] = 0; // nul terminate
        blank();
        printf("%s%s", rx_ln, tx_ln);
        nrx = 0;
    }
    CRITICAL_OUT;
}
static void add_hist(char *ln)
{
    if (last && !strcmp(last->cmd, ln))
        return; // Don't add dup entry
    hist *p = malloc(sizeof(hist));
    p->cmd = strdup(ln);
    p->prev = last;
    if (last)
        last->next = p;
    p->next = NULL;
    if (!head) 
        head = p;
    if (!last)
        last = p;
    last = p;
    cur = NULL;
}
void disp_tx_hist(int dir)
{
    if (!last)
        return;
    if (dir < 0 && !cur)
        cur = last;
    else if (dir < 0 && cur->prev)
        cur = cur->prev;
    else if (dir > 0 && cur->next)
        cur = cur->next;
    else
        return;
    CRITICAL_IN;
    ntx = sprintf(tx_ln, PROMPT "%s", cur->cmd);
    blank();
    printf("%s", tx_ln);
    CRITICAL_OUT;
}
void disp_tx(char c)
{
    CRITICAL_IN;
    tx_ln[ntx++] = c;
    tx_ln[ntx] = 0; // nul terminate
    
    if (c == '\r') {
        blank();
        if (echo)
            printf("%s\n", tx_ln+1);
        add_hist(tx_ln + strlen(PROMPT));
        strcpy(tx_ln, PROMPT);
        ntx = strlen(tx_ln);
    }
    printf("\r%s", tx_ln);
    CRITICAL_OUT;
}

void disp_init(int ec)
{
    mutex = CreateMutex(NULL, FALSE, NULL);
    echo = ec;
    strcpy(tx_ln, PROMPT);
    ntx = strlen(tx_ln);
    printf("%s", tx_ln);
    nrx = 0;
}
