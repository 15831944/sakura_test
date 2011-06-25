/*! @file
	@brief �����F�^�w�i�F����_�C�A���O

	@author ryoji
	@date 2006/04/26 �쐬
*/
/*
	Copyright (C) 2006, ryoji

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
#include "sakura_rc.h"
#include "sakura.hh"
#include "CDlgSameColor.h"

static const DWORD p_helpids[] = {	// 2006.10.10 ryoji
	IDOK,						HIDOK_SAMECOLOR,						// OK
	IDCANCEL,					HIDCANCEL_SAMECOLOR,					// �L�����Z��
	IDC_BUTTON_HELP,			HIDC_BUTTON_SAMECOLOR_HELP,				// �w���v
	IDC_LIST_COLORS,			HIDC_LIST_SAMECOLOR_COLORS,				// �ύX�Ώۂ̐F
	IDC_BUTTON_SELALL,			HIDC_BUTTON_SAMECOLOR_SELALL,			// �S�`�F�b�N
	IDC_BUTTON_SELNOTING,		HIDC_BUTTON_SAMECOLOR_SELNOTING,		// �S����
	IDC_LIST_ITEMINFO,			HIDC_LIST_SAMECOLOR_ITEMINFO,			// �I�𒆂̐F�ɑΉ����鍀�ڂ̃��X�g
	IDC_STATIC_COLOR,			HIDC_STATIC_COLOR,						// ����F
	0, 0
};

LPVOID CDlgSameColor::GetHelpIdTable( void )
{
	return (LPVOID)p_helpids;
}

CDlgSameColor::CDlgSameColor() :
	m_wpColorStaticProc(NULL),
	m_wpColorListProc(NULL),
	m_wID(0),
	m_pTypes(NULL),
	m_cr(0)
{
	return;
}

CDlgSameColor::~CDlgSameColor()
{
	return;
}

/*!
	�W���ȊO�̃��b�Z�[�W��ߑ�����
*/
INT_PTR CDlgSameColor::DispatchEvent( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
	INT_PTR result;
	result = CDialog::DispatchEvent( hWnd, wMsg, wParam, lParam );
	switch( wMsg ){
	case WM_COMMAND:
		// �F�I�����X�g�{�b�N�X�̑I�����ύX���ꂽ�ꍇ�̏���
		if( IDC_LIST_COLORS == LOWORD(wParam) && LBN_SELCHANGE == HIWORD(wParam) ){
			OnSelChangeListColors( (HWND)lParam );
		}
		break;

	case WM_CTLCOLORLISTBOX:
		{
			// ���ڃ��X�g�̔w�i�F��ݒ肷�鏈��
			HWND hwndLB = (HWND)lParam;
			if( IDC_LIST_ITEMINFO == ::GetDlgCtrlID( hwndLB ) ){
				HDC hdcLB = (HDC)wParam;
				::SetTextColor( hdcLB, ::GetSysColor( COLOR_WINDOWTEXT ) );
				::SetBkMode( hdcLB, TRANSPARENT );
				return (INT_PTR)::GetSysColorBrush( COLOR_BTNFACE );
			}
		}
		break;

	default:
		break;
	}
	return result;
}

/*! ���[�_���_�C�A���O�̕\��
	@param wID [in] �^�C�v�ʐݒ�_�C�A���O�ŉ����ꂽ�{�^��ID
	@param pTypes  [in/out] �^�C�v�ʐݒ�f�[�^
	@param cr [in] �w��F

	@date 2006.04.26 ryoji �V�K�쐬
*/
int CDlgSameColor::DoModal( HINSTANCE hInstance, HWND hwndParent, WORD wID, Types* pTypes, COLORREF cr )
{
	m_wID = wID;
	m_pTypes = pTypes;
	m_cr = cr;

	(void)CDialog::DoModal( hInstance, hwndParent, IDD_SAMECOLOR, NULL );

	return TRUE;
}

