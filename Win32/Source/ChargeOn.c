/*****************************************************************************
 * FILE: ChargeOn.c                                                          *
 * DESC: Core module for ChargeOn application                                *
 * AUTH: Kerry Burton                                                        *
 * INFO: Contains core functions related to registry settings, battery status*
 *       and the WinMain application entry point                             *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

#include "ChargeOn.h"
#if 0
/*
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
*/
#endif

  /* Defines */
#define CHARGEON_REGKEY "Software\\Kerry Burton\\ChargeOn"
#define PULSE_REPEATS_DEFAULT   4

  /* Typedefs */
typedef enum { APP_X,                   // 0
               APP_Y,                   // 1
               BATTERYCHARGE_MAX,       // 2
               BATTERYCHARGE_MIN,       // 3
               CHECKCHARGE_INTERVAL,    // 4
               OUTLET_OFFCODE,          // 5
               OUTLET_ONCODE,           // 6
               OUTLET_PROTOCOL,         // 7
               OUTLET_PULSELENGTH,      // 8
               OUTLET_PULSEREPEATS,     // 9
               OUTLET_TURNONBEFOREQUIT, // 10
               OUTLET_VALUELENGTH,      // 11
               UPDATE_EVERYCHECK,       // 12
               MAX_VALUENAME            // 13
             } VALUE_NAME;                       // Registry value type enumeration

  /* Static variables */
static VALENTA vlValList[] = { {"AppX",                   sizeof(DWORD), 0, REG_DWORD },  // 0
                               {"AppY",                   sizeof(DWORD), 0, REG_DWORD },  // 1
                               {"BatteryChargeMax",       sizeof(DWORD), 0, REG_DWORD },  // 2
                               {"BatteryChargeMin",       sizeof(DWORD), 0, REG_DWORD },  // 3
                               {"CheckChargeInterval",    sizeof(DWORD), 0, REG_DWORD },  // 4
                               {"OutletOffCode",          sizeof(DWORD), 0, REG_DWORD },  // 5
                               {"OutletOnCode",           sizeof(DWORD), 0, REG_DWORD },  // 6
                               {"OutletProtocol",         sizeof(DWORD), 0, REG_DWORD },  // 7
                               {"OutletPulseLength",      sizeof(DWORD), 0, REG_DWORD },  // 8
                               {"OutletPulseRepeats",     sizeof(DWORD), 0, REG_DWORD },  // 9
                               {"OutletTurnOnBeforeQuit", sizeof(DWORD), 0, REG_DWORD },  // 10
                               {"OutletValueLength",      sizeof(DWORD), 0, REG_DWORD },  // 11
                               {"UpdateEveryCheck",       sizeof(DWORD), 0, REG_DWORD }   // 12
                             };                  // Registry value names and types

static BOOL  bTurningON  = FALSE;                // In the process of turning the outlet ON?
static BOOL  bTurningOFF = FALSE;                // In the process of turning the outlet OFF?

  /* Global variables */
DWORD  AppX                 = 50;                // Default setting values (in case the registry items don't exist or can't be read)
DWORD  AppY                 = 50;
DWORD  BatteryChargeMax     = 100;
DWORD  BatteryChargeMin     = 20;
DWORD  CheckChargeInterval  = 2;
OUTLET Outlet               = { 0, // ON code     [KJB (5 May 2020): Appropriate outlet settings must be provided to the user on
                                0, // OFF code                       an informational card in the ChargeOn package they receive.
                                0, // Protocol                       The user can enter the outlet-specific values using the
                                0, // PulseLength                    "Tools > Settings > Outlet" property page.
                                PULSE_REPEATS_DEFAULT,
                                   // PulseRepeats                   Alternately, if their ChargeOn module has a built-in Learn
                                0, // TurnOnBeforeQuit               module (and they have a remote control for the outlet) they
                                0  // ValueLength                    can use the "Learn" function on the same page. 
                              };
DWORD UpdateEveryCheck      = 1; 

HINSTANCE hInst;                                 // Handle for the Windows program instance
char      szAppFolder[MAX_PATH];                 // Folder where this program was started from
PORTINFO  SerialPort;                            // Structure containing the serial port's handle and user-friendly name
HWND      hMainDlg;                              // Window handle for the main dialog box
BYTE      byLineStatus      = UNKNOWN_STATUS;    // Is the AC power line currently providing power to the laptop?
BYTE      byBattLifePercent = UNKNOWN_PERCENT;   // The current battery charge as reported by Windows (0-100)

