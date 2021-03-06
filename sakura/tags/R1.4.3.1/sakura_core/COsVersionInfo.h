/*!	@file
	@brief COsVersionInfo

	OSVERSIONINFOをラッピング

	@author YAZAKI
	@date 2002年3月3日
*/
/*
	Copyright (C) 2001, YAZAKI

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#ifndef _COSVERSIONINFO_H_
#define _COSVERSIONINFO_H_

class COsVersionInfo {
public:
	COsVersionInfo(){
		memset( (void *)&m_cOsVersionInfo, 0, sizeof( m_cOsVersionInfo ) );
		m_cOsVersionInfo.dwOSVersionInfoSize = sizeof( m_cOsVersionInfo );
		m_bSuccess = ::GetVersionEx( &m_cOsVersionInfo );
	};
	
	/* OsVersionが取得できたか？ */
	BOOL GetVersion(){
		return m_bSuccess;
	}
	
	/* 使用しているOS（Windows）が、動作対象か確認する */
	BOOL OsIsEnableVersion(){
		return !( m_cOsVersionInfo.dwMajorVersion < 4 );
	};
	
	
	// From Here Jul. 5, 2001 shoji masami
	/*! NTプラットフォームかどうか調べる

		@retval TRUE NT platform
		@retval FALSE non-NT platform
	*/
	BOOL IsWin32NT(){
		return (m_cOsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
	}

	/*	::WinHelp( hwnd, lpszHelp, HELP_COMMAND, (unsigned long)"CONTENTS()" );
		が使用できないバージョンなら、TRUE
		使用できるバージョンなら、FALSE
	*/
	BOOL HasWinHelpContentsProblem(){
		return ( IsWin32NT() && (m_cOsVersionInfo.dwMajorVersion <= 4));
	}
	
	/*	再変換がOS標準で提供されていないか。
		提供されていないなら、TRUE。
		提供されているなら、FALSE。
	
		Windows95 or WindowsNTなら、TRUE（提供されていない）
		それ以外のOSなら、FALSE（提供されている）
	*/
	BOOL OsDoesNOTSupportReconvert(){
		return ((4 == m_cOsVersionInfo.dwMajorVersion) && ( 0 == m_cOsVersionInfo.dwMinorVersion ));
	}
#if 0
	2002.04.11 YAZAKI カプセル化を守る。
	// 2002.04.08 minfu OSVERSIONINFO構造体へのポインタを返す
	POSVERSIONINFO GetOsVersionInfo(){
		return &m_cOsVersionInfo;
	}
#endif

	/*! Luna GUIが使えるか調べる

		@retval TRUE Luna is available (Windows XP or later)
		@retval FALSE Luna is not available 

		@date 2003.09.06 genta
	*/
	BOOL IsLuna(){
		return (m_cOsVersionInfo.dwMajorVersion >= 5 &&
			m_cOsVersionInfo.dwMinorVersion >= 1 );
	}

protected:
	BOOL m_bSuccess;
	OSVERSIONINFO m_cOsVersionInfo;
};

#endif