/*! WM_INITDIALOG ����
	@date 2006.04.26 ryoji �V�K�쐬
*/
BOOL CDlgSameColor::OnInitDialog( HWND hwndDlg, WPARAM wParam, LPARAM lParam )
{
	BOOL bRet = CDialog::OnInitDialog( hwndDlg, wParam, lParam );

	HWND hwndStatic = ::GetDlgItem( m_hWnd, IDC_STATIC_COLOR );
	HWND hwndList = ::GetDlgItem( m_hWnd, IDC_LIST_COLORS );

	// �w��F�X�^�e�B�b�N�A�F�I�����X�g���T�u�N���X��
	::SetWindowLongPtr( hwndStatic, GWLP_USERDATA, (LONG_PTR)this );
	m_wpColorStaticProc = (WNDPROC)::SetWindowLongPtr( hwndStatic, GWLP_WNDPROC, (LONG_PTR)ColorStatic_SubclassProc );
	::SetWindowLongPtr( hwndList, GWLP_USERDATA, (LONG_PTR)this );
	m_wpColorListProc = (WNDPROC)::SetWindowLongPtr( hwndList, GWLP_WNDPROC, (LONG_PTR)ColorList_SubclassProc );


	TCHAR szText[30];
	int nItem;
	int i;

	// LMP: Added
	char _pszLabel[257];

	switch( m_wID )	// �^�C�v�ʐݒ�_�C�A���O�ŉ����ꂽ�{�^��ID
	{
	case IDC_BUTTON_SAMETEXTCOLOR:
		::LoadString( m_hInstance, STR_ERR_DLGSMCLR1, _pszLabel, 255 );  // LMP: Added

		// �^�C�v�ʐݒ肩�當���F���d�����Ȃ��悤�Ɏ��o��
		::SetWindowText( m_hWnd, _pszLabel ) ; //_T("�����F����") );
		for( i = 0; i < COLORIDX_LAST; ++i ){
			if( m_cr != m_pTypes->m_ColorInfoArr[i].m_colTEXT ){
				_ultot( m_pTypes->m_ColorInfoArr[i].m_colTEXT, szText, 10 );
				if( LB_ERR == ::SendMessage( hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szText ) ){
					nItem = ::SendMessage( hwndList, LB_ADDSTRING, (WPARAM)0, (LPARAM)szText );
					::SendMessage( hwndList, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)FALSE ); 
				}
			}
		}
		break;

	case IDC_BUTTON_SAMEBKCOLOR:
		::LoadString( m_hInstance, STR_ERR_DLGSMCLR2, _pszLabel, 255 );  // LMP: Added

		// �^�C�v�ʐݒ肩��w�i�F���d�����Ȃ��悤�Ɏ��o��
		::SetWindowText( m_hWnd, _pszLabel ) ; //_T("�w�i�F����") );
		for( i = 0; i < COLORIDX_LAST; ++i ){
			if( 0 != (g_ColorAttributeArr[i].fAttribute & COLOR_ATTRIB_NO_BACK) )	// 2006.12.18 ryoji �t���O���p�Ŋȑf��
				continue;
			if( m_cr != m_pTypes->m_ColorInfoArr[i].m_colBACK ){
				_ultot( m_pTypes->m_ColorInfoArr[i].m_colBACK, szText, 10 );
				if( LB_ERR == ::SendMessage( hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szText ) ){
					nItem = ::SendMessage( hwndList, LB_ADDSTRING, (WPARAM)0, (LPARAM)szText );
					::SendMessage( hwndList, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)FALSE ); 
				}
			}
		}
		break;

	default:
		CloseDialog( IDCANCEL );
		break;
	}

	if( 0 < ::SendMessage( hwndList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0 ) ){
		::SendMessage( hwndList, LB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
		OnSelChangeListColors( hwndList );
	}

	return bRet;
}

