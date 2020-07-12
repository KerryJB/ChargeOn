/*****************************************************************************
 * FILE: myRCSwitch.h                                                        *
 * DESC: Header file for myRCSwitch module                                   *
 * AUTH: Kerry Burton                                                        *
 * INFO: Controls whether debugging code in myRCSwitch module will be        *
 *       compiled, provides prototypes for "global" functions                *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/
 
#ifndef MYRCSWITCH_H
# define MYRCSWITCH_H

  /* Module-specific defines */
# define MYRCS_DEBUGGING                                      // To debug the myRCSwitch module (when PRJ_DEBUGGING is defined)

  /* Typedefs */
 typedef struct {
   long OnCode;                                               // Pertinent settings for the remote outlet (to be initialized
   long OffCode;                                              //  from Registry values sent by Windows program)
   long Protocol;
   long PulseLength;
   long PulseRepeats;
   long TurnOnBeforeQuit;
   long ValueLength;
 } OUTLET;

  /* Global variables */
 extern OUTLET Outlet;

  /* Public function prototypes */
 void RCTransmitterSetup( void );
 void RCReceiverSetup(    void );
 void RCS_SendOnCode(     void );
 void RCS_SendOffCode(    void );
 bool RCS_CheckForCode( OUTLET *pOutlet  );

#endif   // #ifndef MYRCSWITCH_H