/* === LOCAL FUNCTIONS ===================================================== */

/*****************************************************************************
 * FUNC: InitFromRegistry                                                    *
 * DESC: Load values for "non-volatile" ChargeOn settings from the registry  *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 * NOTE: All of the ChargeOn registry values are read in as a group. If the  *
 *       ChargeOn key (or ANY of its values) does not exist, NONE of the     *
 *       values will be retrieved.                                           *
 *       In this case, the associated global variables will be initialized   *
 *       using a set of default values.                                      *
 *****************************************************************************/
void InitFromRegistry( void ) {
  HKEY    hKey;
  char    ValueBuf[(MAX_VALUENAME+1) * sizeof(DWORD)];
  DWORD   dwTotalSize = sizeof(ValueBuf) / sizeof(ValueBuf[0]);
  LSTATUS lResult     = RegOpenKeyEx( HKEY_CURRENT_USER, CHARGEON_REGKEY, 0, KEY_READ, &hKey );

  if( lResult == ERROR_SUCCESS ) {
// KJB (11 May 2020): It may be better (for backwards compatibility?) to retrieve each value separately.
    lResult = RegQueryMultipleValues( hKey, vlValList, sizeof(vlValList)/sizeof(vlValList[0]), ValueBuf, &dwTotalSize );
    RegCloseKey(hKey);
    if( lResult == ERROR_SUCCESS ) {
      AppX                    = *((DWORD *)vlValList[APP_X]                  .ve_valueptr);
      AppY                    = *((DWORD *)vlValList[APP_Y]                  .ve_valueptr);
      BatteryChargeMax        = *((DWORD *)vlValList[BATTERYCHARGE_MAX]      .ve_valueptr);
      BatteryChargeMin        = *((DWORD *)vlValList[BATTERYCHARGE_MIN]      .ve_valueptr);
      CheckChargeInterval     = *((DWORD *)vlValList[CHECKCHARGE_INTERVAL]   .ve_valueptr);
      Outlet.OffCode          = *((DWORD *)vlValList[OUTLET_OFFCODE]         .ve_valueptr);
      Outlet.OnCode           = *((DWORD *)vlValList[OUTLET_ONCODE]          .ve_valueptr);
      Outlet.Protocol         = *((DWORD *)vlValList[OUTLET_PROTOCOL]        .ve_valueptr);
      Outlet.PulseLength      = *((DWORD *)vlValList[OUTLET_PULSELENGTH]     .ve_valueptr);
      Outlet.PulseRepeats     = *((DWORD *)vlValList[OUTLET_PULSEREPEATS]    .ve_valueptr);
      Outlet.TurnOnBeforeQuit = *((DWORD *)vlValList[OUTLET_TURNONBEFOREQUIT].ve_valueptr);
      Outlet.ValueLength      = *((DWORD *)vlValList[OUTLET_VALUELENGTH]     .ve_valueptr);
      UpdateEveryCheck        = *((DWORD *)vlValList[UPDATE_EVERYCHECK]      .ve_valueptr);
    }
  }
  else {
    MessageBox( hMainDlg,
                "Could not retrieve settings from the registry; applying\ndefault values\n\n"
                  "Please review the settings for your remote outlet\n"
                  "  (Go to Tools > Settings > Outlet)",
                "Registry Error",
                MB_ICONINFORMATION | MB_OK );
  }
}


/*****************************************************************************
 * FUNC: SaveSettingsToRegistry                                              *
 * DESC: Save values for "non-volatile" ChargeOn settings to the registry    *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 * NOTE: The ChargeOn registry key is created if it does not exist.          *
 *****************************************************************************/
