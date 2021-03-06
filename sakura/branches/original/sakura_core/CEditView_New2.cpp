//	$Id$
/************************************************************************
        CEditView_New2.cpp
	Copyright (C) 1998-2000, Norio Nakatani

        CREATE: 1998/12/8
************************************************************************/


//#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>
#include "CEditView.h"
#include "debug.h"
#include "keycode.h"
#include "funccode.h"
#include "CRunningTimer.h"
#include "charcode.h"
#include "mymessage.h"
#include "CWaitCursor.h"
#include "CEditWnd.h"
#include "CShareData.h"
#include "CDlgCancel.h"
#include "sakura_rc.h"
#include "etc_uty.h"
#include "CJre.h"


/* 現在の色を指定 */
void CEditView::SetCurrentColor( HDC hdc, int nCOMMENTMODE )
{
	int				nColorIdx;
	COLORREF		colText;
	COLORREF		colBack;
//	if( NULL != m_hFontOld ){
//		::SelectObject( hdc, m_hFontOld );
//		m_hFontOld = NULL;
//	}
	nColorIdx = -1;
	switch( nCOMMENTMODE ){
	case 0:
		nColorIdx = COLORIDX_TEXT;
//		colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_TEXT].m_colTEXT;	/* テキスト色 */ 
//		colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_TEXT].m_colBACK;	/* 背景色 */     
//		::SetTextColor( hdc, colText );
//		::SetBkColor( hdc, colBack );
//		if( NULL != m_hFontOld ){
//			::SelectObject( hdc, m_hFontOld );
//			m_hFontOld = NULL;
//		}
		
		break;
	case 1:	/* 行コメントである */					
	case 2:	/* ブロックコメントである */					
		nColorIdx = COLORIDX_COMMENT;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_COMMENT].m_bDisp ){	/* コメントを表示する */
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_COMMENT].m_colTEXT;	/* コメント色 */                     
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_COMMENT].m_colBACK;	/* コメント背景の色 */               
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_COMMENT].m_bFatFont ){	/* 太字か */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 3:	/* シングルクォーテーション文字列である */
		nColorIdx = COLORIDX_SSTRING;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SSTRING].m_bDisp ){	/* シングルクォーテーション文字列を表示する */
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SSTRING].m_colTEXT;	/* シングルクォーテーション文字列色 */
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SSTRING].m_colBACK;	/* シングルクォーテーション文字列背景の色 */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SSTRING].m_bFatFont ){	/* 太字か */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 4:	/* ダブルクォーテーション文字列である */					
		nColorIdx = COLORIDX_WSTRING;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_WSTRING].m_bDisp ){	/* ダブルクォーテーション文字列を表示する */
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_WSTRING].m_colTEXT;	/* ダブルクォーテーション文字列色 */
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_WSTRING].m_colBACK;	/* ダブルクォーテーション文字列背景の色 */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_WSTRING].m_bFatFont ){	/* 太字か */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 5:	/* キーワード（登録単語）文字列である */					
		nColorIdx = COLORIDX_KEYWORD;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_KEYWORD].m_bDisp ){
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_KEYWORD].m_colTEXT;	/* 強調キーワードの色 */   
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_KEYWORD].m_colBACK;	/* 強調キーワード背景の色 */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_KEYWORD].m_bFatFont ){	/* 太字か */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 6:	/* コントロールコードである */					
		nColorIdx = COLORIDX_CTRLCODE;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_CTRLCODE].m_bDisp ){
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_CTRLCODE].m_colTEXT;	/* コントロールコードの色 */   
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_CTRLCODE].m_colBACK;	/* コントロールコードの背景色 */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_CTRLCODE].m_bFatFont ){	/* 太字か */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 80:	/* URLである */					
		nColorIdx = COLORIDX_URL;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_URL].m_bDisp ){
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_URL].m_colTEXT;	/* URL */   
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_URL].m_colBACK;	/* URL */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_URL].m_bFatFont ){	/* URL */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	case 90:	/* 検索文字列である */					
		nColorIdx = COLORIDX_SEARCH;
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SEARCH].m_bDisp ){
//			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SEARCH].m_colTEXT;	/* URL */   
//			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SEARCH].m_colBACK;	/* URL */
//			::SetTextColor( hdc, colText );
//			::SetBkColor( hdc, colBack );
//			if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_SEARCH].m_bFatFont ){	/* URL */
//				if( NULL != m_hFontOld ){
//					::SelectObject( hdc, m_hFontOld );		
//				}
//				m_hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//			}
//		}
		break;
	}

	
	
	
	
	if( -1 != nColorIdx ){
		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIdx].m_bDisp ){
			colText = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIdx].m_colTEXT;
			colBack = m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIdx].m_colBACK;
			::SetTextColor( hdc, colText );
			::SetBkColor( hdc, colBack );
			if( NULL != m_hFontOld ){
				::SelectObject( hdc, m_hFontOld );		
			}
			/* フォントを選ぶ */
			m_hFontOld = (HFONT)::SelectObject( hdc, 
				ChooseFontHandle( 
					m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIdx].m_bFatFont, 
					m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIdx].m_bUnderLine 
				) 
			);
		}
	}
	
	return;
}


