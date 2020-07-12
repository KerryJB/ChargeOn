/*****************************************************************************
 * FILE: ChargeOn.ino                                                        *
 * DESC: Main module for ChargeOn sketch                                     *
 * AUTH: Kerry Burton                                                        *
 * INFO: The idea is to prevent "battery burnout" on a Windows laptop by     *
 *       by allowing it to alternately charge and discharge its battery      *
 *       within a user-chosen percentage range.                              *
 *       This sketch responds to "signals" from the corresponding Win32      *
 *       ChargeOn program and initiates the sending of ON/OFF signals to the *
 *       433MHz remote outlet that the laptop is plugged into.               *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

  /* Includes */
#include "Project.h"
#include "myRCSwitch.h"
#include "ChargeOn.h"
#include <EEPROM.h>

  /* Module-specific defines */
#if defined(PRJ_DEBUGGING)                                 // Is debugging enabled for the project?
# if defined(CO_DEBUGGING)                                 //  Yes, is debugging also enabled for this module?
#  define DEBUGGING                                        //   Yes, include DEBUGGING sections of this module in the compilation
# endif
# if defined(CO_DEBUGGING) || defined(MYRCS_DEBUGGING)     //  Yes, is debugging enabled for at least one module?
#  define DO_SERIAL_DEBUG                                  //   Yes, include SendOnlySoftwareSerial / SerialDebug stuff in the compilation
# endif
#endif

#define PRJ_BAUD_RATE      115200                          // For use with both Serial.begin() and SerialDebug.begin() ... in the setup() function
//#define PRJ_BAUD_RATE       57600
//#define PRJ_BAUD_RATE       38400
//#define PRJ_BAUD_RATE       19200
//#define PRJ_BAUD_RATE       14400
//#define PRJ_BAUD_RATE        9600

//#define PRJ_DEBUG_RX_PIN        8                          // Pins for debugging via SoftwareSerial
#define PRJ_DEBUG_TX_PIN        9                          // (SendOnlySoftwareSerial requires only a TX pin)

  /* Global variables */
#ifdef DO_SERIAL_DEBUG
SendOnlySoftwareSerial SerialDebug( PRJ_DEBUG_TX_PIN );
#endif

  /* Local variables */
const byte   numChars            = 90;                    // numChars must be at least 2
      char   receivedChars[numChars];                     // Input string buffer
      bool   bNewData;                                    // Did we successfully read an input string via serial connection?
      OUTLET TempOutlet;

const char WAKE_SIGNAL[]         = "<CO_WAKE>";           // Signal payload strings
const char WAKE_OK_SIGNAL[]      = "<CO_WAKE_OK>";
const char ON_SIGNAL[]           = "<CO_ON>";
const char ON_OK_SIGNAL[]        = "<CO_ON_OK>";
const char OFF_SIGNAL[]          = "<CO_OFF>";
const char OFF_OK_SIGNAL[]       = "<CO_OFF_OK>";
const char HEARTBEAT_SIGNAL[]    = "<CO_BEAT>";
const char HEARTBEAT_OK_SIGNAL[] = "<CO_BEAT_OK>";
const char SETTINGS_SIGNAL[]     = "<CO_SETTINGS>";
const char SETTINGS_OK_SIGNAL[]  = "<CO_SETTINGS_OK>";
const char OUTLET_SIGNAL[]       = "<CO_OUTLET>";
const char OUTLET_OK_SIGNAL[]    = "<CO_OUTLET_OK>";
const char LEARN_SIGNAL[]        = "<CO_LEARN>";
const char LEARN_OK_SIGNAL[]     = "<CO_LEARN_OK>";
const char VERSION_SIGNAL[]      = "<CO_VERSION>";
const char VERSION_OK_SIGNAL[]   = "<CO_VERSION_OK>";
const char EEPROM_SIGNAL[]       = "<CO_EEPROM>";
const char EEPROM_OK_SIGNAL[]    = "<CO_EEPROM_OK>";

  /*  Static function prototypes */
static void EEPROMread(                OUTLET *pOutlet );
static void ReadDelimitedString( const char   startMarker, const char endMarker );
static void ReadSettings(              void );
static void ReadLong(                  char   *longStr,          long *longVariable );
static bool LearnCode(                 OUTLET *pOutlet );
#ifdef DEBUGGING
static void PrintOutletValues(         OUTLET *pOutlet, char *szHeading );
#endif


