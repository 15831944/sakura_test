#include "stdafx.h"
#include "global.h"
#include "CShareData.h"
#include "CRecentGrepFolder.h"
#include <string.h>


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           生成                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

CRecentGrepFolder::CRecentGrepFolder()
{
	Create(
		&GetShareData()->m_aGrepFolders[0],
		&GetShareData()->m_aGrepFolders._GetSizeRef(),
		NULL,
		MAX_GREPFOLDER,
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
const TCHAR* CRecentGrepFolder::GetItemText( int nIndex ) const
{
	return *GetItem(nIndex);
}

int CRecentGrepFolder::CompareItem( const CGrepFolderString* p1, LPCTSTR p2 ) const
{
	return _tcsicmp(*p1,p2);
}

void CRecentGrepFolder::CopyItem( CGrepFolderString* dst, LPCTSTR src ) const
{
	_tcscpy(*dst,src);
}
