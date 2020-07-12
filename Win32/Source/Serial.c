/*****************************************************************************
 * FILE: Serial.c                                                            *
 * DESC: Communications module for ChargeOn (Laptop Battery Conditioner)     *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

  /* Includes */
#include "ChargeOn.h"
#include <stdlib.h>                    // For atol()
  /* Defines */

  /* Typedefs */

  /* Static variables */
static const char WAKE_SIGNAL[]           = "<CO_WAKE>";
static const char WAKE_OK_SIGNAL[]        = "<CO_WAKE_OK>";
static const char WAKE_ERROR[]            = "WAKE";

static const char ON_SIGNAL[]             = "<CO_ON>";
static const char ON_OK_SIGNAL[]          = "<CO_ON_OK>";
static const char TURN_ON_ERROR[]         = "OUTLET ON";

static const char OFF_SIGNAL[]            = "<CO_OFF>";
static const char OFF_OK_SIGNAL[]         = "<CO_OFF_OK>";
static const char TURN_OFF_ERROR[]        = "OUTLET OFF";

static const char HEARTBEAT_SIGNAL[]      = "<CO_BEAT>";
static const char HEARTBEAT_OK_SIGNAL[]   = "<CO_BEAT_OK>";
static const char HEARTBEAT_ERROR[]       = "HEARTBEAT";

static const char SETTINGS_SIGNAL[]       = "<CO_SETTINGS>";
static const char SETTINGS_OK_SIGNAL[]    = "<CO_SETTINGS_OK>";
static const char SETTINGS_ERROR[]        = "SETTINGS";

static const char OUTLET_SIGNAL[]         = "<CO_OUTLET>";
static const char OUTLET_OK_SIGNAL[]      = "<CO_OUTLET_OK>";
static const char OUTLET_ERROR[]          = "OUTLET";

// ---- Insert "special" signal entries after this point

static const char LEARN_SIGNAL[]          = "<CO_LEARN>";
static const char LEARN_OK_SIGNAL[]       = "<CO_LEARN_OK>";
//static const char LEARN_ERROR[]           = "LEARN CODE";

static const char VERSION_SIGNAL[]        = "<CO_VERSION>";
static const char VERSION_OK_SIGNAL[]     = "<CO_VERSION_OK>";
//static const char VERSION_ERROR[]         = "VERSION";

static const char EEPROM_SIGNAL[]         = "<CO_EEPROM>";
static const char EEPROM_OK_SIGNAL[]      = "<CO_EEPROM_OK>";
//static const char EEPROM_ERROR[]          = "EEPROM";

static       char szSettingsBuffer[85];

static const int  CAPTURECODE_TIMEOUTSECS = 3;


  /* Global variables */
BOOL bInitializingPort = FALSE;                            // Flag to prevent certain processes while serial port is being initialized
BOOL bDoingTX_RX       = FALSE;                            // In the process of communicating with the serial port?


  /* Function prototypes */
static BOOL SetPortState( HANDLE hSerial );

/* === LOCAL FUNCTIONS ===================================================== */

/*****************************************************************************
 * FUNC: InitSerial                                                          *
 * DESC: Loop through COM ports until one is found that:                     *
 *         1) Can be opened and configured successfully                      *
 *         2) Sends the appropriate reply in response to our "wake up" signal*
 * ARGS: pSerialPort = Address of port info to be populated                  *
 * RET:  TRUE  = Port was opened and configured correctly                    *
 *       FALSE = Failed to open/configure port                               *
 * NOTE: [None]                                                              *
 *****************************************************************************/
