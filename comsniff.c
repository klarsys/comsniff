//
// ComPrinter.c - This program reads text from a
//   serial port and prints it to the console.
//
// Written by Ted Burke - last updated 3-5-2012
//
// To compile with MinGW:
//
//      gcc -o ComPrinter.exe ComPrinter.c
//
// Example commands:
//
//      ComPrinter /devnum 22 /baudrate 38400
//      ComPrinter /id 12 /quiet
//      ComPrinter /baudrate 38400 /keystrokes
//
// To stop the program, press Ctrl-C
//
 
#define WINVER 0x0500
 
#include <windows.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "disp.h"


enum {
    CTRL_C = 3, // Ctrl+c
    TOGGLE_RTS = 60, // Maps to F2
    EXIT = 62, // Maps to F4
    UP = 72, // Up arrow
    DOWN = 80 // Down arrow (both are after 0x1e)
};

// Declare variables and structures
HANDLE hSerial = INVALID_HANDLE_VALUE;
DCB dcbSerialParams = {0};
COMMTIMEOUTS timeouts = {0};
DWORD dwBytesWritten = 0;
char dev_name[MAX_PATH] = "";
int dev_number = -1;
int baudrate = 74880;
int scan_max = 30;
int scan_min = 1;
int debug = 1; // print some info by default
int rts = 0;

HANDLE  hThread[2]; 
DWORD threadId[2];

#define DEBUG(fmt, ...) if (debug > 1) fprintf(stderr, "DEBUG[%d]: " fmt "\n", __LINE__, ##__VA_ARGS__)
#define INFO(fmt, ...) if (debug > 0) fprintf(stderr, "INFO[%d]: " fmt "\n", __LINE__, ##__VA_ARGS__)
#define WARN(fmt, ...) if (debug > -1) fprintf(stderr, "Warning: " fmt "\n", ##__VA_ARGS__)
#define PERROR(fmt, ...) do { \
    fprintf(stderr, "Error: " fmt "\n", ##__VA_ARGS__); \
    exit(2); \
} while (0)


void CloseSerialPort()
{
    if (hSerial != INVALID_HANDLE_VALUE) {
        // Close serial port
        INFO("Closing serial port.");
        if (CloseHandle(hSerial) == 0)
            PERROR("Unable to close serial port");
        hSerial = INVALID_HANDLE_VALUE;
    }
    exit(0);
}
 

static void help(void) 
{
    printf(
        "comsniff: \n"
        "    A windows command-line utility for monitoring and controlling serial port communication\n"
        "Usage:\n"
        "    comsniff -d com5 -p 115200\n"
        "Options:\n"
        "    -h              This help\n"
        "    -q              Reduce debug/info messages (make quieter)\n"
        "    -v              Increase debug/info messages (make more verbose)\n"
        "    -d <port>       Specify port number\n"
        "    -b <baud_rate>  Specify baud rate\n"
        "    -p <pspec>      Specify a full port specification\n"
        "                    (e.g. 115200N81 ==> 115kbaud, No parity, 7 data bits, 1 stop bit)\n"
        "Operation:\n"
        "    Once opened, you may send text using the keyboard.  The following hotkeys are also supported:\n"
        "\n"
        "    F2  - Toggle RTS\n"
        "    F4  - Quit (you can also use Ctrl-C)\n"
        "\n"
        "History is supported via up and down arrow keys, but no editing is supported.\n"
        "You cannot use left and right arrow keys, backspace, delete etc.\n"
        "\n"
        "Credits:\n"
        "    Copyright (c) 2017, Klar Systems Private Limited.\n"
        "    Based on code from Ted Burke (https://batchloaf.wordpress.com/comprinter/)\n");
    exit(0);
}
static void parse_portspec(char *pspec) 
{
    baudrate = atoi(pspec); // for now FIXME
    DEBUG("Baud rate = %d", baudrate);
}