/* 行番号表示 */
void CEditView::DispLineNumber( 
		HDC						hdc,
		const CLayout*			pcLayout, 
		int						nLineNum, 
		int						y
)
{
	RECT			rcClip;
	HBRUSH			hBrush;
	COLORREF		colTextColorOld;
	COLORREF		colBkColorOld;
	char			szLineNum[18];
	int				nLineHeight = m_nCharHeight + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nLineSpace;
	int				nCharWidth = m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace;
	int				nLineCols;
	UINT			fuOptions = ETO_CLIPPED | ETO_OPAQUE;
	HPEN			hPen, hPenOld;
	int				nColorIndex;
	const CDocLine*	pCDocLine;

	nColorIndex = COLORIDX_GYOU;	/* 行番号 */
	if( NULL != pcLayout ){
//		pCDocLine = m_pcEditDoc->m_cDocLineMgr.GetLineInfo( pcLayout->m_nLinePhysical );
		pCDocLine = pcLayout->m_pCDocLine;

		if( TRUE == m_pcEditDoc->m_bIsModified /* ドキュメントが無変更の状態か */
		 && TRUE == pCDocLine->m_bModify ){	/* 変更フラグ */
//			if( 0 == pCDocLine->m_nModifyCount ){	/* 変更回数 */
				nColorIndex = COLORIDX_GYOU_MOD;	/* 行番号（変更行） */
//			}
//		}else{
//			if( /* FALSE == m_pcEditDoc->m_bIsModified && --*/ /* ドキュメントが無変更の状態か */
//				0 < pCDocLine->m_nModifyCount	/* 変更回数 */
//			){
//				nColorIndex = COLORIDX_GYOU_MODSAVE;	/* 行番号（変更&保存済） */
//			}
		}
	}

//	if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_bDispLINE ){	/* 行番号表示／非表示 */
	if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[/*nColorIndex*/COLORIDX_GYOU].m_bDisp ){	/* 行番号表示／非表示 */
		/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_bLineNumIsCRLF ){
			/* 論理行番号表示モード */
			if( NULL == pcLayout || 0 != pcLayout->m_nOffset ){
				strcpy( szLineNum, " " );
			}else{
				_itoa( pcLayout->m_nLinePhysical + 1, szLineNum, 10 );	/* 対応する論理行番号 */
			}
		}else{
			/* 物理行（レイアウト行）番号表示モード */
			_itoa( nLineNum + 1, szLineNum, 10 );
		}
		nLineCols = lstrlen( szLineNum );
		
		colTextColorOld = ::SetTextColor( hdc, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_colTEXT );	/* 行番号の色 */
		colBkColorOld = ::SetBkColor( hdc, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_colBACK );	/* 行番号背景の色 */

		HFONT	hFontOld;	
		/* フォントを選ぶ */
		hFontOld = (HFONT)::SelectObject( hdc, 
			ChooseFontHandle( 
				m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_bFatFont, 
				m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_bUnderLine 
			) 
		);
