/*!	@file
	@brief 文書関連情報の管理

	@author Norio Nakatani
	@date	1998/03/13 作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2001, genta, YAZAKI, jepro, novice, asa-o, MIK,
	Copyright (C) 2002, YAZAKI, hor, genta, aroka, frozen, Moca, MIK
	Copyright (C) 2003, MIK, genta, ryoji, Moca, zenryaku, naoh, wmlhq
	Copyright (C) 2004, genta, novice, Moca, MIK, zenryaku
	Copyright (C) 2005, genta, naoh, FILE, Moca, ryoji, D.S.Koba, aroka
	Copyright (C) 2006, genta, ryoji, aroka, じゅうじ
	Copyright (C) 2007, ryoji, maru, genta, kobake
	Copyright (C) 2008, ryoji, nasukoji, bosagami, novice, aroka
	Copyright (C) 2009, nasukoji, syat, aroka
	Copyright (C) 2010, ryoji, Moca
	Copyright (C) 2011, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "StdAfx.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>	// Apr. 03, 2003 genta
#include <dlgs.h>
#include <io.h>
#include <cderr.h> // Nov. 3, 2005 genta
#include "CEditDoc.h"
#include "Debug.h"
#include "Funccode.h"
#include "CRunningTimer.h"
#include "charcode.h"
#include "mymessage.h"
#include "CWaitCursor.h"
#include "CShareData.h"
#include "CEditWnd.h"
#include "etc_uty.h"
#include "global.h"
#include "CFuncInfoArr.h" /// 2002/2/3 aroka
#include "CSMacroMgr.h"///
#include "CMarkMgr.h"///
#include "CDocLine.h" /// 2002/2/3 aroka
#include "CPrintPreview.h"
#include "CDlgFileUpdateQuery.h"
#include <assert.h> /// 2002/11/2 frozen
#include "my_icmp.h" // 2002/11/30 Moca 追加
#include "MY_SP.h" // 2005/11/22 aroka 追加
#include "CLayout.h"	// 2007.08.22 ryoji 追加
#include "CMemoryIterator.h"	// 2007.08.22 ryoji 追加
#include "sakura_rc.h"

#define IDT_ROLLMOUSE	1

/*!
	@note
		m_pcEditWnd はコンストラクタ内では使用しないこと．

	@date 2000.05.12 genta 初期化方法変更
	@date 2002.2.17 YAZAKI CShareDataのインスタンスは、CProcessにひとつあるのみ。
	@date 2002.01.14 YAZAKI 印刷プレビューをCPrintPreviewに独立させたことによる変更
	@date 2004.06.21 novice タグジャンプ機能追加
*/
CEditDoc::CEditDoc()
: m_cSaveLineCode( EOL_NONE )		//	保存時のLine Type
, m_bGrepRunning( FALSE )		/* Grep処理中 */
, m_nCommandExecNum( 0 )			/* コマンド実行回数 */
, m_bReadOnly( false )			/* 読み取り専用モード */
, m_bDebugMode( false )			/* デバッグモニタモード */
, m_bGrepMode( false )			/* Grepモードか */
, m_nActivePaneIndex( 0 )
, m_bDoing_UndoRedo( FALSE )		/* アンドゥ・リドゥの実行中か */
, m_nFileShareModeOld( 0 )		/* ファイルの排他制御モード */
, m_hLockedFile( NULL )			/* ロックしているファイルのハンドル */
, m_hInstance( NULL )
, m_hWnd( NULL )
, m_eWatchUpdate( CEditDoc::WU_QUERY )
, m_nSettingTypeLocked( false )	//	設定値変更可能フラグ
, m_nSettingType( 0 )	// Sep. 11, 2002 genta
, m_bInsMode( true )	// Oct. 2, 2005 genta
, m_bIsModified( false )	/* 変更フラグ */ // Jan. 22, 2002 genta 型変更
, m_pcDragSourceView( NULL )
{
	MY_RUNNINGTIMER( cRunningTimer, "CEditDoc::CEditDoc" );

	m_szFilePath[0] = _T('\0');			/* 現在編集中のファイルのパス */
	m_szSaveFilePath[0] = _T('\0');		/* 保存時のファイルのパス（マクロ用） */	// 2006.09.04 ryoji
	m_szGrepKey[0] = _T('\0');

	/* 共有データ構造体のアドレスを返す */
	m_pShareData = CShareData::getInstance()->GetShareData();

	for( int i = 0; i < _countof(m_pcEditViewArr); i++ ){
		m_pcEditViewArr[i] = NULL;
	}
	// 今のところ最大値は固定
	m_nEditViewMaxCount = _countof(m_pcEditViewArr);
	m_nEditViewCount = 1;
	// [0] - [3] まで作成・初期化していたものを[0]だけ作る。ほかは分割されるまで何もしない
	m_pcEditViewArr[0] = new CEditView();

	/* レイアウト管理情報の初期化 */
	m_cLayoutMgr.Create( this, &m_cDocLineMgr );
	// レイアウト情報の変更
	// 2008.06.07 nasukoji	折り返し方法の追加に対応
	// 「指定桁で折り返す」以外の時は折り返し幅をMAXLINEKETASで初期化する
	// 「右端で折り返す」は、この後のOnSize()で再設定される
	STypeConfig ref = GetDocumentAttribute();
	if( ref.m_nTextWrapMethod != WRAP_SETTING_WIDTH ){
		ref.m_nMaxLineKetas = MAXLINEKETAS;
	}	
	m_cLayoutMgr.SetLayoutInfo(
		TRUE,
		NULL,
		ref
	);

	//	自動保存の設定	//	Aug, 21, 2000 genta
	ReloadAutoSaveParam();

	//	Sep, 29, 2001 genta
	//	マクロ
	m_pcSMacroMgr = new CSMacroMgr;
	
	//	m_FileTimeの初期化
	m_FileTime.ClearFILETIME();

	//	Oct. 2, 2005 genta 挿入モード
	SetInsMode( m_pShareData->m_Common.m_sGeneral.m_bIsINSMode );

	// 2008.06.07 nasukoji	テキストの折り返し方法を初期化
	m_nTextWrapMethodCur = GetDocumentAttribute().m_nTextWrapMethod;	// 折り返し方法
	m_bTextWrapMethodCurTemp = false;									// 一時設定適用中を解除

	// 文字コード種別を初期化
	m_nCharCode = m_pShareData->m_Types[0].m_eDefaultCodetype;
	m_bBomExist = m_pShareData->m_Types[0].m_bDefaultBom;
	SetNewLineCode( static_cast<EEolType>(m_pShareData->m_Types[0].m_eDefaultEoltype) );
}


CEditDoc::~CEditDoc()
{
	for( int i = 0; i < m_nEditViewMaxCount; i++ ){
		delete m_pcEditViewArr[i];
		m_pcEditViewArr[i] = NULL;
	}

	if( m_hWnd != NULL ){
		DestroyWindow( m_hWnd );
	}
	/* ファイルの排他ロック解除 */
	delete m_pcSMacroMgr;
	DoFileUnlock();
}


void CEditDoc::Clear()
{
	// ファイルの排他ロック解除
	DoFileUnlock();

	// ファイルの排他制御モード
	m_nFileShareModeOld = 0;

	// アンドゥ・リドゥバッファのクリア
	m_cOpeBuf.ClearAll();

	// テキストデータのクリア
	m_cDocLineMgr.Empty();
	m_cDocLineMgr.Init();

	// ファイルパスとアイコンのクリア
	SetFilePathAndIcon( _T("") );

	// ファイルのタイムスタンプのクリア
	m_FileTime.ClearFILETIME();

	int doctype = CShareData::getInstance()->GetDocumentTypeOfPath( GetFilePath() );
	SetDocumentType( doctype, true );

	// レイアウト管理情報の初期化
	STypeConfig& ref = GetDocumentAttribute();
	m_cLayoutMgr.SetLayoutInfo(
		TRUE,
		NULL,
		ref
	);
}

/* 既存データのクリア */
void CEditDoc::InitDoc()
{
	m_bReadOnly = false;	// 読み取り専用モード
	m_szGrepKey[0] = _T('\0');

	m_bGrepMode = false;	/* Grepモード */
	m_eWatchUpdate = WU_QUERY; // Dec. 4, 2002 genta 更新監視方法

	// 2005.06.24 Moca バグ修正
	//	アウトプットウィンドウで「閉じて(無題)」を行ってもアウトプットウィンドウのまま
	if( m_bDebugMode ){
		m_pcEditWnd->SetDebugModeOFF();
	}

//	Sep. 10, 2002 genta
//	アイコン設定はファイル名設定と一体化のためここからは削除

	Clear();

	/* 変更フラグ */
	SetModified(false,false);	//	Jan. 22, 2002 genta

	/* 文字コード種別 */
	m_nCharCode = m_pShareData->m_Types[0].m_eDefaultCodetype;
	m_bBomExist = m_pShareData->m_Types[0].m_bDefaultBom;
	SetNewLineCode( static_cast<EEolType>(m_pShareData->m_Types[0].m_eDefaultEoltype) );

	//	Oct. 2, 2005 genta 挿入モード
	SetInsMode( m_pShareData->m_Common.m_sGeneral.m_bIsINSMode );
}


/* 全ビューの初期化：ファイルオープン/クローズ時等に、ビューを初期化する */
void CEditDoc::InitAllView( void )
{
	int		i;

	m_nCommandExecNum = 0;	/* コマンド実行回数 */

	// 2008.05.30 nasukoji	テキストの折り返し方法を初期化
	m_nTextWrapMethodCur = GetDocumentAttribute().m_nTextWrapMethod;	// 折り返し方法
	m_bTextWrapMethodCurTemp = false;									// 一時設定適用中を解除

	// 2009.08.28 nasukoji	「折り返さない」ならテキスト最大幅を算出、それ以外は変数をクリア
	if( m_nTextWrapMethodCur == WRAP_NO_TEXT_WRAP )
		m_cLayoutMgr.CalculateTextWidth();		// テキスト最大幅を算出する
	else
		m_cLayoutMgr.ClearLayoutLineWidth();	// 各行のレイアウト行長の記憶をクリアする

	/* 先頭へカーソルを移動 */
	for( i = 0; i < GetAllViewCount(); ++i ){
		//	Apr. 1, 2001 genta
		// 移動履歴の消去
		m_pcEditViewArr[i]->m_cHistory->Flush();

		/* 現在の選択範囲を非選択状態に戻す */
		m_pcEditViewArr[i]->DisableSelectArea( false );

		m_pcEditViewArr[i]->OnChangeSetting();
		m_pcEditViewArr[i]->MoveCursor( 0, 0, true );
		m_pcEditViewArr[i]->m_nCaretPosX_Prev = 0;
	}

	return;
}



/////////////////////////////////////////////////////////////////////////////
//
//	CEditDoc::Create
//	BOOL Create(HINSTANCE hInstance, HWND hwndParent)
//
//	説明
//	  ウィンドウの作成等
//
//	@date Sep. 29, 2001 genta マクロクラスを渡すように
//	@date 2002.01.03 YAZAKI m_tbMyButtonなどをCShareDataからCMenuDrawerへ移動したことによる修正。
/////////////////////////////////////////////////////////////////////////////
BOOL CEditDoc::Create(
	HINSTANCE hInstance,
	HWND hwndParent,
	CImageListMgr* pcIcons
 )
{
	MY_RUNNINGTIMER( cRunningTimer, "CEditDoc::Create" );

	HWND		hWndArr[2];
	CEditWnd*	pCEditWnd;
	m_hInstance = hInstance;
	m_hwndParent = hwndParent;

	/* 分割フレーム作成 */
	pCEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta
	m_cSplitterWnd.Create( m_hInstance, m_hwndParent, pCEditWnd );

	/* ビュー */
	m_pcEditViewArr[0]->Create( m_hInstance, m_cSplitterWnd.m_hWnd, this, 0, /*FALSE,*/ TRUE  );
	m_pcEditViewArr[0]->OnSetFocus();

	/* 子ウィンドウの設定 */
	hWndArr[0] = m_pcEditViewArr[0]->m_hWnd;
	hWndArr[1] = NULL;
	m_cSplitterWnd.SetChildWndArr( hWndArr );
	m_hWnd = m_cSplitterWnd.m_hWnd;

	MY_TRACETIME( cRunningTimer, "View created" );

	//	Oct. 2, 2001 genta
	m_cFuncLookup.Init( m_hInstance, m_pShareData->m_Common.m_sMacro.m_MacroTable, &m_pShareData->m_Common );

	/* 設定プロパティシートの初期化１ */
	m_cPropCommon.Create( m_hInstance, m_hWnd, pcIcons, m_pcSMacroMgr, &(pCEditWnd->m_CMenuDrawer) );
	m_cPropTypes.Create( m_hInstance, m_hWnd );

	MY_TRACETIME( cRunningTimer, "End: PropSheet" );

	return TRUE;
}




/*!
	ファイル名の設定
	
	ファイル名を設定すると同時に，ウィンドウアイコンを適切に設定する．
	
	@param szFile [in] ファイルのパス名
	
	@author genta
	@date 2002.09.09
*/
void CEditDoc::SetFilePathAndIcon(const TCHAR* szFile)
{
	_tcscpy( m_szFilePath, szFile );
	SetDocumentIcon();
}


//	From Here Aug. 14, 2000 genta
//
//	書き換えが禁止されているかどうか
//	戻り値: true: 禁止 / false: 許可
//
bool CEditDoc::IsModificationForbidden( int nCommand )
{
	//	編集可能の場合
	if( IsEditable() )
		return false; // 常に書き換え許可

	//	上書き禁止モードの場合
	//	暫定Case文: 実際にはもっと効率の良い方法を使うべき
	switch( nCommand ){
	//	ファイルを書き換えるコマンドは使用禁止
	case F_CHAR:
	case F_IME_CHAR:
	case F_DELETE:
	case F_DELETE_BACK:
	case F_WordDeleteToEnd:
	case F_WordDeleteToStart:
	case F_WordDelete:
	case F_WordCut:
	case F_LineDeleteToStart:
	case F_LineDeleteToEnd:
	case F_LineCutToStart:
	case F_LineCutToEnd:
	case F_DELETE_LINE:
	case F_CUT_LINE:
	case F_DUPLICATELINE:
	case F_INDENT_TAB:
	case F_UNINDENT_TAB:
	case F_INDENT_SPACE:
	case F_UNINDENT_SPACE:
	case F_CUT:
	case F_PASTE:
	case F_INS_DATE:
	case F_INS_TIME:
	case F_CTRL_CODE_DIALOG:	//@@@ 2002.06.02 MIK
	case F_INSTEXT:
	case F_ADDTAIL:
	case F_PASTEBOX:
	case F_REPLACE_DIALOG:
	case F_REPLACE:
	case F_REPLACE_ALL:
	case F_CODECNV_EMAIL:
	case F_CODECNV_EUC2SJIS:
	case F_CODECNV_UNICODE2SJIS:
	case F_CODECNV_UNICODEBE2SJIS:
	case F_CODECNV_SJIS2JIS:
	case F_CODECNV_SJIS2EUC:
	case F_CODECNV_UTF82SJIS:
	case F_CODECNV_UTF72SJIS:
	case F_CODECNV_SJIS2UTF7:
	case F_CODECNV_SJIS2UTF8:
	case F_CODECNV_AUTO2SJIS:
	case F_TOLOWER:
	case F_TOUPPER:
	case F_TOHANKAKU:
	case F_TOHANKATA:				// 2002/08/29 ai
	case F_TOZENEI:					// 2001/07/30 Misaka
	case F_TOHANEI:
	case F_TOZENKAKUKATA:
	case F_TOZENKAKUHIRA:
	case F_HANKATATOZENKATA:
	case F_HANKATATOZENHIRA:
	case F_TABTOSPACE:
	case F_SPACETOTAB:  //#### Stonee, 2001/05/27
	case F_HOKAN:
	case F_CHGMOD_INS:
	case F_LTRIM:		// 2001.12.03 hor
	case F_RTRIM:		// 2001.12.03 hor
	case F_SORT_ASC:	// 2001.12.11 hor
	case F_SORT_DESC:	// 2001.12.11 hor
	case F_MERGE:		// 2001.12.11 hor
	case F_UNDO:		// 2007.10.12 genta
	case F_REDO:		// 2007.10.12 genta
		return true;
	}
	return false;	//	デフォルトで書き換え許可
}
//	To Here Aug. 14, 2000 genta

/*! コマンドコードによる処理振り分け

	@param[in] nCommand MAKELONG( コマンドコード，送信元識別子 )

	@date 2006.05.19 genta 上位16bitに送信元の識別子が入るように変更
	@date 2007.06.20 ryoji グループ内で巡回するように変更
*/
BOOL CEditDoc::HandleCommand( int nCommand )
{
	int				i;
	int				j;
	int				nGroup;
	int				nRowNum;
	int				nPane;
	HWND			hwndWork;
	EditNode*		pEditNodeArr;

	//	May. 19, 2006 genta 上位16bitに送信元の識別子が入るように変更したので
	//	下位16ビットのみを取り出す
	switch( LOWORD( nCommand )){
	case F_PREVWINDOW:	//前のウィンドウ
		nPane = m_cSplitterWnd.GetPrevPane();
		if( -1 != nPane ){
			SetActivePane( nPane );
		}else{
			/* 現在開いている編集窓のリストを得る */
			nRowNum = CShareData::getInstance()->GetOpenedWindowArr( &pEditNodeArr, TRUE );
			if(  nRowNum > 0 ){
				/* 自分のウィンドウを調べる */
				nGroup = 0;
				for( i = 0; i < nRowNum; ++i )
				{
					if( m_hwndParent == pEditNodeArr[i].m_hWnd )
					{
						nGroup = pEditNodeArr[i].m_nGroup;
						break;
					}
				}
				if( i < nRowNum )
				{
					// 前のウィンドウ
					for( j = i - 1; j >= 0; --j )
					{
						if( nGroup == pEditNodeArr[j].m_nGroup )
							break;
					}
					if( j < 0 )
					{
						for( j = nRowNum - 1; j > i; --j )
						{
							if( nGroup == pEditNodeArr[j].m_nGroup )
								break;
						}
					}
					/* 前のウィンドウをアクティブにする */
					hwndWork = pEditNodeArr[j].m_hWnd;
					/* アクティブにする */
					ActivateFrameWindow( hwndWork );
					/* 最後のペインをアクティブにする */
					::PostMessage( hwndWork, MYWM_SETACTIVEPANE, (WPARAM)-1, 1 );
				}
				delete [] pEditNodeArr;
			}
		}
		return TRUE;
	case F_NEXTWINDOW:	//次のウィンドウ
		nPane = m_cSplitterWnd.GetNextPane();
		if( -1 != nPane ){
			SetActivePane( nPane );
		}else{
			/* 現在開いている編集窓のリストを得る */
			nRowNum = CShareData::getInstance()->GetOpenedWindowArr( &pEditNodeArr, TRUE );
			if(  nRowNum > 0 ){
				/* 自分のウィンドウを調べる */
				nGroup = 0;
				for( i = 0; i < nRowNum; ++i )
				{
					if( m_hwndParent == pEditNodeArr[i].m_hWnd )
					{
						nGroup = pEditNodeArr[i].m_nGroup;
						break;
					}
				}
				if( i < nRowNum )
				{
					// 次のウィンドウ
					for( j = i + 1; j < nRowNum; ++j )
					{
						if( nGroup == pEditNodeArr[j].m_nGroup )
							break;
					}
					if( j >= nRowNum )
					{
						for( j = 0; j < i; ++j )
						{
							if( nGroup == pEditNodeArr[j].m_nGroup )
								break;
						}
					}
					/* 次のウィンドウをアクティブにする */
					hwndWork = pEditNodeArr[j].m_hWnd;
					/* アクティブにする */
					ActivateFrameWindow( hwndWork );
					/* 最初のペインをアクティブにする */
					::PostMessage( hwndWork, MYWM_SETACTIVEPANE, (WPARAM)-1, 0 );
				}
				delete [] pEditNodeArr;
			}
		}
		return TRUE;
	default:
		return m_pcEditViewArr[m_nActivePaneIndex]->HandleCommand( nCommand, true, 0, 0, 0, 0 );
	}
}

/*!	タイプ別設定の適用を変更
	@date 2011.12.15 CEditView::Command_TYPE_LISTから移動
*/
void CEditDoc::OnChangeType()
{
	// 新規で無変更ならデフォルト文字コードを適用する	// 2011.01.24 ryoji
	if( !IsValidPath() ){
		if( !IsModified()  && m_cDocLineMgr.GetLineCount() == 0 ){
			STypeConfig& types = GetDocumentAttribute();
			m_nCharCode = types.m_eDefaultCodetype;
			m_bBomExist = types.m_bDefaultBom;
			SetNewLineCode( static_cast<EEolType>(types.m_eDefaultEoltype) );
		}
	}
	/* 設定変更を反映させる */
	m_bTextWrapMethodCurTemp = false;	// 折り返し方法の一時設定適用中を解除	// 2008.06.08 ryoji
	OnChangeSetting();

	// 2006.09.01 ryoji タイプ変更後自動実行マクロを実行する
	RunAutoMacro( m_pShareData->m_Common.m_sMacro.m_nMacroOnTypeChanged );
}

