/*!	@file
	@brief 1�s���̓_�C�A���O�{�b�N�X

	@author Norio Nakatani
	@date	1998/05/31 �쐬
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2002, MIK
	Copyright (C) 2003, KEITA
	Copyright (C) 2006, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/
/* LMP (Lucien Murray-Pitts) : 2011-02-26 Added Basic English Translation Resources */

#include "stdafx.h"
#include "sakura_rc.h"
#include "CDlgInput1.h"
#include "debug.h"

// ���� CDlgInput1.cpp	//@@@ 2002.01.07 add start MIK
#include "etc_uty.h"
#include "sakura.hh"
static const DWORD p_helpids[] = {	//13000
	IDOK,					HIDOK_DLG1,
	IDCANCEL,				HIDCANCEL_DLG1,
	IDC_EDIT1,				HIDC_DLG1_EDIT1,	//���̓t�B�[���h
	IDC_STATIC_MSG,			HIDC_DLG1_EDIT1,	//���b�Z�[�W
//	IDC_STATIC,				-1,
	0, 0
};	//@@@ 2002.01.07 add end MIK


/* �_�C�A���O�v���V�[�W�� */
INT_PTR CALLBACK CDlgInput1Proc(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam 	// second message parameter
)
{
	CDlgInput1* pCDlgInput1;
	switch( uMsg ){
	case WM_INITDIALOG:
		pCDlgInput1 = ( CDlgInput1* )lParam;
		if( NULL != pCDlgInput1 ){
			return pCDlgInput1->DispatchEvent( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	default:
		// Modified by KEITA for WIN64 2003.9.6
		pCDlgInput1 = ( CDlgInput1* )::GetWindowLongPtr( hwndDlg, DWLP_USER );
		if( NULL != pCDlgInput1 ){
			return pCDlgInput1->DispatchEvent( hwndDlg, uMsg, wParam, lParam );
		}else{
			return FALSE;
		}
	}
}



CDlgInput1::CDlgInput1()
{
	/* �w���v�t�@�C���̃t���p�X��Ԃ� */
	::GetHelpFilePath( m_szHelpFile );	//@@@ 2002.01.07 add

	return;
}



CDlgInput1::~CDlgInput1()
{
	return;
}



/* ���[�h���X�_�C�A���O�̕\�� */
BOOL CDlgInput1::DoModal( HINSTANCE hInstApp, HWND hwndParent, const char* pszTitle, const char* pszMessage, int nMaxTextLen, char* pszText )
{
	BOOL bRet;
	m_hInstance = hInstApp;		/* �A�v���P�[�V�����C���X�^���X�̃n���h�� */
	m_hwndParent = hwndParent;	/* �I�[�i�[�E�B���h�E�̃n���h�� */
	m_pszTitle = pszTitle;		/* �_�C�A���O�^�C�g�� */
	m_pszMessage = pszMessage;		/* ���b�Z�[�W */
	m_nMaxTextLen = nMaxTextLen;	/* ���̓T�C�Y��� */
//	m_pszText = pszText;			/* �e�L�X�g */
	m_cmemText.SetDataSz( pszText );
	bRet = (BOOL)::DialogBoxParam(
		m_hInstance,
		MAKEINTRESOURCE( IDD_INPUT1 ),
		m_hwndParent,
		CDlgInput1Proc,
		(LPARAM)this
	);
	strcpy( pszText, m_cmemText.GetPtr() );
	return bRet;
}



/* �_�C�A���O�̃��b�Z�[�W���� */
INT_PTR CDlgInput1::DispatchEvent(
	HWND hwndDlg,	// handle to dialog box
	UINT uMsg,		// message
	WPARAM wParam,	// first message parameter
	LPARAM lParam 	// second message parameter
)
{
	WORD	wNotifyCode;
	WORD	wID;
	HWND	hwndCtl;
//	int		nRet;
	switch( uMsg ){
	case WM_INITDIALOG:
		/* �_�C�A���O�f�[�^�̐ݒ� */
		// Modified by KEITA for WIN64 2003.9.6
		::SetWindowLongPtr( hwndDlg, DWLP_USER, lParam );

		::SetWindowText( hwndDlg, m_pszTitle );	/* �_�C�A���O�^�C�g�� */
		::SendMessage( ::GetDlgItem( hwndDlg, IDC_EDIT1 ), EM_LIMITTEXT, m_nMaxTextLen, 0 );	/* ���̓T�C�Y��� */
		::SetWindowText( ::GetDlgItem( hwndDlg, IDC_EDIT1 ), m_cmemText.GetPtr() );	/* �e�L�X�g */
		::SetWindowText( ::GetDlgItem( hwndDlg, IDC_STATIC_MSG ), m_pszMessage );	/* ���b�Z�[�W */

		return TRUE;
	case WM_COMMAND:
		wNotifyCode = HIWORD(wParam);	/* �ʒm�R�[�h */
		wID			= LOWORD(wParam);	/* ����ID� �R���g���[��ID� �܂��̓A�N�Z�����[�^ID */
		hwndCtl		= (HWND) lParam;	/* �R���g���[���̃n���h�� */
		switch( wNotifyCode ){
		/* �{�^���^�`�F�b�N�{�b�N�X���N���b�N���ꂽ */
		case BN_CLICKED:
			switch( wID ){
			case IDOK:
				m_cmemText.AllocBuffer( ::GetWindowTextLength( ::GetDlgItem( hwndDlg, IDC_EDIT1 ) ) );
				::GetWindowText( ::GetDlgItem( hwndDlg, IDC_EDIT1 ), m_cmemText.GetPtr(), m_nMaxTextLen + 1 );	/* �e�L�X�g */
				::EndDialog( hwndDlg, TRUE );
				return TRUE;
			case IDCANCEL:
				::EndDialog( hwndDlg, FALSE );
				return TRUE;
			}
			break;	//@@@ 2002.01.07 add
		}
		break;	//@@@ 2002.01.07 add
	//@@@ 2002.01.07 add start
	case WM_HELP:
		{
			HELPINFO *p = (HELPINFO *)lParam;
			MyWinHelp( (HWND)p->hItemHandle, m_szHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelp�ɕύX�ɕύX
		}
		return TRUE;

	//Context Menu
	case WM_CONTEXTMENU:
		MyWinHelp( hwndDlg, m_szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)p_helpids );	// 2006.10.10 ryoji MyWinHelp�ɕύX�ɕύX
		return TRUE;
	//@@@ 2002.01.07 add end
	}
	return FALSE;
}


/*[EOF]*/