//		if( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_bFatFont ){	/* 太字か */
//			hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN_FAT );		
//		}else{
//			hFontOld = (HFONT)::SelectObject( hdc, m_hFont_HAN );		
//		}
		/* 余白を埋める */
		rcClip.left = m_nViewAlignLeft - 3;
		rcClip.right = m_nViewAlignLeft;
		rcClip.top = y;
		rcClip.bottom = y + nLineHeight;
		::ExtTextOut( hdc,
			(m_nViewAlignLeftCols - nLineCols - 1) * ( nCharWidth ),
			y, fuOptions,
			&rcClip, " ", 1, m_pnDx 
		);

		
//		/* 行番号のテキストを表示 */
//		m_pShareData->m_Types[nIdx].m_nLineTermType = 1;			/* 行番号区切り　0=なし 1=縦線 2=任意 */
//		m_pShareData->m_Types[nIdx].m_cLineTermChar = ':';			/* 行番号区切り文字 */

		/* 行番号区切り　0=なし 1=縦線 2=任意 */
		if( 2 == m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nLineTermType ){
			char szLineTerm[2];
			wsprintf( szLineTerm, "%c", m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_cLineTermChar );	/* 行番号区切り文字 */
			strcat( szLineNum, szLineTerm );
		}
		rcClip.left = 0;
		rcClip.right = m_nViewAlignLeft/* - 3*/;
		rcClip.top = y;
		rcClip.bottom = y + nLineHeight;
		::ExtTextOut( hdc,
			(m_nViewAlignLeftCols - nLineCols - 1) * ( nCharWidth ),
			y, fuOptions,
			&rcClip, szLineNum, lstrlen( szLineNum ), m_pnDx 
		);

		
//		hPen = ::CreatePen( PS_SOLID, 0, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_colorGYOU );


		/* 行番号区切り　0=なし 1=縦線 2=任意 */
		if( 1 == m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nLineTermType ){
			hPen = ::CreatePen( PS_SOLID, 0, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex].m_colTEXT );
			hPenOld = (HPEN)::SelectObject( hdc, hPen );
			::MoveToEx( hdc, m_nViewAlignLeft - 4, y, NULL );
			::LineTo( hdc, m_nViewAlignLeft - 4, y + nLineHeight );
			::SelectObject( hdc, hPenOld );
			::DeleteObject( hPen );
		}
		::SetTextColor( hdc, colTextColorOld );
		::SetBkColor( hdc, colBkColorOld );

//		colBkColorOld = ::SetBkColor( hdc, RGB( 255, 0, 0 ) );


		
		
		
		
//		::SetBkColor( hdc, colBkColorOld );


		::SelectObject( hdc, hFontOld );
	}else{
		rcClip.left = 0;
		rcClip.right = m_nViewAlignLeft;
		rcClip.top = y;
		rcClip.bottom = y + nLineHeight;
//		hBrush = ::CreateSolidBrush( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_colorBACK );
		hBrush = ::CreateSolidBrush( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[nColorIndex/*COLORIDX_TEXT*/].m_colBACK );
		::FillRect( hdc, &rcClip, hBrush );
		::DeleteObject( hBrush );
	}
	return;
}




