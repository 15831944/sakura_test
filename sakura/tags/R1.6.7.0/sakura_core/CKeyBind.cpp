/*!	@file
	@brief キー割り当てに関するクラス

	@author Norio Nakatani
	@date 1998/03/25 新規作成
	@date 1998/05/16 クラス内にデータを持たないように変更
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, jepro, genta
	Copyright (C) 2002, YAZAKI, aroka
	Copyright (C) 2007, ryoji, genta

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include <stdio.h>
#include "CKeyBind.h"
#include "Debug.h"
#include "CSMacroMgr.h"// 2002/2/10 aroka
#include "CFuncLookup.h"
#include "mymessage.h"
#include "CMemory.h"// 2002/2/10 aroka


CKeyBind::CKeyBind()
{
}


CKeyBind::~CKeyBind()
{
}




/*! Windows アクセラレータの作成
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
*/
HACCEL CKeyBind::CreateAccerelator(
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr
)
{
	ACCEL*	pAccelArr;
	HACCEL	hAccel;
	int		i, j, k;

	// 機能が割り当てられているキーの数をカウント
	int nAccelArrNum = 0;
	for( i = 0; i < nKeyNameArrNum; ++i ){
		if( 0 != pKeyNameArr[i].m_nKeyCode ){
			for( j = 0; j < 8; ++j ){
				if( 0 != GetFuncCodeAt( pKeyNameArr[i], j ) ){
					nAccelArrNum++;
				}
			}
		}
	}
//	nAccelArrNum = nKeyNameArrNum * 8;


	if( nAccelArrNum <= 0 ){
		/* 機能割り当てがゼロ */
		return NULL;
	}
	pAccelArr = new ACCEL[nAccelArrNum];
	k = 0;
	for( i = 0; i < nKeyNameArrNum; ++i ){
		if( 0 != pKeyNameArr[i].m_nKeyCode ){
			for( j = 0; j < 8; ++j ){
				if( 0 != GetFuncCodeAt( pKeyNameArr[i], j ) ){
					pAccelArr[k].fVirt = FNOINVERT | FVIRTKEY;
					pAccelArr[k].key = pKeyNameArr[i].m_nKeyCode;
					pAccelArr[k].cmd = pKeyNameArr[i].m_nKeyCode | (((WORD)j)<<8) ;
					if( j & _SHIFT ){
						pAccelArr[k].fVirt |= FSHIFT;
					}
					if( j & _CTRL ){
						pAccelArr[k].fVirt |= FCONTROL;
					}
					if( j & _ALT ){
						pAccelArr[k].fVirt |= FALT;
					}
					k++;
				}
			}
		}
	}
	hAccel = ::CreateAcceleratorTable( pAccelArr, nAccelArrNum );
	delete [] pAccelArr;
	return hAccel;
}






/*! アクラセレータ識別子に対応するコマンド識別子を返す．
	対応するアクラセレータ識別子がない場合または機能未割り当ての場合は0を返す．

	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
*/
int CKeyBind::GetFuncCode(
		WORD		nAccelCmd,
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	int i;
	int nCmd = (int)( nAccelCmd & 0x00ff );
	int nSts = (int)( ( nAccelCmd & 0xff00 ) >> 8 );
	for( i = 0; i < nKeyNameArrNum; ++i ){
		if( nCmd == pKeyNameArr[i].m_nKeyCode ){
			return GetFuncCodeAt( pKeyNameArr[i], nSts, bGetDefFuncCode );
		}
	}
	return F_DEFAULT;
}