/*! BN_CLICKED ����
	@date 2006.04.26 ryoji �V�K�쐬
*/
BOOL CDlgSameColor::OnBnClicked( int wID )
{
	HWND hwndList = ::GetDlgItem( m_hWnd, IDC_LIST_COLORS );
	int nItemNum = ::SendMessage( hwndList, LB_GETCOUNT, 0, 0 );
	BOOL bCheck;
	int i;
	int j;

	switch( wID ){
	case IDC_BUTTON_HELP:
		// �w���v	// 2006.10.07 ryoji
		MyWinHelp( m_hWnd, m_szHelpFile, HELP_CONTEXT, HLP000316 );	// 2006.10.10 ryoji MyWinHelp�ɕύX�ɕύX
		return TRUE;

	case IDC_BUTTON_SELALL:
	case IDC_BUTTON_SELNOTING:
		// �S�I���^�S�����̏���
		bCheck = (wID == IDC_BUTTON_SELALL);
		for( i = 0; i < nItemNum; ++i ){
			::SendMessage( hwndList, LB_SETITEMDATA, (WPARAM)i, (LPARAM)bCheck );
		}
		::InvalidateRect( hwndList, NULL, TRUE );
		break;

	case IDOK:
		// �^�C�v�ʐݒ肩��I��F�Ɠ��F�̂��̂����o���Ďw��F�Ɉꊇ�ύX����
		TCHAR szText[30];
		LPTSTR pszStop;
		COLORREF cr;

		for( i = 0; i < nItemNum; ++i ){
			bCheck = (BOOL)::SendMessage( hwndList, LB_GETITEMDATA, (WPARAM)i, (LPARAM)0 );
			if( bCheck ){
				::SendMessage( hwndList, LB_GETTEXT, (WPARAM)i, (LPARAM)szText );
				cr = _tcstoul( szText, &pszStop, 10 );

				switch( m_wID )
				{
				case IDC_BUTTON_SAMETEXTCOLOR:
					for( j = 0; j < COLORIDX_LAST; ++j ){
						if( cr == m_pTypes->m_ColorInfoArr[j].m_colTEXT ){
							m_pTypes->m_ColorInfoArr[j].m_colTEXT = m_cr;
						}
					}
					break;

				case IDC_BUTTON_SAMEBKCOLOR:
					for( j = 0; j < COLORIDX_LAST; ++j ){
						if( cr == m_pTypes->m_ColorInfoArr[j].m_colBACK ){
							m_pTypes->m_ColorInfoArr[j].m_colBACK = m_cr;
						}
					}
					break;

				default:
					break;
				}
			}
		}
		break;

	case IDCANCEL:
		break;
	}
	return CDialog::OnBnClicked( wID );
}

/*! WM_DRAWITEM ����
	@date 2006.04.26 ryoji �V�K�쐬
*/
BOOL CDlgSameColor::OnDrawItem( WPARAM wParam, LPARAM lParam )
{
	LPDRAWITEMSTRUCT pDis = (LPDRAWITEMSTRUCT)lParam;	// ���ڕ`����
	if( IDC_LIST_COLORS != pDis->CtlID )	// �I�[�i�[�`��ɂ��Ă���̂͐F�I�����X�g����
		return TRUE;

	//
	// �F�I�����X�g�̕`�揈��
	//
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	HPEN		hPen;
	HPEN		hPenOld;
	RECT		rc;
	TCHAR		szText[30];
	LPTSTR		pszStop;
	COLORREF	cr;

	::SendMessage( pDis->hwndItem, LB_GETTEXT, pDis->itemID, (LPARAM)szText );
	cr = _tcstoul( szText, &pszStop, 10 );

	rc = pDis->rcItem;

	// �A�C�e����`�h��Ԃ�
	::FillRect( pDis->hDC, &pDis->rcItem, ::GetSysColorBrush( COLOR_WINDOW ) );

	// �A�C�e�����I�����
	if( pDis->itemState & ODS_SELECTED ){
		rc = pDis->rcItem;
		rc.left += (rc.bottom - rc.top);
		::FillRect( pDis->hDC, &rc, ::GetSysColorBrush( COLOR_HIGHLIGHT ) );
	}

	// �A�C�e���Ƀt�H�[�J�X������
	if( pDis->itemState & ODS_FOCUS ){
		::DrawFocusRect( pDis->hDC, &pDis->rcItem );
	}

	// �`�F�b�N�{�b�N�X�\��
	rc = pDis->rcItem;
	rc.top += 2;
	rc.bottom -= 2;
	rc.left += 2;
	rc.right = rc.left + (rc.bottom - rc.top);
	UINT uState =  DFCS_BUTTONCHECK | DFCS_FLAT;
	if( TRUE == (BOOL)pDis->itemData )
		uState |= DFCS_CHECKED;		// �`�F�b�N���
	::DrawFrameControl( pDis->hDC, &rc, DFC_BUTTON, uState );

	// �F���{��`
	rc = pDis->rcItem;
	rc.left += rc.bottom - rc.top + 2;
	rc.top += 2;
	rc.bottom -= 2;
	rc.right -= 2;
	hBrush = ::CreateSolidBrush( cr );
	hBrushOld = (HBRUSH)::SelectObject( pDis->hDC, hBrush );
	hPen = ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DSHADOW ) );
	hPenOld = (HPEN)::SelectObject( pDis->hDC, hPen );
	::RoundRect( pDis->hDC, rc.left, rc.top, rc.right, rc.bottom , 5, 5 );
	::SelectObject( pDis->hDC, hPenOld );
	::SelectObject( pDis->hDC, hBrushOld );
	::DeleteObject( hPen );
	::DeleteObject( hBrush );

	return TRUE;
}

