//	$Id$
/*!	@file
	@brief オープンダイアログ用ファイル拡張子管理

	@author MIK
	$Revision$
*/
/*
	Copyright (C) 2003, MIK

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

#ifndef	_CFILEEXT_H_
#define	_CFILEEXT_H_

#include "global.h"

class SAKURA_CORE_API CFileExt
{
public:
	CFileExt();
	~CFileExt();

	bool AppendExt( const char *pszName, const char *pszExt );
	bool AppendExtRaw( const char *pszName, const char *pszExt );
	const char *GetName( int nIndex );
	const char *GetExt( int nIndex );

	//ダイアログに渡す拡張子フィルタを取得する。(lpstrFilterに直接指定可能)
	const char *GetExtFilter( void );

	int GetCount( void ) { return m_nCount; }

protected:
	bool ConvertTypesExtToDlgExt( const char *pszSrcExt, char *pszDstExt );

private:

	typedef struct {
		char	m_szName[64];		//名前(64文字以下のはず→m_szTypeName)
		char	m_szExt[64*3+1];	//拡張子(64文字以下のはず→m_szTypeExts) なお "*." を追加するのでそれなりに必要
	} FileExtInfoTag;

	int				m_nCount;
	FileExtInfoTag	*m_puFileExtInfo;
	char			m_szFilter[4096];
};

#endif	//_CFILEEXT_H_