/*!
	@return 機能が割り当てられているキーストロークの数
	
	@date Oct. 31, 2001 genta 動的な機能名に対応するため引数追加
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
*/
int CKeyBind::CreateKeyBindList(
	HINSTANCE		hInstance,		//!< [in] インスタンスハンドル
	int				nKeyNameArrNum,	//!< [in]
	KEYDATA*		pKeyNameArr,	//!< [out]
	CMemory&		cMemList,		//!<
	CFuncLookup*	pcFuncLookup,	//!< [in] 機能番号→名前の対応を取る
	BOOL			bGetDefFuncCode //!< [in] ON:デフォルト機能割り当てを使う/OFF:使わない デフォルト:TRUE
)
{
	int		i;
	int		j;
	int		nValidKeys;
	TCHAR	pszStr[256];
	TCHAR	szFuncName[256];
	TCHAR	szFuncNameJapanese[256];

	nValidKeys = 0;
//	cMemList = "";
//	cMemList.SetData( "", strlen( "" ) );
	cMemList.SetString(_T(""));
	const TCHAR*	pszSHIFT = _T("Shift+");
	const TCHAR*	pszCTRL = _T("Ctrl+");
	const TCHAR*	pszALT = _T("Alt+");
//	char*	pszEQUAL = " = ";
	const TCHAR*	pszTAB = _T("\t");

//	char*	pszCR = "\n";	//Feb. 17, 2001 JEPRO \n=0x0a=LFが行末コードになってしまうので
	const TCHAR*	pszCR = _T("\r\n");	//\r=0x0d=CRを追加


	cMemList.AppendString( _T("キー\t機能名\t関数名\t機能番号\tキーマクロ記録可/不可") );
	cMemList.AppendString( pszCR );
	cMemList.AppendString( _T("-----\t-----\t-----\t-----\t-----") );
	cMemList.AppendString( pszCR );

	for( j = 0; j < 8; ++j ){
		for( i = 0; i < nKeyNameArrNum; ++i ){
			int iFunc = GetFuncCodeAt( pKeyNameArr[i], j, bGetDefFuncCode );

			if( 0 != iFunc ){
				nValidKeys++;
				if( j & _SHIFT ){
					cMemList.AppendString( pszSHIFT );
				}
				if( j & _CTRL ){
					cMemList.AppendString( pszCTRL );
				}
				if( j & _ALT ){
					cMemList.AppendString( pszALT );
				}
				cMemList.AppendString( pKeyNameArr[i].m_szKeyName );
				//	Oct. 31, 2001 genta 
				if( !pcFuncLookup->Funccode2Name(
					iFunc,
					szFuncNameJapanese, 255 )){
					strcpy( szFuncNameJapanese, _T("---名前が定義されていない-----") );
				}
				strcpy( szFuncName, _T("")/*"---unknown()--"*/ );

//				/* 機能名日本語 */
//				::LoadString(
//					hInstance,
//					pKeyNameArr[i].m_nFuncCodeArr[j],
//					 szFuncNameJapanese, 255
//				);
				cMemList.AppendString( pszTAB );
				cMemList.AppendString( szFuncNameJapanese );

				/* 機能ID→関数名，機能名日本語 */
				//@@@ 2002.2.2 YAZAKI マクロをCSMacroMgrに統一
//				CMacro::GetFuncInfoByID(
				CSMacroMgr::GetFuncInfoByID(
					hInstance,
					iFunc,
					szFuncName,
					szFuncNameJapanese
				);

				/* 関数名 */
				cMemList.AppendString( pszTAB );
				cMemList.AppendString( szFuncName );

				/* 機能番号 */
				cMemList.AppendString( pszTAB );
				wsprintf( pszStr, _T("%d"), iFunc );
				cMemList.AppendString( pszStr );

				/* キーマクロに記録可能な機能かどうかを調べる */
				cMemList.AppendString( pszTAB );
				//@@@ 2002.2.2 YAZAKI マクロをCSMacroMgrに統一
//				if( CMacro::CanFuncIsKeyMacro( pKeyNameArr[i].m_nFuncCodeArr[j] ) ){
				if( CSMacroMgr::CanFuncIsKeyMacro( iFunc ) ){
					cMemList.AppendString( _T("○") );
				}else{
					cMemList.AppendString( _T("×") );
				}



				cMemList.AppendString( pszCR );
			}
		}
	}
//	delete [] pszStr;
	return nValidKeys;
}

/** 機能に対応するキー名のサーチ(補助関数)

	与えられたシフト状態に対して，指定された範囲のキーエリアから
	当該機能に対応するキーがあるかを調べ，見つかったら
	対応するキー文字列をセットする．
	
	関数から出るときには検索開始位置(nKeyNameArrBegin)に
	次に処理するindexを設定する．

	@param[in,out] nKeyNameArrBegin 調査開始INDEX (終了時には次回の開始INDEXに書き換えられる)
	@param[in] nKeyNameArrBegin 調査終了INDEX + 1
	@param[in] pKeyNameArr キー配列
	@param[in] nShiftState シフト状態
	@param[out] cMemList キー文字列設定先
	@param[in]	nFuncId 検索対象機能ID
	@param[in]	bGetDefFuncCode 標準機能を取得するかどうか
*/
bool CKeyBind::GetKeyStrSub(
		int&		nKeyNameArrBegin,
		int			nKeyNameArrEnd,
		KEYDATA*	pKeyNameArr,
		int			nShiftState,
		CMemory&	cMemList,
		int			nFuncId,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	const TCHAR*	pszSHIFT = _T("Shift+");
	const TCHAR*	pszCTRL  = _T("Ctrl+");
	const TCHAR*	pszALT   = _T("Alt+");

	int i;
	for( i = nKeyNameArrBegin; i < nKeyNameArrEnd; ++i ){
		if( nFuncId == GetFuncCodeAt( pKeyNameArr[i], nShiftState, bGetDefFuncCode ) ){
			if( nShiftState & _SHIFT ){
				cMemList.AppendString( pszSHIFT );
			}
			if( nShiftState & _CTRL ){
				cMemList.AppendString( pszCTRL );
			}
			if( nShiftState & _ALT ){
				cMemList.AppendString( pszALT );
			}
			cMemList.AppendString( pKeyNameArr[i].m_szKeyName );
			nKeyNameArrBegin = i + 1;
			return true;
		}
	}
	nKeyNameArrBegin = i;
	return false;
}


