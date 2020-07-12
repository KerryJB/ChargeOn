/*****************************************************************************
 * FILE: MainDlg.c                                                           *
 * DESC: Dialog procedure for main dialog                                    *
 * AUTH: Kerry Burton                                                        *
 * INFO: Handles messages sent to the main dialog window                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

  /* Includes */
#include "ChargeOn.h"
#include <CommCtrl.h>
#include <commdlg.h>
//#include <tchar.h>
//#include <uxtheme.h>

  /* Defines */
#define MINMIN   20                         // Absolute minimum value for "Min %" spinner control
#define MINMAX   95                         // Absolute maximum value for "Min %" spinner control
#define MAXMIN   25                         // Absolute minimum value for "Max %" spinner control
#define MAXMAX  100                         // Absolute maximum value for "Max %" spinner control

  /* Typedefs */

  /* Static variables */
static HMENU   hMenu;                       // Menu handle for the main application menu
static HFONT   hFontPercent;                // Handle for font to be used in the IDC_PERCENTCHARGED static text control
static HFONT   hFontCharging;               // Handle for font to be used in the IDC_CHARGING static text control
static BOOL    bSerialOK = FALSE;           // Is the serial port connection currently "alive"?
static char    szTempBuffer[300];           // Used as the destination for "sprintf" calls (mostly for Message Box text)
//static LOGFONT m_lfont;

  /* Global variables */
BOOL bMonitorOnly = FALSE;                  // Flag to record user's choice about whether to continue even though no serial port is available

  /* Function prototypes */

/* === LOCAL FUNCTIONS ===================================================== */