void SaveSettingsToRegistry( void )
{
           HKEY    hKey;
           RECT    DlgPos;
  volatile LRESULT lResult;

  lResult = RegCreateKeyEx( HKEY_CURRENT_USER,             // Open application registry key
                            CHARGEON_REGKEY,               //  (if it does not exist, create it)
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &hKey,
                            NULL );
  if( lResult == ERROR_SUCCESS ) {                         // Opened registry key OK?
    if( GetWindowRect(hMainDlg, &DlgPos) ) {               //  Yes, able to collect UI settings?
      AppX = DlgPos.left;                                  //   Yes, populate global variables appropriately
      AppY = DlgPos.top;
      lResult = SendDlgItemMessage( hMainDlg, IDC_MIN_UPDOWN, UDM_GETPOS, 0, 0 );
      if( HIWORD(lResult) == 0 ) {
        BatteryChargeMin = LOWORD( lResult );
      }
      lResult = SendDlgItemMessage( hMainDlg, IDC_MAX_UPDOWN, UDM_GETPOS, 0, 0 );
      if( HIWORD(lResult) == 0 ) {
        BatteryChargeMax = LOWORD( lResult );
      }
    }

      /* Save all settings to registry */
    lResult = RegSetValueEx( hKey, "AppX",                   0, REG_DWORD, (BYTE *)&AppX,                    sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "AppY",                   0, REG_DWORD, (BYTE *)&AppY,                    sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "BatteryChargeMax",       0, REG_DWORD, (BYTE *)&BatteryChargeMax,        sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "BatteryChargeMin",       0, REG_DWORD, (BYTE *)&BatteryChargeMin,        sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "CheckChargeInterval",    0, REG_DWORD, (BYTE *)&CheckChargeInterval,     sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletOffCode",          0, REG_DWORD, (BYTE *)&Outlet.OffCode,          sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletOnCode",           0, REG_DWORD, (BYTE *)&Outlet.OnCode,           sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletProtocol",         0, REG_DWORD, (BYTE *)&Outlet.Protocol,         sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletPulseLength",      0, REG_DWORD, (BYTE *)&Outlet.PulseLength,      sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletPulseRepeats",     0, REG_DWORD, (BYTE *)&Outlet.PulseRepeats,     sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletTurnOnBeforeQuit", 0, REG_DWORD, (BYTE *)&Outlet.TurnOnBeforeQuit, sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "OutletValueLength",      0, REG_DWORD, (BYTE *)&Outlet.ValueLength,      sizeof(DWORD) );
    lResult = RegSetValueEx( hKey, "UpdateEveryCheck",       0, REG_DWORD, (BYTE *)&UpdateEveryCheck,        sizeof(DWORD) );
  }
  RegCloseKey( hKey );
}


/*****************************************************************************
 * FUNC: EnableCharging                                                      *
 * DESC: Send signal to turn ON the AC line (to charge battery)              *
 * ARGS: pSerialPort = Pointer to PORTINFO structure containing a serial     *
 *                     port's handle and user-friendly name                  *
 * RET:  [None]                                                              *
 * NOTE: [None]                                                              *
 *****************************************************************************/
void EnableCharging( PORTINFO *pSerialPort )
{
  ShowWindow( GetDlgItem(hMainDlg, IDC_SWITCH_OUTLET), SW_HIDE );
                                                           // Hide the "Turn outlet ON" button
  SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "Turning outlet ON" );
  if( !SendSignal_GetResponse(pSerialPort, TURN_ON) ) {    // Able to ask ChargeOn module (Arduino) to turn remote outlet ON?
    SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "ERROR while turning outlet ON" );
                                                           //  No, display message
    ShowWindow( GetDlgItem(hMainDlg, IDC_SWITCH_OUTLET), SW_SHOW );
                                                           //   Redisplay the "Turn outlet ON" button
  }
  else {
    bTurningON  = TRUE;                                    //  Yes, set the "turning ON" flag
    bTurningOFF = FALSE;                                   //   Clear the "turning OFF" flag
  }
}


/*****************************************************************************
 * FUNC: DisableCharging                                                     *
 * DESC: Send signal to turn OFF the AC line (to allow battery to discharge) *
 * ARGS: pSerialPort = Pointer to PORTINFO structure containing a serial     *
 *                     port's handle and user-friendly name                  *
 * RET:  [None]                                                              *
 * NOTE: [None]                                                              *
 *****************************************************************************/
void DisableCharging( PORTINFO *pSerialPort )
{
  ShowWindow( GetDlgItem(hMainDlg, IDC_SWITCH_OUTLET), SW_HIDE );
                                                           // Hide the "Turn outlet ON/OFF" button
  SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "Turning outlet OFF" );
  if( !SendSignal_GetResponse(pSerialPort, TURN_OFF) ) {   // Able to ask ChargeOn module (Arduino) to turn remote outlet OFF?
    SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "ERROR while turning outlet OFF" );
                                                           //  No, display message
    ShowWindow( GetDlgItem(hMainDlg, IDC_SWITCH_OUTLET), SW_SHOW );
                                                           //   Redisplay the "Turn outlet OFF" button
  }
  else {
    bTurningOFF = TRUE;                                    //  Yes, set the "turning OFF" flag
    bTurningON  = FALSE;                                   //   Clear the "turning ON" flag
  }
}


