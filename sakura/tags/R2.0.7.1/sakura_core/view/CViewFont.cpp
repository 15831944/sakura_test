#include "StdAfx.h"
#include "CViewFont.h"
#include "env/CShareData.h"
#include "env/DLLSHAREDATA.h"
#include "window/CEditWnd.h"

CViewFont::CViewFont()
{
	DLLSHAREDATA* pShareData = CShareData::getInstance()->GetShareData();

	LOGFONT	lf = CEditWnd::getInstance()->GetLogfont();

	/* フォント作成 */
	m_hFont_HAN = CreateFontIndirect( &lf );

	/* 太字フォント作成 */
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfWeight += 300;
	if( 1000 < lf.lfWeight ){
		lf.lfWeight = 1000;
	}
	m_hFont_HAN_FAT = CreateFontIndirect( &lf );

	/* 下線フォント作成 */
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfUnderline = TRUE;
	m_hFont_HAN_UL = CreateFontIndirect( &lf );

	/* 太字下線フォント作成 */
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfUnderline = TRUE;
	lf.lfWeight += 300;
	if( 1000 < lf.lfWeight ){
		lf.lfWeight = 1000;
	}
	m_hFont_HAN_FAT_UL = CreateFontIndirect( &lf );
}

CViewFont::~CViewFont()
{
	DeleteObject( m_hFont_HAN );
	DeleteObject( m_hFont_HAN_FAT );
	DeleteObject( m_hFont_HAN_UL );
	DeleteObject( m_hFont_HAN_FAT_UL );
}

void CViewFont::UpdateFont()
{
	DLLSHAREDATA* pShareData = CShareData::getInstance()->GetShareData();
	LOGFONT	lf = CEditWnd::getInstance()->GetLogfont();

	/* フォント作成 */
	::DeleteObject( m_hFont_HAN );
	m_hFont_HAN = CreateFontIndirect( &lf );

	/* 太字フォント作成 */
	::DeleteObject( m_hFont_HAN_FAT );
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfWeight += 300;
	if( 1000 < lf.lfWeight ){
		lf.lfWeight = 1000;
	}
	m_hFont_HAN_FAT = CreateFontIndirect( &lf );

	/* 下線フォント作成 */
	::DeleteObject( m_hFont_HAN_UL );
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfUnderline = TRUE;
	m_hFont_HAN_UL = CreateFontIndirect( &lf );

	/* 太字下線フォント作成 */
	::DeleteObject( m_hFont_HAN_FAT_UL );
	lf = CEditWnd::getInstance()->GetLogfont();
	lf.lfUnderline = TRUE;
	lf.lfWeight += 300;
	if( 1000 < lf.lfWeight ){
		lf.lfWeight = 1000;
	}
	m_hFont_HAN_FAT_UL = CreateFontIndirect( &lf );
}

/*! フォントを選ぶ
	@param bFat TRUEで太字
	@param bUnderLine TRUEでアンダーライン
*/
HFONT CViewFont::ChooseFontHandle( BOOL bFat, BOOL bUnderLine ) const
{
	if( bFat ){	/* 太字か */
		if( bUnderLine ){	/* 下線か */
			return m_hFont_HAN_FAT_UL;
		}else{
			return m_hFont_HAN_FAT;
		}
	}else{
		if( bUnderLine ){	/* 下線か */
			return m_hFont_HAN_UL;
		}else{
			return m_hFont_HAN;
		}
	}
}

