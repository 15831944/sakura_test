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
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include "CKeyBind.h"
#include "debug.h"
#include "CSMacroMgr.h"// 2002/2/10 aroka
#include "CFuncLookup.h"
#include "KeyCode.h"// 2002/2/10 aroka
#include "CMemory.h"// 2002/2/10 aroka


CKeyBind::CKeyBind()
{
	return;
}


CKeyBind::~CKeyBind()
{
	return;
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
	int		nAccelArrNum;
	HACCEL	hAccel;
	int		i, j, k;

	/* 機能が割り当てられているキーの数をカウント */
	nAccelArrNum = 0;
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
					pAccelArr[k].fVirt = FNOINVERT | FVIRTKEY;;
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
	return 0;
}






/*!
	@param hInstance [in] インスタンスハンドル
	@param nKeyNameArrNum [in] 
	@param pKeyNameArr [out] 
	@param cMemList
	@param pcFuncLookup [in] 機能番号→名前の対応を取る
	@param bGetDefFuncCode [in] ON:デフォルト機能割り当てを使う/OFF:使わない

	@return 機能が割り当てられているキーストロークの数
	
	@date Oct. 31, 2001 genta 動的な機能名に対応するため引数追加
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
*/
int CKeyBind::CreateKeyBindList(
		HINSTANCE	hInstance,
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr,
		CMemory&	cMemList,
		CFuncLookup* pcFuncLookup,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	int		i;
	int		j;
	int		nValidKeys;
	char	pszStr[256];
	char	szFuncName[256];
	char	szFuncNameJapanese[256];

	nValidKeys = 0;
//	cMemList = "";
//	cMemList.SetData( "", strlen( "" ) );
	cMemList.SetDataSz( "" );
	char*	pszSHIFT = "Shift+";
	char*	pszCTRL = "Ctrl+";
	char*	pszALT = "Alt+";
//	char*	pszEQUAL = " = ";
	char*	pszTAB = "\t";

//	char*	pszCR = "\n";	//Feb. 17, 2001 JEPRO \n=0x0a=LFが行末コードになってしまうので
	char*	pszCR = "\r\n";	//\r=0x0d=CRを追加


	cMemList.AppendSz( "キー\t機能名\t関数名\t機能番号\tキーマクロ記録可/不可" );
	cMemList.AppendSz( pszCR );
	cMemList.AppendSz( "-----\t-----\t-----\t-----\t-----" );
	cMemList.AppendSz( pszCR );

	for( j = 0; j < 8; ++j ){
		for( i = 0; i < nKeyNameArrNum; ++i ){
			int iFunc = GetFuncCodeAt( pKeyNameArr[i], j, bGetDefFuncCode );

			if( 0 != iFunc ){
				nValidKeys++;
				if( j & _SHIFT ){
					cMemList.AppendSz( pszSHIFT );
				}
				if( j & _CTRL ){
					cMemList.AppendSz( pszCTRL );
				}
				if( j & _ALT ){
					cMemList.AppendSz( pszALT );
				}
				cMemList.AppendSz( pKeyNameArr[i].m_szKeyName );
//				cMemList.AppendSz( pszEQUAL );
//				cMemList.AppendSz( pszTAB );
				//	Oct. 31, 2001 genta 
				if( !pcFuncLookup->Funccode2Name(
					iFunc,
					szFuncNameJapanese, 255 )){
					strcpy( szFuncNameJapanese, "---名前が定義されていない-----" );
				}
				strcpy( szFuncName, ""/*"---unknown()--"*/ );

//				/* 機能名日本語 */
//				::LoadString(
//					hInstance,
//					pKeyNameArr[i].m_nFuncCodeArr[j],
//					 szFuncNameJapanese, 255
//				);
				cMemList.AppendSz( pszTAB );
				cMemList.AppendSz( szFuncNameJapanese );

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
				cMemList.AppendSz( pszTAB );
				cMemList.AppendSz( szFuncName );

				/* 機能番号 */
				cMemList.AppendSz( pszTAB );
				wsprintf( pszStr, "%d", iFunc );
				cMemList.AppendSz( pszStr );

				/* キーマクロに記録可能な機能かどうかを調べる */
				cMemList.AppendSz( pszTAB );
				//@@@ 2002.2.2 YAZAKI マクロをCSMacroMgrに統一
//				if( CMacro::CanFuncIsKeyMacro( pKeyNameArr[i].m_nFuncCodeArr[j] ) ){
				if( CSMacroMgr::CanFuncIsKeyMacro( iFunc ) ){
					cMemList.AppendSz( "○" );
				}else{
					cMemList.AppendSz( "×" );
				}



				cMemList.AppendSz( pszCR );
			}
		}
	}
//	delete [] pszStr;
	return nValidKeys;
}