/*! ビューに設定変更を反映させる

	@date 2004.06.09 Moca レイアウト再構築中にProgress Barを表示する．
	@date 2008.05.30 nasukoji	テキストの折り返し方法の変更処理を追加
*/
void CEditDoc::OnChangeSetting()
{
	int			i;
	HWND		hwndProgress = NULL;

	CEditWnd*	pCEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta

	if( NULL != pCEditWnd ){
		hwndProgress = pCEditWnd->m_hwndProgressBar;
		//	Status Barが表示されていないときはm_hwndProgressBar == NULL
	}

	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_SHOW );
	}

	/* ファイルの排他モード変更 */
	if( m_nFileShareModeOld != m_pShareData->m_Common.m_sFile.m_nFileShareMode ){
		/* ファイルの排他ロック解除 */
		DoFileUnlock();
		/* ファイルの排他ロック */
		DoFileLock();
	}
	CShareData::getInstance()->TransformFileName_MakeCache();
	int doctype = CShareData::getInstance()->GetDocumentTypeOfPath( GetFilePath() );
	SetDocumentType( doctype, false );

	int* posSaveAry = SavePhysPosOfAllView();

	/* レイアウト情報の作成 */
	STypeConfig ref = GetDocumentAttribute();
	{
		// 2008.06.07 nasukoji	折り返し方法の追加に対応
		// 折り返し方法の一時設定とタイプ別設定が一致したら一時設定適用中は解除
		if( m_nTextWrapMethodCur == ref.m_nTextWrapMethod )
			m_bTextWrapMethodCurTemp = false;		// 一時設定適用中を解除

		// 一時設定適用中でなければ折り返し方法変更
		if( !m_bTextWrapMethodCurTemp )
			m_nTextWrapMethodCur = ref.m_nTextWrapMethod;	// 折り返し方法

		// 指定桁で折り返す：タイプ別設定を使用
		// 右端で折り返す：仮に現在の折り返し幅を使用
		// 上記以外：MAXLINEKETASを使用
		if( m_nTextWrapMethodCur != WRAP_SETTING_WIDTH ){
			if( m_nTextWrapMethodCur == WRAP_WINDOW_WIDTH )
				ref.m_nMaxLineKetas = m_cLayoutMgr.GetMaxLineKetas();	// 現在の折り返し幅
			else
				ref.m_nMaxLineKetas = MAXLINEKETAS;
		}
	}

	m_cLayoutMgr.SetLayoutInfo(
		TRUE,
		hwndProgress,
		ref
	); /* レイアウト情報の変更 */

	// 2009.08.28 nasukoji	「折り返さない」ならテキスト最大幅を算出、それ以外は変数をクリア
	if( m_nTextWrapMethodCur == WRAP_NO_TEXT_WRAP )
		m_cLayoutMgr.CalculateTextWidth();		// テキスト最大幅を算出する
	else
		m_cLayoutMgr.ClearLayoutLineWidth();	// 各行のレイアウト行長の記憶をクリアする

	/* ビューに設定変更を反映させる */
	int viewCount = GetAllViewCount();
	for( i = 0; i < viewCount; ++i ){
		m_pcEditViewArr[i]->OnChangeSetting();
	}
	RestorePhysPosOfAllView( posSaveAry );
	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_HIDE );
	}
}

/*! ファイルを閉じるときのMRU登録 & 保存確認 ＆ 保存実行

	@retval TRUE: 終了して良い / FALSE: 終了しない
*/
BOOL CEditDoc::OnFileClose()
{
	int			nRet;
	int			nBool;
	HWND		hwndMainFrame;
	hwndMainFrame = ::GetParent( m_hWnd );

	//	Mar. 30, 2003 genta サブルーチンにまとめた
	AddToMRU();

	if( m_bGrepRunning ){		/* Grep処理中 */
		/* アクティブにする */
		ActivateFrameWindow( hwndMainFrame );	//@@@ 2003.06.25 MIK
		TopInfoMessage(
			hwndMainFrame,
			_T("Grepの処理中です。\n")
		);
		return FALSE;
	}


	/* テキストが変更されている場合 */
	if( IsModified()
	&& !m_bDebugMode	/* デバッグモニタモードのときは保存確認しない */
	){
		if( m_bGrepMode ){	/* Grepモードのとき */
			/* Grepモードで保存確認するか */
			if( FALSE == m_pShareData->m_Common.m_sSearch.m_bGrepExitConfirm ){
				return TRUE;
			}
		}
		/* ウィンドウをアクティブにする */
		/* アクティブにする */
		ActivateFrameWindow( hwndMainFrame );
		if( m_bReadOnly ){	/* 読み取り専用モード */
			ConfirmBeep();
			nRet = ::MYMESSAGEBOX(
				hwndMainFrame,
				MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
				GSTR_APPNAME,
				_T("%s\nは変更されています。 閉じる前に保存しますか？\n\n読み取り専用で開いているので、名前を付けて保存すればいいと思います。\n"),
				IsValidPath() ? GetFilePath() : _T("(無題)")
			);
			switch( nRet ){
			case IDYES:
				nBool = FileSaveAs_Dialog();	// 2006.12.30 ryoji
				return nBool;
			case IDNO:
				return TRUE;
			case IDCANCEL:
			default:
				return FALSE;
			}
		}
		else{
			ConfirmBeep();
			nRet = ::MYMESSAGEBOX(
				hwndMainFrame,
				MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
				GSTR_APPNAME,
				_T("%s\nは変更されています。 閉じる前に保存しますか？"),
				IsValidPath() ? GetFilePath() : _T("(無題)")
			);
			switch( nRet ){
			case IDYES:
				if( IsValidPath() ){
					nBool = FileSave();	// 2006.12.30 ryoji
				}
				else{
					nBool = FileSaveAs_Dialog();	// 2006.12.30 ryoji
				}
				return nBool;
			case IDNO:
				return TRUE;
			case IDCANCEL:
			default:
				return FALSE;
			}
		}
	}else{
		return TRUE;
	}
}

/*!	@brief マクロ自動実行

	@param type [in] 自動実行マクロ番号
	@return

	@author ryoji
	@date 2006.09.01 ryoji 作成
	@date 2007.07.20 genta HandleCommandに追加情報を渡す．
		自動実行マクロで発行したコマンドはキーマクロに保存しない
*/
void CEditDoc::RunAutoMacro( int idx, LPCTSTR pszSaveFilePath )
{
	static bool bRunning = false;
	if( bRunning )
		return;	// 再入り実行はしない

	bRunning = true;
	if( m_pcSMacroMgr->IsEnabled(idx) ){
		if( !( ::GetAsyncKeyState(VK_SHIFT) & 0x8000 ) ){	// Shift キーが押されていなければ実行
			if( NULL != pszSaveFilePath )
				_tcscpy( m_szSaveFilePath, pszSaveFilePath );
			//	2007.07.20 genta 自動実行マクロで発行したコマンドはキーマクロに保存しない
			HandleCommand(( F_USERMACRO_0 + idx ) | FA_NONRECORD );
			m_szSaveFilePath[0] = _T('\0');
		}
	}
	bRunning = false;
}

/*
|| メッセージディスパッチャ
*/
LRESULT CEditDoc::DispatchEvent(
	HWND	hwnd,	// handle of window
	UINT	uMsg,	// message identifier
	WPARAM	wParam,	// first message parameter
	LPARAM	lParam 	// second message parameter
)
{
	switch( uMsg ){
	case WM_ENTERMENULOOP:
	case WM_EXITMENULOOP:
		{
			int i;
			for( i = 0; i < GetAllViewCount(); ++i ){
				m_pcEditViewArr[i]->DispatchEvent( hwnd, uMsg, wParam, lParam );
			}
		}
		return 0L;
	default:
		return m_pcEditViewArr[m_nActivePaneIndex]->DispatchEvent( hwnd, uMsg, wParam, lParam );
	}
}



void CEditDoc::OnMove( int x, int y, int nWidth, int nHeight )
{
//	m_cSplitterWnd.OnMove( x, y, nWidth, nHeight );
	::MoveWindow( m_cSplitterWnd.m_hWnd, x, y, nWidth, nHeight, TRUE );

	return;
}




/*! テキストが選択されているか */
BOOL CEditDoc::IsTextSelected( void )
{
	return m_pcEditViewArr[m_nActivePaneIndex]->IsTextSelected();
}




BOOL CEditDoc::SelectFont( LOGFONT* plf )
{
	// 2004.02.16 Moca CHOOSEFONTをメンバから外す
	CHOOSEFONT cf;
	/* CHOOSEFONTの初期化 */
	::ZeroMemory( &cf, sizeof( CHOOSEFONT ) );
	cf.lStructSize = sizeof( cf );
	cf.hwndOwner = m_hWnd;
	cf.hDC = NULL;
//	cf.lpLogFont = &(m_pShareData->m_Common.m_lf);
	cf.Flags = CF_FIXEDPITCHONLY | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
//#ifdef _DEBUG
//	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
//#endif
	cf.lpLogFont = plf;
	if( FALSE == ChooseFont( &cf ) ){
#ifdef _DEBUG
		DWORD nErr;
		nErr = CommDlgExtendedError();
		switch( nErr ){
		case CDERR_FINDRESFAILURE:	MYTRACE_A( "CDERR_FINDRESFAILURE \n" );	break;
		case CDERR_INITIALIZATION:	MYTRACE_A( "CDERR_INITIALIZATION \n" );	break;
		case CDERR_LOCKRESFAILURE:	MYTRACE_A( "CDERR_LOCKRESFAILURE \n" );	break;
		case CDERR_LOADRESFAILURE:	MYTRACE_A( "CDERR_LOADRESFAILURE \n" );	break;
		case CDERR_LOADSTRFAILURE:	MYTRACE_A( "CDERR_LOADSTRFAILURE \n" );	break;
		case CDERR_MEMALLOCFAILURE:	MYTRACE_A( "CDERR_MEMALLOCFAILURE\n" );	break;
		case CDERR_MEMLOCKFAILURE:	MYTRACE_A( "CDERR_MEMLOCKFAILURE \n" );	break;
		case CDERR_NOHINSTANCE:		MYTRACE_A( "CDERR_NOHINSTANCE \n" );		break;
		case CDERR_NOHOOK:			MYTRACE_A( "CDERR_NOHOOK \n" );			break;
		case CDERR_NOTEMPLATE:		MYTRACE_A( "CDERR_NOTEMPLATE \n" );		break;
		case CDERR_STRUCTSIZE:		MYTRACE_A( "CDERR_STRUCTSIZE \n" );		break;
		case CFERR_MAXLESSTHANMIN:	MYTRACE_A( "CFERR_MAXLESSTHANMIN \n" );	break;
		case CFERR_NOFONTS:			MYTRACE_A( "CFERR_NOFONTS \n" );			break;
		}
#endif
		return FALSE;
	}

	return TRUE;
}




