/*****************************************************************************
 * FILE: Serial.h                                                            *
 * DESC: Definitions for serial communications                               *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

#ifndef SERIAL_H
# define SERIAL_H                                // Prevent items below from being processed more than once

    /* Defines */
# define MY_BAUDRATE   CBR_115200                // Baud rate for connection to ChargeOn (Arduino) module
//# define MY_BAUDRATE   CBR_57600
//# define MY_BAUDRATE   CBR_38400
//# define MY_BAUDRATE   CBR_19200
//# define MY_BAUDRATE   CBR_14400
//# define MY_BAUDRATE   CBR_9600

# define MAX_PORT_NUM (256)                      // Highest COM port number we will check for an available ChargeOn module
# define MAX_NAME_LEN (256)                      // Generous size for name strings

    /* Typedefs */
  typedef TCHAR NAMESTRING[MAX_NAME_LEN];

  typedef struct {                               // Information about a COM (serial) port
    HANDLE     hComPort;                         // Handle for port (if it opened OK)
    NAMESTRING szPortName;                       // Port name
  } PORTINFO;

  typedef enum { WAKE,                           // 0
                 TURN_ON,                        // 1
                 TURN_OFF,                       // 2
                 HEARTBEAT,                      // 3
                 SETTINGS,                       // 4
                 SHOW_OUTLET,                    // 5
    // Insert new types - handled by SendSignal_GetResponse() - ABOVE this point
    // Insert "custom" types - with their own handler functions - AFTER this point
                 LEARN,                          // 6
                 VERSION,                        // 7
                 EEPROM,                         // 8
                 MAX_EXCHANGE_TYPE               // 9
               } SerialExchangeType;

  typedef struct { const char *signal;
                   const char *expectedResponse;
                   const char *errorMessage;
                 } TalkParams;

    /* Global variables */
extern BOOL bInitializingPort;                             // Flags to prevent certain processes while serial port is being initialized
extern BOOL bDoingTX_RX;                                   // In the process of communicating with the serial port?


    /* Global function prototypes */
  BOOL InitSerial(              PORTINFO           *phSerialPort );
  BOOL SendSignal_GetResponse(  PORTINFO           *phSerialPort,  SerialExchangeType talkType );
  BOOL Serial_GetOutletInfo(    HWND               hParentWnd,     PORTINFO           *pSerial,
                                SerialExchangeType requestType,    void               *pOutlet );
  BOOL GetArduinoSketchVersion( HWND               hParentWnd,     PORTINFO           *pSerial,
                                char               *szArduinoSketchVersion );


#endif
