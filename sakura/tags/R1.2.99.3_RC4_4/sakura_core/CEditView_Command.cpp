//	$Id$
/*!	@file
	CEditViewクラスのコマンド処理系関数群

	@author Norio Nakatani
	@date	1998/07/17 作成
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "sakura_rc.h"
#include "CEditView.h"
#include "debug.h"
#include "keycode.h"
#include "funccode.h"
#include "CRunningTimer.h"
#include "charcode.h"
#include "CEditApp.h"
#include "CWaitCursor.h"
#include "CSplitterWnd.h"
#include "CMacro.h"
#include "CKeyMacroMgr.h"
#include "etc_uty.h"
#include "CDlgTypeList.h"
#include "CDlgProperty.h"
#include "CDlgCompare.h"
#include "global.h"
#include <htmlhelp.h>
#include "CRunningtimer.h"
#include "CDlgExec.h"
#include "CDlgAbout.h"	//Dec. 24, 2000 JEPRO 追加

//#include "mailapi32_api.h"
//#include "CDlgWords.h"


//	BOOL CALLBACK SendingMailDialogProc(
//	HWND hwndDlg,	/* ダイアログ ボックスのハンドル */
//	UINT uMsg,		/* メッセージ */
//	WPARAM wParam,	/* 第1メッセージ パラメータ */
//	LPARAM lParam	/* 第2メッセージ パラメータ */
//	)
//	{
//	switch( uMsg ){
//	case WM_INITDIALOG:
//		return TRUE;
//	default:
//		return FALSE;
//	}
//	}





/* コマンドコードによる処理振り分け */
BOOL CEditView::HandleCommand(
	int		nCommand,
	BOOL	bRedraw,
	LPARAM	lparam1,
	LPARAM	lparam2,
	LPARAM	lparam3,
	LPARAM	lparam4
)
{
	BOOL	bRet = TRUE;
	BOOL	bRepeat = FALSE;
	int		nFuncID;

	//	Aug, 14. 2000 genta
	if( m_pcEditDoc->IsModificationForbidden( nCommand ) ){
		return TRUE;
	}

//	if( 0 == m_pcEditDoc->m_nCommandExecNum ){
//		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION, GSTR_APPNAME, "一回目のコマンド" );
//	}
	++m_pcEditDoc->m_nCommandExecNum;		/* コマンド実行回数 */
//	if( nCommand != F_COPY ){
		/* 辞書Tipを消す */
		m_cTipWnd.Hide();
		m_dwTipTimer = ::GetTickCount();	/* 辞書Tip起動タイマー */
//	}
	/* 印刷プレビューモードか */
	if( TRUE == m_pcEditDoc->m_bPrintPreviewMode &&
		F_PRINT_PREVIEW != nCommand
	){
		::MessageBeep( MB_ICONHAND );
		return -1;
	}
	/* キーリピート状態 */
	if( m_bPrevCommand == nCommand ){
		bRepeat = TRUE;
	}
	m_bPrevCommand = nCommand;
	if( m_pShareData->m_bRecordingKeyMacro &&									/* キーボードマクロの記録中 */
		m_pShareData->m_hwndRecordingKeyMacro == ::GetParent( m_hwndParent )	/* キーボードマクロを記録中のウィンドウ */
	){
		/* キーリピート状態をなくする */
		bRepeat = FALSE;
		/* キーマクロに記録可能な機能かどうかを調べる */
		if( CMacro::CanFuncIsKeyMacro( nCommand ) ){
			/* キーマクロのバッファにデータ追加 */
			m_pShareData->m_CKeyMacroMgr.Append( nCommand, lparam1 );
		}else{
		}
	}
	/* キーボードマクロの実行中 */
	if( m_bExecutingKeyMacro ){
		/* キーリピート状態をなくする */
		bRepeat = FALSE;
	}


	if( m_bHokan ){
		if( nCommand != F_HOKAN
		 && nCommand != F_CHAR
		 && nCommand != F_IME_CHAR
		 ){
			m_pcEditDoc->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
	}
	if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
//		if( nCommand == F_TAGJUMP ){
//			return Command_TAGJUMP( FALSE );
//		}
		return -1;
//		while( NULL != m_pcOpeBlk ){}

//		delete m_pcOpeBlk;
//		m_pcOpeBlk = NULL;
	}
	m_pcOpeBlk = new COpeBlk;

//	if( !m_pcEditDoc->m_bDebugMode ){
//		char*		szCommandName[256];
//		/* デバッグモニタに出力 */
//		if( 0 < ::LoadString( m_hInstance, nCommand, (LPTSTR)szCommandName, sizeof( szCommandName ) ) ){
//			m_cShareData.TraceOut( "nCommand=%d %s\n", nCommand, szCommandName );
//		}else{
//			m_cShareData.TraceOut( "nCommand=%d [nknown]\n", nCommand );
//		}
//	}

	switch( nCommand ){
	case F_CHAR:	/* 文字入力 */
		/* コントロールコード入力禁止 */
		if(
			( ( (unsigned char)0x0 <= (unsigned char)lparam1 && (unsigned char)lparam1 <= (unsigned char)0x1F ) ||
			  ( (unsigned char)'~' <  (unsigned char)lparam1 && (unsigned char)lparam1 <  (unsigned char)'｡'  ) ||
			  ( (unsigned char)'ﾟ' <  (unsigned char)lparam1 && (unsigned char)lparam1 <= (unsigned char)0xff )
			) &&
			(unsigned char)lparam1 != TAB && (unsigned char)lparam1 != CR && (unsigned char)lparam1 != LF
		){
			::MessageBeep( MB_ICONHAND );
		}else{
			Command_CHAR( (char)lparam1 );
		}
		break;

	/* ファイル操作系 */
	case F_FILENEW:		Command_FILENEW();break;			/* 新規作成 */
	case F_FILEOPEN:	Command_FILEOPEN();break;			/* ファイルを開く */
	case F_FILESAVE:	bRet = Command_FILESAVE();break;	/* 上書き保存 */
	case F_FILESAVEAS:	bRet = Command_FILESAVEAS();break;	/* 名前を付けて保存 */
	case F_FILECLOSE:										//閉じて(無題)	//Oct. 17, 2000 jepro 「ファイルを閉じる」というキャプションを変更
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_FILECLOSE();
		break;
	case F_FILECLOSE_OPEN:	/* 閉じて開く */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_FILECLOSE_OPEN();
		break;
	case F_FILE_REOPEN_SJIS:	Command_FILE_REOPEN_SJIS();break;		//SJISで開き直す
	case F_FILE_REOPEN_JIS:		Command_FILE_REOPEN_JIS();break;		//JISで開き直す
	case F_FILE_REOPEN_EUC:		Command_FILE_REOPEN_EUC();break;		//EUCで開き直す
	case F_FILE_REOPEN_UNICODE:	Command_FILE_REOPEN_UNICODE();break;	//Unicodeで開き直す
	case F_FILE_REOPEN_UTF8:	Command_FILE_REOPEN_UTF8();break;		//UTF-8で開き直す
	case F_FILE_REOPEN_UTF7:	Command_FILE_REOPEN_UTF7();break;		//UTF-7で開き直す
	case F_PRINT:				Command_PRINT();break;					/* 印刷 */
	case F_PRINT_PREVIEW:		Command_PRINT_PREVIEW();break;			/* 印刷プレビュー */
	case F_PRINT_PAGESETUP:		Command_PRINT_PAGESETUP();break;		/* 印刷ページ設定 */	//Sept. 14, 2000 jepro 「印刷のページレイアウトの設定」から変更
	case F_OPEN_HfromtoC:		bRet = Command_OPEN_HfromtoC( (BOOL)lparam1 );break;	/* 同名のC/C++ヘッダ(ソース)を開く */	//Feb. 7, 2001 JEPRO 追加
	case F_OPEN_HHPP:			bRet = Command_OPEN_HHPP( (BOOL)lparam1 );break;		/* 同名のC/C++ヘッダファイルを開く */	//Feb. 9, 2001 jepro「.cまたは.cppと同名の.hを開く」から変更
	case F_OPEN_CCPP:			bRet = Command_OPEN_CCPP( (BOOL)lparam1 );break;		/* 同名のC/C++ソースファイルを開く */	//Feb. 9, 2001 jepro「.hと同名の.c(なければ.cpp)を開く」から変更
	case F_ACTIVATE_SQLPLUS:	Command_ACTIVATE_SQLPLUS();break;		/* Oracle SQL*Plusをアクティブ表示 */
	case F_PLSQL_COMPILE_ON_SQLPLUS:									/* Oracle SQL*Plusで実行 */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_PLSQL_COMPILE_ON_SQLPLUS();
		break;
	case F_BROWSE:				Command_BROWSE();break;				/* ブラウズ */
	case F_PROPERTY_FILE:		Command_PROPERTY_FILE();break;		/* ファイルのプロパティ */
	case F_EXITALL:				Command_EXITALL();break;			/* サクラエディタの全終了 */	//Dec. 26, 2000 JEPRO 追加

	/* 編集系 */
	case F_UNDO:				Command_UNDO();break;				/* 元に戻す(Undo) */
	case F_REDO:				Command_REDO();break;				/* やり直し(Redo) */
	case F_DELETE:				Command_DELETE(); break;			//削除
	case F_DELETE_BACK:			Command_DELETE_BACK(); break;		//カーソル前を削除
	case F_WordDeleteToStart:	Command_WordDeleteToStart(); break;	//単語の左端まで削除
	case F_WordDeleteToEnd:		Command_WordDeleteToEnd(); break;	//単語の右端まで削除
	case F_WordDelete:			Command_WordDelete(); break;		//単語削除
	case F_WordCut:				Command_WordCut(); break;			//単語切り取り
	case F_LineCutToStart:		Command_LineCutToStart(); break;	//行頭まで切り取り(改行単位)
	case F_LineCutToEnd:		Command_LineCutToEnd(); break;		//行末まで切り取り(改行単位)
	case F_LineDeleteToStart:	Command_LineDeleteToStart(); break;	//行頭まで削除(改行単位)
	case F_LineDeleteToEnd:		Command_LineDeleteToEnd(); break;	//行末まで削除(改行単位)
	case F_CUT_LINE:			Command_CUT_LINE();break;			//行切り取り(折り返し単位)
	case F_DELETE_LINE:			Command_DELETE_LINE();break;		//行削除(折り返し単位)
	case F_DUPLICATELINE:		Command_DUPLICATELINE();break;		//行の二重化(折り返し単位)
	case F_INDENT_TAB:												//TABインデント
		/* テキストが２行以上にまたがって選択されているか */
		if( IsTextSelected() &&
			0 != ( m_nSelectLineFrom - m_nSelectLineTo )
		){
			Command_INDENT( TAB );
		}else{
			/* １バイト文字入力 */
			Command_CHAR( (char)TAB );
		}
		break;
	case F_UNINDENT_TAB:	Command_UNINDENT( TAB );break;			//逆TABインデント
	case F_INDENT_SPACE:											//SPACEインデント
		/* テキストが２行以上にまたがって選択されているか */
		if( IsTextSelected() &&
			0 != ( m_nSelectLineFrom - m_nSelectLineTo )
		){
			Command_INDENT( SPACE );
		}else{
			/* １バイト文字入力 */
			Command_CHAR( (char)' ' );
		}
		break;
	case F_UNINDENT_SPACE:			Command_UNINDENT( SPACE );break;	//逆SPACEインデント
//	case F_WORDSREFERENCE:			Command_WORDSREFERENCE();break;		/* 単語リファレンス */

	/* カーソル移動系 */
	case F_IME_CHAR:		Command_IME_CHAR( (WORD)lparam1 ); break;					//全角文字入力
	case F_UP:				Command_UP( m_bSelectingLock, bRepeat ); break;				//カーソル上移動
	case F_DOWN:			Command_DOWN( m_bSelectingLock, bRepeat ); break;			//カーソル下移動
	case F_LEFT:			Command_LEFT( m_bSelectingLock, bRepeat ); break;			//カーソル左移動
	case F_RIGHT:			Command_RIGHT( m_bSelectingLock, FALSE, bRepeat ); break;	//カーソル右移動
	case F_UP2:				Command_UP2( m_bSelectingLock ); break;						//カーソル上移動(２行づつ)
	case F_DOWN2:			Command_DOWN2( m_bSelectingLock ); break;					//カーソル下移動(２行づつ)
	case F_WORDLEFT:		Command_WORDLEFT( m_bSelectingLock ); break;				/* 単語の左端に移動 */
	case F_WORDRIGHT:		Command_WORDRIGHT( m_bSelectingLock ); break;				/* 単語の右端に移動 */
	case F_GOLINETOP:		Command_GOLINETOP( m_bSelectingLock, FALSE ); break;		//行頭に移動(折り返し単位)
	case F_GOLINEEND:		Command_GOLINEEND( m_bSelectingLock, FALSE ); break;		//行末に移動(折り返し単位)
//	case F_ROLLDOWN:		Command_ROLLDOWN( m_bSelectingLock ); break;				//スクロールダウン
//	case F_ROLLUP:			Command_ROLLUP( m_bSelectingLock ); break;					//スクロールアップ
	case F_HalfPageUp:		Command_HalfPageUp( m_bSelectingLock ); break;				//半ページアップ	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
	case F_HalfPageDown:	Command_HalfPageDown( m_bSelectingLock ); break;			//半ページダウン	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
	case F_1PageUp:			Command_1PageUp( m_bSelectingLock ); break;					//１ページアップ	//Oct. 10, 2000 JEPRO 従来のページアップを半ページアップと名称変更し１ページアップを追加
	case F_1PageDown:		Command_1PageDown( m_bSelectingLock ); break;				//１ページダウン	//Oct. 10, 2000 JEPRO 従来のページダウンを半ページダウンと名称変更し１ページダウンを追加
	case F_GOFILETOP:		Command_GOFILETOP( m_bSelectingLock ); break;				//ファイルの先頭に移動
	case F_GOFILEEND:		Command_GOFILEEND( m_bSelectingLock ); break;				//ファイルの最後に移動
	case F_CURLINECENTER:	Command_CURLINECENTER(); break;								/* カーソル行をウィンドウ中央へ */
	case F_JUMPPREV:		Command_JUMPPREV(); break;									//移動履歴: 前へ
	case F_JUMPNEXT:		Command_JUMPNEXT(); break;									//移動履歴: 次へ
	case F_WndScrollDown:	Command_WndScrollDown(); break;								//テキストを１行下へスクロール	// 2001/06/20 asa-o
	case F_WndScrollUp:		Command_WndScrollUp(); break;								//テキストを１行上へスクロール	// 2001/06/20 asa-o

	/* 選択系 */
	case F_SELECTWORD:		Command_SELECTWORD( );break;					//現在位置の単語選択
	case F_SELECTALL:		Command_SELECTALL();break;						//すべて選択
	case F_BEGIN_SEL:		Command_BEGIN_SELECT();break;					/* 範囲選択開始 */
	case F_UP_SEL:			Command_UP( TRUE, bRepeat ); break;				//(範囲選択)カーソル上移動
	case F_DOWN_SEL:		Command_DOWN( TRUE, bRepeat ); break;			//(範囲選択)カーソル下移動
	case F_LEFT_SEL:		Command_LEFT( TRUE, bRepeat ); break;			//(範囲選択)カーソル左移動
	case F_RIGHT_SEL:		Command_RIGHT( TRUE, FALSE, bRepeat ); break;	//(範囲選択)カーソル右移動
	case F_UP2_SEL:			Command_UP2( TRUE ); break;						//(範囲選択)カーソル上移動(２行ごと)
	case F_DOWN2_SEL:		Command_DOWN2( TRUE );break;					//(範囲選択)カーソル下移動(２行ごと)
	case F_WORDLEFT_SEL:	Command_WORDLEFT( TRUE );break;					//(範囲選択)単語の左端に移動
	case F_WORDRIGHT_SEL:	Command_WORDRIGHT( TRUE );break;				//(範囲選択)単語の右端に移動
	case F_GOLINETOP_SEL:	Command_GOLINETOP( TRUE, FALSE );break;			//(範囲選択)行頭に移動(折り返し単位)
	case F_GOLINEEND_SEL:	Command_GOLINEEND( TRUE, FALSE );break;			//(範囲選択)行末に移動(折り返し単位)
//	case F_ROLLDOWN_SEL:	Command_ROLLDOWN( TRUE ); break;				//(範囲選択)スクロールダウン
//	case F_ROLLUP_SEL:		Command_ROLLUP( TRUE ); break;					//(範囲選択)スクロールアップ
	case F_HalfPageUp_Sel:	Command_HalfPageUp( TRUE ); break;				//(範囲選択)半ページアップ
	case F_HalfPageDown_Sel:Command_HalfPageDown( TRUE ); break;			//(範囲選択)半ページダウン
	case F_1PageUp_Sel:		Command_1PageUp( TRUE ); break;					//(範囲選択)１ページアップ
	case F_1PageDown_Sel:	Command_1PageDown( TRUE ); break;				//(範囲選択)１ページダウン
	case F_GOFILETOP_SEL:	Command_GOFILETOP( TRUE );break;				//(範囲選択)ファイルの先頭に移動
	case F_GOFILEEND_SEL:	Command_GOFILEEND( TRUE );break;				//(範囲選択)ファイルの最後に移動

	/* 矩形選択系 */
//	case F_BOXSELALL:		Command_BOXSELECTALL();break;		//矩形ですべて選択
	case F_BEGIN_BOX:		Command_BEGIN_BOXSELECT();break;	/* 矩形範囲選択開始 */
//	case F_UP_BOX:			Command_UP_BOX( bRepeat ); break;			//(矩形選択)カーソル上移動
//	case F_DOWN_BOX:		Command_DOWN( TRUE, bRepeat ); break;		//(矩形選択)カーソル下移動
//	case F_LEFT_BOX:		Command_LEFT( TRUE, bRepeat ); break;		//(矩形選択)カーソル左移動
//	case F_RIGHT_BOX:		Command_RIGHT( TRUE, FALSE, bRepeat ); break;//(矩形選択)カーソル右移動
//	case F_UP2_BOX:			Command_UP2( TRUE ); break;					//(矩形選択)カーソル上移動(２行ごと)
//	case F_DOWN2_BOX:		Command_DOWN2( TRUE );break;				//(矩形選択)カーソル下移動(２行ごと)
//	case F_WORDLEFT_BOX:	Command_WORDLEFT( TRUE );break;				//(矩形選択)単語の左端に移動
//	case F_WORDRIGHT_BOX:	Command_WORDRIGHT( TRUE );break;			//(矩形選択)単語の右端に移動
//	case F_GOLINETOP_BOX:	Command_GOLINETOP( TRUE, FALSE );break;		//(矩形選択)行頭に移動(折り返し単位)
//	case F_GOLINEEND_BOX:	Command_GOLINEEND( TRUE, FALSE );break;		//(矩形選択)行末に移動(折り返し単位)
//	case F_HalfPageUp_Box:	Command_HalfPageUp( TRUE ); break;			//(矩形選択)半ページアップ
//	case F_HalfPageDown_Box:Command_HalfPageDown( TRUE ); break;		//(矩形選択)半ページダウン
//	case F_1PageUp_Box:		Command_1PageUp( TRUE ); break;				//(矩形選択)１ページアップ
//	case F_1PageDown_Box:	Command_1PageDown( TRUE ); break;			//(矩形選択)１ページダウン
//	case F_GOFILETOP_Box:	Command_GOFILETOP( TRUE );break;			//(矩形選択)ファイルの先頭に移動
//	case F_GOFILEEND_Box:	Command_GOFILEEND( TRUE );break;			//(矩形選択)ファイルの最後に移動

	/* クリップボード系 */
	case F_CUT:						Command_CUT();break;					//切り取り(選択範囲をクリップボードにコピーして削除)
	case F_COPY:					Command_COPY( FALSE );break;			//コピー(選択範囲をクリップボードにコピー)
	case F_COPY_CRLF:				Command_COPY( FALSE, EOL_CRLF );break;	//CRLF改行でコピー(選択範囲をクリップボードにコピー)
	case F_PASTE:					Command_PASTE();break;					//貼り付け(クリップボードから貼り付け)
	case F_PASTEBOX:				Command_PASTEBOX();break;				//矩形貼り付け(クリップボードから矩形貼り付け)
	case F_INSTEXT:					Command_INSTEXT( bRedraw, (const char*)lparam1, (BOOL)lparam2 );break;/* テキストを貼り付け */
	case F_ADDTAIL:					Command_ADDTAIL( (const char*)lparam1, (int)lparam2 );break;	/* 最後にテキストを追加 */
	case F_COPYPATH:				Command_COPYPATH();break;				//このファイルのパス名をクリップボードにコピー
	case F_COPYTAG:					Command_COPYTAG();break;				//このファイルのパス名とカーソル位置をコピー	//Sept. 15, 2000 jepro 上と同じ説明になっていたのを修正
	case F_COPYLINES:				Command_COPYLINES();break;				//選択範囲内全行コピー
	case F_COPYLINESASPASSAGE:		Command_COPYLINESASPASSAGE();break;		//選択範囲内全行引用符付きコピー
	case F_COPYLINESWITHLINENUMBER:	Command_COPYLINESWITHLINENUMBER();break;//選択範囲内全行行番号付きコピー
	case F_CREATEKEYBINDLIST:		Command_CREATEKEYBINDLIST();break;		//キー割り当て一覧をコピー //Sept. 15, 2000 JEPRO 追加 //Dec. 25, 2000 復活

	/* 挿入系 */
	case F_INS_DATE:				Command_INS_DATE();break;	//日付挿入
	case F_INS_TIME:				Command_INS_TIME();break;	//時刻挿入

	/* 変換 */
	case F_TOLOWER:					Command_TOLOWER();break;				/* 英大文字→英小文字 */
	case F_TOUPPER:					Command_TOUPPER();break;				/* 英小文字→英大文字 */
	case F_TOHANKAKU:				Command_TOHANKAKU();break;				/* 全角→半角 */
	case F_TOZENKAKUKATA:			Command_TOZENKAKUKATA();break;			/* 半角＋全ひら→全角・カタカナ */	//Sept. 17, 2000 jepro 説明を「半角→全角カタカナ」から変更
	case F_TOZENKAKUHIRA:			Command_TOZENKAKUHIRA();break;			/* 半角＋全カタ→全角・ひらがな */	//Sept. 17, 2000 jepro 説明を「半角→全角ひらがな」から変更
	case F_HANKATATOZENKAKUKATA:	Command_HANKATATOZENKAKUKATA();break;	/* 半角カタカナ→全角カタカナ */
	case F_HANKATATOZENKAKUHIRA:	Command_HANKATATOZENKAKUHIRA();break;	/* 半角カタカナ→全角ひらがな */
	case F_TABTOSPACE:				Command_TABTOSPACE();break;				/* TAB→空白 */
	case F_SPACETOTAB:				Command_SPACETOTAB();break;				/* 空白→TAB */  //#### Stonee, 2001/05/27
	case F_CODECNV_AUTO2SJIS:		Command_CODECNV_AUTO2SJIS();break;		/* 自動判別→SJISコード変換 */
	case F_CODECNV_EMAIL:			Command_CODECNV_EMAIL();break;			/* E-Mail(JIS→SJIS)コード変換 */
	case F_CODECNV_EUC2SJIS:		Command_CODECNV_EUC2SJIS();break;		/* EUC→SJISコード変換 */
	case F_CODECNV_UNICODE2SJIS:	Command_CODECNV_UNICODE2SJIS();break;	/* Unicode→SJISコード変換 */
	case F_CODECNV_UTF82SJIS:		Command_CODECNV_UTF82SJIS();break;		/* UTF-8→SJISコード変換 */
	case F_CODECNV_UTF72SJIS:		Command_CODECNV_UTF72SJIS();break;		/* UTF-7→SJISコード変換 */
	case F_CODECNV_SJIS2JIS:		Command_CODECNV_SJIS2JIS();break;		/* SJIS→JISコード変換 */
	case F_CODECNV_SJIS2EUC:		Command_CODECNV_SJIS2EUC();break;		/* SJIS→EUCコード変換 */
	case F_CODECNV_SJIS2UTF8:		Command_CODECNV_SJIS2UTF8();break;		/* SJIS→UTF-8コード変換 */
	case F_CODECNV_SJIS2UTF7:		Command_CODECNV_SJIS2UTF7();break;		/* SJIS→UTF-7コード変換 */
	case F_BASE64DECODE:			Command_BASE64DECODE();break;			/* Base64デコードして保存 */
	case F_UUDECODE:				Command_UUDECODE();break;				/* uudecodeして保存 */	//Oct. 17, 2000 jepro 説明を「選択部分をUUENCODEデコード」から変更

	/* 検索系 */
	case F_SEARCH_DIALOG:		Command_SEARCH_DIALOG();break;												//検索(単語検索ダイアログ)
	case F_SEARCH_NEXT:			Command_SEARCH_NEXT( bRedraw, (HWND)lparam1, (const char*)lparam2 );break;	//次を検索
	case F_SEARCH_PREV:			Command_SEARCH_PREV( bRedraw, (HWND)lparam1 );break;						//前を検索
	case F_REPLACE:	//置換(置換ダイアログ)
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_REPLACE();
		break;
	case F_SEARCH_CLEARMARK:	Command_SEARCH_CLEARMARK();break;	//検索マークのクリア
	case F_GREP:	//Grep
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_GREP();
		break;
	case F_JUMP:			Command_JUMP();break;							//指定行ヘジャンプ
	case F_OUTLINE:			bRet = Command_FUNCLIST( (BOOL)lparam1 );break;	//アウトライン解析
	case F_TAGJUMP:			Command_TAGJUMP();break;						/* タグジャンプ機能 */
	case F_TAGJUMPBACK:		Command_TAGJUMPBACK();break;					/* タグジャンプバック機能 */
	case F_COMPARE:			Command_COMPARE();break;						/* ファイル内容比較 */
	case F_BRACKETPAIR:		Command_BRACKETPAIR();	break;					//対括弧の検索

	/* モード切り替え系 */
	case F_CHGMOD_INS:		Command_CHGMOD_INS();break;		//挿入／上書きモード切り替え
	case F_CANCEL_MODE:		Command_CANCEL_MODE();break;	//各種モードの取り消し

	/* 設定系 */
	case F_SHOWTOOLBAR:		Command_SHOWTOOLBAR();break;	/* ツールバーの表示/非表示 */
	case F_SHOWFUNCKEY:		Command_SHOWFUNCKEY();break;	/* ファンクションキーの表示/非表示 */
	case F_SHOWSTATUSBAR:	Command_SHOWSTATUSBAR();break;	/* ステータスバーの表示/非表示 */
	case F_TYPE_LIST:		Command_TYPE_LIST();break;		/* タイプ別設定一覧 */
	case F_OPTION_TYPE:		Command_OPTION_TYPE();break;	/* タイプ別設定 */
	case F_OPTION:			Command_OPTION();break;			/* 共通設定 */
	case F_FONT:			Command_FONT();break;			/* フォント設定 */
	case F_WRAPWINDOWWIDTH:	Command_WRAPWINDOWWIDTH();break;/* 現在のウィンドウ幅で折り返し */	//Oct. 7, 2000 JEPRO WRAPWINDIWWIDTH を WRAPWINDOWWIDTH に変更

	/* マクロ系 */
	case F_RECKEYMACRO:		Command_RECKEYMACRO();break;	/* キーマクロの記録開始／終了 */
	case F_SAVEKEYMACRO:	Command_SAVEKEYMACRO();break;	/* キーマクロの保存 */
	case F_LOADKEYMACRO:	Command_LOADKEYMACRO();break;	/* キーマクロの読み込み */
	case F_EXECKEYMACRO:									/* キーマクロの実行 */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_EXECKEYMACRO();break;
	//	From Here Sept. 20, 2000 JEPRO 名称CMMANDをCOMMANDに変更
	//	case F_EXECCMMAND:		Command_EXECCMMAND();break;	/* 外部コマンド実行 */
	case F_EXECCOMMAND:
		/* 再帰処理対策 */// 2001/06/23 N.Nakatani
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_EXECCOMMAND();	/* 外部コマンド実行 */
		break;
	//	To Here Sept. 20, 2000

	/* カスタムメニュー */
	case F_MENU_RBUTTON:	/* 右クリックメニュー */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_MENU_RBUTTON();
		break;
	case F_CUSTMENU_1:  /* カスタムメニュー1 */
	case F_CUSTMENU_2:  /* カスタムメニュー2 */
	case F_CUSTMENU_3:  /* カスタムメニュー3 */
	case F_CUSTMENU_4:  /* カスタムメニュー4 */
	case F_CUSTMENU_5:  /* カスタムメニュー5 */
	case F_CUSTMENU_6:  /* カスタムメニュー6 */
	case F_CUSTMENU_7:  /* カスタムメニュー7 */
	case F_CUSTMENU_8:  /* カスタムメニュー8 */
	case F_CUSTMENU_9:  /* カスタムメニュー9 */
	case F_CUSTMENU_10: /* カスタムメニュー10 */
	case F_CUSTMENU_11: /* カスタムメニュー11 */
	case F_CUSTMENU_12: /* カスタムメニュー12 */
	case F_CUSTMENU_13: /* カスタムメニュー13 */
	case F_CUSTMENU_14: /* カスタムメニュー14 */
	case F_CUSTMENU_15: /* カスタムメニュー15 */
	case F_CUSTMENU_16: /* カスタムメニュー16 */
	case F_CUSTMENU_17: /* カスタムメニュー17 */
	case F_CUSTMENU_18: /* カスタムメニュー18 */
	case F_CUSTMENU_19: /* カスタムメニュー19 */
	case F_CUSTMENU_20: /* カスタムメニュー20 */
	case F_CUSTMENU_21: /* カスタムメニュー21 */
	case F_CUSTMENU_22: /* カスタムメニュー22 */
	case F_CUSTMENU_23: /* カスタムメニュー23 */
	case F_CUSTMENU_24: /* カスタムメニュー24 */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		nFuncID = Command_CUSTMENU( nCommand - F_CUSTMENU_1 + 1 );
		if( 0 != nFuncID ){
			/* コマンドコードによる処理振り分け */
//			HandleCommand( nFuncID, TRUE, 0, 0, 0, 0 );
			::PostMessage( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, 0 ), (LPARAM)NULL );
		}
		break;

	/* ウィンドウ系 */
	case F_SPLIT_V:			Command_SPLIT_V();break;	/* 上下に分割 */	//Sept. 17, 2000 jepro 説明の「縦」を「上下に」に変更
	case F_SPLIT_H:			Command_SPLIT_H();break;	/* 左右に分割 */	//Sept. 17, 2000 jepro 説明の「横」を「左右に」に変更
	case F_SPLIT_VH:		Command_SPLIT_VH();break;	/* 縦横に分割 */	//Sept. 17, 2000 jepro 説明に「に」を追加
	case F_WINCLOSE:		Command_WINCLOSE();break;	//ウィンドウを閉じる
	case F_WIN_CLOSEALL:	/* すべてのウィンドウを閉じる */	//Oct. 7, 2000 jepro 「編集ウィンドウの全終了」を左記のように変更
		//Oct. 17, 2000 JEPRO 名前を変更(F_FILECLOSEALL→F_WIN_CLOSEALL)
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_FILECLOSEALL();
		break;
	case F_CASCADE:			Command_CASCADE();break;		//重ねて表示
	case F_TILE_V:			Command_TILE_V();break;			//上下に並べて表示
	case F_TILE_H:			Command_TILE_H();break;			//左右に並べて表示
	case F_MAXIMIZE_V:		Command_MAXIMIZE_V();break;		//縦方向に最大化
	case F_MAXIMIZE_H:		Command_MAXIMIZE_H();break;		//横方向に最大化 //2001.02.10 by MIK
	case F_MINIMIZE_ALL:	Command_MINIMIZE_ALL();break;	/* すべて最小化 */	//	Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
	case F_REDRAW:			Command_REDRAW();break;			/* 再描画 */
	case F_WIN_OUTPUT:		Command_WIN_OUTPUT();break;		//アウトプットウィンドウ表示

	/* 支援 */
	case F_HOKAN:			Command_HOKAN();break;			//入力補完
	case F_HELP_CONTENTS:	Command_HELP_CONTENTS();break;	/* ヘルプ目次 */				//Nov. 25, 2000 JEPRO 追加
	case F_HELP_SEARCH:		Command_HELP_SEARCH();break;	/* ヘルプトキーワード検索 */	//Nov. 25, 2000 JEPRO 追加
	case F_MENU_ALLFUNC:									/* コマンド一覧 */
		/* 再帰処理対策 */
		if( NULL != m_pcOpeBlk ){	/* 操作ブロック */
			delete m_pcOpeBlk;
			m_pcOpeBlk = NULL;
		}
		Command_MENU_ALLFUNC();break;
	case F_EXTHELP1:	Command_EXTHELP1();break;		/* 外部ヘルプ１ */
	case F_EXTHTMLHELP:	Command_EXTHTMLHELP();break;	/* 外部HTMLヘルプ */
	case F_ABOUT:	Command_ABOUT();break;				/* バージョン情報 */	//Dec. 24, 2000 JEPRO 追加

	/* その他 */
//	case F_SENDMAIL:	Command_SENDMAIL();break;		/* メール送信 */

	}

	/* アンドゥバッファの処理 */
	if( NULL != m_pcOpeBlk ){
		if( 0 < m_pcOpeBlk->GetNum() ){	/* 操作の数を返す */
			/* 操作の追加 */
			m_pcEditDoc->m_cOpeBuf.AppendOpeBlk( m_pcOpeBlk );

		//	2001/06/21 Start by asa-o: 他のペインの表示状態を更新
			m_pcEditDoc->m_cEditViewArr[m_nMyIndex^1].Redraw();
			m_pcEditDoc->m_cEditViewArr[m_nMyIndex^2].Redraw();
			m_pcEditDoc->m_cEditViewArr[(m_nMyIndex^1)^2].Redraw();
			DrawCaretPosInfo();
		//	2001/06/21 End

		}else{
			delete m_pcOpeBlk;
		}
		m_pcOpeBlk = NULL;
	}

	return bRet;
}



/////////////////////////////////// 以下はコマンド群 (Oct. 17, 2000 jepro note) ///////////////////////////////////////////

/* カーソル上移動 */
int CEditView::Command_UP( int bSelect, BOOL bRepeat )
{
	int		i;
	int		nRepeat;
	nRepeat = 0;
//m_pShareData->m_Common.m_nRepeatedScrollLineNum;		/* キーリピート時のスクロール行数 */
//m_pShareData->m_Common.m_nRepeatedScroll_Smooth;		/* キーリピート時のスクロールを滑らかにするか */

	/* キーリピート時のスクロールを滑らかにするか */
	if( !m_pShareData->m_Common.m_nRepeatedScroll_Smooth ){
		if( !bRepeat ){
			i = -1;
		}else{
			i = -1 * m_pShareData->m_Common.m_nRepeatedScrollLineNum;	/* キーリピート時のスクロール行数 */
		}
		Cursor_UPDOWN( i, bSelect );
		nRepeat = -1 * i;
	}else{
		++nRepeat;
		if( Cursor_UPDOWN( -1, bSelect ) && bRepeat ){
			for( i = 0; i < m_pShareData->m_Common.m_nRepeatedScrollLineNum - 1; ++i ){		/* キーリピート時のスクロール行数 */
				Cursor_UPDOWN( -1, bSelect );
				++nRepeat;
			}
		}
	}
	return nRepeat;
}




//jeprotestnow Oct. 25, 2000
/* (矩形選択)カーソル上移動 */
//int CEditView::Command_UP_BOX( BOOL bRepeat )
//{
//	/* 矩形範囲選択開始 */
//	Command_BEGIN_BOXSELECT();
//	/* カーソル上移動 */
//	return Command_UP( TRUE, bRepeat );
//}




/* カーソル下移動 */
int CEditView::Command_DOWN( int bSelect, BOOL bRepeat )
{
	int		i;
	int		nRepeat;
	nRepeat = 0;
	/* キーリピート時のスクロールを滑らかにするか */
	if( !m_pShareData->m_Common.m_nRepeatedScroll_Smooth ){
		if( !bRepeat ){
			i = 1;
		}else{
			i = m_pShareData->m_Common.m_nRepeatedScrollLineNum;	/* キーリピート時のスクロール行数 */
		}
		Cursor_UPDOWN( i, bSelect );
		nRepeat = i;
	}else{
		++nRepeat;
		if( Cursor_UPDOWN( 1, bSelect ) && bRepeat ){
			for( i = 0; i < m_pShareData->m_Common.m_nRepeatedScrollLineNum - 1; ++i ){	/* キーリピート時のスクロール行数 */
				Cursor_UPDOWN( 1, bSelect );
				++nRepeat;
			}
		}
	}
	return nRepeat;
}