static void reader(HANDLE hSerial)
{
    char c;
    DWORD bytes_read;
 
    while(1) {
        ReadFile(hSerial, &c, 1, &bytes_read, NULL);
        if (bytes_read == 1)
            disp_rx(c);
    }

}
static void writer(HANDLE hSerial)
{
    int c[2], state = 0;
    while(1) {
        c[state] = getch();
            DEBUG("Got code %d", c[state]);
        if (state) {
            DEBUG("Got hotkey %d", c[state]);
            switch (c[state]) {
            case TOGGLE_RTS:
                DEBUG("Toggle RTS");
                //Status = 
                EscapeCommFunction(hSerial, (rts?CLRRTS:SETRTS));
                rts = 1 - rts;
                break;
            case EXIT:
                DEBUG("exit");
                CloseSerialPort();
                break;
            case UP:
                disp_tx_hist(-1);
                break;
            case DOWN:
                disp_tx_hist(1);
                break;
            default:
                WARN("Unhandled hotkey 0,%d", c[state]);
            }
            --state;
        } else if (0 == c[state] || 0xe0 == c[state]) {
            ++state;
        } else if (isprint(c[state]) || c[state] == 13) {
            WriteFile(hSerial, &c[state], 1, NULL, NULL);
            disp_tx(c[state]);
        } else if (c[state] == CTRL_C) {
            DEBUG("interrupt");
            CloseSerialPort();            
        }
    }
}
int main(int argc, char *argv[])
{
    int opt, n;

    while ((opt = getopt(argc, argv, "d:b:p:vqh")) != -1) {
        switch (opt) {
        case 'd':
            dev_number = atoi(optarg);
            DEBUG("Got device COM%d", dev_number);
        case 'b':
        case 'p':
            parse_portspec(optarg);
            break;
        case 'v':
            ++debug;
            break;
        case 'q':
            --debug;
            break;
        case 'h':
        default:
            help();
            break;
        }
    }
    DEBUG("com%d naud %d", dev_number, baudrate);

    // Register function to close serial port at exit time
    atexit(CloseSerialPort);
 
    if (dev_number != -1)
    {
        // Limit scan range to specified COM port number
        scan_max = dev_number;
        scan_min = dev_number;
    }
 
    // Scan for an available COM port in _descending_ order
    for (n = scan_max ; n >= scan_min ; --n)
    {
        // Try next device
        sprintf(dev_name, "\\\\.\\COM%d", n);
        DEBUG("Trying %s...", dev_name);
        hSerial = CreateFile(dev_name, GENERIC_READ|GENERIC_WRITE, 0, 0,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial!=INVALID_HANDLE_VALUE)
        {
            INFO("Detected device in %s", dev_name);
            dev_number = n;
            break; // stop scanning COM ports
        }
    }
 
    // Check in case no serial port could be opened
    if (hSerial==INVALID_HANDLE_VALUE)
        PERROR("Could not open serial port");
 
    // If we get this far, a serial port was successfully opened
    DEBUG("Opened COM%d at %d baud\n\n", dev_number, baudrate);
 
    // Set device parameters:
    //  baudrate (default 2400), 1 start bit, 1 stop bit, no parity
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hSerial, &dcbSerialParams) == 0)
        PERROR("Unable to get device state");
    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if(SetCommState(hSerial, &dcbSerialParams) == 0)
        PERROR("Could not set device parameters");
 
    // Set COM port timeout settings
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &timeouts) == 0)
        PERROR("Unable to set timeouts");
    disp_init(1);
    if (!(hThread[0] = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            reader,       // thread function name
            hSerial,          // argument to thread function 
            0,                      // use default creation flags 
            &threadId[0])))   // returns the thread identifier 
        PERROR("Unable to creater reader thread");
    if (!(hThread[1] = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            writer,       // thread function name
            hSerial,          // argument to thread function 
            0,                      // use default creation flags 
            &threadId[1])))   // returns the thread identifier 
        PERROR("Unable to creater writer thread");

    WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

 
    // We should never get to this point because when the user
    // presses Ctrl-C, the atexit function will be called instead.
    return 0;
}