BOOL InitSerial( PORTINFO *pSerialPort )
{
  UINT       uPortNum;
  NAMESTRING pTempPortName;
  BOOL       bStatus        = FALSE;

  bInitializingPort = TRUE;                                // Prevent certain processes while serial port is being initialized

  for( uPortNum = 1; uPortNum < MAX_PORT_NUM; uPortNum++ ) {
                                                           // Until all possible COM ports have been checked...
    sprintf( pTempPortName, TEXT("\\\\.\\COM%u"), uPortNum );
                                                           // Populate next candidate device string
    strcpy( pSerialPort->szPortName, pTempPortName+4 );    // Remember port NAME portion of the string
    pSerialPort->hComPort = CreateFile(                    // Try to open the specified port
                                        pTempPortName,                   // Port name
                                        GENERIC_READ | GENERIC_WRITE,    // Open for read/write
                                        0,                               // No sharing (ports can't be shared)
                                        NULL,                            // No security
                                        OPEN_EXISTING,                   // Open existing port only
                                        FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
                                                                         // Non-overlapped, non-buffered I/O
                                        NULL                             // Template file not needed for comm devices
                                      );
    if( pSerialPort->hComPort == INVALID_HANDLE_VALUE ) {  // Returned handle is invalid?
      continue;                                            //  Yes, try the next COM port
    }
    else {                                                 //  No...
Sleep(1500);
      if( !SetPortState(pSerialPort->hComPort) ) {         //   Able to configure the desired settings for the port?
                                                           //    No, print error message
        CloseHandle( pSerialPort->hComPort );              //     Close the handle
        continue;                                          //     Try the next COM port
      }
      else if( !SendSignal_GetResponse(pSerialPort, WAKE) ) {
                                                           //    Yes, able to "wake up" the COM port?
        CloseHandle( pSerialPort->hComPort );              //     No, close the handle
        continue;                                          //      Try the next COM port
      }
      else {                                               //     Yes (found an available & suitable ChargeOn module!)
        bStatus = TRUE;                                    //      Set status flag to indicate Success
        SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "" );
        SendOutletSettings( pSerialPort );                 //      Make sure ChargeOn module has current outlet settings
        break;
      }
    }
  } /* for */

  bInitializingPort = FALSE;                               // Allow "blocked" processes

  return bStatus;                                          // Return the final status
} // InitSerial()


/*************************************************************************************
 * FUNC: SetPortState                                                                *
 * DESC: Set up DCB (Device Control Block) and timeouts for current port of interest *
 * ARGS: hSerial = Handle for port to be configured                                  *
 * RET:  TRUE  = Port was successfully configured                                    *
 *       FALSE = Failed to configure port                                            *
 *************************************************************************************/

static BOOL SetPortState( HANDLE hSerial )
{
  DCB dcbSerialParams = { 0 };                             // Initialize DCB structure

  dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
  if( !GetCommState(hSerial, &dcbSerialParams) ) {         // Able to retrieve the current DCB settings?
    return FALSE;                                          //  No, FAIL
  }
  else {                                                   //  Yes...
    dcbSerialParams.BaudRate = MY_BAUDRATE;                //   Set DCB values that we care about
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if( !SetCommState(hSerial, &dcbSerialParams) ) {       //   Able to configure the port according to settings in DCB?
      return FALSE;                                        //    No, FAIL
    }
    else {                                                 //    Yes...
/*
      switch( dcbSerialParams.StopBits ) {                 //     Print configured settings
        case ONESTOPBIT:   printf( "1\n" );
                           break;
        case ONE5STOPBITS: printf( "1.5\n" );
                           break;
        case TWOSTOPBITS:  printf( "2\n" );
                           break;
      }
*/        
      COMMTIMEOUTS timeouts = { 0 };                       //     Assign desired timeout settings
      timeouts.ReadIntervalTimeout         = MAXDWORD;
      timeouts.ReadTotalTimeoutConstant    = 0;
      timeouts.ReadTotalTimeoutMultiplier  = 0;
      timeouts.WriteTotalTimeoutConstant   = 0;
      timeouts.WriteTotalTimeoutMultiplier = 0;

      if( !SetCommTimeouts(hSerial, &timeouts) ) {         //     Able to configure the port with the desired timeout settings?
        return FALSE;                                      //      No, FAIL
      }
    }
  }

  return TRUE;                                             // Configured the port successfully!
} // SetPortState()