/* カーソル左移動 */
int CEditView::Command_LEFT( int bSelect, BOOL bRepeat )
{
	int		nRepCount;
	int		nRepeat;
	int		nRes;
	if( bRepeat ){
		nRepeat = 2;
	}else{
		nRepeat = 1;
	}
	for( nRepCount = 0; nRepCount < nRepeat; ++nRepCount ){
		const char*		pLine;
		int				nLineLen;
		int				nPosX;
		int				nPosY = m_nCaretPosY;
		int				i;
		int				nCharChars;
		RECT			rcSel;
		const CLayout*	pcLayout;
		if( bSelect ){
			if( FALSE == IsTextSelected() ){	/* テキストが選択されているか */
				/* 現在のカーソル位置から選択を開始する */
				BeginSelectArea();
			}
		}else{
			if( TRUE == IsTextSelected() ){	/* テキストが選択されているか */
				/* 矩形範囲選択中か */
				if( m_bBeginBoxSelect ){
					/* 2点を対角とする矩形を求める */
					TwoPointToRect(
						&rcSel,
						m_nSelectLineFrom,		/* 範囲選択開始行 */
						m_nSelectColmFrom,		/* 範囲選択開始桁 */
						m_nSelectLineTo,		/* 範囲選択終了行 */
						m_nSelectColmTo			/* 範囲選択終了桁 */
					);
					/* 現在の選択範囲を非選択状態に戻す */
					DisableSelectArea( TRUE );
					/* カーソルを選択開始位置に移動 */
					MoveCursor( rcSel.left, rcSel.top, TRUE );
					m_nCaretPosX_Prev = m_nCaretPosX;
				}else{
					nPosX = m_nSelectColmFrom;
					nPosY = m_nSelectLineFrom;
					/* 現在の選択範囲を非選択状態に戻す */
					DisableSelectArea( TRUE );
					/* カーソルを選択開始位置に移動 */
					MoveCursor( nPosX, nPosY, TRUE );
					m_nCaretPosX_Prev = m_nCaretPosX;
				}
				nRes = 1;
				goto end_of_func;
			}
		}
		/* カーソルが左端にある */
		if( m_nCaretPosX == 0 ){
			if( m_nCaretPosY > 0 ){
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY - 1, &nLineLen, &pcLayout );
				if( NULL == pLine ){
					nLineLen = 0;
				}
				nPosX = 0;
				nCharChars = 0;
				for( i = 0; i < nLineLen; ){
					nPosX += nCharChars;
					if( i >= nLineLen - (pcLayout->m_cEol.GetLen()?1:0 ) ){
						i = nLineLen;
						break;
					}
					if( pLine[i] == TAB ){
						nCharChars = m_pcEditDoc->GetDocumentAttribute().m_nTabSpace
						 - ( nPosX % m_pcEditDoc->GetDocumentAttribute().m_nTabSpace );
						++i;
					}else{
						nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
						if( 0 == nCharChars ){
							nCharChars = 1;
						}
						i+= nCharChars;
					}
				}
				nPosY --;
			}else{
				nRes = 0;
				goto end_of_func;
			}
		}else{
			/* 現在行のデータを取得 */
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
			if( NULL == pLine ){
				nLineLen = 0;
			}
			nPosX = 0;
			for( i = 0; i < nLineLen; ){
				if( pLine[i] == TAB ){
					nCharChars = m_pcEditDoc->GetDocumentAttribute().m_nTabSpace
					 - ( nPosX % m_pcEditDoc->GetDocumentAttribute().m_nTabSpace );
					++i;
				}else{
					nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
					if( 0 == nCharChars ){
						nCharChars = 1;
					}
					i+= nCharChars;
				}
				if( nPosX + nCharChars >= m_nCaretPosX ){
					break;
				}
				nPosX += nCharChars;
			}
			if( i >= nLineLen ){
				nPosX = m_nCaretPosX - nCharChars;
			}
		}
		MoveCursor( nPosX, nPosY, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( bSelect ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nPosX, nPosY );
		}
		nRes = 1;
		goto end_of_func;
end_of_func:;
	}
	return nRes;
}




/* カーソル右移動 */
void CEditView::Command_RIGHT( int bSelect, int bIgnoreCurrentSelection, BOOL bRepeat )
{
	int		nRepCount;
	int		nRepeat;
//	int		nRes;
	if( bRepeat ){
		nRepeat = 2;
	}else{
		nRepeat = 1;
	}
	for( nRepCount = 0; nRepCount < nRepeat; ++nRepCount ){
		const char*	pLine;
		int			nLineLen;
		int			nPosX;
		int			nPosY = m_nCaretPosY;
		int			i;
		int			nCharChars;
		RECT		rcSel;
		const CLayout*	pcLayout;
		/* 現在行のデータを取得 */
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
		if( NULL == pLine ){
			goto end_of_func;
		}
		if( !bIgnoreCurrentSelection ){
			if( bSelect ){
				if( !IsTextSelected() ){	/* テキストが選択されているか */
					/* 現在のカーソル位置から選択を開始する */
					BeginSelectArea();
				}
			}else{
				if( IsTextSelected() ){	/* テキストが選択されているか */
					/* 矩形範囲選択中か */
					if( m_bBeginBoxSelect ){
						/* 2点を対角とする矩形を求める */
						TwoPointToRect(
							&rcSel,
							m_nSelectLineFrom,		/* 範囲選択開始行 */
							m_nSelectColmFrom,		/* 範囲選択開始桁 */
							m_nSelectLineTo,		/* 範囲選択終了行 */
							m_nSelectColmTo			/* 範囲選択終了桁 */
						);
						/* 現在の選択範囲を非選択状態に戻す */
						DisableSelectArea( TRUE );
						/* カーソルを選択終了位置に移動 */
						MoveCursor( rcSel.right, rcSel.bottom, TRUE );
						m_nCaretPosX_Prev = m_nCaretPosX;
					}else{
						nPosX = m_nSelectColmTo;
						nPosY = m_nSelectLineTo;

						/* 現在の選択範囲を非選択状態に戻す */
						DisableSelectArea( TRUE );
						if( nPosY >= m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
							/* ファイルの最後に移動 */
							Command_GOFILEEND(FALSE);
						}else{
							/* カーソルを選択終了位置に移動 */
							MoveCursor( nPosX, nPosY, TRUE );
							m_nCaretPosX_Prev = m_nCaretPosX;
						}
					}
					goto end_of_func;
				}
			}
		}
		nPosX = 0;
		for( i = 0; i < nLineLen; ){
			if( nPosX > m_nCaretPosX ){
				break;
			}
	//		if( i == nLineLen - 1 && (pLine[i] == '\n' || pLine[i] == '\r' ) ){
	//		if( i >= nLineLen - pcLayout->m_cEol.GetLen() ){
			if( i >= nLineLen - (pcLayout->m_cEol.GetLen()?1:0 ) ){
				i = nLineLen;
				break;
			}
			if( pLine[i] == TAB ){
				nCharChars = m_pcEditDoc->GetDocumentAttribute().m_nTabSpace
				 - ( nPosX % m_pcEditDoc->GetDocumentAttribute().m_nTabSpace );
				++i;
			}else{
				nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
				if( 0 == nCharChars ){
					nCharChars = 1;
				}
				i+= nCharChars;
			}
			nPosX += nCharChars;
		}
		if( i >= nLineLen ){
			/* フリーカーソルモードか */
			if( (
				m_pShareData->m_Common.m_bIsFreeCursorMode
			 || IsTextSelected() && m_bBeginBoxSelect	/* 矩形範囲選択中 */
				)
			 &&
				/* 改行で終わっているか */
				( EOL_NONE != pcLayout->m_cEol.GetLen() )
	//			(pLine[ nLineLen - 1 ] == '\n' || pLine[ nLineLen - 1 ] == '\r')
			){
				/*-- フリーカーソルモードの場合 --*/
				if( nPosX <= m_nCaretPosX ){
					/* 最終行か */
					if( m_nCaretPosY + 1 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
						/* 改行で終わっているか */
						if( EOL_NONE != pcLayout->m_cEol.GetType() ){
	//					if( pLine[ nLineLen - 1 ] == '\n' || pLine[ nLineLen - 1 ] == '\r' ){
							nPosX = m_nCaretPosX + 1;
						}else{
							nPosX = m_nCaretPosX;
						}
					}else{
						nPosX = m_nCaretPosX + 1;
					}
				}else{
					nPosX = nPosX;
				}
			}else{
				/*-- フリーカーソルモードではない場合 --*/
				/* 最終行か */
				if( m_nCaretPosY + 1 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
					/* 改行で終わっているか */
					if( EOL_NONE != pcLayout->m_cEol.GetType() ){
	//				if( pLine[ nLineLen - 1 ] == '\n' || pLine[ nLineLen - 1 ] == '\r' ){
						nPosX = 0;
						++nPosY;
					}else{
					}
				}else{
					nPosX = 0;
					++nPosY;
				}
			}
		}
		if( nPosX >= m_pcEditDoc->GetDocumentAttribute().m_nMaxLineSize ){
			nPosX = 0;
			++nPosY;
		}
		MoveCursor( nPosX, nPosY, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( bSelect ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nPosX, nPosY );
		}

end_of_func:;
	}
	return;
}




//	From Here Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL/UP/DOWN→PAGE/DOWN/UP)するために以下をコメントアウト
///* スクロールアップ */
//void CEditView::Command_ROLLUP( int bSelect )
//{
//	Cursor_UPDOWN( ( m_nViewRowNum / 2 ), bSelect );
//	return;
//}
//
//
//
//
///* スクロールダウン */
//void CEditView::Command_ROLLDOWN( int bSelect )
//{
//	Cursor_UPDOWN( - ( m_nViewRowNum / 2 ), bSelect );
//	return;
//}
//	To Here Oct. 6, 2000




/* 半ページアップ */	//Oct. 6, 2000 JEPRO added (実は従来のスクロールダウンそのもの)
void CEditView::Command_HalfPageUp( int bSelect )
{
	Cursor_UPDOWN( - ( m_nViewRowNum / 2 ), bSelect );
	return;
}




/* 半ページダウン */	//Oct. 6, 2000 JEPRO added (実は従来のスクロールアップそのもの)
void CEditView::Command_HalfPageDown( int bSelect )
{
	Cursor_UPDOWN( ( m_nViewRowNum / 2 ), bSelect );
	return;
}




/* １ページアップ */	//Oct. 10, 2000 JEPRO added
void CEditView::Command_1PageUp( int bSelect )
{
	Cursor_UPDOWN( - m_nViewRowNum, bSelect );
	return;
}




/* １ページダウン */	//Oct. 10, 2000 JEPRO added
void CEditView::Command_1PageDown( int bSelect )
{
	Cursor_UPDOWN( m_nViewRowNum, bSelect );
	return;
}




/* カーソル上移動(２行づつ) */
void CEditView::Command_UP2( int bSelect )
{
	Cursor_UPDOWN( -2, bSelect );
	return;
}




/* カーソル下移動(２行づつ) */
void CEditView::Command_DOWN2( int bSelect )
{
	Cursor_UPDOWN( 2, bSelect );
	return;
}




/* 行頭に移動(折り返し単位) */
void CEditView::Command_GOLINETOP( int bSelect, BOOL bLineTopOnly )
{
	const char*		pLine;
	int				nLineLen;
	int				nCaretPosX;
	int				nPos;
	const CLayout*	pcLayout;
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	nCaretPosX = 0;
proc_begin:;
	if( bLineTopOnly ){
		MoveCursor( nCaretPosX, m_nCaretPosY, TRUE );
		m_nCaretPosX_Prev = nCaretPosX;
		if( bSelect ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nCaretPosX, m_nCaretPosY );
		}
		return;
	}
	/* 現在行のデータを取得 */
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		return;
	}
	for( nPos = 0; nPos < nLineLen; ++nPos ){
		if( ' ' != pLine[nPos] && '\t' != pLine[nPos] ){
			if( CR == pLine[nPos] || LF == pLine[nPos] ){
				nPos = nLineLen;
			}
			break;
		}
	}
	if( nPos >= nLineLen ){
		nPos = 0;
	}
	/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
	nPos = LineIndexToColmn( pLine, nLineLen, nPos );
	if( m_nCaretPosX == nPos ){
		nCaretPosX = 0;
		bLineTopOnly = TRUE;
		goto proc_begin;
	}
	nCaretPosX = nPos;
	bLineTopOnly = TRUE;
	goto proc_begin;
	return;
}




/* 行末に移動(折り返し単位) */
void CEditView::Command_GOLINEEND( int bSelect, int bIgnoreCurrentSelection )
{
	const char*		pLine;
	int				nLineLen;
	int				nPosX;
	int				nPosY = m_nCaretPosY;
	int				i;
	int				nCharChars;
	const CLayout*	pcLayout;
	/* 現在行のデータを取得 */
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		return;
	}
	if( !bIgnoreCurrentSelection ){
		if( bSelect ){
			if( !IsTextSelected() ){	/* テキストが選択されているか */
				/* 現在のカーソル位置から選択を開始する */
				BeginSelectArea();
			}
		}else{
			if( IsTextSelected() ){	/* テキストが選択されているか */
				/* 現在の選択範囲を非選択状態に戻す */
				DisableSelectArea( TRUE );
			}
		}
	}
	nPosX = 0;
	nCharChars = 0;
	for( i = 0; i < nLineLen; ){
		nPosX += nCharChars;
//		if( i == nLineLen - 1 && ( pLine[i] == '\n' || pLine[i] == '\r' ) ){
//		if( i >= nLineLen - pcLayout->m_cEol.GetLen() ){
		if( i >= nLineLen - (pcLayout->m_cEol.GetLen()?1:0 ) ){
			i = nLineLen;
			break;
		}
		if( pLine[i] == TAB ){
			nCharChars = m_pcEditDoc->GetDocumentAttribute().m_nTabSpace
			 - ( nPosX % m_pcEditDoc->GetDocumentAttribute().m_nTabSpace );
			++i;
		}else{
			nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[i] ) - &pLine[i];
			if( 0 == nCharChars ){
				nCharChars = 1;
			}
			i+= nCharChars;
		}
	}
	if( i >= nLineLen ){
		if( m_nCaretPosY + 1 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
			/* 改行で終わっているか */
			if( EOL_NONE != pcLayout->m_cEol.GetLen() ){
//			if( pLine[ nLineLen - 1 ] == '\n' || pLine[ nLineLen - 1 ] == '\r' ){
			}else{
				nPosX += nCharChars;
			}
		}
	}
	MoveCursor( nPosX, m_nCaretPosY, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;
	if( bSelect ){
		/* 現在のカーソル位置によって選択範囲を変更 */
		ChangeSelectAreaByCurrentCursor( nPosX, m_nCaretPosY );
	}
	return;
}




/* ファイルの先頭に移動 */
void CEditView::Command_GOFILETOP( int bSelect )
{
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	/* 先頭へカーソルを移動 */
	//	Sep. 8, 2000 genta
	AddCurrentLineToHistory();
	MoveCursor( 0, 0, TRUE );
//やめた	m_nCaretPosX_Prev = m_nCaretPosX;
	if( bSelect ){
		/* 現在のカーソル位置によって選択範囲を変更 */
		ChangeSelectAreaByCurrentCursor( 0, 0 );
	}
	return;
}




/* ファイルの最後に移動 */
void CEditView::Command_GOFILEEND( int bSelect )
{
	HDC			hdc;
	PAINTSTRUCT	ps;
	int bIsFreeCursorMode_old = m_pShareData->m_Common.m_bIsFreeCursorMode;
	int	nLastLine;
	if( 0 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
		return;
	}
	m_pShareData->m_Common.m_bIsFreeCursorMode = FALSE;
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
//		/* 現在のカーソル位置によって選択範囲を変更 */
//		ChangeSelectAreaByCurrentCursor( 0, 0 );
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	nLastLine = m_pcEditDoc->m_cLayoutMgr.GetLineCount() - 1;
	//	Sep. 8, 2000 genta
	AddCurrentLineToHistory();
	MoveCursor( 0, nLastLine, TRUE );
	if( bSelect ){
		m_nSelectLineTo = m_nCaretPosY;		/* 範囲選択終了行 */
	}
	Command_DOWN( bSelect, TRUE );
	Command_GOLINEEND( bSelect, TRUE );
	Command_RIGHT( bSelect, TRUE, FALSE );
//やめた	m_nCaretPosX_Prev = m_nCaretPosX;
	m_pShareData->m_Common.m_bIsFreeCursorMode = bIsFreeCursorMode_old;
	/* 再描画 */
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	OnKillFocus();
	OnPaint( hdc, &ps, TRUE );	/* メモリＤＣを使用してちらつきのない再描画 */
	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );
	return;
}




/* 単語の左端に移動 */
void CEditView::Command_WORDLEFT( int bSelect )
{
	const char*		pLine;
	int				nLineLen;
	int				nIdx;
	int				nLineNew;
	int				nColmNew;
	BOOL			bIsFreeCursorModeOld;
	const CLayout*	pcLayout;
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){		/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		bIsFreeCursorModeOld = m_pShareData->m_Common.m_bIsFreeCursorMode;	/* フリーカーソルモードか */
		m_pShareData->m_Common.m_bIsFreeCursorMode = FALSE;
		/* カーソル左移動 */
		Command_LEFT( bSelect, FALSE );
		m_pShareData->m_Common.m_bIsFreeCursorMode = bIsFreeCursorModeOld;	/* フリーカーソルモードか */
		return;
	}
	/* 指定された桁に対応する行のデータ内の位置を調べる */
	nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );
	/* 現在位置の左の単語の先頭位置を調べる */
	if( m_pcEditDoc->m_cLayoutMgr.PrevOrNextWord( m_nCaretPosY, nIdx, &nLineNew, &nColmNew, TRUE ) ){
		/* 行が変わった */
		if( nLineNew != m_nCaretPosY ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNew, &nLineLen );
			if( NULL == pLine ){
				return;
			}
		}
		/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
		nColmNew = LineIndexToColmn( pLine, nLineLen, nColmNew );
		/* カーソル移動 */
		MoveCursor( nColmNew, nLineNew, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( bSelect ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nColmNew, nLineNew );
		}
	}else{
		bIsFreeCursorModeOld = m_pShareData->m_Common.m_bIsFreeCursorMode;	/* フリーカーソルモードか */
		m_pShareData->m_Common.m_bIsFreeCursorMode = FALSE;
		/* カーソル左移動 */
		Command_LEFT( bSelect, FALSE );
//		if( 0 < m_nCaretPosY ){
//			/* 行末に移動 */
//			Command_GOLINEEND( bSelect, FALSE );
//		}
		m_pShareData->m_Common.m_bIsFreeCursorMode = bIsFreeCursorModeOld;	/* フリーカーソルモードか */
	}
	return;
}




/* 単語の右端に移動 */
void CEditView::Command_WORDRIGHT( int bSelect )
{
	const char*	pLine;
	int			nLineLen;
	int			nIdx;
	int			nCurLine;
	int			nLineNew;
	int			nColmNew;
	int			bTryAgain;
	BOOL		bIsFreeCursorModeOld;
	if( bSelect ){
		if( !IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在のカーソル位置から選択を開始する */
			BeginSelectArea();
		}
	}else{
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	bTryAgain = FALSE;
try_again:;
	nCurLine = m_nCaretPosY;
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nCurLine, &nLineLen );
	if( NULL == pLine ){
		return;
	}
	if( bTryAgain ){
		if( pLine[0] != ' ' && pLine[0] != TAB ){
			return;
		}
	}
	/* 指定された桁に対応する行のデータ内の位置を調べる */
	nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );
	/* 現在位置の右の単語の先頭位置を調べる */
	if( m_pcEditDoc->m_cLayoutMgr.PrevOrNextWord( nCurLine, nIdx, &nLineNew, &nColmNew, FALSE ) ){
		/* 行が変わった */
		if( nLineNew != nCurLine ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNew, &nLineLen );
			if( NULL == pLine ){
				return;
			}
		}
		/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
		nColmNew = LineIndexToColmn( pLine, nLineLen, nColmNew );
		/* カーソル移動 */
		MoveCursor( nColmNew, nLineNew, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( bSelect ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nColmNew, nLineNew );
		}
	}else{
		bIsFreeCursorModeOld = m_pShareData->m_Common.m_bIsFreeCursorMode;	/* フリーカーソルモードか */
		m_pShareData->m_Common.m_bIsFreeCursorMode = FALSE;
		/* カーソル右移動 */
		Command_RIGHT( bSelect, FALSE, FALSE );
		m_pShareData->m_Common.m_bIsFreeCursorMode = bIsFreeCursorModeOld;	/* フリーカーソルモードか */
		if( FALSE == bTryAgain ){
			bTryAgain = TRUE;
			goto try_again;
		}
	}
	return;
}




/* 選択範囲をクリップボードにコピー */
void CEditView::Command_COPY( int bIgnoreLockAndDisdable,
			enumEOLType neweol )
{
	CMemory			cmemBuf;
//	HGLOBAL			hgClip;
//	char*			pszClip;
	const char*		pLine;
	int				nLineLen;
	BOOL			bBeginBoxSelect;
	const CLayout*	pcLayout;

	/* テキストが選択されているか */
	bBeginBoxSelect = FALSE;
	if( IsTextSelected() ){
		if( m_bBeginBoxSelect ){
			bBeginBoxSelect = TRUE;
		}
		/* 選択範囲のデータを取得 */
		/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
		if( FALSE == GetSelectedData( cmemBuf, FALSE, NULL, FALSE, neweol ) ){
			::MessageBeep( MB_ICONHAND );
			return;
		}
	}else{
		/* 非選択時は、カーソル行をコピーする */
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
		if( NULL == pLine ){
			return;
		}
		cmemBuf.SetData( pLine, nLineLen + (pcLayout->m_cEol.GetLen() == 0 ? 0 : (-1)) );
		if( pcLayout->m_cEol.GetLen() != 0 ){
			cmemBuf.AppendSz(
				( neweol == EOL_UNKNOWN ) ?
					pcLayout->m_cEol.GetValue() : CEOL(neweol).GetValue()
			);
		}
	}
	/* クリップボードにデータを設定 */
	if( FALSE == MySetClipboardData( cmemBuf.GetPtr( NULL ), cmemBuf.GetLength(), bBeginBoxSelect ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}


	if( !bIgnoreLockAndDisdable ){
		/* 選択状態のロック */
		if( m_bSelectingLock ){
			m_bSelectingLock = FALSE;
//			/* 現在の選択範囲を非選択状態に戻す */
//			DisableSelectArea( TRUE );
		}
	}
	if( m_pShareData->m_Common.m_bCopyAndDisablSelection ){	/* コピーしたら選択解除 */
		/* テキストが選択されているか */
		if( IsTextSelected() ){
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
	}
	return;
}




/* 切り取り(選択範囲をクリップボードにコピーして削除) */
void CEditView::Command_CUT( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	CMemory	cmemBuf;
	BOOL	bBeginBoxSelect;
	/* 範囲選択がされていない */
	if( !IsTextSelected() ){
		/* 非選択時は、カーソル行を切り取り */
		//行切り取り(折り返し単位)
		Command_CUT_LINE();
		return;
	}
	if( m_bBeginBoxSelect ){
		bBeginBoxSelect = TRUE;
	}else{
		bBeginBoxSelect = FALSE;
	}
	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();	/* 親ウィンドウのタイトルを更新 */


	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
	if( FALSE == GetSelectedData( cmemBuf, FALSE, NULL, FALSE ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* クリップボードにデータを設定 */
	if( FALSE == MySetClipboardData( cmemBuf.GetPtr( NULL ), cmemBuf.GetLength(), bBeginBoxSelect ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	/* カーソル位置または選択エリアを削除 */
	DeleteData( TRUE );
	return;
}




//カーソル位置または選択エリアを削除
void CEditView::Command_DELETE( void )
{
	if( m_bBeginSelect ){		/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	if( !IsTextSelected() ){	/* テキストが選択されているか */
		DeleteData( TRUE );
		return;
	}
	DeleteData( TRUE );
	return;
}




//カーソル前を削除
void CEditView::Command_DELETE_BACK( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	COpe*		pcOpe = NULL;
	BOOL		bBool;
	int			nPosX;
	int			nPosY;
	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();					/* 親ウィンドウのタイトルを更新 */
	if( IsTextSelected() ){				/* テキストが選択されているか */
		DeleteData( TRUE );
	}else{
		nPosX = m_nCaretPosX;
		nPosY = m_nCaretPosY;
		bBool = Command_LEFT( FALSE, FALSE );
		if( bBool ){
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
				pcOpe->m_nOpe = OPE_MOVECARET;				/* 操作種別 */
//				pcOpe->m_nCaretPosX_Before = nPosX;			/* 操作前のキャレット位置Ｘ */
//				pcOpe->m_nCaretPosY_Before = nPosY;			/* 操作前のキャレット位置Ｙ */
//				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//					pcOpe->m_nCaretPosX_Before,
//					pcOpe->m_nCaretPosY_Before,
//					&pcOpe->m_nCaretPosX_PHY_Before,
//					&pcOpe->m_nCaretPosY_PHY_Before
//				);
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					nPosX,
					nPosY,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);


//				pcOpe->m_nCaretPosX_After = m_nCaretPosX;	/* 操作後のキャレット位置Ｘ */
//				pcOpe->m_nCaretPosY_After = m_nCaretPosY;	/* 操作後のキャレット位置Ｙ */
//				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//					pcOpe->m_nCaretPosX_After,
//					pcOpe->m_nCaretPosY_After,
//					&pcOpe->m_nCaretPosX_PHY_After,
//					&pcOpe->m_nCaretPosY_PHY_After
//				);
				pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
				pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}
			DeleteData( TRUE );
		}
	}
	/* 入力補完機能を使用する */
	if( m_pShareData->m_Common.m_bUseHokan
 	 && FALSE == m_bExecutingKeyMacro	/* キーボードマクロの実行中 */
	){
		CMemory	cmemData;
		POINT	poWin;
		/* カーソル直前の単語を取得 */
		if( 0 < GetLeftWord( &cmemData, 100 ) ){
//			MYTRACE( "cmemData=[%s]\n", cmemData.GetPtr( NULL ) );
			/* 補完対象ワードリストを調べる */
			poWin.x = m_nViewAlignLeft
					 + (m_nCaretPosX - m_nViewLeftCol)
						* ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace );
			poWin.y = m_nViewAlignTop
					  + (m_nCaretPosY - m_nViewTopLine)
						* ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			::ClientToScreen( m_hWnd, &poWin );
			poWin.x -= (
				cmemData.GetLength()
				 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace )
			);
			if( 0 < m_pcEditDoc->m_cHokanMgr.Search(
//t				m_hFont_HAN,
				&poWin,
				m_nCharHeight,
				m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace,
				cmemData.GetPtr( NULL ),
//t				(void*)this,
//				m_pShareData->m_Common.m_szHokanFile	// 2001/06/14 asa-o 参照データ変更
				m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
				m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase	// 2001/06/19 asa-o 英大文字小文字を同一視する
			) ){
				m_bHokan = TRUE;
			}else{
				if( m_bHokan ){
					m_pcEditDoc->m_cHokanMgr.Hide();
					m_bHokan = FALSE;
				}
			}
		}else{
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}
	return;
}




//単語の右端まで削除
void CEditView::Command_WordDeleteToEnd( void )
{
	COpe*	pcOpe = NULL;
	CMemory	cmemData;

	/* 矩形選択状態では実行不能((★★もろ手抜き★★)) */
	if( IsTextSelected() ){
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			::MessageBeep( MB_ICONHAND );
			return;
		}
	}
	/* 単語の右端に移動 */
	CEditView::Command_WORDRIGHT( TRUE );
	if( FALSE == IsTextSelected() ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nOpe = OPE_MOVECARET;							/* 操作種別 */
//		pcOpe->m_nCaretPosX_Before = m_nSelectColmFrom;			/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nSelectLineFrom;			/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
			m_nSelectColmFrom,
			m_nSelectLineFrom,
			&pcOpe->m_nCaretPosX_PHY_Before,
			&pcOpe->m_nCaretPosY_PHY_Before
		);


//		pcOpe->m_nCaretPosX_After = pcOpe->m_nCaretPosX_Before;			/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = pcOpe->m_nCaretPosY_Before;			/* 操作後のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}
	/* 削除 */
	DeleteData( TRUE );
	return;
}




//単語の左端まで削除
void CEditView::Command_WordDeleteToStart( void )
{
	COpe*	pcOpe = NULL;
	CMemory	cmemData;
	/* 矩形選択状態では実行不能(★★もろ手抜き★★) */
	if( IsTextSelected() ){
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			::MessageBeep( MB_ICONHAND );
			return;
		}
	}
	/* 単語の左端に移動 */
	CEditView::Command_WORDLEFT( TRUE );
	if( FALSE == IsTextSelected() ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
//		pcOpe->m_nOpe = OPE_MOVECARET;					/* 操作種別 */
//		pcOpe->m_nCaretPosX_Before = m_nSelectColmTo;	/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nSelectLineTo;	/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
			m_nSelectColmTo,
			m_nSelectLineTo,
			&pcOpe->m_nCaretPosX_PHY_Before,
			&pcOpe->m_nCaretPosY_PHY_Before
		);


//		pcOpe->m_nCaretPosX_After = pcOpe->m_nCaretPosX_Before;			/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = pcOpe->m_nCaretPosY_Before;			/* 操作後のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}
	/* 削除 */
	DeleteData( TRUE );
	return;
}




//単語切り取り
void CEditView::Command_WordCut( void )
{
	if( IsTextSelected() ){
		/* 切り取り(選択範囲をクリップボードにコピーして削除) */
		Command_CUT();
		return;
	}
	//現在位置の単語選択
	Command_SELECTWORD();
	/* 切り取り(選択範囲をクリップボードにコピーして削除) */
	Command_CUT();
	return;
}




//単語削除
void CEditView::Command_WordDelete( void )
{
	if( IsTextSelected() ){
		/* 削除 */
		DeleteData( TRUE );
		return;
	}
	//現在位置の単語選択
	Command_SELECTWORD();
	/* 削除 */
	DeleteData( TRUE );
	return;
}




//行頭まで切り取り(改行単位)
void CEditView::Command_LineCutToStart( void )
{
	int			nX;
	int			nY;
	CLayout*	pCLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 切り取り(選択範囲をクリップボードにコピーして削除) */
		Command_CUT();
		return;
	}
	pCLayout = m_pcEditDoc->m_cLayoutMgr.Search( m_nCaretPosY );	/* 指定された物理行のレイアウトデータ(CLayout)へのポインタを返す */
	if( NULL == pCLayout ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( 0, pCLayout->m_nLinePhysical, &nX, &nY );
	if( m_nCaretPosX == nX && m_nCaretPosY == nY ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲の変更 */
	m_nSelectLineBgnFrom = nY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnFrom = nX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnTo = nY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnTo = nX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineFrom =	nY;
	m_nSelectColmFrom = nX;
	m_nSelectLineTo = m_nCaretPosY;
	m_nSelectColmTo = m_nCaretPosX;
//	/* 選択領域描画 */
//	DrawSelectArea();
	/*切り取り(選択範囲をクリップボードにコピーして削除) */
	Command_CUT();
	return;
}




//行末まで切り取り(改行単位)
void CEditView::Command_LineCutToEnd( void )
{
	int			nX;
	int			nY;
	CLayout*	pCLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 切り取り(選択範囲をクリップボードにコピーして削除) */
		Command_CUT();
		return;
	}
	pCLayout = m_pcEditDoc->m_cLayoutMgr.Search( m_nCaretPosY );	/* 指定された物理行のレイアウトデータ(CLayout)へのポインタを返す */
	if( NULL == pCLayout ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	if( EOL_NONE == pCLayout->m_pCDocLine->m_cEol ){	/* 改行コードの種類 */
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() , pCLayout->m_nLinePhysical, &nX, &nY );
	}else{
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() - 1, pCLayout->m_nLinePhysical, &nX, &nY );
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() - pCLayout->m_pCDocLine->m_cEol.GetLen(), pCLayout->m_nLinePhysical, &nX, &nY );
	}
	if( ( m_nCaretPosX == nX && m_nCaretPosY == nY )
	 || ( m_nCaretPosX >  nX && m_nCaretPosY == nY )
	){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲の変更 */
	m_nSelectLineBgnFrom = m_nCaretPosY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnFrom = m_nCaretPosX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnTo = m_nCaretPosY;		/* 範囲選択開始行(原点) */
	m_nSelectColmBgnTo = m_nCaretPosX;		/* 範囲選択開始桁(原点) */
	m_nSelectLineFrom =	m_nCaretPosY;
	m_nSelectColmFrom = m_nCaretPosX;
	m_nSelectLineTo = nY;
	m_nSelectColmTo = nX;
//	/* 選択領域描画 */
//	DrawSelectArea();
	/*切り取り(選択範囲をクリップボードにコピーして削除) */
	Command_CUT();
	return;
}




//行頭まで削除(改行単位)
void CEditView::Command_LineDeleteToStart( void )
{
	int			nX;
	int			nY;
	CLayout*	pCLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		DeleteData( TRUE );
		return;
	}
	pCLayout = m_pcEditDoc->m_cLayoutMgr.Search( m_nCaretPosY );	/* 指定された物理行のレイアウトデータ(CLayout)へのポインタを返す */
	if( NULL == pCLayout ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( 0, pCLayout->m_nLinePhysical, &nX, &nY );
	if( m_nCaretPosX == nX && m_nCaretPosY == nY ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲の変更 */
	m_nSelectLineBgnFrom = nY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnFrom = nX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnTo = nY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnTo = nX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineFrom =	nY;
	m_nSelectColmFrom = nX;
	m_nSelectLineTo = m_nCaretPosY;
	m_nSelectColmTo = m_nCaretPosX;
//	/* 選択領域描画 */
//	DrawSelectArea();
	/* 選択領域削除 */
	DeleteData( TRUE );
	return;
}




//行末まで削除(改行単位)
void CEditView::Command_LineDeleteToEnd( void )
{
	int			nX;
	int			nY;
	CLayout*	pCLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		DeleteData( TRUE );
		return;
	}
	pCLayout = m_pcEditDoc->m_cLayoutMgr.Search( m_nCaretPosY );	/* 指定された物理行のレイアウトデータ(CLayout)へのポインタを返す */
	if( NULL == pCLayout ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	if( EOL_NONE == pCLayout->m_pCDocLine->m_cEol ){	/* 改行コードの種類 */
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() , pCLayout->m_nLinePhysical, &nX, &nY );
	}else{
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() - 1, pCLayout->m_nLinePhysical, &nX, &nY );
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log( pCLayout->m_pCDocLine->m_pLine->GetLength() - pCLayout->m_pCDocLine->m_cEol.GetLen(), pCLayout->m_nLinePhysical, &nX, &nY );
	}
	if( ( m_nCaretPosX == nX && m_nCaretPosY == nY )
	 || ( m_nCaretPosX >  nX && m_nCaretPosY == nY )
	){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲の変更 */
	m_nSelectLineBgnFrom = m_nCaretPosY;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnFrom = m_nCaretPosX;	/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnTo = m_nCaretPosY;		/* 範囲選択開始行(原点) */
	m_nSelectColmBgnTo = m_nCaretPosX;		/* 範囲選択開始桁(原点) */
	m_nSelectLineFrom =	m_nCaretPosY;
	m_nSelectColmFrom = m_nCaretPosX;
	m_nSelectLineTo = nY;
	m_nSelectColmTo = nX;
//	/* 選択領域描画 */
//	DrawSelectArea();
	/* 選択領域削除 */
	DeleteData( TRUE );
	return;
}




//行切り取り(折り返し単位)
void CEditView::Command_CUT_LINE( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	const char*		pLine;
	int				nLineLen;
	int				nCaretPosX_OLD;
	int				nCaretPosY_OLD;
	const CLayout*	pcLayout;
	COpe*			pcOpe = NULL;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		::MessageBeep( MB_ICONHAND );
		return;
	}
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	m_nSelectLineFrom = m_nCaretPosY;		/* 範囲選択開始行 */
	m_nSelectColmFrom = m_nCaretPosX; 		/* 範囲選択開始桁 */
	m_nSelectLineTo = m_nCaretPosY;			/* 範囲選択終了行 */
	m_nSelectColmTo = m_nCaretPosX + 1;		/* 範囲選択終了桁 */
	nCaretPosX_OLD = m_nCaretPosX;
	nCaretPosY_OLD = m_nCaretPosY;
//	/* 選択範囲内の全行をクリップボードにコピーする */
	Command_COPYLINES();
	Command_DELETE();
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
	}else{
		/* 操作前の位置へカーソルを移動 */
		MoveCursor( nCaretPosX_OLD, nCaretPosY_OLD, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
			pcOpe->m_nOpe = OPE_MOVECARET;					/* 操作種別 */
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;		/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;		/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */

//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;						/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;						/* 操作後のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;				/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;				/* 操作後のキャレット位置Ｙ */
			pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
	}
	return;
}




/* 行削除(折り返し単位) */
void CEditView::Command_DELETE_LINE( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	const char*		pLine;
	int				nLineLen;
	int				nCaretPosX_OLD;
	int				nCaretPosY_OLD;
	COpe*			pcOpe = NULL;
	const CLayout*	pcLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		::MessageBeep( MB_ICONHAND );
		return;
	}
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
//	m_nSelectLineFrom = m_nCaretPosY;		/* 範囲選択開始行 */
//	m_nSelectColmFrom = m_nCaretPosX; 		/* 範囲選択開始桁 */
//	m_nSelectLineTo = m_nCaretPosY;			/* 範囲選択終了行 */
//	m_nSelectColmTo = m_nCaretPosX + 1;		/* 範囲選択終了桁 */
	m_nSelectLineFrom = m_nCaretPosY;		/* 範囲選択開始行 */
	m_nSelectColmFrom = 0; 					/* 範囲選択開始桁 */
	m_nSelectLineTo = m_nCaretPosY + 1;		/* 範囲選択終了行 */
	m_nSelectColmTo = 0;					/* 範囲選択終了桁 */

	nCaretPosX_OLD = m_nCaretPosX;
	nCaretPosY_OLD = m_nCaretPosY;
//	/* 選択範囲内の全行をクリップボードにコピーする */
//	CopySelectedAllLines(
//		NULL, /* 引用符 */
//		FALSE /* 行番号を付与する */
//	);
//	Command_COPYLINES();
	Command_DELETE();
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
	}else{
		/* 操作前の位置へカーソルを移動 */
		MoveCursor( nCaretPosX_OLD, nCaretPosY_OLD, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
			pcOpe->m_nOpe = OPE_MOVECARET;					/* 操作種別 */
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;		/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;		/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */

//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;						/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;						/* 操作後のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;				/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;				/* 操作後のキャレット位置Ｙ */
			pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
	}
	return;
}




/* すべて選択 */
void CEditView::Command_SELECTALL( void )
{
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
	}
	/* 先頭へカーソルを移動 */
	//	Sep. 8, 2000 genta
	AddCurrentLineToHistory();
	// MoveCursor( 0, 0, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;

	/* 全体を選択する */
