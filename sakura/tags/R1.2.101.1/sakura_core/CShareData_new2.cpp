//	$Id$
/*!	@file
	ツールバーデータの初期化

	@author Norio Nakatani, Jepro
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani and Jepro

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

#include "CShareData.h"
#include "global.h"

CShareData::CShareData()
{
	m_pszAppName = GSTR_CSHAREDATA;
	m_hFileMap   = NULL;
	m_pShareData = NULL;
	/* ツールバーのボタン TBBUTTON構造体 */
	/*
	typedef struct _TBBUTTON {
		int iBitmap;	// ボタン イメージの 0 から始まるインデックス
		int idCommand;	// ボタンが押されたときに送られるコマンド
		BYTE fsState;	// ボタンの状態--以下を参照
		BYTE fsStyle;	// ボタン スタイル--以下を参照
		DWORD dwData;	// アプリケーション-定義された値
		int iString;	// ボタンのラベル文字列の 0 から始まるインデックス
	} TBBUTTON;
	*/
	SetTBBUTTONVal( &m_tbMyButton[0], 0, 0, 0, TBSTYLE_SEP, 0, 0 );		//セパレータ
	struct TBUTTONDATA {
		int			idCommand;
		BYTE		fsState;
		BYTE		fsStyle;
		DWORD		dwData;
		int			iString;
	};