/*************************************************************************************
 * FUNC: SendSignal_GetResponse                                                      *
 * DESC: Send specific signal to microcontroller, expect appropriate response        *
 * ARGS: pSerial  = Address of PORTINFO struct for serial connection                 *
 *       talkType = WAKE to establish initial contact with microcontroller           *
 *                = TURN_ON   to send signal for MC to turn outlet ON                *
 *                = TURN_OFF  to send signal for MC to turn outlet OFF               *
 *                = HEARTBEAT to check on the "health" of the connection with the MC *
 *                = SETTINGS  to transmit (updated?) settings for the outlet         *
 *                = OUTLET    to have ChargeOn module print current outlet settings  *
 * RET:  TRUE  = Successfully wrote to port and received expected response           *
 *       FALSE = Error while writing/reading, or received unexpected response        *
 *************************************************************************************/

BOOL SendSignal_GetResponse( PORTINFO *pSerial, SerialExchangeType talkType )
{
  const TalkParams chat[]  = { {WAKE_SIGNAL,      WAKE_OK_SIGNAL,      WAKE_ERROR},
                               {ON_SIGNAL,        ON_OK_SIGNAL,        TURN_ON_ERROR},
                               {OFF_SIGNAL,       OFF_OK_SIGNAL,       TURN_OFF_ERROR},
                               {HEARTBEAT_SIGNAL, HEARTBEAT_OK_SIGNAL, HEARTBEAT_ERROR},
                               {SETTINGS_SIGNAL,  SETTINGS_OK_SIGNAL,  SETTINGS_ERROR},
                               {OUTLET_SIGNAL,    OUTLET_OK_SIGNAL,    OUTLET_ERROR}
                             };
  char  *OutBuffer         = (char *)chat[talkType].signal;// OutBuffer should be char or byte array, otherwise write will fail

  if( talkType == SETTINGS ) {                             // SETTINGS signal requires additional data
    sprintf( szSettingsBuffer, "%s[On:%d][Off:%d][Pro:%d][PLen:%d][PReps:%d][TOBQ:%d][VLen:%d][]",
                               (char *)chat[SETTINGS].signal,
                               Outlet.OnCode,
                               Outlet.OffCode,
                               Outlet.Protocol,
                               Outlet.PulseLength,
                               Outlet.PulseRepeats,
                               Outlet.TurnOnBeforeQuit,
                               Outlet.ValueLength );
    OutBuffer = szSettingsBuffer;
  }

  DWORD dwNoOfBytesToWrite = strlen(OutBuffer);            // Number of bytes to write to the port
  DWORD dwNoOfBytesWritten = 0;                            // Number of bytes actually written to the port
  DWORD dwSleepPeriod      = ((dwNoOfBytesToWrite * CBR_115200) / MY_BAUDRATE);
                                                           // Number of millseconds to wait (after sending the signal)
                                                           //  before attempting to read the response
  char  InBuffer[25];                                      // Store response from ChargeOn module (Arduino) here
  DWORD dwNoOfBytesToRead  = strlen( chat[talkType].expectedResponse );
                                                           // Number of bytes to read from the port
  DWORD dwNoOfBytesRead    = 0;                            // Number of bytes actually read from the port
  char  szMessageBuff[70]  = "";                           // Create message string for user (if any) here
  BOOL  bRetVal            = FALSE;                        // Assume failure until proven otherwise

  if( bDoingTX_RX == TRUE ) {                              // Already communicating with serial port? 
    Sleep( 50 );                                           //  Yes, wait before trying again
    if( bDoingTX_RX == TRUE ) {                            //   Still communicating with serial port?
      return FALSE;                                        //    Yes, fail
    }
  }
  bDoingTX_RX = TRUE;                                      // Starting a new "conversation" with serial port

  if( !WriteFile(pSerial->hComPort,                        // Able to write signal to serial port?
                 OutBuffer,
                 dwNoOfBytesToWrite,
                 &dwNoOfBytesWritten,
                 NULL)
    ) {
    if( !bMonitorOnly ) {
      sprintf( szMessageBuff, "** ERROR writing %s signal **\n", chat[talkType].errorMessage );
                                                           //  No, print error message
      SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), szMessageBuff );
    }
    bRetVal = FALSE;                                       //  and FAIL
  }
  else {                                                   //  Yes...
    Sleep( dwSleepPeriod );                                //   Allow plenty of time for microcontroller to send response
    if( !ReadFile(pSerial->hComPort,                       //   Able to read response from serial port?
                  InBuffer,
                  dwNoOfBytesToRead,
                  &dwNoOfBytesRead,
                  NULL)
      ) {
      if( !bMonitorOnly ) {
        sprintf( szMessageBuff, "** ERROR reading %s signal response **\n", chat[talkType].errorMessage );
                                                           //    No, print error message
        SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), szMessageBuff );
      }
      bRetVal = FALSE;                                     //    and FAIL
    }
    else if( strncmp(InBuffer, chat[talkType].expectedResponse, dwNoOfBytesToRead) ) {
                                                           //    Yes, did we get the *expected* response?
      if( !bMonitorOnly ) {
        sprintf( szMessageBuff, "Unexpected response \"%s\" to %s signal", InBuffer, chat[talkType].errorMessage );
                                                           //     No, print error message
        SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), szMessageBuff );
      }
      bRetVal = FALSE;                                     //     and FAIL
    }
    else {                                                 //     Yes (we got the *expected* response)
      if( talkType == TURN_ON ) {                          //      Are we switching the outlet ON?
        Sleep(1250);                                       //       Yes, allow time for microcontroller (and outlet)
                                                           //        to complete the process
// KJB (13 Jun 2020): Try reducing Sleep() value to 500; it's better to send the ON code to the outlet multiple times
//                    (if necessary) than to make the user wait more than a second no matter what
      }
      else if( talkType == TURN_OFF ) {                    //       No, are we switching the outlet OFF?
        Sleep(250);                                        //        Yes, allow time for microcontroller (and outlet)
                                                           //         to complete the process
      }
      bRetVal = TRUE;                                      //      Success!
    }
  }

  bDoingTX_RX = FALSE;                                     // No longer communicating with serial port
  return bRetVal;
} // SendSignal_GetResponse()