//	m_nSelectLineBgn = 0;		/* 範囲選択開始行(原点) */
//	m_nSelectColmBgn = 0;		/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnFrom = 0;	/* 範囲選択開始行(原点) */
	m_nSelectColmBgnFrom = 0;	/* 範囲選択開始桁(原点) */
	m_nSelectLineBgnTo = 0;		/* 範囲選択開始行(原点) */
	m_nSelectColmBgnTo = 0;		/* 範囲選択開始桁(原点) */


	m_nSelectLineFrom =	0;
	m_nSelectColmFrom = 0;
	m_nSelectLineTo = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
	m_nSelectColmTo = 0;

	/* 選択領域描画 */
	DrawSelectArea();
	return;
}




//jeprotestnow Oct. 25, 2000
/* 矩形ですべて選択 */
//void CEditView::Command_SELBOXALL( void )
//{
//}




/* 現在位置の単語選択 */
void CEditView::Command_SELECTWORD( void )
{
	int				nLineFrom;
	int				nColmFrom;
	int				nLineTo;
	int				nColmTo;
	const char*		pLine;
	int				nLineLen;
	int				nIdx;
	const CLayout*	pcLayout;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
//		return;
	}
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		return;
	}
	/* 指定された桁に対応する行のデータ内の位置を調べる */
	nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );

	/* 現在位置の単語の範囲を調べる */
	if( m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
		m_nCaretPosY, nIdx,
		&nLineFrom, &nColmFrom, &nLineTo, &nColmTo, NULL, NULL ) ){

		/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineFrom, &nLineLen );
		nColmFrom = LineIndexToColmn( pLine, nLineLen, nColmFrom );
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineTo, &nLineLen );
		nColmTo = LineIndexToColmn( pLine, nLineLen, nColmTo );

		/* 選択範囲の変更 */
//		m_nSelectLineBgn = nLineFrom;		/* 範囲選択開始行(原点) */
//		m_nSelectColmBgn = nColmFrom;		/* 範囲選択開始桁(原点) */
		m_nSelectLineBgnFrom = nLineFrom;	/* 範囲選択開始行(原点) */
		m_nSelectColmBgnFrom = nColmFrom;	/* 範囲選択開始桁(原点) */
		m_nSelectLineBgnTo = nLineFrom;		/* 範囲選択開始行(原点) */
		m_nSelectColmBgnTo = nColmFrom;		/* 範囲選択開始桁(原点) */


		m_nSelectLineFrom =	nLineFrom;
		m_nSelectColmFrom = nColmFrom;
		m_nSelectLineTo = nLineTo;
		m_nSelectColmTo = nColmTo;

		/* 単語の先頭にカーソルを移動 */
		MoveCursor( nColmFrom, nLineFrom, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;

		/* 選択領域描画 */
		DrawSelectArea();
	}
	return;
}




/* 貼り付け(クリップボードから貼り付け) */
void CEditView::Command_PASTE( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}


//	HGLOBAL		hglb;
	char*		pszText;
	int			nNewLine;		/* 挿入された部分の次の位置の行 */
	int			nNewPos;		/* 挿入された部分の次の位置のデータ位置 */
	COpe*		pcOpe = NULL;
	CWaitCursor cWaitCursor( m_hWnd );
	BOOL		bBox;
	char		szPaste[1024];
	int			i;
	int			nTextLen;

	/* クリップボードからデータを取得 */
	CMemory		cmemClip;
	BOOL		bColmnSelect;
	if( FALSE == MyGetClipboardData( cmemClip, &bColmnSelect ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	/* 矩形コピーのテキストは常に矩形貼り付け */
	if( m_pShareData->m_Common.m_bAutoColmnPaste ){
		/* 矩形コピーのデータなら矩形貼り付け */
		if( bColmnSelect ){
			Command_PASTEBOX();
			return;
		}
	}
	pszText = cmemClip.GetPtr( &nTextLen );

	/* テキストが選択されているか */
	bBox = FALSE;
	if( IsTextSelected() ){
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			bBox = TRUE;
		}else{
//			DeleteData( TRUE );
			//	Jun. 23, 2000 genta
			//	同一行の行末以降のみが選択されている場合には選択無しと見なす
			int			len, pos;
			const char	*line;
			line = m_pcEditDoc->m_cLayoutMgr.GetLineStr( m_nSelectLineFrom, &len );

			pos = ( line == NULL ) ? 0 : LineColmnToIndex( line, len, m_nSelectColmFrom );
			if( pos >= len &&	//	開始位置が行末より後ろで
				m_nSelectLineFrom == m_nSelectLineTo	//	終了位置が同一行
				){
				m_nCaretPosX = m_nSelectColmFrom;
				DisableSelectArea(false);
			}
			else{

				/* データ置換 削除&挿入にも使える */
				ReplaceData_CEditView(
					m_nSelectLineFrom,		/* 範囲選択開始行 */
					m_nSelectColmFrom,		/* 範囲選択開始桁 */
					m_nSelectLineTo,		/* 範囲選択終了行 */
					m_nSelectColmTo,		/* 範囲選択終了桁 */
					NULL,					/* 削除されたデータのコピー(NULL可能) */
					pszText,				/* 挿入するデータ */
					nTextLen,				/* 挿入するデータの長さ */
					TRUE
				);
#ifdef _DEBUG
					gm_ProfileOutput = FALSE;
#endif
				return;
			}
		}
	}
	if( bBox ){
		for( i = 0; i < (int)nTextLen/*lstrlen( pszText )*/; i++  ){
			if( pszText[i] == CR || pszText[i] == LF ){
				break;
			}
		}
		memcpy( szPaste, pszText, i );
		szPaste[i] = '\0';
//		Command_INDENT( szPaste, lstrlen( szPaste ) );
		Command_INDENT( szPaste, i );
	}else{
		m_pcEditDoc->m_bIsModified = TRUE;				/* 変更フラグ */
		SetParentCaption();								/* 親ウィンドウのタイトルを更新 */
		if( !m_bDoing_UndoRedo ){						/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;	/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;	/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
		}
		/* 現在位置にデータを挿入 */
		InsertData_CEditView( m_nCaretPosX, m_nCaretPosY, pszText, nTextLen/*lstrlen(pszText)*/, &nNewLine, &nNewPos, pcOpe, TRUE );
//		::GlobalUnlock(hglb);
		/* 挿入データの最後へカーソルを移動 */
		MoveCursor( nNewPos, nNewLine, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( !m_bDoing_UndoRedo ){								/* アンドゥ・リドゥの実行中か */
//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_After,
//				pcOpe->m_nCaretPosY_After,
//				&pcOpe->m_nCaretPosX_PHY_After,
//				&pcOpe->m_nCaretPosY_PHY_After
//			);
			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
	}
	return;

}




///* テキストを貼り付け ver0 */
//void CEditView::Command_INSTEXT( BOOL bRedraw, const char* pData, int nDataLen )
//{
//	char*	pszText;
//	pszText = new char[ nDataLen + 1];
//	memcpy( pszText, pData, nDataLen );
//	pszText[nDataLen] = '\0';
//	Command_INSTEXT( bRedraw, pszText );
//	delete [] pszText;
//	return;
//}




/* テキストを貼り付け ver1 */
void CEditView::Command_INSTEXT( BOOL bRedraw, const char* pszText, BOOL bNoWaitCursor )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

//	HGLOBAL			hglb;
//	char*			lptstr;
	int				nNewLine;			/* 挿入された部分の次の位置の行 */
	int				nNewPos;			/* 挿入された部分の次の位置のデータ位置 */
	COpe*			pcOpe = NULL;
	CWaitCursor*	pcWaitCursor;
	BOOL			bBox;
	int				i;

	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	if( bRedraw ){
		SetParentCaption();				/* 親ウィンドウのタイトルを更新 */
	}
	if( bNoWaitCursor ){
		pcWaitCursor = NULL;
	}else{
		pcWaitCursor = new CWaitCursor( m_hWnd );
	}


////////////////////デバッグ用テスト→→→→→
//#ifdef _DEBUG
//	if( IsTextSelected()
//	 && FALSE == m_bBeginBoxSelect	/* 矩形範囲選択中ではない */
//	){
//		/* データ置換 削除&挿入にも使える */
//		ReplaceData_CEditView(
//			pszText,				/* 挿入するデータ */
//			strlen( pszText ),		/* 挿入するデータの長さ */
//			bRedraw,
//		);
//		return;
//	}
//#endif _DEBUG
////////////////////←←←←←←←デバッグ用テスト


	/* テキストが選択されているか */
	bBox = FALSE;
	if( IsTextSelected() ){
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			bBox = TRUE;
		}else{
			/* データ置換 削除&挿入にも使える */
			ReplaceData_CEditView(
				m_nSelectLineFrom,		/* 範囲選択開始行 */
				m_nSelectColmFrom,		/* 範囲選択開始桁 */
				m_nSelectLineTo,		/* 範囲選択終了行 */
				m_nSelectColmTo,		/* 範囲選択終了桁 */
				NULL,					/* 削除されたデータのコピー(NULL可能) */
				pszText,				/* 挿入するデータ */
				strlen( pszText ),		/* 挿入するデータの長さ */
				bRedraw
			);
#ifdef _DEBUG
				gm_ProfileOutput = FALSE;
#endif
			//	Jun, 7, 2000 みつ
			if( NULL != pcWaitCursor ){
				delete pcWaitCursor;
			}

			return;
		}
	}
	if( bBox ){
//		for( i = 0; i < (int)lstrlen( pszText ); i++  ){
		for( i = 0; i < (int)lstrlen( pszText ); i++  ){
			if( pszText[i] == CR || pszText[i] == LF ){
				break;
			}
		}
//		memcpy( szPaste, pszText, i );
//		szPaste[i] = '\0';
//		Command_INDENT( szPaste, lstrlen( szPaste ) );
		Command_INDENT( pszText, i );
	}else{
		m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
		SetParentCaption();			/* 親ウィンドウのタイトルを更新 */
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;	/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;	/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
		}
		/* 現在位置にデータを挿入 */
		InsertData_CEditView( m_nCaretPosX, m_nCaretPosY, pszText, lstrlen(pszText), &nNewLine, &nNewPos, pcOpe, TRUE );
//		::GlobalUnlock(hglb);
		/* 挿入データの最後へカーソルを移動 */
		MoveCursor( nNewPos, nNewLine, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;	/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;	/* 操作後のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_After,
//				pcOpe->m_nCaretPosY_After,
//				&pcOpe->m_nCaretPosX_PHY_After,
//				&pcOpe->m_nCaretPosY_PHY_After
//			);
			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
	}
	if( NULL != pcWaitCursor ){
		delete pcWaitCursor;
	}
	return;
}




/* 矩形貼り付け(クリップボードから矩形貼り付け) */
void CEditView::Command_PASTEBOX( void )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}


	if( FALSE == m_pShareData->m_Common.m_bFontIs_FIXED_PITCH ){	/* 現在のフォントは固定幅フォントである */
		return;
	}
//	HGLOBAL			hglb;
	char*			lptstr;
	int				nstrlen;
	int				nBgn;
	int				nPos;
	int				nCount;
	CMemory			cMem;
	int				nNewLine;		/* 挿入された部分の次の位置の行 */
	int				nNewPos;		/* 挿入された部分の次の位置のデータ位置 */
	int				nCurXOld;
	int				nCurYOld;
	COpe*			pcOpe = NULL;
	CWaitCursor 	cWaitCursor( m_hWnd );
	BOOL			bAddLastCR;
	int				nInsPosX;
	const char*		pLine;
	int				nLineLen;
//	BOOL			bBeginBoxSelect;
	const CLayout*	pcLayout;

	/* クリップボードからデータを取得 */
	CMemory			cmemClip;
	if( FALSE == MyGetClipboardData( cmemClip, NULL ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	lptstr = cmemClip.GetPtr( NULL );


	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();					/* 親ウィンドウのタイトルを更新 */

	/* テキストが選択されているか */
	if( IsTextSelected() ){
		DeleteData( TRUE );
	}
	nCurXOld = m_nCaretPosX;
	nCurYOld = m_nCaretPosY;

	nstrlen = lstrlen( lptstr );
	nCount = 0;
	nBgn = 0;
	for( nPos = 0; nPos < nstrlen; ){
		if( lptstr[nPos] == CR || lptstr[nPos] == LF ){
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
//				pcOpe->m_nCaretPosX_Before = nCurXOld/*m_nCaretPosX*/;			/* 操作前のキャレット位置Ｘ */
//				pcOpe->m_nCaretPosY_Before = nCurYOld + nCount/*m_nCaretPosY*/;	/* 操作前のキャレット位置Ｙ */
//				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//					pcOpe->m_nCaretPosX_Before,
//					pcOpe->m_nCaretPosY_Before,
//					&pcOpe->m_nCaretPosX_PHY_Before,
//					&pcOpe->m_nCaretPosY_PHY_Before
//				);
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					nCurXOld/*m_nCaretPosX*/,
					nCurYOld + nCount/*m_nCaretPosY*/,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);
			}
			/* 現在位置にデータを挿入 */
			if( m_pcEditDoc->m_cLayoutMgr.GetLineCount() <= nCurYOld + nCount ){
				InsertData_CEditView(
					nCurXOld,
					nCurYOld + nCount,
					&lptstr[nBgn],
					nPos - nBgn + 1,
					&nNewLine,
					&nNewPos,
					pcOpe,
					TRUE
				);
				{
					char szTest[1024];
					memcpy( szTest, &lptstr[nBgn], nPos - nBgn + 1 );
					szTest[nPos - nBgn + 1] = '\0';
//					MYTRACE( "ins-1:[%s]\n", szTest );
				}
			}else{
				if( nPos - nBgn > 0 ){
					InsertData_CEditView(
						nCurXOld,
						nCurYOld + nCount,
						&lptstr[nBgn],
						nPos - nBgn,
						&nNewLine,
						&nNewPos,
						pcOpe,
						TRUE
					);
					{
						char szTest[1024];
						memcpy( szTest, &lptstr[nBgn], nPos - nBgn );
						szTest[nPos - nBgn] = '\0';
//						MYTRACE( "ins-2:[%s]\n", szTest );
					}
				}
			}
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//				pcOpe->m_nCaretPosX_After = nNewPos/*m_nCaretPosX*/;	/* 操作後のキャレット位置Ｘ */
//				pcOpe->m_nCaretPosY_After = nNewLine/*m_nCaretPosY*/;	/* 操作後のキャレット位置Ｙ */
//				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//					pcOpe->m_nCaretPosX_After,
//					pcOpe->m_nCaretPosY_After,
//					&pcOpe->m_nCaretPosX_PHY_After,
//					&pcOpe->m_nCaretPosY_PHY_After
//				);
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					nNewPos/*m_nCaretPosX*/,
					nNewLine/*m_nCaretPosY*/,
					&pcOpe->m_nCaretPosX_PHY_After,
					&pcOpe->m_nCaretPosY_PHY_After
				);

				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}
			/* この行の挿入位置へカーソルを移動 */
			MoveCursor( nCurXOld, nCurYOld + nCount, TRUE );
			m_nCaretPosX_Prev = m_nCaretPosX;
			/* 行末に改行を付加するか？ */
			/* カーソル行が最後の行かつ行末に改行が無く、挿入すべきデータがまだある場合 */
			bAddLastCR = FALSE;
			if( m_pcEditDoc->m_cLayoutMgr.GetLineCount() - 1 == m_nCaretPosY ){
				nLineLen = 0;
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
				if( NULL != pLine && 1 <= nLineLen ){
					if( pLine[nLineLen - 1] == CR || pLine[nLineLen - 1] == LF ){
					}else{
						/* 挿入すべきデータがまだあるか */
						int nPosWork;
						for( nPosWork = nPos; nPosWork < nstrlen; ++nPosWork ){
							if( lptstr[nPosWork] == CR ||  lptstr[nPosWork] == LF ){
							}else{
								bAddLastCR = TRUE;
								nInsPosX = LineIndexToColmn( pLine, nLineLen, nLineLen );
								break;
							}
						}
					}
				}
			}
			if( bAddLastCR ){
//				MYTRACE( " カーソル行が最後の行かつ行末に改行が無く、\n挿入すべきデータがまだある場合は行末に改行を挿入。\n" );
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					pcOpe = new COpe;
//					pcOpe->m_nCaretPosX_Before = nInsPosX/*m_nCaretPosX*/;		/* 操作前のキャレット位置Ｘ */
//					pcOpe->m_nCaretPosY_Before = m_nCaretPosY/*m_nCaretPosY*/;	/* 操作前のキャレット位置Ｙ */
//					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//						pcOpe->m_nCaretPosX_Before,
//						pcOpe->m_nCaretPosY_Before,
//						&pcOpe->m_nCaretPosX_PHY_Before,
//						&pcOpe->m_nCaretPosY_PHY_Before
//					);
					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
						nInsPosX/*m_nCaretPosX*/,
						m_nCaretPosY/*m_nCaretPosY*/,
						&pcOpe->m_nCaretPosX_PHY_Before,
						&pcOpe->m_nCaretPosY_PHY_Before
					);
				}
				InsertData_CEditView(
					nInsPosX,
					m_nCaretPosY,
					CRLF,
					1,
					&nNewLine,
					&nNewPos,
					pcOpe,
					TRUE
				);
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//					pcOpe->m_nCaretPosX_After = nNewPos/*m_nCaretPosX*/;	/* 操作後のキャレット位置Ｘ */
//					pcOpe->m_nCaretPosY_After = nNewLine/*m_nCaretPosY*/;	/* 操作後のキャレット位置Ｙ */
//					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//						pcOpe->m_nCaretPosX_After,
//						pcOpe->m_nCaretPosY_After,
//						&pcOpe->m_nCaretPosX_PHY_After,
//						&pcOpe->m_nCaretPosY_PHY_After
//					);

					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
						nNewPos/*m_nCaretPosX*/,
						nNewLine/*m_nCaretPosY*/,
						&pcOpe->m_nCaretPosX_PHY_After,
						&pcOpe->m_nCaretPosY_PHY_After
					);

					/* 操作の追加 */
					m_pcOpeBlk->AppendOpe( pcOpe );
				}
			}
			if( (nPos + 1 < nstrlen ) &&
				(
				 ( lptstr[nPos] == '\n' && lptstr[nPos + 1] == '\r') ||
				 ( lptstr[nPos] == '\r' && lptstr[nPos + 1] == '\n')
				)
			){
				nBgn = nPos + 2;
			}else{
				nBgn = nPos + 1;
			}
			nPos = nBgn;
			++nCount;
		}else{
			++nPos;
		}
	}
	if( nPos - nBgn > 0 ){
		/* 現在位置にデータを挿入 */
		cMem.SetData( &lptstr[nBgn], nPos - nBgn );
		if( m_pcEditDoc->m_cLayoutMgr.GetLineCount() <= nCurYOld + nCount ){
			cMem += CR;
		}
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
//			pcOpe->m_nCaretPosX_Before = nCurXOld/*m_nCaretPosX*/;			/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = nCurYOld + nCount/*m_nCaretPosY*/;	/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
				nCurXOld/*m_nCaretPosX*/,
				nCurYOld + nCount/*m_nCaretPosY*/,
				&pcOpe->m_nCaretPosX_PHY_Before,
				&pcOpe->m_nCaretPosY_PHY_Before
			);
		}
		InsertData_CEditView(
			nCurXOld,
			nCurYOld + nCount,
			cMem.GetPtr( NULL ),
			cMem.GetLength(),
			&nNewLine,
			&nNewPos,
			pcOpe,
			TRUE
		);
//		{
//			char szTest[1024];
//			memcpy( szTest, cMem.GetPtr( NULL ), cMem.GetLength() );
//			szTest[cMem.GetLength()] = '\0';
//			MYTRACE( "ins-3:[%s]\n", szTest );
//		}
		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//			pcOpe->m_nCaretPosX_After = nNewPos/*m_nCaretPosX*/;	/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = nNewLine/*m_nCaretPosY*/;	/* 操作後のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_After,
//				pcOpe->m_nCaretPosY_After,
//				&pcOpe->m_nCaretPosX_PHY_After,
//				&pcOpe->m_nCaretPosY_PHY_After
//			);

			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
				nNewPos/*m_nCaretPosX*/,
				nNewLine/*m_nCaretPosY*/,
				&pcOpe->m_nCaretPosX_PHY_After,
				&pcOpe->m_nCaretPosY_PHY_After
			);

			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
		/* この行の挿入位置へカーソルを移動 */
		MoveCursor( nCurXOld, nCurYOld + nCount, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;
	}
	/* 挿入データの先頭位置へカーソルを移動 */
	MoveCursor( nCurXOld, nCurYOld, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nOpe = OPE_MOVECARET;						/* 操作種別 */
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;			/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;			/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */

//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;						/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;						/* 操作後のキャレット位置Ｙ */
//		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;				/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;				/* 操作後のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}
	return;
}




/* １バイト文字入力 */
void CEditView::Command_CHAR( char cChar )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}


	CMemory			cmemData;
	CMemory			cmemIndent;
	int				nPos;
	const char*		pLine;
	int				nLineLen;
	int				nCharChars;
	int				nIdxTo;
	int				nPosX;
	int				nNewLine;	/* 挿入された部分の次の位置の行 */
	int				nNewPos;	/* 挿入された部分の次の位置のデータ位置 */
	COpe*			pcOpe = NULL;
	char			szCurrent[10];
	POINT			poWin;
	const CLayout*	pcLayout;

	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();	/* 親ウィンドウのタイトルを更新 */

	/* 現在位置にデータを挿入 */
	nPosX = -1;
	cmemData = cChar;
	if( cChar == CR ||
		cChar == LF ){
		/* 現在、Enterなどで挿入する改行コードの種類を取得 */
		// enumEOLType nWorkEOL;
		CEOL cWork;
		cWork = GetCurrentInsertEOL();
		cmemData.SetData( cWork.GetValue(), cWork.GetLen() );

		/* テキストが選択されているか */
		if( IsTextSelected() ){
			DeleteData( TRUE );
		}
		if( m_pcEditDoc->m_bGrepMode && m_pShareData->m_Common.m_bGTJW_RETURN ){
			/* タグジャンプ機能 */
			Command_TAGJUMP();
			return;
		}else
		if( m_pShareData->m_Common.m_bAutoIndent ){	/* オートインデント */
			const CLayout* pCLayout;
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pCLayout );
			if( NULL != pCLayout ){
				pLine = m_pcEditDoc->m_cDocLineMgr.GetLineStr( pCLayout->m_nLinePhysical, &nLineLen );
//				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
				if( NULL != pLine ){
					/*
					  カーソル位置変換
					  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
					  →
					  物理位置(行頭からのバイト数、折り返し無し行位置)
					*/
					int		nX;
					int		nY;
					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
						m_nCaretPosX,
						m_nCaretPosY,
						&nX,
						&nY
					);

					/* 指定された桁に対応する行のデータ内の位置を調べる */
					nIdxTo = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );
					for( nPos = 0; nPos < /*nIdxTo*/nLineLen && nPos < nX; ){
						nCharChars = CMemory::MemCharNext( pLine, nLineLen, &pLine[nPos] ) - &pLine[nPos];

						/* その他のインデント文字 */
						if( 0 < nCharChars
						 && 0 < (int)lstrlen( m_pcEditDoc->GetDocumentAttribute().m_szIndentChars )
						){
							memcpy( szCurrent, &pLine[nPos], nCharChars );
							szCurrent[nCharChars] = '\0';
							/* その他のインデント対象文字 */
							if( NULL != strstr(
								m_pcEditDoc->GetDocumentAttribute().m_szIndentChars,
								szCurrent
							) ){
								goto end_of_for;
							}
						}
						if( nCharChars == 1 ){
							if( pLine[nPos] == SPACE ||
								pLine[nPos] == TAB ){
							}else{
								break;
							}
						}else
						if( nCharChars == 2 ){
							if( m_pShareData->m_Common.m_bAutoIndent_ZENSPACE ){	/* 日本語空白もインデント */
								if( pLine[nPos    ] == (char)0x81 &&
									pLine[nPos + 1] == (char)0x40 ){
								}else{
									break;
								}
							}else{
								break;
							}
						}else
						if( nCharChars == 0 ){
							break;
						}else{
							break;
						}
						end_of_for:;
						nPos += nCharChars;
					}
					if( nPos > 0 ){
						nPosX = LineIndexToColmn( pLine, nLineLen, nPos );
					}
					cmemIndent.SetData( pLine, nPos );
					cmemData += cmemIndent;
				}
			}
		}
	}else{
		/* テキストが選択されているか */
		if( IsTextSelected() ){
			/* 矩形範囲選択中か */
			if( m_bBeginBoxSelect ){
				Command_INDENT( cChar );
				return;
			}else{
				DeleteData( TRUE );
			}
		}else{
			if( !m_pShareData->m_Common.m_bIsINSMode ){		/* 挿入／上書きモード */
				BOOL bDelete = TRUE;
				if( m_pShareData->m_Common.m_bNotOverWriteCRLF ){	/* 改行は上書きしない */
					pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
					if( NULL != pLine ){
						/* 指定された桁に対応する行のデータ内の位置を調べる */
						nIdxTo = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );
//						if( nIdxTo == nLineLen - 1 && ( pLine[nIdxTo] == CR || pLine[nIdxTo] == LF ) ){
						if( nIdxTo == nLineLen - (pcLayout->m_cEol.GetLen()?1:0 ) ){

							/* 現在位置が改行ならば削除しない */
							bDelete = FALSE;
						}
					}
				}
				if( bDelete ){
					/* 上書きモードなので、現在位置の文字を１文字消去 */
					DeleteData( FALSE );
				}
			}
		}
	}
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;			/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;			/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
	}


	InsertData_CEditView(
		m_nCaretPosX,
		m_nCaretPosY,
		cmemData.GetPtr( NULL ),
		cmemData.GetLength(),
		&nNewLine,
		&nNewPos,
		pcOpe,
		TRUE
	);
	/* 挿入データの最後へカーソルを移動 */
	MoveCursor( nNewPos, nNewLine, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;
//?	if( nPosX != -1 ){
//?		MoveCursor( nPosX, m_nCaretPosY, TRUE );
//?		m_nCaretPosX_Prev = m_nCaretPosX;
//?	}
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;	/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;	/* 操作後のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_After,
//			pcOpe->m_nCaretPosY_After,
//			&pcOpe->m_nCaretPosX_PHY_After,
//			&pcOpe->m_nCaretPosY_PHY_After
//		);
		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}


	/* スマートインデント */
	if( SMARTINDENT_CPP == m_pcEditDoc->GetDocumentAttribute().m_nSmartIndent ){	/* スマートインデント種別 */
		/* C/C++スマートインデント処理 */
		SmartIndent_CPP( cChar );
	}


	/* 入力補完機能を使用する */
	if( m_pShareData->m_Common.m_bUseHokan
 	 && FALSE == m_bExecutingKeyMacro	/* キーボードマクロの実行中 */
	){
		/* カーソル直前の単語を取得 */
		if( 0 < GetLeftWord( &cmemData, 100 ) ){
//			MYTRACE( "cmemData=[%s]\n", cmemData.GetPtr( NULL ) );
			/* 補完対象ワードリストを調べる */
			poWin.x = m_nViewAlignLeft
					 + (m_nCaretPosX - m_nViewLeftCol)
					  * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace );
			poWin.y = m_nViewAlignTop
					  + (m_nCaretPosY - m_nViewTopLine)
					   * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			::ClientToScreen( m_hWnd, &poWin );
			poWin.x -= (
				cmemData.GetLength()
				 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace )
			);
			if( 0 < m_pcEditDoc->m_cHokanMgr.Search(
//t				m_hFont_HAN,
				&poWin,
				m_nCharHeight,
				m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace,
				cmemData.GetPtr( NULL ),
//t				(void*)this,
//				m_pShareData->m_Common.m_szHokanFile	// 2001/06/14 asa-o 参照データ変更
				m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
				m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase	// 2001/06/19 asa-o 英大文字小文字を同一視する
			) ){
				m_bHokan = TRUE;
			}else{
				if( m_bHokan ){
					m_pcEditDoc->m_cHokanMgr.Hide();
					m_bHokan = FALSE;
				}
			}
		}else{
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}
	return;
}




/* 2バイト文字入力 */
void CEditView::Command_IME_CHAR( WORD wChar )
{
	if( m_bBeginSelect ){	/* マウスによる範囲選択中 */
		::MessageBeep( MB_ICONHAND );
		return;
	}

	const char*		pLine;
	int				nLineLen;
	int				nIdxTo;
	CMemory			cmemData;
	int				nNewLine;		/* 挿入された部分の次の位置の行 */
	int				nNewPos;		/* 挿入された部分の次の位置のデータ位置 */
	COpe*			pcOpe = NULL;
	POINT			poWin;
	const CLayout*	pcLayout;
	if( 0 == (wChar & 0x00ff) ){
		Command_CHAR( (char)((wChar&0xff00)>>8) );
		return;
	}
	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();	/* 親ウィンドウのタイトルを更新 */

	/* テキストが選択されているか */
	if( IsTextSelected() ){
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			Command_INDENT( (const char*)&wChar, sizeof( wChar ) );
			return;
		}else{
			DeleteData( TRUE );
		}
	}else{
		if( !m_pShareData->m_Common.m_bIsINSMode ){		/* 挿入／上書きモード */
			BOOL bDelete = TRUE;
			if( m_pShareData->m_Common.m_bNotOverWriteCRLF ){	/* 改行は上書きしない */
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
				if( NULL != pLine ){
					/* 指定された桁に対応する行のデータ内の位置を調べる */
					nIdxTo = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );
					if( nIdxTo == nLineLen - 1 && ( pLine[nIdxTo] == CR || pLine[nIdxTo] == LF ) ){
						/* 現在位置が改行ならば削除しない */
						bDelete = FALSE;
					}
				}
			}
			if( bDelete ){
				/* 上書きモードなので、現在位置の文字を１文字消去 */
				DeleteData( FALSE );
			}
		}
	}
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;			/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;			/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
	}
	InsertData_CEditView( m_nCaretPosX, m_nCaretPosY, (char*)&wChar, 2, &nNewLine, &nNewPos, pcOpe, TRUE );

	/* 挿入データの最後へカーソルを移動 */
	MoveCursor( nNewPos, nNewLine, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_After,
//			pcOpe->m_nCaretPosY_After,
//			&pcOpe->m_nCaretPosX_PHY_After,
//			&pcOpe->m_nCaretPosY_PHY_After
//		);
		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}

	/* 入力補完機能を使用する */
	if( m_pShareData->m_Common.m_bUseHokan
 	 && FALSE == m_bExecutingKeyMacro	/* キーボードマクロの実行中 */
	){

		/* カーソル直前の単語を取得 */
		if( 0 < GetLeftWord( &cmemData, 100 ) ){
//			MYTRACE( "cmemData=[%s]\n", cmemData.GetPtr( NULL ) );
			/* 補完対象ワードリストを調べる */
			poWin.x = m_nViewAlignLeft
					 + (m_nCaretPosX - m_nViewLeftCol)
					  * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace );
			poWin.y = m_nViewAlignTop
					 + (m_nCaretPosY - m_nViewTopLine)
					  * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
			::ClientToScreen( m_hWnd, &poWin );
			poWin.x -= (
				cmemData.GetLength()
				 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace )
			);
			if( 0 < m_pcEditDoc->m_cHokanMgr.Search(
//t				m_hFont_HAN,
				&poWin,
				m_nCharHeight,
				m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace,
				cmemData.GetPtr( NULL ),
//t				(void*)this,
//				m_pShareData->m_Common.m_szHokanFile	// 2001/06/14 asa-o 参照データ変更
				m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
				m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase	// 2001/06/19 asa-o 英大文字小文字を同一視する
			) ){
				m_bHokan = TRUE;
			}else{
				if( m_bHokan ){
					m_pcEditDoc->m_cHokanMgr.Hide();
					m_bHokan = FALSE;
				}
			}
		}else{
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}
	return;
}




/* 挿入／上書きモード切り替え */
void CEditView::Command_CHGMOD_INS( void )
{
	/* 挿入モードか？ */
	if( m_pShareData->m_Common.m_bIsINSMode ){
		m_pShareData->m_Common.m_bIsINSMode = FALSE;
	}else{
		m_pShareData->m_Common.m_bIsINSMode = TRUE;
	}
	/* キャレットの表示・更新 */
	ShowEditCaret();
	/* キャレットの行桁位置を表示する */
	DrawCaretPosInfo();
	return;
}




/* 検索(単語検索ダイアログ) */
void CEditView::Command_SEARCH_DIALOG( void )
{
//	int			nRet;
	CMemory		cmemCurText;

	/* 現在カーソル位置単語または選択範囲より検索等のキーを取得 */
	GetCurrentTextForSearch( cmemCurText );

	/* 検索文字列を初期化 */
	strcpy( m_pcEditDoc->m_cDlgFind.m_szText, cmemCurText.GetPtr( NULL ) );

	/* 検索ダイアログの表示 */
	if( NULL == m_pcEditDoc->m_cDlgFind.m_hWnd ){
		m_pcEditDoc->m_cDlgFind.DoModeless( m_hInstance, m_hWnd, (LPARAM)&m_pcEditDoc->m_cEditViewArr[m_pcEditDoc->m_nActivePaneIndex] );
	}else{
		/* アクティブにする */
		ActivateFrameWindow( m_pcEditDoc->m_cDlgFind.m_hWnd );
		::SetDlgItemText( m_pcEditDoc->m_cDlgFind.m_hWnd, IDC_COMBO_TEXT, cmemCurText.GetPtr( NULL ) );
	}
	return;
}




/* 前を検索 */
void CEditView::Command_SEARCH_PREV( BOOL bReDraw, HWND hwndParent )
{
	int			nLineNum;
	const char*	pLine;
	int			nLineLen;
	int			nIdx;
	int			nLineFrom;
	int			nColmFrom;
	int			nLineTo;
	int			nColmTo;
	BOOL		bSelecting;
//	int			nSelectLineBgn_Old;
//	int			nSelectColBgn_Old;
	int			nSelectLineBgnFrom_Old;
	int			nSelectColBgnFrom_Old;
	int			nSelectLineBgnTo_Old;
	int			nSelectColBgnTo_Old;
	int			nSelectLineFrom_Old;
	int			nSelectColFrom_Old;
	int			nSelectLineTo_Old;
	int			nSelectColTo_Old;
	BOOL		bSelectingLock_Old;
//	BOOL		bFlag1;
	BOOL		bFound = FALSE;
	nLineFrom = m_nCaretPosY;
	nColmFrom = m_nCaretPosX;
	nLineTo = m_nCaretPosY;
	nColmTo = m_nCaretPosX;
//	bFlag1 = FALSE;
	bSelecting = FALSE;
//	if( 0 == lstrlen( m_pShareData->m_szSEARCHKEYArr[0] ) ){
	if( '\0' == m_pShareData->m_szSEARCHKEYArr[0][0] ){
		goto end_of_func;
	}
	if( IsTextSelected() ){	/* テキストが選択されているか */
//		nSelectLineBgn_Old = m_nSelectLineBgn;			/* 範囲選択開始行(原点) */
//		nSelectColBgn_Old = m_nSelectColmBgn;			/* 範囲選択開始桁(原点) */
		nSelectLineBgnFrom_Old = m_nSelectLineBgnFrom;	/* 範囲選択開始行(原点) */
		nSelectColBgnFrom_Old = m_nSelectColmBgnFrom;	/* 範囲選択開始桁(原点) */
		nSelectLineBgnTo_Old = m_nSelectLineBgnTo;		/* 範囲選択開始行(原点) */
		nSelectColBgnTo_Old = m_nSelectColmBgnTo;		/* 範囲選択開始桁(原点) */
		nSelectLineFrom_Old = m_nSelectLineFrom;
		nSelectColFrom_Old = m_nSelectColmFrom;
		nSelectLineTo_Old = m_nSelectLineTo;
		nSelectColTo_Old = m_nSelectColmTo;
		bSelectingLock_Old = m_bSelectingLock;
		/* 矩形範囲選択中か */
		if( !m_bBeginBoxSelect && TRUE == m_bSelectingLock ){	/* 選択状態のロック */
//			if( ( m_nSelectLineBgn <  m_nCaretPosY ) ||
//				( m_nSelectLineBgn == m_nCaretPosY && m_nSelectColmBgn < m_nCaretPosX )
//			){
//				bFlag1 = TRUE;
//			}
			bSelecting = TRUE;
//			bSelectingLock_Old = m_bSelectingLock;
//			/* 現在の選択範囲を非選択状態に戻す */
//			DisableSelectArea( bReDraw );

		}else{
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( bReDraw );
		}
	}
	nLineNum = m_nCaretPosY;
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
	if( NULL == pLine ){
		nLineNum--;
		if( nLineNum < 0 ){
			goto end_of_func;
		}
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
		if( NULL == pLine ){
			goto end_of_func;
		}
		/* カーソル左移動 */
		Command_LEFT( FALSE, FALSE );
	}
	/* 指定された桁に対応する行のデータ内の位置を調べる */
	nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );

	BOOL	bChangeState;
	if( FALSE == m_bCurSrchKeyMark
	 || 0 != strcmp( m_szCurSrchKey, m_pShareData->m_szSEARCHKEYArr[0] )
	 || m_bCurSrchRegularExp != m_pShareData->m_Common.m_bRegularExp
	 || m_bCurSrchLoHiCase != m_pShareData->m_Common.m_bLoHiCase
	 || m_bCurSrchWordOnly != m_pShareData->m_Common.m_bWordOnly
	){
		bChangeState = TRUE;
	}else{
		bChangeState = FALSE;
	}

	m_bCurSrchKeyMark = TRUE;									/* 検索文字列のマーク */
	strcpy( m_szCurSrchKey, m_pShareData->m_szSEARCHKEYArr[0] );/* 検索文字列 */
	m_bCurSrchRegularExp = m_pShareData->m_Common.m_bRegularExp;/* 検索／置換  1==正規表現 */
	m_bCurSrchLoHiCase = m_pShareData->m_Common.m_bLoHiCase;	/* 検索／置換  1==大文字小文字の区別 */
	m_bCurSrchWordOnly = m_pShareData->m_Common.m_bWordOnly;	/* 検索／置換  1==単語のみ検索 */
	/* 正規表現 */
	if( m_bCurSrchRegularExp
	 && bChangeState
	){
		//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
		if( !InitRegexp( m_hWnd, m_CurRegexp, true ) ){
			return;
		}
		/* 検索パターンのコンパイル */
		m_CurRegexp.Compile( m_szCurSrchKey );
	}

	if( bChangeState ){
		/* フォーカス移動時の再描画 */
		RedrawAll();
	}


	/* 現在位置より前の位置を検索する */
	if( m_pcEditDoc->m_cLayoutMgr.SearchWord(
		nLineNum, 								/* 検索開始行 */
		nIdx, 									/* 検索開始位置 */
		m_pShareData->m_szSEARCHKEYArr[0],		/* 検索条件 */
		FALSE,									/* 0==前方検索 1==後方検索 */
		m_pShareData->m_Common.m_bRegularExp,	/* 1==正規表現 */
		m_pShareData->m_Common.m_bLoHiCase,		/* 1==大文字小文字の区別 */
		m_pShareData->m_Common.m_bWordOnly,		/* 1==単語のみ検索 */
		&nLineFrom,								/* マッチレイアウト行from */
		&nColmFrom, 							/* マッチレイアウト位置from */
		&nLineTo, 								/* マッチレイアウト行to */
		&nColmTo, 								/* マッチレイアウト位置to */
		&m_CurRegexp							/* 正規表現コンパイルデータ */
	) ){
		/* フォーカス移動時の再描画 */
		RedrawAll();

		/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineFrom, &nLineLen );
