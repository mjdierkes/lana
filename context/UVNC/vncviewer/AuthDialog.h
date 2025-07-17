/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#pragma once

enum DialogType{dtUserPass, dtPass, dtUserPassNotEncryption, dtPassUpgrade, dtUserPassRSA, dtPassRSA};

class AuthDialog  
{
public:
	AuthDialog();
	virtual ~AuthDialog();
	int DoDialog(DialogType dialogType, TCHAR IN_host[MAX_HOST_NAME_LEN], int IN_port, char hex[24] ="", char catchphrase[1024] ="");
	TCHAR m_passwd[256];
	TCHAR m_domain[256];
	TCHAR m_user[256];
	static BOOL CALLBACK DlgProc(  HWND hwndDlg,  UINT uMsg, 
		WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK DlgProc1(  HWND hwndDlg,  UINT uMsg, 
		WPARAM wParam, LPARAM lParam );

	
	//adzm 2010-05-12 - passphrase
	bool m_bPassphraseMode;
	TCHAR _host[MAX_HOST_NAME_LEN];
	int _port;
	DialogType dialogType;
	char hex[24]{};
	char catchphrase[1024]{};
};