/*! ファイルを開く

	@return 成功: TRUE/pbOpened==FALSE,
			既に開かれている: FALSE/pbOpened==TRUE
			失敗: FALSE/pbOpened==FALSE

	@note genta 近いうちに見直した方がいいな．

	@date 2000.01.18 システム属性のファイルが開けない問題
	@date 2000.05,12 genta 改行コードの設定
	@date 2000.10.25 genta 文字コードの異常な値をチェック
	@date 2000.11.20 genta IME状態の設定
	@date 2001.12.26 YAZAKI MRUリストは、CMRUに依頼する
	@date 2002.01.16 hor ブックマーク復元
	@date 2002.10.19 genta 読み取り不可のファイルは文字コード判別で失敗する
	@date 2003.03.28 MIK 改行の真ん中にカーソルが来ないように
	@date 2003.07.26 ryoji BOM引数追加
	@date 2002.05.26 Moca gm_pszCodeNameArr_1 を使うように変更
	@date 2004.06.18 moca ファイルが開けなかった場合にpbOpenedがFALSEに初期化されていなかった．
	@date 2004.10.09 genta 存在しないファイルを開こうとしたときに
					フラグに応じて警告を出す（以前の動作）ように
	@date 2006.12.16 じゅうじ 前回の文字コードを優先する
	@date 2007.03.12 maru ファイルが存在しなくても前回の文字コードを継承
						多重オープン処理をCEditDoc::IsPathOpenedに移動
*/
BOOL CEditDoc::FileRead(
	TCHAR*	pszPath,	//!< [in/out]
	BOOL*	pbOpened,	//!< [out] すでに開かれていたか
	ECodeType	nCharCode,		/*!< [in] 文字コード種別 */
	bool	bReadOnly,			/*!< [in] 読み取り専用か */
	bool	bConfirmCodeChange	/*!< [in] 文字コード変更時の確認をするかどうか */
)
{
	int				i;
	HWND			hWndOwner;
	BOOL			bRet;
	EditInfo		fi;
//	EditInfo*		pfi;
	HWND			hwndProgress;
	CWaitCursor		cWaitCursor( m_hWnd );
	BOOL			bIsExistInMRU;
	int				nRet;
	BOOL			bFileIsExist;
	int				doctype;

	*pbOpened = FALSE;	// 2004.06.18 Moca 初期化ミス
	m_bReadOnly = bReadOnly;	/* 読み取り専用モード */

//@@@ 2001.12.26 YAZAKI MRUリストは、CMRUに依頼する
	CMRUFile			cMRU;

	/* ファイルの存在チェック */
	bFileIsExist = FALSE;
	if( !fexist( pszPath ) ){
	}else{
		HANDLE			hFind;
		WIN32_FIND_DATA	w32fd;
		hFind = ::FindFirstFile( pszPath, &w32fd );
		::FindClose( hFind );
//? 2000.01.18 システム属性のファイルが開けない問題
//?		if( w32fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ){
//?		}else{
			bFileIsExist = TRUE;
//?		}
		/* フォルダが指定された場合 */
		if( w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
			/* 指定フォルダで「開くダイアログ」を表示 */
			{
				TCHAR*		pszPathNew = new TCHAR[_MAX_PATH];

				pszPathNew[0] = _T('\0');

				/* 「ファイルを開く」ダイアログ */
				nCharCode = CODE_AUTODETECT;	/* 文字コード自動判別 */
				bReadOnly = false;
//				::ShowWindow( m_hWnd, SW_SHOW );
				if( !OpenFileDialog( m_hWnd, pszPath, pszPathNew, &nCharCode, &bReadOnly ) ){
					delete [] pszPathNew;
					return FALSE;
				}
				_tcscpy( pszPath, pszPathNew );
				delete [] pszPathNew;
				if( !fexist( pszPath ) ){
					bFileIsExist = FALSE;
				}else{
					bFileIsExist = TRUE;
				}
			}
		}

	}

	//	From Here Oct. 19, 2002 genta
	//	読み込みアクセス権が無い場合には漢字コード判定でファイルを
	//	開けないので文字コード判別エラーと出てしまう．
	//	より適切なメッセージを出すため，読めないファイルは
	//	事前に判定・排除する
	//
	//	_taccessではロックされたファイルの状態を取得できないので
	//	実際にファイルを開いて確認する
	if( bFileIsExist){
		HANDLE hTest = 	CreateFile( pszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
		if( hTest == INVALID_HANDLE_VALUE ){
			// 読み込みアクセス権がない
			ErrorMessage(
				m_hWnd,
				_T("\'%s\'\nというファイルを開けません。\n読み込みアクセス権がありません。"),
				pszPath
			 );
			return FALSE;
		}
		else {
			CloseHandle( hTest );
		}
	}
	//	To Here Oct. 19, 2002 genta

	CEditWnd*	pCEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta
	if( NULL != pCEditWnd ){
		hwndProgress = pCEditWnd->m_hwndProgressBar;
	}else{
		hwndProgress = NULL;
	}
	bRet = TRUE;
	if( NULL == pszPath ){
		MYMESSAGEBOX(
			m_hWnd,
			MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST,
			_T("バグじゃぁあああ！！！"),
			_T("CEditDoc::FileRead()\n\nNULL == pszPath\n【対処】エラーの出た状況を作者に連絡してくださいね。")
		);
		return FALSE;
	}
	/* 指定ファイルが開かれているか調べる */
	if( CShareData::getInstance()->ActiveAlreadyOpenedWindow(pszPath, &hWndOwner, nCharCode) ){	/* 2007.03.12 maru 多重オープン処理はIsPathOpenedにまとめる */
		*pbOpened = TRUE;
		bRet = FALSE;
		goto end_of_func;
	}
	for( i = 0; i < GetAllViewCount(); ++i ){
		if( m_pcEditViewArr[i]->IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			m_pcEditViewArr[i]->DisableSelectArea( true );
		}
	}

	//	Sep. 10, 2002 genta
	SetFilePathAndIcon( pszPath ); /* 現在編集中のファイルのパス */


	/* 指定された文字コード種別に変更する */
	//	Oct. 25, 2000 genta
	//	文字コードとして異常な値が設定された場合の対応
	//	-1以上CODE_MAX未満のみ受け付ける
	//	Oct. 26, 2000 genta
	//	CODE_AUTODETECTはこの範囲から外れているから個別にチェック
	if( nCharCode == CODE_NONE || IsValidCodeType(nCharCode) || nCharCode == CODE_AUTODETECT )
		m_nCharCode = nCharCode;
	
	/* MRUリストに存在するか調べる  存在するならばファイル情報を返す */
//@@@ 2001.12.26 YAZAKI MRUリストは、CMRUに依頼する
	if ( cMRU.GetEditInfo( pszPath, &fi ) ){
		bIsExistInMRU = TRUE;

		if( CODE_NONE == m_nCharCode ){
			/* 前回に指定された文字コード種別に変更する */
			m_nCharCode = fi.m_nCharCode;
		}
		
		if( !bConfirmCodeChange && ( CODE_AUTODETECT == m_nCharCode ) ){	// 文字コード指定の再オープンなら前回を無視
			m_nCharCode = fi.m_nCharCode;
		}
		if( (FALSE == bFileIsExist) && (CODE_AUTODETECT == m_nCharCode) ){
			/* 存在しないファイルの文字コード指定なしなら前回を継承 */
			m_nCharCode = fi.m_nCharCode;
		}
	} else {
		bIsExistInMRU = FALSE;
	}

	/* 文字コード自動判別 */
	if( CODE_AUTODETECT == m_nCharCode ) {
		if( FALSE == bFileIsExist ){	/* ファイルが存在しない */
			m_nCharCode = CODE_DEFAULT;
		} else {
			m_nCharCode = CMemory::CheckKanjiCodeOfFile( pszPath );
			if( CODE_NONE == m_nCharCode ){
				TopWarningMessage( m_hWnd,
					_T("%s\n文字コードの判別処理でエラーが発生しました。"),
					pszPath
				);
				//	Sep. 10, 2002 genta
				SetFilePathAndIcon( _T("") );
				bRet = FALSE;
				goto end_of_func;
			}
		}
	}
	/* 文字コードが異なるときに確認する */
	if( bConfirmCodeChange && bIsExistInMRU ){
		if (m_nCharCode != fi.m_nCharCode ) {	// MRU の文字コードと判別が異なる
			LPCTSTR	pszCodeNameOld = NULL;
			LPCTSTR	pszCodeNameNew = NULL;

			// gm_pszCodeNameArr_1 を使うように変更 Moca. 2002/05/26
			if( IsValidCodeType(fi.m_nCharCode) ){
				pszCodeNameOld = gm_pszCodeNameArr_1[fi.m_nCharCode];
			}
			if( IsValidCodeType(m_nCharCode) ){
				pszCodeNameNew = gm_pszCodeNameArr_1[m_nCharCode];
			}
			if( pszCodeNameOld != NULL ){
				ConfirmBeep();
				nRet = MYMESSAGEBOX(
					m_hWnd,
					MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
					_T("文字コード情報"),
					_T("%s\n")
					_T("\n")
					_T("このファイルは、前回は別の文字コード %s で開かれています。\n")
					_T("前回と同じ文字コードを使いますか？\n")
					_T("\n")
					_T("・[はい(Y)]  ＝%s\n")
					_T("・[いいえ(N)]＝%s\n")
					_T("・[キャンセル]＝開きません"),
					GetFilePath(),
					pszCodeNameOld,
					pszCodeNameOld,
					pszCodeNameNew
				);
				if( IDYES == nRet ){
					/* 前回に指定された文字コード種別に変更する */
					m_nCharCode = fi.m_nCharCode;
				}else
				if( IDCANCEL == nRet ){
					m_nCharCode = CODE_DEFAULT;
					//	Sep. 10, 2002 genta
					SetFilePathAndIcon( _T("") );
					bRet = FALSE;
					goto end_of_func;
				}
			}else{
				MYMESSAGEBOX(
					m_hWnd,
					MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST,
					_T("バグじゃぁあああ！！！"),
					_T("【対処】エラーの出た状況を作者に連絡してください。")
				);
				//	Sep. 10, 2002 genta
				SetFilePathAndIcon( _T("") );
				bRet = FALSE;
				goto end_of_func;
			}
		}
	}
	if( CODE_NONE == m_nCharCode ){
		m_nCharCode = CODE_DEFAULT;
	}

	//	Nov. 12, 2000 genta ロングファイル名の取得を前方に移動
	char szWork[MAX_PATH];
	/* ロングファイル名を取得する */
	if( TRUE == ::GetLongFileName( pszPath, szWork ) ){
		//	Sep. 10, 2002 genta
		SetFilePathAndIcon( szWork );
	}

	// タイプ別設定
	if( bIsExistInMRU && ((fi.m_nType>=0 && fi.m_nType<MAX_TYPES)) ){
		doctype = fi.m_nType;
	}else{
		doctype = CShareData::getInstance()->GetDocumentTypeOfPath( GetFilePath() );
	}
	SetDocumentType( doctype, true );

	//	From Here Jul. 26, 2003 ryoji BOMの有無の初期状態を設定
	switch( m_nCharCode ){
	case CODE_UNICODE:
	case CODE_UNICODEBE:
		m_bBomExist = true;
		break;
	case CODE_UTF8:
	default:
		m_bBomExist = false;
		break;
	}
	//	To Here Jul. 26, 2003 ryoji BOMの有無の初期状態を設定

	//ファイルが存在する場合はファイルを読む
	if( bFileIsExist ){
		/* ファイルを読む */
		if( NULL != hwndProgress ){
			::ShowWindow( hwndProgress, SW_SHOW );
		}
		//	Jul. 26, 2003 ryoji BOM引数追加
		if( FALSE == m_cDocLineMgr.ReadFile( GetFilePath(), m_hWnd, hwndProgress,
			m_nCharCode, &m_FileTime, m_pShareData->m_Common.m_sFile.GetAutoMIMEdecode(), &m_bBomExist ) ){
			//	Sep. 10, 2002 genta
			SetFilePathAndIcon( _T("") );
			bRet = FALSE;
			goto end_of_func;
		}
	}else{
		// 存在しないときもドキュメントに文字コードを反映する
		const STypeConfig& type = GetDocumentAttribute();
		m_nCharCode = type.m_eDefaultCodetype;
		m_bBomExist = type.m_bDefaultBom;

		//	Oct. 09, 2004 genta フラグに応じて警告を出す（以前の動作）ように
		if( m_pShareData->m_Common.m_sFile.GetAlertIfFileNotExist() ){
			InfoBeep();

			//	Feb. 15, 2003 genta Popupウィンドウを表示しないように．
			//	ここでステータスメッセージを使っても画面に表示されない．
			TopInfoMessage(
				m_hwndParent,
				_T("%s\nというファイルは存在しません。\n\nファイルを保存したときに、ディスク上にこのファイルが作成されます。"),	//Mar. 24, 2001 jepro 若干修正
				pszPath
			);
		}
	}

	/* レイアウト情報の変更 */
	{
//		STypeConfig& ref = GetDocumentAttribute();
		// 2008.06.07 nasukoji	折り返し方法の追加に対応
		// 「指定桁で折り返す」以外の時は折り返し幅をMAXLINEKETASで初期化する
		// 「右端で折り返す」は、この後のOnSize()で再設定される
		STypeConfig ref = GetDocumentAttribute();
		if( ref.m_nTextWrapMethod != WRAP_SETTING_WIDTH )
			ref.m_nMaxLineKetas = MAXLINEKETAS;

		m_cLayoutMgr.SetLayoutInfo(
			TRUE,
			hwndProgress,
			ref
		);
	}

	/* 全ビューの初期化：ファイルオープン/クローズ時等に、ビューを初期化する */
	InitAllView();

	//	Nov. 20, 2000 genta
	//	IME状態の設定
	SetImeMode( GetDocumentAttribute().m_nImeState );

	if( bIsExistInMRU && m_pShareData->m_Common.m_sFile.GetRestoreCurPosition() ){
		/*
		  カーソル位置変換
		  物理位置(行頭からのバイト数、折り返し無し行位置)
		  →
		  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		*/
		int		nCaretPosX;
		int		nCaretPosY;
		m_cLayoutMgr.LogicToLayout(
			fi.m_nX,
			fi.m_nY,
			&nCaretPosX,
			&nCaretPosY
		);
		if( nCaretPosY >= m_cLayoutMgr.GetLineCount() ){
			/*ファイルの最後に移動 */
			m_pcEditViewArr[m_nActivePaneIndex]->HandleCommand( F_GOFILEEND, false, 0, 0, 0, 0 );
		}else{
			m_pcEditViewArr[m_nActivePaneIndex]->m_nViewTopLine = fi.m_nViewTopLine; // 2001/10/20 novice
			m_pcEditViewArr[m_nActivePaneIndex]->m_nViewLeftCol = fi.m_nViewLeftCol; // 2001/10/20 novice
			// From Here Mar. 28, 2003 MIK
			// 改行の真ん中にカーソルが来ないように。
			const CDocLine *pTmpDocLine = m_cDocLineMgr.GetLine( fi.m_nY );	// 2008.08.22 ryoji 改行単位の行番号を渡すように修正
			if( pTmpDocLine ){
				if( pTmpDocLine->GetLengthWithoutEOL() < fi.m_nX ) nCaretPosX--;
			}
			// To Here Mar. 28, 2003 MIK
			m_pcEditViewArr[m_nActivePaneIndex]->MoveCursor( nCaretPosX, nCaretPosY, true );
			m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX_Prev = m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX;
		}
	}
	// 2002.01.16 hor ブックマーク復元
	if( bIsExistInMRU ){
		if( m_pShareData->m_Common.m_sFile.GetRestoreBookmarks() ){
			m_cDocLineMgr.SetBookMarks(fi.m_szMarkLines);
		}
	}else{
		fi.m_szMarkLines[0] = '\0';
	}
	GetEditInfo( &fi );

	//	May 12, 2000 genta
	//	改行コードの設定
	{
		const STypeConfig& type = GetDocumentAttribute();
		if ( m_nCharCode == type.m_eDefaultCodetype ){
			SetNewLineCode( static_cast<EEolType>(type.m_eDefaultEoltype) );	// 2011.01.24 ryoji デフォルトEOL
		}
		else{
			SetNewLineCode( EOL_CRLF );
		}
		CDocLine*	pFirstlineinfo = m_cDocLineMgr.GetLine( 0 );
		if( pFirstlineinfo != NULL ){
			EEolType t = (EEolType)pFirstlineinfo->m_cEol;
			if( t != EOL_NONE && t != EOL_UNKNOWN ){
				SetNewLineCode( t );
			}
		}
	}

	/* MRUリストへの登録 */
//@@@ 2001.12.26 YAZAKI MRUリストは、CMRUに依頼する
	cMRU.Add( &fi );
	
	/* カレントディレクトリの変更 */
	{
		char	szCurDir[_MAX_PATH];
		char	szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
		_splitpath( GetFilePath(), szDrive, szDir, NULL, NULL );
		strcpy( szCurDir, szDrive);
		strcat( szCurDir, szDir );
		::SetCurrentDirectory( szCurDir );
	}

end_of_func:;
	//	2004.05.13 Moca 改行コードの設定内からここに移動
	m_pcEditViewArr[m_nActivePaneIndex]->DrawCaretPosInfo();

	if( NULL != hwndProgress ){
		::ShowWindow( hwndProgress, SW_HIDE );
	}
	if( TRUE == bRet && IsValidPath() ){
		/* ファイルの排他ロック */
		DoFileLock();
	}
	//	From Here Jul. 26, 2003 ryoji エラーの時は規定のBOM設定とする
	if( FALSE == bRet ){
		switch( m_nCharCode ){
		case CODE_UNICODE:
		case CODE_UNICODEBE:
			m_bBomExist = true;
			break;
		case CODE_UTF8:
		default:
			m_bBomExist = false;
			break;
		}
	}
	//	To Here Jul. 26, 2003 ryoji
	return bRet;
}


/*!	@brief ファイルの保存
	
	@param pszPath [in] 保存ファイル名
	@param cEolType [in] 改行コード種別
	
	pszPathはNULLであってはならない。
	
	@date Feb. 9, 2001 genta 改行コード用引数追加
*/
BOOL CEditDoc::FileWrite( const char* pszPath, EEolType cEolType )
{
	BOOL		bRet;
	EditInfo	fi;
	HWND		hwndProgress;
//@@@ 2001.12.26 YAZAKI MRUリストは、CMRUに依頼する
	CMRUFile	cMRU;
	//	Feb. 9, 2001 genta
	CEol	cEol( cEolType );

	//	Jun.  5, 2004 genta ここでReadOnlyチェックをすると，ファイル名を変更しても
	//	保存できなくなってしまうので，チェックを上書き保存処理へ移動．

	//	Sep. 7, 2003 genta
	//	保存が完了するまではファイル更新の通知を抑制する
	WatchUpdate wuSave = m_eWatchUpdate;
	m_eWatchUpdate = WU_NONE;

	bRet = TRUE;

	CEditWnd*	pCEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta
	if( NULL != pCEditWnd ){
		hwndProgress = pCEditWnd->m_hwndProgressBar;
	}else{
		hwndProgress = NULL;
	}
	if( NULL != hwndProgress ){
		::ShowWindow( hwndProgress, SW_SHOW );
	}


	/* ファイルの排他ロック解除 */
	DoFileUnlock();

	if( m_pShareData->m_Common.m_sBackup.m_bBackUp ){	/* バックアップの作成 */
		//	Jun.  5, 2004 genta ファイル名を与えるように．戻り値に応じた処理を追加．
		switch( MakeBackUp( pszPath )){
		case 2:	//	中断指示
			return FALSE;
		case 3: //	ファイルエラー
			if( IDYES != ::MYMESSAGEBOX(
				m_hWnd,
				MB_YESNO | MB_ICONQUESTION | MB_TOPMOST,
				_T("ファイル保存"),
				_T("バックアップの作成に失敗しました．元ファイルへの上書きを継続して行いますか．")
			)){
				return FALSE;
			}
		case 4:	//	バックアップファイルのパスが長すぎる
			if( IDYES != ::MYMESSAGEBOX(
				m_hWnd,
				MB_YESNO | MB_ICONQUESTION | MB_TOPMOST,
				_T("ファイル保存"),
				_T("ファイルパスが長すぎるためバックアップの作成に失敗しました．\n")
				_T("ANSI 版では %d バイト以上の絶対パスを扱えません．\n\n")
				_T("元ファイルへの上書きを継続して行いますか．"),
				_MAX_PATH
			)){
				return FALSE;
			}
		break;
		}
	}

	CWaitCursor cWaitCursor( m_hWnd );
	//	Jul. 26, 2003 ryoji BOM引数追加
	if( FALSE == m_cDocLineMgr.WriteFile( pszPath, m_hWnd, hwndProgress,
		m_nCharCode, &m_FileTime, cEol , m_bBomExist ) ){
		bRet = FALSE;
		goto end_of_func;
	}
	/* 行変更状態をすべてリセット */
	m_cDocLineMgr.ResetAllModifyFlag();

	int	v;
	for( v = 0; v < GetAllViewCount(); ++v ){
		if( m_nActivePaneIndex != v ){
			m_pcEditViewArr[v]->RedrawAll();
		}
	}
	m_pcEditViewArr[m_nActivePaneIndex]->RedrawAll();

	//	Sep. 10, 2002 genta
	SetFilePathAndIcon( pszPath ); /* 現在編集中のファイルのパス */

	SetModified(false,false);	//	Jan. 22, 2002 genta 関数化 更新フラグのクリア

	//	Mar. 30, 2003 genta サブルーチンにまとめた
	AddToMRU();

	/* 現在位置で無変更な状態になったことを通知 */
	m_cOpeBuf.SetNoModified();

	m_bReadOnly = false;	/* 読み取り専用モード */

	/* 親ウィンドウのタイトルを更新 */
	UpdateCaption();
end_of_func:;

	if( IsValidPath() &&
		!m_bReadOnly && /* 読み取り専用モード ではない */
		TRUE == bRet
	){
		/* ファイルの排他ロック */
		DoFileLock();
	}
	if( NULL != hwndProgress ){
		::ShowWindow( hwndProgress, SW_HIDE );
	}
	//	Sep. 7, 2003 genta
	//	ファイル更新の通知を元に戻す
	m_eWatchUpdate = wuSave;


	return bRet;
}

std::tstring CEditDoc::GetDlgInitialDir()
{
	if( IsValidPath() ){
		return GetFilePath();
	}
	else if( m_pShareData->m_Common.m_sEdit.m_eOpenDialogDir == OPENDIALOGDIR_CUR ){
		// 2002.10.25 Moca
		TCHAR pszCurDir[_MAX_PATH];
		int nCurDir = ::GetCurrentDirectory( _countof(pszCurDir), pszCurDir );
		if( 0 == nCurDir || _MAX_PATH < nCurDir ){
			return _T("");
		}
		else{
			return pszCurDir;
		}
	}else if( m_pShareData->m_Common.m_sEdit.m_eOpenDialogDir == OPENDIALOGDIR_MRU ){
		const CMRUFile cMRU;
		std::vector<LPCTSTR> vMRU = cMRU.GetPathList();
		if( !vMRU.empty() ){
			return vMRU[0];
		}else{
			TCHAR pszCurDir[_MAX_PATH];
			int nCurDir = ::GetCurrentDirectory( _countof(pszCurDir), pszCurDir );
			if( 0 == nCurDir || _MAX_PATH < nCurDir ){
				return _T("");
			}
			else{
				return pszCurDir;
			}
		}
	}else{
		TCHAR selDir[_MAX_PATH];
		CShareData::ExpandMetaToFolder( m_pShareData->m_Common.m_sEdit.m_OpenDialogSelDir , selDir, _countof(selDir) );
		return selDir;
	}
}

/* 「ファイルを開く」ダイアログ */
//	Mar. 30, 2003 genta	ファイル名未定時の初期ディレクトリをカレントフォルダに
bool CEditDoc::OpenFileDialog(
	HWND		hwndParent,
	const char*	pszOpenFolder,	//!< [in]  NULL以外を指定すると初期フォルダを指定できる
	char*		pszPath,		//!< [out] 開くファイルのパスを受け取るアドレス
	ECodeType*	pnCharCode,		//!< [out] 指定された文字コード種別を受け取るアドレス
	bool*		pbReadOnly		//!< [out] 読み取り専用か
)
{
	/* アクティブにする */
	ActivateFrameWindow( hwndParent );

	/* ファイルオープンダイアログの初期化 */
	m_cDlgOpenFile.Create(
		m_hInstance,
		hwndParent,
		"*.*",
		pszOpenFolder ? pszOpenFolder : GetDlgInitialDir().c_str(),	// 初期フォルダ
		CMRUFile().GetPathList(),
		CMRUFolder().GetPathList()
	);
	return m_cDlgOpenFile.DoModalOpenDlg( pszPath, pnCharCode, pbReadOnly );
}



/*! 「ファイル名を付けて保存」ダイアログ

	@param pszPath [out]	保存ファイル名
	@param pnCharCode [out]	保存文字コードセット
	@param pcEol [out]		保存改行コード

	@date 2001.02.09 genta	改行コードを示す引数追加
	@date 2003.03.30 genta	ファイル名未定時の初期ディレクトリをカレントフォルダに
	@date 2003.07.20 ryoji	BOMの有無を示す引数追加
	@date 2006.11.10 ryoji	ユーザー指定の拡張子を状況依存で変化させる
*/
BOOL CEditDoc::SaveFileDialog( char* pszPath, ECodeType* pnCharCode, CEol* pcEol, bool* pbBomExist )
{
	char	szDefaultWildCard[_MAX_PATH + 10];	// ユーザー指定拡張子
	char	szExt[_MAX_EXT];

	/* ファイル保存ダイアログの初期化 */
	/* ファイル名の無いファイルだったら、ppszMRU[0]をデフォルトファイル名として？ppszOPENFOLDERじゃない？ */
	// ファイル名の無いときはカレントフォルダをデフォルトにします。Mar. 30, 2003 genta
	// 掲示板要望 No.2699 (2003/02/05)
	if( !IsValidPath() ){
		// 2002.10.25 Moca さんのコードを流用 Mar. 23, 2003 genta
		strcpy(szDefaultWildCard, "*.txt");
		if( m_pShareData->m_Common.m_sFile.m_bNoFilterSaveNew )
			strcat(szDefaultWildCard, ";*.*");	// 全ファイル表示
	}else{
		_splitpath(GetFilePath(), NULL, NULL, NULL, szExt);
		if( szExt[0] == _T('.') && szExt[1] != _T('\0') ){
			strcpy(szDefaultWildCard, "*");
			strcat(szDefaultWildCard, szExt);
			if( m_pShareData->m_Common.m_sFile.m_bNoFilterSaveFile )
				strcat(szDefaultWildCard, ";*.*");	// 全ファイル表示
		}else{
			strcpy(szDefaultWildCard, "*.*");
		}
	}

	/* ダイアログを表示 */
	m_cDlgOpenFile.Create(
		m_hInstance,
		m_hWnd,
		szDefaultWildCard,
		GetDlgInitialDir().c_str(),	// 初期フォルダ
		CMRUFile().GetPathList(),		//	最近のファイル
		CMRUFolder().GetPathList()	//	最近のフォルダ
	);
	return m_cDlgOpenFile.DoModalSaveDlg( pszPath, pnCharCode, pcEol, pbBomExist );
}





/*! 共通設定 プロパティシート */
BOOL CEditDoc::OpenPropertySheet( int nPageNum/*, int nActiveItem*/ )
{
	int		i;
//	BOOL	bModify;
	
	// 2002.12.11 Moca この部分で行われていたデータのコピーをCPropCommonに移動・関数化
	// 共通設定の一時設定領域にSharaDataをコピーする
	m_cPropCommon.InitData();
	
	/* プロパティシートの作成 */
	if( m_cPropCommon.DoPropertySheet( nPageNum/*, nActiveItem*/ ) ){

		// 2002.12.11 Moca この部分で行われていたデータのコピーをCPropCommonに移動・関数化
		// ShareData に 設定を適用・コピーする
		// 2007.06.20 ryoji グループ化に変更があったときはグループIDをリセットする
		BOOL bGroup = (m_pShareData->m_Common.m_sTabBar.m_bDispTabWnd && !m_pShareData->m_Common.m_sTabBar.m_bDispTabWndMultiWin);
		m_cPropCommon.ApplyData();
		m_pcSMacroMgr->UnloadAll();	// 2007.10.19 genta マクロ登録変更を反映するため，読み込み済みのマクロを破棄する
		if( bGroup != (m_pShareData->m_Common.m_sTabBar.m_bDispTabWnd && !m_pShareData->m_Common.m_sTabBar.m_bDispTabWndMultiWin ) ){
			CShareData::getInstance()->ResetGroupId();
		}

		/* アクセラレータテーブルの再作成 */
		::SendMessage( m_pShareData->m_sHandles.m_hwndTray, MYWM_CHANGESETTING,  (WPARAM)0, (LPARAM)0 );

		/* フォントが変わった */
		for( i = 0; i < GetAllViewCount(); ++i ){
			m_pcEditViewArr[i]->m_cTipWnd.ChangeFont( &(m_pShareData->m_Common.m_sHelper.m_lf_kh) );
		}

		/* 設定変更を反映させる */
		CShareData::getInstance()->SendMessageToAllEditors( MYWM_CHANGESETTING, (WPARAM)0, (LPARAM)m_hwndParent, m_hwndParent );	/* 全編集ウィンドウへメッセージをポストする */

		return TRUE;
	}else{
		return FALSE;
	}
}



/*! タイプ別設定 プロパティシート */
BOOL CEditDoc::OpenPropertySheetTypes( int nPageNum, int nSettingType )
{
	m_cPropTypes.SetTypeData( m_pShareData->m_Types[nSettingType] );
	// Mar. 31, 2003 genta メモリ削減のためポインタに変更しProperySheet内で取得するように
	//m_cPropTypes.m_CKeyWordSetMgr = m_pShareData->m_CKeyWordSetMgr;

	/* プロパティシートの作成 */
	if( m_cPropTypes.DoPropertySheet( nPageNum ) ){
//		/* 変更されたか？ */
//		if( 0 == memcmp( &m_pShareData->m_Types[nSettingType], &m_cPropTypes.m_Types, sizeof( STypeConfig ) ) ){
//			/* 無変更 */
//			return FALSE;
//		}
//		/* 変更フラグ(タイプ別設定) のセット */
//		m_pShareData->m_nTypesModifyArr[nSettingType] = TRUE;
		/* 変更された設定値のコピー */
		int nTextWrapMethodOld = GetDocumentAttribute().m_nTextWrapMethod;
		m_cPropTypes.GetTypeData( m_pShareData->m_Types[nSettingType] );

		// 2008.06.01 nasukoji	テキストの折り返し位置変更対応
		// タイプ別設定を呼び出したウィンドウについては、タイプ別設定が変更されたら
		// 折り返し方法の一時設定適用中を解除してタイプ別設定を有効とする。
		if( nTextWrapMethodOld != GetDocumentAttribute().m_nTextWrapMethod )		// 設定が変更された
			m_bTextWrapMethodCurTemp = false;	// 一時設定適用中を解除

		/* アクセラレータテーブルの再作成 */
		::SendMessage( m_pShareData->m_sHandles.m_hwndTray, MYWM_CHANGESETTING,  (WPARAM)0, (LPARAM)0 );

		/* 設定変更を反映させる */
		CShareData::getInstance()->SendMessageToAllEditors( MYWM_CHANGESETTING, (WPARAM)0, (LPARAM)m_hwndParent, m_hwndParent );	/* 全編集ウィンドウへメッセージをポストする */

		return TRUE;
	}else{
		return FALSE;
	}
}



/* Undo(元に戻す)可能な状態か？ */
bool CEditDoc::IsEnableUndo( void )
{
	return m_cOpeBuf.IsEnableUndo();
}



/*! Redo(やり直し)可能な状態か？ */
bool CEditDoc::IsEnableRedo( void )
{
	return m_cOpeBuf.IsEnableRedo();
}




/*! クリップボードから貼り付け可能か？ */
BOOL CEditDoc::IsEnablePaste( void )
{
	UINT uFormatSakuraClip;
	uFormatSakuraClip = ::RegisterClipboardFormat( "SAKURAClip" );

	// 2008/02/16 クリップボードからのファイルパス貼り付け対応	bosagami	zlib/libpng license
	if( ::IsClipboardFormatAvailable( CF_OEMTEXT )
	 || ::IsClipboardFormatAvailable( CF_HDROP )
	 || ::IsClipboardFormatAvailable( uFormatSakuraClip )
	){
		return TRUE;
	}
	return FALSE;
}





/*! 親ウィンドウのタイトルを更新

	@date 2007.03.08 ryoji bKillFocusパラメータを除去
*/
void CEditDoc::UpdateCaption()
{
	if( !m_pcEditViewArr[m_nActivePaneIndex]->m_bDrawSWITCH )return;

	//キャプション文字列の生成 -> pszCap
	char	pszCap[1024];
	const char* pszFormat = NULL;
	if( !m_pcEditWnd->IsActiveApp() )	pszFormat = m_pShareData->m_Common.m_sWindow.m_szWindowCaptionInactive;
	else								pszFormat = m_pShareData->m_Common.m_sWindow.m_szWindowCaptionActive;
	ExpandParameter(
		pszFormat,
		pszCap,
		_countof( pszCap )
	);

	//キャプション更新
	::SetWindowText( m_hwndParent, pszCap );

	//@@@ From Here 2003.06.13 MIK
	//タブウインドウのファイル名を通知
	ExpandParameter( m_pShareData->m_Common.m_sTabBar.m_szTabWndCaption, pszCap, _countof( pszCap ));
	m_pcEditWnd->ChangeFileNameNotify( pszCap, m_szFilePath, m_bGrepMode );	// 2006.01.28 ryoji ファイル名、Grepモードパラメータを追加
	//@@@ To Here 2003.06.13 MIK
}




/*! バックアップの作成
	@author genta
	@date 2001.06.12 asa-o
		ファイルの時刻を元にバックアップファイル名を作成する機能
	@date 2001.12.11 MIK バックアップファイルをゴミ箱に入れる機能
	@date 2004.06.05 genta バックアップ対象ファイルを引数で与えるように．
		名前を付けて保存の時は自分のバックアップを作っても無意味なので．
		また，バックアップも保存も行わない選択肢を追加．
	@date 2005.11.26 aroka ファイル名生成をFormatBackUpPathに分離
	@date 2008.11.23 nasukoji パスが長すぎる場合への対応（戻り値4を追加）

	@retval 0 バックアップ作成失敗．
	@retval 1 バックアップ作成成功．
	@retval 2 バックアップ作成失敗．保存中断指示．
	@retval 3 ファイル操作エラーによるバックアップ作成失敗．
	@retval 4 バックアップ作成失敗．ファイルパスが長すぎる．
	
	@todo Advanced modeでの世代管理
*/
int CEditDoc::MakeBackUp(
	const TCHAR* target_file
)
{
	int		nRet;

	/* バックアップソースの存在チェック */
	//	Aug. 21, 2005 genta 書き込みアクセス権がない場合も
	//	ファイルがない場合と同様に何もしない
	if( (_taccess( target_file, 2 )) == -1 ){
		return 0;
	}

	if( m_pShareData->m_Common.m_sBackup.m_bBackUpFolder ){	/* 指定フォルダにバックアップを作成する */
		//	Aug. 21, 2005 genta 指定フォルダがない場合に警告
		if( (!fexist( m_pShareData->m_Common.m_sBackup.m_szBackUpFolder ))){
			if( ::TopConfirmMessage(
				m_hWnd,
				_T("バックアップエラー"),
				_T("以下のバックアップフォルダが見つかりません．\n%s\n")
				_T("バックアップを作成せずに上書き保存してよろしいですか．"),
				m_pShareData->m_Common.m_sBackup.m_szBackUpFolder
			) == IDYES ){
				return 0;//	保存継続
			}
			else {
				return 2;// 保存中断
			}
		}
	}

	TCHAR	szPath[_MAX_PATH];
	if( !FormatBackUpPath( szPath, _countof(szPath), target_file ) ){
		return 4;	// 作成すべきバックアップファイルのパスが長すぎる
	}

	//@@@ 2002.03.23 start ネットワーク・リムーバブルドライブの場合はごみ箱に放り込まない
	bool dustflag = false;
	if( m_pShareData->m_Common.m_sBackup.m_bBackUpDustBox ){
		dustflag = !IsLocalDrive( szPath );
	}
	//@@@ 2002.03.23 end

	if( m_pShareData->m_Common.m_sBackup.m_bBackUpDialog ){	/* バックアップの作成前に確認 */
		ConfirmBeep();
		if( m_pShareData->m_Common.m_sBackup.m_bBackUpDustBox && !dustflag ){	//共通設定：バックアップファイルをごみ箱に放り込む	//@@@ 2001.12.11 add start MIK	//2002.03.23
			nRet = ::MYMESSAGEBOX(
				m_hWnd,
				MB_YESNO/*CANCEL*/ | MB_ICONQUESTION | MB_TOPMOST,
				_T("バックアップ作成の確認"),
				_T("変更される前に、バックアップファイルを作成します。\n")
				_T("よろしいですか？  [いいえ(N)] を選ぶと作成せずに上書き（または名前を付けて）保存になります。\n")
				_T("\n")
				_T("%s\n")
				_T("    ↓\n")
				_T("%s\n")
				_T("\n")
				_T("作成したバックアップファイルをごみ箱に放り込みます。\n"),
				target_file,
				szPath
			);
		}
		else{	//@@@ 2001.12.11 add end MIK
			nRet = ::MYMESSAGEBOX(
				m_hWnd,
				MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
				_T("バックアップ作成の確認"),
				_T("変更される前に、バックアップファイルを作成します。\n")
				_T("よろしいですか？  [いいえ(N)] を選ぶと作成せずに上書き（または名前を付けて）保存になります。\n")
				_T("\n")
				_T("%s\n")
				_T("    ↓\n")
				_T("%s\n")
				_T("\n"),
				target_file,
				szPath
			);	//Jul. 06, 2001 jepro [名前を付けて保存] の場合もあるのでメッセージを修正
		}	//@@@ 2001.12.11 add MIK
		//	Jun.  5, 2005 genta 戻り値変更
		if( IDNO == nRet ){
			return 0;//	保存継続
		}else if( IDCANCEL == nRet ){
			return 2;// 保存中断
		}
	}

	//	From Here Aug. 16, 2000 genta
	//	Jun.  5, 2005 genta 1の拡張子を残す版を追加
	if( m_pShareData->m_Common.m_sBackup.GetBackupType() == 3 ||
		m_pShareData->m_Common.m_sBackup.GetBackupType() == 6 ){
		//	既に存在するBackupをずらす処理
		int				i;

		//	ファイル検索用
		HANDLE			hFind;
		WIN32_FIND_DATA	fData;

		TCHAR*	pBase = szPath + _tcslen( szPath ) - 2;	//	2: 拡張子の最後の2桁の意味

		//------------------------------------------------------------------
		//	1. 該当ディレクトリ中のbackupファイルを1つずつ探す
		for( i = 0; i <= 99; i++ ){	//	最大値に関わらず，99（2桁の最大値）まで探す
			//	ファイル名をセット
			wsprintf( pBase, _T("%02d"), i );

			hFind = ::FindFirstFile( szPath, &fData );
			if( hFind == INVALID_HANDLE_VALUE ){
				//	検索に失敗した == ファイルは存在しない
				break;
			}
			::FindClose( hFind );
			//	見つかったファイルの属性をチェック
			//	は面倒くさいからしない．
			//	同じ名前のディレクトリがあったらどうなるのだろう...
		}
		--i;

		//------------------------------------------------------------------
		//	2. 最大値から制限数-1番までを削除
		int boundary = m_pShareData->m_Common.m_sBackup.GetBackupCount();
		boundary = boundary > 0 ? boundary - 1 : 0;	//	最小値は0
		//::MessageBox( NULL, pBase, "書き換え場所", MB_OK );

		for( ; i >= boundary; --i ){
			//	ファイル名をセット
			wsprintf( pBase, _T("%02d"), i );
			if( ::DeleteFile( szPath ) == 0 ){
				::MessageBox( m_hWnd, szPath, _T("削除失敗"), MB_OK );
				//	Jun.  5, 2005 genta 戻り値変更
				//	失敗しても保存は継続
				return 0;
				//	失敗した場合
				//	後で考える
			}
		}

		//	この位置でiは存在するバックアップファイルの最大番号を表している．

		//	3. そこから0番まではコピーしながら移動
		TCHAR szNewPath[MAX_PATH];
		TCHAR *pNewNrBase;

		_tcscpy( szNewPath, szPath );
		pNewNrBase = szNewPath + _tcslen( szNewPath ) - 2;

		for( ; i >= 0; --i ){
			//	ファイル名をセット
			wsprintf( pBase, _T("%02d"), i );
			wsprintf( pNewNrBase, _T("%02d"), i + 1 );

			//	ファイルの移動
			if( ::MoveFile( szPath, szNewPath ) == 0 ){
				//	失敗した場合
				//	後で考える
				::MessageBox( m_hWnd, szPath, _T("移動失敗"), MB_OK );
				//	Jun.  5, 2005 genta 戻り値変更
				//	失敗しても保存は継続
				return 0;
			}
		}
	}
	//	To Here Aug. 16, 2000 genta

	//::MessageBox( NULL, szPath, "直前のバックアップファイル", MB_OK );
	/* バックアップの作成 */
	//	Aug. 21, 2005 genta 現在のファイルではなくターゲットファイルをバックアップするように
	TCHAR	szDrive[_MAX_DIR];
	TCHAR	szDir[_MAX_DIR];
	TCHAR	szFname[_MAX_FNAME];
	TCHAR	szExt[_MAX_EXT];
	_splitpath( szPath, szDrive, szDir, szFname, szExt );
	TCHAR szPath2[MAX_PATH];
	wsprintf( szPath2, _T("%s%s"), szDrive, szDir );
		HANDLE			hFind;
		WIN32_FIND_DATA	fData;

	hFind = ::FindFirstFile( szPath2, &fData );
	if( hFind == INVALID_HANDLE_VALUE ){
		//	検索に失敗した == ファイルは存在しない
		::CreateDirectory( szPath2, NULL );
	}
	::FindClose( hFind );

	if( ::CopyFile( target_file, szPath, FALSE ) ){
		/* 正常終了 */
		//@@@ 2001.12.11 start MIK
		if( m_pShareData->m_Common.m_sBackup.m_bBackUpDustBox && !dustflag ){	//@@@ 2002.03.23 ネットワーク・リムーバブルドライブでない
			TCHAR	szDustPath[_MAX_PATH+1];
			_tcscpy(szDustPath, szPath);
			szDustPath[_tcslen(szDustPath) + 1] = _T('\0');
			SHFILEOPSTRUCT	fos;
			fos.hwnd   = m_hWnd;
			fos.wFunc  = FO_DELETE;
			fos.pFrom  = szDustPath;
			fos.pTo    = NULL;
			fos.fFlags = FOF_ALLOWUNDO | FOF_SIMPLEPROGRESS | FOF_NOCONFIRMATION;	//ダイアログなし
			fos.fAnyOperationsAborted = true; //false;
			fos.hNameMappings = NULL;
			fos.lpszProgressTitle = NULL; //"バックアップファイルをごみ箱に移動しています...";
			if( ::SHFileOperation(&fos) == 0 ){
				/* 正常終了 */
			}else{
				/* エラー終了 */
			}
		}
		//@@@ 2001.12.11 end MIK
	}else{
		/* エラー終了 */
		//	Jun.  5, 2005 genta 戻り値変更
		return 3;
	}
	//	Jun.  5, 2005 genta 戻り値変更
	return 1;
}

/*! バックアップの作成

	@author aroka
	@date 2005.11.29 aroka
		MakeBackUpから分離．書式を元にバックアップファイル名を作成する機能追加
	@date 2008.11.23 nasukoji	パスが長すぎる場合への対応
	@date 2009.10.10 aroka	階層が浅いときに落ちるバグの対応
	@date 2012.12.26 aroka	詳細設定のファイル保存日時と現在時刻で書式を合わせる対応

	@retval true  成功
	@retval false	バッファ不足
	
	@todo Advanced modeでの世代管理
*/
bool CEditDoc::FormatBackUpPath(
	TCHAR*			szNewPath,		//!< [out] バックアップ先パス名
	DWORD 			newPathCount,	//!< [in]  szNewPathのサイズ
	const TCHAR*	target_file		//!< [in]  バックアップ元パス名
)
{
	TCHAR	szDrive[_MAX_DIR];
	TCHAR	szDir[_MAX_DIR];
	TCHAR	szFname[_MAX_FNAME];
	TCHAR	szExt[_MAX_EXT];
	TCHAR	szTempPath[1024];		// パス名作成用の一時バッファ（_MAX_PATHよりある程度大きいこと）

	bool	bOverflow = false;		// バッファオーバーフロー

	/* パスの分解 */
	_tsplitpath( target_file, szDrive, szDir, szFname, szExt );

	if( m_pShareData->m_Common.m_sBackup.m_bBackUpFolder ){	/* 指定フォルダにバックアップを作成する */
		_tcscpy( szTempPath, m_pShareData->m_Common.m_sBackup.m_szBackUpFolder );
		/* フォルダの最後が半角かつ'\\'でない場合は、付加する */
		AddLastYenFromDirectoryPath( szTempPath );
	}
	else{
		wsprintf( szTempPath, _T("%s%s"), szDrive, szDir );
	}

	/* 相対フォルダを挿入 */
	if( !m_pShareData->m_Common.m_sBackup.m_bBackUpPathAdvanced ){
		time_t	ltime;
		struct	tm *today, *gmt;
		TCHAR	szTime[64];
		TCHAR	szForm[64];
		TCHAR*	pBase;

		pBase = szTempPath + _tcslen( szTempPath );

		/* バックアップファイル名のタイプ 1=(.bak) 2=*_日付.* */
		switch( m_pShareData->m_Common.m_sBackup.GetBackupType() ){
		case 1:
			wsprintf( pBase, _T("%s.bak"), szFname );
			break;
		case 5: //	Jun.  5, 2005 genta 1の拡張子を残す版
			wsprintf( pBase, _T("%s%s.bak"), szFname, szExt );
			break;
		case 2:	//	日付，時刻
			_tzset();
			_strdate( szTime );
			time( &ltime );				/* システム時刻を得ます */
			gmt = gmtime( &ltime );		/* 万国標準時に変換する */
			today = localtime( &ltime );/* 現地時間に変換する */

			_tcscpy( szForm, _T("") );
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_YEAR) ){	/* バックアップファイル名：日付の年 */
				strcat( szForm, "%Y" );
			}
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_MONTH) ){	/* バックアップファイル名：日付の月 */
				strcat( szForm, "%m" );
			}
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_DAY) ){	/* バックアップファイル名：日付の日 */
				strcat( szForm, "%d" );
			}
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_HOUR) ){	/* バックアップファイル名：日付の時 */
				strcat( szForm, "%H" );
			}
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_MIN) ){	/* バックアップファイル名：日付の分 */
				strcat( szForm, "%M" );
			}
			if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_SEC) ){	/* バックアップファイル名：日付の秒 */
				strcat( szForm, "%S" );
			}
			/* YYYYMMDD時分秒 形式に変換 */
			strftime( szTime, _countof( szTime ) - 1, szForm, today );
			wsprintf( pBase, _T("%s_%s%s"), szFname, szTime, szExt );
			break;
	//	2001/06/12 Start by asa-o: ファイルに付ける日付を前回の保存時(更新日時)にする
		case 4:	//	日付，時刻
			{
				CFileTime ctimeLastWrite;
				GetLastWriteTimestamp( target_file, &ctimeLastWrite );

				_tcscpy( szTime, _T("") );
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_YEAR) ){	/* バックアップファイル名：日付の年 */
					wsprintf(szTime,_T("%d"),ctimeLastWrite->wYear);
				}
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_MONTH) ){	/* バックアップファイル名：日付の月 */
					wsprintf(szTime,_T("%s%02d"),szTime,ctimeLastWrite->wMonth);
				}
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_DAY) ){	/* バックアップファイル名：日付の日 */
					wsprintf(szTime,_T("%s%02d"),szTime,ctimeLastWrite->wDay);
				}
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_HOUR) ){	/* バックアップファイル名：日付の時 */
					wsprintf(szTime,_T("%s%02d"),szTime,ctimeLastWrite->wHour);
				}
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_MIN) ){	/* バックアップファイル名：日付の分 */
					wsprintf(szTime,_T("%s%02d"),szTime,ctimeLastWrite->wMinute);
				}
				if( m_pShareData->m_Common.m_sBackup.GetBackupOpt(BKUP_SEC) ){	/* バックアップファイル名：日付の秒 */
					wsprintf(szTime,_T("%s%02d"),szTime,ctimeLastWrite->wSecond);
				}
				wsprintf( pBase, _T("%s_%s%s"), szFname, szTime, szExt );
			}
			break;
	// 2001/06/12 End

		case 3: //	?xx : xx = 00~99, ?は任意の文字
		case 6: //	Jun.  5, 2005 genta 3の拡張子を残す版
			//	Aug. 15, 2000 genta
			//	ここでは作成するバックアップファイル名のみ生成する．
			//	ファイル名のRotationは確認ダイアログの後で行う．
			{
				//	Jun.  5, 2005 genta 拡張子を残せるように処理起点を操作する
				TCHAR* ptr;
				if( m_pShareData->m_Common.m_sBackup.GetBackupType() == 3 ){
					ptr = szExt;
				}
				else {
					ptr = szExt + _tcslen( szExt );
				}
				*ptr   = _T('.');
				*++ptr = m_pShareData->m_Common.m_sBackup.GetBackupExtChar();
				*++ptr = _T('0');
				*++ptr = _T('0');
				*++ptr = _T('\0');
			}
			wsprintf( pBase, _T("%s%s"), szFname, szExt );
			break;
		}

	}else{ // 詳細設定使用する
		TCHAR szFormat[1024];

		switch( m_pShareData->m_Common.m_sBackup.GetBackupTypeAdv() ){
		case 4:	//	ファイルの日付，時刻
			{
				// 2005.10.20 ryoji FindFirstFileを使うように変更
				CFileTime ctimeLastWrite;
				GetLastWriteTimestamp( target_file, &ctimeLastWrite );
				if( !GetDateTimeFormat( szFormat, _countof(szFormat), m_pShareData->m_Common.m_sBackup.m_szBackUpPathAdvanced , ctimeLastWrite.GetSYSTEMTIME() ) ){
					return false;
				}
			}
			break;
		case 2:	//	現在の日付，時刻
		default:
			{
				// 2012.12.26 aroka	詳細設定のファイル保存日時と現在時刻で書式を合わせる
				SYSTEMTIME	SystemTime;
				::GetSystemTime(&SystemTime);			// 現在時刻を取得

				GetDateTimeFormat( szFormat, sizeof(szFormat), m_pShareData->m_Common.m_sBackup.m_szBackUpPathAdvanced , SystemTime );
			}
			break;
		}

		{
			// make keys
			// $0-$9に対応するフォルダ名を切り出し
			TCHAR keybuff[1024];
			_tcscpy( keybuff, szDir );
			CutLastYenFromDirectoryPath( keybuff );

			TCHAR *folders[10];
			{
				//	Jan. 9, 2006 genta VC6対策
				int idx;
				for( idx=0; idx<10; ++idx ){
					folders[idx] = _T("");		// 2009.10.10 aroka	階層が浅いときに落ちるバグの対応
				}
				folders[0] = szFname;

				for( idx=1; idx<10; ++idx ){
					TCHAR *cp;
					cp = sjis_strrchr2((unsigned char*)keybuff, _T('\\'), _T('\\'));
					if( cp != NULL ){
						folders[idx] = cp+1;
						*cp = _T('\0');
					}
					else{
						break;
					}
				}
			}
			{
				// $0-$9を置換
				//_tcscpy( szNewPath, _T("") );
				TCHAR *q= szFormat;
				TCHAR *q2 = szFormat;
				while( *q ){
					if( *q==_T('$') ){
						++q;
						if( isdigit(*q) ){
							q[-1] = _T('\0');

							// 2008.11.25 nasukoji	バッファオーバーフローチェック
							if( _tcslen( szTempPath ) + _tcslen( q2 ) + _tcslen( folders[*q-_T('0')] ) >= newPathCount ){
								bOverflow = true;
								break;
							}

							if( _T('\\') == *q2 ){
								// 2010.04.13 Moca \の重複チェック C:\backup\\dirになるのを先に防ぐ
								// ただしネットワークパスを取り除くと困るので、3文字以上
								int nTempPathLen = strlen( szTempPath );
								if( 3 <= nTempPathLen && *::CharPrev( szTempPath, &szTempPath[nTempPathLen] ) == _T('\\') ){
									q2 +=1; // \を飛ばす
								}
							}

							strcat( szTempPath, q2 );
							//if( folders[*q-'0'] != 0 ){	// 2009.10.10 aroka	バグ対応でチェック不要になった
							strcat( szTempPath, folders[*q-_T('0')] );
							//}
							q2 = q+1;
						}
					}
					++q;
				}

				// 2008.11.25 nasukoji	バッファオーバーフローチェック
				if( !bOverflow ){
					if( _tcslen( szTempPath ) + _tcslen( q2 ) >= newPathCount ){
						bOverflow = true;
					}else{
						strcat( szTempPath, q2 );
					}
				}
			}
		}

		if( !bOverflow ){
			TCHAR temp[1024];
			TCHAR *cp;
			//	2006.03.25 Aroka szExt[0] == '\0'のときのオーバラン問題を修正
			TCHAR *ep = (szExt[0]!=0) ? &szExt[1] : &szExt[0];

			while( strchr( szTempPath, _T('*') ) ){
				_tcscpy( temp, szTempPath );
				cp = strchr( temp, _T('*') );
				*cp = 0;

				// 2008.11.25 nasukoji	バッファオーバーフローチェック
				if( cp - temp + _tcslen( ep ) + _tcslen( cp + 1 ) >= newPathCount ){
					bOverflow = true;
					break;
				}

				wsprintf( szTempPath, _T("%s%s%s"), temp, ep, cp+1 );
			}

			if( !bOverflow ){
				//	??はバックアップ連番にしたいところではあるが，
				//	連番処理は末尾の2桁にしか対応していないので
				//	使用できない文字?を_に変換してお茶を濁す
				while(( cp = strchr( szTempPath, _T('?') ) ) != NULL){
					*cp = _T('_');
				}
			}
		}
	}

	// 作成したパスがszNewPathに収まらなければエラー
	if( bOverflow || _tcslen( szTempPath ) >= newPathCount ){
		return false;
	}

	_tcscpy( szNewPath, szTempPath );	// 作成したパスをコピー

	return true;
}