//		nColmFrom = LineIndexToColmn( pLine, nLineLen, nColmFrom );
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineTo, &nLineLen );
//		nColmTo = LineIndexToColmn( pLine, nLineLen, nColmTo );

//		MYTRACE( "bSelecting=%d\n", bSelecting );
		if( bSelecting ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nColmFrom, nLineFrom );
			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */
		}else{
			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nLineFrom;		/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nColmFrom;		/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nLineFrom;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nColmFrom;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nLineFrom;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nColmFrom;		/* 範囲選択開始桁(原点) */

			m_nSelectLineFrom =	nLineFrom;
			m_nSelectColmFrom = nColmFrom;
			m_nSelectLineTo = nLineTo;
			m_nSelectColmTo = nColmTo;

			if( bReDraw ){
				/* 選択領域描画 */
				DrawSelectArea();
			}
		}
		/* カーソル移動 */
		//	Sep. 8, 2000 genta
		AddCurrentLineToHistory();
		MoveCursor( nColmFrom, nLineFrom, bReDraw );
		m_nCaretPosX_Prev = m_nCaretPosX;
#if 0
		if( m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_UNDERLINE].m_bDisp && -1 != m_nOldUnderLineY ){
			/* カーソル行アンダーラインの消去 */
			HPEN	hPen;
			HPEN	hPenOld;
			HDC		hdc;
			hdc = ::GetDC( m_hWnd );
			hPen = ::CreatePen( PS_SOLID, 0, m_pcEditDoc->GetDocumentAttribute().m_ColorInfoArr[COLORIDX_TEXT].m_colBACK );
			hPenOld = (HPEN)::SelectObject( hdc, hPen );
			::MoveToEx(
				hdc,
				m_nViewAlignLeft,
				m_nOldUnderLineY,
				NULL
			);
			::LineTo(
				hdc,
				m_nViewCx + m_nViewAlignLeft,
				m_nOldUnderLineY
			);
			::SelectObject( hdc, hPenOld );
			::DeleteObject( hPen );
			::ReleaseDC( m_hWnd, hdc );
			m_nOldUnderLineY = -1;
		}
		if( bSelecting ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( int nCaretPosX, int nCaretPosY )

			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */
			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nSelectLineBgn_Old;			/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nSelectColBgn_Old;			/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nSelectLineBgnFrom_Old;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nSelectColBgnFrom_Old;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nSelectLineBgnTo_Old;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nSelectColBgnTo_Old;		/* 範囲選択開始桁(原点) */

			m_nSelectLineFrom =	nLineFrom;
			m_nSelectColmFrom = nColmFrom;
			m_nSelectLineTo = nLineTo;
			m_nSelectColmTo = nColmTo;
			if( ( m_nSelectLineFrom > m_nSelectLineBgn ) ||
				( m_nSelectLineFrom == m_nSelectLineBgn && m_nSelectColmFrom > m_nSelectColmBgn ) ){
				m_nSelectLineFrom = m_nSelectLineBgn;
				m_nSelectColmFrom = m_nSelectColmBgn;
			}
			if( ( m_nSelectLineTo < m_nSelectLineBgn ) ||
				( m_nSelectLineTo == m_nSelectLineBgn && m_nSelectColmTo < m_nSelectColmBgn )
			){
				m_nSelectLineTo = m_nSelectLineBgn;
				m_nSelectColmTo = m_nSelectColmBgn;
			}
		}else{
			/* 選択範囲の変更 */
			m_nSelectLineBgn = nLineFrom;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgn = nColmFrom;	/* 範囲選択開始桁(原点) */
			m_nSelectLineFrom =	nLineFrom;
			m_nSelectColmFrom = nColmFrom;
			m_nSelectLineTo = nLineTo;
			m_nSelectColmTo = nColmTo;
		}
		/* 選択領域描画 */
		DrawSelectArea();
#endif
		bFound = TRUE;
	}else{
		/* フォーカス移動時の再描画 */
		RedrawAll();
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( bReDraw );
		}
		if( bSelecting ){
			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */
			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nSelectLineBgn_Old;			/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nSelectColBgn_Old;			/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nSelectLineBgnFrom_Old;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nSelectColBgnFrom_Old;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nSelectLineBgnTo_Old;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nSelectColBgnTo_Old;		/* 範囲選択開始桁(原点) */

			m_nSelectLineFrom =	nSelectLineFrom_Old;
			m_nSelectColmFrom = nSelectColFrom_Old;
			m_nSelectLineTo = nSelectLineTo_Old;
			m_nSelectColmTo = nSelectColTo_Old;
			/* カーソル移動 */
			MoveCursor( nColmFrom, nLineFrom, bReDraw );
			m_nCaretPosX_Prev = m_nCaretPosX;
			/* 選択領域描画 */
			DrawSelectArea();
		}
	}
end_of_func:;
	if( FALSE == bFound ){
		::MessageBeep( MB_ICONHAND );
		if( bReDraw	&&
			m_pShareData->m_Common.m_bNOTIFYNOTFOUND 	/* 検索／置換  見つからないときメッセージを表示 */
		){
			if( NULL == hwndParent ){
				hwndParent = m_hWnd;
			}
			::MYMESSAGEBOX( hwndParent,	MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
//				"↑ 前方に、文字列 '%s' が見つかりません。",
				"前方(↑) に文字列 '%s' が１つも見つかりません。",	//Jan. 25, 2001 jepro メッセージを若干変更
				m_pShareData->m_szSEARCHKEYArr[0]
			);
		}
	}
	return;
}




/* 次を検索 */
void CEditView::Command_SEARCH_NEXT( BOOL bRedraw, HWND hwndParent, const char* pszNotFoundMessage )
{

//#ifdef _DEBUG
//	gm_ProfileOutput = 1;
//	CRunningTimer*  pCRunningTimer = new CRunningTimer( (const char*)"CEditView::Command_SEARCH_NEXT()" );
//#endif
	int			nLineNum;
	const char*	pLine;
	int			nLineLen;
	int			nIdx;
	int			nLineFrom;
	int			nColmFrom;
	int			nLineTo;
	int			nColmTo;
	BOOL		bSelecting;
//	int			nSelectLineBgn_Old;
//	int			nSelectColBgn_Old;
	int			nSelectLineBgnFrom_Old;
	int			nSelectColBgnFrom_Old;
	int			nSelectLineBgnTo_Old;
	int			nSelectColBgnTo_Old;
	int			nSelectLineFrom_Old;
	int			nSelectColFrom_Old;
	int			nSelectLineTo_Old;
	int			nSelectColTo_Old;
//	int			nSelectLineFrom;
//	int			nSelectColmFrom;
//	int			nSelectLineTo;
//	int			nSelectColmTo;
	BOOL		bFlag1;
	BOOL		bSelectingLock_Old;
	BOOL		bFound = FALSE;

	nLineFrom = m_nCaretPosY;
	nColmFrom = m_nCaretPosX;
	nLineTo = m_nCaretPosY;
	nColmTo = m_nCaretPosX;

	bSelecting = FALSE;
//	if( 0 == lstrlen( m_pShareData->m_szSEARCHKEYArr[0] ) ){
	if( '\0' == m_pShareData->m_szSEARCHKEYArr[0][0] ){
		goto end_of_func;
	}

	bFlag1 = FALSE;
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 矩形範囲選択中でない & 選択状態のロック */
		if( !m_bBeginBoxSelect && TRUE == m_bSelectingLock ){
			bSelecting = TRUE;
			bSelectingLock_Old = m_bSelectingLock;
//			nSelectLineBgn_Old = m_nSelectLineBgn;			/* 範囲選択開始行(原点) */
//			nSelectColBgn_Old = m_nSelectColmBgn;			/* 範囲選択開始桁(原点) */
			nSelectLineBgnFrom_Old = m_nSelectLineBgnFrom;	/* 範囲選択開始行(原点) */
			nSelectColBgnFrom_Old = m_nSelectColmBgnFrom;	/* 範囲選択開始桁(原点) */
			nSelectLineBgnTo_Old = m_nSelectLineBgnTo;		/* 範囲選択開始行(原点) */
			nSelectColBgnTo_Old = m_nSelectColmBgnTo;		/* 範囲選択開始桁(原点) */
			nSelectLineFrom_Old = m_nSelectLineFrom;
			nSelectColFrom_Old = m_nSelectColmFrom;
			nSelectLineTo_Old = m_nSelectLineTo;
			nSelectColTo_Old = m_nSelectColmTo;

			if( ( m_nSelectLineBgnFrom >  m_nCaretPosY ) ||
				( m_nSelectLineBgnFrom == m_nCaretPosY && m_nSelectColmBgnFrom >= m_nCaretPosX )
			){
				/* カーソル移動 */
				MoveCursor( m_nSelectColmFrom, m_nSelectLineFrom, bRedraw );
				bFlag1 = TRUE;
			}else{
				/* カーソル移動 */
				MoveCursor( m_nSelectColmTo, m_nSelectLineTo, bRedraw );
			}

//			/* 現在の選択範囲を非選択状態に戻す */
//			DisableSelectArea( bRedraw );
		}else{
			/* カーソル移動 */
			MoveCursor( m_nSelectColmTo, m_nSelectLineTo, bRedraw );

			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( bRedraw );
		}
	}
	nLineNum = m_nCaretPosY;
	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
	if( NULL == pLine ){
		goto end_of_func;
	}
	/* 指定された桁に対応する行のデータ内の位置を調べる */
	nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );


	BOOL	bChangeState;
	if( FALSE == m_bCurSrchKeyMark
	 || 0 != strcmp( m_szCurSrchKey, m_pShareData->m_szSEARCHKEYArr[0] )
	 || m_bCurSrchRegularExp != m_pShareData->m_Common.m_bRegularExp
	 || m_bCurSrchLoHiCase != m_pShareData->m_Common.m_bLoHiCase
	 || m_bCurSrchWordOnly != m_pShareData->m_Common.m_bWordOnly
	){
		bChangeState = TRUE;
	}else{
		bChangeState = FALSE;
	}

	m_bCurSrchKeyMark = TRUE;									/* 検索文字列のマーク */
	strcpy( m_szCurSrchKey, m_pShareData->m_szSEARCHKEYArr[0] );/* 検索文字列 */
	m_bCurSrchRegularExp = m_pShareData->m_Common.m_bRegularExp;/* 検索／置換  1==正規表現 */
	m_bCurSrchLoHiCase = m_pShareData->m_Common.m_bLoHiCase;	/* 検索／置換  1==大文字小文字の区別 */
	m_bCurSrchWordOnly = m_pShareData->m_Common.m_bWordOnly;	/* 検索／置換  1==単語のみ検索 */
	/* 正規表現 */
	if( m_bCurSrchRegularExp
	 && bChangeState
	){
		//	Jun. 27, 2001 genta	正規表現ライブラリの差し替え
		if( !InitRegexp( m_hWnd, m_CurRegexp, true ) ){
			return;
		}
		/* 検索パターンのコンパイル */
		m_CurRegexp.Compile( m_szCurSrchKey );
	}
	if( bChangeState ){
		/* フォーカス移動時の再描画 */
		RedrawAll();
	}


re_do:;
	 /* 現在位置より後ろの位置を検索する */
	if( m_pcEditDoc->m_cLayoutMgr.SearchWord(
		nLineNum, 								/* 検索開始行 */
		nIdx, 									/* 検索開始位置 */
		m_pShareData->m_szSEARCHKEYArr[0],		/* 検索条件 */
		TRUE,									/* 0==前方検索 1==後方検索 */
		m_pShareData->m_Common.m_bRegularExp,	/* 1==正規表現 */
		m_pShareData->m_Common.m_bLoHiCase,		/* 1==英大文字小文字の区別 */
		m_pShareData->m_Common.m_bWordOnly,		/* 1==単語のみ検索 */
		&nLineFrom,								/* マッチレイアウト行from */
		&nColmFrom, 							/* マッチレイアウト位置from */
		&nLineTo, 								/* マッチレイアウト行to */
		&nColmTo, 								/* マッチレイアウト位置to */
		&m_CurRegexp							/* 正規表現コンパイルデータ */
	) ){

//		/* フォーカス移動時の再描画 */
//		RedrawAll();

		/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineFrom, &nLineLen );
//		nColmFrom = LineIndexToColmn( pLine, nLineLen, nColmFrom );

		if( bFlag1 && m_nCaretPosX == nColmFrom && m_nCaretPosY == nLineFrom ){
			nLineNum = nLineTo;
			nIdx = nColmTo;
			goto re_do;
		}
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineTo, &nLineLen );
//		nColmTo = LineIndexToColmn( pLine, nLineLen, nColmTo );

		if( bSelecting ){
			/* 現在のカーソル位置によって選択範囲を変更 */
			ChangeSelectAreaByCurrentCursor( nColmTo, nLineTo );
//			ChangeSelectAreaByCurrentCursor( nColmFrom, nLineFrom );
			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */
		}else{
			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nLineFrom;		/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nColmFrom;		/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nLineFrom;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nColmFrom;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nLineFrom;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nColmFrom;		/* 範囲選択開始桁(原点) */
			m_nSelectLineFrom =	nLineFrom;
			m_nSelectColmFrom = nColmFrom;
			m_nSelectLineTo = nLineTo;
			m_nSelectColmTo = nColmTo;

			if( bRedraw ){
				/* 選択領域描画 */
				DrawSelectArea();
			}
//			if( IsTextSelected() ){	/* テキストが選択されているか */
//				/* 現在の選択範囲を非選択状態に戻す */
//				DisableSelectArea( bRedraw );
//			}
		}

		/* カーソル移動 */
		//	Sep. 8, 2000 genta
		AddCurrentLineToHistory();
		MoveCursor( nColmFrom, nLineFrom, bRedraw );
		m_nCaretPosX_Prev = m_nCaretPosX;
#if 0
		if( bSelecting ){
			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */
			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nSelectLineBgn_Old;			/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nSelectColBgn_Old;			/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nSelectLineBgnFrom_Old;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nSelectColBgnFrom_Old;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nSelectLineBgnTo_Old;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nSelectColBgnTo_Old;		/* 範囲選択開始桁(原点) */
			m_nSelectLineFrom = nSelectLineFrom_Old;
			m_nSelectColmFrom = nSelectColFrom_Old;
			m_nSelectLineTo = nSelectLineTo_Old;
			m_nSelectColmTo = nSelectColTo_Old;

			if( ( nSelectLineFrom_Old <  nLineFrom ) ||
				( nSelectLineFrom_Old == nLineFrom && nSelectColFrom_Old < nColmFrom ) ){
				if( bFlag1 ){
					m_nSelectLineFrom = nLineFrom;
					m_nSelectColmFrom = nColmFrom;
				}
			}
			if( ( nSelectLineTo_Old <  nLineTo ) ||
				( nSelectLineTo_Old == nLineTo && nSelectColTo_Old < nColmTo ) ){
				m_nSelectLineTo = nLineTo;
				m_nSelectColmTo = nColmTo;
			}
			if( ( m_nSelectLineFrom >  m_nSelectLineBgn ) ||
				( m_nSelectLineFrom == m_nSelectLineBgn && m_nSelectColmFrom > m_nSelectColmBgn ) ){
				m_nSelectLineFrom = m_nSelectLineBgn;
				m_nSelectColmFrom = m_nSelectColmBgn;
			}
			if( ( m_nSelectLineTo < m_nSelectLineBgn ) ||
				( m_nSelectLineTo == m_nSelectLineBgn && m_nSelectColmTo < m_nSelectColmBgn )
			){
				m_nSelectLineBgn = m_nSelectLineTo;
				m_nSelectColmBgn = m_nSelectColmTo;
			}
		}else{
			/* 選択範囲の変更 */
			m_nSelectLineBgn = nLineFrom;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgn = nColmFrom;	/* 範囲選択開始桁(原点) */
			m_nSelectLineFrom = nLineFrom;
			m_nSelectColmFrom = nColmFrom;
			m_nSelectLineTo = nLineTo;
			m_nSelectColmTo = nColmTo;
		}
		if( bRedraw ){
			/* 選択領域描画 */
			DrawSelectArea();
		}
#endif
		bFound = TRUE;
	}else{
//		/* フォーカス移動時の再描画 */
//		RedrawAll();

		if( bSelecting ){
			m_bSelectingLock = bSelectingLock_Old;	/* 選択状態のロック */

			/* 選択範囲の変更 */
//			m_nSelectLineBgn = nSelectLineBgn_Old;			/* 範囲選択開始行(原点) */
//			m_nSelectColmBgn = nSelectColBgn_Old;			/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnFrom = nSelectLineBgnFrom_Old;	/* 範囲選択開始行(原点) */
			m_nSelectColmBgnFrom = nSelectColBgnFrom_Old;	/* 範囲選択開始桁(原点) */
			m_nSelectLineBgnTo = nSelectLineBgnTo_Old;		/* 範囲選択開始行(原点) */
			m_nSelectColmBgnTo = nSelectColBgnTo_Old;		/* 範囲選択開始桁(原点) */
			m_nSelectLineFrom =	nSelectLineFrom_Old;
			m_nSelectColmFrom = nSelectColFrom_Old;
//			m_nSelectLineTo = nSelectLineTo_Old;
//			m_nSelectColmTo = nSelectColTo_Old;
			m_nSelectLineTo = nLineFrom;
			m_nSelectColmTo = nColmFrom;

			/* カーソル移動 */
			MoveCursor( nColmFrom, nLineFrom, bRedraw );
			m_nCaretPosX_Prev = m_nCaretPosX;

			if( bRedraw ){
				/* 選択領域描画 */
				DrawSelectArea();
			}
		}
	}

//	/* カーソル移動 */
//	MoveCursor( nColmFrom, nLineFrom, bRedraw );
//	m_nCaretPosX_Prev = m_nCaretPosX;

end_of_func:;
	if( FALSE == bFound ){
		::MessageBeep( MB_ICONHAND );
		if( bRedraw	&&
			m_pShareData->m_Common.m_bNOTIFYNOTFOUND	/* 検索／置換  見つからないときメッセージを表示 */
		){
			if( NULL == hwndParent ){
				hwndParent = m_hWnd;
			}
			if( NULL == pszNotFoundMessage ){
				::MYMESSAGEBOX( hwndParent,	MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
//					"↓ 後方に、文字列 '%s' が見つかりません。",
					"後方(↓) に文字列 '%s' が１つも見つかりません。",	//Jan. 25, 2001 jepro メッセージを若干変更
					m_pShareData->m_szSEARCHKEYArr[0]
				);
			}else{
				::MYMESSAGEBOX( hwndParent, MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
					pszNotFoundMessage
				);
			}
		}
	}
//#ifdef _DEBUG
//	delete pCRunningTimer;
//	pCRunningTimer = NULL;
//
//	gm_ProfileOutput = 0;
//#endif
	return;
}




/* 各種モードの取り消し */
void CEditView::Command_CANCEL_MODE( void )
{
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
	}
	m_bSelectingLock = FALSE;	/* 選択状態のロック */
	return;
}




/* 範囲選択開始 */
void CEditView::Command_BEGIN_SELECT( void )
{
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在のカーソル位置から選択を開始する */
		BeginSelectArea();
	}
	m_bSelectingLock = TRUE;	/* 選択状態のロック */
	return;
}




/* 矩形範囲選択開始 */
void CEditView::Command_BEGIN_BOXSELECT( void )
{
	if( FALSE == m_pShareData->m_Common.m_bFontIs_FIXED_PITCH ){	/* 現在のフォントは固定幅フォントである */
		return;
	}

	if( !IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在のカーソル位置から選択を開始する */
		BeginSelectArea();
	}
	m_bSelectingLock = TRUE;	/* 選択状態のロック */
	m_bBeginBoxSelect = TRUE;	/* 矩形範囲選択中 */
	return;
}




/* 新規作成 */
void CEditView::Command_FILENEW( void )
{
	/* 新たな編集ウィンドウを起動 */
	CEditApp::OpenNewEditor( m_hInstance, m_hWnd, (char*)NULL, 0, FALSE );
	return;
}




/* ファイルを開く */
void CEditView::Command_FILEOPEN( void )
{
	char*		pszPath = new char[_MAX_PATH];
	BOOL		bOpened;
	int			nCharCode;
	BOOL		bReadOnly;
	FileInfo*	pfi;
	HWND		hWndOwner;

	strcpy( pszPath, "" );

	/* 「ファイルを開く」ダイアログ */
	nCharCode = CODE_AUTODETECT;	/* 文字コード自動判別 */
	bReadOnly = FALSE;
	if( !m_pcEditDoc->OpenFileDialog( m_hWnd, NULL, pszPath, &nCharCode, &bReadOnly ) ){
		return;
	}
	/* 指定ファイルが開かれているか調べる */
	if( m_cShareData.IsPathOpened( pszPath, &hWndOwner ) ){
		::SendMessage( hWndOwner, MYWM_GETFILEINFO, 0, 0 );
//		pfi = (FileInfo*)m_pShareData->m_szWork;
		pfi = (FileInfo*)&m_pShareData->m_FileInfo_MYWM_GETFILEINFO;

		int		nCharCodeNew;
		if( CODE_AUTODETECT == nCharCode ){	/* 文字コード自動判別 */
			/*
			|| ファイルの日本語コードセット判別
			||
			|| 【戻り値】
			||	SJIS	0
			||	JIS		1
			||	EUC		2
			||	Unicode	3
			||	エラー	-1
			*/
			nCharCodeNew = CMemory::CheckKanjiCodeOfFile( pszPath );
			if( -1 == nCharCodeNew ){

			}else{
				nCharCode = nCharCodeNew;
			}
		}
		if( nCharCode != pfi->m_nCharCode ){	/* 文字コード種別 */
			char*	pszCodeNameCur;
			char*	pszCodeNameNew;
			switch( pfi->m_nCharCode ){
			case CODE_SJIS:		/* SJIS */		pszCodeNameCur = "SJIS";break;	//	Sept. 27, 2000 jepro 'シフト'を'S'に変更
			case CODE_JIS:		/* JIS */		pszCodeNameCur = "JIS";break;
			case CODE_EUC:		/* EUC */		pszCodeNameCur = "EUC";break;
			case CODE_UNICODE:	/* Unicode */	pszCodeNameCur = "Unicode";break;
			case CODE_UTF8:	/* UTF-8 */			pszCodeNameCur = "UTF-8";break;
			case CODE_UTF7:	/* UTF-7 */			pszCodeNameCur = "UTF-7";break;
			}
			switch( nCharCode ){
			case CODE_SJIS:		/* SJIS */		pszCodeNameNew = "SJIS";break;	//	Sept. 27, 2000 jepro 'シフト'を'S'に変更
			case CODE_JIS:		/* JIS */		pszCodeNameNew = "JIS";break;
			case CODE_EUC:		/* EUC */		pszCodeNameNew = "EUC";break;
			case CODE_UNICODE:	/* Unicode */	pszCodeNameNew = "Unicode";break;
			case CODE_UTF8:	/* UTF-8 */			pszCodeNameNew = "UTF-8";break;
			case CODE_UTF7:	/* UTF-7 */			pszCodeNameNew = "UTF-7";break;
			}
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST, GSTR_APPNAME,
				"%s\n\n\n既に開いているファイルを違う文字コードで開く場合は、\n一旦閉じてから開いてください。\n\n現在の文字コードセット=[%s]\n新しい文字コードセット=[%s]",
				pszPath, pszCodeNameCur, pszCodeNameNew
			);
		}
		/* 自分が開いているか */
		if( 0 == strcmp( m_pcEditDoc->m_szFilePath, pszPath ) ){
			/* 何もしない */
		}else{
			/* 開いているウィンドウをアクティブにする */
			/* アクティブにする */
			ActivateFrameWindow( hWndOwner );
		}
	}else{
		/* ファイルが開かれていない */
		/* 変更フラグがオフで、ファイルを読み込んでいない場合 */
		if( !m_pcEditDoc->m_bIsModified &&
//			0 == lstrlen( m_pcEditDoc->m_szFilePath )	/* 現在編集中のファイルのパス */
			'\0' == m_pcEditDoc->m_szFilePath[0]		/* 現在編集中のファイルのパス */
		){
			/* ファイル読み込み */
			m_pcEditDoc->FileRead( pszPath, &bOpened, nCharCode, bReadOnly, TRUE );
		}else{
			if( strchr( pszPath, ' ' ) ){
				char	szFile2[_MAX_PATH + 3];
				wsprintf( szFile2, "\"%s\"", pszPath );
				strcpy( pszPath, szFile2 );
			}
			/* 新たな編集ウィンドウを起動 */
			CEditApp::OpenNewEditor( m_hInstance, m_hWnd, pszPath, nCharCode, bReadOnly );
		}
	}
	delete [] pszPath;
	return;
}




/* 閉じて(無題) */	//Oct. 17, 2000 jepro 「ファイルを閉じる」というキャプションを変更
void CEditView::Command_FILECLOSE( void )
{
	/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
	if( !m_pcEditDoc->OnFileClose() ){
		return;
	}
	/* 既存データのクリア */
	m_pcEditDoc->Init();

	/* 全ビューの初期化 */
	m_pcEditDoc->InitAllView();

	/* 親ウィンドウのタイトルを更新 */
	SetParentCaption();

	return;
}




/* 閉じて開く */
void CEditView::Command_FILECLOSE_OPEN( void )
{
	/* ファイルを閉じるときのMRU登録 & 保存確認 & 保存実行 */
	if( !m_pcEditDoc->OnFileClose() ){
		return;
	}
	/* 既存データのクリア */
	m_pcEditDoc->Init();

	/* 全ビューの初期化 */
	m_pcEditDoc->InitAllView();

	/* 親ウィンドウのタイトルを更新 */
	SetParentCaption();

	/* ファイルを開く */
	Command_FILEOPEN();

	return;
}




/* 上書き保存 */
BOOL CEditView::Command_FILESAVE( void )
{

	/* 無変更でも上書きするか */
	if( FALSE == m_pShareData->m_Common.m_bEnableUnmodifiedOverwrite
	 && FALSE == m_pcEditDoc->m_bIsModified	/* 変更フラグ */
	 ){
		::MessageBeep( MB_ICONHAND );
		return TRUE;
	}

	if( m_pcEditDoc->SaveFile( false ) ){
		/* キャレットの行桁位置を表示する */
		DrawCaretPosInfo();
		return TRUE;
	}
	return FALSE;
}




/* 名前を付けて保存 */
BOOL CEditView::Command_FILESAVEAS( void )
{
	if( m_pcEditDoc->SaveFile( true ) ){
		/* キャレットの行桁位置を表示する */
		DrawCaretPosInfo();
		return TRUE;
	}
	return FALSE;
}




/* 現在編集中のファイルのパス名をクリップボードにコピー */
void CEditView::Command_COPYPATH( void )
{
//	HGLOBAL		hgClip;
//	char*		pszClip;
//	if( 0 < lstrlen( m_pcEditDoc->m_szFilePath ) ){
	if( '\0' != m_pcEditDoc->m_szFilePath[0] ){
		/* クリップボードにデータを設定 */
		MySetClipboardData( m_pcEditDoc->m_szFilePath, lstrlen( m_pcEditDoc->m_szFilePath ), FALSE );

//		/* Windowsクリップボードにコピー */
//		hgClip = ::GlobalAlloc(
//			GMEM_MOVEABLE | GMEM_DDESHARE,
//			lstrlen( m_pcEditDoc->m_szFilePath ) + 1
//		);
//		pszClip = (char*)::GlobalLock( hgClip );
//		strcpy( pszClip, (char*)m_pcEditDoc->m_szFilePath );
//		::GlobalUnlock( hgClip );
//		::OpenClipboard( m_hWnd );
//		::EmptyClipboard();
//		::SetClipboardData( CF_OEMTEXT, hgClip );
//		::CloseClipboard();
	}else{
		::MessageBeep( MB_ICONHAND );
	}
	return;

}




//	May 9, 2000 genta
/* 現在編集中のファイルのパス名とカーソル位置をクリップボードにコピー */
void CEditView::Command_COPYTAG( void )
{
	if( '\0' != m_pcEditDoc->m_szFilePath[0] ){
		char	buf[ MAX_PATH + 20 ];
		int		line, col;

		//	論理行番号を得る
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys( m_nCaretPosX, m_nCaretPosY, &col, &line );

		/* クリップボードにデータを設定 */
		wsprintf( buf, "%s (%d,%d): ", m_pcEditDoc->m_szFilePath, line+1, col+1 );
		MySetClipboardData( buf, lstrlen( buf ), FALSE );
	}else{
		::MessageBeep( MB_ICONHAND );
	}
	return;

}




/* 指定行ヘジャンプ */
void CEditView::Command_JUMP( void )
{
	const char*	pLine;
	int			nLineLen;
	int			nLineCount;
	int			i;
	int			nMode;
	int			nLineNum;
	int			bValidLine;
	int			nCurrentLine;
	int			nCommentBegin;
	int			nBgn;
//	int			nCharChars;
//	m_pcEditDoc->m_cDlgJump.Create( m_hInstance, m_hWnd, (void *)m_pcEditDoc );
	if( !m_pcEditDoc->m_cDlgJump.DoModal(
		m_hInstance, m_hWnd, (LPARAM)m_pcEditDoc,
		m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
//		&m_pcEditDoc->m_hwndActiveDialog						/* アクティブな子ダイアログ */
	) ){
//		::MessageBeep( MB_ICONHAND );	//Feb. 20, 2001 JEPRO [キャンセル]時に鳴る警告音の正体はこれ(コメントアウトにした)
		return;
	}
	if( 0 == m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 行番号 */
	nLineNum = m_pcEditDoc->m_cDlgJump.m_nLineNum;
	if( !m_pcEditDoc->m_cDlgJump.m_bPLSQL ){	/* PL/SQLソースの有効行か */
		/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
		if( m_pcEditDoc->m_cDlgJump.m_bLineNumIsCRLF ){
			if( 0 >= nLineNum ){
				nLineNum = 1;
			}
			/*
			  カーソル位置変換
			  物理位置(行頭からのバイト数、折り返し無し行位置)
			  →
			  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
			*/
			int		nPosX;
			int		nPosY;
			m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log(
				0,
				nLineNum - 1,
				&nPosX,
				&nPosY
			);
			nLineNum = nPosY + 1;
		}else{
			if( 0 >= nLineNum ){
				nLineNum = 1;
			}
			if( nLineNum > m_pcEditDoc->m_cLayoutMgr.GetLineCount() ){
				nLineNum = m_pcEditDoc->m_cLayoutMgr.GetLineCount();
			}
		}
		/* 範囲選択中か */
		if( IsTextSelected() ){	/* テキストが選択されているか */
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
		}
		/* カーソルを選択開始位置に移動 */
		//	Sep. 8, 2000 genta
		AddCurrentLineToHistory();
		MoveCursor( 0, nLineNum - 1, TRUE, _CARETMARGINRATE / 3 );
		m_nCaretPosX_Prev = m_nCaretPosX;
		return;
	}
	if( 0 >= nLineNum ){
		nLineNum = 1;
	}
	nMode = 0;
	nCurrentLine = m_pcEditDoc->m_cDlgJump.m_nPLSQL_E2 - 1;
	nLineCount = m_pcEditDoc->m_cDlgJump.m_nPLSQL_E1 - 1;
	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
	if( FALSE == m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF ){
		/*
		  カーソル位置変換
		  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
		  →
		  物理位置(行頭からのバイト数、折り返し無し行位置)
		*/
		int nPosX,nPosY;
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
			0,
			nLineCount,
			(int*)&nPosX,
			(int*)&nPosY
		);
		nLineCount = nPosY;
	}
//	for( ; nLineCount <  m_pcEditDoc->m_cLayoutMgr.GetLineCount(); ++nLineCount ){
	for( ; nLineCount <  m_pcEditDoc->m_cDocLineMgr.GetLineCount(); ++nLineCount ){
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineCount, &nLineLen );
		pLine = m_pcEditDoc->m_cDocLineMgr.GetLineStr( nLineCount, &nLineLen );
		bValidLine = FALSE;
		nBgn = 0;
		for( i = 0; i < nLineLen; ++i ){
			if( ' ' != pLine[i] &&
				TAB != pLine[i]
			){
				break;
			}
		}
		nBgn = i;
		for( i = nBgn; i < nLineLen; ++i ){
			/* シングルクォーテーション文字列読み込み中 */
			if( 20 == nMode ){
				bValidLine = TRUE;
				if( '\'' == pLine[i] ){
					if( i > 0 && '\\' == pLine[i - 1] ){
					}else{
						nMode = 0;
						continue;
					}
				}else{
				}
			}else
			/* ダブルクォーテーション文字列読み込み中 */
			if( 21 == nMode ){
				bValidLine = TRUE;
				if( '"' == pLine[i] ){
					if( i > 0 && '\\' == pLine[i - 1] ){
					}else{
						nMode = 0;
						continue;
					}
				}else{
				}
			}else
			/* コメント読み込み中 */
			if( 8 == nMode ){
				if( i < nLineLen - 1 && '*' == pLine[i] &&  '/' == pLine[i + 1] ){
					if( /*nCommentBegin != nLineCount &&*/ nCommentBegin != 0){
						bValidLine = TRUE;
					}
					++i;
					nMode = 0;
					continue;
				}else{
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
					bValidLine = TRUE;
					break;
				}else
				if( i < nLineLen - 1 && '/' == pLine[i] &&  '*' == pLine[i + 1] ){
					++i;
					nMode = 8;
					nCommentBegin = nLineCount;
					continue;
				}else
				if( '\'' == pLine[i] ){
					nMode = 20;
					continue;
				}else
				if( '"' == pLine[i] ){
					nMode = 21;
					continue;
				}else{
					bValidLine = TRUE;
				}
			}
		}
		/* コメント読み込み中 */
		if( 8 == nMode ){
			if( nCommentBegin != 0){
				bValidLine = TRUE;
			}
			/* コメントブロック内の改行だけの行 */
			if( CR == pLine[nBgn] ||
				LF == pLine[nBgn] ){
				bValidLine = FALSE;
			}
		}
		if( bValidLine ){
			++nCurrentLine;
			if( nCurrentLine >= nLineNum ){
				break;
			}
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
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log(
		0,
		nLineCount,
		&nPosX,
		&nPosY
	);
	/* 範囲選択中か */
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
	}
	/* カーソルを選択開始位置に移動 */
	//	Sep. 8, 2000 genta
	AddCurrentLineToHistory();
	MoveCursor( nPosX, nPosY, TRUE, _CARETMARGINRATE / 3 );
	m_nCaretPosX_Prev = m_nCaretPosX;
	return;
}




/* フォント設定 */
void CEditView::Command_FONT( void )
{
	HWND	hwndFrame;
	hwndFrame = ::GetParent( m_hwndParent );

	/* フォント設定ダイアログ */
	if( m_pcEditDoc->SelectFont( &(m_pShareData->m_Common.m_lf) ) ){

//		/* 変更フラグ フォント */
//		m_pShareData->m_bFontModify = TRUE;

		if( m_pShareData->m_Common.m_lf.lfPitchAndFamily & FIXED_PITCH  ){
			m_pShareData->m_Common.m_bFontIs_FIXED_PITCH = TRUE;	/* 現在のフォントは固定幅フォントである */
		}else{
			m_pShareData->m_Common.m_bFontIs_FIXED_PITCH = FALSE;	/* 現在のフォントは固定幅フォントである */
		}
		/* 設定変更を反映させる */
		/* 全編集ウィンドウへメッセージをポストする */
		m_cShareData.PostMessageToAllEditors(
//		m_cShareData.SendMessageToAllEditors(
			MYWM_CHANGESETTING,
			(WPARAM)0, (LPARAM)0, hwndFrame
		);

		/* キャレットの表示 */
//		::HideCaret( m_hWnd );
//		::ShowCaret( m_hWnd );

//		/* アクティブにする */
//		/* アクティブにする */
//		ActivateFrameWindow( hwndFrame );
	}
	return;
}




/* 共通設定 */
void CEditView::Command_OPTION( void )
{
	/* 設定プロパティシート テスト用 */
	m_pcEditDoc->OpenPropertySheet( -1/*, -1*/ );

	return;
}




/* タイプ別設定 */
void CEditView::Command_OPTION_TYPE( void )
{
	m_pcEditDoc->OpenPropertySheetTypes( -1, m_pcEditDoc->GetDocumentType() );

	return;
}