/*************************************************************************************
 * FUNC: Serial_GetOutletInfo                                                        *
 * DESC: Send EEPROM or LEARN signal to microcontroller, expect response with data   *
 *       for remote outlet's settings (as read from ChargeOn module's EEPROM or      *
 *       generated by outlet's remote control, respectively)                         *
 * ARGS: hParentWnd  = Handle of MessageBox's parent window                          *
 *       pSerial     = Address of PORTINFO struct for serial connection              *
 *       requestType = EEPROM or LEARN                                               *
 *       pOut        = Address of Outlet structure to be populated                   *
 * RET:  TRUE  = Successfully wrote to port and received good response               *
 *       FALSE = Error while writing/reading, or received unexpected response        *
 *************************************************************************************/

BOOL Serial_GetOutletInfo( HWND hParentWnd, PORTINFO *pSerial, SerialExchangeType requestType, void *pOut )
{
  OUTLET *pOutlet           = (OUTLET *)pOut;
  char   *OutBuffer;                                       // OutBuffer should be char or byte array, otherwise write will fail
  char   *OKsignal;
  DWORD  dwNoOfBytesToWrite;                               // Number of bytes to write to the port
  DWORD  dwNoOfBytesWritten = 0;                           // Number of bytes actually written to the port
  DWORD  dwSleepPeriod;                                    // Number of milliseconds to wait for a response

  char   InBuffer[85];                                     // Store response from ChargeOn module (Arduino) here
  DWORD  dwNoOfBytesRead    = 0;                           // Number of bytes actually read from the port
  BOOL   bRetVal            = FALSE;                       // Assume failure until proven otherwise

  if( bDoingTX_RX == TRUE ) {                              // Already communicating with serial port? 
    Sleep( 50 );                                           //  Yes, wait before trying again
    if( bDoingTX_RX == TRUE ) {                            //   Still communicating with serial port?
      MessageBox( hParentWnd, "Serial port seems to be busy. Please try again later.", "Serial Port Busy", MB_OK );
      return FALSE;                                        //    Yes, FAIL
    }
  }
  bDoingTX_RX = TRUE;                                      // Starting a new "conversation" with serial port

  switch( requestType ) {
    case EEPROM:
      OutBuffer          = (char *)EEPROM_SIGNAL;
      OKsignal           = (char *)EEPROM_OK_SIGNAL;
      dwNoOfBytesToWrite = strlen(OutBuffer);
      dwSleepPeriod      = ((dwNoOfBytesToWrite * CBR_115200 * 2) / MY_BAUDRATE);
      break;

    case LEARN:
      OutBuffer          = (char *)LEARN_SIGNAL;
      OKsignal           = (char *)LEARN_OK_SIGNAL;
      dwNoOfBytesToWrite = strlen(OutBuffer);
      dwSleepPeriod      = (CAPTURECODE_TIMEOUTSECS * 1000) + 200;
      break;
  }

  if( WriteFile(pSerial->hComPort,                         // Able to write signal to serial port?
                 OutBuffer,
                 dwNoOfBytesToWrite,
                 &dwNoOfBytesWritten,
                 NULL) ) {
    Sleep( dwSleepPeriod );                                //  Yes, allow Arduino time to capture data and send response 
    if( ReadFile(pSerial->hComPort,                        //   Able to read response from serial port?
                 InBuffer,
                 sizeof(InBuffer),
                 &dwNoOfBytesRead,
                 NULL) ) {
      if( !strncmp(InBuffer, OKsignal, strlen(OKsignal)) ) {
                                                           //    Yes, did we get the *expected* response?
        char *nameToken;
        char *valToken;

        nameToken = strtok( InBuffer + strlen(OKsignal), "[:" );
                                                           //     Yes, parse the remaining names and values 
        while( nameToken ) {
          valToken = strtok( NULL, "]" );

          if( !strcmp(nameToken, "Code") ) {               //      (Only expected for LEARN)
            pOutlet->OnCode = pOutlet->OffCode = atol( valToken );
          }
          else if( !strcmp(nameToken, "On") ) {            //      (Only expected for EEPROM)
            pOutlet->OnCode = atol( valToken );
          }
          else if( !strcmp(nameToken, "Off") ) {           //      (Only expected for EEPROM)
            pOutlet->OffCode = atol( valToken );
          }
          else if( !strcmp(nameToken, "Pro") ) {
            pOutlet->Protocol = atol( valToken );
          }
          else if( !strcmp(nameToken, "PLen") ) {
            pOutlet->PulseLength = atol( valToken );
          }
          else if( !strcmp(nameToken, "PReps") ) {         //      (Only expected for EEPROM)
            pOutlet->PulseRepeats = atol( valToken );
          }
          else if( !strcmp(nameToken, "TOBQ") ) {          //      (Only expected for EEPROM)
            pOutlet->TurnOnBeforeQuit = atol( valToken );
          }
          else if( !strcmp(nameToken, "VLen") ) {
            pOutlet->ValueLength = atol( valToken );
          }

          nameToken = strtok( NULL, "[:" );
        }

        if(    (requestType == EEPROM)                     //      Did we read from the ChargeOn module's EEPROM
            || (    (requestType == LEARN)                 //        OR
                 && pOutlet->OnCode                        //      Did we try to capture a code from a outlet's
                 && pOutlet->OffCode                       //        remote control and *succeed*?
                 && pOutlet->PulseLength                   //        (Don't assume that Protocol can't be 0;
                 && pOutlet->ValueLength                   //         ignore values for "PulseRepeats" and "TurnOnBeforeQuit")
               )
          ) {
          bRetVal = TRUE;                                  //       Yes, success!
        }
      }
    }
  }

  bDoingTX_RX = FALSE;                                     // No longer communicating with serial port
  return bRetVal;
} // Serial_GetOutletInfo()


