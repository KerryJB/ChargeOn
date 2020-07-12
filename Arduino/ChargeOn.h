/*****************************************************************************
 * FILE: ChargeOn.h                                                          *
 * DESC: Header file for main ChargeOn module                                *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/
 
#ifndef CHARGEON_H
# define CHARGEON_H

  /* Module-specific defines */
# define CO_DEBUGGING                                      // To debug the ChargeOn module (when PRJ_DEBUGGING is defined)
                                    
  /* Global variables */
# if defined(CO_DEBUGGING) || defined(MYRCS_DEBUGGING)     // Is debugging enabled for at least one module?
#  include <SendOnlySoftwareSerial.h>                      //  Yes, include SendOnlySoftwareSerial / SerialDebug
   extern SendOnlySoftwareSerial SerialDebug;              //       stuff in the compilation
# endif

  /* Public function prototypes */

#endif   // #ifndef CHARGEON_H