/*! 機能に対応するキー名の取得
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
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
	char*	pszSHIFT = "Shift+";
	char*	pszCTRL = "Ctrl+";
	char*	pszALT = "Alt+";
//	cMemList.SetData( "", strlen( "" ) );
	cMemList.SetDataSz( "" );
	for( j = 0; j < 8; ++j ){
		for( i = 0; i < nKeyNameArrNum; ++i ){
			if( nFuncId == GetFuncCodeAt( pKeyNameArr[i], j, bGetDefFuncCode ) ){
				if( j & _SHIFT ){
//					cMemList.Append( pszSHIFT, strlen( pszSHIFT ) );
					cMemList.AppendSz( pszSHIFT );
				}
				if( j & _CTRL ){
//					cMemList.Append( pszCTRL, strlen( pszCTRL ) );
					cMemList.AppendSz( pszCTRL );
				}
				if( j & _ALT ){
//					cMemList.Append( pszALT, strlen( pszALT ) );
					cMemList.AppendSz( pszALT );
				}
//				cMemList.Append( pKeyNameArr[i].m_szKeyName, strlen( pKeyNameArr[i].m_szKeyName ) );
				cMemList.AppendSz( pKeyNameArr[i].m_szKeyName );
				return 1;
			}
		}
	}
	return 0;
}


/*! 機能に対応するキー名の取得(複数)
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
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
	char*	pszSHIFT = "Shift+";
	char*	pszCTRL = "Ctrl+";
	char*	pszALT = "Alt+";
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
		for( i = 0; i < nKeyNameArrNum; ++i ){
			if( nFuncId == GetFuncCodeAt( pKeyNameArr[i], j, bGetDefFuncCode ) ){
				if( j & _SHIFT ){
//					(*pppcMemList)[nAssignedKeysNum]->Append( pszSHIFT, strlen( pszSHIFT ) );
					(*pppcMemList)[nAssignedKeysNum]->AppendSz( pszSHIFT );
				}
				if( j & _CTRL ){
//					(*pppcMemList)[nAssignedKeysNum]->Append( pszCTRL, strlen( pszCTRL ) );
					(*pppcMemList)[nAssignedKeysNum]->AppendSz( pszCTRL );
				}
				if( j & _ALT ){
//					(*pppcMemList)[nAssignedKeysNum]->Append( pszALT, strlen( pszALT ) );
					(*pppcMemList)[nAssignedKeysNum]->AppendSz( pszALT );
				}
//				(*pppcMemList)[nAssignedKeysNum]->Append( pKeyNameArr[i].m_szKeyName, strlen( pKeyNameArr[i].m_szKeyName ) );
				(*pppcMemList)[nAssignedKeysNum]->AppendSz( pKeyNameArr[i].m_szKeyName );
				nAssignedKeysNum++;
			}
		}
	}
	return nAssignedKeysNum;
}


/*! メニューラベルの作成
	@date 2007.02.22 ryoji デフォルト機能割り当てに関する処理を追加
*/
char* CKeyBind::GetMenuLabel(
		HINSTANCE	hInstance,
		int			nKeyNameArrNum,
		KEYDATA*	pKeyNameArr,
		int			nFuncId,
		char*		pszLabel,
		BOOL		bKeyStr,
		BOOL		bGetDefFuncCode /* = TRUE */
)
{
	CMemory		cMemList;
//	int			i;


	if( 0 == strlen( pszLabel ) ){
		strcpy( pszLabel, "-- undefined name --" );
		::LoadString( hInstance, nFuncId, pszLabel, 255 );
	}


	/* 機能に対応するキー名を追加するか */
	if( bKeyStr ){
		/* 機能に対応するキー名の取得 */
		if( ( IDM_SELWINDOW <= nFuncId && nFuncId <= IDM_SELWINDOW + 999 )
		 || ( IDM_SELMRU <= nFuncId && nFuncId <= IDM_SELMRU + 999 )
		 || ( IDM_SELOPENFOLDER <= nFuncId && nFuncId <= IDM_SELOPENFOLDER + 999 )
		 ){
		}else{
			strcat( pszLabel, "\t" );
		}
		if( GetKeyStr( hInstance, nKeyNameArrNum, pKeyNameArr, cMemList, nFuncId, bGetDefFuncCode ) ){
			strcat( pszLabel, cMemList.GetPtr() );
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
		return 0;

	int nDefFuncCode = 0;
	if( nKeyCode == VK_F4 ){
		if( nState == _CTRL ){
			nDefFuncCode = F_FILECLOSE;	// 閉じて(無題)
			if( pShareData->m_Common.m_bDispTabWnd && !pShareData->m_Common.m_bDispTabWndMultiWin ){
				nDefFuncCode = F_WINCLOSE;	// 閉じる
			}
		}else if( nState == _ALT ){
			nDefFuncCode = F_WINCLOSE;	// 閉じる
			if( pShareData->m_Common.m_bDispTabWnd && !pShareData->m_Common.m_bDispTabWndMultiWin ){
				if( !pShareData->m_Common.m_bTab_CloseOneWin ){
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
	return 0;
};


/*[EOF]*/