/* タイプ別設定一覧 */
void CEditView::Command_TYPE_LIST( void )
{
	CDlgTypeList	cDlgTypeList;
	int				nSettingType;
//	cDlgTypeList.Create( m_hInstance, m_hWnd );
	nSettingType = m_pcEditDoc->GetDocumentType();
	if( cDlgTypeList.DoModal( m_hInstance, m_hWnd, &nSettingType ) ){
		//	Nov. 29, 2000 genta
		//	一時的な設定適用機能を無理矢理追加
		if( nSettingType & PROP_TEMPCHANGE_FLAG ){
			m_pcEditDoc->SetDocumentType( nSettingType & ~PROP_TEMPCHANGE_FLAG, true );
			m_pcEditDoc->LockDocumentType();
			/* 設定変更を反映させる */
			m_pcEditDoc->OnChangeSetting();
		}
		else{
			/* タイプ別設定 */
			m_pcEditDoc->OpenPropertySheetTypes( -1, nSettingType );
		}
	}
	return;
}




/* 行の二重化(折り返し単位) */
void CEditView::Command_DUPLICATELINE( void )
{
	const char*		pLine;
	int				nLineLen;
	int				nCaretPosXOld;
	int				nCaretPosYOld;
	COpe*			pcOpe = NULL;
	int				nNewLine;
	int				nNewPos;
//	int				i;
	int				bCRLF;
	int				bAddCRLF;
	CMemory			cmemBuf;
	const CLayout*	pcLayout;

	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
	}

	pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
	if( NULL == pLine ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nOpe = OPE_MOVECARET;									/* 操作種別 */
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}

	nCaretPosXOld = m_nCaretPosX;
	nCaretPosYOld = m_nCaretPosY + 1;

	//行頭に移動(折り返し単位)
	Command_GOLINETOP( m_bSelectingLock, TRUE );

	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nOpe = OPE_MOVECARET;									/* 操作種別 */
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}



	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
	}

	/* 二重化したい行を調べる
	||	・改行で終わっている
	||	・改行で終わっていない
	||	・最終行である
	||	→折り返しでない
	||	・最終行でない
	||	→折り返しである
	*/
	bCRLF = ( EOL_NONE == pcLayout->m_cEol ) ? FALSE : TRUE;
//	for( i = 0; i < nLineLen; ++i ){
//		if( pLine[i] == CR || pLine[i] == LF ){
//			bCRLF = TRUE;
//			break;
//		}
//	}
	bAddCRLF = FALSE;
	if( !bCRLF ){
		if( m_nCaretPosY == m_pcEditDoc->m_cLayoutMgr.GetLineCount() - 1 ){
			bAddCRLF = TRUE;
		}
	}

	cmemBuf.SetData( pLine, nLineLen + ( (0 == pcLayout->m_cEol.GetLen()) ? (0) : (pcLayout->m_cEol.GetLen() - 1) ) );
	if( bAddCRLF ){
		/* 現在、Enterなどで挿入する改行コードの種類を取得 */
		CEOL cWork = GetCurrentInsertEOL();
		cmemBuf.Append( cWork.GetValue(), cWork.GetLen() );
//		cmemBuf.Append( CRLF, lstrlen( CRLF ) );
	}

	/* 現在位置にデータを挿入 */
	InsertData_CEditView(
		m_nCaretPosX,
		m_nCaretPosY,
		(char*)cmemBuf.GetPtr( NULL ),
		cmemBuf.GetLength(),
		&nNewLine,
		&nNewPos,
		pcOpe,
		TRUE
	);

	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe->m_nCaretPosX_After = nNewPos;	/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = nNewLine;	/* 操作後のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_After,
//			pcOpe->m_nCaretPosY_After,
//			&pcOpe->m_nCaretPosX_PHY_After,
//			&pcOpe->m_nCaretPosY_PHY_After
//		);
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
			nNewPos,
			nNewLine,
			&pcOpe->m_nCaretPosX_PHY_After,
			&pcOpe->m_nCaretPosY_PHY_After
		);

		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}

	/* カーソルを移動 */
	MoveCursor( nCaretPosXOld, nCaretPosYOld, TRUE );
	m_nCaretPosX_Prev = m_nCaretPosX;


	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
		pcOpe->m_nOpe = OPE_MOVECARET;				/* 操作種別 */
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;	/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;	/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */

//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;						/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;						/* 操作後のキャレット位置Ｙ */
//		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;				/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;				/* 操作後のキャレット位置Ｙ */
		pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}
	return;
}




/* 英大文字→英小文字 */
void CEditView::Command_TOLOWER( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TOLOWER );
	return;
}




/* 英小文字→英大文字 */
void CEditView::Command_TOUPPER( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TOUPPER );
	return;
}




/* 全角→半角 */
void CEditView::Command_TOHANKAKU( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TOHANKAKU );
	return;
}




/* 半角＋全ひら→全角・カタカナ */	//Sept. 17, 2000 jepro 説明を「半角→全角カタカナ」から変更
void CEditView::Command_TOZENKAKUKATA( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TOZENKAKUKATA );
	return;
}




/* 半角＋全カタ→全角・ひらがな */	//Sept. 17, 2000 jepro 説明を「半角→全角ひらがな」から変更
void CEditView::Command_TOZENKAKUHIRA( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TOZENKAKUHIRA );
	return;
}




/* 半角カタカナ→全角カタカナ */
void CEditView::Command_HANKATATOZENKAKUKATA( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_HANKATATOZENKAKUKATA );
	return;
}




/* 半角カタカナ→全角ひらがな */
void CEditView::Command_HANKATATOZENKAKUHIRA( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_HANKATATOZENKAKUHIRA );
	return;
}




/* TAB→空白 */
void CEditView::Command_TABTOSPACE( void )
{
	HDC			hdc;
	PAINTSTRUCT	ps;
	int			nSelectLineFromOld;	/* 範囲選択開始行 */
	int			nSelectColFromOld; 	/* 範囲選択開始桁 */
	int			nSelectLineToOld;	/* 範囲選択終了行 */
	int			nSelectColToOld;	/* 範囲選択終了桁 2*/
	RECT		rcSel;
	CMemory		cmemBuf;
//	HGLOBAL		hgClip;
//	char*		pszClip;
//	const char*	pLine;
//	int			nLineLen;
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		return;
	}
	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){
		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_nSelectLineFrom,		/* 範囲選択開始行 */
			m_nSelectColmFrom,		/* 範囲選択開始桁 */
			m_nSelectLineTo,		/* 範囲選択終了行 */
			m_nSelectColmTo			/* 範囲選択終了桁 */
		);
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
//		/* 挿入データの先頭位置へカーソルを移動 */
//		MoveCursor( rcSel.left, rcSel.top, FALSE );
		m_nSelectLineFrom = rcSel.top;			/* 範囲選択開始行 */
		m_nSelectColmFrom = 0;		 			/* 範囲選択開始桁 */
		m_nSelectLineTo = rcSel.bottom + 1;		/* 範囲選択終了行 */
		m_nSelectColmTo = 0;					/* 範囲選択終了桁 */
		m_bBeginBoxSelect = FALSE;
	}else{
		nSelectLineFromOld = m_nSelectLineFrom;	/* 範囲選択開始行 */
		nSelectColFromOld = 0; 					/* 範囲選択開始桁 */
		nSelectLineToOld = m_nSelectLineTo;		/* 範囲選択終了行 */
		if( m_nSelectColmTo > 0 ){
			++nSelectLineToOld;					/* 範囲選択終了行 */
		}
		nSelectColToOld = 0;					/* 範囲選択終了桁 */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
		m_nSelectLineFrom = nSelectLineFromOld;	/* 範囲選択開始行 */
		m_nSelectColmFrom = nSelectColFromOld; 	/* 範囲選択開始桁 */
		m_nSelectLineTo = nSelectLineToOld;		/* 範囲選択終了行 */
		m_nSelectColmTo = nSelectColToOld;		/* 範囲選択終了桁 */
	}
	/* 再描画 */
	//	::UpdateWindow();
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	OnKillFocus();
	OnPaint( hdc, &ps, TRUE );	/* メモリＤＣを使用してちらつきのない再描画 */
	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );
	/* 選択範囲をクリップボードにコピー */
	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合は終了する */
	if( FALSE == GetSelectedData(
		cmemBuf,
		FALSE,
		NULL, /* 引用符 */
		FALSE /* 行番号を付与する */
	) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_TABTOSPACE );
	return;
}

/* 空白→TAB */ //#### Stonee, 2001/05/27
void CEditView::Command_SPACETOTAB( void )
{
	HDC			hdc;
	PAINTSTRUCT	ps;
	int			nSelectLineFromOld;	/* 範囲選択開始行 */
	int			nSelectColFromOld; 	/* 範囲選択開始桁 */
	int			nSelectLineToOld;	/* 範囲選択終了行 */
	int			nSelectColToOld;	/* 範囲選択終了桁 2*/
	RECT		rcSel;
	CMemory		cmemBuf;
//	HGLOBAL		hgClip;
//	char*		pszClip;
//	const char*	pLine;
//	int			nLineLen;
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		return;
	}
	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){
		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_nSelectLineFrom,		/* 範囲選択開始行 */
			m_nSelectColmFrom,		/* 範囲選択開始桁 */
			m_nSelectLineTo,		/* 範囲選択終了行 */
			m_nSelectColmTo			/* 範囲選択終了桁 */
		);
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
//		/* 挿入データの先頭位置へカーソルを移動 */
//		MoveCursor( rcSel.left, rcSel.top, FALSE );
		m_nSelectLineFrom = rcSel.top;			/* 範囲選択開始行 */
		m_nSelectColmFrom = 0;		 			/* 範囲選択開始桁 */
		m_nSelectLineTo = rcSel.bottom + 1;		/* 範囲選択終了行 */
		m_nSelectColmTo = 0;					/* 範囲選択終了桁 */
		m_bBeginBoxSelect = FALSE;
	}else{
		nSelectLineFromOld = m_nSelectLineFrom;	/* 範囲選択開始行 */
		nSelectColFromOld = 0; 					/* 範囲選択開始桁 */
		nSelectLineToOld = m_nSelectLineTo;		/* 範囲選択終了行 */
		if( m_nSelectColmTo > 0 ){
			++nSelectLineToOld;					/* 範囲選択終了行 */
		}
		nSelectColToOld = 0;					/* 範囲選択終了桁 */
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );
		m_nSelectLineFrom = nSelectLineFromOld;	/* 範囲選択開始行 */
		m_nSelectColmFrom = nSelectColFromOld; 	/* 範囲選択開始桁 */
		m_nSelectLineTo = nSelectLineToOld;		/* 範囲選択終了行 */
		m_nSelectColmTo = nSelectColToOld;		/* 範囲選択終了桁 */
	}
	/* 再描画 */
	//	::UpdateWindow();
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	OnKillFocus();
	OnPaint( hdc, &ps, TRUE );	/* メモリＤＣを使用してちらつきのない再描画 */
	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );
	/* 選択範囲をクリップボードにコピー */
	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合は終了する */
	if( FALSE == GetSelectedData(
		cmemBuf,
		FALSE,
		NULL, /* 引用符 */
		FALSE /* 行番号を付与する */
	) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}

	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_SPACETOTAB );
	return;
}



//#define F_HANKATATOZENKAKUKATA	30557	/* 半角カタカナ→全角カタカナ */
//#define F_HANKATATOZENKAKUHIRA	30558	/* 半角カタカナ→全角ひらがな */




/* E-Mail(JIS→SJIS)コード変換 */
void CEditView::Command_CODECNV_EMAIL( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_EMAIL );
	return;
}




/* EUC→SJISコード変換 */
void CEditView::Command_CODECNV_EUC2SJIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_EUC2SJIS );
	return;
}




/* Unicode→SJISコード変換 */
void CEditView::Command_CODECNV_UNICODE2SJIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_UNICODE2SJIS );
	return;
}




/* SJIS→JISコード変換 */
void CEditView::Command_CODECNV_SJIS2JIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_SJIS2JIS );
	return;
}




/* SJIS→EUCコード変換 */
void CEditView::Command_CODECNV_SJIS2EUC( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_SJIS2EUC );
	return;
}




/* UTF-8→SJISコード変換 */
void CEditView::Command_CODECNV_UTF82SJIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_UTF82SJIS );
	return;
}




/* UTF-7→SJISコード変換 */
void CEditView::Command_CODECNV_UTF72SJIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_UTF72SJIS );
	return;
}




/* SJIS→UTF-7コード変換 */
void CEditView::Command_CODECNV_SJIS2UTF7( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_SJIS2UTF7 );
	return;
}




/* SJIS→UTF-8コード変換 */
void CEditView::Command_CODECNV_SJIS2UTF8( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_SJIS2UTF8 );
	return;
}




/* 自動判別→SJISコード変換 */
void CEditView::Command_CODECNV_AUTO2SJIS( void )
{
	/* 選択エリアのテキストを指定方法で変換 */
	ConvSelectedArea( F_CODECNV_AUTO2SJIS );
	return;
}




/* アウトライン解析 */
BOOL CEditView::Command_FUNCLIST( BOOL bCheckOnly )
{
	if( bCheckOnly ){
		return TRUE;
	}

	static CFuncInfoArr	cFuncInfoArr;
//	int		nLine;
	int		nListType;

	if( NULL != m_pcEditDoc->m_cDlgFuncList.m_hWnd ){
		/* アクティブにする */
		ActivateFrameWindow( m_pcEditDoc->m_cDlgFuncList.m_hWnd );
		return TRUE;
	}

	/* 解析結果データを空にする */
	cFuncInfoArr.Empty();

	/* タイプ別に設定されたアウトライン解析方法 */
	nListType = m_pcEditDoc->GetDocumentAttribute().m_nDefaultOutline;
	switch( nListType ){
//	case OUTLINE_C:			m_pcEditDoc->MakeFuncList_C( &cFuncInfoArr );break;
	case OUTLINE_CPP:
		m_pcEditDoc->MakeFuncList_C( &cFuncInfoArr );
		/* C言語標準保護委員会勧告特別処理実装箇所(嘘) */
		if( CheckEXT( m_pcEditDoc->m_szFilePath, "c" ) ){
			nListType = OUTLINE_C;	/* これでC関数一覧リストビューになる */
		}
		break;
	case OUTLINE_PLSQL:		m_pcEditDoc->MakeFuncList_PLSQL( &cFuncInfoArr );break;
	case OUTLINE_JAVA:		m_pcEditDoc->MakeFuncList_Java( &cFuncInfoArr );break;
	case OUTLINE_COBOL:		m_pcEditDoc->MakeTopicList_cobol( &cFuncInfoArr );break;
	case OUTLINE_ASM:		m_pcEditDoc->MakeTopicList_asm( &cFuncInfoArr );break;
	case OUTLINE_PERL:		m_pcEditDoc->MakeFuncList_Perl( &cFuncInfoArr );break;	//	Sep. 8, 2000 genta
	case OUTLINE_VB:		m_pcEditDoc->MakeFuncList_VisualBasic( &cFuncInfoArr );break;	//	June 23, 2001 N.Nakatani
	case OUTLINE_TEXT:
//	case OUTLINE_UNKNOWN:	//Jul. 08, 2001 JEPRO 使わないように変更
	default:
		m_pcEditDoc->MakeTopicList_txt( &cFuncInfoArr );
		break;
	}

	/* 解析対象ファイル名 */
	strcpy( cFuncInfoArr.m_szFilePath, m_pcEditDoc->m_szFilePath );

	/* アウトライン ダイアログ */
//	m_pcEditDoc->m_cDlgFuncList.Create(
//		m_hInstance,
//		/*m_pcEditDoc->*/m_hWnd,
//		&cFuncInfoArr,
//		m_nCaretPosY + 1,
//		nListType,
//		m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
//	);

//	/* アウトライン ダイアログをモーダルにするか */
//	if( ????? ){
//		nLine = m_pcEditDoc->m_cDlgFuncList.DoModal();
//		if( nLine > 0 ){
//			/* 矩形範囲選択中か */
//			if( IsTextSelected() ){	/* テキストが選択されているか */
//				/* 現在の選択範囲を非選択状態に戻す */
//				DisableSelectArea( TRUE );
//			}
//
//			/*
//			  カーソル位置変換
//			  物理位置(行頭からのバイト数、折り返し無し行位置)
//			  →
//			  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
//			*/
//			int		nPosX;
//			int		nPosY;
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log(
//				0,
//				nLine - 1,
//				&nPosX,
//				&nPosY
//			);
//			/* カーソルを選択開始位置に移動 */
//			MoveCursor( 0, nPosY, TRUE, _CARETMARGINRATE / 3 );
//			m_nCaretPosX_Prev = m_nCaretPosX;
//		}
//	}else{
		/* アウトライン ダイアログの表示 */
		if( NULL == m_pcEditDoc->m_cDlgFuncList.m_hWnd ){
//			m_pcEditDoc->m_cDlgFuncList.DoModeless( this );
			m_pcEditDoc->m_cDlgFuncList.DoModeless(
				m_hInstance,
				/*m_pcEditDoc->*/m_hWnd,
				(LPARAM)this,
				&cFuncInfoArr,
				m_nCaretPosY + 1,
				nListType,
				m_pcEditDoc->GetDocumentAttribute().m_bLineNumIsCRLF	/* 行番号の表示 FALSE=折り返し単位／TRUE=改行単位 */
			);
		}else{
			/* アクティブにする */
			ActivateFrameWindow( m_pcEditDoc->m_cDlgFuncList.m_hWnd );
//			::SetDlgItemText( m_pcEditDoc->m_cDlgFuncList.m_hWnd, IDC_COMBO_TEXT, cmemCurText.GetPtr( NULL ) );
		}
//	}

	return TRUE;
}




/* 上下に分割 */	//Sept. 17, 2000 jepro 説明の「縦」を「上下に」に変更
void CEditView::Command_SPLIT_V( void )
{
	m_pcEditDoc->m_cSplitterWnd.VSplitOnOff();
	return;
}




/* 左右に分割 */	//Sept. 17, 2000 jepro 説明の「横」を「左右に」に変更
void CEditView::Command_SPLIT_H( void )
{
	m_pcEditDoc->m_cSplitterWnd.HSplitOnOff();
	return;
}




/* 縦横に分割 */	//Sept. 17, 2000 jepro 説明に「に」を追加
void CEditView::Command_SPLIT_VH( void )
{
	m_pcEditDoc->m_cSplitterWnd.VHSplitOnOff();
	return;
}




//From Here Nov. 25, 2000 JEPRO
/* ヘルプ目次 */
void CEditView::Command_HELP_CONTENTS( void )
{
	char	szHelp[_MAX_PATH + 1];
	/* ヘルプファイルのフルパスを返す */
	::GetHelpFilePath( szHelp );
//From Here Jan. 13, 2001 JEPRO HELP_FINDERでは前回アクティブだったトピックの検索のタブになってしまう
// 一方 HELP_CONTENTS (あるいは HELP＿INDEX) だと目次ページが出てくる。それもいいが...
//	::WinHelp( m_hWnd, szHelp, HELP_FINDER, 0 );
	::WinHelp( m_hWnd, szHelp, HELP_COMMAND, (unsigned long)"CONTENTS()" );	//[目次]タブの表示
//To Here Jan. 13, 2001
	return;
}




/* ヘルプキーワード検索 */
void CEditView::Command_HELP_SEARCH( void )
{
	char	szHelp[_MAX_PATH + 1];
	/* ヘルプファイルのフルパスを返す */
	::GetHelpFilePath( szHelp );
	::WinHelp( m_hWnd, szHelp, HELP_KEY, (unsigned long)"" );
	return;
}
//To Here Nov. 25, 2000




/* コマンド一覧 */
void CEditView::Command_MENU_ALLFUNC( void )
{

	char	szLabel[300];
//	char	szLabel2[300];
	UINT	uFlags;
	POINT	po;
	HMENU	hMenu;
	HMENU	hMenuPopUp;
	int		i;
	int		j;
	int		nId;

//	From Here Sept. 15, 2000 JEPRO
//	サブメニュー、特に「その他」のコマンドに対してステータスバーに表示されるキーアサイン情報が
//	メニューで隠れないように右にずらした
//	(本当はこの「コマンド一覧」メニューをダイアログに変更しバーをつまんで自由に移動できるようにしたい)
//	po.x = 0;
	po.x = 540;
//	To Here Sept. 15, 2000 (Oct. 7, 2000 300→500; Nov. 3, 2000 500→540)
	po.y = 0;
	::ClientToScreen( m_hWnd, &po );

	CEditWnd*	pCEditWnd;
	pCEditWnd = ( CEditWnd* )::GetWindowLong( ::GetParent( m_hwndParent ), GWL_USERDATA );
	pCEditWnd->m_CMenuDrawer.ResetContents();

	hMenu = ::CreatePopupMenu();
//Oct. 14, 2000 JEPRO 「--未定義--」を表示させないように変更したことで1番(カーソル移動系)が前にシフトされた(この変更によって i=1→i=0 と変更)
	for( i = 0; i < nsFuncCode::nFuncKindNum; i++ ){
		hMenuPopUp = ::CreatePopupMenu();
		for( j = 0; j < nsFuncCode::pnFuncListNumArr[i]; j++ ){
			::LoadString( m_hInstance, nsFuncCode::ppnFuncListArr[i][j], szLabel, 256 );
			uFlags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
//			uFlags = MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED;
			pCEditWnd->m_CMenuDrawer.MyAppendMenu( hMenuPopUp, uFlags, nsFuncCode::ppnFuncListArr[i][j] , szLabel );
		}
		pCEditWnd->m_CMenuDrawer.MyAppendMenu( hMenu, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT)hMenuPopUp , nsFuncCode::ppszFuncKind[i] );
	}

	nId = ::TrackPopupMenu(
		hMenu,
		TPM_TOPALIGN
		| TPM_LEFTALIGN
		| TPM_RETURNCMD
		| TPM_LEFTBUTTON
		,
		po.x,
		po.y,
		0,
		::GetParent( m_hwndParent )/*m_hWnd*/,
		NULL
	);
	::DestroyMenu( hMenu );
	if( 0 != nId ){
		/* コマンドコードによる処理振り分け */
//		HandleCommand( nFuncID, TRUE, 0, 0, 0, 0 );
		::PostMessage( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nId, 0 ), (LPARAM)NULL );
	}
	return;
}




/* 外部ヘルプ１ */
void CEditView::Command_EXTHELP1( void )
{
retry:;
	if( 0 == strlen( m_pShareData->m_Common.m_szExtHelp1 ) ){
		::MessageBeep( MB_ICONHAND );
//From Here Sept. 15, 2000 JEPRO
//		[Esc]キーと[x]ボタンでも中止できるように変更
//		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
//To Here Sept. 15, 2000
			"外部ヘルプ１が設定されていません。\n今すぐ設定しますか?"
		) ){
			/* 共通設定 プロパティシート */
			if( !m_pcEditDoc->OpenPropertySheet( ID_PAGENUM_HELPER/*, IDC_EDIT_EXTHELP1*/ ) ){
				return;
			}
			goto retry;
		}
		//	Jun. 15, 2000 genta
		else{
			return;
		}
	}

	CMemory		cmemCurText;
	/* 現在カーソル位置単語または選択範囲より検索等のキーを取得 */
	GetCurrentTextForSearch( cmemCurText );
	::WinHelp( m_hwndParent, m_pShareData->m_Common.m_szExtHelp1, HELP_KEY, (DWORD)(char*)cmemCurText.GetPtr( NULL ) );
	return;
}




/* 外部HTMLヘルプ */
void CEditView::Command_EXTHTMLHELP( void )
{
	HWND		hwndHtmlHelp;
//	HWND		hwndHtmlHelpChild;
	CMemory		cmemCurText;
	int			nLen;

retry:;
	if( 0 == strlen( m_pShareData->m_Common.m_szExtHtmlHelp ) ){
		::MessageBeep( MB_ICONHAND );
//	From Here Sept. 15, 2000 JEPRO
//		[Esc]キーと[x]ボタンでも中止できるように変更
//		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
//	To Here Sept. 15, 2000
			"外部HTMLヘルプが設定されていません。\n今すぐ設定しますか?"
		) ){
			/* 共通設定 プロパティシート */
			if( !m_pcEditDoc->OpenPropertySheet( ID_PAGENUM_HELPER/*, IDC_EDIT_EXTHTMLHELP*/ ) ){
				return;
			}
			goto retry;
		}
		//	Jun. 15, 2000 genta
		else{
			return;
		}
	}

	/* 現在カーソル位置単語または選択範囲より検索等のキーを取得 */
	GetCurrentTextForSearch( cmemCurText );

	/* HtmlHelpビューアはひとつ */
	if( m_pShareData->m_Common.m_bHtmlHelpIsSingle ){
		// タスクトレイのプロセスにHtmlHelpを起動させる
		strcpy( m_pShareData->m_szWork, m_pShareData->m_Common.m_szExtHtmlHelp );
		nLen = lstrlen( m_pShareData->m_Common.m_szExtHtmlHelp );
		strcpy( &m_pShareData->m_szWork[nLen + 1], cmemCurText.GetPtr( NULL ) );
		hwndHtmlHelp = (HWND)::SendMessage( m_pShareData->m_hwndTray, MYWM_HTMLHELP, (WPARAM)::GetParent( m_hwndParent ), 0 );
	}else{
		/* 自分でHtmlHelpを起動させる */
		HH_AKLINK	link;
		link.cbStruct = sizeof( HH_AKLINK ) ;
		link.fReserved = FALSE ;
		link.pszKeywords = (char*)cmemCurText.GetPtr( NULL );
		link.pszUrl = NULL;
		link.pszMsgText = NULL;
		link.pszMsgTitle = NULL;
		link.pszWindow = NULL;
		link.fIndexOnFail = TRUE;

		//	Jul. 6, 2001 genta HtmlHelpの呼び出し方法変更
		hwndHtmlHelp = OpenHtmlHelp(
			NULL/*m_pShareData->m_hwndTray*/,
			m_pShareData->m_Common.m_szExtHtmlHelp,
			HH_KEYWORD_LOOKUP,
			(DWORD)&link
		);
	}



//	if( NULL != hwndHtmlHelp ){
//		hwndHtmlHelpChild = ::FindWindowEx( hwndHtmlHelp, NULL, "#32770", NULL );
//		if( NULL != hwndHtmlHelpChild ){
//			hwndHtmlHelp = hwndHtmlHelpChild;
//		}else{
//		}
//	}
//	int		nResult;
//	nResult = ::PostMessage(
//		hwndHtmlHelpChild,
//		WM_MOUSEACTIVATE,
//		(WPARAM) hwndHtmlHelp,					// アプリケーションのウィンドウを指定
//		MAKELPARAM( HTCLIENT, WM_LBUTTONDOWN )	// とりあえずクライアント領域でLBUTTONDOWNにしておく
//	);
//	if ( nResult == MA_ACTIVATE || nResult == MA_ACTIVATEANDEAT ){
//		::SetFocus( hwndHtmlHelpChild );		// アクティブにする場合だけフォーカスを移す
//		/* アクティブにする */
//		ActivateFrameWindow( hwndHtmlHelpChild );
//	}
//
//	HWND	hToplevel = ::FindWindow( "#32770", "設定" );
//	HWND	hWnd = ::FindWindowEx( hToplevel, NULL, "Edit", NULL );
//	if ( hwndHtmlHelp != NULL ){
//		DWORD	dwPID;
//		DWORD	dwTID = ::GetWindowThreadProcessId( hwndHtmlHelp, &dwPID );
//		::AttachThreadInput( ::GetCurrentThreadId(), dwTID, TRUE) ;
//		::SetFocus( hwndHtmlHelp );
//		::AttachThreadInput( ::GetCurrentThreadId(), dwTID, FALSE );
//	}

/* 自分でHtmlHelpを起動させる */
//	hwndHtmlHelp = ::HtmlHelp(
//		NULL/*m_pShareData->m_hwndTray*/,
//		m_pShareData->m_Common.m_szExtHtmlHelp,
//		HH_DISPLAY_TOPIC,
//		(DWORD)0
//	);
//	HH_AKLINK	link;
//	link.cbStruct = sizeof(HH_AKLINK) ;
//	link.fReserved = FALSE ;
//	link.pszKeywords = (char*)cmemCurText.GetPtr( NULL );
//	link.pszUrl = NULL;
//	link.pszMsgText = NULL;
//	link.pszMsgTitle = NULL;
//	link.pszWindow = NULL;
//	link.fIndexOnFail = TRUE;
//	hwndHtmlHelp = ::HtmlHelp(
//		NULL/*m_pShareData->m_hwndTray*/,
//		m_pShareData->m_Common.m_szExtHtmlHelp,
//		HH_KEYWORD_LOOKUP,
//		(DWORD)&link
//	);


//	memcpy( m_pShareData->m_szWork, (void*)&link, sizeof(HH_AKLINK) );

//	/* アクティブにする */
//	ActivateFrameWindow( hwndHtmlHelp );
//	hwndHtmlHelpChild = ::FindWindowEx( hwndHtmlHelp, NULL, "#32770", NULL );
//	if( NULL != hwndHtmlHelpChild ){
//		::BringWindowToTop( hwndHtmlHelpChild );

//		/* アクティブにする */
//		ActivateFrameWindow( hwndHtmlHelpChild );
//		::SetFocus( hwndHtmlHelpChild );
//		::SetActiveWindow( hwndHtmlHelpChild );
//		::PostMessage( hwndHtmlHelpChild, WM_ACTIVATEAPP, TRUE, NULL );
//	}else{
//		::BringWindowToTop( hwndHtmlHelp );

//		/* アクティブにする */
//		ActivateFrameWindow( hwndHtmlHelp );
//		::SetFocus( hwndHtmlHelp );
//		::SetActiveWindow( hwndHtmlHelp );
//		::PostMessage( hwndHtmlHelp, WM_ACTIVATEAPP, TRUE, NULL );
//	}

	//	Jul. 6, 2001 genta hwndHtmlHelpのチェックを追加
	if( hwndHtmlHelp != NULL ){
		::BringWindowToTop( hwndHtmlHelp );
	}

	return;
}




//From Here Dec. 25, 2000 JEPRO
/* バージョン情報 */
void CEditView::Command_ABOUT( void )
{
	CDlgAbout cDlgAbout;
	cDlgAbout.DoModal( m_hInstance, m_hWnd );
	return;
}
//To Here Dec. 25, 2000




/* 右クリックメニュー */
void CEditView::Command_MENU_RBUTTON( void )
{
	int			nId;
	char*		pszStr;
	int			nLength;
//	HGLOBAL		hgClip;
//	char*		pszClip;
	/* ポップアップメニュー(右クリック) */
	nId = CreatePopUpMenu_R();
	if( 0 == nId ){
		return;
	}
	switch( nId ){
	case IDM_COPYDICINFO:
		pszStr = m_cTipWnd.m_cInfo.GetPtr( &nLength );

		/* クリップボードにデータを設定 */
		MySetClipboardData( pszStr, nLength, FALSE );

//		/* Windowsクリップボードにコピー */
//		hgClip = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, nLength + 1 );
//		pszClip = (char*)::GlobalLock( hgClip );
//		memcpy( pszClip, pszStr, nLength + 1 );
//		::GlobalUnlock( hgClip );
//		::OpenClipboard( m_hWnd );
//		::EmptyClipboard();
//		::SetClipboardData( CF_OEMTEXT, hgClip );
//		::CloseClipboard();

		break;
	default:
		/* コマンドコードによる処理振り分け */
//		HandleCommand( nId, TRUE, 0, 0, 0, 0 );
		::PostMessage( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nId, 0 ),  (LPARAM)NULL );
		break;
	}
	return;
}





//typedef BOOL (*LPSENDTEXTMAIL) ( const char*, long, const char*, const char*, const char*, const char*, const char*, const char*, const char*, const char*, long, const char**, long*, long, const char**, const char**, BOOL, BOOL, HWND, HWND, char* );



//	/* メール送信 */
//	void CEditView::Command_SENDMAIL( void )
//	{
//	//	return;
//	BOOL			bRet;
//	HGLOBAL			hgText;
//	char*			pszText;
//	int				nTextLen;
//	const char*		pLine;
//	int				nLineLen;
//	int				i;
//	int				j;
//	int				nPos;
//	HWND			m_hwndSendilg;
//	char			szIP[64];
//	const char*		ppszHeaderNames[] = {  "X-Mailer", "X-IP" };
//	const char*		ppszHeaderValies[] = {  GSTR_APPNAME, "" };
//	ppszHeaderValies[1] = szIP;
//	int				nHeaderNum = sizeof( ppszHeaderNames ) / sizeof( ppszHeaderNames[0] );
//	char			szErrorMessage[1024];
//	HINSTANCE		hinstMailApi32;
//	LPSENDTEXTMAIL	pfSendMail;
//
//	hinstMailApi32 = NULL;
//	hinstMailApi32 = ::LoadLibrary( "mailapi32.dll" );
//	if( NULL == hinstMailApi32 ){
//		::MessageBeep( MB_ICONHAND );
//		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//			"あっ！ メール機能を利用するには、mailapi32.dllが必要なの。え〜ん。"
//		);
//		goto end_of_func;
//	}
//	pfSendMail = (LPSENDTEXTMAIL)::GetProcAddress( hinstMailApi32, "_SENDTEXTMAIL@80" );
//	if( NULL == pfSendMail ){
//		::MessageBeep( MB_ICONHAND );
//		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//			"あっ！ _SENDTEXTMAIL@80()のアドレスが解決できないよ〜。え〜ん。"
//		);
//		goto end_of_func;
//	}
//
//
//
//
//
//	check_setting:;
//	if(    0 == lstrlen( m_pShareData->m_Common.m_szEMailUserName )		//-- 送信者：名前
//		|| 0 == lstrlen( m_pShareData->m_Common.m_szEMailUserAddress )	//-- 送信者：メールアドレス)
//		|| 0 == lstrlen( m_pShareData->m_Common.m_szSMTPServer )		//-- SMTPホスト名・アドレス
//		|| 0 == m_pShareData->m_Common.m_nSMTPPort						//-- SMTPポート番号(通常は25)
//	){
//		/* 設定プロパティシート テスト用 */
//		if( FALSE == m_pcEditDoc->OpenPropertySheet( 8 ) ){
//			goto end_of_func;
//		}
//		goto check_setting;
//	}
//
//
//
//
//
//
//	/* ネットワークIPアドレスを取得 */
//	IN_ADDR in;
//	char name[64];
//	hostent *phostent;
//	if( SOCKET_ERROR != gethostname( name, sizeof( name ) ) ){
//		phostent = gethostbyname( name );
//		if (phostent != NULL) {
//			memcpy( &in, phostent->h_addr, 4 );
//			sprintf( szIP, "%s", inet_ntoa( in ) );
//		}else{
//			strcpy( szIP, "" );
//		}
//	}
//
//	nTextLen = 0;
//	for( i = 0; i < m_pcEditDoc->m_cLayoutMgr.GetLineCount(); ++i ){
//		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( i, &nLineLen );
//		if( NULL != pLine ){
//			for( j = 0; j < nLineLen; ++j ){
//				if( pLine[j] == CR || pLine[j] == LF ){
//					break;
//				}
//			}
//			j += 2;
//			nTextLen += j;
//		}
//	}
//	if( nTextLen == 0 ){
//		::MessageBeep( MB_ICONHAND );
//		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//			"送信すべきテキストがありません。ファイルを開いたり、編集してください。"
//		);
//		goto end_of_func;
//	}
//
//
//	/* メール送信ダイアログ */
//	m_pcEditDoc->m_cDlgSendMail.Create( m_hInstance, m_hWnd );
//	input_info:;
//	if( m_pcEditDoc->m_cDlgSendMail.DoModal() ){
//		if( 0 == lstrlen( m_pcEditDoc->m_cDlgSendMail.m_szTO ) ){
//			::MessageBeep( MB_ICONHAND );
//			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION, GSTR_APPNAME,
//				"宛先を指定してください。"
//			);
//			goto input_info;
//		}
//		if( 0 == lstrlen( m_pcEditDoc->m_cDlgSendMail.m_szSUBJECT ) ){
//			::MessageBeep( MB_ICONHAND );
//			if( IDYES != ::MYMESSAGEBOX( m_hWnd, MB_YESNO | MB_ICONQUESTION, GSTR_APPNAME,
//				"件名が空欄ですが、このまま送信しますか？"
//			) ){
//				goto input_info;
//			}
//		}
//
//
//		hgText = ::GlobalAlloc( GHND, nTextLen + 1 );
//		if( NULL == hgText ){
//			::MessageBeep( MB_ICONHAND );
//			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//				"作業用のメモリが確保できませんでした。"
//			);
//			goto end_of_func;
//		}
//		pszText = (char*)::GlobalLock( hgText );
//		nPos = 0;
//		for( i = 0; i < m_pcEditDoc->m_cLayoutMgr.GetLineCount(); ++i ){
//			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( i, &nLineLen );
//			if( NULL != pLine ){
//				for( j = 0; j < nLineLen; ++j ){
//					if( pLine[j] == CR || pLine[j] == LF ){
//						break;
//					}
//				}
//				memcpy( &pszText[nPos], pLine, j );
//				nPos += j;
//				memcpy( &pszText[nPos], CRLF, lstrlen( CRLF ) );
//				nPos += lstrlen( CRLF );
//
//			}
//		}
//
//
//		m_hwndSendilg = ::CreateDialog(
//			m_hInstance,
//			MAKEINTRESOURCE( IDD_SENDINGMAIL ),
//			m_hWnd,
//			(DLGPROC)SendingMailDialogProc
//		);
//		::EnableWindow( m_hWnd, FALSE );
//		::ShowWindow( m_hwndSendilg, SW_SHOW );
//
//		strcpy( szErrorMessage, "" );
//		bRet = FALSE;
//		bRet = (*pfSendMail)(
//			(const char*)m_pShareData->m_Common.m_szSMTPServer,			//-- SMTPホスト名・アドレス
//			(long)m_pShareData->m_Common.m_nSMTPPort,					//-- SMTPポート番号(通常は25)
//			(const char*)m_pcEditDoc->m_cDlgSendMail.m_szSUBJECT,		//-- 件名
//			(const char*)m_pShareData->m_Common.m_szEMailUserName,		//-- 送信者：名前
//			(const char*)m_pShareData->m_Common.m_szEMailUserAddress,	//-- 送信者：メールアドレス
//			(const char*)"",											//-- 送信者：組織名
//			(const char*)m_pcEditDoc->m_cDlgSendMail.m_szTO,			//-- To宛先一覧  (カンマ区切り \'または \" で囲まれた部分はコメントとみなす)
//			(const char*)m_pcEditDoc->m_cDlgSendMail.m_szCC,			//-- Cc宛先一覧  (カンマ区切り \'または \" で囲まれた部分はコメントとみなす)
//			(const char*)m_pcEditDoc->m_cDlgSendMail.m_szBCC,			//-- Bcc宛先一覧 (カンマ区切り \'または \" で囲まれた部分はコメントとみなす)
//			(const char*)pszText,										//-- 本文テキスト
//			(long)0,													//-- 添付ファイルの数
//			(const char**)NULL,											//-- 添付ファイルのフルパス文字列のアドレスの配列の、先頭アドレス
//			(long*)NULL,												//-- 添付ファイルの符号化方式の配列の先頭アドレス (現在は無視してすべてBase64で符号化する)	//Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
//			(long)nHeaderNum,											//-- 追加ヘッダーの数
//			(const char**)ppszHeaderNames,								//-- 追加ヘッダーのヘッダー名 (例"X-Mailer")
//			(const char**)ppszHeaderValies,								//-- 追加ヘッダーの内容
//			(BOOL)FALSE,												//-- Message-ID:ヘッダーの付与 ( 0==しない )
//			(BOOL)FALSE,												//-- Date:ヘッダーの付与 ( 0==しない )
//			(HWND)::GetDlgItem( m_hwndSendilg, IDC_STATIC_MSG )			//-- 進捗表示用スタティックコントロールのウィンドウハンドル(不要なら0を指定)
//			(HWND)::GetDlgItem( m_hwndSendilg, IDC_PROGRESS_SENDING ),	//-- 進捗表示用プログレスコントロールのウィンドウハンドル(不要なら0を指定)
//			(char*)szErrorMessage
//		);
//		if( bRet ){
//			::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//				"メールを送信しました。\n"
//			 );
//		}else{
//			::MessageBeep( MB_ICONHAND );
//			::MYMESSAGEBOX( m_hwndSendilg, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//	//				"メールの送信ができませんでした。\n\n%s", szErrorMessage
//				"メールの送信機能は今は使えません(開発中)。\n"
//			);
//		}
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テストA\n"
//	//		 );
//		::EnableWindow( m_hWnd, TRUE );
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テストB\n"
//	//		 );
//		 ::DestroyWindow( m_hwndSendilg );
//		m_hwndSendilg = NULL;
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テストC\n"
//	//		 );
//
//		::GlobalUnlock( hgText );
//		::GlobalFree( hgText );
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テストD\n"
//	//		 );
//	}
//	end_of_func:;
//	if( NULL != hinstMailApi32 ){
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テスト1\n"
//	//		 );
//		::FreeLibrary( hinstMailApi32 );
//	//		::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//			"テスト2\n"
//	//		 );
//		hinstMailApi32 = NULL;
//	}
//	//	::MYMESSAGEBOX( m_hwndSendilg, MB_OK , GSTR_APPNAME,
//	//		"テスト3\n"
//	//	 );
//	return;
//	}




