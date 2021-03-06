/*
	2008.05.18 kobake CShareData から分離
*/

#include "StdAfx.h"
#include "DLLSHAREDATA.h"

#include "CSearchKeywordManager.h"
#include "recent/CRecent.h"


/*!	m_aSearchKeysにpszSearchKeyを追加する。
	YAZAKI
*/
void CSearchKeywordManager::AddToSearchKeyArr( const wchar_t* pszSearchKey )
{
	CRecentSearch	cRecentSearchKey;
	cRecentSearchKey.AppendItem( pszSearchKey );
	cRecentSearchKey.Terminate();
}

/*!	m_aReplaceKeysにpszReplaceKeyを追加する
	YAZAKI
*/
void CSearchKeywordManager::AddToReplaceKeyArr( const wchar_t* pszReplaceKey )
{
	CRecentReplace	cRecentReplaceKey;
	cRecentReplaceKey.AppendItem( pszReplaceKey );
	cRecentReplaceKey.Terminate();

	return;
}

/*!	m_aGrepFilesにpszGrepFileを追加する
	YAZAKI
*/
void CSearchKeywordManager::AddToGrepFileArr( const TCHAR* pszGrepFile )
{
	CRecentGrepFile	cRecentGrepFile;
	cRecentGrepFile.AppendItem( pszGrepFile );
	cRecentGrepFile.Terminate();
}

/*!	m_aGrepFolders.size()にpszGrepFolderを追加する
	YAZAKI
*/
void CSearchKeywordManager::AddToGrepFolderArr( const TCHAR* pszGrepFolder )
{
	CRecentGrepFolder	cRecentGrepFolder;
	cRecentGrepFolder.AppendItem( pszGrepFolder );
	cRecentGrepFolder.Terminate();
}