/** 機能に対応するキー名の取得
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
	@date 2007.11.04 genta マウスクリックよりキー割り当ての優先度を上げる
	@date 2007.11.04 genta 共通機能のサブルーチン化
*/
int CKeyBind::GetKeyStr(
		HINSTANCE	hInstance,
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr,
		CMemory&	cMemList,
		int			nFuncId,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	int		i;
	int		j;
	cMemList.SetString(_T(""));

	//	先にキー部分を調査する
	for( j = 0; j < 8; ++j ){
		for( i = MOUSEFUNCTION_KEYBEGIN; i < nKeyNameArrNum; /* 1を加えてはいけない */ ){
			if( GetKeyStrSub( i, nKeyNameArrNum, pKeyNameArr, j, cMemList, nFuncId, bGetDefFuncCode )){
				return 1;
			}
		}
	}

	//	後にマウス部分を調査する
	for( j = 0; j < 8; ++j ){
		for( i = 0; i < MOUSEFUNCTION_KEYBEGIN; /* 1を加えてはいけない */ ){
			if( GetKeyStrSub( i, nKeyNameArrNum, pKeyNameArr, j, cMemList, nFuncId, bGetDefFuncCode )){
				return 1;
			}
		}
	}
	return 0;
}


/** 機能に対応するキー名の取得(複数)
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
	@date 2007.11.04 genta 共通機能のサブルーチン化
*/
int CKeyBind::GetKeyStrList(
	HINSTANCE	hInstance,
	int			nKeyNameArrNum,
	KEYDATA*	pKeyNameArr,
	CMemory***	pppcMemList,
	int			nFuncId,
	BOOL		bGetDefFuncCode /* = TRUE */
)
{
	int		i;
	int		j;
	const TCHAR*	pszSHIFT = _T("Shift+");
	const TCHAR*	pszCTRL = _T("Ctrl+");
	const TCHAR*	pszALT = _T("Alt+");
	int		nAssignedKeysNum;

	nAssignedKeysNum = 0;
	if( 0 == nFuncId ){
		return 0;
	}
	for( j = 0; j < 8; ++j ){
		for( i = 0; i < nKeyNameArrNum; ++i ){
			if( nFuncId == GetFuncCodeAt( pKeyNameArr[i], j, bGetDefFuncCode ) ){
				nAssignedKeysNum++;
			}
		}
	}
	if( 0 == nAssignedKeysNum ){
		return 0;
	}
	(*pppcMemList) = new CMemory*[nAssignedKeysNum + 1];
	for( i = 0; i < nAssignedKeysNum; ++i ){
		(*pppcMemList)[i] = new CMemory;
	}
	(*pppcMemList)[i] = NULL;


	nAssignedKeysNum = 0;
	for( j = 0; j < 8; ++j ){
		for( i = 0; i < nKeyNameArrNum; /* 1を加えてはいけない */ ){
			//	2007.11.04 genta 共通機能のサブルーチン化
			if( GetKeyStrSub( i, nKeyNameArrNum, pKeyNameArr, j,
					*((*pppcMemList)[nAssignedKeysNum]), nFuncId, bGetDefFuncCode )){
				nAssignedKeysNum++;
			}
		}
	}
	return nAssignedKeysNum;
}




// アクセスキー付きの文字列の作成
TCHAR*	CKeyBind::MakeMenuLabel(const TCHAR* sName, const TCHAR* sKey)
{
	static	TCHAR	sLabel[300];
	const	TCHAR*	p;

	if (sKey == NULL || sKey[0] == _T('\0')) {
		return const_cast<TCHAR*>( sName );
	}
	else {
		if( (p = _tcschr( sName, _T('(') )) != NULL
			  && (p = _tcschr( p, sKey[0] )) != NULL) {
			// (付その後にアクセスキー
			_tcsncpy( sLabel, sName, _countof(sLabel) );
			sLabel[p-sName] = _T('&');
			_tcsncpy( sLabel + (p-sName) + 1, p, _countof(sLabel) );
		}
		else if (_tcscmp( sName + _tcslen(sName) - 3, _T("...") ) == 0) {
			// 末尾...
			_tcsncpy( sLabel, sName, _countof(sLabel) );
			sLabel[_tcslen(sName) - 3] = '\0';						// 末尾の...を取る
			_tcsncat( sLabel, _T("(&"), _countof(sLabel) );
			_tcsncat( sLabel, sKey, _countof(sLabel) );
			_tcsncat( sLabel, _T(")..."), _countof(sLabel) );
		}
		else {
			_stprintf( sLabel, _T("%s(&%s)"), sName, sKey );
		}

		return sLabel;
	}
}