/* インデント ver1 */
void CEditView::Command_INDENT( char cChar )
{
	char szWork[2];
	wsprintf( szWork, "%c", cChar );
	Command_INDENT( szWork, lstrlen( szWork ) );
	return;
}




/* インデント ver0 */
void CEditView::Command_INDENT( const char* pData, int nDataLen )
{
	int			nSelectLineFromOld;	/* 範囲選択開始行 */
	int			nSelectColFromOld; 	/* 範囲選択開始桁 */
	int			nSelectLineToOld;	/* 範囲選択終了行 */
	int			nSelectColToOld;	/* 範囲選択終了桁 */
	const char*	pLine;
	int			nLineLen;
	CMemory*	pcMemDeleted;
	CMemory		cMem;
	CWaitCursor cWaitCursor( m_hWnd );
	COpe*		pcOpe = NULL;
	int			nNewLine;			/* 挿入された部分の次の位置の行 */
	int			nNewPos;			/* 挿入された部分の次の位置のデータ位置 */
	int			i;
	HDC			hdc;
	PAINTSTRUCT	ps;
//	char		szWork[16];
	CMemory		cmemBuf;
	RECT		rcSel;
	int			nPosX;
	int			nPosY;
	int			nIdxFrom;
	int			nIdxTo;
	int			nLineNum;
	int			nDelPos;
	int			nDelLen;
	int			nDelPosNext;
	int			nDelLenNext;
	const char*	pLine2;
	int			nLineLen2;
	int*		pnKey_CharCharsArr;
	pnKey_CharCharsArr = NULL;

	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();					/* 親ウィンドウのタイトルを更新 */

	if( !IsTextSelected() ){			/* テキストが選択されているか */
//		/* 1バイト文字入力 */
		char*	pszWork;
		pszWork = new char[nDataLen + 1];
		memcpy( pszWork, pData, nDataLen );
		pszWork[nDataLen] = '\0';
		/* テキストを貼り付け ver0 */
		Command_INSTEXT( TRUE, pszWork, FALSE );
		delete [] pszWork;
		return;
	}
	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){
		/* 2点を対角とする矩形を求める */
		TwoPointToRect(
			&rcSel,
			m_nSelectLineFrom,		/* 範囲選択開始行 */
			m_nSelectColmFrom,		/* 範囲選択開始桁 */
			m_nSelectLineTo,		/* 範囲選択終了行 */
			m_nSelectColmTo			/* 範囲選択終了桁 */
		);
		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( TRUE );

		nIdxFrom = 0;
		nIdxTo = 0;
		for( nLineNum = rcSel.bottom; nLineNum >= rcSel.top - 1; nLineNum-- ){
			nDelPosNext = nIdxFrom;
			nDelLenNext	= nIdxTo - nIdxFrom;
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
			if( NULL != pLine ){
				/* 指定された桁に対応する行のデータ内の位置を調べる */
				nIdxFrom = LineColmnToIndex( pLine, nLineLen, rcSel.left );
				nIdxTo = LineColmnToIndex( pLine, nLineLen, rcSel.right );

				for( i = nIdxFrom; i <= nIdxTo; ++i ){
					if( pLine[i] == CR || pLine[i] == LF ){
						nIdxTo = i;
						break;
					}
				}
			}else{
				nIdxFrom = 0;
				nIdxTo = 0;
			}
			nDelPos = nDelPosNext;
			nDelLen	= nDelLenNext;
			if( nLineNum < rcSel.bottom ){
				/* TABやスペースインデントの時 */
				if( 1 == nDataLen && ( ' ' == pData[0] || TAB == pData[0] ) ){
					if( 0 == nDelLen ){
						goto nextline;
					}
				}
				pLine2 = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum + 1, &nLineLen2 );
				nPosX = LineIndexToColmn( pLine2, nLineLen2, nDelPos );
				nPosY = nLineNum + 1;
				cmemBuf.SetData( pData, nDataLen );
				if( 0 < nDelLen ){
					if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
						pcOpe = new COpe;
//						pcOpe->m_nCaretPosX_Before = nPosX;	/* 操作前のキャレット位置Ｘ */
//						pcOpe->m_nCaretPosY_Before = nPosY;	/* 操作前のキャレット位置Ｙ */
//						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//							pcOpe->m_nCaretPosX_Before,
//							pcOpe->m_nCaretPosY_Before,
//							&pcOpe->m_nCaretPosX_PHY_Before,
//							&pcOpe->m_nCaretPosY_PHY_Before
//						);
						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
							nPosX,
							nPosY,
							&pcOpe->m_nCaretPosX_PHY_Before,
							&pcOpe->m_nCaretPosY_PHY_Before
						);

					}else{
						pcOpe = NULL;
					}
					pcMemDeleted = new CMemory;
					/* 指定位置の指定長データ削除 */
					DeleteData2(
						nPosX/*rcSel.left*/,
						nPosY/*nLineNum + 1*/,
						nDelLen,
						pcMemDeleted,
						pcOpe,				/* 編集操作要素 COpe */
						FALSE,
						FALSE
					);
	//				sprintf( szWork, "%c", cChar );
	//				cmemBuf.SetData( szWork, lstrlen(szWork) );
	//				cmemBuf.SetData( pData, nDataLen );
					cmemBuf.Append( pcMemDeleted );
					if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//						pcOpe->m_nCaretPosX_After = nPosX;	/* 操作後のキャレット位置Ｘ */
//						pcOpe->m_nCaretPosY_After = nPosY;	/* 操作後のキャレット位置Ｙ */
//						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//							pcOpe->m_nCaretPosX_After,
//							pcOpe->m_nCaretPosY_After,
//							&pcOpe->m_nCaretPosX_PHY_After,
//							&pcOpe->m_nCaretPosY_PHY_After
//						);
						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
							nPosX,
							nPosY,
							&pcOpe->m_nCaretPosX_PHY_After,
							&pcOpe->m_nCaretPosY_PHY_After
						);

						/* 操作の追加 */
						m_pcOpeBlk->AppendOpe( pcOpe );
					}else{
						delete pcMemDeleted;
						pcMemDeleted = NULL;
					}
				}
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
					pcOpe = new COpe;
//					pcOpe->m_nCaretPosX_Before = nPosX;	/* 操作前のキャレット位置Ｘ */
//					pcOpe->m_nCaretPosY_Before = nPosY;	/* 操作前のキャレット位置Ｙ */
//					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//						pcOpe->m_nCaretPosX_Before,
//						pcOpe->m_nCaretPosY_Before,
//						&pcOpe->m_nCaretPosX_PHY_Before,
//						&pcOpe->m_nCaretPosY_PHY_Before
//					);
					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
						nPosX,
						nPosY,
						&pcOpe->m_nCaretPosX_PHY_Before,
						&pcOpe->m_nCaretPosY_PHY_Before
					);
				}
				/* 現在位置にデータを挿入 */
				InsertData_CEditView(
					rcSel.left/*nPosX*/,
					nPosY,
					cmemBuf.GetPtr( NULL ),
					cmemBuf.GetLength(),
					&nNewLine,
					&nNewPos,
					pcOpe,
					FALSE
				);
				MoveCursor( nNewPos, nNewLine, FALSE );
				m_nCaretPosX_Prev = m_nCaretPosX;
				if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//					pcOpe->m_nCaretPosX_After = m_nCaretPosX		;	/* 操作後のキャレット位置Ｘ */
//					pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//					m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//						pcOpe->m_nCaretPosX_After,
//						pcOpe->m_nCaretPosY_After,
//						&pcOpe->m_nCaretPosX_PHY_After,
//						&pcOpe->m_nCaretPosY_PHY_After
//					);
					pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
					pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
					/* 操作の追加 */
					m_pcOpeBlk->AppendOpe( pcOpe );
				}
			}
			nextline:;
		}
		/* 挿入データの先頭位置へカーソルを移動 */
		MoveCursor( rcSel.left, rcSel.top, FALSE );

		/* 挿入文字列の情報 */
		CDocLineMgr::CreateCharCharsArr(
			(const unsigned char *)pData, nDataLen,
			&pnKey_CharCharsArr
		);
		for( i = 0; i < nDataLen; ){
			/* カーソル右移動 */
			Command_RIGHT( FALSE, TRUE, FALSE );
			i+= pnKey_CharCharsArr[i];
		}
		if( NULL != pnKey_CharCharsArr ){
			delete [] pnKey_CharCharsArr;
		}
		rcSel.left = m_nCaretPosX;

		/* カーソルを移動 */
		MoveCursor( rcSel.left, rcSel.top, TRUE );
		m_nCaretPosX_Prev = m_nCaretPosX;

		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
			pcOpe = new COpe;
			pcOpe->m_nOpe = OPE_MOVECARET;				/* 操作種別 */
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;	/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;	/* 操作前のキャレット位置Ｙ */
//			m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//				pcOpe->m_nCaretPosX_Before,
//				pcOpe->m_nCaretPosY_Before,
//				&pcOpe->m_nCaretPosX_PHY_Before,
//				&pcOpe->m_nCaretPosY_PHY_Before
//			);
			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;				/* 操作前のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;				/* 操作前のキャレット位置Ｙ */

//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;						/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;						/* 操作後のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;				/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;				/* 操作後のキャレット位置Ｙ */
			pcOpe->m_nCaretPosX_PHY_After = pcOpe->m_nCaretPosX_PHY_Before;	/* 操作後のキャレット位置Ｘ */
			pcOpe->m_nCaretPosY_PHY_After = pcOpe->m_nCaretPosY_PHY_Before;	/* 操作後のキャレット位置Ｙ */
			/* 操作の追加 */
			m_pcOpeBlk->AppendOpe( pcOpe );
		}
		m_nSelectLineFrom = rcSel.top;			/* 範囲選択開始行 */
		m_nSelectColmFrom = rcSel.left; 		/* 範囲選択開始桁 */
		m_nSelectLineTo = rcSel.bottom;			/* 範囲選択終了行 */
		m_nSelectColmTo = nNewPos;				/* 範囲選択終了桁 */
		m_bBeginBoxSelect = TRUE;
	}else{
		nSelectLineFromOld = m_nSelectLineFrom;	/* 範囲選択開始行 */
		nSelectColFromOld = 0;					/* 範囲選択開始桁 */
		nSelectLineToOld = m_nSelectLineTo;		/* 範囲選択終了行 */
		if( m_nSelectColmTo > 0 ){
			++nSelectLineToOld;					/* 範囲選択終了行 */
		}
		nSelectColToOld = 0;					/* 範囲選択終了桁 */

		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( FALSE );

		const CLayout* pcLayout;
		for( i = nSelectLineToOld - 1; i >= nSelectLineFromOld; i-- ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( i, &nLineLen, &pcLayout );
			if( NULL == pLine ){
				continue;
			}
			if( pLine[0] == CR || pLine[0] == LF ){
				continue;
			}
			cmemBuf.SetData( pData, nDataLen );
			cmemBuf.Append( pLine, nLineLen + ( (0 == pcLayout->m_cEol.GetLen()) ? (0) : (pcLayout->m_cEol.GetLen() - 1) ) );

			/* カーソルを移動 */
			MoveCursor( 0, i, FALSE );
			m_nCaretPosX_Prev = m_nCaretPosX;

			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);
			}else{
				pcOpe = NULL;
			}
			pcMemDeleted = new CMemory;
			/* 指定位置の指定長データ削除 */
			DeleteData2(
				0/*rcSel.left*/,
				i/*nLineNum + 1*/,
				nLineLen,
				pcMemDeleted,
				pcOpe,				/* 編集操作要素 COpe */
				FALSE,
				FALSE
			);
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_After,
					&pcOpe->m_nCaretPosY_PHY_After
				);
				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}else{
				delete pcMemDeleted;
				pcMemDeleted = NULL;
			}
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);
			}
			/* 現在位置にデータを挿入 */
			InsertData_CEditView(
				0,
				i,
				cmemBuf.GetPtr( NULL ),
				cmemBuf.GetLength(),
				&nNewLine,
				&nNewPos,
				pcOpe,
				FALSE
			);
			/* カーソルを移動 */
			MoveCursor( nNewPos, nNewLine, FALSE );
			m_nCaretPosX_Prev = m_nCaretPosX;
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
				pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}
		}
		m_nSelectLineFrom = nSelectLineFromOld;	/* 範囲選択開始行 */
		m_nSelectColmFrom = nSelectColFromOld; 	/* 範囲選択開始桁 */
		m_nSelectLineTo = nSelectLineToOld;		/* 範囲選択終了行 */
		m_nSelectColmTo = nSelectColToOld;		/* 範囲選択終了桁 */
	}
	/* 再描画 */
	//	::UpdateWindow();
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	OnKillFocus();
	OnPaint( hdc, &ps, TRUE );	/* メモリＤＣを使用してちらつきのない再描画 */
	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );
	return;
}




/* 逆インデント */
void CEditView::Command_UNINDENT( char cChar )
{
	if( !IsTextSelected() ){	/* テキストが選択されているか */
		/* １バイト文字入力 */
		Command_CHAR( cChar );
		return;
	}
	int			nSelectLineFromOld;	/* 範囲選択開始行 */
	int			nSelectColFromOld; 	/* 範囲選択開始桁 */
	int			nSelectLineToOld;	/* 範囲選択終了行 */
	int			nSelectColToOld;	/* 範囲選択終了桁 */
	const char*	pLine;
	int			nLineLen;
	CMemory*	pcMemDeleted;
	CMemory		cMem;
	CWaitCursor cWaitCursor( m_hWnd );
	COpe*		pcOpe = NULL;
	int			nNewLine;			/* 挿入された部分の次の位置の行 */
	int			nNewPos;			/* 挿入された部分の次の位置のデータ位置 */
	int			i;
	HDC			hdc;
	PAINTSTRUCT	ps;
	CMemory		cmemBuf;
//	RECT		rcSel;
//	int			nPosX;
//	int			nPosY;
//	int			nIdxFrom;
//	int			nIdxTo;
//	int			nLineNum;
//	int			nDelPos;
//	int			nDelLen;
//	int			nDelPosNext;
//	int			nDelLenNext;
//	const char*	pLine2;
//	int			nLineLen2;
//	char*		pWork;
//	int			nWork;

	if( !IsTextSelected() ){	/* テキストが選択されているか */
		/* 1バイト文字入力 */
		Command_CHAR( cChar );
		return;
	}
	/* 矩形範囲選択中か */
	if( m_bBeginBoxSelect ){
		::MessageBeep( MB_ICONHAND );
//**********************************************
//	 箱型逆インデントについては、保留とする (1998.10.22)
//**********************************************
//		/* 2点を対角とする矩形を求める */
//		TwoPointToRect(
//			&rcSel,
//			m_nSelectLineFrom,	/* 範囲選択開始行 */
//			m_nSelectColmFrom,	/* 範囲選択開始桁 */
//			m_nSelectLineTo,	/* 範囲選択終了行 */
//			m_nSelectColmTo		/* 範囲選択終了桁 */
//		);
//
//		/* 現在の選択範囲を非選択状態に戻す */
//		DisableSelectArea( TRUE );
//
//		nIdxFrom = 0;
//		nIdxTo = 0;
//		for( nLineNum = rcSel.bottom; nLineNum >= rcSel.top - 1; nLineNum-- ){
//			nDelPosNext = nIdxFrom;
//			nDelLenNext	= nIdxTo - nIdxFrom;
//			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum, &nLineLen );
//			if( NULL != pLine ){
//				/* 指定された桁に対応する行のデータ内の位置を調べる */
//				nIdxFrom = LineColmnToIndex( pLine, nLineLen, rcSel.left );
//				nIdxTo = LineColmnToIndex( pLine, nLineLen, rcSel.right );
//
//				for( i = nIdxFrom; i <= nIdxTo; ++i ){
//					if( pLine[i] == CR || pLine[i] == LF ){
//						nIdxTo = i;
//						break;
//					}
//				}
//			}else{
//				nIdxFrom = 0;
//				nIdxTo = 0;
//			}
//			nDelPos = nDelPosNext;
//			nDelLen	= nDelLenNext;
//			if( nLineNum < rcSel.bottom && 0 < nDelLen ){
//
//				pLine2 = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineNum + 1, &nLineLen2 );
//				if( NULL != pLine2 ){
//					nPosX = LineIndexToColmn( pLine2, nLineLen2, nDelPos );
//					nPosY = nLineNum + 1;
//
//					if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//						pcOpe = new COpe;
//						pcOpe->m_nCaretPosX_Before = nPosX;	/* 操作前のキャレット位置Ｘ */
//						pcOpe->m_nCaretPosY_Before = nPosY;	/* 操作前のキャレット位置Ｙ */
//						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//							pcOpe->m_nCaretPosX_Before,
//							pcOpe->m_nCaretPosY_Before,
//							&pcOpe->m_nCaretPosX_PHY_Before,
//							&pcOpe->m_nCaretPosY_PHY_Before
//						);
//					}else{
//						pcOpe = NULL;
//					}
//
//					pcMemDeleted = new CMemory;
//					/* 指定位置の指定長データ削除 */
//					DeleteData(
//						nPosX/*rcSel.left*/,
//						nPosY/*nLineNum + 1*/,
//						nDelLen,
//						pcMemDeleted,
//						pcOpe,				/* 編集操作要素 COpe */
//						FALSE,
//						FALSE
//					);
//					if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//						pcOpe->m_nCaretPosX_After = nPosX;	/* 操作後のキャレット位置Ｘ */
//						pcOpe->m_nCaretPosY_After = nPosY;	/* 操作後のキャレット位置Ｙ */
//						/*  カーソル位置変換
//							レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
//						  →物理位置(行頭からのバイト数、折り返し無し行位置) */
//						m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//							pcOpe->m_nCaretPosX_After,
//							pcOpe->m_nCaretPosY_After,
//							&pcOpe->m_nCaretPosX_PHY_After,
//							&pcOpe->m_nCaretPosY_PHY_After
//						);
//						/* 操作の追加 */
//						m_pcOpeBlk->AppendOpe( pcOpe );
//					}else{
//						delete pcMemDeleted;
//						pcMemDeleted = NULL;
//					}
//
//					pWork = pcMemDeleted->GetPtr( &nWork );
//					if( nWork > 0 ){
//						if( pWork[0] == cChar ){
//							cmemBuf.SetData( pWork + 1, nWork - 1 );
//						}else{
//							cmemBuf.SetData( pWork, nWork );
//						}
//
//						if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//							pcOpe = new COpe;
//							pcOpe->m_nCaretPosX_Before = nPosX;	/* 操作前のキャレット位置Ｘ */
//							pcOpe->m_nCaretPosY_Before = nPosY;	/* 操作前のキャレット位置Ｙ */
//							m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//								pcOpe->m_nCaretPosX_Before,
//								pcOpe->m_nCaretPosY_Before,
//								&pcOpe->m_nCaretPosX_PHY_Before,
//								&pcOpe->m_nCaretPosY_PHY_Before
//							);
//						}
//						/* 現在位置にデータを挿入 */
//						InsertData_CEditView(
//							nPosX,
//							nPosY,
//							cmemBuf.GetPtr( NULL ),
//							cmemBuf.GetLength(),
//							&nNewLine,
//							&nNewPos,
//							pcOpe,
//							FALSE
//						);
//						/* カーソルを移動 */
//						MoveCursor( nNewPos, nNewLine, FALSE );
//						m_nCaretPosX_Prev = m_nCaretPosX;
//						if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//							pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//							pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//							pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
//							pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
//							/* 操作の追加 */
//							m_pcOpeBlk->AppendOpe( pcOpe );
//						}
//					}
//				}
//			}
//		}
//		/* 挿入データの先頭位置へカーソルを移動 */
//		MoveCursor( rcSel.left, rcSel.top, TRUE );
//		m_nCaretPosX_Prev = m_nCaretPosX;
//
//		if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//			pcOpe = new COpe;
//			pcOpe->m_nOpe = OPE_MOVECARET;				/* 操作種別 */
//			pcOpe->m_nCaretPosX_Before = m_nCaretPosX;			/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_Before = m_nCaretPosY;			/* 操作前のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//			pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
//			pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
//			/* 操作の追加 */
//			m_pcOpeBlk->AppendOpe( pcOpe );
//		}
//
//		m_nSelectLineFrom = rcSel.top;			/* 範囲選択開始行 */
//		m_nSelectColmFrom = rcSel.left; 		/* 範囲選択開始桁 */
//		m_nSelectLineTo = rcSel.bottom;			/* 範囲選択終了行 */
//		m_nSelectColmTo = nNewPos;				/* 範囲選択終了桁 */
//		m_bBeginBoxSelect = TRUE;
	}else{
		m_pcEditDoc->m_bIsModified = TRUE;		/* 変更フラグ */
		SetParentCaption();						/* 親ウィンドウのタイトルを更新 */

		nSelectLineFromOld = m_nSelectLineFrom;	/* 範囲選択開始行 */
		nSelectColFromOld = 0;					/* 範囲選択開始桁 */
		nSelectLineToOld = m_nSelectLineTo;		/* 範囲選択終了行 */
		if( m_nSelectColmTo > 0 ){
			nSelectLineToOld++;					/* 範囲選択終了行 */
		}
		nSelectColToOld = 0;					/* 範囲選択終了桁 */

		/* 現在の選択範囲を非選択状態に戻す */
		DisableSelectArea( FALSE );

		const CLayout*	pcLayout;
		int				nDelLen;
		for( i = nSelectLineToOld - 1; i >= nSelectLineFromOld; i-- ){
			pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( i, &nLineLen, &pcLayout );
			if( NULL == pLine ){
				continue;
			}
			if( TAB == cChar ){
				if( pLine[0] == cChar ){
					nDelLen = 1;
				}else{

					int i;
					for( i = 0; i < nLineLen; i++ ){
						if( SPACE != pLine[i] ){
							break;
						}
//						if( i >= m_pcEditDoc->GetDocumentAttribute().m_nTabSpace - 1 ){
						if( i >= m_pcEditDoc->GetDocumentAttribute().m_nTabSpace ){
							break;
						}
					}
//					if( i < m_pcEditDoc->GetDocumentAttribute().m_nTabSpace - 1 ){
					if( 0 == i ){
						continue;
					}
//					nDelLen = m_pcEditDoc->GetDocumentAttribute().m_nTabSpace;
					nDelLen = i;
				}
			}else{
				if( pLine[0] != cChar ){
					continue;
				}
				nDelLen = 1;
			}
			cMem.SetData( &pLine[nDelLen], nLineLen + ( (0 == pcLayout->m_cEol.GetLen()) ? (0) : (pcLayout->m_cEol.GetLen() - 1) ) - nDelLen );
			/* カーソルを移動 */
			MoveCursor( 0, i, FALSE );
			m_nCaretPosX_Prev = m_nCaretPosX;
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);
			}else{
				pcOpe = NULL;
			}
			pcMemDeleted = new CMemory;
			/* 指定位置の指定長データ削除 */
			DeleteData2(
				0/*rcSel.left*/,
				i/*nLineNum + 1*/,
				nLineLen,
				pcMemDeleted,
				pcOpe,				/* 編集操作要素 COpe */
				FALSE,
				FALSE
			);
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_After,
					&pcOpe->m_nCaretPosY_PHY_After
				);
				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}else{
				delete pcMemDeleted;
				pcMemDeleted = NULL;
			}
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe = new COpe;
				m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
					0,
					i,
					&pcOpe->m_nCaretPosX_PHY_Before,
					&pcOpe->m_nCaretPosY_PHY_Before
				);
			}

			/* 現在位置にデータを挿入 */
			InsertData_CEditView(
				0,
				i,
				cMem.GetPtr( NULL ),
				cMem.GetLength(),
				&nNewLine,
				&nNewPos,
				pcOpe,
				FALSE
			);
			/* カーソルを移動 */
			MoveCursor( nNewPos, nNewLine, FALSE );
			m_nCaretPosX_Prev = m_nCaretPosX;
			if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
				pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
				pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
				/* 操作の追加 */
				m_pcOpeBlk->AppendOpe( pcOpe );
			}
		}
		m_nSelectLineFrom = nSelectLineFromOld;	/* 範囲選択開始行 */
		m_nSelectColmFrom = nSelectColFromOld; 	/* 範囲選択開始桁 */
		m_nSelectLineTo = nSelectLineToOld;		/* 範囲選択終了行 */
		m_nSelectColmTo = nSelectColToOld;		/* 範囲選択終了桁 */
	}
	/* 再描画 */
	hdc = ::GetDC( m_hWnd );
	ps.rcPaint.left = 0;
	ps.rcPaint.right = m_nViewAlignLeft + m_nViewCx;
	ps.rcPaint.top = m_nViewAlignTop;
	ps.rcPaint.bottom = m_nViewAlignTop + m_nViewCy;
	OnKillFocus();
	OnPaint( hdc, &ps, TRUE );	/* メモリＤＣを使用してちらつきのない再描画 */
	OnSetFocus();
	::ReleaseDC( m_hWnd, hdc );
	return;
}




/* GREP */
void CEditView::Command_GREP( void )
{
	int			nRet;
	CMemory		cmWork1;
	CMemory		cmWork2;
	CMemory		cmWork3;
	CMemory		cmemCurText;

	/* 編集ウィンドウの上限チェック */
	if( m_pShareData->m_nEditArrNum + 1 > MAX_EDITWINDOWS ){
		char szMsg[512];
		wsprintf( szMsg, "編集ウィンドウ数の上限は%dです。\nこれ以上は同時に開けません。", MAX_EDITWINDOWS );
		::MessageBox( m_hWnd, szMsg, GSTR_APPNAME, MB_OK );
		return;
	}
	/* 現在カーソル位置単語または選択範囲より検索等のキーを取得 */
	GetCurrentTextForSearch( cmemCurText );

	/* キーがないなら、履歴からとってくる */
	if( 0 == cmemCurText.GetLength() ){
//		cmemCurText.SetData( m_pShareData->m_szSEARCHKEYArr[0], lstrlen( m_pShareData->m_szSEARCHKEYArr[0] ) );
		cmemCurText.SetDataSz( m_pShareData->m_szSEARCHKEYArr[0] );
	}
	strcpy( m_pcEditDoc->m_cDlgGrep.m_szText, cmemCurText.GetPtr( NULL ) );

	/* Grepダイアログの表示 */
	nRet = m_pcEditDoc->m_cDlgGrep.DoModal( m_hInstance, m_hWnd, m_pcEditDoc->m_szFilePath );
//	MYTRACE( "nRet=%d\n", nRet );
	if( FALSE == nRet ){
		return;
	}
//	MYTRACE( "m_pcEditDoc->m_cDlgGrep.m_szText  =[%s]\n", m_pcEditDoc->m_cDlgGrep.m_szText );
//	MYTRACE( "m_pcEditDoc->m_cDlgGrep.m_szFile  =[%s]\n", m_pcEditDoc->m_cDlgGrep.m_szFile );
//	MYTRACE( "m_pcEditDoc->m_cDlgGrep.m_szFolder=[%s]\n", m_pcEditDoc->m_cDlgGrep.m_szFolder );
	cmWork1.SetDataSz( m_pcEditDoc->m_cDlgGrep.m_szText );
	cmWork2.SetDataSz( m_pcEditDoc->m_cDlgGrep.m_szFile );
	cmWork3.SetDataSz( m_pcEditDoc->m_cDlgGrep.m_szFolder );
	/* 変更フラグがオフで、ファイルを読み込んでいない場合 */
	if( !m_pcEditDoc->m_bIsModified &&
		0 == lstrlen( m_pcEditDoc->m_szFilePath )		/* 現在編集中のファイルのパス */
	){
		DoGrep(
			&cmWork1,
			&cmWork2,
			&cmWork3,
			m_pcEditDoc->m_cDlgGrep.m_bSubFolder,
			m_pcEditDoc->m_cDlgGrep.m_bLoHiCase,
			m_pcEditDoc->m_cDlgGrep.m_bRegularExp,
			m_pcEditDoc->m_cDlgGrep.m_bKanjiCode_AutoDetect,
			m_pcEditDoc->m_cDlgGrep.m_bGrepOutputLine,
			m_pcEditDoc->m_cDlgGrep.m_bWordOnly,
			m_pcEditDoc->m_cDlgGrep.m_nGrepOutputStyle
		);
	}else{
		/*======= Grepの実行 =============*/
		/* Grep結果ウィンドウの表示 */
		char*	pCmdLine = new char[1024];
		char*	pOpt = new char[64];
		int		nDataLen;
		cmWork1.Replace( "\"", "\"\"" );
		cmWork2.Replace( "\"", "\"\"" );
		cmWork3.Replace( "\"", "\"\"" );
		/*
		|| -GREPMODE -GKEY="1" -GFILE="*.*;*.c;*.h" -GFOLDER="c:\" -GOPT=S
		*/
		wsprintf( pCmdLine, "-GREPMODE -GKEY=\"%s\" -GFILE=\"%s\" -GFOLDER=\"%s\"",
			cmWork1.GetPtr( &nDataLen ),
			cmWork2.GetPtr( &nDataLen ),
			cmWork3.GetPtr( &nDataLen )
		);
		pOpt[0] = '\0';
		if( m_pcEditDoc->m_cDlgGrep.m_bSubFolder ){	/* サブフォルダからも検索する */
			strcat( pOpt, "S" );
		}
	//	if( m_bFromThisText ){	/* この編集中のテキストから検索する */
	//
	//	}
		if( m_pcEditDoc->m_cDlgGrep.m_bLoHiCase ){	/* 英大文字と英小文字を区別する */
			strcat( pOpt, "L" );
		}
		if( m_pcEditDoc->m_cDlgGrep.m_bRegularExp ){	/* 正規表現 */
			strcat( pOpt, "R" );
		}
		if( m_pcEditDoc->m_cDlgGrep.m_bKanjiCode_AutoDetect ){	/* 文字コード自動判別 */
			strcat( pOpt, "K" );
		}
		if( m_pcEditDoc->m_cDlgGrep.m_bGrepOutputLine ){	/* 行を出力するか該当部分だけ出力するか */
			strcat( pOpt, "P" );
		}
		if( 1 == m_pcEditDoc->m_cDlgGrep.m_nGrepOutputStyle ){	/* Grep: 出力形式 */
			strcat( pOpt, "1" );
		}
		if( 2 == m_pcEditDoc->m_cDlgGrep.m_nGrepOutputStyle ){	/* Grep: 出力形式 */
			strcat( pOpt, "2" );
		}
		if( 0 < lstrlen( pOpt ) ){
			strcat( pCmdLine, " -GOPT=" );
			strcat( pCmdLine, pOpt );
		}
//		MYTRACE( "pCmdLine=[%s]\n", pCmdLine );
		/* 新規編集ウィンドウの追加 ver 0 */
		CEditApp::OpenNewEditor( m_hInstance, m_pShareData->m_hwndTray, pCmdLine, 0, FALSE );
		delete [] pCmdLine;
		delete [] pOpt;
		/*======= Grepの実行 =============*/
		/* Grep結果ウィンドウの表示 */
	}
	return;
}




/* 最後にテキストを追加 */
void CEditView::Command_ADDTAIL( const char* pszData, int nDataLen )
{
	int		nNewLine;					/* 挿入された部分の次の位置の行 */
	int		nNewPos;					/* 挿入された部分の次の位置のデータ位置 */
	COpe*	pcOpe = NULL;
	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
	SetParentCaption();					/* 親ウィンドウのタイトルを更新 */
	/*ファイルの最後に移動 */
	Command_GOFILEEND( FALSE );
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
		pcOpe = new COpe;
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;	/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;	/* 操作前のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_Before,
//			pcOpe->m_nCaretPosY_Before,
//			&pcOpe->m_nCaretPosX_PHY_Before,
//			&pcOpe->m_nCaretPosY_PHY_Before
//		);
		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
	}
	/* 現在位置にデータを挿入 */
	InsertData_CEditView(
		m_nCaretPosX,
		m_nCaretPosY,
		(char*)pszData,
		nDataLen,
		&nNewLine,
		&nNewPos,
		pcOpe,
		TRUE
	);
	/* 挿入データの最後へカーソルを移動 */
	MoveCursor( nNewPos, nNewLine, FALSE );
	m_nCaretPosX_Prev = m_nCaretPosX;
	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//		m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
//			pcOpe->m_nCaretPosX_After,
//			pcOpe->m_nCaretPosY_After,
//			&pcOpe->m_nCaretPosX_PHY_After,
//			&pcOpe->m_nCaretPosY_PHY_After
//		);
		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
		/* 操作の追加 */
		m_pcOpeBlk->AppendOpe( pcOpe );
	}
	return;
}




/* タグジャンプ */
void/*BOOL*/ CEditView::Command_TAGJUMP( void/*BOOL bCheckOnly*/ )
{
	const char*	pLine;
	int			nLineLen;
	int			nJumpToLine;
	int			nJumpToColm;
	char		szJumpToFile[1024];
	HWND		hwndOwner;
	POINT		poCaret;
	int			nPathLen;
	int			nBgn;
	memset( szJumpToFile, 0, sizeof(szJumpToFile) );
	nJumpToLine = 1;
	nJumpToColm = 1;
	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	int		nX;
	int		nY;
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		&nX,
		&nY
	);
	/* 現在行のデータを取得 */
	pLine = m_pcEditDoc->m_cDocLineMgr.GetLineStr( nY, &nLineLen );
	if( NULL == pLine ){
		goto can_not_tagjump_end;
	}
	/* WZ風のタグリストか */
	if( 0 == memcmp( pLine, "・", 2 )
	 || 0 == memcmp( pLine, "■\"", 3 )
	){
		if( 0 == memcmp( pLine, "■\"", 3 ) ){
			if( IsFilePath( &pLine[3], &nBgn, &nPathLen ) ){
				memcpy( szJumpToFile, &pLine[3 + nBgn], nPathLen );
				GetLineColm( &pLine[3] + nPathLen, &nJumpToLine, &nJumpToColm );
			}else{
				goto can_not_tagjump;
			}
		}else{
			GetLineColm( &pLine[2], &nJumpToLine, &nJumpToColm );
			nY--;

			for( ; 0 <= nY; nY-- ){
				pLine = m_pcEditDoc->m_cDocLineMgr.GetLineStr( nY, &nLineLen );
				if( NULL == pLine ){
					goto can_not_tagjump;
				}
				if( 0 == memcmp( pLine, "・", 2 ) ){
					continue;
				}else
				if( 0 == memcmp( pLine, "■\"", 3 ) ){
					if( IsFilePath( &pLine[3], &nBgn, &nPathLen ) ){
						memcpy( szJumpToFile, &pLine[3 + nBgn], nPathLen );
						break;
					}else{
						goto can_not_tagjump;
					}
				}else{
					goto can_not_tagjump;
				}
			}
		}
	}else{
		if( IsFilePath( pLine, &nBgn, &nPathLen ) ){
			memcpy( szJumpToFile, &pLine[nBgn], nPathLen );
			GetLineColm( &pLine[nBgn + nPathLen], &nJumpToLine, &nJumpToColm );
		}else{
			goto can_not_tagjump;
		}
	}
	char szWork[MAX_PATH];
	/* ロングファイル名を取得する */
	if( TRUE == ::GetLongFileName( szJumpToFile, szWork ) ){
		strcpy( szJumpToFile, szWork );
	}
	/* 指定ファイルが開かれているか調べる */
	/* 開かれている場合は開いているウィンドウのハンドルも返す */
	/* ファイルを開いているか */
	if( m_cShareData.IsPathOpened( (const char*)szJumpToFile, &hwndOwner ) ){
		/* カーソルを移動させる */
		poCaret.x = nJumpToColm - 1;
		poCaret.y = nJumpToLine - 1;
		memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof(poCaret) );
		::SendMessage( hwndOwner, MYWM_SETCARETPOS, 0, 0 );
		/* アクティブにする */
		ActivateFrameWindow( hwndOwner );
	}else{
		/* 新しく開く */
		FileInfo	inf;
		bool		bSuccess;

		strcpy( inf.m_szPath, szJumpToFile );
		inf.m_nX = nJumpToColm - 1;
		inf.m_nY = nJumpToLine - 1;
		inf.m_nViewLeftCol = inf.m_nViewTopLine = -1;
		inf.m_nCharCode = CODE_AUTODETECT;

		bSuccess = CEditApp::OpenNewEditor2(
			m_hInstance,
			m_pShareData->m_hwndTray,
			&inf,
			FALSE,	/* 読み取り専用か */
			true	//	同期モードで開く
		);

		if( !bSuccess )	//	ファイルが開けなかった
			return;

#if 0
		/* ファイルを開いているか */
		DWORD dwTime;
		dwTime = ::GetTickCount();

		while( FALSE == ( bSuccess = m_cShareData.IsPathOpened( (const char*)szJumpToFile, &hwndOwner ) )
		 && 3000 > ::GetTickCount() - dwTime
		){
			/* 失敗しても3秒間ぐらいは調べてみる */
			Sleep(200);
		}

		if( !bSuccess ){
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
				"タグジャンプに失敗しました。\n\nファイル[%s]\n行[%d]桁[%d]\n",
				szJumpToFile, nJumpToLine, nJumpToColm
			);
			return;
		}