/*****************************************************************************
 * FUNC: setup                                                               *
 * DESC: Project entry point                                                 *
 *       Executes only once, right after the Arduino is powered up or Reset  *
 *       Sets up comm links for 433MHz transmitter and serial ports          *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void setup( void )
{
  Serial.begin( PRJ_BAUD_RATE );                            // For communicating with Windows "ChargeOn" program
#ifdef DO_SERIAL_DEBUG
  SerialDebug.begin( PRJ_BAUD_RATE );                       // Start serial communications (for debugging)
  SerialDebug.println( "************************************************************" );
#endif
  EEPROMread( &Outlet );                                    // Restore outlet settings from EEPROM
  RCTransmitterSetup();                                     // Configure 433MHz RF Transmitter
  RCReceiverSetup();                                        // Configure 433MHz RF Receiver
}


/*****************************************************************************
 * FUNC: EEPROMread                                                          *
 * DESC: (Re)populate Outlet settings from EEPROM                            *
 *         1) Call this function from setup()                                *
 *            This is necessary because of the "Hibernate" feature of        *
 *            Windows. If the PC is "Hibernated" while ChargeOn is running,  *
 *            when the PC is restored it will appear that ChargeOn is running*
 *            fine -- but the Arduino module will have lost the outlet info  *
 *            when it was powered down at "Hibernate" time. If this function *
 *            is NOT called first, when it's time to turn the outlet ON or   *
 *            OFF the wrong code (0) will be sent to the outlet.             *
 *         2) Call this function any time to capture current EEPROM values   *
 * ARGS: pOutlet = Address of Outlet structure to be populated from EEPROM   *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void EEPROMread( OUTLET *pOutlet ) {
  int  nStartOffset = 0;

  EEPROM.get( nStartOffset, *pOutlet );
/*
#ifdef DEBUGGING
  PrintOutletValues( pOutlet, "Current EEPROM" );
#endif
*/
}  // EEPROMread()


/*****************************************************************************
 * FUNC: loop                                                                *
 * DESC: Main program logic                                                  *
 *       Executes after setup(); runs "forever" (until power is removed)     *
 *       Watches for angle-bracket-delimited signals from a PC running the   *
 *       Win32 "ChargeOn" program                                            *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void loop( void )
{
  ReadDelimitedString( '<', '>' );                         // Watch for next signal string
  if( bNewData ) {                                         // Did we read the signal successfully?
#ifdef DEBUGGING
    SerialDebug.print( "Received signal: " );  SerialDebug.println( receivedChars );
                                                           //  Yes, report back
#endif

      // See what kind of signal it is...
    if( !strcmp(receivedChars, WAKE_SIGNAL) ) {            //   WAKE signal
      Serial.print( WAKE_OK_SIGNAL );                      //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( WAKE_OK_SIGNAL );
#endif
    }

    else if( !strcmp(receivedChars, ON_SIGNAL) ) {         //   ON signal
      Serial.print( ON_OK_SIGNAL );                        //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( ON_OK_SIGNAL );
#endif
      RCS_SendOnCode();                                    //    Send "Turn ON" signal to remote outlet
    }

    else if( !strcmp(receivedChars, OFF_SIGNAL) ) {        //   OFF signal
      Serial.print( OFF_OK_SIGNAL );                       //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( OFF_OK_SIGNAL );
#endif
      RCS_SendOffCode();                                   //    Send "Turn OFF" signal to remote outlet
    }
    
    else if( !strcmp(receivedChars, HEARTBEAT_SIGNAL) ) {  //   BEAT signal
      Serial.print( HEARTBEAT_OK_SIGNAL );                 //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( HEARTBEAT_OK_SIGNAL );
#endif
    }

    else if( !strcmp(receivedChars, SETTINGS_SIGNAL) ) {   //   SETTINGS signal
      ReadSettings();                                      //    Read series of square-bracket-delimited Outlet Setting names & values
      Serial.print( SETTINGS_OK_SIGNAL );                  //    Send response to PC
#ifdef DEBUGGING
      PrintOutletValues( &Outlet, "Post-SETTINGS" );
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( SETTINGS_OK_SIGNAL );
#endif
      RCTransmitterSetup();                                //    Initialize RF Transmitter
    }

    else if( !strcmp(receivedChars, OUTLET_SIGNAL) ) {     //   OUTLET signal
      Serial.print( OUTLET_OK_SIGNAL );                    //    Send response to PC
#ifdef DEBUGGING
      PrintOutletValues( &Outlet, "Current Outlet" );
      SerialDebug.print( "  Sending reply: " );   SerialDebug.println( OUTLET_OK_SIGNAL );
#endif
    }

    else if( !strcmp(receivedChars, LEARN_SIGNAL) ) {      //   LEARN signal
      char szLearnCodeBuffer[75];
      if( LearnCode( &TempOutlet) ) {                      //    Learn code (and related info) from button press on outlet's remote control
        sprintf( szLearnCodeBuffer, "%s[Code:%ld][Pro:%ld][PLen:%ld][VLen:%ld][]",
                                    LEARN_OK_SIGNAL,
                                    TempOutlet.OnCode,
                                    TempOutlet.Protocol,
                                    TempOutlet.PulseLength,
                                    TempOutlet.ValueLength );
/*
#ifdef DEBUGGING
        SerialDebug.print( " Detected code = " );  SerialDebug.println( TempOutlet.OnCode );
        SerialDebug.print( "      Protocol = " );  SerialDebug.println( TempOutlet.Protocol );
        SerialDebug.print( "  Pulse Length = " );  SerialDebug.println( TempOutlet.PulseLength );
        SerialDebug.print( "    Bit Length = " );  SerialDebug.println( TempOutlet.ValueLength );
#endif
*/
      }
      else {
        sprintf( szLearnCodeBuffer, "%s[]", LEARN_OK_SIGNAL );
      }
      Serial.print( szLearnCodeBuffer );                   //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( szLearnCodeBuffer );