/*****************************************************************************
 * FUNC: SendOutletSettings                                                  *
 * DESC: Transmit outlet settings to the ChargeOn module (Arduino)           *
 * ARGS: [None]                                                              *
 * RET:  [None]                                                              *
 * NOTE: [None]                                                              *
 *****************************************************************************/
void SendOutletSettings( PORTINFO *pSerialPort )
{
  SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "Sending outlet settings" );
  if(    (Outlet.OnCode           == 0)                    // Outlet settings all have default values?
      && (Outlet.OffCode          == 0)                    // (Unable to read settings from registry?)
      && (Outlet.Protocol         == 0)
      && (Outlet.PulseLength      == 0)
      && (Outlet.PulseRepeats     == PULSE_REPEATS_DEFAULT)
      && (Outlet.TurnOnBeforeQuit == 0)
      && (Outlet.ValueLength      == 0) ) {
    Serial_GetOutletInfo( hMainDlg, pSerialPort, EEPROM, (void *)&Outlet );
                                                           //  Yes, (try to) read outlet settings from ChargeOn module (Arduino)
  }
 
  if( !SendSignal_GetResponse(pSerialPort, SETTINGS) ) {   // Able to send (new?) outlet settings to ChargeOn module (Arduino)?
    SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "ERROR while sending outlet settings" );
                                                           //  No, display error message
  }
  else {
    SaveSettingsToRegistry();
    SetWindowText( GetDlgItem(hMainDlg, IDC_STATUS2), "" );//  Yes, remove original notification
  }
}


/*****************************************************************************
 * FUNC: CollectBatteryInfo                                                  *
 * DESC: Collect information about the battery's charge and status           *
 * ARGS: Pointer to SYSTEM_POWER_STATUS structure to be populated with info  *
 *       about the battery state and whether AC power is currently applied   *
 * RET:  TRUE if collection was gathered successfully                        *
 *       Otherwise, FALSE                                                    *
 * NOTE: [None]                                                              *
 *****************************************************************************/
BOOL CollectBatteryInfo( SYSTEM_POWER_STATUS *pSPS )
{
  BOOL bCallSuccess = FALSE;

  bCallSuccess = GetSystemPowerStatus( pSPS );             // Ask Windows to provide battery status info
  return bCallSuccess;
}


/*****************************************************************************
 * FUNC: ProcessBatteryInfo                                                  *
 * DESC: Make decisions and take actions (if any) based on battery state     *
 * ARGS: pSPS        = Pointer to SYSTEM_POWER_STATUS structure containing   *
 *                     info about the battery and AC power state             *
 *       bInfoIsGood = SYSTEM_POWER_STATUS data came from a successful call  *
 *                     to CollectBatteryInfo()                               *
 *       pSerialPort = Pointer to PORTINFO structre containing a handle to   *
 *                     serial port and the port's user-friendly name         *
 * RET:  [None]                                                              *
 * NOTE: If anything is "wrong" (SYSTEM_POWER_DATA is "bad" or missing) the  *
 *       default action is to turn the remote outlet ON.                     *
 *****************************************************************************/
