#include "StdAfx.h"
#include "CRecentGrepFile.h"
#include <string.h>
#include "env/DLLSHAREDATA.h"


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           生成                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

CRecentGrepFile::CRecentGrepFile()
{
	Create(
		GetShareData()->m_sSearchKeywords.m_aGrepFiles.dataPtr(),
		&GetShareData()->m_sSearchKeywords.m_aGrepFiles._GetSizeRef(),
		NULL,
		MAX_GREPFILE,
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
const TCHAR* CRecentGrepFile::GetItemText( int nIndex ) const
{
	return *GetItem(nIndex);
}

bool CRecentGrepFile::DataToReceiveType( LPCTSTR* dst, const CGrepFileString* src ) const
{
	*dst = *src;
	return true;
}

bool CRecentGrepFile::TextToDataType( CGrepFileString* dst, LPCTSTR pszText ) const
{
	CopyItem(dst, pszText);
	return true;
}

int CRecentGrepFile::CompareItem( const CGrepFileString* p1, LPCTSTR p2 ) const
{
	return _tcsicmp(*p1,p2);
}

void CRecentGrepFile::CopyItem( CGrepFileString* dst, LPCTSTR src ) const
{
	_tcscpy(*dst,src);
}
