# comsniff

A windows command-line utility for monitoring and controlling serial port communication.

I have always used the excellent [ESPlorer](https://esp8266.ru/esplorer/) to debug the ESP8266 over serial port, but I often wished I had something simpler at hand.  A console program I could invoke directly from the terminal, use as part of a test setup etc.  *comsniff* was created to scratch that itch.

## Usage

    comsniff -d com5 -b 115200

## Options

    -h              This help
    -q              Reduce debug/info messages (make quieter)
    -v              Increase debug/info messages (make more verbose)
    -d <port>       Specify port number
    -b <baud_rate>  Specify baud rate

## Operation

Once opened, you may type and send text over the serial port using the keyboard.  History is supported via up and down arrow keys, but no in-line editing is supported.
You cannot use left and right arrow keys, backspace, delete etc.

The following hotkeys are also supported:

    F2  - Toggle RTS (this is often tied to the reset pin of the uC)
    F4  - Quit (you can also use Ctrl-C)


## Credits

Copyright (c) 2017, [Klar Systems Private Limited.](http://www.zmote.io)

Bugs, feature requests? [Tell me!](mailto:harik@zmote.io)

Adapted from code for written for [ComPrinter](https://batchloaf.wordpress.com/comprinter/) by [Ted Burke](https://batchloaf.wordpress.com/about/)