/* ファイルの排他ロック */
void CEditDoc::DoFileLock( void )
{
	char*	pszMode;
	int		nAccessMode;
	BOOL	bCheckOnly;

	/* ロックしている */
	if( NULL != m_hLockedFile ){
		/* ロック解除 */
		::_lclose( m_hLockedFile );
		m_hLockedFile = NULL;
	}

	/* ファイルが存在しない */
	if( !fexist( GetFilePath() ) ){
		/* ファイルの排他制御モード */
		m_nFileShareModeOld = 0;
		return;
	}else{
		/* ファイルの排他制御モード */
		m_nFileShareModeOld = m_pShareData->m_Common.m_sFile.m_nFileShareMode;
	}


	/* ファイルを開いていない */
	if( !IsValidPath() ){
		return;
	}
	/* 読み取り専用モード */
	if( m_bReadOnly ){
		return;
	}


	nAccessMode = 0;
	if( m_pShareData->m_Common.m_sFile.m_nFileShareMode == OF_SHARE_DENY_WRITE ||
		m_pShareData->m_Common.m_sFile.m_nFileShareMode == OF_SHARE_EXCLUSIVE ){
		bCheckOnly = FALSE;
	}else{
		/* 排他制御しないけどロックされているかのチェックは行うのでreturnしない */
//		return;
		bCheckOnly = TRUE;
	}
	/* 書込み禁止かどうか調べる */
	if( -1 == _taccess( GetFilePath(), 2 ) ){	/* アクセス権：書き込み許可 */
		m_hLockedFile = NULL;
		/* 親ウィンドウのタイトルを更新 */
		UpdateCaption();
		return;
	}


	m_hLockedFile = ::_lopen( GetFilePath(), OF_READWRITE );
	_lclose( m_hLockedFile );
	if( HFILE_ERROR == m_hLockedFile ){
		WarningBeep();
		TopWarningMessage(
			m_hWnd,
			_T("%s\nは現在他のプロセスによって書込みが禁止されています。"),
			IsValidPath() ? GetFilePath() : _T("(無題)")
		);
		m_hLockedFile = NULL;
		/* 親ウィンドウのタイトルを更新 */
		UpdateCaption();
		return;
	}
	m_hLockedFile = ::_lopen( GetFilePath(), nAccessMode | m_pShareData->m_Common.m_sFile.m_nFileShareMode );
	if( HFILE_ERROR == m_hLockedFile ){
		switch( m_pShareData->m_Common.m_sFile.m_nFileShareMode ){
		case OF_SHARE_EXCLUSIVE:	/* 読み書き */
			pszMode = "読み書き禁止モード";
			break;
		case OF_SHARE_DENY_WRITE:	/* 書き */
			pszMode = "書き込み禁止モード";
			break;
		default:
			pszMode = "未定義のモード（問題があります）";
			break;
		}
		WarningBeep();
		TopWarningMessage(
			m_hWnd,
			_T("%s\nを%sでロックできませんでした。\n現在このファイルに対する排他制御は無効となります。"),
			IsValidPath() ? GetFilePath() : _T("(無題)"),
			pszMode
		);
		/* 親ウィンドウのタイトルを更新 */
		UpdateCaption();
		return;
	}
	/* 排他制御しないけどロックされているかのチェックは行う場合 */
	if( bCheckOnly ){
		/* ロックを解除する */
		DoFileUnlock();

	}
	return;
}


