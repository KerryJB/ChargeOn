/*****************************************************************************
 * FILE: SettingsDlg.h                                                       *
 * DESC: Definitions for Settings dialog box                                 *
 * AUTH: Kerry Burton                                                        *
 * INFO:                                                                     *
 *****************************************************************************
 * COPYRIGHT 2020 Kerry Burton. ALL RIGHTS RESERVED.                         *
 *****************************************************************************/

#ifndef SETTINGSDLG_H
# define SETTINGSDLG_H                                     // Prevent items below from being processed more than once

    /* Defines */

    /* Typedefs */

    /* Global variables */
  extern PROPSHEETHEADER Settings_PropSheet;               // Property sheet for maintaining Settings values

    /* Global function prototypes */
  void InitSettingsPropSheet(    void );
  BOOL CALLBACK SettingsDlgProc( HWND hSetDlg,     UINT message, WPARAM wParam, LPARAM lParam );
  BOOL CALLBACK BatteryDlgProc(  HWND hBatteryDlg, UINT message, WPARAM wParam, LPARAM lParam );
  BOOL CALLBACK OutletDlgProc(   HWND hOutletDlg,  UINT message, WPARAM wParam, LPARAM lParam );

#endif
