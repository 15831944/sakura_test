/*!	@file
	@brief 最近使ったリスト

	お気に入りを含む最近使ったリストを管理する。

	@author MIK
	@date Apr. 05, 2003
	@date Apr. 03, 2005
*/
/*
	Copyright (C) 2003, MIK
	Copyright (C) 2005, MIK

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

#ifndef	_CRECENT_H_
#define	_CRECENT_H_

#include "global.h"

class CRecent {
public:
	CRecent();
	virtual ~CRecent();

	bool IsAvailable( void ) const;

	//初期処理
	bool Create( 
			char	*pszItemArray,	//アイテム配列へのポインタ
			int		*pnItemCount,	//アイテム個数へのポインタ
			bool	*pbItemFavorite,	//お気に入りへのポインタ
			int		nArrayCount, 
			int		*nViewCount, 
			int		nItemSize, 
			int		nOffset, 
			int		nCmpSize
		);

	void Terminate( void );

	bool ChangeViewCount( int nViewCount );	//表示数の変更
	bool UpdateView( void );

	//アイテム制御系
	bool AppendItem( const char *pszData );	//アイテムを先頭に追加
	const char *GetItem( int nIndex ) const;		//アイテムを取得
	const char *GetDataOfItem( int nIndex ) const;	//アイテムの比較要素を取得
	bool DeleteItem( int nIndex );			//アイテムをクリア
	bool DeleteItem( const char *pszItemData ) { return DeleteItem( FindItem( pszItemData ) ); }
	void DeleteAllItem( void );				//アイテムをすべてクリア
	virtual int FindItem( const char *pszItemData ) const = 0;
	bool MoveItem( int nSrcIndex, int nDstIndex );	//アイテムを移動

	//お気に入り制御系
	bool SetFavorite( int nIndex, bool bFavorite );	//お気に入りに設定
	bool SetFavorite( int nIndex ) { return SetFavorite( nIndex, true ); }	//お気に入りに設定
	bool ResetFavorite( int nIndex ) { return SetFavorite( nIndex, false ); }	//お気に入りを解除
	void ResetAllFavorite( void );			//お気に入りをすべて解除
	bool IsFavorite( int nIndex ) const;			//お気に入りか調べる

	//プロパティ取得系
	int GetArrayCount( void ) { return m_nArrayCount; }	//最大要素数
	int GetItemCount( void ) const { return ( IsAvailable() ? *m_pnUserItemCount : 0); }	//登録アイテム数
	int GetViewCount( void ) const { return ( IsAvailable() ? (m_pnUserViewCount ? *m_pnUserViewCount : m_nArrayCount) : 0); }	//表示数
	//int GetItemSize( void ) { return m_nItemSize; }		//アイテムサイズ
	//int GetCmpType( void ) { return m_nCmpType; }		//比較タイプ
	int GetOffset( void ) { return m_nOffset; }		//比較位置
	//int GetCmpSize( void ) { return m_nCmpSize; }		//比較サイズ

protected:
	//	共有メモリアクセス用。
	struct DLLSHAREDATA*	m_pShareData;		//	共有メモリを参照するよ。


protected:
	bool	m_bCreate;		//Create済みか

	int		m_nArrayCount;	//配列個数
	//int		m_nItemCount;	//有効個数(実際にデータが入っている個数) → *m_pnUserItemCount
	//int		m_nViewCount;	//表示個数
	int		m_nItemSize;	//データサイズ
	int		m_nCmpType;		//比較タイプ(0=strcmp,1=stricmp)
	int		m_nOffset;		//比較位置
	int		m_nCmpSize;		//比較サイズ

	char	*m_puUserItemData;	//アイテムデータ
	int		*m_pnUserItemCount;
	bool	*m_pbUserItemFavorite;
	int		*m_pnUserViewCount;

	void ZeroItem( int nIndex );	//アイテムをゼロクリアする
	int GetOldestItem( int nIndex, bool bFavorite );	//最古のアイテムを探す
	char *GetArrayOffset( int nIndex ) const { return m_puUserItemData + (nIndex * m_nItemSize); }
	bool CopyItem( int nSrcIndex, int nDstIndex );
};

#include "CRecentFile.h"
#include "CRecentFolder.h"
#include "CRecentSearch.h"
#include "CRecentReplace.h"
#include "CRecentGrepFile.h"
#include "CRecentGrepFolder.h"
#include "CRecentCmd.h"
#include "CRecentEditNode.h"
#include "CRecentTagjumpKeyword.h"

#endif	//_CRECENT_H_

/*[EOF]*/