/* ファイルの排他ロック解除 */
void CEditDoc::DoFileUnlock( void )
{
	if( NULL != m_hLockedFile ){
		/* ロック解除 */
		::_lclose( m_hLockedFile );
		m_hLockedFile = NULL;
		/* ファイルの排他制御モード */
		m_nFileShareModeOld = 0;
	}
	return;
}

/*
	C関数リスト作成はCEditDoc_FuncList1.cppへ移動
*/

/*! PL/SQL関数リスト作成 */
void CEditDoc::MakeFuncList_PLSQL( CFuncInfoArr* pcFuncInfoArr )
{
	const char*	pLine;
	int			nLineLen;
	int			nLineCount;
	int			i;
	int			nCharChars;
	char		szWordPrev[100];
	char		szWord[100];
	int			nWordIdx = 0;
	int			nMaxWordLeng = 70;
	int			nMode;
	char		szFuncName[100];
	int			nFuncLine;
	int			nFuncId;
	int			nFuncNum;
	int			nFuncOrProc = 0;
	int			nParseCnt = 0;

	szWordPrev[0] = '\0';
	szWord[nWordIdx] = '\0';
	nMode = 0;
	nFuncNum = 0;
	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		for( i = 0; i < nLineLen; ++i ){
			/* 1バイト文字だけを処理する */
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, i );
			if( 0 == nCharChars ){
				nCharChars = 1;
			}
//			if( 1 < nCharChars ){
//				i += (nCharChars - 1);
//				continue;
//			}
			/* シングルクォーテーション文字列読み込み中 */
			if( 20 == nMode ){
				if( '\'' == pLine[i] ){
					if( i + 1 < nLineLen && '\'' == pLine[i + 1] ){
						++i;
					}else{
						nMode = 0;
						continue;
					}
				}else{
				}
			}else
			/* コメント読み込み中 */
			if( 8 == nMode ){
				if( i + 1 < nLineLen && '*' == pLine[i] &&  '/' == pLine[i + 1] ){
					++i;
					nMode = 0;
					continue;
				}else{
				}
			}else
			/* 単語読み込み中 */
			if( 1 == nMode ){
				if( (1 == nCharChars && (
					'_' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )
					) )
				 || 2 == nCharChars
				){
//					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);
					}
				}else{
					if( 0 == nParseCnt && 0 == my_stricmp( szWord, "FUNCTION" ) ){
						nFuncOrProc = 1;
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;

					}else
					if( 0 == nParseCnt && 0 == my_stricmp( szWord, "PROCEDURE" ) ){
						nFuncOrProc = 2;
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}else
					if( 0 == nParseCnt && 0 == my_stricmp( szWord, "PACKAGE" ) ){
						nFuncOrProc = 3;
						nParseCnt = 1;
						nFuncLine = nLineCount + 1;
					}else
					if( 1 == nParseCnt && 3 == nFuncOrProc && 0 == my_stricmp( szWord, "BODY" ) ){
						nFuncOrProc = 4;
						nParseCnt = 1;
					}else
					if( 1 == nParseCnt ){
						if( 1 == nFuncOrProc ||
							2 == nFuncOrProc ||
							3 == nFuncOrProc ||
							4 == nFuncOrProc ){
							++nParseCnt;
							strcpy( szFuncName, szWord );
//						}else
//						if( 3 == nFuncOrProc ){

						}
					}else
					if( 2 == nParseCnt ){
						if( 0 == my_stricmp( szWord, "IS" ) ){
							if( 1 == nFuncOrProc ){
								nFuncId = 11;	/* ファンクション本体 */
							}else
							if( 2 == nFuncOrProc ){
								nFuncId = 21;	/* プロシージャ本体 */
							}else
							if( 3 == nFuncOrProc ){
								nFuncId = 31;	/* パッケージ仕様部 */
							}else
							if( 4 == nFuncOrProc ){
								nFuncId = 41;	/* パッケージ本体 */
							}
							++nFuncNum;
							/*
							  カーソル位置変換
							  物理位置(行頭からのバイト数、折り返し無し行位置)
							  →
							  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
							*/
							int		nPosX;
							int		nPosY;
							m_cLayoutMgr.LogicToLayout(
								0,
								nFuncLine - 1,
								&nPosX,
								&nPosY
							);
							pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1, szFuncName, nFuncId );
							nParseCnt = 0;
						}
						if( 0 == my_stricmp( szWord, "AS" ) ){
							if( 3 == nFuncOrProc ){
								nFuncId = 31;	/* パッケージ仕様部 */
								++nFuncNum;
								/*
								  カーソル位置変換
								  物理位置(行頭からのバイト数、折り返し無し行位置)
								  →
								  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
								*/
								int		nPosX;
								int		nPosY;
								m_cLayoutMgr.LogicToLayout(
									0,
									nFuncLine - 1,
									&nPosX,
									&nPosY
								);
								pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1 , szFuncName, nFuncId );
								nParseCnt = 0;
							}else
							if( 4 == nFuncOrProc ){
								nFuncId = 41;	/* パッケージ本体 */
								++nFuncNum;
								/*
								  カーソル位置変換
								  物理位置(行頭からのバイト数、折り返し無し行位置)
								  →
								  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
								*/
								int		nPosX;
								int		nPosY;
								m_cLayoutMgr.LogicToLayout(
									0,
									nFuncLine - 1,
									&nPosX,
									&nPosY
								);
								pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1 , szFuncName, nFuncId );
								nParseCnt = 0;
							}
						}
					}
					strcpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = '\0';
					nMode = 0;
					i--;
					continue;
				}
			}else
			/* 記号列読み込み中 */
			if( 2 == nMode ){
				if( '_' == pLine[i] ||
					'~' == pLine[i] ||
					('a' <= pLine[i] &&	pLine[i] <= 'z' )||
					('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
					('0' <= pLine[i] &&	pLine[i] <= '9' )||
					'\t' == pLine[i] ||
					 ' ' == pLine[i] ||
					  CR == pLine[i] ||
					  LF == pLine[i] ||
					 '{' == pLine[i] ||
					 '}' == pLine[i] ||
					 '(' == pLine[i] ||
					 ')' == pLine[i] ||
					 ';' == pLine[i] ||
					'\'' == pLine[i] ||
					 '/' == pLine[i] ||
					 '-' == pLine[i]
				){
					strcpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = '\0';
					nMode = 0;
					i--;
					continue;
				}else{
//					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);
					}
				}
			}else
			/* 長過ぎる単語無視中 */
			if( 999 == nMode ){
				/* 空白やタブ記号等を飛ばす */
				if( '\t' == pLine[i] ||
					 ' ' == pLine[i] ||
					  CR == pLine[i] ||
					  LF == pLine[i]
				){
					nMode = 0;
					continue;
				}
			}else
			/* ノーマルモード */
			if( 0 == nMode ){
				/* 空白やタブ記号等を飛ばす */
				if( '\t' == pLine[i] ||
					 ' ' == pLine[i] ||
					  CR == pLine[i] ||
					  LF == pLine[i]
				){
					continue;
				}else
				if( i < nLineLen - 1 && '-' == pLine[i] &&  '-' == pLine[i + 1] ){
					break;
				}else
				if( i < nLineLen - 1 && '/' == pLine[i] &&  '*' == pLine[i + 1] ){
					++i;
					nMode = 8;
					continue;
				}else
				if( '\'' == pLine[i] ){
					nMode = 20;
					continue;
				}else
				if( ';' == pLine[i] ){
					if( 2 == nParseCnt ){
						if( 1 == nFuncOrProc ){
							nFuncId = 10;	/* ファンクション宣言 */
						}else{
							nFuncId = 20;	/* プロシージャ宣言 */
						}
						++nFuncNum;
						/*
						  カーソル位置変換
						  物理位置(行頭からのバイト数、折り返し無し行位置)
						  →
						  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
						*/
						int		nPosX;
						int		nPosY;
						m_cLayoutMgr.LogicToLayout(
							0,
							nFuncLine - 1,
							&nPosX,
							&nPosY
						);
						pcFuncInfoArr->AppendData( nFuncLine, nPosY + 1 , szFuncName, nFuncId );
						nParseCnt = 0;
					}
					nMode = 0;
					continue;
				}else{
					if( (1 == nCharChars && (
						'_' == pLine[i] ||
						'~' == pLine[i] ||
						('a' <= pLine[i] &&	pLine[i] <= 'z' )||
						('A' <= pLine[i] &&	pLine[i] <= 'Z' )||
						('0' <= pLine[i] &&	pLine[i] <= '9' )
						) )
					 || 2 == nCharChars
					){
						strcpy( szWordPrev, szWord );
						nWordIdx = 0;

//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);

						nMode = 1;
					}else{
						strcpy( szWordPrev, szWord );
						nWordIdx = 0;
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';

						memcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = '\0';
						nWordIdx += (nCharChars);

						nMode = 2;
					}
				}
			}
			i += (nCharChars - 1);
		}
	}
	return;
}





/*!	テキスト・トピックリスト作成
	
	@date 2002.04.01 YAZAKI CDlgFuncList::SetText()を使用するように改訂。
	@date 2002.11.03 Moca 階層が最大値を超えるとバッファオーバーランするのを修正
		最大値以上は追加せずに無視する
*/
void CEditDoc::MakeTopicList_txt( CFuncInfoArr* pcFuncInfoArr )
{
	const unsigned char*	pLine;
	int						nLineLen;
	int						nLineCount;
	int						i;
	int						j;
	int						nCharChars;
	int						nCharChars2;
	const char*				pszStarts;
	int						nStartsLen;
	char*					pszText;


	pszStarts = m_pShareData->m_Common.m_sFormat.m_szMidashiKigou; 	/* 見出し記号 */
	nStartsLen = lstrlen( pszStarts );

	/*	ネストの深さは、nMaxStackレベルまで、ひとつのヘッダは、最長32文字まで区別
		（32文字まで同じだったら同じものとして扱います）
	*/
	const int nMaxStack = 32;	//	ネストの最深
	int nDepth = 0;				//	いまのアイテムの深さを表す数値。
	char pszStack[nMaxStack][32];
	char szTitle[32];			//	一時領域
	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = (const unsigned char *)m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		if( NULL == pLine ){
			break;
		}
		for( i = 0; i < nLineLen; ++i ){
			if( pLine[i] == ' ' ||
				pLine[i] == '\t'){
				continue;
			}else
			if( i + 1 < nLineLen && pLine[i] == 0x81 && pLine[i + 1] == 0x40 ){
				++i;
				continue;
			}
			break;
		}
		if( i >= nLineLen ){
			continue;
		}
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pLine, nLineLen, i );
		for( j = 0; j < nStartsLen; j+=nCharChars2 ){
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars2 = CMemory::GetSizeOfChar( pszStarts, nStartsLen, j );
			if( nCharChars == nCharChars2 ){
				if( 0 == memcmp( &pLine[i], &pszStarts[j], nCharChars ) ){
					strncpy( szTitle, &pszStarts[j], nCharChars);	//	szTitleに保持。
					szTitle[nCharChars] = '\0';
					break;
				}
			}
		}
		if( j >= nStartsLen ){
			continue;
		}
		/* 見出し文字に(が含まれていることが前提になっている! */
		if( nCharChars == 1 && pLine[i] == '(' ){
			if( pLine[i + 1] >= '0' && pLine[i + 1] <= '9' )  {
				strcpy( szTitle, "(0)" );
			}
			else if ( pLine[i + 1] >= 'A' && pLine[i + 1] <= 'Z' ) {
				strcpy( szTitle, "(A)" );
			}
			else if ( pLine[i + 1] >= 'a' && pLine[i + 1] <= 'z' ) {
				strcpy( szTitle, "(a)" );
			}
			else {
				continue;
			}
		}else
		if( 2 == nCharChars ){
			// 2003.06.28 Moca 1桁目から始まっていないと同一レベルと認識されずに
			//	どんどん子ノードになってしまうのを，字下げによらず同一レベルと認識されるように
			/* 全角数字 */
			if( pLine[i] == 0x82 && ( pLine[i + 1] >= 0x4f && pLine[i + 1] <= 0x58 ) ) {
				strcpy( szTitle, "０" );
			}
			/* �@〜�S */
			else if( pLine[i] == 0x87 && ( pLine[i + 1] >= 0x40 && pLine[i + 1] <= 0x53 ) ){
				strcpy( szTitle, "�@" );
			}
			/* �T〜�] */
			else if( pLine[i] == 0x87 && ( pLine[i + 1] >= 0x54 && pLine[i + 1] <= 0x5d ) ){
				strcpy( szTitle, "�T" );
			}
			// 2003.06.28 Moca 漢数字も同一階層に
			//	漢数字が異なる＝番号が異なると異なる見出し記号と認識されていたのを
			//	皆同じ階層と識別されるように
			else{
				char szCheck[3];
				szCheck[0] = pLine[i];
				szCheck[1] = pLine[i + 1];
				szCheck[2] = '\0';
				/* 一〜十 */
				if( NULL != strstr( "〇一二三四五六七八九十百零壱弐参伍", szCheck ) ){
					strcpy( szTitle, "一" );
				}
			}
		}
		/*	「見出し記号」に含まれる文字で始まるか、
			(0、(1、...(9、(A、(B、...(Z、(a、(b、...(z
			で始まる行は、アウトライン結果に表示する。
		*/
		pszText = new char[nLineLen + 1];
		memcpy( pszText, (const char *)&pLine[i], nLineLen );
		pszText[nLineLen] = '\0';
		for( i = 0; i < (int)lstrlen(pszText); ++i ){
			if( pszText[i] == CR ||
				pszText[i] == LF ){
				pszText[i] = '\0';
			}
		}
		/*
		  カーソル位置変換
		  物理位置(行頭からのバイト数、折り返し無し行位置)
		  →
		  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		*/
		int		nPosX;
		int		nPosY;
		m_cLayoutMgr.LogicToLayout(
			0,
			nLineCount,
			&nPosX,
			&nPosY
		);
		/* nDepthを計算 */
		int k;
		BOOL bAppend;
		bAppend = TRUE;
		for ( k = 0; k < nDepth; k++ ){
			int nResult = strcmp( pszStack[k], szTitle );
			if ( nResult == 0 ){
				break;
			}
		}
		if ( k < nDepth ){
			//	ループ途中でbreak;してきた。＝今までに同じ見出しが存在していた。
			//	ので、同じレベルに合わせてAppendData.
			nDepth = k;
		}
		else if( nMaxStack > k ){
			//	いままでに同じ見出しが存在しなかった。
			//	ので、pszStackにコピーしてAppendData.
			strcpy(pszStack[nDepth], szTitle);
		}else{
			// 2002.11.03 Moca 最大値を超えるとバッファオーバーラン
			// nDepth = nMaxStack;
			bAppend = FALSE;
		}
		
		if( FALSE != bAppend ){
			pcFuncInfoArr->AppendData( nLineCount + 1, nPosY + 1 , (char *)pszText, 0, nDepth );
			nDepth++;
		}
		delete [] pszText;

	}
	return;
}


/*! ルールファイルの1行を管理する構造体

	@date 2002.04.01 YAZAKI
	@date 2007.11.29 kobake 名前変更: oneRule→SOneRule
*/
struct SOneRule {
	char szMatch[256];
	int  nLength;
	char szGroupName[256];
};

/*! ルールファイルを読み込み、ルール構造体の配列を作成する

	@date 2002.04.01 YAZAKI
	@date 2002.11.03 Moca 引数nMaxCountを追加。バッファ長チェックをするように変更
*/
int CEditDoc::ReadRuleFile( const char* pszFilename, SOneRule* pcOneRule, int nMaxCount )
{
	long	i;
	// 2003.06.23 Moca 相対パスは実行ファイルからのパスとして開く
	// 2007.05.19 ryoji 相対パスは設定ファイルからのパスを優先
	FILE*	pFile = _tfopen_absini( pszFilename, "r" );
	if( NULL == pFile ){
		return 0;
	}
	char	szLine[LINEREADBUFSIZE];
	const char*	pszDelimit = " /// ";
	const char*	pszKeySeps = ",\0";
	char*	pszWork;
	int nDelimitLen = strlen( pszDelimit );
	int nCount = 0;
	while( NULL != fgets( szLine, sizeof(szLine), pFile ) && nCount < nMaxCount ){
		pszWork = strstr( szLine, pszDelimit );
		if( NULL != pszWork && szLine[0] != ';' ){
			*pszWork = '\0';
			pszWork += nDelimitLen;

			/* 最初のトークンを取得します。 */
			char* pszToken = strtok( szLine, pszKeySeps );
			while( NULL != pszToken ){
//				nRes = _stricmp( pszKey, pszToken );
				for( i = 0; i < (int)lstrlen(pszWork); ++i ){
					if( pszWork[i] == '\r' ||
						pszWork[i] == '\n' ){
						pszWork[i] = '\0';
						break;
					}
				}
				strncpy( pcOneRule[nCount].szMatch, pszToken, 255 );
				strncpy( pcOneRule[nCount].szGroupName, pszWork, 255 );
				pcOneRule[nCount].szMatch[255] = '\0';
				pcOneRule[nCount].szGroupName[255] = '\0';
				pcOneRule[nCount].nLength = strlen(pcOneRule[nCount].szMatch);
				nCount++;
				pszToken = strtok( NULL, pszKeySeps );
			}
		}
	}
	fclose( pFile );
	return nCount;
}

/*! ルールファイルを元に、トピックリストを作成

	@date 2002.04.01 YAZAKI
	@date 2002.11.03 Moca ネストの深さが最大値を超えるとバッファオーバーランするのを修正
		最大値以上は追加せずに無視する
*/
void CEditDoc::MakeFuncList_RuleFile( CFuncInfoArr* pcFuncInfoArr )
{
	const unsigned char*	pLine;
	int						nLineLen;
	int						nLineCount;
	int						i;
	int						j;
	char*					pszText;

	/* ルールファイルの内容をバッファに読み込む */
	const int nRuleSize = 1024;
	SOneRule* test = new SOneRule[nRuleSize];	//	1024個許可。 // 516*1024 = 528,384 byte
	int nCount = ReadRuleFile(GetDocumentAttribute().m_szOutlineRuleFilename, test, nRuleSize );
	if ( nCount < 1 ){
		return;
	}

	/*	ネストの深さは、32レベルまで、ひとつのヘッダは、最長256文字まで区別
		（256文字まで同じだったら同じものとして扱います）
	*/
	const int nMaxStack = 32;	//	ネストの最深
	int nDepth = 0;				//	いまのアイテムの深さを表す数値。
	char pszStack[nMaxStack][256];
	char szTitle[256];			//	一時領域
	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = (const unsigned char *)m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		if( NULL == pLine ){
			break;
		}
		for( i = 0; i < nLineLen; ++i ){
			if( pLine[i] == ' ' ||
				pLine[i] == '\t'){
				continue;
			}else
			if( i + 1 < nLineLen && pLine[i] == 0x81 && pLine[i + 1] == 0x40 ){
				++i;
				continue;
			}
			break;
		}
		if( i >= nLineLen ){
			continue;
		}
		for( j = 0; j < nCount; j++ ){
			if ( 0 == strncmp( (const char*)&pLine[i], test[j].szMatch, test[j].nLength ) ){
				strcpy( szTitle, test[j].szGroupName );
				break;
			}
		}
		if( j >= nCount ){
			continue;
		}
		/*	ルールにマッチした行は、アウトライン結果に表示する。
		*/
		pszText = new char[nLineLen + 1];
		memcpy( pszText, (const char *)&pLine[i], nLineLen );
		pszText[nLineLen] = '\0';
		int nTextLen = lstrlen( pszText );
		for( i = 0; i < nTextLen; ++i ){
			if( pszText[i] == CR ||
				pszText[i] == LF ){
				pszText[i] = '\0';
				break;
			}
		}
		/*
		  カーソル位置変換
		  物理位置(行頭からのバイト数、折り返し無し行位置)
		  →
		  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		*/
		int		nPosX;
		int		nPosY;
		m_cLayoutMgr.LogicToLayout(
			0,
			nLineCount,
			&nPosX,
			&nPosY
		);
		/* nDepthを計算 */
		int k;
		BOOL bAppend;
		bAppend = TRUE;
		for ( k = 0; k < nDepth; k++ ){
			int nResult = strcmp( pszStack[k], szTitle );
			if ( nResult == 0 ){
				break;
			}
		}
		if ( k < nDepth ){
			//	ループ途中でbreak;してきた。＝今までに同じ見出しが存在していた。
			//	ので、同じレベルに合わせてAppendData.
			nDepth = k;
		}
		else if( nMaxStack> k ){
			//	いままでに同じ見出しが存在しなかった。
			//	ので、pszStackにコピーしてAppendData.
			strcpy(pszStack[nDepth], szTitle);
		}else{
			// 2002.11.03 Moca 最大値を超えるとバッファオーバーランするから規制する
			// nDepth = nMaxStack;
			bAppend = FALSE;
		}
		
		if( FALSE != bAppend ){
			pcFuncInfoArr->AppendData( nLineCount + 1, nPosY + 1 , (char *)pszText, 0, nDepth );
			nDepth++;
		}
		delete [] pszText;

	}
	delete [] test;
	return;
}