/*! メニューラベルの作成
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
	2010/5/17	アクセスキーの追加
*/
TCHAR* CKeyBind::GetMenuLabel(
		HINSTANCE	hInstance,
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr,
		int			nFuncId,
		TCHAR*		pszLabel,
		const TCHAR*	pszKey,
		BOOL		bKeyStr,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	const int LABEL_MAX = 256;

	if( _T('\0') == pszLabel[0] ){
		::LoadString( hInstance, nFuncId, pszLabel, LABEL_MAX );
	}
	if( _T('\0') == pszLabel[0] ){
		_tcscpy( pszLabel, _T("-- undefined name --") );
	}
	// アクセスキーの追加	2010/5/17 Uchi
	_tcsncpy( pszLabel, MakeMenuLabel( pszLabel, pszKey ), LABEL_MAX);

	/* 機能に対応するキー名を追加するか */
	if( bKeyStr ){
		CMemory		cMemList;
		/* 機能に対応するキー名の取得 */
		if( ( IDM_SELWINDOW <= nFuncId && nFuncId <= IDM_SELWINDOW + 999 )
		 || ( IDM_SELMRU <= nFuncId && nFuncId <= IDM_SELMRU + 999 )
		 || ( IDM_SELOPENFOLDER <= nFuncId && nFuncId <= IDM_SELOPENFOLDER + 999 )
		 ){
		}else{
			_tcscat( pszLabel, _T("\t") );
		}
		if( GetKeyStr( hInstance, nKeyNameArrNum, pKeyNameArr, cMemList, nFuncId, bGetDefFuncCode ) ){
			_tcscat( pszLabel, cMemList.GetStringPtr() );
		}
	}
	return pszLabel;
}


/*! キーのデフォルト機能を取得する

	@param nKeyCode [in] キーコード
	@param nState [in] Shift,Ctrl,Altキー状態

	@return 機能番号

	@date 2007.02.22 ryoji 新規作成
*/
int CKeyBind::GetDefFuncCode( int nKeyCode, int nState )
{
	DLLSHAREDATA* pShareData = CShareData::getInstance()->GetShareData();
	if( pShareData == NULL )
		return F_DEFAULT;

	int nDefFuncCode = F_DEFAULT;
	if( nKeyCode == VK_F4 ){
		if( nState == _CTRL ){
			nDefFuncCode = F_FILECLOSE;	// 閉じて(無題)
			if( pShareData->m_Common.m_sTabBar.m_bDispTabWnd && !pShareData->m_Common.m_sTabBar.m_bDispTabWndMultiWin ){
				nDefFuncCode = F_WINCLOSE;	// 閉じる
			}
		}
		else if( nState == _ALT ){
			nDefFuncCode = F_WINCLOSE;	// 閉じる
			if( pShareData->m_Common.m_sTabBar.m_bDispTabWnd && !pShareData->m_Common.m_sTabBar.m_bDispTabWndMultiWin ){
				if( !pShareData->m_Common.m_sTabBar.m_bTab_CloseOneWin ){
					nDefFuncCode = F_GROUPCLOSE;	// グループを閉じる	// 2007.06.20 ryoji
				}
			}
		}
	}
	return nDefFuncCode;
}


/*! 特定のキー情報から機能コードを取得する

	@param KeyData [in] キー情報
	@param nState [in] Shift,Ctrl,Altキー状態
	@param bGetDefFuncCode [in] デフォルト機能を取得するかどうか

	@return 機能番号

	@date 2007.03.07 ryoji インライン関数から通常の関数に変更（BCCの最適化バグ対策）
*/
int CKeyBind::GetFuncCodeAt( KEYDATA& KeyData, int nState, BOOL bGetDefFuncCode )
{
	if( 0 != KeyData.m_nFuncCodeArr[nState] )
		return KeyData.m_nFuncCodeArr[nState];
	if( bGetDefFuncCode )
		return GetDefFuncCode( KeyData.m_nKeyCode, nState );
	return F_DEFAULT;
}


/*[EOF]*/