/*! �F�I�����X�g�� LBN_SELCHANGE ����
	@date 2006.05.01 ryoji �V�K�쐬
*/
BOOL CDlgSameColor::OnSelChangeListColors( HWND hwndCtl )
{
	// �F�I�����X�g�Ō��݃t�H�[�J�X�̂���F�ɂ���
	// �^�C�v�ʐݒ肩�瓯�F�̍��ڂ����o���č��ڃ��X�g�ɕ\������
	HWND hwndListInfo;
	COLORREF cr;
	TCHAR szText[30];
	LPTSTR pszStop;
	int i;
	int j;

	hwndListInfo = ::GetDlgItem( m_hWnd, IDC_LIST_ITEMINFO );
	::SendMessage( hwndListInfo, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );

	i = ::SendMessage( hwndCtl, LB_GETCARETINDEX, (WPARAM)0, (LPARAM)0 );
	if( LB_ERR != i ){
		::SendMessage( hwndCtl, LB_GETTEXT, (WPARAM)i, (LPARAM)szText );
		cr = _tcstoul( szText, &pszStop, 10 );

		switch( m_wID )
		{
		case IDC_BUTTON_SAMETEXTCOLOR:
			for( j = 0; j < COLORIDX_LAST; ++j ){
				if( cr == m_pTypes->m_ColorInfoArr[j].m_colTEXT ){
					::SendMessage( hwndListInfo, LB_ADDSTRING, (WPARAM)0, (LPARAM)m_pTypes->m_ColorInfoArr[j].m_szName);
				}
			}
			break;

		case IDC_BUTTON_SAMEBKCOLOR:
			for( j = 0; j < COLORIDX_LAST; ++j ){
			if( 0 != (g_ColorAttributeArr[j].fAttribute & COLOR_ATTRIB_NO_BACK) )	// 2006.12.18 ryoji �t���O���p�Ŋȑf��
					continue;
				if( cr == m_pTypes->m_ColorInfoArr[j].m_colBACK ){
					::SendMessage( hwndListInfo, LB_ADDSTRING, (WPARAM)0, (LPARAM)m_pTypes->m_ColorInfoArr[j].m_szName);
				}
			}
			break;

		default:
			break;
		}
	}

	return TRUE;
}

