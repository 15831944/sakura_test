/*! @file
	@brief 更新通知及び確認ダイアログ

	ファイルの更新通知と動作の確認を行うダイアログボックス

	@author genta
	@date 2002.12.04
*/
/*
	Copyright (C) 2002, genta

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
/* LMP (Lucien Murray-Pitts) : 2011-02-26 Added Basic English Translation Resources */

#include "stdafx.h"
#include "CDlgFileUpdateQuery.h"
#include "sakura_rc.h"

BOOL CDlgFileUpdateQuery::OnInitDialog( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	// LMP: Added
	char _pszLabel[257];
	if( m_bModified )
		::LoadString( m_hInstance, STR_ERR_DLGUPQRY1, _pszLabel, 255 );  // LMP: Added
	else
		::LoadString( m_hInstance, STR_ERR_DLGUPQRY2, _pszLabel, 255 );  // LMP: Added

	::SetDlgItemText( hWnd, IDC_UPDATEDFILENAME, m_pFilename );
	::SetDlgItemText( hWnd, IDC_QUERYRELOADMSG, _pszLabel ) ; 
		//m_bModified ? "再ロードを行うと変更が失われますがよろしいですか?":"再ロードしますか?" );

	return CDialog::OnInitDialog( hWnd, wParam, lParam );
}

/*!
	ボタンが押されたときの動作
*/
BOOL CDlgFileUpdateQuery::OnBnClicked(int id)
{
	int result;
	switch( id ){
	case IDC_BTN_RELOAD: // 再読込
		result = 1;
		break;
	case IDC_BTN_CLOSE: // 閉じる
		result = 0;
		break;
	case IDC_BTN_NOTIFYONLY: // 以後通知メッセージのみ
		result = 2;
		break;
	case IDC_BTN_NOSUPERVISION: // 以後更新を監視しない
		result = 3;
		break;
	default:
		result = 0;
		break;
	}
	CloseDialog( result );
	
	return 0;
}