/*! COBOL アウトライン解析 */
void CEditDoc::MakeTopicList_cobol( CFuncInfoArr* pcFuncInfoArr )
{
	const unsigned char*	pLine;
	int						nLineLen;
	int						nLineCount;
	int						i;
//	int						j;
//	int						nCharChars;
//	int						nCharChars2;
//	int						nStartsLen;
//	char*					pszText;
	int						k;
//	int						m;
	char					szDivision[1024];
	char					szLabel[1024];
	const char*				pszKeyWord;
	int						nKeyWordLen;
	BOOL					bDivision;

	szDivision[0] = '\0';
	szLabel[0] =  '\0';


	for( nLineCount = 0; nLineCount <  m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = (const unsigned char *)m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		if( NULL == pLine ){
			break;
		}
		/* コメント行か */
		if( 7 <= nLineLen && pLine[6] == '*' ){
			continue;
		}
		/* ラベル行か */
		if( 8 <= nLineLen && pLine[7] != ' ' ){
			k = 0;
			for( i = 7; i < nLineLen; ){
				if( pLine[i] == '.'
				 || pLine[i] == CR
				 || pLine[i] == LF
				){
					break;
				}
				szLabel[k] = pLine[i];
				++k;
				++i;
				if( pLine[i - 1] == ' ' ){
					for( ; i < nLineLen; ++i ){
						if( pLine[i] != ' ' ){
							break;
						}
					}
				}
			}
			szLabel[k] = '\0';
//			MYTRACE_A( "szLabel=[%s]\n", szLabel );



			pszKeyWord = "division";
			nKeyWordLen = lstrlen( pszKeyWord );
			bDivision = FALSE;
			for( i = 0; i <= (int)lstrlen( szLabel ) - nKeyWordLen; ++i ){
				if( 0 == my_memicmp( &szLabel[i], pszKeyWord, nKeyWordLen ) ){
					szLabel[i + nKeyWordLen] = '\0';
					strcpy( szDivision, szLabel );
					bDivision = TRUE;
					break;
				}
			}
			if( bDivision ){
				continue;
			}
			/*
			  カーソル位置変換
			  物理位置(行頭からのバイト数、折り返し無し行位置)
			  →
			  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
			*/

			int		nPosX;
			int		nPosY;
			char	szWork[1024];
			m_cLayoutMgr.LogicToLayout(
				0,
				nLineCount,
				&nPosX,
				&nPosY
			);
			wsprintf( szWork, "%s::%s", szDivision, szLabel );
			pcFuncInfoArr->AppendData( nLineCount + 1, nPosY + 1 , (char *)szWork, 0 );
		}
	}
	return;
}


/*! アセンブラ アウトライン解析

	@author MIK
	@date 2004.04.12 作り直し
*/
void CEditDoc::MakeTopicList_asm( CFuncInfoArr* pcFuncInfoArr )
{
	int nTotalLine;

	nTotalLine = m_cDocLineMgr.GetLineCount();

	for( int nLineCount = 0; nLineCount < nTotalLine; nLineCount++ ){
		const TCHAR* pLine;
		int nLineLen;
		TCHAR* pTmpLine;
		int length;
		int offset;
#define MAX_ASM_TOKEN 2
		TCHAR* token[MAX_ASM_TOKEN];
		int j;
		TCHAR* p;

		//1行取得する。
		pLine = m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		if( pLine == NULL ) break;

		//作業用にコピーを作成する。バイナリがあったらその後ろは知らない。
		pTmpLine = _tcsdup( pLine );
		if( pTmpLine == NULL ) break;
		if( _tcslen( pTmpLine ) >= (unsigned int)nLineLen ){	//バイナリを含んでいたら短くなるので...
			pTmpLine[ nLineLen ] = _T('\0');	//指定長で切り詰め
		}

		//行コメント削除
		p = _tcsstr( pTmpLine, _T(";") );
		if( p ) *p = _T('\0');

		length = _tcslen( pTmpLine );
		offset = 0;

		//トークンに分割
		for( j = 0; j < MAX_ASM_TOKEN; j++ ) token[ j ] = NULL;
		for( j = 0; j < MAX_ASM_TOKEN; j++ ){
			token[ j ] = my_strtok( pTmpLine, length, &offset, _T(" \t\r\n") );
			if( token[ j ] == NULL ) break;
			//トークンに含まれるべき文字でないか？
			if( _tcsstr( token[ j ], _T("\"")) != NULL
			 || _tcsstr( token[ j ], _T("\\")) != NULL
			 || _tcsstr( token[ j ], _T("'" )) != NULL ){
				token[ j ] = NULL;
				break;
			}
		}

		if( token[ 0 ] != NULL ){	//トークンが1個以上ある
			int nFuncId = -1;
			TCHAR* entry_token = NULL;

			length = _tcslen( token[ 0 ] );
			if( length >= 2
			 && token[ 0 ][ length - 1 ] == _T(':') ){	//ラベル
				token[ 0 ][ length - 1 ] = _T('\0');
				nFuncId = 51;
				entry_token = token[ 0 ];
			}else
			if( token[ 1 ] != NULL ){	//トークンが2個以上ある
				if( my_stricmp( token[ 1 ], _T("proc") ) == 0 ){	//関数
					nFuncId = 50;
					entry_token = token[ 0 ];
				}else
				if( my_stricmp( token[ 1 ], _T("endp") ) == 0 ){	//関数終了
					nFuncId = 52;
					entry_token = token[ 0 ];
				//}else
				//if( my_stricmp( token[ 1 ], _T("macro") ) == 0 ){	//マクロ
				//	nFuncId = -1;
				//	entry_token = token[ 0 ];
				//}else
				//if( my_stricmp( token[ 1 ], _T("struc") ) == 0 ){	//構造体
				//	nFuncId = -1;
				//	entry_token = token[ 0 ];
				}
			}

			if( nFuncId >= 0 ){
				/*
				  カーソル位置変換
				  物理位置(行頭からのバイト数、折り返し無し行位置)
				  →
				  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
				*/
				int		nPosX;
				int		nPosY;
				m_cLayoutMgr.LogicToLayout(
					0,
					nLineCount/*nFuncLine - 1*/,
					&nPosX,
					&nPosY
				);
				pcFuncInfoArr->AppendData( nLineCount + 1/*nFuncLine*/, nPosY + 1, entry_token, nFuncId );
			}
		}

		free( pTmpLine );
	}

	return;
}



/*! 階層付きテキスト アウトライン解析

	@author zenryaku
	@date 2003.05.20 zenryaku 新規作成
	@date 2003.05.25 genta 実装方法一部修正
	@date 2003.06.21 Moca 階層が2段以上深くなる場合を考慮
*/
void CEditDoc::MakeTopicList_wztxt(CFuncInfoArr* pcFuncInfoArr)
{
	int levelPrev = 0;

	for(int nLineCount=0;nLineCount<m_cDocLineMgr.GetLineCount();nLineCount++)
	{
		const char*	pLine;
		int			nLineLen;

		pLine = m_cDocLineMgr.GetLineStr(nLineCount,&nLineLen);
		if(!pLine)
		{
			break;
		}
		//	May 25, 2003 genta 判定順序変更
		if( *pLine == '.' )
		{
			const char* pPos;	//	May 25, 2003 genta
			int			nLength;
			char		szTitle[1024];

			//	ピリオドの数＝階層の深さを数える
			for( pPos = pLine + 1 ; *pPos == '.' ; ++pPos )
				;

			int	nPosX;
			int	nPosY;
			m_cLayoutMgr.LogicToLayout(
				0,
				nLineCount,
				&nPosX,
				&nPosY
			);
			
			int level = pPos - pLine;

			// 2003.06.27 Moca 階層が2段位上深くなるときは、無題の要素を追加
			if( levelPrev < level && level != levelPrev + 1  ){
				int dummyLevel;
				// (無題)を挿入
				//	ただし，TAG一覧には出力されないように
				for( dummyLevel = levelPrev + 1; dummyLevel < level; dummyLevel++ ){
					pcFuncInfoArr->AppendData( nLineCount+1, nPosY+1,
						"(無題)", FUNCINFO_NOCLIPTEXT, dummyLevel - 1 );
				}
			}
			levelPrev = level;

			nLength = wsprintf(szTitle,"%d - ", level );
			
			char *pDest = szTitle + nLength; // 書き込み先
			char *pDestEnd = szTitle + sizeof(szTitle) - 2;
			
			while( pDest < pDestEnd )
			{
				if( *pPos =='\r' || *pPos =='\n' || *pPos == '\0')
				{
					break;
				}
				//	May 25, 2003 genta 2バイト文字の切断を防ぐ
				else if( _IS_SJIS_1( *pPos )){
					*pDest++ = *pPos++;
					*pDest++ = *pPos++;
				}
				else {
					*pDest++ = *pPos++;
				}
			}
			*pDest = '\0';
			pcFuncInfoArr->AppendData(nLineCount+1,nPosY+1,szTitle, 0, level - 1);
		}
	}
}

/*! HTML アウトライン解析

	@author zenryaku
	@date 2003.05.20 zenryaku 新規作成
	@date 2004.04.19 zenryaku 空要素を判定
	@date 2004.04.20 Moca コメント処理と、不明な終了タグを無視する処理を追加
	@date 2008.08.15 aroka 見出しと段落の深さ制御を追加 2008.09.07修正
*/
void CEditDoc::MakeTopicList_html(CFuncInfoArr* pcFuncInfoArr)
{
	const unsigned char*	pLineBuf;	//	pLineBuf は行全体を指し、
	const unsigned char*	pLine;		//	pLine は処理中の文字以降の部分を指します。
	int						nLineLen;
	int						nLineCount;
	int						i;
	int						j;
	int						k;
	bool					bEndTag;
	bool					bCommentTag = false;
	bool					bParaTag = false;	//	2008.08.15 aroka

	/*	ネストの深さは、nMaxStackレベルまで、ひとつのヘッダは、最長32文字まで区別
		（32文字まで同じだったら同じものとして扱います）
	*/
	const int				nMaxStack = 32;	//	ネストの最深
	int						nDepth = 0;				//	いまのアイテムの深さを表す数値。
	char					pszStack[nMaxStack][32];
	char					szTitle[32];			//	一時領域
	char					szTag[32];				//	一時領域  小文字で保持して高速化しています。
	int						nLabelType;				// default, inlined, ignore, empty, block, p, heading
	/*	同じ見出し要素（hy）を次に上位レベルの見出し(hx)が現れるまで同じ深さにそろえます。
		このため、見出しの深さを記憶しておきます。
		下位レベルの見出しの深さは現れるまで不定で、前の章節での深さは影響しません。 2008.08.15 aroka
	*/
	int						nHeadDepth[6+1];		// [0]は 空けておく
	for(k=0;k<=6;k++){
		nHeadDepth[k] = -1;
	}
	for(nLineCount=0;nLineCount<m_cDocLineMgr.GetLineCount();nLineCount++)
	{
		pLineBuf = (const unsigned char *)m_cDocLineMgr.GetLineStr(nLineCount,&nLineLen);
		if( !pLineBuf )
		{
			break;
		}
		for(i=0;i<nLineLen-1;i++)
		{
			pLine = &pLineBuf[i];
			// 2004.04.20 Moca コメントを処理する
			if( bCommentTag )
			{
				if( i < nLineLen - 3 && 0 == memcmp( "-->", pLine, 3 ) )
				{
					bCommentTag = false;
					i += 2;
					pLine += 2;
				}
				continue;
			}
			// 2004.04.20 Moca To Here
			if( *pLine!='<' || nDepth>=nMaxStack )
			{
				continue;
			}
			bEndTag = false;
			pLine++; i++;
			if( *pLine=='/')
			{
				pLine++; i++;
				bEndTag = true;
			}
			for(j=0;i+j<nLineLen && j<sizeof(szTitle)-1; )
			{
				// タグ名を切り出す
				// スペース、タブ、「_:-.英数」以外の半角文字、１文字目の「-.数字」は認めない。
				if( (pLine[j]==' ' || pLine[j]=='\t') ||
					(pLine[j]<0x80 && !strchr("_:-.",pLine[j]) && !isalnum(pLine[j])) ||
					(j==0 &&( (pLine[j]>='0' && pLine[j]<='9') || pLine[j]=='-' || pLine[j]=='.' )) )
				{
					break;
				}
				int nCharSize = CMemory::GetSizeOfChar((char*)pLine, nLineLen-i, j);
				memcpy(szTitle + j, pLine + j, nCharSize);
				j += nCharSize;
			}
			if(j==0)
			{
				// 2004.04.20 Moca From Here コメントを処理する
				if( i < nLineLen - 3 && 0 == memcmp( "!--", pLine, 3 ) )
				{
					bCommentTag = true;
					i += 3;
					pLine += 3;
				}
				// 2004.04.20 Moca To Here
				continue;
			}
			szTitle[j] = '\0';
			/*	タグの種類ごとに処理を変える必要があるが、
				都度比較するのはコストが高いので、最初に分類しておく。 2008.08.15 aroka
				比較の回数が多いため、小文字に変換しておいてstrcmpを使う。
			*/
			strcpy( szTag, szTitle );
			_strlwr( szTag );
			
			nLabelType = 'de';//default
			// 物理要素（見た目を変えるためのタグ）は構造解析しない。
			if( !strcmp(szTag,"b") || !strcmp(szTag,"big") || !strcmp(szTag,"blink")
			 || !strcmp(szTag,"font") || !strcmp(szTag,"i") || !strcmp(szTag,"marquee")
			 || !strcmp(szTag,"nobr") || !strcmp(szTag,"s") || !strcmp(szTag,"small")
			 || !strcmp(szTag,"strike") || !strcmp(szTag,"tt") || !strcmp(szTag,"u")
			 || !strcmp(szTag,"bdo") || !strcmp(szTag,"sub") || !strcmp(szTag,"sup") )
			{
				nLabelType = 'in';//inlined
			}
			// インラインテキスト要素（テキストを修飾するタグ）は構造解析しない?
//			if( !strcmp(szTag,"abbr") || !strcmp(szTag,"acronym") || !strcmp(szTag,"dfn")
//			 || !strcmp(szTag,"em") || !strcmp(szTag,"strong") || !strcmp(szTag,"span")
//			 || !strcmp(szTag,"code") || !strcmp(szTag,"samp") || !strcmp(szTag,"kbd")
//			 || !strcmp(szTag,"var") || !strcmp(szTag,"cite") || !strcmp(szTag,"q") )
//			{
//				nLabelType = 'in';//inlined
//			}
			// ルビ要素（XHTML1.1）は構造解析しない。
			if( !strcmp(szTag,"rbc") || !strcmp(szTag,"rtc") || !strcmp(szTag,"ruby")
			 || !strcmp(szTag,"rb") || !strcmp(szTag,"rt") || !strcmp(szTag,"rp") )
			{
				nLabelType = 'in';//inlined
			}
			// 空要素（内容を持たないタグ）のうち構造に関係ないものは構造解析しない。
			if( !strcmp(szTag,"br") || !strcmp(szTag,"base") || !strcmp(szTag,"basefont")
			 || !strcmp(szTag,"frame") )
			{
				nLabelType = 'ig';//empty
			}
			// 空要素（内容を持たないタグ）のうち構造に関係するもの。
			if( !strcmp(szTag,"area") || !strcmp(szTag,"hr") || !strcmp(szTag,"img")
			 || !strcmp(szTag,"input") || !strcmp(szTag,"link") || !strcmp(szTag,"meta")
			 || !strcmp(szTag,"param") )
			{
				nLabelType = 'em';//empty
			}
			if( !strcmp(szTag,"div") || !strcmp(szTag,"center")
			 || !strcmp(szTag,"address") || !strcmp(szTag,"blockquote")
			 || !strcmp(szTag,"noscript") || !strcmp(szTag,"noframes")
			 || !strcmp(szTag,"ol") || !strcmp(szTag,"ul") || !strcmp(szTag,"dl")
			 || !strcmp(szTag,"dir") || !strcmp(szTag,"menu")
			 || !strcmp(szTag,"pre") || !strcmp(szTag,"table")
			 || !strcmp(szTag,"form") || !strcmp(szTag,"fieldset") || !strcmp(szTag,"isindex") )
			{
				nLabelType = 'bl';//block
			}
			if( !strcmp(szTag,"p") )
			{
				nLabelType = 'pa';//paragraph
			}
			if( (szTag[0]=='h') && ('1'<=szTitle[1]&&szTitle[1]<='6') ){
				nLabelType = 'hd';//heading
			}

			// 2009.08.08 syat 「/>」で終わるタグの判定のため、終了タグ処理を開始タグ処理の後にした。
			//                  （開始タグ処理の中で、bEndTagをtrueにしている所がある。）

			if( ! bEndTag ) // 開始タグ
			{
				if( nLabelType!='in' && nLabelType!='ig' ){
					// pの中でブロック要素がきたら、自動的にpを閉じる。 2008.09.07 aroka
					if( bParaTag ){
						if( nLabelType=='hd' || nLabelType=='pa' || nLabelType=='bl' ){
							nDepth--;
						}
					}
					if( nLabelType=='hd' ){
						if( nHeadDepth[szTitle[1]-'0'] != -1 ) // 小見出し:既出
						{
							nDepth = nHeadDepth[szTitle[1]-'0'];
							for(k=szTitle[1]-'0';k<=6;k++){
								nHeadDepth[k] = -1;
							}
							nHeadDepth[szTitle[1]-'0'] = nDepth;
							bParaTag = false;
						}
					}
					if( nLabelType=='pa' ){
						bParaTag = true;
					}
					if( nLabelType=='bl' ){
						bParaTag = false;
					}

					int		nPosX;
					int		nPosY;

					m_cLayoutMgr.LogicToLayout(
						i,
						nLineCount,
						&nPosX,
						&nPosY
					);

					if( nLabelType!='em' ){
						// 終了タグなしを除く全てのタグらしきものを判定
						strcpy(pszStack[nDepth],szTitle);
						k	=	j;
						if(j<sizeof(szTitle)-3)
						{
							for(;i+j<nLineLen;j++)
							{
								if( pLine[j]=='/' && pLine[j+1]=='>' )
								{
									bEndTag = true;
									break;
								}
								else if( pLine[j]=='>' )
								{
									break;
								}
							}
							if(!bEndTag)
							{
								szTitle[k++]	=	' ';
								for(j-=k-1;i+j+k<nLineLen && k<sizeof(szTitle)-1;k++)
								{
									if( pLine[j+k]=='<' || pLine[j+k]=='\r' || pLine[j+k]=='\n' )
									{
										break;
									}
									szTitle[k] = pLine[j+k];
								}
								j += k-1;
							}
						}
						szTitle[k]	=	'\0';
						pcFuncInfoArr->AppendData( nLineCount+1, nPosY+1, szTitle, 0, nDepth++ );
					}
					else
					{
						for(;i+j<nLineLen && j<sizeof(szTitle)-1;j++)
						{
							if( pLine[j]=='>' )
							{
								break;
							}
							szTitle[j] = pLine[j];
						}
						szTitle[j]	=	'\0';
						pcFuncInfoArr->AppendData(nLineCount+1,nPosY+1,szTitle,0,nDepth);
					}
				}
			}
			if( bEndTag ) // 終了タグ
			{
				int nDepthOrg = nDepth; // 2004.04.20 Moca 追加
				while(nDepth>0)
				{
					nDepth--;
					if(!my_stricmp(pszStack[nDepth],szTitle))
					{
						break;
					}
				}
				// 2004.04.20 Moca ツリー中と一致しないときは、この終了タグは無視
				if( nDepth == 0 )
				{
					if(my_stricmp(pszStack[nDepth],szTitle))
					{
						nDepth = nDepthOrg;
					}
				}else{
					if( nLabelType=='hd' ){	//	見出しの終わり
						nHeadDepth[szTitle[1]-'0'] = nDepth;
						nDepth++;
					}
					if( nLabelType=='pa' ){
						bParaTag = false;
					}
				}
			}
			i	+=	j;
		}
	}
}