/* テキスト表示 */
int CEditView::DispText( HDC hdc, int x, int y, const unsigned char* pData, int nLength )
{
	if( 0 >= nLength ){
		return 0;
	}
	RECT	rcClip;
	int		nLineHeight = m_nCharHeight + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nLineSpace;
	int		nCharWidth = m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace;
	UINT	fuOptions = ETO_CLIPPED | ETO_OPAQUE;
	rcClip.left = x;
	rcClip.right = rcClip.left + ( nCharWidth ) * nLength;
	rcClip.top = y;
	if( rcClip.left < m_nViewAlignLeft ){
		rcClip.left = m_nViewAlignLeft;
	}
	if( rcClip.left < rcClip.right
	 && rcClip.left < m_nViewAlignLeft + m_nViewCx && rcClip.right > m_nViewAlignLeft 
	 && rcClip.top >= m_nViewAlignTop
	){
		rcClip.bottom = y + nLineHeight;
		if( rcClip.right - rcClip.left > 3000 ){
			rcClip.right = rcClip.left + 3000;
		}
//		if( nLength > m_nViewColNum ){
//		}
		::ExtTextOut( hdc, x, y, fuOptions, &rcClip, (const char *)pData, nLength, m_pnDx );
	}
	return nLength;

}


/* テキスト反転 */
void CEditView::DispTextSelected( HDC hdc, int nLineNum, int x, int y, int nX  )
{
//	MYTRACE( "CEditView::DispTextSelected()\n" );

	int			nROP_Old;
	int			nSelectFrom;
	int			nSelectTo;
	RECT		rcClip;
//	HPEN		hPen;
//	HPEN		hPenOld;
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	int			nLineHeight = m_nCharHeight + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nLineSpace;
	int			nCharWidth = m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace;
	HRGN		hrgnDraw; 

	/* 選択範囲内の行かな */
//	if( IsTextSelected() ){
		if( nLineNum >= m_nSelectLineFrom && nLineNum <= m_nSelectLineTo ){
			if(	m_bBeginBoxSelect){		/* 矩形範囲選択中 */
				nSelectFrom = m_nSelectColmFrom;
				nSelectTo   = m_nSelectColmTo;
			}else{
				if( m_nSelectLineFrom == m_nSelectLineTo ){
						nSelectFrom = m_nSelectColmFrom;
						nSelectTo   = m_nSelectColmTo;
				}else{
					if( nLineNum == m_nSelectLineFrom	){
						nSelectFrom = m_nSelectColmFrom;
						nSelectTo   = nX;
					}else
					if( nLineNum == m_nSelectLineTo ){
						nSelectFrom = 0;
						nSelectTo   = m_nSelectColmTo;
					}else{
						nSelectFrom = 0;
						nSelectTo   = nX;
					}
				}
			}
			if(	nSelectFrom < m_nViewLeftCol ){
				nSelectFrom = m_nViewLeftCol;
			}
			if(	nSelectTo < m_nViewLeftCol ){
				nSelectTo = m_nViewLeftCol;
			}
			rcClip.left   = x + nSelectFrom * ( nCharWidth );
			rcClip.right  = x + nSelectTo   * ( nCharWidth );
			rcClip.top    = y;
			rcClip.bottom = y + nLineHeight;
			if( rcClip.right - rcClip.left > 3000 ){
				rcClip.right = rcClip.left + 3000;
			}
			hBrush = ::CreateSolidBrush( SELECTEDAREA_RGB );
			nROP_Old = ::SetROP2( hdc, SELECTEDAREA_ROP2 );
			hBrushOld = (HBRUSH)::SelectObject( hdc, hBrush );
			hrgnDraw = ::CreateRectRgn( rcClip.left, rcClip.top, rcClip.right, rcClip.bottom );
			::PaintRgn( hdc, hrgnDraw );
			::DeleteObject( hrgnDraw );
			SetROP2( hdc, nROP_Old );
			SelectObject( hdc, hBrushOld );
			DeleteObject( hBrush );
		}
//	}
	return;
}