//	キーワード：アイコン順序(アイコンインデックス)
//	Sept. 16, 2000 Jepro note: アイコン登録メニュー
//	以下の登録はツールバーだけでなくアイコンをもつすべてのメニューで利用されている
//	数字はビットマップリソースのIDB_MYTOOLに登録されているアイコンの先頭からの順番のようである
//	アイコンをもっと登録できるように横幅を16dotsx218=2048dotsに拡大
//	縦も15dotsから16dotsにして「プリンタ」アイコンや「ヘルプ1」の、下が欠けている部分を補ったが15dotsまでしか表示されないらしく効果なし
//	→
//	Sept. 17, 2000 縦16dot目を表示できるようにした
//	修正したファイルにはJEPRO_16thdotとコメントしてあるのでもし間違っていたらそれをキーワードにして検索してください(Sept. 28, 2000現在 6箇所変更)
//	IDB_MYTOOLの16dot目に見やすいように横16dotsづつの区切りになる目印を付けた
//
//	Sept. 16, 2000 見やすいように横に20個(あるいは32個)づつに配列しようとしたが配列構造を変えなければうまく格納できないので
//	それを解決するのが先決(→げんた氏改修版ur3β13で解決)
//
//	Sept. 16, 2000 JEPRO できるだけ系ごとに集まるように順番を大幅に入れ替えた  それに伴いCShareData.cppで設定している初期設定値も変更
//	Oct. 22, 2000 JEPRO アイコンのビットマップリソースの2次元配置が可能になったため根本的に配置転換した
//	・配置の基本は「コマンド一覧」に入っている機能(コマンド)順	なお「コマンド一覧」自体は「メニューバー」の順におおよそ準拠している
//	・アイコンビットマップファイルには横32個X13段あるが有効にしてあるのは11段まで(12段目は基本的に作業用, 13段目は試作品など保管用)
//	・メニューに属する系および各系の段との関係は次の通り(Oct. 22, 2000 現在)：
//		ファイル----- ファイル操作系	(1段目32個: 1-32)
//		編集--------- 編集系			(2段目32個: 33-64)
//		移動--------- カーソル移動系	(3段目32個: 65-96)
//		選択--------- 選択系			(4段目32個: 97-128)
//					+ 矩形選択系		(5段目32個: 129-160) //(注. 矩形選択系のほとんどは未実装)
//					+ クリップボード系	(6段目24個: 161-184)
//			★挿入系					(6段目残りの8個: 185-192)
//		変換--------- 変換系			(7段目32個: 193-224)
//		検索--------- 検索系			(8段目32個: 225-256)
//		ツール------- モード切り替え系	(9段目4個: 257-260)
//					+ 設定系			(9段目次の16個: 261-276)
//					+ マクロ系			(9段目最後の12個: 277-288)
//					+ カスタムメニュー	(10段目32個: 289-320) */
//		ウィンドウ--- ウィンドウ系		(11段目22個: 321-342)
//		ヘルプ------- 支援				(11段目残りの10個: 343-352)
//			★その他					(12段目32個: 353-384)
//	注1.「挿入系」はメニューでは「編集」に入っている
//	注2.「その他」はメニューには入っていないものを入れる (現在何もないので12段目を設定してない)
//	注3.「コマンド一覧」で敢えて重複していれてあるコマンドはその「本家」の方に配置した
//	注4.「コマンド一覧」に入ってないコマンドもわかっている範囲で位置予約にしておいた
//  注5. F_DISABLE は未定義用(ダミーとしても使う)
//	注6. ユーザー用に確保された場所は特にないので各段の空いている後ろの方を使ってください。

	TBUTTONDATA tbd[] = {
/* ファイル操作系(1段目32個: 1-32) */
/*  1 */		F_FILENEW					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//新規作成
/*  2 */		F_FILEOPEN					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//開く
/*  3 */		F_FILESAVE					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//上書き保存
/*  4 */		F_FILESAVEAS				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//名前を付けて保存	//Sept. 18, 2000 JEPRO 追加
/*  5 */		F_FILECLOSE					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//閉じて(無題)	//Oct. 17, 2000 jepro 「ファイルを閉じる」というキャプションを変更
/*  6 */		F_FILECLOSE_OPEN			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//閉じて開く
/*  7 */		F_FILE_REOPEN_SJIS			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SJISで開き直す
/*  8 */		F_FILE_REOPEN_JIS			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//JISで開き直す
/*  9 */		F_FILE_REOPEN_EUC			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//EUCで開き直す
/* 10 */		F_FILE_REOPEN_UNICODE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Unicodeで開き直す
/* 11 */		F_FILE_REOPEN_UTF8			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//UTF-8で開き直す
/* 12 */		F_FILE_REOPEN_UTF7			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//UTF-7で開き直す
/* 13 */		F_PRINT						, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//印刷
/* 14 */		F_PRINT_PREVIEW				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//印刷プレビュー
/* 15 */		F_PRINT_PAGESETUP			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//印刷ページ設定	//Sept. 21, 2000 JEPRO 追加
/* 16 */		F_OPEN_HfromtoC				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//同名のC/C++ヘッダ(ソース)を開く	//Feb. 7, 2001 JEPRO 追加
/* 17 */		F_OPEN_HHPP					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//同名のC/C++ヘッダファイルを開く	//Feb. 9, 2001 jepro「.cまたは.cppと同名の.hを開く」から変更
/* 18 */		F_OPEN_CCPP					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//同名のC/C++ソースファイルを開く	//Feb. 9, 2001 jepro「.hと同名の.c(なければ.cpp)を開く」から変更
/* 19 */		F_ACTIVATE_SQLPLUS			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Oracle SQL*Plusをアクティブ表示 */	//Sept. 20, 2000 JEPRO 追加
/* 20 */		F_PLSQL_COMPILE_ON_SQLPLUS	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Oracle SQL*Plusで実行 */	//Sept. 17, 2000 jepro 説明の「コンパイル」を「実行」に統一
/* 21 */		F_BROWSE					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ブラウズ
/* 22 */		F_PROPERTY_FILE				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ファイルのプロパティ//Sept. 16, 2000 JEPRO mytool1.bmpにあった「ファイルのプロパティ」アイコンをIDB_MYTOOLにコピー
/* 23 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 24 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 25 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 26 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 27 */		F_EXITALL					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//サクラエディタの全終了	//Dec. 27, 2000 JEPRO 追加
/* 28 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 29 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 30 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 31 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 32 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 編集系(2段目32個: 32-64) */
/* 33 */		F_UNDO				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//元に戻す(Undo)
/* 34 */		F_REDO				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//やり直し(Redo)
/* 35 */		F_DELETE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//削除
/* 36 */		F_DELETE_BACK		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル前を削除
/* 37 */		F_WordDeleteToStart	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語の左端まで削除
/* 38 */		F_WordDeleteToEnd	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語の右端まで削除
/* 39 */		F_WordDelete		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語削除
/* 40 */		F_WordCut			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語切り取り
/* 41 */		F_LineDeleteToStart	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行頭まで削除(改行単位)
/* 42 */		F_LineDeleteToEnd	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行末まで削除(改行単位)
/* 43 */		F_LineCutToStart	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行頭まで切り取り(改行単位)
/* 44 */		F_LineCutToEnd		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行末まで切り取り(改行単位)
/* 45 */		F_DELETE_LINE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行削除(折り返し単位)
/* 46 */		F_CUT_LINE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行切り取り(改行単位)
/* 47 */		F_DUPLICATELINE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行の二重化(折り返し単位)
/* 48 */		F_INDENT_TAB		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//TABインデント
/* 49 */		F_UNINDENT_TAB		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//逆TABインデント
/* 50 */		F_INDENT_SPACE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SPACEインデント
/* 51 */		F_UNINDENT_SPACE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//逆SPACEインデント
/* 52 */		F_DISABLE/*F_WORDSREFERENCE*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語リファレンス	//アイコン未作
/* 53 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 54 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 55 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 56 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 57 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 58 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 59 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 60 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 61 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 62 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 63 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 64 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* カーソル移動系(3段目32個: 65-96) */
/* 65 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 66 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 67 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 68 */		F_UP			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル上移動
/* 69 */		F_DOWN			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル下移動
/* 70 */		F_LEFT			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル左移動
/* 71 */		F_RIGHT			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル右移動
/* 72 */		F_UP2			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル上移動(２行ごと)
/* 73 */		F_DOWN2			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル下移動(２行ごと)
/* 74 */		F_WORDLEFT		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語の左端に移動
/* 75 */		F_WORDRIGHT		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//単語の右端に移動
/* 76 */		F_GOLINETOP		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行頭に移動(折り返し単位)
/* 77 */		F_GOLINEEND		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//行末に移動(折り返し単位)
/* 78 */		F_HalfPageUp	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半ページアップ	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
/* 79 */		F_HalfPageDown	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半ページダウン	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
/* 80 */		F_1PageUp		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//１ページアップ	//Oct. 10, 2000 JEPRO 従来のページアップを半ページアップと名称変更し１ページアップを追加
/* 81 */		F_1PageDown		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//１ページダウン	//Oct. 10, 2000 JEPRO 従来のページダウンを半ページダウンと名称変更し１ページダウンを追加
/* 82 */		F_DISABLE/*F_DISPLAYTOP*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//画面の先頭に移動(未実装)
/* 83 */		F_DISABLE/*F_DISPLAYEND*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//画面の最後に移動(未実装)
/* 84 */		F_GOFILETOP		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ファイルの先頭に移動
/* 85 */		F_GOFILEEND		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ファイルの最後に移動
/* 86 */		F_CURLINECENTER	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カーソル行をウィンドウ中央へ
/* 87 */		F_JUMPPREV		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//移動履歴: 前へ	//Sept. 28, 2000 JEPRO 追加
/* 88 */		F_JUMPNEXT		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//移動履歴: 次へ	//Sept. 28, 2000 JEPRO 追加
/* 89 */		F_WndScrollDown	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//テキストを１行下へスクロール	//Jun. 28, 2001 JEPRO 追加
/* 90 */		F_WndScrollUp	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//テキストを１行上へスクロール	//Jun. 28, 2001 JEPRO 追加
/* 91 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 92 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 93 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 94 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 95 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 96 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 選択系(4段目32個: 97-128) */
/* 97 */		F_SELECTWORD		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//現在位置の単語選択
/* 98 */		F_SELECTALL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//すべて選択	//Sept. 21, 2000 JEPRO 追加
/* 99 */		F_BEGIN_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//範囲選択開始
/* 100 */		F_UP_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル上移動
/* 101 */		F_DOWN_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル下移動
/* 102 */		F_LEFT_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル左移動
/* 103 */		F_RIGHT_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル右移動
/* 104 */		F_UP2_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル上移動(２行ごと)
/* 105 */		F_DOWN2_SEL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)カーソル下移動(２行ごと)
/* 106 */		F_WORDLEFT_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)単語の左端に移動
/* 107 */		F_WORDRIGHT_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)単語の右端に移動
/* 108 */		F_GOLINETOP_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)行頭に移動(折り返し単位)
/* 109 */		F_GOLINEEND_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)行末に移動(折り返し単位)
/* 110 */		F_HalfPageUp_Sel	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)半ページアップ	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
/* 111 */		F_HalfPageDown_Sel	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)半ページダウン	//Oct. 6, 2000 JEPRO 名称をPC-AT互換機系に変更(ROLL→PAGE) //Oct. 10, 2000 JEPRO 名称変更
/* 112 */		F_1PageUp_Sel		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)１ページアップ	//Oct. 10, 2000 JEPRO 従来のページアップを半ページアップと名称変更し１ページアップを追加
/* 113 */		F_1PageDown_Sel		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)１ページダウン	//Oct. 10, 2000 JEPRO 従来のページダウンを半ページダウンと名称変更し１ページダウンを追加
/* 114 */		F_DISABLE/*F_DISPLAYTOP_SEL*/, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)画面の先頭に移動(未実装)
/* 115 */		F_DISABLE/*F_DISPLAYEND_SEL*/, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)画面の最後に移動(未実装)
/* 116 */		F_GOFILETOP_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)ファイルの先頭に移動
/* 117 */		F_GOFILEEND_SEL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(範囲選択)ファイルの最後に移動
/* 118 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 119 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 120 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 121 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 122 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 123 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 124 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 125 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 126 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 127 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 128 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 矩形選択系(5段目32個: 129-160) */ //(注. 矩形選択系のほとんどは未実装)
/* 129 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 130 */		F_DISABLE/*F_BOXSELALL*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//矩形ですべて選択
/* 131 */		F_BEGIN_BOX						, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//矩形範囲選択開始	//Sept. 29, 2000 JEPRO 追加
/* 132 */		F_DISABLE/*F_UP_BOX*/			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル上移動
/* 133 */		F_DISABLE/*F_DOWN_BOX*/			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル下移動
/* 134 */		F_DISABLE/*F_LEFT_BOX*/			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル左移動
/* 135 */		F_DISABLE/*F_RIGHT_BOX*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル右移動
/* 136 */		F_DISABLE/*F_UP2_BOX*/			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル上移動(２行ごと)
/* 137 */		F_DISABLE/*F_DOWN2_BOX*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)カーソル下移動(２行ごと)
/* 138 */		F_DISABLE/*F_WORDLEFT_BOX*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)単語の左端に移動
/* 139 */		F_DISABLE/*F_WORDRIGHT_BOX*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)単語の右端に移動
/* 140 */		F_DISABLE/*F_GOLINETOP_BOX*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)行頭に移動(折り返し単位)
/* 141 */		F_DISABLE/*F_GOLINEEND_BOX*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)行末に移動(折り返し単位)
/* 142 */		F_DISABLE/*F_HalfPageUp_Box*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)半ページアップ
/* 143 */		F_DISABLE/*F_HalfPageDown_Box*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)半ページダウン
/* 144 */		F_DISABLE/*F_1PageUp_Box*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)１ページアップ
/* 145 */		F_DISABLE/*F_1PageDown_Box*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)１ページダウン
/* 146 */		F_DISABLE/*F_DISABLE/*F_DISPLAYTOP_BOX*/, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)画面の先頭に移動(未実装)
/* 147 */		F_DISABLE/*F_DISPLAYEND_BOX*/, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)画面の最後に移動(未実装)
/* 148 */		F_DISABLE/*F_GOFILETOP_BOX*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)ファイルの先頭に移動
/* 149 */		F_DISABLE/*F_GOFILEEND_BOX*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//(矩形選択)ファイルの最後に移動
/* 150 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 151 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 152 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 153 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 154 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 155 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 156 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 157 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 158 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 159 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 160 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* クリップボード系(6段目24個: 161-184) */
/* 161 */		F_CUT						, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//切り取り(選択範囲をクリップボードにコピーして削除)
/* 162 */		F_COPY						, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//コピー(選択範囲をクリップボードにコピー)
/* 163 */		F_COPY_CRLF					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//CRLF改行でコピー
/* 164 */		F_PASTE						, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//貼り付け(クリップボードから貼り付け)
/* 165 */		F_PASTEBOX					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//矩形貼り付け(クリップボードから貼り付け)
/* 166 */		F_DISABLE/*F_INSTEXT*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//テキストを貼り付け	(未公開コマンド？未完成？)
/* 167 */		F_DISABLE/*F_ADDTAIL*/		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//最後にテキストを追加	(未公開コマンド？未完成？)
/* 168 */		F_COPYLINES					, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//選択範囲内全行コピー	//Sept. 30, 2000 JEPRO 追加
/* 169 */		F_COPYLINESASPASSAGE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//選択範囲内全行引用符付きコピー	//Sept. 30, 2000 JEPRO 追加
/* 170 */		F_COPYLINESWITHLINENUMBER	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//選択範囲内全行行番号付きコピー	//Sept. 30, 2000 JEPRO 追加
/* 171 */		F_COPYPATH	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//このファイルのパス名をコピー //added Oct. 22, 2000 JEPRO				//Nov. 5, 2000 JEPRO 追加
/* 172 */		F_COPYTAG	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//このファイルのパス名とカーソル位置をコピー //added Oct. 22, 2000 JEPRO	//Nov. 5, 2000 JEPRO 追加
/* 173 */		F_CREATEKEYBINDLIST			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//キー割り当て一覧をコピー //added Oct. 22, 2000 JEPRO	//Dec. 25, 2000 JEPRO アイコン追加
/* 174 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 175 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 176 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 177 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 178 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 179 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 180 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 181 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 182 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 183 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 184 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 挿入系(6段目残り8個: 185-192) */
/* 185 */		F_INS_DATE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//日付挿入	//Nov. 5, 2000 JEPRO 追加
/* 186 */		F_INS_TIME	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//時刻挿入	//Nov. 5, 2000 JEPRO 追加
/* 187 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 188 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 189 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 190 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 191 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 192 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 変換系(7段目32個: 193-224) */
/* 193 */		F_TOLOWER				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//英大文字→英小文字
/* 194 */		F_TOUPPER				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//英小文字→英大文字
/* 195 */		F_TOHANKAKU				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//全角→半角
/* 196 */		F_TOZENKAKUKATA			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半角＋全ひら→全角・カタカナ	//Sept. 17, 2000 jepro 説明を「半角→全角カタカナ」から変更
/* 197 */		F_TOZENKAKUHIRA			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半角＋全カタ→全角・ひらがな	//Sept. 17, 2000 jepro 説明を「半角→全角ひらがな」から変更
/* 198 */		F_HANKATATOZENKAKUKATA	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半角カタカナ→全角カタカナ	//Sept. 18, 2000 JEPRO 追加
/* 199 */		F_HANKATATOZENKAKUHIRA	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半角カタカナ→全角ひらがな	//Sept. 18, 2000 JEPRO 追加
/* 200 */		F_TABTOSPACE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//TAB→空白	//Sept. 20, 2000 JEPRO 追加
/* 201 */		F_CODECNV_AUTO2SJIS		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//自動判別→SJISコード変換
/* 202 */		F_CODECNV_EMAIL			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//E-Mail(JIS→SIJIS)コード変換
/* 203 */		F_CODECNV_EUC2SJIS		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//EUC→SJISコード変換
/* 204 */		F_CODECNV_UNICODE2SJIS	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Unicode→SJISコード変換
/* 205 */		F_CODECNV_UTF82SJIS		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//UTF-8→SJISコード変換
/* 206 */		F_CODECNV_UTF72SJIS		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//UTF-7→SJISコード変換
/* 207 */		F_CODECNV_SJIS2JIS		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SJIS→JISコード変換
/* 208 */		F_CODECNV_SJIS2EUC		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SJIS→EUCコード変換
/* 209 */		F_CODECNV_SJIS2UTF8		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SJIS→UTF-8コード変換
/* 210 */		F_CODECNV_SJIS2UTF7		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//SJIS→UTF-7コード変換
/* 211 */		F_BASE64DECODE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Base64デコードして保存	//Sept. 28, 2000 JEPRO 追加
/* 212 */		F_UUDECODE				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//uudecodeしてファイルに保存//Sept. 28, 2000 JEPRO 追加	//Oct. 17, 2000 jepro 説明を「選択部分をUUENCODEデコード」から変更
/* 213 */		F_SPACETOTAB			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//空白→TAB	//Jun. 01, 2001 JEPRO 追加
/* 214 */		F_TOZENEI				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//半角英数→全角英数 //July. 30, 2001 Misaka 追加
/* 215 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 216 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 217 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 218 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 219 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 220 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 221 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 222 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 223 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 224 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 検索系(8段目32個: 225-256) */
/* 225 */		F_SEARCH_DIALOG		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//検索(単語検索ダイアログ)
/* 226 */		F_SEARCH_NEXT		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//次を検索
/* 227 */		F_SEARCH_PREV		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//前を検索
/* 228 */		F_REPLACE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//置換	//Sept. 21, 2000 JEPRO 追加
/* 229 */		F_SEARCH_CLEARMARK	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//検索マークのクリア
/* 230 */		F_GREP				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//Grep
/* 231 */		F_JUMP				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//指定行へジャンプ		//Sept. 21, 2000 JEPRO 追加
/* 232 */		F_OUTLINE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//アウトライン解析
/* 233 */		F_TAGJUMP			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//タグジャンプ機能			//Sept. 21, 2000 JEPRO 追加
/* 234 */		F_TAGJUMPBACK		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//タグジャンプバック機能	//Sept. 21, 2000 JEPRO 追加
/* 235 */		F_COMPARE			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ファイル内容比較	//Sept. 21, 2000 JEPRO 追加
/* 236 */		F_BRACKETPAIR		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//対括弧の検索	//Sept. 20, 2000 JEPRO 追加
/* 237 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 238 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 239 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 240 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 241 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 242 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 243 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 244 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 245 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 246 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 247 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 248 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 249 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 250 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 251 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 252 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 253 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 254 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 255 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 256 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* モード切り替え系(9段目4個: 257-260) */
/* 257 */		F_CHGMOD_INS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//挿入／上書きモード切り替え	//Nov. 5, 2000 JEPRO 追加
/* 258 */		F_CANCEL_MODE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//各種モードの取り消し			//Nov. 7, 2000 JEPRO 追加
/* 259 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 260 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 設定系(9段目次の16個: 261-276) */
/* 261 */		F_SHOWTOOLBAR		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ツールバーの表示
/* 262 */		F_SHOWFUNCKEY		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ファンクションキーの表示
/* 263 */		F_SHOWSTATUSBAR		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ステータスバーの表示
/* 264 */		F_TYPE_LIST			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//タイプ別設定一覧	//Sept. 18, 2000 JEPRO 追加
/* 265 */		F_OPTION_TYPE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//タイプ別設定
/* 266 */		F_OPTION			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//共通設定			//Sept. 16, 2000 jepro 説明を「設定プロパティシート」から変更
/* 267 */		F_FONT				, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//フォント設定
/* 268 */		F_WRAPWINDOWWIDTH	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//現在のウィンドウ幅で折り返し	//	Oct. 7, 2000 JEPRO WRAPWINDIWWIDTH を WRAPWINDOWWIDTH に変更
/* 269 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 270 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 271 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 272 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 273 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 274 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 275 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 276 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* マクロ系(9段目最後の12個: 277-288) */
/* 277 */		F_RECKEYMACRO	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//キーマクロの記録開始／終了
/* 278 */		F_SAVEKEYMACRO	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//キーマクロの保存		//Sept. 21, 2000 JEPRO 追加
/* 279 */		F_LOADKEYMACRO	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//キーマクロの読み込み	//Sept. 21, 2000 JEPRO 追加
/* 280 */		F_EXECKEYMACRO	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//キーマクロの実行		//Sept. 16, 2000 JEPRO 下から上に移動した
/* 281 */		F_EXECCOMMAND	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//外部コマンド実行//Sept. 20, 2000 JEPRO 名称をCMMANDからCOMMANDに変更(EXECCMMAND→EXECCMMAND)
/* 282 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 283 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 284 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 285 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 286 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 287 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 288 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* カスタムメニュー(10段目32個: 289-320) */
/* 289 */		F_MENU_RBUTTON	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//右クリックメニュー 	//Sept. 30, 2000 JEPRO 追加
/* 290 */		F_CUSTMENU_1	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー1
/* 291 */		F_CUSTMENU_2	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー2
/* 292 */		F_CUSTMENU_3	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー3
/* 293 */		F_CUSTMENU_4	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー4
/* 294 */		F_CUSTMENU_5	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー5
/* 295 */		F_CUSTMENU_6	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー6
/* 296 */		F_CUSTMENU_7	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー7
/* 297 */		F_CUSTMENU_8	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー8
/* 298 */		F_CUSTMENU_9	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー9
/* 299 */		F_CUSTMENU_10	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー10
/* 300 */		F_DISABLE/*F_CUSTMENU_11*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー11	//アイコン未作
/* 301 */		F_DISABLE/*F_CUSTMENU_12*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー12	//アイコン未作
/* 302 */		F_DISABLE/*F_CUSTMENU_13*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー13	//アイコン未作
/* 303 */		F_DISABLE/*F_CUSTMENU_14*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー14	//アイコン未作
/* 304 */		F_DISABLE/*F_CUSTMENU_15*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー15	//アイコン未作
/* 305 */		F_DISABLE/*F_CUSTMENU_16*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー16	//アイコン未作
/* 306 */		F_DISABLE/*F_CUSTMENU_17*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー17	//アイコン未作
/* 307 */		F_DISABLE/*F_CUSTMENU_18*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー18	//アイコン未作
/* 308 */		F_DISABLE/*F_CUSTMENU_19*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー19	//アイコン未作
/* 309 */		F_DISABLE/*F_CUSTMENU_20*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー20	//アイコン未作
/* 310 */		F_DISABLE/*F_CUSTMENU_21*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー21	//アイコン未作
/* 311 */		F_DISABLE/*F_CUSTMENU_22*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー22	//アイコン未作
/* 312 */		F_DISABLE/*F_CUSTMENU_23*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー23	//アイコン未作
/* 313 */		F_DISABLE/*F_CUSTMENU_24*/	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//カスタムメニュー24	//アイコン未作
/* 314 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 315 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 316 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 317 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 318 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 319 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 320 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* ウィンドウ系(11段目22個: 321-342) */
/* 321 */		F_SPLIT_V		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//上下に分割	//Sept. 16, 2000 jepro 説明を「縦」から「上下に」に変更
/* 322 */		F_SPLIT_H		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//左右に分割	//Sept. 16, 2000 jepro 説明を「横」から「左右に」に変更
/* 323 */		F_SPLIT_VH		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//縦横に分割	//Sept. 17, 2000 jepro 説明に「に」を追加
/* 324 */		F_WINCLOSE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ウィンドウを閉じる
/* 325 */		F_WIN_CLOSEALL	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//すべてのウィンドウを閉じる	//Sept. 18, 2000 JEPRO 追加	//Oct. 17, 2000 JEPRO 名前を変更(F_FILECLOSEALL→F_WIN_CLOSEALL)
/* 329 */		F_NEXTWINDOW	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//次のウィンドウ
/* 330 */		F_PREVWINDOW	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//前のウィンドウ
/* 326 */		F_CASCADE		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//重ねて表示
/* 237 */		F_TILE_V		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//上下に並べて表示
/* 328 */		F_TILE_H		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//左右に並べて表示
/* 331 */		F_MAXIMIZE_V	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//縦方向に最大化
/* 332 */		F_MAXIMIZE_H	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//横方向に最大化 //2001.02.10 by MIK
/* 333 */		F_MINIMIZE_ALL	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//すべて最小化					//Sept. 17, 2000 jepro 説明の「全て」を「すべて」に統一
/* 334 */		F_REDRAW		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//再描画						//Sept. 30, 2000 JEPRO 追加
/* 335 */		F_WIN_OUTPUT	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//アウトプットウィンドウ表示	//Sept. 18, 2000 JEPRO 追加
/* 336 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 337 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 338 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 339 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 340 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 341 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 342 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* 支援(11段目残りの10個: 343-352) */
/* 343 */		F_HOKAN			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//入力補完
/* 344 */		F_HELP_CONTENTS , TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ヘルプ目次			//Nov. 25, 2000 JEPRO 追加
/* 345 */		F_HELP_SEARCH	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ヘルプキーワード検索	//Nov. 25, 2000 JEPRO 追加
/* 346 */		F_MENU_ALLFUNC	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//コマンド一覧			//Sept. 30, 2000 JEPRO 追加
/* 347 */		F_EXTHELP1		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//外部ヘルプ１
/* 348 */		F_EXTHTMLHELP	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//外部HTMLヘルプ
/* 349 */		F_ABOUT			, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//バージョン情報	//Dec. 24, 2000 JEPRO 追加
/* 350 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 351 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 352 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー

/* その他(12段目32個: 353-384) */
///* 353 */		F_SENDMAIL		, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//メール送信	//Oct. 22, 2000 JEPRO (ただしメール機能は死んでいる)
/* 354 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー(「????」用予約)	//Sept. 20, 2000 JEPRO ダミーとしてF_DISABLEが使えるのを発見！
/* 355 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 356 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 357 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 358 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 359 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 360 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 361 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 362 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 363 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 364 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 365 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 366 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 367 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 368 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 369 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 370 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 371 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 372 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 373 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 374 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 375 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 376 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 377 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 378 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 379 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 380 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 381 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 382 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 383 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0,	//ダミー
/* 384 */		F_DISABLE	, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0	//最終行用ダミー(Jepro note: 最終行末にはカンマを付けないこと)

};
	int tbd_num = sizeof( tbd ) / sizeof( tbd[0] );
	for( int i = 0; i < tbd_num; i++ ){
		SetTBBUTTONVal(
			&m_tbMyButton[i+1],
			i,
			tbd[i].idCommand,
			tbd[i].fsState,
			tbd[i].fsStyle,
			tbd[i].dwData,
			tbd[i].iString
		);
	}
	m_nMyButtonNum = tbd_num + 1;
	return;
}


/*[EOF]*/
