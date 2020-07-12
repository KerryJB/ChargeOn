/*****************************************************************************
 * FILE: SettingsDlg.c                                                       *
 * DESC: Dialog procedure for Settings dialog box                            *
 * AUTH: Kerry Burton                                                        *
 * INFO: Handles messages sent to the Settings dialog window                 *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/
  
  /* Includes */
#include "ChargeOn.h"

  /* Defines */

  /* Typedefs */

  /* Static variables */
static HWND          hSettingsPropSheet;
static PROPSHEETPAGE Settings_PropPage[2];                 // Pages contained by the Settings property sheet (Battery, Outlet)
static OUTLET        TempOutlet;

  /* Global variables */
PROPSHEETHEADER Settings_PropSheet;                        // Property sheet for maintaining Settings values

  /* Function prototypes */
static BOOL LearnOutletCodes( HWND hParent );


/*****************************************************************************
 * FUNC: InitSettingsPropSheet                                               *
 * DESC: Initialize property sheet (and pages) for Settings dialog           *
 * ARGS: None                                                                *
 * RET:  None                                                                *
 *****************************************************************************/
void InitSettingsPropSheet( void )
{
    // Set up Property Sheet and Property Pages for Settings dialog
  memset( Settings_PropPage,   0, sizeof(Settings_PropPage)  );
  memset( &Settings_PropSheet, 0, sizeof(Settings_PropSheet) );

  Settings_PropPage[0].dwSize      = sizeof( PROPSHEETPAGE );
  Settings_PropPage[0].dwFlags     = PSP_DEFAULT|PSP_USETITLE;
  Settings_PropPage[0].hInstance   = hInst;
  Settings_PropPage[0].pszTemplate = (LPCSTR)IDD_BATTERY_PROPS_DIALOG;
  Settings_PropPage[0].pszTitle    = "Battery";
  Settings_PropPage[0].pfnDlgProc  = (DLGPROC)BatteryDlgProc;
  //Settings_PropPage[0].pfnCallback = (LPFNPSPCALLBACK) Page1Proc;

  Settings_PropPage[1].dwSize      = sizeof( PROPSHEETPAGE );
  Settings_PropPage[1].dwFlags     = PSP_USETITLE;
  //Settings_PropPage[1].dwFlags     = PSP_USETITLE | PSP_USECALLBACK;
  //Settings_PropPage[1].dwFlags     = PSP_USECALLBACK;
  Settings_PropPage[1].hInstance   = hInst;
  Settings_PropPage[1].pszTemplate = (LPCSTR)IDD_OUTLET_PROPS_DIALOG;
  Settings_PropPage[1].pszTitle    = "Outlet";
  Settings_PropPage[1].pfnDlgProc  = (DLGPROC)OutletDlgProc;
  //Settings_PropPage[1].pfnCallback = (LPFNPSPCALLBACK) Page2Proc;

  Settings_PropSheet.dwSize      = sizeof( PROPSHEETHEADER );
  Settings_PropSheet.dwFlags     = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_NOAPPLYNOW;
  //Settings_PropSheet.dwFlags     = PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_MODELESS;
  Settings_PropSheet.hInstance   = hInst;
  Settings_PropSheet.pszCaption  = (LPSTR) "Settings";
  Settings_PropSheet.nPages      = 2;
  Settings_PropSheet.nStartPage  = 0;
  Settings_PropSheet.ppsp        = (LPCPROPSHEETPAGE)Settings_PropPage;
  Settings_PropSheet.pfnCallback = (PFNPROPSHEETCALLBACK) SettingsDlgProc;
  Settings_PropSheet.hwndParent  = hMainDlg;
}


/* === LOCAL FUNCTIONS ===================================================== */

/*****************************************************************************
 * FUNC: SettingsDialogProc                                                  *
 * DESC: Dialog procedure for main Settings property sheet (dialog box)      *
 * ARGS: hSetDlg = Handle to dialog window                                   *
 *       uMsg    = Message to be handled                                     *
 *       wParam  = Typically identifies a specific control window            *
 *       lParam  = Provides additional details related to the message        *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
BOOL CALLBACK SettingsDlgProc( HWND hSetDlg, UINT message, WPARAM wParam, LPARAM lParam ) 
{
  switch( message ) {
    case WM_INITDIALOG:                                    // Invoked once, at dialog creation time
      hSettingsPropSheet = hSetDlg;
      break;

/*
KJB (27 May 2020): Figure out why following code doesn't work
    case PSCB_PRECREATE:
      // Remove the DS_CONTEXTHELP style from the dialog template
      if (((DLGTEMPLATEEX *)lParam)->signature ==  0xFFFF){
         ((DLGTEMPLATEEX *)lParam)->style &= ~DS_CONTEXTHELP;
      }
      else {
         ((DLGTEMPLATE *)lParam)->style &= ~DS_CONTEXTHELP;
      }
      break;
*/
  }
  return FALSE;
}