/* 現在位置が検索文字列に該当するか */
BOOL CEditView::IsSeaechString( const char* pszData, int nDataLen, int nPos, int* pnSearchEnd  )
{
	int		nKeyLength;

//	m_bCurSrchKeyMark = TRUE;	/* 検索文字列のマーク */
//	strcpy( m_szCurSrchKey, m_pShareData->m_szSEARCHKEYArr[0] );	/* 検索文字列 */
//	m_bCurSrchRegularExp = m_pShareData->m_Common.m_bRegularExp;	/* 検索／置換  1==正規表現 */
//	m_bCurSrchLoHiCase = m_pShareData->m_Common.m_bLoHiCase;	/* 検索／置換  1==英大文字小文字の区別 */
//	m_bCurSrchWordOnly = m_pShareData->m_Common.m_bWordOnly;	/* 検索／置換  1==単語のみ検索 */

	char*	pszRes;
	if( m_bCurSrchRegularExp ){
		/* 行頭ではない? */
		if( '^' == m_szCurSrchKey[0] && 0 != nPos ){
			return FALSE;
		}
		
		
		if( ( NULL != ( pszRes = (char *)m_CurSrch_CJre.GetMatchInfo( &pszData[nPos], nDataLen - nPos, 0 ) ) ) 
		 && ( pszRes == &pszData[nPos] )
		){
			*pnSearchEnd = nPos + m_CurSrch_CJre.m_jreData.nLength;
			return TRUE;
  
		}else{
			return FALSE;
		}
	}else{
		nKeyLength = lstrlen( m_szCurSrchKey );		/* 検索条件 */
		if( 0 == nKeyLength || nKeyLength > nDataLen - nPos ){
			return FALSE;
		}
		if( m_bCurSrchLoHiCase ){	/* 1==英大文字小文字の区別 */
			if( 0 == memcmp( &pszData[nPos], m_szCurSrchKey, nKeyLength ) ){
				*pnSearchEnd = nPos + nKeyLength;
				return TRUE;
			}
		}else{
			if( 0 == memicmp( &pszData[nPos], m_szCurSrchKey, nKeyLength ) ){
				*pnSearchEnd = nPos + nKeyLength;
				return TRUE;
			}
		}
	}
	return FALSE;
}




/* ルーラー描画 */
void CEditView::DispRuler( HDC hdc )
{

#ifdef _DEBUG	
//	if( 0 != m_pShareData->m_Common.m_nRulerType ){	/* ルーラーのタイプ */
//		DispRulerEx( hdc );
//		return;
//	}
#endif

	
	if( !m_bDrawSWITCH ){
		return;
	}
	if( !m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_RULER].m_bDisp ){
		return;	
	}
	
	/* 描画処理 */
	HBRUSH		hBrush;
	HBRUSH		hBrushOld;
	HRGN		hRgn;
	RECT		rc;
	int			i;
	int			nX;
	int			nY;
	LOGFONT		lf;
	HFONT		hFont;
	HFONT		hFontOld;
	char		szColm[32];
//	SIZE		size;
	HPEN		hPen;
	HPEN		hPenOld;
	int			nROP_Old;
	COLORREF	colTextOld;
	int			nToX;
	/* ルーラーとテキストの間の余白 */
/**
	hBrush = ::CreateSolidBrush( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_TEXT].m_colBACK );
	rc.left = 0;	
	rc.top = m_nViewAlignTop - m_nTopYohaku;	
	rc.right = m_nViewAlignLeft + m_nViewCx;	
	rc.bottom = m_nViewAlignTop;	
	::FillRect( hdc, &rc, hBrush );
	::DeleteObject( hBrush );
**/


	/* LOGFONTの初期化 */