/*! TeX アウトライン解析

	@author naoh
	@date 2003.07.21 naoh 新規作成
	@date 2005.01.03 naoh 「マ」などの"}"を含む文字に対する修正、prosperのslideに対応
*/
void CEditDoc::MakeTopicList_tex(CFuncInfoArr* pcFuncInfoArr)
{
	const char*	pLine;
	int						nLineLen;
	int						nLineCount;
	int						i;
	int						j;
	int						k;

	const int nMaxStack = 8;	//	ネストの最深
	int nDepth = 0;				//	いまのアイテムの深さを表す数値。
	char szTag[32], szTitle[256];			//	一時領域
	int thisSection=0, lastSection = 0;	// 現在のセクション種類と一つ前のセクション種類
	int stackSection[nMaxStack];		// 各深さでのセクションの番号
	int nStartTitlePos;					// \section{dddd} の dddd の部分の始まる番号
	int bNoNumber;						// * 付の場合はセクション番号を付けない

	// 一行ずつ
	for(nLineCount=0;nLineCount<m_cDocLineMgr.GetLineCount();nLineCount++)
	{
		pLine	=	(const char *)m_cDocLineMgr.GetLineStr(nLineCount,&nLineLen);
		if(!pLine) break;
		// 一文字ずつ
		for(i=0;i<nLineLen-1;i++)
		{
			if(pLine[i] == '%' && !(i>0 && _IS_SJIS_1(pLine[i-1])) ) break;	// コメントなら以降はいらない
			if(pLine[i] != '\\' 
				&& !(i>0 && _IS_SJIS_1(pLine[i-1]))	// 「\」の前の文字がSJISの1バイト目なら次の文字へ
				|| nDepth>=nMaxStack) continue;	// 「\」がないなら次の文字へ
			++i;
			// 見つかった「\」以降の文字列チェック
			for(j=0;i+j<nLineLen && j<sizeof(szTag)-1;j++)
			{
				if(pLine[i+j] == '{' && !(i+j>0 && _IS_SJIS_1((unsigned char)pLine[i+j-1])) ) {	// SJIS1チェック
					bNoNumber = (pLine[i+j-1] == '*');
					nStartTitlePos = j+i+1;
					break;
				}
				szTag[j] = pLine[i+j];
			}
			if(j==0) continue;
			if(bNoNumber){
				szTag[j-1] = '\0';
			}else{
				szTag[j]   = '\0';
			}
//			MessageBox(NULL, szTitle, "", MB_OK);

			thisSection = 0;
			if(!strcmp(szTag,"subsubsection")) thisSection = 4;
			else if(!strcmp(szTag,"subsection")) thisSection = 3;
			else if(!strcmp(szTag,"section")) thisSection = 2;
			else if(!strcmp(szTag,"chapter")) thisSection = 1;
			else if(!strcmp(szTag,"begin")) {		// beginなら prosperのslideの可能性も考慮
				// さらに{slide}{}まで読みとっておく
				if(strstr(pLine, "{slide}")){
					k=0;
					for(j=nStartTitlePos+1;i+j<nLineLen && j<sizeof(szTag)-1;j++)
					{
						if(pLine[i+j] == '{' && !(i+j>0 && _IS_SJIS_1((unsigned char)pLine[i+j-1])) ) {	// SJIS1チェック
							nStartTitlePos = j+i+1;
							break;
						}
						szTag[k++]	=	pLine[i+j];
					}
					szTag[k] = '\0';
					thisSection = 1;
				}
			}

			if( thisSection > 0)
			{
				// sectionの中身取得
				for(k=0;nStartTitlePos+k<nLineLen && k<sizeof(szTitle)-1;k++)
				{
					if(_IS_SJIS_1((unsigned char)pLine[k+nStartTitlePos])) {
						szTitle[k] = pLine[k+nStartTitlePos];
						k++;	// 次はチェック不要
					} else if(pLine[k+nStartTitlePos] == '}') {
						break;
					}
					szTitle[k] = pLine[k+nStartTitlePos];
				}
				szTitle[k] = '\0';

				int		nPosX;
				int		nPosY;
				TCHAR tmpstr[256];
				TCHAR secstr[4];

				m_cLayoutMgr.LogicToLayout(
					i,
					nLineCount,
					&nPosX,
					&nPosY
				);

				int sabunSection = thisSection - lastSection;
				if(lastSection == 0){
					nDepth = 0;
					stackSection[0] = 1;
				}else{
					nDepth += sabunSection;
					if(sabunSection > 0){
						if(nDepth >= nMaxStack) nDepth=nMaxStack-1;
						stackSection[nDepth] = 1;
					}else{
						if(nDepth < 0) nDepth=0;
						++stackSection[nDepth];
					}
				}
				tmpstr[0] = '\0';
				if(!bNoNumber){
					for(k=0; k<=nDepth; k++){
						sprintf(secstr, "%d.", stackSection[k]);
						strcat(tmpstr, secstr);
					}
					strcat(tmpstr, " ");
				}
				strcat(tmpstr, szTitle);
				pcFuncInfoArr->AppendData(nLineCount+1,nPosY+1, tmpstr, 0, nDepth);
				if(!bNoNumber) lastSection = thisSection;
			}
			i	+=	j;
		}
	}
}




/* アクティブなペインを設定 */
void  CEditDoc::SetActivePane( int nIndex )
{
	/* アクティブなビューを切り替える */
	int nOldIndex = m_nActivePaneIndex;
	m_nActivePaneIndex = nIndex;

	// フォーカスを移動する	// 2007.10.16 ryoji
	m_pcEditViewArr[nOldIndex]->m_cUnderLine.CaretUnderLineOFF( true );	//	2002/05/11 YAZAKI
	if( ::GetActiveWindow() == m_pcEditWnd->m_hWnd
		&& ::GetFocus() != m_pcEditViewArr[m_nActivePaneIndex]->m_hWnd )
	{
		// ::SetFocus()でフォーカスを切り替える
		::SetFocus( m_pcEditViewArr[m_nActivePaneIndex]->m_hWnd );
	}else{
		// 2010.04.08 ryoji
		// 起動直後にエディットボックスにフォーカスのあるダイアログを表示する場合にキャレットが消える問題の修正
		// （例: -GREPDLGオプションで起動するとGREPダイアログのキャレットが消えている）
		// この問題を修正するのため、内部的な切り替え動作をするのはアクティブペインが替わるときだけにした．
		if( m_nActivePaneIndex != nOldIndex ){
			// アクティブでないときに::SetFocus()するとアクティブになってしまう
			// （不可視なら可視になる）ので内部的に切り替えるだけにする
			m_pcEditViewArr[nOldIndex]->OnKillFocus();
			m_pcEditViewArr[m_nActivePaneIndex]->OnSetFocus();
		}
	}

	m_pcEditViewArr[m_nActivePaneIndex]->RedrawAll();	/* フォーカス移動時の再描画 */

	m_cSplitterWnd.SetActivePane( nIndex );

	if( NULL != m_cDlgFind.m_hWnd ){		/* 「検索」ダイアログ */
		/* モードレス時：検索対象となるビューの変更 */
		m_cDlgFind.ChangeView( (LPARAM)m_pcEditViewArr[m_nActivePaneIndex] );
	}
	if( NULL != m_cDlgReplace.m_hWnd ){	/* 「置換」ダイアログ */
		/* モードレス時：検索対象となるビューの変更 */
		m_cDlgReplace.ChangeView( (LPARAM)m_pcEditViewArr[m_nActivePaneIndex] );
	}
	if( NULL != m_cHokanMgr.m_hWnd ){	/* 「入力補完」ダイアログ */
		m_cHokanMgr.Hide();
		/* モードレス時：検索対象となるビューの変更 */
		m_cHokanMgr.ChangeView( (LPARAM)m_pcEditViewArr[m_nActivePaneIndex] );
	}
	if( NULL != m_cDlgFuncList.m_hWnd ){	/* 「アウトライン」ダイアログ */ // 20060201 aroka
		/* モードレス時：現在位置表示の対象となるビューの変更 */
		m_cDlgFuncList.ChangeView( (LPARAM)m_pcEditViewArr[m_nActivePaneIndex] );
	}

	return;
}



/* アクティブなペインを取得 */
int CEditDoc::GetActivePane( void )
{
	return m_nActivePaneIndex;
}



/** すべてのペインの描画スイッチを設定する

	@param bDraw [in] 描画スイッチの設定値

	@date 2008.06.08 ryoji 新規作成
*/
void CEditDoc::SetDrawSwitchOfAllViews( bool bDraw )
{
	int i;
	CEditView* pView;

	for( i = 0; i < GetAllViewCount(); i++ ){
		pView = m_pcEditViewArr[i];
		pView->m_bDrawSWITCH = bDraw;
	}
}

/** すべてのペインをRedrawする

	スクロールバーの状態更新はパラメータでフラグ制御 or 別関数にしたほうがいい？

	@param pViewExclude [in] Redrawから除外するビュー

	@date 2007.07.22 ryoji スクロールバーの状態更新を追加
	@date 2008.06.08 ryoji pViewExclude パラメータ追加
*/
void CEditDoc::RedrawAllViews( CEditView* pViewExclude )
{
	int i;
	CEditView* pView;

	for( i = 0; i < GetAllViewCount(); i++ ){
		pView = m_pcEditViewArr[i];
		if( pView == pViewExclude )
			continue;
		if( i == m_nActivePaneIndex ){
			pView->RedrawAll();
		}else{
			pView->Redraw();
			pView->AdjustScrollBars();
		}
	}
}

void CEditDoc::Views_Redraw()
{
	//アクティブ以外を再描画してから…
	for( int v = 0; v < GetAllViewCount(); ++v ){
		if( m_nActivePaneIndex != v ){
			m_pcEditViewArr[v]->Redraw();
		}
	}
	//アクティブを再描画
	ActiveView().Redraw();
}


/* すべてのペインで、行番号表示に必要な幅を再設定する（必要なら再描画する） */
BOOL CEditDoc::DetectWidthOfLineNumberAreaAllPane( BOOL bRedraw )
{
	if( 1 == GetAllViewCount() ){
		return m_pcEditViewArr[m_nActivePaneIndex]->DetectWidthOfLineNumberArea( bRedraw );
	}
	// 以下2,4分割限定

	if ( m_pcEditViewArr[m_nActivePaneIndex]->DetectWidthOfLineNumberArea( bRedraw ) ){
		/* ActivePaneで計算したら、再設定・再描画が必要と判明した */
		if ( m_cSplitterWnd.GetAllSplitCols() == 2 ){
			m_pcEditViewArr[m_nActivePaneIndex^1]->DetectWidthOfLineNumberArea( bRedraw );
		}
		else {
			//	表示されていないので再描画しない
			m_pcEditViewArr[m_nActivePaneIndex^1]->DetectWidthOfLineNumberArea( FALSE );
		}
		if ( m_cSplitterWnd.GetAllSplitRows() == 2 ){
			m_pcEditViewArr[m_nActivePaneIndex^2]->DetectWidthOfLineNumberArea( bRedraw );
			if ( m_cSplitterWnd.GetAllSplitCols() == 2 ){
				m_pcEditViewArr[(m_nActivePaneIndex^1)^2]->DetectWidthOfLineNumberArea( bRedraw );
			}
		}
		else {
			m_pcEditViewArr[m_nActivePaneIndex^2]->DetectWidthOfLineNumberArea( FALSE );
			m_pcEditViewArr[(m_nActivePaneIndex^1)^2]->DetectWidthOfLineNumberArea( FALSE );
		}
		return TRUE;
	}
	return FALSE;
}

/** 右端で折り返す
	@param nViewColNum	[in] 右端で折り返すペインの番号
	@retval 折り返しを変更したかどうか
	@date 2008.06.08 ryoji 新規作成
*/
BOOL CEditDoc::WrapWindowWidth( int nPane )
{
	// 右端で折り返す
	int nWidth = m_pcEditViewArr[nPane]->ViewColNumToWrapColNum( m_pcEditViewArr[nPane]->m_nViewColNum );
	if( m_cLayoutMgr.GetMaxLineKetas() != nWidth ){
		ChangeLayoutParam( false, m_cLayoutMgr.GetTabSpace(), nWidth );
		return TRUE;
	}
	return FALSE;
}

/** 折り返し方法関連の更新
	@retval 画面更新したかどうか
	@date 2008.06.10 ryoji 新規作成
*/
BOOL CEditDoc::UpdateTextWrap( void )
{
	// この関数はコマンド実行ごとに処理の最終段階で利用する
	// （アンドゥ登録＆全ビュー更新のタイミング）
	if( m_nTextWrapMethodCur == WRAP_WINDOW_WIDTH ){
		BOOL bWrap = WrapWindowWidth( 0 );	// 右端で折り返す
		if( bWrap ){
			// WrapWindowWidth() で追加した更新リージョンで画面更新する
			for( int i = 0; i < GetAllViewCount(); i++ ){
				::UpdateWindow( m_pcEditViewArr[i]->m_hWnd );
			}
		}
		return bWrap;	// 画面更新＝折り返し変更
	}
	return FALSE;	// 画面更新しなかった
}