void ProcessBatteryInfo( SYSTEM_POWER_STATUS *pSPS, BOOL bInfoIsGood, PORTINFO *pSerialPort )
{
  volatile BOOL bNeedToEnableCharging                     = FALSE;
  volatile BOOL bNeedToDisableCharging                    = FALSE;
  static   BOOL bNeedToNotifyAboutDischargingBelowMinimum = FALSE;
//  static BOOL bNeedToNotifyAboutChargingAboveMaximum    = FALSE;

  if(    pSPS->BatteryLifePercent == UNKNOWN_PERCENT       // Battery status is unknown
      || pSPS->ACLineStatus       == UNKNOWN_STATUS        // OR AC line status is unknown?
    ) {
    bNeedToEnableCharging = TRUE;                          //  Yes, we'd better enable charging just in case
  }
  else {
    if( bInfoIsGood ) {                                    // Got battery info OK?
      if( pSPS->ACLineStatus == 1 ) {                      //  Yes, currently charging?
        bNeedToNotifyAboutDischargingBelowMinimum = FALSE; //   Yes, clear "need to notify about discharging too far" flag (if it's set)
        if(   (pSPS->BatteryLifePercent != UNKNOWN_PERCENT)//    Battery percentage is known
           && (pSPS->BatteryLifePercent >= (BYTE)BatteryChargeMax)
                                                           //    AND battery percentage matches/exceeds maximum allowed?
          ) {
          bNeedToDisableCharging = TRUE;                   //     Yes, need to disable charging
/* KJB (11 May 2020): Do we really care about the battery charging up too far?
          if( pSPS->BatteryLifePercent > BatteryChargeMax+2 ){
                                                           //     Have we continued to charge past the target percentage?
            if( bNeedToNotifyAboutChargingAboveMaximum ) { //      Yes, do we need to alert the user?
              MessageBox( 0,                               //       Yes, do so
                          "Outlet has not turned off as expected.\nTry repositioning the ChargeOn module, or turn off the outlet manually.",
                          "Outlet did not turn OFF",
                          MB_OK | MB_ICONINFORMATION
                        );
              bNeedToNotifyAboutChargingAboveMaximum = FALSE;
                                                           //        Clear "need to notify about charging too far" flag
            }
          }  // Charging past target percentage?
*/
        }  // Need to turn outlet OFF?
        if( bTurningOFF ) {                                //    (Still) trying to turn the outlet OFF?
          bNeedToDisableCharging = TRUE;                   //     Yes, need to disable charging
        }
      }  // Currently charging?

      else {                                               //   No (not currently charging)...
//      bNeedToNotifyAboutChargingAboveMaximum = FALSE;      //    Clear "need to notify about charging too far" flag (if it's set)
        if(    (pSPS->BatteryLifePercent <= (BYTE)BatteryChargeMin)
                                                           //    Currently discharging at or below the minimum charge allowed
            || bTurningON                                  //    OR (still) trying to turn the outlet ON?
          ) {                                              
          bNeedToEnableCharging = TRUE;                    //     Yes, need to enable charging
        }
      }
    }
    else {                                                 //  No, (didn't get battery info OK)
      bNeedToEnableCharging = TRUE;                        //   We'd better enable charging just in case
    }
  }

  if( bNeedToDisableCharging ) {                           // Need to disable charging?
    DisableCharging( pSerialPort );                        //  Yes, do so
    bTurningON = FALSE;
  }
  else if( bNeedToEnableCharging ) {                       //  No, need to enable charging?
    EnableCharging( pSerialPort );                         //   Yes, do it!
    bTurningOFF = FALSE;
  }
  else {                                                   //   No, continue charging / discharging normally...
    bTurningON = bTurningOFF = FALSE;
  }
}


/*****************************************************************************
 * FUNC: WinMain                                                             *
 * DESC: Entry point for the ChargeOn application                            *
 * ARGS: hInstance = Handle for this application instance                    *
 *       h0        = Handle for previous app instance? (obsolete?)           *
 *       lpCmdLine = String of command line arguments                        *
 *       nCmdShow  = Controls initial appearance of the main window?         *
 * RET:  [None]                                                              *
 * NOTE: If anything is "wrong" (SYSTEM_POWER_DATA is "bad" or missing) the  *
 *       default action is to turn the remote outlet ON.                     *
 *****************************************************************************/
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow )
{
  HWND hPrevInstanceWindow;
  char *pLastSlash;
  MSG  msg;
  BOOL ret;
 
  hPrevInstanceWindow = FindWindow(NULL, MAIN_DIALOGCAPTION);
  if( hPrevInstanceWindow ) {                              // Is ChargeOn already running?
    SetForegroundWindow( hPrevInstanceWindow );            //  Yes, bring the window to the front
    return -1;                                             //       and EXIT
  }
  hInst = hInstance;                                       // Capture instance handle

  strcpy( szAppFolder, GetCommandLine()+1 );               // Make copy of command line (minus leading " character)
// KJB (30 May 2020): To be safe, may want to truncate command line at initial occurrence of ".exe" first
  pLastSlash = strrchr( szAppFolder, '\\' );               // Truncate command line at last backslash ('\') to get
  *pLastSlash = '\0';                                      //  folder containing ChargeOn.exe


//  InitCommonControls();
  hMainDlg = CreateDialogParam( hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), 0, MainDialogProc, 0 );
                                                           // Create main dialog window
    // Main message loop
  while( (ret = GetMessage(&msg, 0, 0, 0)) != 0 ) {        // As long as messages keep coming...
    if( ret == -1 ) {                                      // Was there a problem?
      return -1;                                           //  Yes, exit with an error
    }
    if( !IsDialogMessage(hMainDlg, &msg) ) {               //  No, did the main dialog window handle the message?
      TranslateMessage( &msg );                            //   No, so process it "normally"
      DispatchMessage( &msg );
    }
  }

  return 0;                                                // Exit cleanly
}