/* LOGFONTの初期化 */
	memset( &lf, 0, sizeof(LOGFONT) );
	lf.lfHeight         = -12;
	lf.lfWidth          = 5/*0*/;
	lf.lfEscapement     = 0;
	lf.lfOrientation    = 0;
	lf.lfWeight         = 400;
	lf.lfItalic         = 0;
	lf.lfUnderline      = 0;
	lf.lfStrikeOut      = 0;
	lf.lfCharSet        = 0;
	lf.lfOutPrecision   = 3;
	lf.lfClipPrecision  = 2;
	lf.lfQuality        = 1;
	lf.lfPitchAndFamily = 34;
	strcpy( lf.lfFaceName, "Arial" );
	hFont = ::CreateFontIndirect( &lf );
	hFontOld = (HFONT)::SelectObject( hdc, hFont );
	::SetBkMode( hdc, TRANSPARENT );

	hBrush = ::CreateSolidBrush( m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_RULER].m_colBACK );
	rc.left = 0;	
	rc.top = 0;	
	rc.right = m_nViewAlignLeft + m_nViewCx;	
	rc.bottom = m_nViewAlignTop - m_nTopYohaku;	
	::FillRect( hdc, &rc, hBrush );
	::DeleteObject( hBrush );

	nX = m_nViewAlignLeft;
	nY = m_nViewAlignTop - m_nTopYohaku - 2;

	hPen = ::CreatePen( PS_SOLID, 0, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_RULER].m_colTEXT );
	hPenOld = (HPEN)::SelectObject( hdc, hPen );
	colTextOld = ::SetTextColor( hdc, m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_ColorInfoArr[COLORIDX_RULER].m_colTEXT );  

	
	nToX = m_nViewAlignLeft + m_nViewCx;
	
	nToX = m_nViewAlignLeft + (m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nMaxLineSize - m_nViewLeftCol) * ( m_nCharWidth  + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace );
	if( nToX > m_nViewAlignLeft + m_nViewCx ){
		nToX = m_nViewAlignLeft + m_nViewCx;
	}

	::MoveToEx( hdc, m_nViewAlignLeft, nY + 1, NULL );
	::LineTo( hdc, nToX/*m_nViewAlignLeft + m_nViewCx*/, nY + 1 );

	
	for( i = m_nViewLeftCol; 
		i <= m_nViewLeftCol + m_nViewColNum + 1 
	 && i <= m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nMaxLineSize; 
		i++ 
	){
		if( i == m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nMaxLineSize ){
			::MoveToEx( hdc, nX, nY, NULL );
			::LineTo( hdc, nX, 0/*nY - 8*/ );
		}
		if( 0 == ( (i) % 10 ) ){
			::MoveToEx( hdc, nX, nY, NULL );
			::LineTo( hdc, nX, 0/*nY - 8*/ );
			itoa( (i) / 10, szColm, 10 );
			::TextOut( hdc, nX + 2 + 0, -1 + 0, szColm, lstrlen( szColm ) );
		}else
		if( 0 == ( (i) % 5 ) ){
			::MoveToEx( hdc, nX, nY, NULL );
			::LineTo( hdc, nX, nY - 6 );
		}else{
			::MoveToEx( hdc, nX, nY, NULL );
			::LineTo( hdc, nX, nY - 3 );
		}
		nX += ( m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace );
	}
	::SetTextColor( hdc, colTextOld );  
	::SelectObject( hdc, hPenOld );
	::DeleteObject( hPen );

	if( m_nViewLeftCol <= m_nCaretPosX
	 && m_nViewLeftCol + m_nViewColNum + 2 >= m_nCaretPosX
	){
		if( 0 == m_nCaretWidth ){
			hBrush = ::CreateSolidBrush( RGB( 128, 128, 128 ) );
		}else{
			hBrush = ::CreateSolidBrush( RGB( 0, 0, 0 ) );
		}
		rc.left = m_nViewAlignLeft + ( m_nCaretPosX - m_nViewLeftCol ) * ( m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace ) + 1;	
		rc.top = 0;	
		//	Aug. 18, 2000 あお
		rc.right = rc.left + m_nCharWidth + m_pShareData->m_Types[m_pcEditDoc->m_nSettingType].m_nColmSpace - 1;	
		rc.bottom = m_nViewAlignTop - m_nTopYohaku - 1;	
		nROP_Old = ::SetROP2( hdc, R2_NOTXORPEN );
		hRgn = ::CreateRectRgnIndirect( &rc );
		hBrushOld = (HBRUSH)::SelectObject( hdc, hBrush );
		::PaintRgn( hdc, hRgn );
		::SelectObject( hdc, hBrushOld );
		::DeleteObject( hRgn );
//		::FillRect( hdc, &rc, hBrush );
		::DeleteObject( hBrush );
		::SetROP2( hdc, nROP_Old );
	}
