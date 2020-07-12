/*****************************************************************************
 * FILE: myRCSwitch.cpp                                                      *
 * DESC: Module to control 433MHz remote outlet                              *
 * AUTH: Kerry Burton                                                        *
 * INFO: Sends RF signals to a remote outlet to turn it ON and OFF           *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/
  /*  Includes */
#include "Project.h"
#include "ChargeOn.h"
#include "RCSwitch.h"
#include "myRCSwitch.h"

  /* Module-specific defines */
#if defined(PRJ_DEBUGGING) && defined(MYRCS_DEBUGGING)      // Is debugging enabled for the whole project and for this module?
# define DEBUGGING                                          //  Yes, include DEBUGGING sections of this module in the compilation
#endif
    /* Arduino pin numbers for 433MHz receiver */
//#define RCS_RX_GND_PIN --                                   // [We're using the actual GND pin, so no #define needed]
#define RCS_RX_VCC_PIN 2                                    // "Virtual VCC" pin
#define RCS_RX_DAT_PIN 3                                    // Input data pin (must be associated with a hardware interrupt)

    /* Arduino pin numbers for 433MHz transmitter */
#define RCS_TX_GND_PIN 4                                    // "Virtual GND" pin
#define RCS_TX_VCC_PIN 5                                    // "Virtual VCC" pin
#define RCS_TX_DAT_PIN 6                                    // Output data pin

  /* Global variables */
OUTLET Outlet = {0, 0, 0, 0, 0, 0, 0};                      // Global settings for the remote outlet

  /* Static variables */
static RCSwitch mySwitch = RCSwitch();

  /* Local functions */

/*****************************************************************************
 * FUNC: RCTransmitterSetup                                                  *
 * DESC: Configure 433MHz transmitter to talk to remote 120VAC outlet        *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void RCTransmitterSetup( void ) {
  pinMode( RCS_TX_GND_PIN, OUTPUT );                        // Establish "virtual GND" for 433MHz transmitter
  digitalWrite( RCS_TX_GND_PIN, LOW );
  pinMode( RCS_TX_VCC_PIN, OUTPUT );                        // Establish "virtual VCC" for 433MHz transmitter

  mySwitch.setProtocol(       Outlet.Protocol );            // Set protocol; default is 1, will work for most outlets
  mySwitch.setPulseLength(    Outlet.PulseLength );         // Set pulse length
  mySwitch.setRepeatTransmit( Outlet.PulseRepeats );        // Set number of transmission repetitions
}


/*****************************************************************************
 * FUNC: RCTransmitterEnable                                                 *
 * DESC: Provide power to 433MHz transmitter module                          *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void RCTransmitterEnable( void ) {
  digitalWrite( RCS_TX_VCC_PIN, HIGH );
  mySwitch.enableTransmit( RCS_TX_DAT_PIN );                // Assign output pin for transmitter data connection
//  delay( 30 );
#ifdef DEBUGGING
/*
SerialDebug.print( "433MHz transmission enabled on pin ");
SerialDebug.println( RCS_TX_DAT_PIN );
*/
#endif
}


