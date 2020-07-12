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

  /* Global variables */

  /* Function prototypes */

/* === LOCAL FUNCTIONS ===================================================== */

/*****************************************************************************
 * FUNC: SettingsDialogProc                                                   *
 * DESC: Manage everything related to the (modal) Settings dialog box         *
 * ARGS: hSetDlg = Handle to dialog window                                    *
 *       uMsg    = Message to be handled                                      *
 *       wParam  = Typically identifies a specific control window             *
 *       lParam  = Provides additional details related to the message         *
 * RET:  TRUE  = Message was handled by this function                        *
 *       FALSE = Message was NOT handled by this function                    *
 *****************************************************************************/
BOOL CALLBACK SettingsDlgProc( HWND hSetDlg, UINT message, WPARAM wParam, LPARAM lParam ) 
{
  switch( message ) {
    case WM_INITDIALOG:                                    // Invoked once, at dialog creation time
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_ON_CODE,      EM_SETLIMITTEXT, 8, 0 );
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_OFF_CODE,     EM_SETLIMITTEXT, 8, 0 );
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_CODELENGTH,   EM_SETLIMITTEXT, 2, 0 );
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_PROTOCOL,     EM_SETLIMITTEXT, 1, 0 );
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_PULSELENGTH,  EM_SETLIMITTEXT, 3, 0 );
      SendDlgItemMessage( hSetDlg, IDC_OUTLET_PULSEREPEATS, EM_SETLIMITTEXT, 1, 0 );
      SendDlgItemMessage( hSetDlg, IDC_CHECK_INTERVAL,      EM_SETLIMITTEXT, 1, 0 );

      SetDlgItemInt( hSetDlg, IDC_OUTLET_ON_CODE,      OutletOnCode,        TRUE );
      SetDlgItemInt( hSetDlg, IDC_OUTLET_OFF_CODE,     OutletOffCode,       TRUE );
      SetDlgItemInt( hSetDlg, IDC_OUTLET_CODELENGTH,   OutletValueLength,   TRUE );
      SetDlgItemInt( hSetDlg, IDC_OUTLET_PROTOCOL,     OutletProtocol,      TRUE );
      SetDlgItemInt( hSetDlg, IDC_OUTLET_PULSELENGTH,  OutletPulseLength,   TRUE );
      SetDlgItemInt( hSetDlg, IDC_OUTLET_PULSEREPEATS, OutletPulseRepeats,  TRUE );
      SetDlgItemInt( hSetDlg, IDC_CHECK_INTERVAL,      CheckChargeInterval, TRUE );
      CheckDlgButton( hSetDlg,
                      IDC_TURNON_BEFOREQUIT,
                      TurnOnBeforeQuit
                        ? BST_CHECKED
                        : BST_UNCHECKED );
      CheckDlgButton( hSetDlg,
                      UpdateEveryCheck
                        ? IDC_UPDATE_EVERYCHECK
                        : IDC_UPDATE_ONCHANGE,
                      BST_CHECKED );
      break;

    case WM_COMMAND:
      switch( wParam ) {
        case IDOK:
        {
          BOOL bTranslatedOK;

          OutletOnCode        = GetDlgItemInt( hSetDlg, IDC_OUTLET_ON_CODE,      &bTranslatedOK, FALSE );
          OutletOffCode       = GetDlgItemInt( hSetDlg, IDC_OUTLET_OFF_CODE,     &bTranslatedOK, FALSE );
          OutletValueLength   = GetDlgItemInt( hSetDlg, IDC_OUTLET_CODELENGTH,   &bTranslatedOK, FALSE );
          OutletProtocol      = GetDlgItemInt( hSetDlg, IDC_OUTLET_PROTOCOL,     &bTranslatedOK, FALSE );
          OutletPulseLength   = GetDlgItemInt( hSetDlg, IDC_OUTLET_PULSELENGTH,  &bTranslatedOK, FALSE );
          OutletPulseRepeats  = GetDlgItemInt( hSetDlg, IDC_OUTLET_PULSEREPEATS, &bTranslatedOK, FALSE );
          CheckChargeInterval = GetDlgItemInt( hSetDlg, IDC_CHECK_INTERVAL,      &bTranslatedOK, FALSE );
          TurnOnBeforeQuit    = IsDlgButtonChecked( hSetDlg, IDC_TURNON_BEFOREQUIT ) ? 1 : 0;
          UpdateEveryCheck    = IsDlgButtonChecked( hSetDlg, IDC_UPDATE_EVERYCHECK ) ? 1 : 0;
        }
        case IDCANCEL:
          EndDialog( hSetDlg, FALSE );
          break;
      }
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
