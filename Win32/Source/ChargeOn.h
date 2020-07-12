/*****************************************************************************
 * FILE: ChargeOn.h                                                          *
 * DESC: Project-wide definitions                                            *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

#ifndef CHARGEON_H
# define CHARGEON_H                    // Prevent items below from being processed more than once

# define WIN32_LEAN_AND_MEAN           // To be effective, must be defined before windows.h is included

  /* Includes */
# include <windows.h>
# include <shlwapi.h>                  // For PathFileExists()
# include <prsht.h>                    // Property Sheet stuff
# include <stdio.h>                    // For sprintf()
# include <uxtheme.h>
# include "MainDlg.h"
# include "SettingsDlg.h"
# include "Serial.h"
# include "resource.h"

  /* Defines */
# define WIN32_APP_VERSION   "0.8.07"
# define UNKNOWN_STATUS      255
# define UNKNOWN_PERCENT     255

# if 0
  typedef struct _SYSTEM_POWER_STATUS {
    BYTE  ACLineStatus;         //   0 = Offline
                                //   1 = Online
                                // 255 = Unknown status
    BYTE  BatteryFlag;          //   0 = Capacity is 33%-66% and battery is not being charged
                                //   1 = High; more than 66%
                                //   2 = Low; less than 33%
                                //   4 = Critical; less than 5%
                                //   8 = Charging
                                // 128 = No system battery
                                // 255 = Unknown status; unable to read battery flag info
    BYTE  BatteryLifePercent;   // 0-100 = Percentage of full battery charge remaining
                                //   255 = Status is unknown
    BYTE  SystemStatusFlag;     // [Introduced in Windows 10]
                                // 0 = Battery saver is OFF
                                // 1 = Battery saver is ON
    DWORD BatteryLifeTime;      // The number of seconds of battery life remaining
                                //   –1 = Unknown, or the device is connected to AC power
    DWORD BatteryFullLifeTime;  // The number of seconds of battery life when at full charge
                                //   –1 = Unknown, or the device is connected to AC power
  } SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;
# endif


    /* Typedefs */
  typedef struct Outlet {
    DWORD OnCode;                      // Pertinent settings for the remote outlet
    DWORD OffCode;                     //  (to be initialized from ChargeOn module's EEPROM
    DWORD Protocol;                    //   or from Registry values sent by Windows program)
    DWORD PulseLength;
    DWORD PulseRepeats;
    DWORD TurnOnBeforeQuit;
    DWORD ValueLength;
  } OUTLET;

    /* Global function prototypes */
  void InitFromRegistry(       void );
  void SaveSettingsToRegistry( void );
  void EnableCharging(         PORTINFO            *pSerialPort );
  void DisableCharging(        PORTINFO            *pSerialPort );
  BOOL CollectBatteryInfo(     SYSTEM_POWER_STATUS *pSPS );
  void SendOutletSettings(     PORTINFO            *pSerialPort );
  void ProcessBatteryInfo(     SYSTEM_POWER_STATUS *pSPS, BOOL bInfoIsGood, PORTINFO *pSerialPort );

    /* Global variables declared in this module */
  extern DWORD     AppX;               // Non-volatile settings that get stored in the registry
  extern DWORD     AppY ;
  extern DWORD     BatteryChargeMax;
  extern DWORD     BatteryChargeMin;
  extern DWORD     CheckChargeInterval;
  extern OUTLET    Outlet;
  extern DWORD     UpdateEveryCheck;

  extern HINSTANCE hInst;              // Handle for the Windows program instance
  extern char      szAppFolder[];      // Folder where this program was started from
  extern PORTINFO  SerialPort;         // Structure containing the serial port's handle and user-friendly name
  extern HWND      hMainDlg;           // Window handle for the main dialog box
  extern BYTE      byLineStatus;       // Is the AC power line currently providing power to the laptop?
  extern BYTE      byBattLifePercent;  // The current battery charge as reported by Windows (0-100)
  extern BOOL      bSendingSettings;   // In the process of sending outlet settings to ChargeOn module (Arduino?)

#endif
