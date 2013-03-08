/*!	@file
	@brief MRUリストと呼ばれるリストを管理する

	@author YAZAKI
	@date 2001/12/23  新規作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, YAZAKI
	Copyright (C) 2002, YAZAKI, Moca, genta
	Copyright (C) 2003, MIK

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include "CMRUFolder.h"
#include "CShareData.h"
#include "CMenuDrawer.h"	//	これでいいのか？
#include "CRecent.h"	//履歴の管理	//@@@ 2003.04.08 MIK
#include "etc_uty.h"
#include "my_icmp.h" // 2002/11/30 Moca 追加

/*!	コンストラクタ

	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
*/
CMRUFolder::CMRUFolder()
{
	//	初期化。
	m_pShareData = CShareData::getInstance()->GetShareData();

	//履歴の管理	//@@@ 2003.04.08 MIK
	(void)m_cRecent.EasyCreate( RECENT_FOR_FOLDER );
}

/*	デストラクタ	*/
CMRUFolder::~CMRUFolder()
{
	m_cRecent.Terminate();
}

/*!
	フォルダ履歴メニューの作成
	
	@param 追加するメニューのハンドル
	@param pCMenuDrawer [in] (out?) メニュー作成で用いるMenuDrawer
	
	@author Norio Nakantani
	@return メニューのハンドル
*/
HMENU CMRUFolder::CreateMenu( CMenuDrawer* pCMenuDrawer ) const
{
	HMENU	hMenuPopUp;
	char	szFolder2[_MAX_PATH * 2];	//	全部&でも問題ないように :-)
	TCHAR	szMenu[_MAX_PATH * 2 + 10];				//	メニューキャプション
	int		i;
	bool	bFavorite;

	hMenuPopUp = ::CreatePopupMenu();	// Jan. 29, 2002 genta
	CShareData::getInstance()->TransformFileName_MakeCache();
	for( i = 0; i < m_cRecent.GetItemCount(); ++i )
	{
		//	「共通設定」→「全般」→「ファイルの履歴MAX」を反映
		if ( i >= m_cRecent.GetViewCount() ) break;

		CShareData::getInstance()->GetTransformFileNameFast( m_cRecent.GetDataOfItem( i ), szMenu, _MAX_PATH );
		//	&を&&に置換。
		//	Jan. 19, 2002 genta
		dupamp( szMenu, szFolder2 );

		bFavorite = m_cRecent.IsFavorite( i );
		//	j >= 10 + 26 の時の考慮を省いた(に近い)がフォルダの履歴MAXを36個にしてあるので事実上OKでしょう
		wsprintf( szMenu, "&%c %s%s", 
			(i < 10) ? ('0' + i) : ('A' + i - 10), 
			(FALSE == m_pShareData->m_Common.m_sWindow.m_bMenuIcon && bFavorite) ? "★ " : "",
			szFolder2 );

		//	メニューに追加
		pCMenuDrawer->MyAppendMenu( hMenuPopUp, MF_BYPOSITION | MF_STRING, IDM_SELOPENFOLDER + i, szMenu, _T(""), TRUE,
			bFavorite ? F_FAVORITE : -1 );
	}
	return hMenuPopUp;
}

std::vector<LPCTSTR> CMRUFolder::GetPathList() const
{
	int	i;
	std::vector<LPCTSTR> ret;
	for( i = 0; i < m_cRecent.GetItemCount(); ++i ){
		//	「共通設定」→「全般」→「フォルダの履歴MAX」を反映
		if ( i >= m_cRecent.GetViewCount() ) break;
		ret.push_back(m_cRecent.GetDataOfItem(i));
	}
	return ret;
}

int CMRUFolder::Length() const
{
	return m_cRecent.GetItemCount();
}

void CMRUFolder::ClearAll()
{
	m_cRecent.DeleteAllItem();
}

/*	@brief 開いたフォルダ リストへの登録

	@date 2001.12.26  CShareData::AddOPENFOLDERListから移動した。（YAZAKI）
*/
void CMRUFolder::Add( const TCHAR* pszFolder )
{
	if( NULL == pszFolder
	 || 0 == _tcslen( pszFolder ) )
	{	//	長さが0なら排除。
		return;
	}

	(void)m_cRecent.AppendItem( pszFolder );
}

const TCHAR* CMRUFolder::GetPath(int num) const
{
	return m_cRecent.GetDataOfItem( num );
}