/*****************************************************************************
 * FUNC: BatteryDialogProc                                                   *
 * DESC: Manage everything related to the Battery property page              *
 * ARGS: hOutletDlg = Handle to dialog window                                *
 *       uMsg    = Message to be handled                                     *
 *       wParam  = Typically identifies a specific control window            *
 *       lParam  = Provides additional details related to the message        *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
BOOL CALLBACK BatteryDlgProc( HWND hBatteryDlg, UINT message, WPARAM wParam, LPARAM lParam ) 
{
  switch( message ) {
    case WM_INITDIALOG:                                    // Invoked once, at dialog creation time
      SendDlgItemMessage( hBatteryDlg, IDC_CHECK_INTERVAL, EM_SETLIMITTEXT,     1, 0 );
      SetDlgItemInt(      hBatteryDlg, IDC_CHECK_INTERVAL, CheckChargeInterval, TRUE );
      CheckDlgButton(     hBatteryDlg, UpdateEveryCheck  ? IDC_UPDATE_EVERYCHECK
                                                         : IDC_UPDATE_ONCHANGE, BST_CHECKED );
      return TRUE;

		case WM_NOTIFY:
			switch( ((NMHDR *)lParam)->code ) {
			  case PSN_APPLY:     				                       // User clicked OK or APPLY; it is this page's turn to validate/save values
        {
          BOOL bTranslatedOK;

				  CheckChargeInterval = GetDlgItemInt( hBatteryDlg, IDC_CHECK_INTERVAL, &bTranslatedOK, FALSE );
          if( CheckChargeInterval == 0 ) {                 // Is the battery check interval left blank (or set to 0?)
            CheckChargeInterval = 1;                       //  Yes, we can't have that; set it to 1
          }
          UpdateEveryCheck = IsDlgButtonChecked( hBatteryDlg, IDC_UPDATE_EVERYCHECK ) ? 1 : 0;
          SetWindowLongPtr( hBatteryDlg, DWLP_MSGRESULT, PSNRET_NOERROR );
                                                           // This page has no data validation issues
          return TRUE;
        } // PSN_APPLY

			  case PSN_KILLACTIVE:                               // User clicked OK or selected another page
          SetWindowLongPtr( hBatteryDlg, DWLP_MSGRESULT, FALSE );
                                                           // It's OK to leave this page
          return TRUE;

        case PSN_QUERYCANCEL:                              // User pressed the CANCEL or [X] button
          EndDialog( hSettingsPropSheet, 0 );              // Destroy the property sheet
          hSettingsPropSheet = NULL;                       // Set the global variable to NULL
          break;
			} // WM_NOTIFY
  }
  return FALSE;
} // BatteryDlgProc()


/*****************************************************************************
 * FUNC: OutletDialogProc                                                    *
 * DESC: Manage everything related to the Outlet property page               *
 * ARGS: hOutletDlg = Handle to dialog window                                *
 *       uMsg    = Message to be handled                                     *
 *       wParam  = Typically identifies a specific control window            *
 *       lParam  = Provides additional details related to the message        *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
BOOL CALLBACK OutletDlgProc( HWND hOutletDlg, UINT message, WPARAM wParam, LPARAM lParam ) 
{
  switch( message ) {
    case WM_INITDIALOG:                                    // Invoked once, at dialog creation time
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_ON_CODE,      EM_SETLIMITTEXT, 8, 0 );
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_OFF_CODE,     EM_SETLIMITTEXT, 8, 0 );
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_CODELENGTH,   EM_SETLIMITTEXT, 2, 0 );
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_PROTOCOL,     EM_SETLIMITTEXT, 1, 0 );
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_PULSELENGTH,  EM_SETLIMITTEXT, 3, 0 );
      SendDlgItemMessage( hOutletDlg, IDC_OUTLET_PULSEREPEATS, EM_SETLIMITTEXT, 1, 0 );

      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_ON_CODE,      Outlet.OnCode,          TRUE );
      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_OFF_CODE,     Outlet.OffCode,         TRUE );
      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_CODELENGTH,   Outlet.ValueLength,     TRUE );
      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_PROTOCOL,     Outlet.Protocol,        TRUE );
      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_PULSELENGTH,  Outlet.PulseLength,     TRUE );
      SetDlgItemInt(  hOutletDlg, IDC_OUTLET_PULSEREPEATS, Outlet.PulseRepeats,    TRUE );
      CheckDlgButton( hOutletDlg, IDC_TURNON_BEFOREQUIT,   Outlet.TurnOnBeforeQuit ? BST_CHECKED
                                                                                   : BST_UNCHECKED );
      return TRUE; // WM_INITDIALOG


    case WM_COMMAND:
      switch( LOWORD(wParam) ) {
        case IDC_OUTLET_LEARN:                             // "Learn" button
          if( HIWORD(wParam) == BN_CLICKED ) {             // Was the button clicked?
            BOOL bLearnResult = LearnOutletCodes( hOutletDlg );
                                                           //  Yes, work with user to learn (new?) outlet ON/OFF codes
            if( bLearnResult ) {                           //   Captured codes successfully?
              SetDlgItemInt(  hOutletDlg, IDC_OUTLET_ON_CODE,     TempOutlet.OnCode,      TRUE );
              SetDlgItemInt(  hOutletDlg, IDC_OUTLET_OFF_CODE,    TempOutlet.OffCode,     TRUE );
              SetDlgItemInt(  hOutletDlg, IDC_OUTLET_CODELENGTH,  TempOutlet.ValueLength, TRUE );
              SetDlgItemInt(  hOutletDlg, IDC_OUTLET_PROTOCOL,    TempOutlet.Protocol,    TRUE );
              SetDlgItemInt(  hOutletDlg, IDC_OUTLET_PULSELENGTH, TempOutlet.PulseLength, TRUE );
                                                           //    Yes, update edit control values
            }
          }
          break;
      }
      break; // WM_COMMAND


		case WM_NOTIFY:
			switch( ((NMHDR *)lParam)->code ) {
			  case PSN_APPLY:       				                     // User clicked OK or APPLY; it is this page's turn to validate/save values
        {
          BOOL bTranslatedOK;

          Outlet.OnCode           = GetDlgItemInt( hOutletDlg, IDC_OUTLET_ON_CODE,      &bTranslatedOK, FALSE );
          Outlet.OffCode          = GetDlgItemInt( hOutletDlg, IDC_OUTLET_OFF_CODE,     &bTranslatedOK, FALSE );
          Outlet.Protocol         = GetDlgItemInt( hOutletDlg, IDC_OUTLET_PROTOCOL,     &bTranslatedOK, FALSE );
          Outlet.PulseLength      = GetDlgItemInt( hOutletDlg, IDC_OUTLET_PULSELENGTH,  &bTranslatedOK, FALSE );
          Outlet.PulseRepeats     = GetDlgItemInt( hOutletDlg, IDC_OUTLET_PULSEREPEATS, &bTranslatedOK, FALSE );
          Outlet.TurnOnBeforeQuit = IsDlgButtonChecked( hOutletDlg, IDC_TURNON_BEFOREQUIT ) ? 1 : 0;
          Outlet.ValueLength      = GetDlgItemInt( hOutletDlg, IDC_OUTLET_CODELENGTH,   &bTranslatedOK, FALSE );

          SetWindowLongPtr( hOutletDlg, DWLP_MSGRESULT, PSNRET_NOERROR );
                                                           // This page has no data validation issues
// KJB (27 May 2020): Should we be hard-nosed about edit controls that are left blank or set to 0?
          return TRUE;
        }

			  case PSN_KILLACTIVE:                               // User clicked OK or selected another page
          SetWindowLongPtr( hOutletDlg, DWLP_MSGRESULT, FALSE );
                                                           // It's OK to leave this page
          return TRUE;

        case PSN_QUERYCANCEL:                              // User pressed the CANCEL or [X] button
          EndDialog( hSettingsPropSheet, 0 );              // Destroy the property sheet
          hSettingsPropSheet = NULL;                       // Set the global variable to NULL
          break;
			} // WM_NOTIFY
  }
  return FALSE;
} // OutletDialogProc()


/*****************************************************************************
 * FUNC: LearnOutletCodes                                                    *
 * DESC: Work with user to capture ON and OFF codes for (new?) power outlet  *
 * ARGS: hOutletDlg = Handle to dialog window                                *
 *       uMsg    = Message to be handled                                     *
 *       wParam  = Typically identifies a specific control window            *
 *       lParam  = Provides additional details related to the message        *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
static BOOL LearnOutletCodes( HWND hParent )
{
  char  szMessageBuffer[600];
  int   nUserResponse  = FALSE;
  BOOL  bCaptureResult = FALSE;
  BOOL  bReturnCode    = FALSE;

  do {
    nUserResponse = MessageBox( hParent,
                                "To take advantage of the \"Learn\" feature you will need:\n"
                                  "  1) A connected ChargeOn hardware module with an attached\n"
                                  "     Learn module\n"
                                  "  2) A remote control for the outlet you want to use with ChargeOn\n"
                                  "      (The remote should have separate ON and OFF buttons for the\n"
                                  "       outlet)\n\n"
                                  "To proceed:\n"
                                  "  A) Click OK button\n"
                                  "  B) Repeatedly press the remote control ON button for the intended\n"
                                  "      outlet\n"
                                  "      (Some remote controls will repeatedly send the ON signal as long as\n"
                                  "       the ON button is continuously pressed)",
                                "Learn Outlet ON Code",
                                MB_OKCANCEL );
    if( nUserResponse == IDOK ) {
      DWORD OnCode;

      bCaptureResult = Serial_GetOutletInfo( hParent, &SerialPort, LEARN, &TempOutlet );
      if( bCaptureResult ) {
        OnCode = TempOutlet.OnCode;
        sprintf( szMessageBuffer, "Outlet ON code was successfully captured.\n"
                                    "      ON Code\t=  %d\n"
                                    "      Code Length\t=  %d\n"
                                    "      Protocol\t=  %d\n"
                                    "      Pulse Length\t=  %d\n\n\n"
                                    "To capture the OFF code:\n"
                                    "  A) Click OK button\n"
                                    "  B) Repeatedly press the remote control OFF button for the intended\n"
                                    "      outlet\n"
                                    "      (Some remote controls will repeatedly send the OFF signal as long\n"
                                    "       as the OFF button is continuously pressed)",
                                  TempOutlet.OnCode, TempOutlet.ValueLength, TempOutlet.Protocol, TempOutlet.PulseLength );
        nUserResponse = MessageBox( hParent,
                                    szMessageBuffer,
                                    "Outlet ON Code Captured",
                                    MB_OKCANCEL );
        if( nUserResponse == IDOK ) {
          do {
            bCaptureResult = Serial_GetOutletInfo( hParent, &SerialPort, LEARN, &TempOutlet );
            if( bCaptureResult ) {
              TempOutlet.OnCode = OnCode;
              sprintf( szMessageBuffer, "Outlet OFF code was successfully captured.\n"
                                          "      OFF Code\t=  %d\n"
                                          "      Code Length\t=  %d\n"
                                          "      Protocol\t=  %d\n"
                                          "      Pulse Length\t=  %d\n\n\n"
                                          "Replace current outlet values with newly-captured values?",
                                        TempOutlet.OffCode, TempOutlet.ValueLength, TempOutlet.Protocol, TempOutlet.PulseLength );
              nUserResponse = MessageBox( hParent,
                                          szMessageBuffer,
                                          "Outlet OFF Code Captured",
                                          MB_YESNO );
              if( nUserResponse == IDYES ) {
                bReturnCode = TRUE;
                break;
              }
              else if( nUserResponse == IDNO ) {
                bReturnCode = FALSE;
                break;
              }
            }
            else {
              nUserResponse = MessageBox( hParent,
                                          "Unable to capture the outlet OFF code\n\n"
                                            "It may help to press the remote control OFF button continuously\n"
                                            "or to change your position relative to the ChargeOn module.\n\n"
                                            "What would you like to do?",
                                          "Capture of OFF code failed",
                                          MB_RETRYCANCEL );
            }
          } while( nUserResponse == IDRETRY );
        }
        else if( nUserResponse == IDCANCEL ) {
          bReturnCode = FALSE;
          break;
        }
      }
      else {
        nUserResponse = MessageBox( hParent,
                                    "Unable to capture the outlet ON code\n\n"
                                      "It may help to press the remote control ON button continuously\n"
                                      "or to change your position relative to the ChargeOn module.\n\n"
                                      "What would you like to do?",
                                    "Capture of ON code failed",
                                    MB_RETRYCANCEL );
      }
    }
  } while( nUserResponse == IDRETRY );

  return bReturnCode;
} // LearnOutletCodes()