/***	
	rc.left = 0;	
	rc.top = 0;	
	rc.right = m_nViewAlignLeft + m_nViewCx;	
	rc.bottom = m_nViewAlignTop - m_nTopYohaku;	
	CSplitBoxWnd::Draw3dRect( 
		hdc, 
		rc.left, rc.top, rc.right, rc.bottom, 
		::GetSysColor( COLOR_3DHILIGHT ), 
		::GetSysColor( COLOR_3DSHADOW )
	);
***/
	::SelectObject( hdc, hFontOld );
	::DeleteObject( hFont );
	return;
}

//======================================================================
//	Jun. 16, 2000 genta
//
//	PosX, PosY: 検索開始点のレイアウト座標
//	NewX, NewY: 移動先のレイアウト座標
//
//	戻り値: true : 成功 / false : 失敗
//
bool CEditView::SearchBracket( int LayoutX, int LayoutY, int* NewX, int* NewY )
{
	int len;	//	行の長さ
	int nCharSize;	//	（メモリ上の）文字幅
	int PosX, PosY;	//	物理位置

	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys( LayoutX, LayoutY, &PosX, &PosY );
	const char *cline = m_pcEditDoc->m_cDocLineMgr.GetLineStr( PosY, &len );

	//	Jun. 19, 2000 genta
	if( cline == NULL )	//	最後の行に本文がない場合
		return false;	
//	PosX = LineColmnToIndex( cline, len, PosX );	不要

	nCharSize = CMemory::MemCharNext( cline, len, cline + PosX ) - cline - PosX;

	if( nCharSize == 1 ){	//	1バイト文字
//		char buf[] = "Bracket:  Forward";
//		buf[8] = cline[ PosX ];
//		::MessageBox( NULL, buf, "Bracket", MB_OK );

		switch( cline[ PosX ] ){
		case '(':	return SearchBracketForward( PosX, PosY, NewX, NewY, '(', ')' );
		case '[':	return SearchBracketForward( PosX, PosY, NewX, NewY, '[', ']' );
		case '{':	return SearchBracketForward( PosX, PosY, NewX, NewY, '{', '}' );
		case '<':	return SearchBracketForward( PosX, PosY, NewX, NewY, '<', '>' );

		case ')':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '(', ')' );
		case ']':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '[', ']' );
		case '}':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '{', '}' );
		case '>':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '<', '>' );
		}
	}

	//	括弧が見つからなかったら，カーソルの直前の文字を調べる

	if( PosX <= 0 ){
//		::MessageBox( NULL, "NO DATA", "Bracket", MB_OK );
		return false;	//	前の文字はない
	}
	const char *bPos = CMemory::MemCharPrev( cline, PosX, cline + PosX );
	nCharSize = cline + PosX - bPos;
	if( nCharSize == 1 ){	//	1バイト文字
		PosX = bPos - cline;

//		char buf[] = "Bracket:  Back";
//		buf[8] = cline[ PosX ];
//		::MessageBox( NULL, buf, "Bracket", MB_OK );

		switch( cline[ PosX ] ){
		case '(':	return SearchBracketForward( PosX, PosY, NewX, NewY, '(', ')' );
		case '[':	return SearchBracketForward( PosX, PosY, NewX, NewY, '[', ']' );
		case '{':	return SearchBracketForward( PosX, PosY, NewX, NewY, '{', '}' );
		case '<':	return SearchBracketForward( PosX, PosY, NewX, NewY, '<', '>' );

		case ')':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '(', ')' );
		case ']':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '[', ']' );
		case '}':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '{', '}' );
		case '>':	return SearchBracketBackward( PosX, PosY, NewX, NewY, '<', '>' );
		}
	}
	return false;
}