#endif
		//	Apr. 23, 2001 genta
		//	hwndOwnerに値が入らなくなってしまったために
		//	Tag Jump Backが動作しなくなっていたのを修正
		if( FALSE == m_cShareData.IsPathOpened( (const char*)szJumpToFile, &hwndOwner ) )
			return;
	}
	/*
	カーソル位置変換
	レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	→
	物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
//	POINT	poCaret;
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		(int*)&poCaret.x,
		(int*)&poCaret.y
	);
	/* タグジャンプ元通知 */
	memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof( poCaret ) );
	::SendMessage( hwndOwner, MYWM_SETREFERER, (WPARAM)(m_pcEditDoc->m_hwndParent), 0 );
	return;
can_not_tagjump:;
can_not_tagjump_end:;
	::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
		"タグジャンプできません。\n[%s]", szJumpToFile
	);
	return;
}




/* タグジャンプバック */
void/*BOOL*/ CEditView::Command_TAGJUMPBACK( void/*BOOL bCheckOnly*/ )
{
	HWND hwndReferer = m_pcEditDoc->m_hwndReferer;
	if( NULL == hwndReferer ){	/* 参照元ウィンドウ */
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"タグジャンプバックできません。\n戻り先がありません。\n"
		);
		return;
	}
	/* ウィンドウが編集ウィンドウのフレームウィンドウかどうか調べる */
	if( !CShareData::IsEditWnd( hwndReferer ) ){
		m_pcEditDoc->m_hwndReferer = NULL;
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"タグジャンプバックできません。\n戻り先がありません。\n"
		);
		return;
	}
	/* アクティブにする */
	ActivateFrameWindow( hwndReferer );
	if( 0 < m_pcEditDoc->m_nRefererLine ){
		/* カーソルを移動させる */
		POINT poCaret;
		poCaret.x = m_pcEditDoc->m_nRefererX;
		poCaret.y = m_pcEditDoc->m_nRefererLine;
		memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof( poCaret ) );
		::SendMessage( hwndReferer, MYWM_SETCARETPOS, 0, 0 );
	}
	return;
}




/* C/C++ヘッダファイル オープン機能 */		//Feb. 10, 2001 jepro	説明を「インクルードファイル」から変更
//BOOL CEditView::Command_OPENINCLUDEFILE( BOOL bCheckOnly )
BOOL CEditView::Command_OPEN_HHPP( BOOL bCheckOnly )
{
//From Here Feb. 7, 2001 JEPRO 追加
	static char* source_ext[] = { "c", "cpp", "cxx", "cc", "cp", "c++" };
	static char* header_ext[] = { "h", "hpp", "hxx", "hh", "hp", "h++" };
	int		src_extno = 6;
	int		hdr_extno = 6;
	int		i;
	BOOL	bwantopen_h;
//To Here Feb. 7, 2001

	/* 編集中のファイルの拡張子を調べる */
//Feb. 7, 2001 JEPRO 原作版をコメントアウト
//	if( CheckEXT( m_pcEditDoc->m_szFilePath, "cpp" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "cxx" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "c" ) ){
//	}else{
//		if( !bCheckOnly ){
//			::MessageBeep( MB_ICONHAND );
//		}
//		return FALSE;
//	}

//From Here Feb. 7, 2001 JEPRO 追加
	for( i = 0; i < src_extno; i++ ){
		if( CheckEXT( m_pcEditDoc->m_szFilePath, source_ext[i] ) ){
			bwantopen_h = TRUE;
			goto open_h;
		}
	}
	if( !bCheckOnly ){
		::MessageBeep( MB_ICONHAND );
	}
	return FALSE;

open_h:;
//To Here Feb. 7, 2001

	char	szPath[_MAX_PATH];
	char	szDrive[_MAX_DRIVE];
	char	szDir[_MAX_DIR];
	char	szFname[_MAX_FNAME];
	char	szExt[_MAX_EXT];
	HWND	hwndOwner;

	_splitpath( m_pcEditDoc->m_szFilePath, szDrive, szDir, szFname, szExt );
//Feb. 7, 2001 JEPRO 原作版をコメントアウト
//	_makepath( szPath, szDrive, szDir, szFname, "h" );
//	if( -1 == _access( (const char *)szPath, 0 ) ){
//		if( !bCheckOnly ){
//			::MessageBeep( MB_ICONHAND );
//		}
//		return FALSE;
//	}
//	if( bCheckOnly ){
//		return TRUE;
//	}

//From Here Feb. 7, 2001 JEPRO 追加
	for( i = 0; i < hdr_extno; i++ ){
		_makepath( szPath, szDrive, szDir, szFname, header_ext[i] );
		if( -1 == _access( (const char *)szPath, 0 ) ){
			if( i < hdr_extno - 1 )
				continue;
			if( !bCheckOnly ){
				::MessageBeep( MB_ICONHAND );
			}
			return FALSE;
		}
		break;
	}
	if( bCheckOnly ){
		return TRUE;
	}
//To Here Feb. 7, 2001

	/* 指定ファイルが開かれているか調べる */
	/* 開かれている場合は開いているウィンドウのハンドルも返す */
	/* ファイルを開いているか */
	if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
	}else{
		/* 新しく開く */
		char	szPath2[_MAX_PATH + 3];
		if( strchr( szPath, ' ' ) ){
			wsprintf( szPath2, "\"%s\"", szPath );
		}else{
			strcpy( szPath2, szPath );
		}
		/* 文字コードはこのファイルに合わせる */
		CEditApp::OpenNewEditor(
			m_hInstance,
			m_pShareData->m_hwndTray,
			szPath2,
			m_pcEditDoc->m_nCharCode,
			FALSE,	/* 読み取り専用か */
			true
		);
		/* ファイルを開いているか */
		if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
		}else{
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//				"インクルードファイルのオープンに失敗しました。\n\n%s\n\n", szPath	//Feb. 10, 2001 jepro メッセージを若干変更
				"ヘッダファイルのオープンに失敗しました。\n\n%s\n\n", szPath
			);
			return FALSE;
		}
	}
	/* アクティブにする */
	ActivateFrameWindow( hwndOwner );
	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	POINT	poCaret;
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		(int*)&poCaret.x,
		(int*)&poCaret.y
	);
	/* タグジャンプ元通知 */
	memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof( poCaret ) );
	::SendMessage( hwndOwner, MYWM_SETREFERER, (WPARAM)(m_pcEditDoc->m_hwndParent), 0 );
	return TRUE;
}




/* C/C++ソースファイル オープン機能 */
//BOOL CEditView::Command_OPENCCPP( BOOL bCheckOnly )	//Feb. 10, 2001 JEPRO	コマンド名を若干変更
BOOL CEditView::Command_OPEN_CCPP( BOOL bCheckOnly )
{
//From Here Feb. 7, 2001 JEPRO 追加
	static char* source_ext[] = { "c", "cpp", "cxx", "cc", "cp", "c++" };
	static char* header_ext[] = { "h", "hpp", "hxx", "hh", "hp", "h++" };
	int		src_extno = 6;
	int		hdr_extno = 6;
	int		i;
	BOOL	bwantopen_c;
//To Here Feb. 7, 2001

	/* 編集中ファイルの拡張子を調べる */
//Feb. 7, 2001 JEPRO 原作版をコメントアウト
//	if( CheckEXT( m_pcEditDoc->m_szFilePath, "h" ) ){
//	}else{
//		if( !bCheckOnly ){
//			::MessageBeep( MB_ICONHAND );
//		}
//		return FALSE;
//	}

//From Here Feb. 7, 2001 JEPRO 追加
	for( i = 0; i < hdr_extno; i++ ){
		if( CheckEXT( m_pcEditDoc->m_szFilePath, header_ext[i] ) ){
			bwantopen_c = TRUE;
			goto open_c;
		}
	}
	if( !bCheckOnly ){
		::MessageBeep( MB_ICONHAND );
	}
	return FALSE;

open_c:;
//To Here Feb. 7, 2001

	char	szPath[_MAX_PATH];
	char	szDrive[_MAX_DRIVE];
	char	szDir[_MAX_DIR];
	char	szFname[_MAX_FNAME];
	char	szExt[_MAX_EXT];
	HWND	hwndOwner;

	_splitpath( m_pcEditDoc->m_szFilePath, szDrive, szDir, szFname, szExt );
//Feb. 7, 2001 JEPRO 原作版をコメントアウト
//	_makepath( szPath, szDrive, szDir, szFname, "c" );
//	if( -1 == _access( (const char *)szPath, 0 ) ){
//		_makepath( szPath, szDrive, szDir, szFname, "cpp" );
//		if( -1 == _access( (const char *)szPath, 0 ) ){
//			_makepath( szPath, szDrive, szDir, szFname, "cxx" );
//			if( -1 == _access( (const char *)szPath, 0 ) ){
//				if( !bCheckOnly ){
//					::MessageBeep( MB_ICONHAND );
//				}
//				return FALSE;
//			}
//		}
//	}
//	if( bCheckOnly ){
//		return TRUE;
//	}

//From Here Feb. 7, 2001 JEPRO 追加
	for( i = 0; i < src_extno; i++ ){
		_makepath( szPath, szDrive, szDir, szFname, source_ext[i] );
		if( -1 == _access( (const char *)szPath, 0 ) ){
			if( i < src_extno - 1 )
				continue;
			if( !bCheckOnly ){
				::MessageBeep( MB_ICONHAND );
			}
			return FALSE;
		}
		break;
	}
	if( bCheckOnly ){
		return TRUE;
	}
//To Here Feb. 7, 2001

	/* 指定ファイルが開かれているか調べる */
	/* 開かれている場合は開いているウィンドウのハンドルも返す */
	/* ファイルを開いているか */
	if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
	}else{
		/* 新しく開く */
		char	szPath2[_MAX_PATH + 3];
		if( strchr( szPath, ' ' ) ){
			wsprintf( szPath2, "\"%s\"", szPath );
		}else{
			strcpy( szPath2, szPath );
		}
		/* 文字コードはこのファイルに合わせる */
		CEditApp::OpenNewEditor(
			m_hInstance,
			m_pShareData->m_hwndTray,
			szPath2,
			m_pcEditDoc->m_nCharCode,
			FALSE,	/* 読み取り専用か */
			true
		);
		/* ファイルを開いているか */
		if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
		}else{
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
				"C/C++ソースファイルのオープンに失敗しました。\n\n%s\n\n", szPath
			);
			return FALSE;
		}
	}
	/* アクティブにする */
	ActivateFrameWindow( hwndOwner );
	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	POINT	poCaret;
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		(int*)&poCaret.x,
		(int*)&poCaret.y
	);
	/* タグジャンプ元通知 */
	memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof( poCaret ) );
	::SendMessage( hwndOwner, MYWM_SETREFERER, (WPARAM)(m_pcEditDoc->m_hwndParent), 0 );
	return TRUE;
}




//From Here Feb. 10, 2001 JEPRO 追加
/* C/C++ヘッダファイルまたはソースファイル オープン機能 */
BOOL CEditView::Command_OPEN_HfromtoC( BOOL bCheckOnly )
{
	static char* source_ext[] = { "c", "cpp", "cxx", "cc", "cp", "c++" };
	static char* header_ext[] = { "h", "hpp", "hxx", "hh", "hp", "h++" };
	int		src_extno = 6;
	int		hdr_extno = 6;
	int		i;
	BOOL	bwantopen_h;

	/* 編集中ファイルの拡張子を調べる */
//Feb. 8, 2001 JEPRO VC++で使用される拡張子のみ対応(初期バージョンなのでコメントアウト)
//	if( CheckEXT( m_pcEditDoc->m_szFilePath, "cpp" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "cxx" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "c" ) ){
//		bopen_h = TRUE;
//	}else if( CheckEXT( m_pcEditDoc->m_szFilePath, "h" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "hpp" ) ||
//		CheckEXT( m_pcEditDoc->m_szFilePath, "hxx" ) ){
//		bopen_h = FALSE;
//	}else{
//		if( !bCheckOnly ){
//			::MessageBeep( MB_ICONHAND );
//		}
//		return FALSE;
//	}

	for( i = 0; i < src_extno; i++ ){
		if( CheckEXT( m_pcEditDoc->m_szFilePath, source_ext[i] ) ){
			bwantopen_h = TRUE;
			goto open_hc;
		}
	}
	for( i = 0; i < hdr_extno; i++ ){
		if( CheckEXT( m_pcEditDoc->m_szFilePath, header_ext[i] ) ){
			bwantopen_h = FALSE;
			goto open_hc;
		}
	}
	if( !bCheckOnly ){
		::MessageBeep( MB_ICONHAND );
	}
	return FALSE;

open_hc:;

	char	szPath[_MAX_PATH];
	char	szDrive[_MAX_DRIVE];
	char	szDir[_MAX_DIR];
	char	szFname[_MAX_FNAME];
	char	szExt[_MAX_EXT];
	HWND	hwndOwner;

	_splitpath( m_pcEditDoc->m_szFilePath, szDrive, szDir, szFname, szExt );
//Feb. 8, 2001 JEPRO VC++で使用される拡張子のみ対応(初期バージョンなのでコメントアウト)
//	if( TRUE == bwantopen_h ){
//		_makepath( szPath, szDrive, szDir, szFname, "h" );
//		if( -1 == _access( (const char *)szPath, 0 ) ){
//			_makepath( szPath, szDrive, szDir, szFname, "hpp" );
//			if( -1 == _access( (const char *)szPath, 0 ) ){
//				_makepath( szPath, szDrive, szDir, szFname, "hxx" );
//				if( -1 == _access( (const char *)szPath, 0 ) ){
//					if( !bCheckOnly ){
//						::MessageBeep( MB_ICONHAND );
//					}
//					return FALSE;
//				}
//			}
//		}
//	}else{
//		_makepath( szPath, szDrive, szDir, szFname, "c" );
//		if( -1 == _access( (const char *)szPath, 0 ) ){
//			_makepath( szPath, szDrive, szDir, szFname, "cpp" );
//			if( -1 == _access( (const char *)szPath, 0 ) ){
//				_makepath( szPath, szDrive, szDir, szFname, "cxx" );
//				if( -1 == _access( (const char *)szPath, 0 ) ){
//					if( !bCheckOnly ){
//						::MessageBeep( MB_ICONHAND );
//					}
//					return FALSE;
//				}
//			}
//		}
//	}
//	if( bCheckOnly ){
//		return TRUE;
//	}

//From Here Feb. 10, 2001 JEPRO 追加
	if( TRUE == bwantopen_h ){
		for( i = 0; i < hdr_extno; i++ ){
			_makepath( szPath, szDrive, szDir, szFname, header_ext[i] );
			if( -1 == _access( (const char *)szPath, 0 ) ){
				if( i < hdr_extno - 1 )
					continue;
				if( !bCheckOnly ){
					::MessageBeep( MB_ICONHAND );
				}
				return FALSE;
			}
			break;
		}
	}else{
		for( i = 0; i < src_extno; i++ ){
			_makepath( szPath, szDrive, szDir, szFname, source_ext[i] );
			if( -1 == _access( (const char *)szPath, 0 ) ){
				if( i < src_extno - 1 )
					continue;
				if( !bCheckOnly ){
					::MessageBeep( MB_ICONHAND );
				}
				return FALSE;
			}
			break;
		}
	}
	if( bCheckOnly ){
		return TRUE;
	}
//To Here Feb. 10, 2001

	/* 指定ファイルが開かれているか調べる */
	/* 開かれている場合は開いているウィンドウのハンドルも返す */
	/* ファイルを開いているか */
	if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
	}else{
		/* 新しく開く */
		char	szPath2[_MAX_PATH + 3];
		if( strchr( szPath, ' ' ) ){
			wsprintf( szPath2, "\"%s\"", szPath );
		}else{
			strcpy( szPath2, szPath );
		}
		/* 文字コードはこのファイルに合わせる */
		CEditApp::OpenNewEditor(
			m_hInstance,
			m_pShareData->m_hwndTray,
			szPath2,
			m_pcEditDoc->m_nCharCode,
			FALSE,	/* 読み取り専用か */
			true
		);
		/* ファイルを開いているか */
		if( m_cShareData.IsPathOpened( (const char*)szPath, &hwndOwner ) ){
		}else{
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
				(TRUE == bwantopen_h) ? "C/C++ヘッダファイルのオープンに失敗しました。\n\n%s\n\n"
									:	"C/C++ソースファイルのオープンに失敗しました。\n\n%s\n\n", szPath
			);
			return FALSE;
		}
	}
	/* アクティブにする */
	ActivateFrameWindow( hwndOwner );
	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	POINT	poCaret;
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		(int*)&poCaret.x,
		(int*)&poCaret.y
	);
	/* タグジャンプ元通知 */
	memcpy( m_pShareData->m_szWork, (void*)&poCaret, sizeof( poCaret ) );
	::SendMessage( hwndOwner, MYWM_SETREFERER, (WPARAM)(m_pcEditDoc->m_hwndParent), 0 );
	return TRUE;
}




//重ねて表示
void CEditView::Command_CASCADE( void )
{
	EditNode*	pEditNodeArr;
	HWND*		phwndArr;
	int			nRowNum;
	int			i;
	/* 現在開いている編集窓のリストをメニューにする */
	nRowNum = m_cShareData.GetOpenedWindowArr( &pEditNodeArr, TRUE/*FALSE*/ );
	if( nRowNum > 0 ){
		phwndArr = new HWND[nRowNum];
		for( i = 0; i < nRowNum; ++i ){
			phwndArr[i] = pEditNodeArr[i].m_hWnd;
			if( ::IsZoomed( phwndArr[i] ) ){
				::ShowWindow( phwndArr[i], SW_RESTORE );
			}
		}
		::CascadeWindows( NULL, MDITILE_SKIPDISABLED, NULL, nRowNum, phwndArr );
		delete [] phwndArr;
		delete [] pEditNodeArr;
	}
	return;
}




//左右に並べて表示
void CEditView::Command_TILE_H( void )
{
	EditNode*	pEditNodeArr;
	HWND*		phwndArr;
	int			nRowNum;
	int			i;
	/* 現在開いている編集窓のリストをメニューにする */
	nRowNum = m_cShareData.GetOpenedWindowArr( &pEditNodeArr, TRUE/*FALSE*/ );
	if( nRowNum > 0 ){
		phwndArr = new HWND[nRowNum];
		for( i = 0; i < nRowNum; ++i ){
			phwndArr[i] = pEditNodeArr[i].m_hWnd;
			if( ::IsZoomed( phwndArr[i] ) ){
				::ShowWindow( phwndArr[i], SW_RESTORE );
			}
		}
		::TileWindows( NULL, MDITILE_VERTICAL, NULL, nRowNum, phwndArr );
		delete [] phwndArr;
		delete [] pEditNodeArr;
	}
	return;
}




//上下に並べて表示
void CEditView::Command_TILE_V( void )
{
	EditNode*	pEditNodeArr;
	HWND*		phwndArr;
	int			nRowNum;
	int			i;
	/* 現在開いている編集窓のリストをメニューにする */
	nRowNum = m_cShareData.GetOpenedWindowArr( &pEditNodeArr, TRUE/*FALSE*/ );
	if( nRowNum > 0 ){
		phwndArr = new HWND[nRowNum];
		for( i = 0;i < nRowNum; ++i ){
			phwndArr[i] = pEditNodeArr[i].m_hWnd;
			if( ::IsZoomed( phwndArr[i] ) ){
				::ShowWindow( phwndArr[i], SW_RESTORE );
			}
		}
		::TileWindows( NULL, MDITILE_HORIZONTAL, NULL, nRowNum, phwndArr );
		delete [] phwndArr;
		delete [] pEditNodeArr;
	}
	return;
}




//縦方向に最大化
void CEditView::Command_MAXIMIZE_V( void )
{
	HWND	hwndFrame;
	RECT	rcOrg;
	RECT	rcDesktop;
	hwndFrame = ::GetParent( m_hwndParent );
	::GetWindowRect( hwndFrame, &rcOrg );
	::SystemParametersInfo( SPI_GETWORKAREA, NULL, &rcDesktop, 0 );
	::SetWindowPos(
		hwndFrame, 0,
		rcOrg.left, rcDesktop.top,
		rcOrg.right - rcOrg.left, rcDesktop.bottom - rcDesktop.top,
		SWP_NOOWNERZORDER | SWP_NOZORDER
	);
	return;
}




//2001.02.10 Start by MIK: 横方向に最大化
//横方向に最大化
void CEditView::Command_MAXIMIZE_H( void )
{
	HWND	hwndFrame;
	RECT	rcOrg;
	RECT	rcDesktop;

	hwndFrame = ::GetParent( m_hwndParent );
	::GetWindowRect( hwndFrame, &rcOrg );
	::SystemParametersInfo( SPI_GETWORKAREA, NULL, &rcDesktop, 0 );
	::SetWindowPos(
		hwndFrame, 0,
		rcDesktop.left, rcOrg.top,
		rcDesktop.right - rcDesktop.left, rcOrg.bottom - rcOrg.top,
		SWP_NOOWNERZORDER | SWP_NOZORDER
	);
	return;
}
//2001.02.10 End: 横方向に最大化




/* すべて最小化 */	//	Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
void CEditView::Command_MINIMIZE_ALL( void )
{
	HWND*	phWndArr;
	int		i;
	int		j;
	j = m_pShareData->m_nEditArrNum;
	if( 0 == j ){
		return;
	}
	phWndArr = new HWND[j];
	for( i = 0; i < j; ++i ){
		phWndArr[i] = m_pShareData->m_pEditArr[i].m_hWnd;
	}
	for( i = 0; i < j; ++i ){
		if( CShareData::IsEditWnd( phWndArr[i] ) ){
			::ShowWindow( phWndArr[i], SW_MINIMIZE );
		}
	}
	delete [] phWndArr;
	return;
}




//置換(置換ダイアログ)
void CEditView::Command_REPLACE( void )
{
//	int			nRet;
	CMemory		cmemCurText;
	const char*	pLine;
	int			nLineLen;
	int			nLineFrom;
	int			nColmFrom;
	int			nLineTo;
	int			nColmTo;
	int			nIdx;
	int			i;
	BOOL		bSelected = FALSE;
	const CLayout*	pcLayout;
	/* 検索文字列を初期化 */
	m_pcEditDoc->m_cDlgReplace.m_szText[0] = '\0';
	if( IsTextSelected() ){	/* テキストが選択されているか */
		/* 選択範囲のデータを取得 */
		if( GetSelectedData( cmemCurText, FALSE, NULL, FALSE ) ){
			/* 検索文字列を現在位置の単語で初期化 */
			strncpy( m_pcEditDoc->m_cDlgReplace.m_szText, cmemCurText.GetPtr( NULL ), _MAX_PATH - 1 );

			m_pcEditDoc->m_cDlgReplace.m_szText[_MAX_PATH - 1] = '\0';
		}
		/* 矩形範囲選択中か */
		if( m_bBeginBoxSelect ){
			/* 現在の選択範囲を非選択状態に戻す */
			DisableSelectArea( TRUE );
			bSelected = FALSE;
		}else{
			bSelected = TRUE;
		}
	}else{
		pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr2( m_nCaretPosY, &nLineLen, &pcLayout );
		if( NULL != pLine ){
			/* 指定された桁に対応する行のデータ内の位置を調べる */
			nIdx = LineColmnToIndex( pLine, nLineLen, m_nCaretPosX );

			/* 現在位置の単語の範囲を調べる */
			if( m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
				m_nCaretPosY, nIdx,
				&nLineFrom, &nColmFrom, &nLineTo, &nColmTo, NULL, NULL ) ){

				/* 指定された行のデータ内の位置に対応する桁の位置を調べる */
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineFrom, &nLineLen );
				nColmFrom = LineIndexToColmn( pLine, nLineLen, nColmFrom );
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( nLineTo, &nLineLen );
				nColmTo = LineIndexToColmn( pLine, nLineLen, nColmTo );

				/* 選択範囲の変更 */
//				m_nSelectLineBgn = nLineFrom;		/* 範囲選択開始行(原点) */
//				m_nSelectColmBgn = nColmFrom;		/* 範囲選択開始桁(原点) */
				m_nSelectLineBgnFrom = nLineFrom;	/* 範囲選択開始行(原点) */
				m_nSelectColmBgnFrom = nColmFrom;	/* 範囲選択開始桁(原点) */
				m_nSelectLineBgnTo = nLineFrom;		/* 範囲選択開始行(原点) */
				m_nSelectColmBgnTo = nColmFrom;		/* 範囲選択開始桁(原点) */
				m_nSelectLineFrom =	nLineFrom;
				m_nSelectColmFrom = nColmFrom;
				m_nSelectLineTo = nLineTo;
				m_nSelectColmTo = nColmTo;

				/* 選択範囲のデータを取得 */
				if( GetSelectedData( cmemCurText, FALSE, NULL, FALSE ) ){
					/* 検索文字列を現在位置の単語で初期化 */
					strncpy( m_pcEditDoc->m_cDlgReplace.m_szText, cmemCurText.GetPtr( NULL ), MAX_PATH - 1 );
					m_pcEditDoc->m_cDlgReplace.m_szText[MAX_PATH - 1] = '\0';
				}
				/* 現在の選択範囲を非選択状態に戻す */
				DisableSelectArea( FALSE );
			}
		}
	}
	/* 検索文字列は改行まで */
	for( i = 0; i < (int)lstrlen( m_pcEditDoc->m_cDlgReplace.m_szText ); ++i ){
		if( m_pcEditDoc->m_cDlgReplace.m_szText[i] == CR ||
			m_pcEditDoc->m_cDlgReplace.m_szText[i] == LF ){
			m_pcEditDoc->m_cDlgReplace.m_szText[i] = '\0';
			break;
		}
	}
	/* 置換ダイアログの表示 */
//	nRet = m_pcEditDoc->m_cDlgReplace.DoModal( (LPARAM)this, bSelected );
//	MYTRACE( "nRet=%d\n", nRet );
	//	From Here Jul. 2, 2001 genta 置換ウィンドウの2重開きを抑止
	if( !::IsWindow( m_pcEditDoc->m_cDlgReplace.m_hWnd ) ){
		m_pcEditDoc->m_cDlgReplace.DoModeless( m_hInstance, m_hWnd/*::GetParent( m_hwndParent )*/, (LPARAM)this, bSelected );
	}
	else {
			/* アクティブにする */
		ActivateFrameWindow( m_pcEditDoc->m_cDlgReplace.m_hWnd );
		::SetDlgItemText( m_pcEditDoc->m_cDlgReplace.m_hWnd, IDC_COMBO_TEXT, cmemCurText.GetPtr( NULL ) );
	}
	//	To Here Jul. 2, 2001 genta 置換ウィンドウの2重開きを抑止
	return;
}




//	/* 単語リファレンス*/
//	void CEditView::Command_WORDSREFERENCE( void )
//	{
//	int				nNewLine;		/* 挿入された部分の次の位置の行 */
//	int				nNewPos;		/* 挿入された部分の次の位置のデータ位置 */
//	COpe*			pcOpe = NULL;
//	CWaitCursor		cWaitCursor;
//	CDlgWords		cDlgWords;
//
//	/* 初期化 */
//	cDlgWords.Create( m_hInstance, m_hWnd, (void*)m_pcEditDoc );
//
//	/* モーダルダイアログの表示 */
//	if( FALSE == cDlgWords.DoModal() ){
//		return;
//	}
//
//
//	/* テキストが選択されているか */
//	if( IsTextSelected() ){
//		DeleteData( TRUE );
//	}
//
//	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe = new COpe;
//		pcOpe->m_nCaretPosX_Before = m_nCaretPosX;			/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_Before = m_nCaretPosY;			/* 操作前のキャレット位置Ｙ */
//		pcOpe->m_nCaretPosX_PHY_Before = m_nCaretPosX_PHY;	/* 操作前のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_PHY_Before = m_nCaretPosY_PHY;	/* 操作前のキャレット位置Ｙ */
//	}
//
//	/* 現在位置にデータを挿入 */
//	InsertData_CEditView( m_nCaretPosX, m_nCaretPosY, cDlgWords.m_szWord, lstrlen( cDlgWords.m_szWord ), &nNewLine, &nNewPos, pcOpe, TRUE );
//
//	/* 挿入データの最後へカーソルを移動 */
//	MoveCursor( nNewPos, nNewLine, TRUE );
//	m_nCaretPosX_Prev = m_nCaretPosX;
//
//	if( !m_bDoing_UndoRedo ){	/* アンドゥ・リドゥの実行中か */
//		pcOpe->m_nCaretPosX_After = m_nCaretPosX;			/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_After = m_nCaretPosY;			/* 操作後のキャレット位置Ｙ */
//		pcOpe->m_nCaretPosX_PHY_After = m_nCaretPosX_PHY;	/* 操作後のキャレット位置Ｘ */
//		pcOpe->m_nCaretPosY_PHY_After = m_nCaretPosY_PHY;	/* 操作後のキャレット位置Ｙ */
//		/* 操作の追加 */
//		m_pcOpeBlk->AppendOpe( pcOpe );
//	}
//
//	m_pcEditDoc->m_bIsModified = TRUE;	/* 変更フラグ */
//	SetParentCaption();	/* 親ウィンドウのタイトルを更新 */
//
//	return;
//	}




/* カーソル行をウィンドウ中央へ */
void CEditView::Command_CURLINECENTER( void )
{
	int		nViewTopLine;
	nViewTopLine = m_nCaretPosY - ( m_nViewRowNum / 2 );
	if( 0 <= nViewTopLine ){
		m_nViewTopLine = nViewTopLine;
		/* フォーカス移動時の再描画 */
		RedrawAll();
	}
	return;
}




/* Base64デコードして保存 */
void CEditView::Command_BASE64DECODE( void )
{
	CMemory		cmemBuf;
	char		szPath[_MAX_PATH];
	HFILE		hFile;

	/* テキストが選択されているか */
	if( !IsTextSelected() ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
	if( FALSE == GetSelectedData( cmemBuf, FALSE, NULL, FALSE ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* Base64デコード */
	cmemBuf.BASE64Decode();
	strcpy( szPath, "" );

	/* 保存ダイアログ モーダルダイアログの表示 */
	if( !m_pcEditDoc->SaveFileDialog( (char*)szPath,  NULL ) ){
		return;
	}
	if(HFILE_ERROR == (hFile = _lcreat( szPath, 0 ) ) ){
		::MessageBeep( MB_ICONHAND );
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルの作成に失敗しました。\n\n%s", szPath
		);
		return;
	}
	if( HFILE_ERROR == _lwrite( hFile, cmemBuf.GetPtr( NULL ), cmemBuf.GetLength() ) ){
		::MessageBeep( MB_ICONHAND );
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルの書き込みに失敗しました。\n\n%s", szPath
		);
	}
	_lclose( hFile );
	return;
}




/* uudecodeして保存 */
void CEditView::Command_UUDECODE( void )
{
	CMemory		cmemBuf;
	char		szPath[_MAX_PATH];
	HFILE		hFile;
	/* テキストが選択されているか */
	if( !IsTextSelected() ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	/* 選択範囲のデータを取得 */
	/* 正常時はTRUE,範囲未選択の場合はFALSEを返す */
	if( FALSE == GetSelectedData( cmemBuf, FALSE, NULL, FALSE ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	strcpy( szPath, "" );

	/* uudecode(デコード) */	//Oct. 17, 2000 jepro 説明を「UUENCODE復号化(デコード) 」から変更
	cmemBuf.UUDECODE( szPath );

	/* 保存ダイアログ モーダルダイアログの表示 */
	if( !m_pcEditDoc->SaveFileDialog( (char*)szPath,  NULL ) ){
		return;
	}
	if(HFILE_ERROR == (hFile = _lcreat( szPath, 0 ) ) ){
		::MessageBeep( MB_ICONHAND );
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルの作成に失敗しました。\n\n%s", szPath
		);
		return;
	}
	if( HFILE_ERROR == _lwrite( hFile, cmemBuf.GetPtr( NULL ), cmemBuf.GetLength() ) ){
		::MessageBeep( MB_ICONHAND );
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"ファイルの書き込みに失敗しました。\n\n%s", szPath
		);
	}
	_lclose( hFile );
	return;
}




/* 再描画 */
void CEditView::Command_REDRAW( void )
{
	/* フォーカス移動時の再描画 */
	RedrawAll();
	return;
}




/* Oracle SQL*Plusで実行 */
void CEditView::Command_PLSQL_COMPILE_ON_SQLPLUS( void )
{
//	HGLOBAL		hgClip;
//	char*		pszClip;
	HWND		hwndSQLPLUS;
	int			nRet;
	BOOL		nBool;
	char		szPath[MAX_PATH + 2];
	int			i;
	BOOL		bSPACE;
	BOOL		bResult;
	DWORD		dwResult;
	hwndSQLPLUS = ::FindWindow( "SqlplusWClass", "Oracle SQL*Plus" );
	if( NULL == hwndSQLPLUS ){
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"Oracle SQL*Plusで実行します。\n\n\nOracle SQL*Plusが起動されていません。\n"
		);
		return;
	}
	/* テキストが変更されている場合 */
	if( m_pcEditDoc->m_bIsModified ){
		nRet = ::MYMESSAGEBOX(
			m_hWnd,
			MB_YESNOCANCEL | MB_ICONEXCLAMATION,
			GSTR_APPNAME,
			"%s\nは変更されています。 Oracle SQL*Plusで実行する前に保存しますか？",
			lstrlen( m_pcEditDoc->m_szFilePath ) ? m_pcEditDoc->m_szFilePath : "(無題)"
		);
		switch( nRet ){
		case IDYES:
			if( 0 < lstrlen( m_pcEditDoc->m_szFilePath ) ){
				nBool = HandleCommand( F_FILESAVE, TRUE, 0, 0, 0, 0 );
			}else{
				nBool = HandleCommand( F_FILESAVEAS, TRUE, 0, 0, 0, 0 );
			}
			if( FALSE == nBool ){
				return;
			}
			break;
		case IDNO:
			return;
		case IDCANCEL:
		default:
			return;
		}
	}
	if( 0 < lstrlen( m_pcEditDoc->m_szFilePath ) ){
		/* ファイルパスに空白が含まれている場合はダブルクォーテーションで囲む */
		bSPACE = FALSE;
		for( i = 0; i < (int)lstrlen( m_pcEditDoc->m_szFilePath ); ++i ){
			if( m_pcEditDoc->m_szFilePath[i] == ' ' ){
				bSPACE = TRUE;
				break;
			}
		}
		if( bSPACE ){
			wsprintf( szPath, "@\"%s\"\r", m_pcEditDoc->m_szFilePath );
		}else{
			wsprintf( szPath, "@%s\r", m_pcEditDoc->m_szFilePath );
		}
		/* クリップボードにデータを設定 */
		MySetClipboardData( szPath, lstrlen( szPath ), FALSE );

		/* Oracle SQL*Plusをアクティブにする */
		/* アクティブにする */
		ActivateFrameWindow( hwndSQLPLUS );

		/* Oracle SQL*Plusにペーストのコマンドを送る */
		bResult = ::SendMessageTimeout(
			hwndSQLPLUS, WM_COMMAND, MAKELONG( 201, 0 ), 0,
			SMTO_ABORTIFHUNG | SMTO_NORMAL,
			3000,
			&dwResult
		);
		if( !bResult ){
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_TOPMOST | MB_ICONSTOP, GSTR_APPNAME,
				"Oracle SQL*Plusからの反応がありません。\nしばらく待ってから再び実行してください。"
			);
		}
	}else{
		::MessageBeep( MB_ICONHAND );
		::MYMESSAGEBOX( m_hWnd,
			 MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"SQLをファイルに保存しないとOracle SQL*Plusで実行できません。\n"
		);
		return;
	}
	return;
}




/* Oracle SQL*Plusをアクティブ表示 */
void CEditView::Command_ACTIVATE_SQLPLUS( void )
{
	HWND		hwndSQLPLUS;
	hwndSQLPLUS = ::FindWindow( "SqlplusWClass", "Oracle SQL*Plus" );
	if( NULL == hwndSQLPLUS ){
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
			"Oracle SQL*Plusをアクティブ表示します。\n\n\nOracle SQL*Plusが起動されていません。\n"
		);
		return;
	}
	/* Oracle SQL*Plusをアクティブにする */
	/* アクティブにする */
	ActivateFrameWindow( hwndSQLPLUS );
	return;
}




