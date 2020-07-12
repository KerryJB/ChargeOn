/*****************************************************************************
 * FILE: MainDlg.h                                                           *
 * DESC: Definitions for main dialog box                                     *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

#ifndef MAINDLG_H
# define MAINDLG_H                // Prevent items below from being processed more than once

    /* Defines */

    /* Typedefs */

    /* Global variables */
  BOOL bMonitorOnly;

    /* Global function prototypes */
  INT_PTR CALLBACK MainDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

#endif