#endif
    }

    else if( !strcmp(receivedChars, VERSION_SIGNAL) ) {    //   VERSION signal
      char szVersionInfoBuffer[50];

      sprintf( szVersionInfoBuffer, "%s[Build:%s][]", VERSION_OK_SIGNAL, PRJ_VERSION );
      Serial.print( szVersionInfoBuffer );                 //    Send response to PC
#ifdef DEBUGGING
      SerialDebug.print( "  Sending reply: " );  SerialDebug.println( szVersionInfoBuffer );
#endif
    }

    else if( !strcmp(receivedChars, EEPROM_SIGNAL) ) {     //   EEPROM signal
      char szEEPROMbuffer[75];

      EEPROMread( &TempOutlet );                           //    Read outlet settings from EEPROM
      sprintf( szEEPROMbuffer, "%s[On:%ld][Off:%ld][Pro:%ld][PLen:%ld][PReps:%ld][TOBQ:%ld][VLen:%ld][]",
                                  EEPROM_OK_SIGNAL,
                                  TempOutlet.OnCode,
                                  TempOutlet.OffCode,
                                  TempOutlet.Protocol,
                                  TempOutlet.PulseLength,
                                  TempOutlet.PulseRepeats,
                                  TempOutlet.TurnOnBeforeQuit,
                                  TempOutlet.ValueLength );
      Serial.print( szEEPROMbuffer );                      //    Send response to PC
#ifdef DEBUGGING
      PrintOutletValues( &TempOutlet, "Current EEPROM" );
      SerialDebug.print( "  Sending reply: " );   SerialDebug.println( szEEPROMbuffer );
#endif
    }

    receivedChars[0] = '\0';                               //   Truncate the latest input string
    bNewData         = false;                              //   We no longer have an "active" input string
  }
}


/*****************************************************************************
 * FUNC: ReadDelimitedString                                                 *
 * DESC: Read a delimited value string                                       *
 * ARGS: startMarker = Delimiter preceding the value                         *
 *       endMarker   = Delimiter following the value                         *
 * RET:  [None]                                                              *
 *****************************************************************************/
void ReadDelimitedString( const char startMarker, const char endMarker )
{
  boolean recvInProgress = false;
  byte    ndx            = 0;
  char    rc;

  while( Serial.available() ) {                            // While data is waiting...
    rc = Serial.read();                                    //  Read the next byte
                                 /* KJB: Not sure why the following delay seems to be necessary? */
                                 /* KJB: Without it, the received characters do not seem to "register" ... */
    delayMicroseconds(200);
    if( !recvInProgress && (rc != startMarker) ) {         //  Have we seen the start marker yet?
      continue;                                            //   No, throw away the new byte
    }
    else {                                                 //   Yes...
      receivedChars[ndx++] = rc;                           //    Append new byte to the input buffer
      if( (rc == startMarker) && (ndx == 1) ) {            //    New byte IS the (first) start marker?
        recvInProgress = true;                             //     Yes, we're now between the start marker and end marker
      }
      else if( (ndx == numChars) || (rc == endMarker) ) {  //     No, is buffer full, or is new byte the end marker?
        receivedChars[ndx] = '\0';                         //      Yes, null-terminate the buffer
        bNewData = true;                                   //       We now have a complete input string
        break;                                             //       Exit the loop - we're done!
      }
    }
  }  // while loop
}