//	LayoutX, LayoutY: 検索開始点の物理座標
//	NewX, NewY: 移動先のレイアウト座標
//
//	戻り値: true : 成功 / false : 失敗
//
bool CEditView::SearchBracketForward( int PosX, int PosY, int* NewX, int* NewY,
									int upChar, int dnChar )
{
	CDocLine* ci;

	int len;
	const char *cPos, *nPos;
	char *cline, *lineend;
	int level = 0;

//	char buf[50];	Debug用

	//	初期位置の設定
	ci = m_pcEditDoc->m_cDocLineMgr.GetLineInfo( PosY );
	cline = ci->m_pLine->GetPtr( &len );
	lineend = cline + len;
	cPos = cline + PosX;

	do {
		while( cPos < lineend ){
			nPos = CMemory::MemCharNext( cline, len, cPos );
			if( nPos - cPos > 1 ){
				//	skip
				cPos = nPos;
				continue;
			}
			if( *cPos == upChar )		++level;
			else if( *cPos == dnChar )	--level;

			if( level == 0 ){	//	見つかった！
				PosX = cPos - cline;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( PosX, PosY, NewX, NewY );
//				wsprintf( buf, "Layout: %d, %d\nPhys: %d, %d", *NewX, *NewY, PosX, PosY );
//				::MessageBox( NULL, buf, "Bracket", MB_OK );
				return true;
				//	Happy Ending
			}
			cPos = nPos;	//	次の文字へ
		}

		//	次の行へ
		++PosY;
		ci = ci->m_pNext;	//	次のアイテム
		if( ci == NULL )
			break;	//	終わりに達した

		cline = ci->m_pLine->GetPtr( &len );
		cPos = cline;
		lineend = cline + len;
	}while( cline != NULL );

	return false;
}

//	LayoutX, LayoutY: 検索開始点の物理座標
//	NewX, NewY: 移動先のレイアウト座標
//
//	戻り値: true : 成功 / false : 失敗
//
bool CEditView::SearchBracketBackward( int PosX, int PosY, int* NewX, int* NewY,
									int dnChar, int upChar )
{
	CDocLine* ci;

	int len;
	const char *cPos, *pPos;
	char *cline, *lineend;
	int level = 1;

//	char buf[50];	Debug用

	//	初期位置の設定
	ci = m_pcEditDoc->m_cDocLineMgr.GetLineInfo( PosY );
	cline = ci->m_pLine->GetPtr( &len );
	lineend = cline + len;
	cPos = cline + PosX;

	do {
		while( cPos > cline ){
			pPos = CMemory::MemCharPrev( cline, len, cPos );
			if( cPos - pPos > 1 ){
				//	skip
				cPos = pPos;
				continue;
			}
			if( *pPos == upChar )		++level;
			else if( *pPos == dnChar )	--level;

			if( level == 0 ){	//	見つかった！
				PosX = pPos - cline;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( PosX, PosY, NewX, NewY );
//				wsprintf( buf, "Layout: %d, %d\nPhys: %d, %d", *NewX, *NewY, PosX, PosY );
//				::MessageBox( NULL, buf, "Bracket", MB_OK );
				return true;
				//	Happy Ending
			}
			cPos = pPos;	//	次の文字へ
		}

		//	次の行へ
		--PosY;
		ci = ci->m_pPrev;	//	次のアイテム
		if( ci == NULL )
			break;	//	終わりに達した

		cline = ci->m_pLine->GetPtr( &len );
		cPos = cline + len;
	}while( cline != NULL );

	return false;
}

//	現在のカーソル行位置を履歴に登録する
//
//
void CEditView::AddCurrentLineToHistory(void)
{
	int PosX, PosY;	//	物理位置（改行単位の計算）

	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys( m_nCaretPosX, m_nCaretPosY, &PosX, &PosY );

	CMarkMgr::CMark m( PosX, PosY );
	m_cHistory->Add( m );
	
//	char buf[256];
//	wsprintf( buf, "Line: %d, Ext: %d",m.GetLine(),m.GetExtra());
//	::MessageBox( NULL, buf, "Mark Add", MB_OK );
}

/* [EOF] */