/*****************************************************************************
 * FUNC: MainDialogProc                                                      *
 * DESC: Manage everything related to the main dialog box                    *
 * ARGS: hDlg   = Handle to dialog window                                    *
 *       uMsg   = Message to be handled                                      *
 *       wParam = Typically identifies a specific control window             *
 *       lParam = Provides additional details related to the message         *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
BOOL CALLBACK MainDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg ) {
    case WM_INITDIALOG:                                    // Invoked once, at dialog creation time
      SetClassLong( hDlg, GCL_HICON, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(ICON_32)) );
                                                           // Load application icon
      hMenu = LoadMenu( hInst, MAKEINTRESOURCE(IDM_MENU) );// Set up the main menu
      SetMenu( hDlg, hMenu );
      InitFromRegistry();                                  // Load application (and remote outlet) settings from the registry

      hFontPercent  = CreateFont( 24, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial" );
      hFontCharging = CreateFont( 16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial" );
      SendDlgItemMessage( hDlg, IDC_PERCENTCHARGED, WM_SETFONT, (int)hFontPercent, TRUE );
                                                           // Use a large font to display the battery charge percentage
      SendDlgItemMessage( hDlg, IDC_CHARGING, WM_SETFONT, (int)hFontCharging, TRUE );
                                                           // Use a medium font for the "Charging..." and "Discharging..." text
      SetWindowText( GetDlgItem(hDlg, IDC_CHARGING), "Checking battery status..." );
                                                           // Display preliminary text while the app starts up
      SetWindowTheme( GetDlgItem(hDlg, IDC_PROGRESS), L" ", L" ");
                                                           // Don't use animated progress bar style
      SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETBKCOLOR, 0, 0x00C0C0C0 );
                                                           // Use light gray background for the progress bar
      SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETBARCOLOR, 0, 0x00ffffff );
                                                           // Set default progress bar color to white

                                                           // Set ranges and current positions for the "Min %" and "Max %" spinner controls
      SendDlgItemMessage( hDlg, IDC_MIN_UPDOWN, UDM_SETPOS,   0, BatteryChargeMin );
      SendDlgItemMessage( hDlg, IDC_MIN_UPDOWN, UDM_SETRANGE, 0, MAKELPARAM(MINMAX, MINMIN) );
      SendDlgItemMessage( hDlg, IDC_MAX_UPDOWN, UDM_SETPOS,   0, BatteryChargeMax );
      SendDlgItemMessage( hDlg, IDC_MAX_UPDOWN, UDM_SETRANGE, 0, MAKELPARAM(MAXMAX, MAXMIN) );

      ShowWindow( GetDlgItem(hDlg, IDC_SWITCH_OUTLET), SW_HIDE );
                                                           // Don't display the "Turn outlet ON/OFF" button initially

      SetWindowPos( hDlg, HWND_TOPMOST, AppX, AppY, 1, 1, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );
                                                           // Set main dialog window's position and SHOW the window
      bSerialOK = InitSerial( &SerialPort );               // Look for / configure a ChargeOn hardware module (usually connected via USB)
      if( !bSerialOK ) {                                   // Found a (connected & available) ChargeOn module?
        int nRetval = MessageBox( hDlg,                    //  No, let user decide whether to continue or quit
                                  "ERROR: Could not find an available/suitable ChargeOn module.\n\nPress OK to monitor the battery, or Cancel to exit.",
                                  "ChargeOn Module Not Found",
                                  MB_ICONEXCLAMATION | MB_OKCANCEL );
        if( nRetval == IDCANCEL ) {                        //   Did user decide to quit?
          SendMessage(hDlg, WM_CLOSE, 0, 0);               //    Yes, close the app
          return TRUE;
        }
        else {                                             //    No...
          bMonitorOnly = TRUE;                             //     We can't control the remote outlet without a ChargeOn module,
                                                           //     so just monitor the battery charge
          SetWindowText( GetDlgItem(hDlg, IDC_STATUS),
                         bMonitorOnly ? "Could not find available ChargeOn module"
                                      : "" );
          SetWindowText( GetDlgItem(hDlg, IDC_STATUS2), "Outlet control is DISABLED" );
        }
      }
      SetTimer(hDlg, IDT_TIMER1, CheckChargeInterval*1000, (TIMERPROC)NULL );
                                                           // Set "check battery state" timer to fire every X seconds (user-configurable)

      InitSettingsPropSheet();                             // Initialize property sheet (and pages) for Settings dialog
      RegisterHotKey( hDlg,                                // Set up "hot keys" for various special functions
                      1,
                      MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                      0x45 );                              //  Hotkey #1 is Ctrl-Shift-E
      RegisterHotKey( hDlg,
                      2,
                      MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT,
                      0x4F );                              //  Hotkey #2 is Ctrl-Shift-O

      return TRUE;
      break;  // WM_INITDIALOG

    case WM_TIMER:
      switch( wParam ) { 
        case IDT_TIMER1:                                   // It's time to check the battery state!
        {
          if( bInitializingPort || bDoingTX_RX ) {         // Hang on ... are we in the middle of talking to the
                                                           // ChargeOn module (Arduino)?
            return 0;                                      //  Yes, ignore this timer tick and wait for the next one
          }

          if( bMonitorOnly ) {                             // In "only monitor battery" mode?
             bSerialOK = InitSerial( &SerialPort );        //  Yes, see if a ChargeOn hardware module has been plugged in
                                                           //   (and if so - configure it)
             if( bSerialOK ) {                             //   Found & configured a ChargeOn hardware module?
               bMonitorOnly = FALSE;                       //    Yes, turn off "only monitor battery" mode
             }
          }

          SYSTEM_POWER_STATUS SysPowStat;                  // Windows API structure to store battery-related info
          BOOL                bCollectedInfoOK;            // Indicates whether battery-related info was collected successfully
          BOOL                bBattPctChanged     = FALSE; // Indicates whether battery life percent value changed
          BOOL                bLineStatusChanged  = FALSE; // Indicates whether AC line started/stopped providing power
          SYSTEMTIME          stSysTime;                   // Stores current time

          GetLocalTime( &stSysTime );                      // Capture current time

          bCollectedInfoOK  = CollectBatteryInfo( &SysPowStat );
                                                           // Get details about battery's current state
          if( bCollectedInfoOK ) {                         // Was the battery state info captured successfully?
            if( SysPowStat.BatteryLifePercent != byBattLifePercent ) {
                                                           //  Yes, did the battery percentage change?
              bBattPctChanged   = TRUE;                    //   Yes, remember that
              byBattLifePercent = SysPowStat.BatteryLifePercent;
                                                           //    Update battery percentage variable
            }
            if( UpdateEveryCheck || bBattPctChanged ){     //   Need to update the display on every timer interval
                                                           //   OR the battery percentage changed?
              sprintf( szTempBuffer, "%d%%", byBattLifePercent );
                                                           //    Yes, display current battery percentage (numerical value)
              SetWindowText( GetDlgItem(hDlg, IDC_PERCENTCHARGED), szTempBuffer );
              SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETPOS, byBattLifePercent, 0 );
                                                           //     Display current battery percentage (progress bar)
            }

            if( SysPowStat.ACLineStatus != byLineStatus ){ //   Did the AC line status change?
              bLineStatusChanged = TRUE;                   //    Yes, remember that 
              byLineStatus       = SysPowStat.ACLineStatus;//     Update line status variable
            }
            if( UpdateEveryCheck || bLineStatusChanged ){  //   Need to update the display on every timer interval
                                                           //   OR the AC line status changed?
              SetWindowText( GetDlgItem(hDlg, IDC_CHARGING),
                             (byLineStatus == 0) ? "Discharging..."
                                                 : "Charging..." );
                                                           //    Yes, display current dis/charging status in textual form
/* KJB (06 May 2020): Intended for logging purposes, using an "extra" window and/or a log file
              sprintf( szTempBuffer, "%02d%%   %s   %02d:%02d:%02d",
                                     byBattLifePercent,
                                     (byLineStatus == 0) ? "Discharging..." : "Charging...",
                                     stSysTime.wHour,
                                     stSysTime.wMinute,
                                     stSysTime.wSecond );
*/
              SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETBARCOLOR, 0, byLineStatus ? 0x0010fe10 : 0x002020fe );
                                                           //     Set progress bar to green if charging, red if discharging
              if( !bMonitorOnly ) {                        //     Stuck in "monitor only" mode?
                if( bLineStatusChanged ) {                 //      No, did the power line status change?
                  SetWindowText( GetDlgItem(hDlg, IDC_STATUS2), "" );
                                                           //       Yes, clear the Status2 message
                }
                SetWindowText( GetDlgItem(hDlg, IDC_SWITCH_OUTLET),
                               (byLineStatus == 0) ? "Turn outlet ON"
                                                   : "Turn outlet OFF" );
                ShowWindow( GetDlgItem(hDlg, IDC_SWITCH_OUTLET), bSerialOK ? SW_SHOW : SW_HIDE);
                                                           //        Display (or hide) the appropriate button
              }
            }
          }
          else {                                           //  No (battery state info was NOT captured successfully)...
            SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETPOS, byBattLifePercent, 100 );
            SendDlgItemMessage( hDlg, IDC_PROGRESS, PBM_SETBARCOLOR, 0, 0x00ffffff );
                                                           //   Set progress bar to white
            sprintf( szTempBuffer, "%02d:%02d:%02d    Could not collect battery info",
                                   stSysTime.wHour,
                                   stSysTime.wMinute,
                                   stSysTime.wSecond );
            SetWindowText( GetDlgItem(hDlg, IDC_CHARGING), szTempBuffer );
                                                           //   Display message
          }
          if( !bMonitorOnly ) {                            // Are we only doing no-outlet-control battery monitoring?
            ProcessBatteryInfo( &SysPowStat, bCollectedInfoOK, &SerialPort );
                                                           //  No, so make decisions and take actions (if any) based on battery state
          }
        }

          // Regardless of whether we collected / acted on battery state info...
        if( !bMonitorOnly && !bInitializingPort ) {        // Are we in "control" mode and NOT currently initializing a serial port?
          if( SendSignal_GetResponse(&SerialPort, HEARTBEAT) ) {
                                                           //  Yes, were we able to send a "heartbeat" signal to the ChargeOn module
                                                           //  and receive an appropriate response?
            sprintf( szTempBuffer, "Connected on %s", SerialPort.szPortName );
            SetWindowText( GetDlgItem(hDlg, IDC_STATUS), szTempBuffer );
                                                           //   Yes, display name of the serial port we're connected to
          }
          else {                                           //   No (heartbeat signal exchange failed)...
            SetWindowText( GetDlgItem(hDlg, IDC_STATUS), "Lost communication with ChargeOn module" );
                                                           //    Display message
            CloseHandle( SerialPort.hComPort );            //    Close current COM port handle
            bSerialOK = FALSE;
            bSerialOK = InitSerial( &SerialPort );         //    Try to find/connect to a ChargeOn module
          }
        }
        return 0;                                          // Message was processed
      }  // WM_TIMER

    case WM_NOTIFY:
    {
      LPNMUPDOWN pUpDown = (LPNMUPDOWN)lParam;
      switch( pUpDown->hdr.code ) {
        case UDN_DELTAPOS:
          if( pUpDown->hdr.idFrom == IDC_MIN_UPDOWN )      // "Minimum" spin control
          {
            DWORD dwNewMinCandidate = pUpDown->iPos + pUpDown->iDelta;

            if(    (pUpDown->iDelta < 0)                   //   Down arrow was clicked
                && (dwNewMinCandidate >= MINMIN) )         //   and not yet at bottom of hard-coded MIN range?
            {
              BatteryChargeMin = dwNewMinCandidate;        //    Yes, subtract 1 from minimum value
            }
            else if(    (pUpDown->iDelta > 0)              //    No, Up arrow was clicked
                     && (dwNewMinCandidate <= MINMAX) )    //    and not yet at top of hard-coded MIN range?
            {
              BatteryChargeMin = dwNewMinCandidate;        //     Yes, add 1 to minimum value
              if( BatteryChargeMin > BatteryChargeMax-2 ){ //      Does new minimum value differ from maximum value by less than 2?
                BatteryChargeMax = BatteryChargeMin+2;     //       Yes, increase maximum value so it's at least 2 greater than minimum value
                SendDlgItemMessage( hDlg, IDC_MAX_UPDOWN, UDM_SETPOS, 0, BatteryChargeMax );
              }
            }
          }
          else if( pUpDown->hdr.idFrom == IDC_MAX_UPDOWN ){// "Maximum" spin control
            DWORD dwNewMaxCandidate = pUpDown->iPos + pUpDown->iDelta;

            if(    (pUpDown->iDelta < 0)                   //   Down arrow was clicked
                && (dwNewMaxCandidate >= MAXMIN) )         //   and not yet at bottom of hard-coded MAX range?
            {
              BatteryChargeMax = dwNewMaxCandidate;        //    Yes, subtract 1 from maximum value
              if( BatteryChargeMax < BatteryChargeMin+2 ){ //     Does new maximum value differ from minimum value by less than 2?
                BatteryChargeMin = BatteryChargeMax-2;     //      Yes, decrease minimum value so it's at least 2 less than maximum value
                SendDlgItemMessage( hDlg, IDC_MIN_UPDOWN, UDM_SETPOS, 0, BatteryChargeMin );
              }
            }
            else if(    (pUpDown->iDelta > 0)              //    No, Up arrow was clicked
                     && (dwNewMaxCandidate <= MAXMAX) )    //    and not yet at top of hard-coded MAX range?
            {
              BatteryChargeMax = dwNewMaxCandidate;        //     Yes, add 1 to maximum value
            }
          }
// KJB (04 May 2020): Briefly experimented with a "dynamic" progress bar range based on the user-selected Min/Max values...
//     (11 May 2020): This could be a preferences setting? [Progress range: 1) 0%-100%   2) Min% to Max%]
          //SendMessage( GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(BatteryChargeMin, BatteryChargeMax) );
          return FALSE;
        break;
      }
      break;  // WM_NOTIFY
    }

    case WM_COMMAND:
      switch( LOWORD(wParam) ) {
        case IDC_MIN_EDIT:                                 // "Min %" control
          if( HIWORD(wParam) == EN_KILLFOCUS ) {           // Just lost focus...
            int  nEnteredMinVal;
            BOOL bTranslatedOK;

            nEnteredMinVal = GetDlgItemInt( hDlg, IDC_MIN_EDIT, &bTranslatedOK, FALSE );
            if( nEnteredMinVal < MINMIN ) {                // Is the entered value less than the minimum allowed?
              BatteryChargeMin = MINMIN;                   //  Yes, set battery minimum to the minimum allowed
            }
            else if( nEnteredMinVal > MINMAX ) {           //  No, is it greater than the maximum allowed?
              BatteryChargeMin = MINMAX;                   //   Yes, set battery minimum to the maximum allowed
            }
            else {
              BatteryChargeMin = nEnteredMinVal;           //   No, set battery minimum to the entered value
            }
            SendDlgItemMessage( hDlg, IDC_MIN_UPDOWN, UDM_SETPOS, 0, BatteryChargeMin );
                                                           // Update position of "Min %" spinner

            if( BatteryChargeMin > BatteryChargeMax-2 ) {  // (New) battery minimum is too close to current battery maximum?
              BatteryChargeMax = BatteryChargeMin + 2;     //  Yes, adjust battery maximum appropriately
              SendDlgItemMessage( hDlg, IDC_MAX_UPDOWN, UDM_SETPOS, 0, BatteryChargeMax );
                                                           //   Update position of "Max %" spinner to match
            }
// KJB (04 May 2020): Briefly experimented with a "dynamic" progress bar range based on the user-selected Min/Max values...
//     (11 May 2020): This could be a preferences setting? [Progress range: 1) 0%-100%   2) Min% to Max%]
//SendMessage( GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(BatteryChargeMin, BatteryChargeMax) );
            return FALSE;
          }
          break; // IDC_MIN_EDIT


        case IDC_MAX_EDIT:                                 // "Max %" control
          if( HIWORD(wParam) == EN_KILLFOCUS ) {           // Just lost focus...
            int  nEnteredMaxVal;
            BOOL bTranslatedOK;

            nEnteredMaxVal = GetDlgItemInt( hDlg, IDC_MAX_EDIT, &bTranslatedOK, FALSE );
            if( nEnteredMaxVal < MAXMIN ) {                // Is the entered value less than the minimum allowed?
              BatteryChargeMax = MAXMIN;                   //  Yes, set battery maximum to the minimum allowed
            }
            else if( nEnteredMaxVal > MAXMAX ) {           //  No, is it greater than the maximum allowed?
              BatteryChargeMax = MAXMAX;                   //   Yes, set battery maximum to the maximum allowed
            }
            else {
              BatteryChargeMax = nEnteredMaxVal;           //   No, set battery maximum to the entered value
            }
            SendDlgItemMessage( hDlg, IDC_MAX_UPDOWN, UDM_SETPOS, 0, BatteryChargeMax );
                                                           // Update position of "Max %" spinner

            if( BatteryChargeMax < BatteryChargeMin+2 ) {  // (New) battery maximum is too close to current battery minimum?
              BatteryChargeMin = BatteryChargeMax - 2;     //  Yes, adjust battery minimum appropriately
              SendDlgItemMessage( hDlg, IDC_MIN_UPDOWN, UDM_SETPOS, 0, BatteryChargeMin );
                                                           //   Update position of "Min %" spinner to match
            }
// KJB (04 May 2020): Briefly experimented with a "dynamic" progress bar range based on the user-selected Min/Max values...
//     (11 May 2020): This could be a preferences setting? [Progress range: 1) 0%-100%   2) Min% to Max%]
//SendMessage( GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(BatteryChargeMin, BatteryChargeMax) );
            return FALSE;
          }
          break; // IDC_MAX_EDIT


        case IDM_TOOLS_SETTINGS:                           // Menu item "Tools > Settings..."
        {
          INT_PTR nPropSheetResult = PropertySheet( &Settings_PropSheet );
                                                           // Start the "Settings" property sheet (dialog box)
          if( nPropSheetResult > 0 ) {                     // Did user save changes?
            SetTimer(hDlg, IDT_TIMER1, CheckChargeInterval*1000, (TIMERPROC)NULL );
                                                           //  Yes, set "check battery state" timer to fire every X seconds (user-configurable)
            if( bSerialOK ) {                              //   Found a (connected & available) ChargeOn module?
              SendOutletSettings( &SerialPort );           //    Yes, send (updated?) outlet settings to ChargeOn module
            }
          }
        }
        break; // IDM_TOOLS_SETTINGS


        case IDM_TOOLS_UPDATE:                             // Menu item "Tools > Update firmware..."
        {
          OPENFILENAME ofn;
          char         szaFilter[]                     = {"HEX Files\0*.hex\0\0"};
          char         szFilename[MAX_PATH]            = "";
          char         szInitialDir[MAX_PATH];
          char         szAvrdudeCommandLine[MAX_PATH];


// KJB (12 Jun 2020): Need to make sure we're currently connected to a ChargeOn module before proceeding
//                    If not, put up a MessageBox saying that to update the firmware for a ChargeOn module,
//                    that module must be plugged in and recognized by the ChargeOn program

          sprintf( szInitialDir, "%s\\avrdude\\hexfiles", szAppFolder );

          ofn.lStructSize       = sizeof( ofn );
          ofn.hwndOwner         = hDlg;
          ofn.hInstance         = NULL;
          ofn.lpstrFilter       = szaFilter;
          ofn.lpstrCustomFilter = NULL;
          ofn.nMaxCustFilter    = 0;
          ofn.nFilterIndex      = 0;
          ofn.lpstrFile         = szFilename;              // Full path of selected file is returned here
          ofn.nMaxFile          = sizeof( szFilename );
          ofn.lpstrFileTitle    = NULL;
          ofn.nMaxFileTitle     = 0;
          ofn.lpstrInitialDir   = szInitialDir;
          ofn.lpstrTitle        = NULL;
          ofn.Flags             =   OFN_DONTADDTORECENT
                                  | OFN_FILEMUSTEXIST
                                  | OFN_HIDEREADONLY
                                  | OFN_NONETWORKBUTTON
                                  | OFN_PATHMUSTEXIST;
          ofn.nFileOffset       = 0;
          ofn.nFileExtension    = 0;
          ofn.lpstrDefExt       = NULL;
          ofn.lCustData         = 0;
          ofn.lpfnHook          = NULL;
          ofn.lpTemplateName    = NULL;
          ofn.FlagsEx           = OFN_EX_NOPLACESBAR;
/*
          ofn.lpEditInfo    // LPEDITMENU
          ofn.lpstrPrompt   // LPCSTR
          ofn.pvReserved    // void
          ofn.dwReserved    // DWORD
*/

          if( GetOpenFileName(&ofn) ) {                    // User specified an existing *.hex file?
            STARTUPINFO         si;
            PROCESS_INFORMATION pi;

            sprintf( szAvrdudeCommandLine,                 //  Yes, create the command line for AVRDUDE
                     "%s\\avrdude\\bin\\avrdude -C%s\\avrdude\\etc\\avrdude.conf "
                       "-v -patmega328p -carduino -P%s -b57600 -D "
                       "-Uflash:w:%s:i",
                     szAppFolder,
                     szAppFolder,
                     SerialPort.szPortName,
                     ofn.lpstrFile );
/*
            MessageBox( hDlg,                              //   (Temporary, for testing/debugging only)
                        szAvrdudeCommandLine,
                        "AVRDUDE Command Line",
                        MB_OK);
*/
            CloseHandle( SerialPort.hComPort );            //   Close serial port
            bSerialOK   = FALSE;
            bDoingTX_RX = TRUE;                            //   "Disable" timer ticks while we're using the serial port

            ZeroMemory( &si, sizeof(si) );                 //   Prepare to kick off AVRDUDE (to reflash Arduino with specified *.hex file)
            si.cb = sizeof(si);
            ZeroMemory( &pi, sizeof(pi) );
            sprintf( szInitialDir, "%s\\avrdude\\bin", szAppFolder );
            if( CreateProcess(                             //   Invoked AVRDUDE sucessfully?
                              NULL,                        //      No module name (use command line)
                              szAvrdudeCommandLine,        //      Command line
                              NULL,                        //      Process handle not inheritable
                              NULL,                        //      Thread handle not inheritable
                              FALSE,                       //      Set handle inheritance to FALSE
                              0,                           //      No creation flags
                              NULL,                        //      Use parent's environment block
                              szInitialDir,                //      Starting directory is the one containing AVRDUDE executable
                              &si,                         //      Pointer to STARTUPINFO structure
                              &pi) )                       //      Pointer to PROCESS_INFORMATION structure
            {
              WaitForSingleObject( pi.hProcess, INFINITE );//    Yes, wait until child process exits
              CloseHandle( pi.hProcess );                  //     Close process and thread handles
              CloseHandle( pi.hThread );
            }
            else {                                         //    No...
/*
              if( GetLastError() == 740 ) {
                strcpy( szTempBuffer,
                        "Unable to start AVRDUDE as regular user.\n\n"
                          "  Close ChargeOn program, then use\n"
                          "  \t\"Run as administrator\"\n"
                          "  to restart it - and try again.\n\n" );
              }
              else {
*/
                sprintf( szTempBuffer, "Unknown error %d            ", GetLastError() );
//              }
              MessageBox( hDlg, szTempBuffer, "Could not run AVRDUDE", MB_ICONWARNING | MB_OK );
            }
            bDoingTX_RX = FALSE;                           //   Re-enable timer ticks (and use of serial port)
          }
        }
        break; // IDM_TOOLS_UPDATE


        case IDM_TOOLS_DRIVER:                             // Menu item "Tools > Install driver..."
        {
          STARTUPINFO         si;
          PROCESS_INFORMATION pi;
          char szDriverFolder[MAX_PATH];
          char szCommandLine[MAX_PATH];

          ZeroMemory( &si, sizeof(si) );
          si.cb = sizeof(si);
          ZeroMemory( &pi, sizeof(pi) );
          sprintf( szDriverFolder, "%s\\drivers\\CH341SER\\", szAppFolder );
          sprintf( szCommandLine, "%sSETUP.EXE", szDriverFolder );

          if( PathFileExists(szCommandLine) ) {
/*
  if( bDoingTX_RX == TRUE ) {                              // Already communicating with serial port? 
    Sleep( 50 );                                           //  Yes, wait before trying again
    if( bDoingTX_RX == TRUE ) {                            //   Still communicating with serial port?
      return FALSE;                                        //    Yes, fail
    }
  }
  bDoingTX_RX = TRUE;                                      // Starting a new "conversation" with serial port
*/
            CloseHandle( SerialPort.hComPort );            // Close serial port
            bSerialOK   = FALSE;
            bDoingTX_RX = TRUE;                            //   "Disable" timer ticks while we're using the serial port

            // Start the child process. 
            if( CreateProcess(NULL,                        // No module name (use command line)
                              szCommandLine,               // Command line
                              NULL,                        // Process handle not inheritable
                              NULL,                        // Thread handle not inheritable
                              FALSE,                       // Set handle inheritance to FALSE
                              0,                           // No creation flags
                              NULL,                        // Use parent's environment block
                              szDriverFolder,              // Starting directory is the one containing the driver installer app
                              &si,                         // Pointer to STARTUPINFO structure
                              &pi) ) {                     // Pointer to PROCESS_INFORMATION structure
              WaitForSingleObject( pi.hProcess, INFINITE );// Wait until child process exits
              CloseHandle( pi.hProcess );                  // Close process and thread handles
              CloseHandle( pi.hThread );
            }
            else {
              if( GetLastError() == 740 ) {
                sprintf( szTempBuffer,
                         "Unable to start driver installer as regular user.\n\n"
                           "Either:\n"
                           "  1) Close ChargeOn program, then use\n"
                           "  \t\"Run as administrator\"\n"
                           "      to restart it - and try again.\n\n"
                           "  2) Navigate to folder\n"
                           "  \t%s\n"
                           "      and run the SETUP.EXE program manually.",
                         szDriverFolder );
              }
              else {
                strcpy( szTempBuffer, "Unknown error" );
              }
              MessageBox( hDlg, szTempBuffer, "Could not run driver installer", MB_ICONWARNING | MB_OK );
            }
          }
          else {
            sprintf( szTempBuffer,
                       "Driver installer file\n"
                         "    %s\n"
                         "does not exist.\n\n"
                         "Be sure to run ChargeOn from the folder where it was installed.",
                     szCommandLine );
            MessageBox( hDlg, szTempBuffer, "Driver installer not found", MB_ICONWARNING | MB_OK );
          }
          bDoingTX_RX = FALSE;                             //   Re-enable timer ticks (and use of serial port)

        }
        break; // IDM_TOOLS_DRIVER

// ===========================================================

        case IDM_HELP_ABOUT:                               // Menu item "Help > About ChargeOn"
        {
          char szArduinoVersion[50];
          BOOL bGotArduinoVersionOK;

          bGotArduinoVersionOK = GetArduinoSketchVersion( hDlg, &SerialPort, szArduinoVersion );
          sprintf( szTempBuffer,
                   "ChargeOn\n"
                     "    Win32 program version: %s\n"
                     "    Arduino sketch  version: %s\n\n"
                     "    Copyright © Kerry Burton 2020\n\n"
                     "       =============================\n\n"
                     "Icon images courtesy of FreeVector.com\n"
                     "  (https://www.freevector.com/batteries-vectors#)",
                   WIN32_APP_VERSION,
                   bGotArduinoVersionOK ? szArduinoVersion : "[Unknown]");
          MessageBox( hDlg,
                      szTempBuffer,
                      "About ChargeOn",
                      MB_OK );
//          DialogBox( hInst, MAKEINTRESOURCE(IDD_HELPABOUTDIALOG), hDlg, OptionsDlgProc );
                                                           // Start the "Help > About" dialog box
        }
        break; // IDM_HELP_ABOUT

        case IDC_SWITCH_OUTLET:                            // "Turn outlet ON/OFF" button
          if( HIWORD(wParam) == BN_CLICKED ) {             // Was the button clicked?
            if( byLineStatus ) {                           //  Yes, is the outlet currently ON?
              DisableCharging( &SerialPort );              //   Yes, turn outlet OFF
            }
            else {                                         //   No...
              EnableCharging( &SerialPort );               //    Turn outlet ON
            }
          }
          break;

        case IDM_FILE_EXIT:                                // Received message to exit the application
          SendMessage(hDlg, WM_CLOSE, 0, 0);               // Send message to close the main dialog
          break;

        default:
          return FALSE;
      }
      return TRUE;
      break;  // WM_COMMAND


    case WM_HOTKEY:                                        // Registered hotkey was pressed
      if( wParam == 1 ) {                                  //  Was it hotkey #1?
        Serial_GetOutletInfo( hMainDlg, &SerialPort, EEPROM, (void *)&Outlet );
                                                           //   Yes, tell ChargeOn module to display EEPROM contents
      }
      else if( wParam == 2 ) {                             //   No, was it hotkey #2?
        SendSignal_GetResponse( &SerialPort, SHOW_OUTLET );//    Yes, tell ChargeOn module to display Outlet values
      }
      break;  // WM_HOTKEY


    case WM_CLOSE:                                         // Received message to close the main dialog
    {
      KillTimer(hDlg, IDT_TIMER1);                         // Don't do any more battery checks
      SaveSettingsToRegistry();                            // Save ALL settings (not just UI settings) to the registry
      if( bSerialOK && (byLineStatus == 0) ) {             // ChargeOn module is connected, and battery is currently discharging?
        int nRetval;
      
        if( Outlet.TurnOnBeforeQuit ) {                    //   Yes, is the "Turn outlet ON before quitting" box checked?
          nRetval = IDYES;                                 //    Yes, remember that
        }
        else {
          nRetval = MessageBox( hDlg,
                                "Would you like the outlet to be turned ON before quitting?\n\n"
                                  "          -----------------------------------------\n\n"
                                  "To make this the default behavior, go to\n"
                                  "      Tools > Settings > Outlet\n"
                                  "and check the box\n"
                                  "      \"Turn outlet ON before quitting\"",
                                "BATTERY IS DISCHARGING!",
                                MB_ICONWARNING | MB_YESNOCANCEL ); 
                                                           //    No, let user decide whether to turn outlet on before quitting
        }
        if( nRetval == IDCANCEL ) {                        //     User clicked "Cancel"?
          return TRUE;                                     //      Yes, don't close the app
        }
        else if( nRetval == IDYES ) {                      //      No, user clicked "Yes"?
          SendSignal_GetResponse( &SerialPort, TURN_ON );  //       Yes, turn on the outlet!
        }
        CloseHandle( SerialPort.hComPort );                //     Close current COM port handle
      }
      DestroyWindow(hDlg);                                 // Send message to destroy the main dialog window
      return TRUE;
    } // WM_CLOSE


    case WM_DESTROY:                                       // Received message to destroy the main dialog window
      PostQuitMessage(0);                                  // Exit the main message loop (end the application)
      return TRUE;
  }

  return FALSE;
}  // MainDlgProc