/*! �T�u�N���X�����ꂽ�w��F�X�^�e�B�b�N�̃E�B���h�E�v���V�[�W��
	@date 2006.04.26 ryoji �V�K�쐬
*/
LRESULT CALLBACK CDlgSameColor::ColorStatic_SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	HDC			hDC;
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	HPEN		hPen;
	HPEN		hPenOld;
	RECT		rc;

	CDlgSameColor* pCDlgSameColor;
	pCDlgSameColor = (CDlgSameColor*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );

	switch( uMsg ){
	case WM_PAINT:
		// �E�B���h�E�`��
		PAINTSTRUCT ps;

		hDC = ::BeginPaint( hwnd, &ps );

		// �F���{��`
		::GetClientRect( hwnd, &rc );
		rc.left += 2;
		rc.top += 2;
		rc.right -=2;
		rc.bottom -= 2;
		hBrush = ::CreateSolidBrush( pCDlgSameColor->m_cr );
		hBrushOld = (HBRUSH)::SelectObject( hDC, hBrush );
		hPen = ::CreatePen( PS_SOLID, 1, ::GetSysColor( COLOR_3DSHADOW ) );
		hPenOld = (HPEN)::SelectObject( hDC, hPen );
		::RoundRect( hDC, rc.left, rc.top, rc.right, rc.bottom, 5, 5 );
		::SelectObject( hDC, hPenOld );
		::SelectObject( hDC, hBrushOld );
		::DeleteObject( hPen );
		::DeleteObject( hBrush );

		::EndPaint( hwnd, &ps );
		return (LRESULT)0;

	case WM_ERASEBKGND:
		// �w�i�`��
		hDC = (HDC)wParam;
		::GetClientRect( hwnd, &rc );

		// �e��WM_CTLCOLORSTATIC�𑗂��Ĕw�i�u���V���擾���A�w�i�`�悷��
		hBrush = (HBRUSH)::SendMessage( GetParent( hwnd ), WM_CTLCOLORSTATIC, wParam, (LPARAM)hwnd );
		hBrushOld = (HBRUSH)::SelectObject( hDC, hBrush );
		::FillRect( hDC, &rc, hBrush );
		::SelectObject( hDC, hBrushOld );

		return (LRESULT)1;

	case WM_DESTROY:
		// �T�u�N���X������
		::SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)pCDlgSameColor->m_wpColorStaticProc );
		pCDlgSameColor->m_wpColorStaticProc = NULL;
		return (LRESULT)0;

	default:
		break;
	}

	return CallWindowProc( (WNDPROC)pCDlgSameColor->m_wpColorStaticProc, hwnd, uMsg, wParam, lParam );
}

/*! �T�u�N���X�����ꂽ�F�I�����X�g�̃E�B���h�E�v���V�[�W��
	@date 2006.04.26 ryoji �V�K�쐬
*/
LRESULT CALLBACK CDlgSameColor::ColorList_SubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	POINT po;
	RECT rcItem;
	RECT rc;
	int nItemNum;
	int i;

	CDlgSameColor* pCDlgSameColor;
	pCDlgSameColor = (CDlgSameColor*)::GetWindowLongPtr( hwnd, GWLP_USERDATA );

	switch( uMsg ){
	case WM_LBUTTONUP:
		// �}�E�X�{�^�����ɂ��鍀�ڂ̑I���^�I���������g�O������
		po.x = LOWORD(lParam);	// horizontal position of cursor
		po.y = HIWORD(lParam);	// vertical position of cursor
		nItemNum = ::SendMessage( hwnd, LB_GETCOUNT, 0, 0 );
		for( i = 0; i < nItemNum; ++i ){
			::SendMessage( hwnd, LB_GETITEMRECT, i, (LPARAM)&rcItem );
			rc = rcItem;
			rc.top += 2;
			rc.bottom -= 2;
			rc.left += 2;
			rc.right = rc.left + (rc.bottom - rc.top);
			if( ::PtInRect( &rc, po ) ){
				BOOL bCheck;
				bCheck = !(BOOL)::SendMessage( hwnd, LB_GETITEMDATA, (WPARAM)i, (LPARAM)0 );
				::SendMessage( hwnd, LB_SETITEMDATA, (WPARAM)i, (LPARAM)bCheck );
				::InvalidateRect( hwnd, &rcItem, TRUE );
				break;
			}
		}
		break;

	case WM_KEYUP:
		// �t�H�[�J�X���ڂ̑I���^�I���������g�O������
		if( VK_SPACE == wParam ){
			BOOL bCheck;
			i = ::SendMessage( hwnd, LB_GETCARETINDEX, (WPARAM)0, (LPARAM)0 );
			if( LB_ERR != i ){
				bCheck = !(BOOL)::SendMessage( hwnd, LB_GETITEMDATA, (WPARAM)i, (LPARAM)0 );
				::SendMessage( hwnd, LB_SETITEMDATA, (WPARAM)i, (LPARAM)bCheck );
				::InvalidateRect( hwnd, NULL, TRUE );
			}
		}
		break;

	case WM_DESTROY:
		// �T�u�N���X������
		::SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)pCDlgSameColor->m_wpColorListProc );
		pCDlgSameColor->m_wpColorListProc = NULL;
		return (LRESULT)0;

	default:
		break;
	}

	return ::CallWindowProc( (WNDPROC)pCDlgSameColor->m_wpColorListProc, hwnd, uMsg, wParam, lParam );
}

/*[EOF]*/