/*!
	レイアウトの変更に先立って，全てのViewの座標を物理座標に変換して保存する．

	@return データを保存した配列へのポインタ

	@note 取得した値はレイアウト変更後にCEditDoc::RestorePhysPosOfAllViewへ渡す．
	渡し忘れるとメモリリークとなる．

	@date 2005.08.11 genta 新規作成
*/
int* CEditDoc::SavePhysPosOfAllView(void)
{
	const int NUM_OF_VIEW = GetAllViewCount();
	const int NUM_OF_POS = 5;
	const int XY = 2;
	
	int* posary = new int[ NUM_OF_VIEW * NUM_OF_POS * XY ];
	
	for( int i = 0; i < NUM_OF_VIEW; ++i ){
		m_cLayoutMgr.LayoutToLogic(
			m_pcEditViewArr[i]->m_nCaretPosX,
			m_pcEditViewArr[i]->m_nCaretPosY,
			&posary[i * ( NUM_OF_POS * XY ) + 0 * XY + 0 ],
			&posary[i * ( NUM_OF_POS * XY ) + 0 * XY + 1 ]
		);
		if( m_pcEditViewArr[i]->m_nSelectLineBgnFrom >= 0 ){
			m_cLayoutMgr.LayoutToLogic(
				m_pcEditViewArr[i]->m_nSelectColmBgnFrom,
				m_pcEditViewArr[i]->m_nSelectLineBgnFrom,
				&posary[i * ( NUM_OF_POS * XY ) + 1 * XY + 0 ],
				&posary[i * ( NUM_OF_POS * XY ) + 1 * XY + 1 ]
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineBgnTo >= 0 ){
			m_cLayoutMgr.LayoutToLogic(
				m_pcEditViewArr[i]->m_nSelectColmBgnTo,
				m_pcEditViewArr[i]->m_nSelectLineBgnTo,
				&posary[i * ( NUM_OF_POS * XY ) + 2 * XY + 0 ],
				&posary[i * ( NUM_OF_POS * XY ) + 2 * XY + 1 ]
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineFrom >= 0 ){
			m_cLayoutMgr.LayoutToLogic(
				m_pcEditViewArr[i]->m_nSelectColmFrom,
				m_pcEditViewArr[i]->m_nSelectLineFrom,
				&posary[i * ( NUM_OF_POS * XY ) + 3 * XY + 0 ],
				&posary[i * ( NUM_OF_POS * XY ) + 3 * XY + 1 ]
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineTo >= 0 ){
			m_cLayoutMgr.LayoutToLogic(
				m_pcEditViewArr[i]->m_nSelectColmTo,
				m_pcEditViewArr[i]->m_nSelectLineTo,
				&posary[i * ( NUM_OF_POS * XY ) + 4 * XY + 0 ],
				&posary[i * ( NUM_OF_POS * XY ) + 4 * XY + 1 ]
			);
		}
	}
	return posary;
}

/*!	座標の復元

	CEditDoc::SavePhysPosOfAllViewで保存したデータを元に座標値を再計算する．

	@date 2005.08.11 genta 新規作成
*/
void CEditDoc::RestorePhysPosOfAllView( int* posary )
{
	const int NUM_OF_VIEW = GetAllViewCount();
	const int NUM_OF_POS = 5;
	const int XY = 2;

	for( int i = 0; i < NUM_OF_VIEW; ++i ){
		int		nPosX;
		int		nPosY;
		m_cLayoutMgr.LogicToLayout(
			posary[i * ( NUM_OF_POS * XY ) + 0 * XY + 0 ],
			posary[i * ( NUM_OF_POS * XY ) + 0 * XY + 1 ],
			&nPosX,
			&nPosY
		);
		m_pcEditViewArr[i]->MoveCursor( nPosX, nPosY, true );
		m_pcEditViewArr[i]->m_nCaretPosX_Prev = m_pcEditViewArr[i]->m_nCaretPosX;

		if( m_pcEditViewArr[i]->m_nSelectLineBgnFrom >= 0 ){
			m_cLayoutMgr.LogicToLayout(
				posary[i * ( NUM_OF_POS * XY ) + 1 * XY + 0 ],
				posary[i * ( NUM_OF_POS * XY ) + 1 * XY + 1 ],
				&m_pcEditViewArr[i]->m_nSelectColmBgnFrom,
				&m_pcEditViewArr[i]->m_nSelectLineBgnFrom
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineBgnTo >= 0 ){
			m_cLayoutMgr.LogicToLayout(
				posary[i * ( NUM_OF_POS * XY ) + 2 * XY + 0 ],
				posary[i * ( NUM_OF_POS * XY ) + 2 * XY + 1 ],
				&m_pcEditViewArr[i]->m_nSelectColmBgnTo,
				&m_pcEditViewArr[i]->m_nSelectLineBgnTo
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineFrom >= 0 ){
			m_cLayoutMgr.LogicToLayout(
				posary[i * ( NUM_OF_POS * XY ) + 3 * XY + 0 ],
				posary[i * ( NUM_OF_POS * XY ) + 3 * XY + 1 ],
				&m_pcEditViewArr[i]->m_nSelectColmFrom,
				&m_pcEditViewArr[i]->m_nSelectLineFrom
			);
		}
		if( m_pcEditViewArr[i]->m_nSelectLineTo >= 0 ){
			m_cLayoutMgr.LogicToLayout(
				posary[i * ( NUM_OF_POS * XY ) + 4 * XY + 0 ],
				posary[i * ( NUM_OF_POS * XY ) + 4 * XY + 1 ],
				&m_pcEditViewArr[i]->m_nSelectColmTo,
				&m_pcEditViewArr[i]->m_nSelectLineTo
			);
		}
	}
	delete[] posary;
}

/* 編集ファイル情報を格納 */
void CEditDoc::GetEditInfo(
	EditInfo* pfi	//!< [out]
)
{
	int		nX;
	int		nY;

	//ファイルパス
	_tcscpy( pfi->m_szPath, GetFilePath() );

	//表示域
	pfi->m_nViewTopLine = m_pcEditViewArr[m_nActivePaneIndex]->m_nViewTopLine;	/* 表示域の一番上の行(0開始) */
	pfi->m_nViewLeftCol = m_pcEditViewArr[m_nActivePaneIndex]->m_nViewLeftCol;	/* 表示域の一番左の桁(0開始) */

	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	m_cLayoutMgr.LayoutToLogic(
		m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX,	/* ビュー左端からのカーソル桁位置(０開始) */
		m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosY,	/* ビュー上端からのカーソル行位置(０開始) */
		&nX,
		&nY
	);
	pfi->m_nX = nX;		/* カーソル 物理位置(行頭からのバイト数) */
	pfi->m_nY = nY;		/* カーソル 物理位置(折り返し無し行位置) */

	//各種状態
	pfi->m_bIsModified = IsModified();			/* 変更フラグ */
	pfi->m_nCharCode = m_nCharCode;				/* 文字コード種別 */
	pfi->m_nType = GetDocumentType();

	//GREPモード
	pfi->m_bIsGrep = m_bGrepMode;
	_tcscpy( pfi->m_szGrepKey, m_szGrepKey );

	//デバッグモニタ (アウトプットウインドウ) モード
	pfi->m_bIsDebug = m_bDebugMode;
}


/*
	分割指示。2つ目以降のビューを作る
	@param nViewCount  既存のビューも含めたビューの合計要求数
*/
bool CEditDoc::CreateEditViewBySplit(int nViewCount )
{
	if( m_nEditViewMaxCount < nViewCount ){
		return false;
	}
	if( GetAllViewCount() < nViewCount ){
		int i;
		for( i = GetAllViewCount(); i < nViewCount; i++ ){
			assert( NULL == m_pcEditViewArr[i] );
			m_pcEditViewArr[i] = new CEditView();
			m_pcEditViewArr[i]->Create( m_hInstance, m_cSplitterWnd.m_hWnd, this, i, FALSE );
		}
		m_nEditViewCount = nViewCount;

		std::vector<HWND> hWndArr;
		hWndArr.reserve(nViewCount + 1);
		for( i = 0; i < nViewCount; i++ ){
			hWndArr.push_back( m_pcEditViewArr[i]->m_hWnd );
		}
		hWndArr.push_back( NULL );

		m_cSplitterWnd.SetChildWndArr( &hWndArr[0] );
	}
	return true;
}


/* ファイルのタイムスタンプのチェック処理 */
void CEditDoc::CheckFileTimeStamp( void )
{
	HWND		hwndActive;
	BOOL		bUpdate;
	bUpdate = FALSE;
	if( m_pShareData->m_Common.m_sFile.m_bCheckFileTimeStamp	/* 更新の監視 */
	 // Dec. 4, 2002 genta
	 && m_eWatchUpdate != WU_NONE
	 && m_pShareData->m_Common.m_sFile.m_nFileShareMode == 0	/* ファイルの排他制御モード */
	 && NULL != ( hwndActive = ::GetActiveWindow() )	/* アクティブ? */
	 && hwndActive == m_hwndParent
	 && IsValidPath()
	 && ( !m_FileTime.IsZero() ) 	/* 現在編集中のファイルのタイムスタンプ */

	){
		/* ファイルスタンプをチェックする */

		// 2005.10.20 ryoji FindFirstFileを使うように変更（ファイルがロックされていてもタイムスタンプ取得可能）
		CFileTime ftime;
		if( GetLastWriteTimestamp( GetFilePath(), &ftime )){
			if( 0 != ::CompareFileTime( &m_FileTime.GetFILETIME(), &ftime.GetFILETIME() ) )	//	Aug. 13, 2003 wmlhq タイムスタンプが古く変更されている場合も検出対象とする
			{
				bUpdate = TRUE;
				m_FileTime = ftime.GetFILETIME();
			}
		}
	}

	//	From Here Dec. 4, 2002 genta
	if( bUpdate ){
		switch( m_eWatchUpdate ){
		case WU_NOTIFY:
			{
				TCHAR szText[40];
				const CFileTime& ctime = m_FileTime;
				wsprintf( szText, _T("★ファイル更新 %02d:%02d:%02d"), ctime->wHour, ctime->wMinute, ctime->wSecond );
				m_pcEditWnd->SendStatusMessage( szText );
			}	
			break;
		default:
			{
				m_eWatchUpdate = WU_NONE; // 更新監視の抑制

				CDlgFileUpdateQuery dlg( GetFilePath(), IsModified() );
				int result = dlg.DoModal(
					m_hInstance,
					m_hWnd,
					IDD_FILEUPDATEQUERY,
					0
				);

				switch( result ){
				case 1:	// 再読込
					/* 同一ファイルの再オープン */
					ReloadCurrentFile( m_nCharCode, m_bReadOnly );
					m_eWatchUpdate = WU_QUERY;
					break;
				case 2:	// 以後通知メッセージのみ
					m_eWatchUpdate = WU_NOTIFY;
					break;
				case 3:	// 以後更新を監視しない
					m_eWatchUpdate = WU_NONE;
					break;
				case 0:	// CLOSE
				default:
					m_eWatchUpdate = WU_QUERY;
					break;
				}
			}
			break;
		}
	}
	//	To Here Dec. 4, 2002 genta
}





/*! 同一ファイルの再オープン */
void CEditDoc::ReloadCurrentFile(
	ECodeType	nCharCode,		/*!< [in] 文字コード種別 */
	bool	bReadOnly		/*!< [in] 読み取り専用モード */
)
{
	if( !fexist( GetFilePath() ) ){
		/* ファイルが存在しない */
		//	Jul. 26, 2003 ryoji BOMを標準設定に
		m_nCharCode = nCharCode;
		switch( m_nCharCode ){
		case CODE_UNICODE:
		case CODE_UNICODEBE:
			m_bBomExist = true;
			break;
		case CODE_UTF8:
		default:
			m_bBomExist = false;
			break;
		}
		return;
	}


	BOOL	bOpened;
	char	szFilePath[MAX_PATH];
	int		nViewTopLine;
	int		nViewLeftCol;
	int		nCaretPosX;
	int		nCaretPosY;
	nViewTopLine = m_pcEditViewArr[m_nActivePaneIndex]->m_nViewTopLine;	/* 表示域の一番上の行(0開始) */
	nViewLeftCol = m_pcEditViewArr[m_nActivePaneIndex]->m_nViewLeftCol;	/* 表示域の一番左の桁(0開始) */
	nCaretPosX = m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX;
	nCaretPosY = m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosY;

	strcpy( szFilePath, GetFilePath() );

	// Mar. 30, 2003 genta ブックマーク保存のためMRUへ登録
	AddToMRU();

	/* 既存データのクリア */
	InitDoc();

	/* 全ビューの初期化 */
	InitAllView();

	/* 親ウィンドウのタイトルを更新 */
	UpdateCaption();

	/* ファイル読み込み */
	FileRead(
		szFilePath,
		&bOpened,
		nCharCode,	/* 文字コード自動判別 */
		bReadOnly,	/* 読み取り専用か */
		false		/* 文字コード変更時の確認をするかどうか */
	);

	// レイアウト行単位のカーソル位置復元
	// ※ここではオプションのカーソル位置復元（＝改行単位）が指定されていない場合でも復元する
	// 2007.08.23 ryoji 表示領域復元
	if( nCaretPosY < m_cLayoutMgr.GetLineCount() ){
		m_pcEditViewArr[m_nActivePaneIndex]->m_nViewTopLine = nViewTopLine;
		m_pcEditViewArr[m_nActivePaneIndex]->m_nViewLeftCol = nViewLeftCol;
	}
	m_pcEditViewArr[m_nActivePaneIndex]->MoveCursorProperly( nCaretPosX, nCaretPosY, true );	// 2007.08.23 ryoji MoveCursor()->MoveCursorProperly()
	m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX_Prev = m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX;

	// 2006.09.01 ryoji オープン後自動実行マクロを実行する
	RunAutoMacro( m_pShareData->m_Common.m_sMacro.m_nMacroOnOpened );
}

//	From Here Nov. 20, 2000 genta
/*!	IME状態の設定
	
	@param mode [in] IMEのモード
	
	@date Nov 20, 2000 genta
*/
void CEditDoc::SetImeMode( int mode )
{
	DWORD	conv, sent;
	HIMC	hIme;

	hIme = ImmGetContext( m_hwndParent );

	//	最下位ビットはIME自身のOn/Off制御
	if( ( mode & 3 ) == 2 ){
		ImmSetOpenStatus( hIme, FALSE );
	}
	if( ( mode >> 2 ) > 0 ){
		ImmGetConversionStatus( hIme, &conv, &sent );

		switch( mode >> 2 ){
		case 1:	//	FullShape
			conv |= IME_CMODE_FULLSHAPE;
			conv &= ~IME_CMODE_NOCONVERSION;
			break;
		case 2:	//	FullShape & Hiragana
			conv |= IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE;
			conv &= ~( IME_CMODE_KATAKANA | IME_CMODE_NOCONVERSION );
			break;
		case 3:	//	FullShape & Katakana
			conv |= IME_CMODE_FULLSHAPE | IME_CMODE_NATIVE | IME_CMODE_KATAKANA;
			conv &= ~IME_CMODE_NOCONVERSION;
			break;
		case 4: //	Non-Conversion
			conv |= IME_CMODE_NOCONVERSION;
			break;
		}
		ImmSetConversionStatus( hIme, conv, sent );
	}
	if( ( mode & 3 ) == 1 ){
		ImmSetOpenStatus( hIme, TRUE );
	}
	ImmReleaseContext( m_hwndParent, hIme );
}
//	To Here Nov. 20, 2000 genta


/*!	$xの展開

	特殊文字は以下の通り
	@li $  $自身
	@li A  アプリ名
	@li F  開いているファイルのフルパス。名前がなければ(無題)。
	@li f  開いているファイルの名前（ファイル名+拡張子のみ）
	@li g  開いているファイルの名前（拡張子除く）
	@li /  開いているファイルの名前（フルパス。パスの区切りが/）
	@li N  開いているファイルの名前(簡易表示)
	@li E  開いているファイルのあるフォルダの名前(簡易表示)
	@li e  開いているファイルのあるフォルダの名前
	@li C  現在選択中のテキスト
	@li x  現在の物理桁位置(先頭からのバイト数1開始)
	@li y  現在の物理行位置(1開始)
	@li d  現在の日付(共通設定の日付書式)
	@li t  現在の時刻(共通設定の時刻書式)
	@li p  現在のページ
	@li P  総ページ
	@li D  ファイルのタイムスタンプ(共通設定の日付書式)
	@li T  ファイルのタイムスタンプ(共通設定の時刻書式)
	@li V  エディタのバージョン文字列
	@li h  Grep検索キーの先頭32byte
	@li S  サクラエディタのフルパス
	@li I  iniファイルのフルパス
	@li M  現在実行しているマクロファイルパス

	@date 2003.04.03 genta strncpy_ex導入によるfor文の削減
	@date 2005.09.15 FILE 特殊文字S, M追加
	@date 2007.09.21 kobake 特殊文字A(アプリ名)を追加
	@date 2008.05.05 novice GetModuleHandle(NULL)→NULLに変更
	@date 2012.10.11 Moca 特殊文字n追加
*/
void CEditDoc::ExpandParameter(const char* pszSource, char* pszBuffer, int nBufferLen)
{
	
	// Apr. 03, 2003 genta 固定文字列をまとめる
	static const char PRINT_PREVIEW_ONLY[] = "(印刷プレビューでのみ使用できます)";
	const int PRINT_PREVIEW_ONLY_LEN = sizeof( PRINT_PREVIEW_ONLY ) - 1;
	static const char NO_TITLE[] = "(無題)";
	const int NO_TITLE_LEN = sizeof( NO_TITLE ) - 1;
	static const char NOT_SAVED[] = "(保存されていません)";
	const int NOT_SAVED_LEN = sizeof( NOT_SAVED ) - 1;

	const char *p, *r;	//	p：目的のバッファ。r：作業用のポインタ。
	char *q, *q_max;
	for( p = pszSource, q = pszBuffer, q_max = pszBuffer + nBufferLen; *p != '\0' && q < q_max;){
		if( *p != '$' ){
			*q++ = *p++;
			continue;
		}
		switch( *(++p) ){
		case '$':	//	 $$ -> $
			*q++ = *p++;
			break;
		case 'A':	//アプリ名
			q = strncpy_ex( q, q_max - q, GSTR_APPNAME, _tcslen(GSTR_APPNAME) );
			++p;
			break;
		case 'F':	//	開いているファイルの名前（フルパス）
			if ( !IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
				++p;
			} 
			else {
				r = GetFilePath();
				q = strncpy_ex( q, q_max - q, r, strlen( r ));
				++p;
			}
			break;
		case 'f':	//	開いているファイルの名前（ファイル名+拡張子のみ）
			// Oct. 28, 2001 genta
			//	ファイル名のみを渡すバージョン
			//	ポインタを末尾に
			if ( ! IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
				++p;
			} 
			else {
				r = GetFileName(); // 2002.10.13 Moca ファイル名(パスなし)を取得。日本語対応
				//	万一\\が末尾にあってもその後ろには\0があるのでアクセス違反にはならない。
				q = strncpy_ex( q, q_max - q, r, strlen( r ));
				++p;
			}
			break;
		case 'g':	//	開いているファイルの名前（拡張子を除くファイル名のみ）
			//	From Here Sep. 16, 2002 genta
			if ( ! IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
				++p;
			} 
			else {
				//	ポインタを末尾に
				const char *dot_position, *end_of_path;
				r = GetFileName(); // 2002.10.13 Moca ファイル名(パスなし)を取得。日本語対応
				end_of_path = dot_position =
					r + strlen( r );
				//	後ろから.を探す
				for( --dot_position ; dot_position > r && *dot_position != '.'
					; --dot_position )
					;
				//	rと同じ場所まで行ってしまった⇔.が無かった
				if( dot_position == r )
					dot_position = end_of_path;

				q = strncpy_ex( q, q_max - q, r, dot_position - r );
				++p;
			}
			break;
			//	To Here Sep. 16, 2002 genta
		case '/':	//	開いているファイルの名前（フルパス。パスの区切りが/）
			// Oct. 28, 2001 genta
			if ( !IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
				++p;
			} 
			else {
				//	パスの区切りとして'/'を使うバージョン
				for( r = GetFilePath(); *r != '\0' && q < q_max; ++r, ++q ){
					if( *r == '\\' )
						*q = '/';
					else
						*q = *r;
				}
				++p;
			}
			break;
		//	From Here 2003/06/21 Moca
		case 'N':	//	開いているファイルの名前(簡易表示)
			if( !IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
				++p;
			}
			else {
				char szText[1024];
				CShareData::getInstance()->GetTransformFileNameFast( GetFilePath(), szText, 1023 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			break;
		//	To Here 2003/06/21 Moca
		case L'n':
			if( !IsValidPath() ){
				if( m_bGrepMode ){
				}else if( m_bDebugMode ){
				}else{
					TCHAR szText[10];
					const EditNode* node = CShareData::getInstance()->GetEditNode( m_pcEditWnd->m_hWnd );
					_stprintf( szText, "%d", node->m_nId );
					q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				}
			}
			++p;
			break;
		case 'E':	// 開いているファイルのあるフォルダの名前(簡易表示)	2012/12/2 Uchi
			if( !IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
			}
			else {
				TCHAR	buff[_MAX_PATH];		// \の処理をする為TCHAR
				TCHAR*	pEnd;
				TCHAR*	p;

				_tcscpy( buff, GetFilePath() );
				pEnd = NULL;
				for ( p = buff; *p != '\0'; p++) {
					if (*p == '\\') {
						pEnd = p;
					}
				}
				if (pEnd != NULL) {
					// 最後の\の後で終端
					*(pEnd+1) = '\0';
				}

				// 簡易表示に変換
				TCHAR szText[1024];
				CShareData::getInstance()->GetTransformFileNameFast( buff, szText, _countof(szText)-1 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
			}
			++p;
			break;
		case 'e':	// 開いているファイルのあるフォルダの名前		2012/12/2 Uchi
			if( !IsValidPath() ){
				q = strncpy_ex( q, q_max - q, NO_TITLE, NO_TITLE_LEN );
			}
			else {
				const TCHAR*	pStr;
				const TCHAR*	pEnd;
				const TCHAR*	p;

				pStr = GetFilePath();
				pEnd = pStr - strlen(pStr) - 1;
				for ( p = pStr; *p != '\0'; p++) {
					if (*p == '\\') {
						pEnd = p;
					}
				}
				q = strncpy_ex( q, q_max - q, pStr, _tcslen(pStr) );
			}
			++p;
			break;
		//	From Here Jan. 15, 2002 hor
		case 'C':	//	現在選択中のテキスト
			{
				CMemory cmemCurText;
				m_pcEditViewArr[m_nActivePaneIndex]->GetCurrentTextForSearch( cmemCurText );
				q = strncpy_ex( q, q_max - q, cmemCurText.GetStringPtr(), cmemCurText.GetStringLength());
				++p;
			}
		//	To Here Jan. 15, 2002 hor
			break;
		//	From Here 2002/12/04 Moca
		case 'x':	//	現在の物理桁位置(先頭からのバイト数1開始)
			{
				char szText[11];
				_itot( m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosX_PHY + 1, szText, 10 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			break;
		case 'y':	//	現在の物理行位置(1開始)
			{
				char szText[11];
				_itot( m_pcEditViewArr[m_nActivePaneIndex]->m_nCaretPosY_PHY + 1, szText, 10 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			break;
		//	To Here 2002/12/04 Moca
		case 'd':	//	共通設定の日付書式
			{
				char szText[1024];
				SYSTEMTIME systime;
				::GetLocalTime( &systime );
				CShareData::getInstance()->MyGetDateFormat( systime, szText, sizeof( szText ) - 1 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			break;
		case 't':	//	共通設定の時刻書式
			{
				char szText[1024];
				SYSTEMTIME systime;
				::GetLocalTime( &systime );
				CShareData::getInstance()->MyGetTimeFormat( systime, szText, sizeof( szText ) - 1 );
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			break;
		case 'p':	//	現在のページ
			{
				CEditWnd*	pcEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta
				if (pcEditWnd->m_pPrintPreview){
					char szText[1024];
					itoa(pcEditWnd->m_pPrintPreview->GetCurPageNum() + 1, szText, 10);
					q = strncpy_ex( q, q_max - q, szText, strlen(szText));
					++p;
				}
				else {
					q = strncpy_ex( q, q_max - q, PRINT_PREVIEW_ONLY, PRINT_PREVIEW_ONLY_LEN );
					++p;
				}
			}
			break;
		case 'P':	//	総ページ
			{
				CEditWnd*	pcEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta
				if (pcEditWnd->m_pPrintPreview){
					char szText[1024];
					itoa(pcEditWnd->m_pPrintPreview->GetAllPageNum(), szText, 10);
					q = strncpy_ex( q, q_max - q, szText, strlen(szText));
					++p;
				}
				else {
					q = strncpy_ex( q, q_max - q, PRINT_PREVIEW_ONLY, PRINT_PREVIEW_ONLY_LEN );
					++p;
				}
			}
			break;
		case 'D':	//	タイムスタンプ
			if (m_FileTime.GetFILETIME().dwLowDateTime){
				TCHAR szText[1024];
				CShareData::getInstance()->MyGetDateFormat(
					m_FileTime.GetSYSTEMTIME(),
					szText,
					_countof( szText ) - 1
				);
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			else {
				q = strncpy_ex( q, q_max - q, NOT_SAVED, NOT_SAVED_LEN );
				++p;
			}
			break;
		case 'T':	//	タイムスタンプ
			if (m_FileTime.GetFILETIME().dwLowDateTime){
				TCHAR szText[1024];
				CShareData::getInstance()->MyGetTimeFormat(
					m_FileTime.GetSYSTEMTIME(),
					szText,
					_countof( szText ) - 1
				);
				q = strncpy_ex( q, q_max - q, szText, strlen(szText));
				++p;
			}
			else {
				q = strncpy_ex( q, q_max - q, NOT_SAVED, NOT_SAVED_LEN );
				++p;
			}
			break;
		case 'V':	// Apr. 4, 2003 genta
			// Version number
			{
				char buf[28]; // 6(符号含むWORDの最大長) * 4 + 4(固定部分)
				//	2004.05.13 Moca バージョン番号は、プロセスごとに取得する
				DWORD dwVersionMS, dwVersionLS;
				GetAppVersionInfo( NULL, VS_VERSION_INFO,
					&dwVersionMS, &dwVersionLS );
				int len = sprintf( buf, "%d.%d.%d.%d",
					HIWORD( dwVersionMS ),
					LOWORD( dwVersionMS ),
					HIWORD( dwVersionLS ),
					LOWORD( dwVersionLS )
				);
				q = strncpy_ex( q, q_max - q, buf, len );
				++p;
			}
			break;
		case 'h':	//	Apr. 4, 2003 genta
			//	Grep Key文字列 MAX 32文字
			//	中身はSetParentCaption()より移植
			{
				CMemory		cmemDes;
				LimitStringLengthB( m_szGrepKey, _tcslen( m_szGrepKey ),
					(q_max - q > 32 ? 32 : q_max - q - 3), cmemDes );
				if( (int)_tcslen( m_szGrepKey ) > cmemDes.GetStringLength() ){
					cmemDes.AppendString( "...", 3 );
				}
				q = strncpy_ex( q, q_max - q, cmemDes.GetStringPtr(), cmemDes.GetStringLength());
				++p;
			}
			break;
		case 'S':	//	Sep. 15, 2005 FILE
			//	サクラエディタのフルパス
			{
				char	szPath[_MAX_PATH + 1];

				::GetModuleFileName( NULL, szPath, sizeof(szPath) );
				q = strncpy_ex( q, q_max - q, szPath, strlen(szPath) );
				++p;
			}
			break;
		case 'I':	//	May. 19, 2007 ryoji
			//	iniファイルのフルパス
			{
				char	szPath[_MAX_PATH + 1];

				CShareData::getInstance()->GetIniFileName( szPath );
				q = strncpy_ex( q, q_max - q, szPath, strlen(szPath) );
				++p;
			}
			break;
		case 'M':	//	Sep. 15, 2005 FILE
			//	現在実行しているマクロファイルパスの取得
			{
				// 実行中マクロのインデックス番号 (INVALID_MACRO_IDX:無効 / STAND_KEYMACRO:標準マクロ)
				switch( m_pcSMacroMgr->GetCurrentIdx() ){
				case INVALID_MACRO_IDX:
					break;
				case TEMP_KEYMACRO:
					{
						const TCHAR* pszMacroFilePath = m_pcSMacroMgr->GetFile(TEMP_KEYMACRO);
						q = strncpy_ex( q, q_max - q, pszMacroFilePath, strlen(pszMacroFilePath) );
					}
					break;
				case STAND_KEYMACRO:
					{
						char* pszMacroFilePath = CShareData::getInstance()->GetShareData()->m_Common.m_sMacro.m_szKeyMacroFileName;
						q = strncpy_ex( q, q_max - q, pszMacroFilePath, strlen(pszMacroFilePath) );
					}
					break;
				default:
					{
						char szMacroFilePath[_MAX_PATH * 2];
						int n = CShareData::getInstance()->GetMacroFilename( m_pcSMacroMgr->GetCurrentIdx(), szMacroFilePath, sizeof(szMacroFilePath) );
						if ( 0 < n ){
							q = strncpy_ex( q, q_max - q, szMacroFilePath, strlen(szMacroFilePath) );
						}
					}
					break;
				}
				++p;
			}
			break;
		//	Mar. 31, 2003 genta
		//	条件分岐
		//	${cond:string1$:string2$:string3$}
		//	
		case '{':	// 条件分岐
			{
				int cond;
				cond = ExParam_Evaluate( p + 1 );
				while( *p != '?' && *p != '\0' )
					++p;
				if( *p == '\0' )
					break;
				p = ExParam_SkipCond( p + 1, cond );
			}
			break;
		case ':':	// 条件分岐の中間
			//	条件分岐の末尾までSKIP
			p = ExParam_SkipCond( p + 1, -1 );
			break;
		case '}':	// 条件分岐の末尾
			//	特にすることはない
			++p;
			break;
		default:
			*q++ = '$';
			*q++ = *p++;
			break;
		}
	}
	*q = '\0';
}

/*! @brief 処理の読み飛ばし

	条件分岐の構文 ${cond:A0$:A1$:A2$:..$} において，
	指定した番号に対応する位置の先頭へのポインタを返す．
	指定番号に対応する物が無ければ$}の次のポインタを返す．

	${が登場した場合にはネストと考えて$}まで読み飛ばす．

	@param pszSource [in] スキャンを開始する文字列の先頭．cond:の次のアドレスを渡す．
	@param part [in] 移動する番号＝読み飛ばす$:の数．-1を与えると最後まで読み飛ばす．

	@return 移動後のポインタ．該当領域の先頭かあるいは$}の直後．

	@author genta
	@date 2003.03.31 genta 作成
*/
const char* CEditDoc::ExParam_SkipCond(const char* pszSource, int part)
{
	if( part == 0 )
		return pszSource;
	
	int nest = 0;	// 入れ子のレベル
	bool next = true;	// 継続フラグ
	const char *p;
	for( p = pszSource; next && *p != '\0'; ++p ) {
		if( *p == '$' && p[1] != '\0' ){ // $が末尾なら無視
			switch( *(++p)){
			case '{':	// 入れ子の開始
				++nest;
				break;
			case '}':
				if( nest == 0 ){
					//	終了ポイントに達した
					next = false; 
				}
				else {
					//	ネストレベルを下げる
					--nest;
				}
				break;
			case ':':
				if( nest == 0 && --part == 0){ // 入れ子でない場合のみ
					//	目的のポイント
					next = false;
				}
				break;
			}
		}
	}
	return p;
}

/*!	@brief 条件の評価

	@param pCond [in] 条件種別先頭．'?'までを条件と見なして評価する
	@return 評価の値

	@note
	ポインタの読み飛ばし作業は行わないので，'?'までの読み飛ばしは
	呼び出し側で別途行う必要がある．

	@author genta
	@date 2003.03.31 genta 作成

*/
int CEditDoc::ExParam_Evaluate( const char* pCond )
{
	switch( *pCond ){
	case 'R': // 読み取り専用
		if( m_bReadOnly ){	/* 読み取り専用モード */
			return 0;
		}else
		if( 0 != m_nFileShareModeOld && /* ファイルの排他制御モード */
			NULL == m_hLockedFile		/* ロックしていない */
		){
			return 1;
		}else{
			return 2;
		}
	case 'w': // Grepモード/Output Mode
		if( m_bGrepMode ){
			return 0;
		}else if( m_bDebugMode ){
			return 1;
		}else {
			return 2;
		}
	case 'M': // キーボードマクロの記録中
		if( TRUE == m_pShareData->m_sFlags.m_bRecordingKeyMacro &&
		m_pShareData->m_sFlags.m_hwndRecordingKeyMacro == m_hwndParent ){ /* ウィンドウ */
			return 0;
		}else {
			return 1;
		}
	case 'U': // 更新
		if( IsModified()){
			return 0;
		}
		else {
			return 1;
		}
	case 'N': // $N 新規/(無題)		2012/12/2 Uchi
		if (!IsValidPath()) {
			return 0;
		}
		else {
			return 1;
		}
	case 'I': // アイコン化されているか
		if( ::IsIconic( m_hwndParent )){
			return 0;
		} else {
 			return 1;
 		}
	default:
		return 0;
	}
	return 0;
}


/*[EOF]*/