/*****************************************************************************
 * FUNC: ReadSettings                                                        *
 * DESC: Read series of square-bracket-delimited Outlet Setting names/values *
 *       into a set of variables ... for use with the 433MHz transmitter     *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void ReadSettings( void )
{
  do {
    bNewData = false;                                      // We don't have an "active" input string yet
    ReadDelimitedString( '[', ']' );                       // Watch for next setting string
    if( bNewData ) {                                       // Did we read the signal string successfully?
/*
#ifdef DEBUGGING
      SerialDebug.print( "  ReadSettings() - Received: " );
      SerialDebug.println( receivedChars );
#endif
*/
                                                           //  Yes, read the value into the appropriate variable
      if( !strncmp(receivedChars, "[On:", 4) ) {
        ReadLong( receivedChars+4, &Outlet.OnCode );
      }
      else if( !strncmp(receivedChars, "[Off:", 5) ) {
        ReadLong( receivedChars+5, &Outlet.OffCode );
      }
      else if( !strncmp(receivedChars, "[Pro:", 5) ) {
        ReadLong( receivedChars+5, &Outlet.Protocol );
      }
      else if( !strncmp(receivedChars, "[PLen:", 6) ) {
        ReadLong( receivedChars+6, &Outlet.PulseLength );
      }
      else if( !strncmp(receivedChars, "[PReps:", 7) ) {
        ReadLong( receivedChars+7, &Outlet.PulseRepeats );
      }
      else if( !strncmp(receivedChars, "[TOBQ:", 6) ) {
        ReadLong( receivedChars+6, &Outlet.TurnOnBeforeQuit );
      }
      else if( !strncmp(receivedChars, "[VLen:", 6) ) {
        ReadLong( receivedChars+6, &Outlet.ValueLength );
      }

      receivedChars[0] = '\0';                             //   Truncate the latest input string
    }
  } while( bNewData && strncmp(receivedChars, "[]", 2) );  // Loop until we encounter the "empty setting"

  EEPROM.put( 0, Outlet );                                 // Update outlet settings in EEPROM (in case user "Hibernates"
                                                           //  laptop while ChargeOn is running)
}  // ReadSettings()


/*****************************************************************************
 * FUNC: ReadLong                                                            *
 * DESC: Read a series of digits and convert them into a decimal value       *
 *       Populate the passed-in "long" variable with the results             *
 * ARGS: longStr      = String of digits                                     *
 *       longVariable = Pointer to "long" variable to be populated           *
 * RET:  [None]                                                              *
 *****************************************************************************/
void ReadLong( char *longStr, long *longVariable )
{
  *longVariable = 0;                                       // Initialize result to 0

  while( *longStr ) {                                      // At end of input string yet?
    if( isdigit(*longStr) ) {                              //  No, is next character a digit?
      *longVariable = ((*longVariable) * 10) + (long)((*longStr)-48);
                                                           //   Yes, multiply current result by 10 and add new digit
    }
    else if( (*longStr) == ']' ) {                         //   No, is it the "end of number" indicator?
      break;                                               //    Yes, exit the loop
    }
    longStr++;                                             //  Go to next character in input string
  }
}


/*****************************************************************************
 * FUNC: LearnCode                                                           *
 * DESC: Learn code (and related info) from button press on outlet's remote  *
 *       control. Store info in variables ... to be sent to Win32 program    *
 *       "over the wire" later.                                              * 
 * ARGS: pOutlet = Address of OUTLET structure to populate                   *
 * RET:  true  if signal from outlet remote was detected                     *
 *       false otherwise                                                     *
 *****************************************************************************/
bool LearnCode( OUTLET *pOutlet )
{
  bool bRetVal = false;
  
  bRetVal = RCS_CheckForCode( pOutlet );
  return bRetVal;
}


#ifdef DEBUGGING
/*****************************************************************************
 * FUNC: PrintOutletValues                                                   *
 * DESC: Print outlet-related values from a specified Outlet structure       *
 * ARGS: pOutlet   = Address of OUTLET structure to print values from        *
 *       szHeading = Describes source of printed values                      *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void PrintOutletValues( OUTLET *pOutlet, char *szHeading )
{
  SerialDebug.println( "  =====================" );
  SerialDebug.print(   "  " );  SerialDebug.print( szHeading );  SerialDebug.println( " Values" );
  SerialDebug.println( "  =====================" );
  SerialDebug.print(   "                ON code = " );  SerialDebug.println( pOutlet->OnCode );
  SerialDebug.print(   "               OFF code = " );  SerialDebug.println( pOutlet->OffCode );
  SerialDebug.print(   "               Protocol = " );  SerialDebug.println( pOutlet->Protocol );
  SerialDebug.print(   "           Pulse Length = " );  SerialDebug.println( pOutlet->PulseLength );
  SerialDebug.print(   "          Pulse Repeats = " );  SerialDebug.println( pOutlet->PulseRepeats );
  SerialDebug.print(   "    Turn On Before Quit = " );  SerialDebug.println( pOutlet->TurnOnBeforeQuit );
  SerialDebug.print(   "           Value Length = " );  SerialDebug.println( pOutlet->ValueLength );
  SerialDebug.println( "  =====================" );
}
#endif