/* ファイルのプロパティ */
void CEditView::Command_PROPERTY_FILE( void )
{
#ifdef _DEBUG
	{
		/* 全行データを返すテスト */
		char*	pDataAll;
		int		nDataAllLen;
		CRunningTimer cRunningTimer( "CEditView::Command_PROPERTY_FILE 全行データを返すテスト" );
		cRunningTimer.Reset();
		pDataAll = m_pcEditDoc->m_cDocLineMgr.GetAllData( &nDataAllLen );
//		MYTRACE( "全データ取得             (%dバイト) 所要時間(ミリ秒) = %d\n", nDataAllLen, cRunningTimer.Read() );
		free( pDataAll );
		pDataAll = NULL;
//		MYTRACE( "全データ取得のメモリ開放 (%dバイト) 所要時間(ミリ秒) = %d\n", nDataAllLen, cRunningTimer.Read() );
	}
#endif


	CDlgProperty	cDlgProperty;
//	cDlgProperty.Create( m_hInstance, m_hWnd, (void *)m_pcEditDoc );
	cDlgProperty.DoModal( m_hInstance, m_hWnd, (LPARAM)m_pcEditDoc );
	return;
}




/* サクラエディタの全終了 */	//Dec. 27, 2000 JEPRO 追加
void CEditView::Command_EXITALL( void )
{
	CEditApp::TerminateApplication();
	return;
}




/* すべてのウィンドウを閉じる */	//Oct. 7, 2000 jepro 「編集ウィンドウの全終了」という説明を左記のように変更
void CEditView::Command_FILECLOSEALL( void )
{
	CEditApp::CloseAllEditor();
	return;
}




/* ウィンドウを閉じる */
void CEditView::Command_WINCLOSE( void )
{
	/* 閉じる */
	::PostMessage( ::GetParent( m_hwndParent ), WM_CLOSE, 0, 0 );
	return;
}

//アウトプットウィンドウ表示
void CEditView::Command_WIN_OUTPUT( void )
{
	if( NULL == m_pShareData->m_hwndDebug
		|| !CShareData::IsEditWnd( m_pShareData->m_hwndDebug )
	){
		CEditApp::OpenNewEditor( NULL, NULL, "-DEBUGMODE", CODE_SJIS, FALSE, true );
#if 0
		//	Jun. 25, 2001 genta OpenNewEditorのsync機能を利用するように変更
		//アウトプットウインドウが出来るまで5秒ぐらい待つ。
		CRunningTimer wait_timer( NULL );
		while( NULL == m_pShareData->m_hwndDebug && 5000 > wait_timer.Read() ){
			Sleep(1);
		}
		Sleep(10);
#endif
	}else{
		/* 開いているウィンドウをアクティブにする */
		/* アクティブにする */
		ActivateFrameWindow( m_pShareData->m_hwndDebug );
	}
	return;
}




/* カスタムメニュー表示 */
int CEditView::Command_CUSTMENU( int nMenuIdx )
{
	HMENU		hMenu;
	int			nId;
	POINT		po;
	int			i;
	char		szLabel[300];
	char		szLabel2[300];
	UINT		uFlags;
//	BOOL		bBool;

	CEditWnd*	pCEditWnd;
	pCEditWnd = ( CEditWnd* )::GetWindowLong( ::GetParent( m_hwndParent ), GWL_USERDATA );
	pCEditWnd->m_CMenuDrawer.ResetContents();

	if( nMenuIdx < 0 || MAX_CUSTOM_MENU <= nMenuIdx ){
		return 0;
	}
	if( 0 == m_pShareData->m_Common.m_nCustMenuItemNumArr[nMenuIdx] ){
		return 0;
	}
	hMenu = ::CreatePopupMenu();
	for( i = 0; i < m_pShareData->m_Common.m_nCustMenuItemNumArr[nMenuIdx]; ++i ){
		if( 0 == m_pShareData->m_Common.m_nCustMenuItemFuncArr[nMenuIdx][i] ){
			::AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
		}else{
			::LoadString( m_hInstance, m_pShareData->m_Common.m_nCustMenuItemFuncArr[nMenuIdx][i], szLabel, 256 );
			/* キー */
			if( '\0' == m_pShareData->m_Common.m_nCustMenuItemKeyArr[nMenuIdx][i] ){
				strcpy( szLabel2, szLabel );
			}else{
				wsprintf( szLabel2, "%s (&%c)", szLabel, m_pShareData->m_Common.m_nCustMenuItemKeyArr[nMenuIdx][i] );
			}
			/* 機能が利用可能か調べる */
			if( TRUE == CEditWnd::IsFuncEnable( m_pcEditDoc, m_pShareData, m_pShareData->m_Common.m_nCustMenuItemFuncArr[nMenuIdx][i] ) ){
				uFlags = MF_STRING | MF_ENABLED;
			}else{
				uFlags = MF_STRING | MF_DISABLED | MF_GRAYED;
			}
//			bBool = ::AppendMenu( hMenu, uFlags, m_pShareData->m_Common.m_nCustMenuItemFuncArr[nMenuIdx][i], szLabel2 );
			pCEditWnd->m_CMenuDrawer.MyAppendMenu(
				hMenu, /*MF_BYPOSITION | MF_STRING*/uFlags,
				m_pShareData->m_Common.m_nCustMenuItemFuncArr[nMenuIdx][i] , szLabel2 );
		}
	}
	po.x = 0;
	po.y = 0;
	::ClientToScreen( m_hWnd, &po );
	nId = ::TrackPopupMenu(
		hMenu,
		TPM_TOPALIGN
		| TPM_LEFTALIGN
		| TPM_RETURNCMD
		| TPM_LEFTBUTTON
		,
		po.x,
		po.y,
		0,
		::GetParent( m_hwndParent )/*m_hWnd*/,
		NULL
	);
	::DestroyMenu( hMenu );
	return nId;
}




//選択範囲内全行コピー
void CEditView::Command_COPYLINES( void )
{
	/* 選択範囲内の全行をクリップボードにコピーする */
	CopySelectedAllLines(
		NULL,	/* 引用符 */
		FALSE	/* 行番号を付与する */
	);
	return;
}




//選択範囲内全行引用符付きコピー
void CEditView::Command_COPYLINESASPASSAGE( void )
{
	/* 選択範囲内の全行をクリップボードにコピーする */
	CopySelectedAllLines(
		m_pShareData->m_Common.m_szInyouKigou,	/* 引用符 */
		FALSE 									/* 行番号を付与する */
	);
	return;
}




//選択範囲内全行行番号付きコピー
void CEditView::Command_COPYLINESWITHLINENUMBER( void )
{
	/* 選択範囲内の全行をクリップボードにコピーする */
	CopySelectedAllLines(
		NULL,	/* 引用符 */
		TRUE	/* 行番号を付与する */
	);
	return;
}




////キー割り当て一覧をコピー
	//Dec. 26, 2000 JEPRO //Jan. 24, 2001 JEPRO debug version (directed by genta)
void CEditView::Command_CREATEKEYBINDLIST( void )
{
	CMemory		cMemKeyList;
	HGLOBAL		hgClip;
	char*		pszClip;

	CKeyBind::CreateKeyBindList(
	m_hInstance,
	m_pShareData->m_nKeyNameArrNum,
	m_pShareData->m_pKeyNameArr,
	cMemKeyList
	 );

	/* Windowsクリップボードにコピー */
	hgClip = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, cMemKeyList.GetLength() + 1 );
	pszClip = (char*)::GlobalLock( hgClip );
	memcpy( pszClip, cMemKeyList.GetPtr( NULL ), cMemKeyList.GetLength() + 1 );
	::GlobalUnlock( hgClip );
	::OpenClipboard( CEditView::m_pcEditDoc->m_hWnd );
	::EmptyClipboard();
	::SetClipboardData( CF_OEMTEXT, hgClip );
	::CloseClipboard();
	return;
}




/* 入力補完 */
void CEditView::Command_HOKAN( void )
{
	CMemory		cmemData;
	POINT		poWin;
	CMemory		cmemHokanWord;
	int			nKouhoNum;
	char*		pszKouhoWord;

retry:;
//	if( 0 == strlen( m_pShareData->m_Common.m_szHokanFile ) ){	// 2001/06/14 asa-o 参照データ変更
	if( 0 == strlen( m_pcEditDoc->GetDocumentAttribute().m_szHokanFile ) ){
		::MessageBeep( MB_ICONHAND );
//	From Here Sept. 15, 2000 JEPRO
//		[Esc]キーと[x]ボタンでも中止できるように変更
//		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
//	To Here Sept. 15, 2000
			"補完候補一覧ファイルが設定されていません。\n今すぐ設定しますか?"
		) ){
//	2001/06/14 Start by asa-o: 開くプロパティシートをタイプ別に変更
			/* 共通設定 プロパティシート */
//			if( !m_pcEditDoc->OpenPropertySheet( ID_PAGENUM_HELPER/*, IDC_EDIT_HOKANFILE*/ ) ){
//				return;
//			}
			/* タイプ別設定 プロパティシート */
			if( !m_pcEditDoc->OpenPropertySheetTypes( 2, m_pcEditDoc->GetDocumentType() ) ){
				return;
			}
//	2001/06/14 End
			goto retry;
		}
	}

	/* カーソル直前の単語を取得 */
	if( 0 < GetLeftWord( &cmemData, 100 ) ){
//		MYTRACE( "cmemData=[%s]\n", cmemData.GetPtr( NULL ) );
		/* 補完対象ワードリストを調べる */
		poWin.x = m_nViewAlignLeft
				 + (m_nCaretPosX - m_nViewLeftCol)
				  * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace );
		poWin.y = m_nViewAlignTop
				 + (m_nCaretPosY - m_nViewTopLine)
				  * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
		::ClientToScreen( m_hWnd, &poWin );
		poWin.x -= (
			cmemData.GetLength()
			 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace )
		);
//		if( 1 < m_pcEditDoc->m_cHokanMgr.Search(
		nKouhoNum = m_pcEditDoc->m_cHokanMgr.Search(
//t			m_hFont_HAN,
			&poWin,
			m_nCharHeight,
			m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColmSpace,
			cmemData.GetPtr( NULL ),
//t			(void*)this,
//			m_pShareData->m_Common.m_szHokanFile	// 2001/06/14 asa-o 参照データ変更
			m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
			// 2001/06/19 asa-o 英大文字小文字を同一視する
			m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase,
			&cmemHokanWord
		);
//		) ){
		if(nKouhoNum > 1){
			m_bHokan = TRUE;
		}else
		// 2001/06/19 asa-o 候補が1つのときはそれに確定する
		if(nKouhoNum == 1){
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
			pszKouhoWord = cmemHokanWord.GetPtr(NULL);
			pszKouhoWord[lstrlen(pszKouhoWord)-1] = '\0';
			Command_WordDeleteToStart();
			Command_INSTEXT( TRUE, (const char*)pszKouhoWord, TRUE );
		}else{
			if( m_bHokan ){
				m_pcEditDoc->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}else{
		::MessageBeep( MB_ICONHAND );
	}
	return;
}




/* ファイル内容比較 */
void CEditView::Command_COMPARE( void )
{
	HWND		hwndCompareWnd;
	char		szPath[_MAX_PATH + 1];
	POINT		poSrc;
	POINT		poDes;
	CDlgCompare	cDlgCompare;
	BOOL		bDefferent;
	const char*	pLineSrc;
	int			nLineLenSrc;
	const char*	pLineDes;
	int			nLineLenDes;
	POINT*		ppoCaretDes;
//	cDlgCompare.Create( m_hInstance, m_hWnd, (void *)m_pcEditDoc );
	/* 比較後、左右に並べて表示 */
	cDlgCompare.m_bCompareAndTileHorz = m_pShareData->m_Common.m_bCompareAndTileHorz;
//	cDlgCompare.m_bCompareAndTileHorz = m_pShareData->m_Common.m_bCompareAndTileHorz;	//Oct. 10, 2000 JEPRO チェックボックスをボタン化すればこの行は不要のはず
//	if( FALSE == cDlgCompare.DoModal( m_pcEditDoc->m_szFilePath, m_pcEditDoc->m_bIsModified, szPath, &hwndCompareWnd ) ){
	if( FALSE == cDlgCompare.DoModal( m_hInstance, m_hWnd, (LPARAM)m_pcEditDoc, m_pcEditDoc->m_szFilePath, m_pcEditDoc->m_bIsModified, szPath, &hwndCompareWnd ) ){
		return;
	}
	/* 比較後、左右に並べて表示 */
	m_pShareData->m_Common.m_bCompareAndTileHorz = cDlgCompare.m_bCompareAndTileHorz;
//	m_pShareData->m_Common.m_bCompareAndTileHorz = cDlgCompare.m_bCompareAndTileHorz;	//Oct. 10, 2000 JEPRO チェックボックスをボタン化すればこの行は不要のはず


	/*
	  カーソル位置変換
	  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
	  →
	  物理位置(行頭からのバイト数、折り返し無し行位置)
	*/
	m_pcEditDoc->m_cLayoutMgr.CaretPos_Log2Phys(
		m_nCaretPosX,
		m_nCaretPosY,
		(int*)&poSrc.x,
		(int*)&poSrc.y
	);
	/* カーソル位置取得要求 */
	::SendMessage( hwndCompareWnd, MYWM_GETCARETPOS, 0, 0 );
	ppoCaretDes = (POINT*)m_pShareData->m_szWork;
	poDes.x = ppoCaretDes->x;
	poDes.y = ppoCaretDes->y;
	bDefferent = TRUE;
	pLineDes = m_pShareData->m_szWork;
	pLineSrc = m_pcEditDoc->m_cDocLineMgr.GetLineStr( poSrc.y, &nLineLenSrc );
	/* 行(改行単位)データの要求 */
	nLineLenDes = ::SendMessage( hwndCompareWnd, MYWM_GETLINEDATA, poDes.y, 0 );
	while( 1 ){
		if( pLineSrc == NULL &&	0 == nLineLenDes ){
			bDefferent = FALSE;
			break;
		}
		if( pLineSrc == NULL || 0 == nLineLenDes ){
			break;
		}
		if( nLineLenDes > sizeof( m_pShareData->m_szWork ) ){
			::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONSTOP | MB_TOPMOST, GSTR_APPNAME,
				"比較先のファイル\n%s\n%dバイトを超える行があります。\n比較できません。", szPath, sizeof( m_pShareData->m_szWork )
			);
			return;
		}
		for( ; poSrc.x < nLineLenSrc; ){
			if( poDes.x >= nLineLenDes ){
				goto end_of_compare;
			}
			if( pLineSrc[poSrc.x] != pLineDes[poDes.x] ){
				goto end_of_compare;
			}
			poSrc.x++;
			poDes.x++;
		}
		if( poDes.x < nLineLenDes ){
			goto end_of_compare;
		}
		poSrc.x = 0;
		poSrc.y++;
		poDes.x = 0;
		poDes.y++;
		pLineSrc = m_pcEditDoc->m_cDocLineMgr.GetLineStr( poSrc.y, &nLineLenSrc );
		/* 行(改行単位)データの要求 */
		nLineLenDes = ::SendMessage( hwndCompareWnd, MYWM_GETLINEDATA, poDes.y, 0 );
	}
end_of_compare:;
	/* 比較後、左右に並べて表示 */
//From Here Oct. 10, 2000 JEPRO	チェックボックスをボタン化すれば以下の行(To Here まで)は不要のはずだが
//	うまくいかなかったので元に戻してある…
	if( m_pShareData->m_Common.m_bCompareAndTileHorz ){
		HWND* phwndArr = new HWND[2];
		phwndArr[0] = ::GetParent( m_hwndParent );
		phwndArr[1] = hwndCompareWnd;
		for( int i = 0; i < 2; ++i ){
			if( ::IsZoomed( phwndArr[i] ) ){
				::ShowWindow( phwndArr[i], SW_RESTORE );
			}
		}
		::TileWindows( NULL, MDITILE_VERTICAL, NULL, 2, phwndArr );
		delete [] phwndArr;
	}
//To Here Oct. 10, 2000

	if( FALSE == bDefferent ){
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, GSTR_APPNAME,
			"異なる箇所は見つかりませんでした。"
		);
	}else{
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, GSTR_APPNAME,
			"異なる箇所が見つかりました。"
		);
		/* カーソルを移動させる */
		memcpy( m_pShareData->m_szWork, (void*)&poDes, sizeof( poDes ) );
		::SendMessage( hwndCompareWnd, MYWM_SETCARETPOS, 0, 0 );

		/* カーソルを移動させる */
		memcpy( m_pShareData->m_szWork, (void*)&poSrc, sizeof( poSrc ) );
		::PostMessage( ::GetParent( m_hwndParent ), MYWM_SETCARETPOS, 0, 0 );
	}
	/* 開いているウィンドウをアクティブにする */
	/* アクティブにする */
	ActivateFrameWindow( hwndCompareWnd );

	/* 開いているウィンドウをアクティブにする */
	/* アクティブにする */
	ActivateFrameWindow( ::GetParent( m_hwndParent ) );
	return;
}




//jeprotestnow Oct. 30, 2000
/* メニューバーの表示/非表示 */
//void CEditView::Command_SHOWMENUBAR( void )
//{
//	HWND		hwndFrame;
//	CEditWnd*	pCEditWnd;
//	RECT		rc;
//	hwndFrame = ::GetParent( m_hwndParent );
//	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );
//	if( NULL == pCEditWnd->m_hwndToolBar ){
//		/* メニューバー作成 */
//		pCEditWnd->CreateToolBar();
//		m_pShareData->m_Common.m_bDispTOOLBAR = TRUE;	/* 次回ウィンドウを開いたときメニューバーを表示する */
//	}else{
//		::DestroyWindow( pCEditWnd->m_hwndToolBar );
//		pCEditWnd->m_hwndToolBar = NULL;
//		m_pShareData->m_Common.m_bDispTOOLBAR = FALSE;	/* 次回ウィンドウを開いたときメニューバーを表示しない */	//Sept. 9, 2000 jepro 「表示する」となっていたのを修正
//	}
////	/* 変更フラグ(共通設定の全体) のセット */
////	m_pShareData->m_nCommonModify = TRUE;
//	::GetClientRect( pCEditWnd->m_hWnd, &rc );
//	::SendMessage( pCEditWnd->m_hWnd, WM_SIZE, pCEditWnd->m_nWinSizeType, MAKELONG( rc.right - rc.left, rc.bottom - rc.top ) );
//	return;
//}
//jeprotestnow To Here




/* ツールバーの表示/非表示 */
void CEditView::Command_SHOWTOOLBAR( void )
{
	HWND		hwndFrame;
	CEditWnd*	pCEditWnd;
	RECT		rc;
	hwndFrame = ::GetParent( m_hwndParent );
	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );
	if( NULL == pCEditWnd->m_hwndToolBar ){
		/* ツールバー作成 */
		pCEditWnd->CreateToolBar();
		m_pShareData->m_Common.m_bDispTOOLBAR = TRUE;	/* 次回ウィンドウを開いたときツールバーを表示する */
	}else{
		::DestroyWindow( pCEditWnd->m_hwndToolBar );
		pCEditWnd->m_hwndToolBar = NULL;
		m_pShareData->m_Common.m_bDispTOOLBAR = FALSE;	/* 次回ウィンドウを開いたときツールバーを表示しない */	//Sept. 9, 2000 jepro 「表示する」となっていたのを修正
	}
//	/* 変更フラグ(共通設定の全体) のセット */
//	m_pShareData->m_nCommonModify = TRUE;
	::GetClientRect( pCEditWnd->m_hWnd, &rc );
	::SendMessage( pCEditWnd->m_hWnd, WM_SIZE, pCEditWnd->m_nWinSizeType, MAKELONG( rc.right - rc.left, rc.bottom - rc.top ) );
	return;
}




/* ステータスバーの表示/非表示 */
void CEditView::Command_SHOWSTATUSBAR( void )
{
	HWND		hwndFrame;
	CEditWnd*	pCEditWnd;
	RECT		rc;
	hwndFrame = ::GetParent( m_hwndParent );
	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );
	if( NULL == pCEditWnd->m_hwndStatusBar ){
		/* ステータスバー作成 */
		pCEditWnd->CreateStatusBar();
		m_pShareData->m_Common.m_bDispSTATUSBAR = TRUE;	/* 次回ウィンドウを開いたときステータスバーを表示する */
	}else{
		/* ステータスバー破棄 */
		pCEditWnd->DestroyStatusBar();
		m_pShareData->m_Common.m_bDispSTATUSBAR = FALSE;	/* 次回ウィンドウを開いたときステータスバーを表示しない */	//Sept. 9, 2000 jepro 「表示する」となっていたのを修正
	}
//	/* 変更フラグ(共通設定の全体) のセット */
//	m_pShareData->m_nCommonModify = TRUE;
	::GetClientRect( pCEditWnd->m_hWnd, &rc );
	::SendMessage( pCEditWnd->m_hWnd, WM_SIZE, pCEditWnd->m_nWinSizeType, MAKELONG( rc.right - rc.left, rc.bottom - rc.top ) );
	::RedrawWindow( pCEditWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW );
	return;
}




/* ファンクションキーの表示/非表示 */
void CEditView::Command_SHOWFUNCKEY( void )
{
	HWND		hwndFrame;
	CEditWnd*	pCEditWnd;
	RECT		rc;
	BOOL		bSizeBox;
	hwndFrame = ::GetParent( m_hwndParent );
	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );
	if( NULL == pCEditWnd->m_CFuncKeyWnd.m_hWnd ){
		m_pShareData->m_Common.m_bDispFUNCKEYWND = TRUE;	/* 次回ウィンドウを開いたときファンクションキーを表示する */
		if( m_pShareData->m_Common.m_nFUNCKEYWND_Place == 0 ){	/* ファンクションキー表示位置／0:上 1:下 */
			bSizeBox = FALSE;
		}else{
			bSizeBox = TRUE;
			/* ステータスバーがあるときはサイズボックスを表示しない */
			if( NULL != pCEditWnd->m_hwndStatusBar ){
				bSizeBox = FALSE;
			}
		}
		pCEditWnd->m_CFuncKeyWnd.Open( m_hInstance, pCEditWnd->m_hWnd, m_pcEditDoc, bSizeBox );
	}else{
		pCEditWnd->m_CFuncKeyWnd.Close();
		m_pShareData->m_Common.m_bDispFUNCKEYWND = FALSE;	/* 次回ウィンドウを開いたときファンクションキーを表示しない */	//Sept. 9, 2000 jepro 「表示する」となっていたのを修正
	}
	m_pcEditDoc->m_cSplitterWnd.DoSplit( -1, -1 );
	::GetClientRect( pCEditWnd->m_hWnd, &rc );
	::SendMessage( pCEditWnd->m_hWnd, WM_SIZE, pCEditWnd->m_nWinSizeType, MAKELONG( rc.right - rc.left, rc.bottom - rc.top ) );
	return;
}




/* 印刷 */
void CEditView::Command_PRINT( void )
{
	PRINTDLG	pd;
//	HWND		hwnd;

//	PRINTDLG	pd;
	/* 初期化 */

	/* デフォルトのプリンタ情報を取得 */
	if( FALSE == CPrint::GetDefaultPrinter( &pd ) ){
		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, GSTR_APPNAME,
			"印刷する前に、プリンタをインストールしてください。\n"
		);
		return;
	}
	::GlobalUnlock( pd.hDevMode );
	::GlobalUnlock( pd.hDevNames );
	::GlobalFree( pd.hDevMode );
	::GlobalFree( pd.hDevNames );


	// Initialize PRINTDLG
	ZeroMemory( &pd, sizeof( PRINTDLG ) );
	pd.lStructSize	= sizeof(PRINTDLG);
	pd.hwndOwner	= m_hWnd;
	pd.hDevMode		= NULL;		// Don't forget to free or store hDevMode.
	pd.hDevNames	= NULL;		// Don't forget to free or store hDevNames.
	pd.Flags		= PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
	pd.nCopies		= 1;
	pd.nFromPage	= 0xFFFF;
	pd.nToPage		= 0xFFFF;
	pd.nMinPage		= 1;
	pd.nMaxPage		= 0xFFFF;

	if( PrintDlg( &pd ) == TRUE ){
		// GDI calls to render output.
		// Delete DC when done.
		DeleteDC(pd.hDC);

		::MYMESSAGEBOX( m_hWnd, MB_OK | MB_ICONINFORMATION | MB_TOPMOST, GSTR_APPNAME,
//			"開発中。印刷プレビューから印刷してください。\n"
			"未実装です。 印刷プレビューから印刷してください。\n"	//Jan. 15, 2001 jepro 「開発」してないのでメッセージ変更
		);

	}
}




/* 印刷プレビュー */
void CEditView::Command_PRINT_PREVIEW( void )
{
	HWND		hwndFrame;
	CEditWnd*	pCEditWnd;
	hwndFrame = ::GetParent( m_hwndParent );
	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );

	/* 印刷プレビューモードのオン/オフ */
	pCEditWnd->PrintPreviewModeONOFF();
	return;
}




/* 印刷のページレイアウトの設定 */
void CEditView::Command_PRINT_PAGESETUP( void )
{
	HWND		hwndFrame;
	CEditWnd*	pCEditWnd;
	BOOL		bRes;
	hwndFrame = ::GetParent( m_hwndParent );
	pCEditWnd = ( CEditWnd* )::GetWindowLong( hwndFrame, GWL_USERDATA );

	/* 印刷ページ設定 */
	bRes = pCEditWnd->OnPrintPageSetting();
	return;
}




/* ブラウズ */
void CEditView::Command_BROWSE( void )
{
	if( 0 == lstrlen( m_pcEditDoc->m_szFilePath ) ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	char	szURL[MAX_PATH + 64];
	wsprintf( szURL, "file://%s", m_pcEditDoc->m_szFilePath );
	/* URLを開く */
	::ShellExecute( NULL, "open", szURL, NULL, NULL, SW_SHOW );
	return;
}




/* キーマクロの記録開始／終了 */
void CEditView::Command_RECKEYMACRO( void )
{
	if( m_pShareData->m_bRecordingKeyMacro ){									/* キーボードマクロの記録中 */
		m_pShareData->m_bRecordingKeyMacro = FALSE;
		m_pShareData->m_hwndRecordingKeyMacro = NULL;							/* キーボードマクロを記録中のウィンドウ */
	}else{
		m_pShareData->m_bRecordingKeyMacro = TRUE;
		m_pShareData->m_hwndRecordingKeyMacro = ::GetParent( m_hwndParent );;	/* キーボードマクロを記録中のウィンドウ */
		/* キーマクロのバッファをクリアする */
		m_pShareData->m_CKeyMacroMgr.Clear();
	}
	/* 親ウィンドウのタイトルを更新 */
	SetParentCaption();

	/* キャレットの行桁位置を表示する */
	DrawCaretPosInfo();

	return;
}




/* キーマクロの保存 */
void CEditView::Command_SAVEKEYMACRO( void )
{
	m_pShareData->m_bRecordingKeyMacro = FALSE;
	m_pShareData->m_hwndRecordingKeyMacro = NULL;	/* キーボードマクロを記録中のウィンドウ */

	CDlgOpenFile	cDlgOpenFile;
	char*			pszMRU = NULL;;
	char*			pszOPENFOLDER = NULL;;
	char			szPath[_MAX_PATH + 1];
	char			szInitDir[_MAX_PATH + 1];
	strcpy( szPath, "" );
	strcpy( szInitDir, m_pShareData->m_szMACROFOLDER );	/* マクロ用フォルダ */

	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create(
		m_hInstance,
		m_hWnd,
		"*.mac",
		szInitDir,
		(const char **)&pszMRU,
		(const char **)&pszOPENFOLDER
	);
	if( !cDlgOpenFile.DoModal_GetSaveFileName( szPath ) ){
		return;
	}
	/* ファイルのフルパスを、フォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szMACROFOLDER, NULL );
	strcat( m_pShareData->m_szMACROFOLDER, "\\" );

	/* キーボードマクロの保存 */
	m_pShareData->m_CKeyMacroMgr.SaveKeyMacro( m_hInstance, m_hWnd, szPath );
	return;
}




/* キーマクロの実行 */
void CEditView::Command_EXECKEYMACRO( void )
{
	m_pShareData->m_bRecordingKeyMacro = FALSE;
	m_pShareData->m_hwndRecordingKeyMacro = NULL;	/* キーボードマクロを記録中のウィンドウ */

	/* キーボードマクロの実行中 */
	m_bExecutingKeyMacro = TRUE;

	/* キーボードマクロの実行 */
	m_pShareData->m_CKeyMacroMgr.ExecKeyMacro( (void*)this );

	/* キーボードマクロの実行中 */
	m_bExecutingKeyMacro = FALSE;

	/* フォーカス移動時の再描画 */
	RedrawAll();

	return;
}




/* キーマクロの読み込み */
void CEditView::Command_LOADKEYMACRO( void )
{
	m_pShareData->m_bRecordingKeyMacro = FALSE;
	m_pShareData->m_hwndRecordingKeyMacro = NULL;	/* キーボードマクロを記録中のウィンドウ */

	CDlgOpenFile	cDlgOpenFile;
	char*			pszMRU = NULL;;
	char*			pszOPENFOLDER = NULL;;
	char			szPath[_MAX_PATH + 1];
	char			szInitDir[_MAX_PATH + 1];
	strcpy( szPath, "" );
	strcpy( szInitDir, m_pShareData->m_szMACROFOLDER );	/* マクロ用フォルダ */
	/* ファイルオープンダイアログの初期化 */
	cDlgOpenFile.Create(
		m_hInstance,
		m_hWnd,
		"*.mac",
		szInitDir,
		(const char **)&pszMRU,
		(const char **)&pszOPENFOLDER
	);
	if( !cDlgOpenFile.DoModal_GetOpenFileName( szPath ) ){
		return;
	}
	/* ファイルのフルパスを、フォルダとファイル名に分割 */
	/* [c:\work\test\aaa.txt] → [c:\work\test] + [aaa.txt] */
	::SplitPath_FolderAndFile( szPath, m_pShareData->m_szMACROFOLDER, NULL );
	strcat( m_pShareData->m_szMACROFOLDER, "\\" );

	/* キーボードマクロの読み込み */
	m_pShareData->m_CKeyMacroMgr.LoadKeyMacro( m_hInstance, m_hWnd, szPath );
	return;
}




/* 現在のウィンドウ幅で折り返し */
void CEditView::Command_WRAPWINDOWWIDTH( void )	//	Oct. 7, 2000 JEPRO WRAPWINDIWWIDTH を WRAPWINDOWWIDTH に変更
{
	if( 10 > m_nViewColNum - 1 ){
		::MessageBeep( MB_ICONHAND );
		return;
	}
	m_pcEditDoc->GetDocumentAttribute().m_nMaxLineSize = m_nViewColNum;

	m_pcEditDoc->OnChangeSetting();	/* ビューに設定変更を反映させる */

	/* 設定変更を反映させる */
	m_cShareData.SendMessageToAllEditors(
		MYWM_CHANGESETTING, (WPARAM)0, (LPARAM)0, ::GetParent( m_hwndParent )
	);	/* 全編集ウィンドウへメッセージをポストする */

	m_nViewLeftCol = 0;		/* 表示域の一番左の桁(0開始) */

	/* フォーカス移動時の再描画 */
	RedrawAll();
	return;
}




//検索マークのクリア
void CEditView::Command_SEARCH_CLEARMARK( void )
{
	m_bCurSrchKeyMark = FALSE;	/* 検索文字列のマーク */
	/* フォーカス移動時の再描画 */
	RedrawAll();
	return;
}




/* 再オープン */
void CEditView::ReOpen_XXX( int nCharCode )
{
	if( -1 != _access( m_pcEditDoc->m_szFilePath, 0 )
	 && m_pcEditDoc->m_bIsModified	/* 変更フラグ */
	){
		if( IDOK != MYMESSAGEBOX( m_hWnd, MB_OKCANCEL | MB_ICONQUESTION | MB_TOPMOST, GSTR_APPNAME,
			"%s\n\nこのファイルは変更されています。\n再ロードを行うと変更が失われますが、よろしいですか?",
			m_pcEditDoc->m_szFilePath
		) ){
			return;
		}
	}
	/* 同一ファイルの再オープン */
	 m_pcEditDoc->ReloadCurrentFile(
		nCharCode,					/* 文字コード種別 */
		m_pcEditDoc->m_bReadOnly	/* 読み取り専用モード */
	);
	/* キャレットの行桁位置を表示する */
	DrawCaretPosInfo();
	return;

}




//SJISで開き直す
void CEditView::Command_FILE_REOPEN_SJIS( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_SJIS );
	return;
}
//JISで開き直す
void CEditView::Command_FILE_REOPEN_JIS( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_JIS );
	return;
}
//EUCで開き直す
void CEditView::Command_FILE_REOPEN_EUC( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_EUC );
	return;
}
//Unicodeで開き直す
void CEditView::Command_FILE_REOPEN_UNICODE( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_UNICODE );
	return;
}
//UTF-8で開き直す
void CEditView::Command_FILE_REOPEN_UTF8( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_UTF8 );
	return;
}
//UTF-7で開き直す
void CEditView::Command_FILE_REOPEN_UTF7( void )
{
	/* 再オープン */
	ReOpen_XXX( CODE_UTF7 );
	return;
}




//日付挿入
void CEditView::Command_INS_DATE( void )
{
	/* 日付をフォーマット */
	char szText[1024];
	::MyGetDateFormat( szText, sizeof( szText ) - 1, m_pShareData->m_Common.m_nDateFormatType, m_pShareData->m_Common.m_szDateFormat );
	/* テキストを貼り付け ver1 */
	Command_INSTEXT( TRUE, szText, TRUE );
	return;
}




//時刻挿入
void CEditView::Command_INS_TIME( void )
{
	/* 時刻をフォーマット */
	char szText[1024];
	::MyGetTimeFormat( szText, sizeof( szText ) - 1, m_pShareData->m_Common.m_nTimeFormatType, m_pShareData->m_Common.m_szTimeFormat );
	/* テキストを貼り付け ver1 */
	Command_INSTEXT( TRUE, szText, TRUE );
	return;
}




//外部コマンド実行
//	From Here Sept. 20, 2000 JEPRO 名称CMMANDをCOMMANDに変更
//	void CEditView::Command_EXECCMMAND( void )
void CEditView::Command_EXECCOMMAND( void )
//	To Here Sept. 20, 2000
{
	CDlgExec cDlgExec;
	/* モードレスダイアログの表示 */
	if( FALSE == cDlgExec.DoModal( m_hInstance, m_hWnd, 0 ) ){
		return;
	}
//	MYTRACE( "cDlgExec.m_szCommand=[%s]\n", cDlgExec.m_szCommand );

	AddToCmdArr( cDlgExec.m_szCommand );

	// 子プロセスの標準出力をリダイレクトする
	ExecCmd( cDlgExec.m_szCommand, cDlgExec.m_bGetStdout );
	return;
}




void CEditView::AddToCmdArr( const char* szCmd )
{
	CMemory*	pcmWork;
	int			i;
	int			j;
	pcmWork = NULL;
	pcmWork = new CMemory( szCmd, lstrlen( szCmd ) );
	for( i = 0; i < m_pShareData->m_nCmdArrNum; ++i ){
		if( 0 == strcmp( szCmd, m_pShareData->m_szCmdArr[i] ) ){
			break;
		}
	}
	if( i < m_pShareData->m_nCmdArrNum ){
		for( j = i; j > 0; j-- ){
			strcpy( m_pShareData->m_szCmdArr[j], m_pShareData->m_szCmdArr[j - 1] );
		}
	}else{
		for( j = MAX_CMDARR - 1; j > 0; j-- ){
			strcpy( m_pShareData->m_szCmdArr[j], m_pShareData->m_szCmdArr[j - 1] );
		}
		++m_pShareData->m_nCmdArrNum;
		if( m_pShareData->m_nCmdArrNum > MAX_CMDARR ){
			m_pShareData->m_nCmdArrNum = MAX_CMDARR;
		}
	}
	strcpy( m_pShareData->m_szCmdArr[0], pcmWork->GetPtr( NULL ) );
	delete pcmWork;
	pcmWork = NULL;
	return;
}




//	Jun. 16, 2000 genta
//	対括弧の検索
void CEditView::Command_BRACKETPAIR( void )
{
	int nLine, nCol;

	if( SearchBracket( m_nCaretPosX, m_nCaretPosY, &nCol, &nLine ) ){
		MoveCursor( nCol, nLine, true );
	}
	else{
		//	失敗した場合は nCol/nLineには有効な値が入っていない.
		//	何もしない
	}
}




//	From HERE Sep. 8, 2000 genta
//	移動履歴を前へたどる
//
void CEditView::Command_JUMPPREV( void )
{
	if( m_cHistory->CheckPrev() ){
		int x, y;
		if( ! m_cHistory->PrevValid() ){
			::MessageBox( NULL, "Inconsistent Implementation", "PrevValid", MB_OK );
		}
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log(
			m_cHistory->GetCurrent().GetPos(),
			m_cHistory->GetCurrent().GetLine(),
			&x, &y );
		MoveCursor( x, y, true );
	}
}




//	移動履歴を次へたどる
void CEditView::Command_JUMPNEXT( void )
{
	if( m_cHistory->CheckNext() ){
		int x, y;
		if( ! m_cHistory->NextValid() ){
			::MessageBox( NULL, "Inconsistent Implementation", "NextValid", MB_OK );
		}
		m_pcEditDoc->m_cLayoutMgr.CaretPos_Phys2Log(
			m_cHistory->GetCurrent().GetPos(),
			m_cHistory->GetCurrent().GetLine(),
			&x, &y );
		MoveCursor( x, y, true );
	}
}
//	To HERE Sep. 8, 2000 genta


/*[EOF]*/