/*************************************************************************************
 * FUNC: GetArduinoSketchVersion                                                     *
 * DESC: Send VERSION signal to microcontroller, expect response with Arduino sketch *
 *       version information                                                         *
 * ARGS: szArduinoSketchVersion = Buffer to receive Arduino sketch version info      *
 * RET:  TRUE  = Successfully wrote to port and received good response               *
 *       FALSE = Error while writing/reading, or received unexpected response        *
 *************************************************************************************/

BOOL GetArduinoSketchVersion( HWND hParentWnd, PORTINFO *pSerial, char *szArduinoSketchVersion )
{
  const char *OutBuffer    = VERSION_SIGNAL;               // OutBuffer should be char or byte array, otherwise write will fail

  DWORD dwNoOfBytesToWrite = strlen(OutBuffer);            // Number of bytes to write to the port
  DWORD dwNoOfBytesWritten = 0;                            // Number of bytes actually written to the port
  DWORD dwSleepPeriod      = ((dwNoOfBytesToWrite * CBR_115200) / MY_BAUDRATE);

  char  InBuffer[75];                                      // Input buffer
//  DWORD dwNoOfBytesToRead;                                 // Number of bytes to read from the port
  DWORD dwNoOfBytesRead    = 0;                            // Number of bytes actually read from the port
  BOOL  bRetVal            = FALSE;

  if( bDoingTX_RX == TRUE ) {                              // Already communicating with serial port? 
    Sleep( 50 );                                           //  Yes, wait before trying again
    if( bDoingTX_RX == TRUE ) {                            //   Still communicating with serial port?
      MessageBox( hParentWnd, "Serial port seems to be busy. Please try again later.", "Serial Port Busy", MB_OK );
      return FALSE;                                        //    Yes, fail
    }
  }
  bDoingTX_RX = TRUE;                                      // Starting a new "conversation" with serial port

  if( WriteFile(pSerial->hComPort,                         // Able to write signal to serial port?
                 OutBuffer,
                 dwNoOfBytesToWrite,
                 &dwNoOfBytesWritten,
                 NULL) ) {
    Sleep( dwSleepPeriod );                                //  Yes, allow Arduino time to capture data and send response 
    if( ReadFile(pSerial->hComPort,                        //   Able to read response from serial port?
                 InBuffer,
                 sizeof(InBuffer),
                 &dwNoOfBytesRead,
                 NULL) ) {
      if( !strncmp(InBuffer, VERSION_OK_SIGNAL, strlen(VERSION_OK_SIGNAL)) ) {
                                                           //    Yes, did we get the *expected* response?
        char *nameToken;
        char *valToken;

        nameToken = strtok( InBuffer + strlen(VERSION_OK_SIGNAL), "[:" );
                                                           //     Yes, parse the remaining names and values 
        while( nameToken ) {
          valToken = strtok( NULL, "]" );

          if( !strcmp(nameToken, "Build") ) {
            strcpy( szArduinoSketchVersion, valToken );
            bRetVal = TRUE;
// KJB (05 May 2020): If support is added for additional fields, be sure to examine when/how bRetVal is set to TRUE
          }

          nameToken = strtok( NULL, "[:" );
        }

      }
    }
  }

  bDoingTX_RX = FALSE;                                     // No longer communicating with serial port
  return bRetVal;
} // GetArduinoSketchVersion()
