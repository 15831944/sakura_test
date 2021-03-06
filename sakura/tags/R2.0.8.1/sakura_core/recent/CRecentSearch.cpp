#include "StdAfx.h"
#include "CRecentSearch.h"
#include "config/maxdata.h"
#include "env/DLLSHAREDATA.h"
#include <string.h>





// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           生成                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

CRecentSearch::CRecentSearch()
{
	Create(
		GetShareData()->m_sSearchKeywords.m_aSearchKeys.dataPtr(),
		&GetShareData()->m_sSearchKeywords.m_aSearchKeys._GetSizeRef(),
		NULL,
		MAX_SEARCHKEY,
		NULL
	);
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                      オーバーライド                         //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*
	アイテムの比較要素を取得する。

	@note	取得後のポインタはユーザ管理の構造体にキャストして参照してください。
*/
const TCHAR* CRecentSearch::GetItemText( int nIndex ) const
{
	return to_tchar(*GetItem(nIndex));
}

bool CRecentSearch::DataToReceiveType( LPCWSTR* dst, const CSearchString* src ) const
{
	*dst = *src;
	return true;
}

bool CRecentSearch::TextToDataType( CSearchString* dst, LPCTSTR pszText ) const
{
	CopyItem(dst, to_wchar(pszText));
	return true;
}

int CRecentSearch::CompareItem( const CSearchString* p1, LPCWSTR p2 ) const
{
	return wcscmp(*p1,p2);
}

void CRecentSearch::CopyItem( CSearchString* dst, LPCWSTR src ) const
{
	wcscpy(*dst,src);
}