/*****************************************************************************
 * FUNC: RCTransmitterDisable                                                *
 * DESC: Remove power from 433MHz transmitter module                         *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void RCTransmitterDisable( void ) {
  mySwitch.disableTransmit();
  digitalWrite( RCS_TX_VCC_PIN, LOW );
}


/*****************************************************************************
 * FUNC: RCS_SendOnCode                                                      *
 * DESC: Tell remote 120VAC outlet to turn ON                                *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void RCS_SendOnCode( void ) {
  RCTransmitterEnable();
  mySwitch.send( Outlet.OnCode, Outlet.ValueLength );
  RCTransmitterDisable();
#ifdef DEBUGGING
  SerialDebug.print( "  Sent ON code to outlet: " );
  SerialDebug.println( Outlet.OnCode );
#endif
}


/*****************************************************************************
 * FUNC: RCS_SendOffCode                                                     *
 * DESC: Tell remote 120VAC outlet to turn OFF                               *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void RCS_SendOffCode( void ) {
  RCTransmitterEnable();
  mySwitch.send( Outlet.OffCode, Outlet.ValueLength );
  RCTransmitterDisable();
#ifdef DEBUGGING
  SerialDebug.print( "  Sent OFF code to outlet: " );
  SerialDebug.println( Outlet.OffCode );
#endif
}


/*****************************************************************************
 * FUNC: RCReceiverSetup                                                     *
 * DESC: Configure 433MHz receiver to detect RC signals                      *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
void RCReceiverSetup( void ) {
//  pinMode( RCS_RX_GND_PIN, OUTPUT );                         // Establish "virtual GND" for 433MHz receiver
//  digitalWrite( RCS_RX_GND_PIN, LOW );                       // [Currently using *real* GND pin]
  pinMode( RCS_RX_VCC_PIN, OUTPUT );                         // Establish "virtual VCC" for 433MHz receiver
#ifdef DEBUGGING
/*
SerialDebug.print( "433MHz receiver enabled on pin ");
SerialDebug.print( RCS_RX_DAT_PIN );
SerialDebug.print( " (interrupt ");
SerialDebug.print( digitalPinToInterrupt(RCS_RX_DAT_PIN) );
SerialDebug.println( ")");
*/
#endif
}


/*****************************************************************************
 * FUNC: RCReceiverEnable                                                    *
 * DESC: Provide power to 433MHz receiver module                             *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void RCReceiverEnable( void ) {
  digitalWrite( RCS_RX_VCC_PIN, HIGH );                    // Provide power to receiver module
//  delay( 100 );
  mySwitch.enableReceive(  digitalPinToInterrupt(RCS_RX_DAT_PIN) );
                                                           // Assign input pin for receiver data connection
#ifdef DEBUGGING
SerialDebug.println( "    Called RCReceiverEnable");
#endif

}


/*****************************************************************************
 * FUNC: RCReceiverDisable                                                   *
 * DESC: Remove power from 433MHz receiver module                            *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 *****************************************************************************/
static void RCReceiverDisable( void ) {
#ifdef DEBUGGING
SerialDebug.println( "    Called RCReceiverDisable");
#endif

  mySwitch.disableReceive( );
  digitalWrite( RCS_RX_VCC_PIN, LOW );                     // Remove power from receiver module
}


/*****************************************************************************
 * FUNC: RCS_CheckForCode                                                    *
 * DESC: Try to detect 433MHz code                                           *
 * ARGS: pOutlet = Address of OUTLET structure to populate                   *
 * RET:  true  if signal from outlet remote was detected                     *
 *       false otherwise                                                     *
 *****************************************************************************/
bool RCS_CheckForCode( OUTLET *pOutlet  )
{
  unsigned long startMillis = millis();
           bool bResult     = false;

#ifdef DEBUGGING
SerialDebug.println( "    Called RCS_CheckForCode");
#endif

  RCReceiverEnable();
  while( startMillis + 3000 > millis() ) {
    if( mySwitch.available() ) {
#ifdef DEBUGGING
SerialDebug.println( "    Detected a button press!");
#endif
      pOutlet->OnCode      = (long)mySwitch.getReceivedValue();  // Not sure which code is being requested, so populate both
      pOutlet->OffCode     = pOutlet->OnCode;                    // ON and OFF codes with detected value
      pOutlet->Protocol    = (long)mySwitch.getReceivedProtocol();
      pOutlet->PulseLength = (long)mySwitch.getReceivedDelay();
      pOutlet->ValueLength = (long)mySwitch.getReceivedBitlength();

      mySwitch.resetAvailable();
      bResult = true;
      break;
    }
  }
  RCReceiverDisable();

  return bResult;
}
