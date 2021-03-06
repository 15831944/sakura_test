//	$Id$
/*!	@file
	キーボードマクロ

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000-2001, jepro
	Copyright (C) 2001, hor

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "funccode.h"
#include "CMacro.h"
#include "CEditApp.h"
#include "CEditView.h" //2002/2/10 aroka
#include "CSMacroMgr.h" //2002/2/10 aroka
#include "etc_uty.h" //2002/2/10 aroka

CMacro::CMacro( int nFuncID )
{
	m_nFuncID = nFuncID;
	m_pNext = NULL;
	m_pParamTop = m_pParamBot = NULL;
}

CMacro::~CMacro( void )
{
	CMacroParam* p = m_pParamTop;
	CMacroParam* del_p;
	while (p){
		del_p = p;
		p = p->m_pNext;
		delete del_p->m_pData;
		delete del_p;
	}
	return;
}

/*	引数の型振り分け
	機能IDによって、期待する型は異なります。
	そこで、引数の型を機能IDによって振り分けて、AddParamしましょう。
	たとえば、F_INSTEXTの1つめ、2つめの引数は文字列、3つめの引数はintだったりするのも、ここでうまく振り分けられることを期待しています。

	lParamは、HandleCommandのparamに値を渡しているコマンドの場合にのみ使います。
*/
void CMacro::AddLParam( LPARAM lParam, CEditView* pcEditView )
{
	switch( m_nFuncID ){
	/*	文字列パラメータを追加 */
	case F_INSTEXT:
	case F_FILEOPEN:
	case F_EXECCOMMAND:
		{
			AddParam( (const char *)lParam );	//	lParamを追加。
			LPARAM lFlag = 0x00;
			lFlag |= pcEditView->m_pShareData->m_bGetStdout ? 0x01 : 0x00;
			AddParam( lFlag );
		}
		break;

	case F_JUMP:	//	指定行へジャンプ（ただしPL/SQLコンパイルエラー行へのジャンプは未対応）
		{
			AddParam( pcEditView->m_pcEditDoc->m_cDlgJump.m_nLineNum );
			LPARAM lFlag = 0x00;
			lFlag |= pcEditView->m_pShareData->m_bLineNumIsCRLF		? 0x01 : 0x00;
			lFlag |= pcEditView->m_pcEditDoc->m_cDlgJump.m_bPLSQL	? 0x02 : 0x00;
			AddParam( lFlag );
		}
		break;

	case F_BOOKMARK_PATTERN:	//2002.02.08 hor
	case F_SEARCH_NEXT:
	case F_SEARCH_PREV:
		{
			AddParam( pcEditView->m_pShareData->m_szSEARCHKEYArr[0] );	//	lParamを追加。

			LPARAM lFlag = 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bWordOnly			? 0x01 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bLoHiCase			? 0x02 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bRegularExp		? 0x04 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bNOTIFYNOTFOUND	? 0x08 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bAutoCloseDlgFind	? 0x10 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bSearchAll		? 0x20 : 0x00;
			AddParam( lFlag );
		}
		break;
	case F_REPLACE:
	case F_REPLACE_ALL:
		{
			AddParam( pcEditView->m_pShareData->m_szSEARCHKEYArr[0] );	//	lParamを追加。
			AddParam( pcEditView->m_pShareData->m_szREPLACEKEYArr[0] );	//	lParamを追加。

			LPARAM lFlag = 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bWordOnly			? 0x01 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bLoHiCase			? 0x02 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bRegularExp		? 0x04 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bNOTIFYNOTFOUND	? 0x08 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bAutoCloseDlgFind	? 0x10 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bSearchAll		? 0x20 : 0x00;
			lFlag |= pcEditView->m_pcEditDoc->m_cDlgReplace.m_nPaste		? 0x40 : 0x00;	//	CShareDataに入れなくていいの？
			lFlag |= pcEditView->m_pShareData->m_Common.m_bSelectedArea		? 0x80 : 0x00;	//	置換する時は選べない
			lFlag |= pcEditView->m_pcEditDoc->m_cDlgReplace.m_nReplaceTarget << 8;	//	8bitシフト（0x100で掛け算）
			AddParam( lFlag );
		}
		break;
	case F_GREP:
		{
			AddParam( pcEditView->m_pShareData->m_szSEARCHKEYArr[0] );	//	lParamを追加。
			AddParam( pcEditView->m_pShareData->m_szGREPFILEArr[0] );	//	lParamを追加。
			AddParam( pcEditView->m_pShareData->m_szGREPFOLDERArr[0] );	//	lParamを追加。

			LPARAM lFlag = 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bGrepSubFolder				? 0x01 : 0x00;
			//			この編集中のテキストから検索する(0x02.未実装)
			lFlag |= pcEditView->m_pShareData->m_Common.m_bLoHiCase						? 0x04 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bRegularExp					? 0x08 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bGrepKanjiCode_AutoDetect		? 0x10 : 0x00;
			lFlag |= pcEditView->m_pShareData->m_Common.m_bGrepOutputLine				? 0x20 : 0x00;
			lFlag |= (pcEditView->m_pShareData->m_Common.m_nGrepOutputStyle == 2)		? 0x40 : 0x00;	//	CShareDataに入れなくていいの？
			AddParam( lFlag );
		}
		break;
	/*	数値パラメータを追加 */
	case F_CHAR:
		AddParam( lParam );
		break;

	/*	標準もパラメータを追加 */
	default:
		AddParam( lParam );
		break;
	}
}

/*	引数に文字列を追加。
*/
void CMacro::AddParam( const char* szParam )
{
	CMacroParam* param = new CMacroParam;
	param->m_pNext = NULL;

	//	必要な領域を確保してコピー。
	int nLen = lstrlen( szParam );
	param->m_pData = new char[nLen + 1];
	memcpy((char*)param->m_pData, szParam, nLen );
	param->m_pData[nLen] = '\0';

	//	リストの整合性を保つ
	if (m_pParamTop){
		m_pParamBot->m_pNext = param; 
		m_pParamBot = param;
	}
	else {
		m_pParamTop = param;
		m_pParamBot = m_pParamTop;
	}
}

/*	引数に数値を追加。
*/
void CMacro::AddParam( const int nParam )
{
	CMacroParam* param = new CMacroParam;
	param->m_pNext = NULL;

	//	必要な領域を確保してコピー。
	param->m_pData = new char[16];	//	数値格納（最大16桁）用
	itoa(nParam, param->m_pData, 10);

	//	リストの整合性を保つ
	if (m_pParamTop){
		m_pParamBot->m_pNext = param; 
		m_pParamBot = param;
	}
	else {
		m_pParamTop = param;
		m_pParamBot = m_pParamTop;
	}
}

/*	コマンドを実行する（pcEditView->HandleCommandを発行する）
	m_nFuncIDによって、引数の型を正確に渡してあげましょう。
	
	paramArrは何かのポインタ（アドレス）をLONGであらわした値になります。
	引数がchar*のときは、paramArr[i]をそのままHandleCommandに渡してかまいません。
	引数がintのときは、*((int*)paramArr[i])として渡しましょう。
	
	たとえば、F_INSTEXTの1つめ、2つめの引数は文字列、3つめの引数はint、4つめの引数が無し。だったりする場合は、次のようにしましょう。
	pcEditView->HandleCommand( m_nFuncID, TRUE, paramArr[0], paramArr[1], *((int*)paramArr[2]), 0);
*/
void CMacro::Exec( CEditView* pcEditView )
{
	const char* paramArr[4] = {NULL, NULL, NULL, NULL};	//	4つに限定。
	
	CMacroParam* p = m_pParamTop;
	int i = 0;
	for (i = 0; i < 4; i++) {
		if (!p) break;	//	pが無ければbreak;
		paramArr[i] = p->m_pData;
		p = p->m_pNext;
	}
	CMacro::HandleCommand(pcEditView, m_nFuncID, paramArr, i);
}

/*	CMacroを再現するための情報をhFileに書き出します。

	InsText("なんとか");
	のように。
*/
void CMacro::Save( HINSTANCE hInstance, HFILE hFile )
{
	char	szFuncName[1024];
	char	szFuncNameJapanese[500];
	int		nTextLen;
	const char*	pText;
//	int		i;
	char		szLine[1024];
//	const char*	szFuncName;
	CMemory		cmemWork;

	/* 2002.2.2 YAZAKI CSMacroMgrに頼む */
	if (CSMacroMgr::GetFuncInfoByID( hInstance, m_nFuncID, szFuncName, szFuncNameJapanese)){
#if 0
	for( i = 0; i < m_nMacroFuncInfoArrNum; ++i ){
		if( m_MacroFuncInfoArr[i].m_nFuncID == m_nFuncID ){
			szFuncName = m_MacroFuncInfoArr[i].m_pszFuncName;
			::LoadString( hInstance, m_nFuncID, szFuncNameJapanese, 255 );
#endif
		switch ( m_nFuncID ){
		case F_INSTEXT:
		case F_FILEOPEN:
		case F_JUMP:		//	指定行へジャンプ（ただしPL/SQLコンパイルエラー行へのジャンプは未対応）
			wsprintf( szLine, "%s(%d, %d);\t// %s\r\n", szFuncName, (m_pParamTop->m_pData ? atoi(m_pParamTop->m_pData) : 1), m_pParamTop->m_pNext->m_pData ? atoi(m_pParamTop->m_pNext->m_pData) : 0, szFuncNameJapanese );
			_lwrite( hFile, szLine, strlen( szLine ) );
			break;
		case F_BOOKMARK_PATTERN:	//2002.02.08 hor
		case F_SEARCH_NEXT:
		case F_SEARCH_PREV:
			pText = m_pParamTop->m_pData;
			nTextLen = strlen(pText);
			cmemWork.SetData( pText, nTextLen );
			cmemWork.Replace( "\\", "\\\\" );
			cmemWork.Replace( "\'", "\\\'" );
			wsprintf( szLine, "%s(\'%s\', %d);\t// %s\r\n", szFuncName, cmemWork.GetPtr( NULL ), m_pParamTop->m_pNext->m_pData ? atoi(m_pParamTop->m_pNext->m_pData) : 0, szFuncNameJapanese );
			_lwrite( hFile, szLine, strlen( szLine ) );
			break;
		case F_EXECCOMMAND:
			//	引数ひとつ分だけ保存
			pText = m_pParamTop->m_pData;
			nTextLen = strlen(pText);
			cmemWork.SetData( pText, nTextLen );
			cmemWork.Replace( "\\", "\\\\" );
			cmemWork.Replace( "\'", "\\\'" );
			wsprintf( szLine, "%s(\'%s\', %d);\t// %s\r\n", szFuncName, cmemWork.GetPtr( NULL ), m_pParamTop->m_pNext->m_pData ? atoi(m_pParamTop->m_pNext->m_pData) : 0, szFuncNameJapanese );
			_lwrite( hFile, szLine, strlen( szLine ) );
			break;
		case F_REPLACE:
		case F_REPLACE_ALL:
			pText = m_pParamTop->m_pData;
			nTextLen = strlen(pText);
			cmemWork.SetData( pText, nTextLen );
			cmemWork.Replace( "\\", "\\\\" );
			cmemWork.Replace( "\'", "\\\'" );
			{
				CMemory cmemWork2(m_pParamTop->m_pNext->m_pData, strlen(m_pParamTop->m_pNext->m_pData));
				cmemWork2.Replace( "\\", "\\\\" );
				cmemWork2.Replace( "\'", "\\\'" );
				wsprintf( szLine, "%s(\'%s\', \'%s\', %d);\t// %s\r\n", szFuncName, cmemWork.GetPtr2(), cmemWork2.GetPtr2(), m_pParamTop->m_pNext->m_pNext->m_pData ? atoi(m_pParamTop->m_pNext->m_pNext->m_pData) : 0, szFuncNameJapanese );
				_lwrite( hFile, szLine, strlen( szLine ) );
			}
			break;
		case F_GREP:
			pText = m_pParamTop->m_pData;
			nTextLen = strlen(pText);
			cmemWork.SetData( pText, nTextLen );
			cmemWork.Replace( "\\", "\\\\" );
			cmemWork.Replace( "\'", "\\\'" );
			{
				CMemory cmemWork2(m_pParamTop->m_pNext->m_pData, strlen(m_pParamTop->m_pNext->m_pData));
				cmemWork2.Replace( "\\", "\\\\" );
				cmemWork2.Replace( "\'", "\\\'" );

				CMemory cmemWork3(m_pParamTop->m_pNext->m_pNext->m_pData, strlen(m_pParamTop->m_pNext->m_pNext->m_pData));
				cmemWork3.Replace( "\\", "\\\\" );
				cmemWork3.Replace( "\'", "\\\'" );
				wsprintf( szLine, "%s(\'%s\', \'%s\', \'%s\', %d);\t// %s\r\n", szFuncName, cmemWork.GetPtr2(), cmemWork2.GetPtr2(), cmemWork3.GetPtr2(), m_pParamTop->m_pNext->m_pNext->m_pNext->m_pData ? atoi(m_pParamTop->m_pNext->m_pNext->m_pNext->m_pData) : 0, szFuncNameJapanese );
				_lwrite( hFile, szLine, strlen( szLine ) );
			}
			break;
		default:
			if( 0 == m_pParamTop ){
				wsprintf( szLine, "%s();\t// %s\r\n", szFuncName, szFuncNameJapanese );
			}else{
				wsprintf( szLine, "%s(%d);\t// %s\r\n", szFuncName, m_pParamTop->m_pData ? atoi(m_pParamTop->m_pData) : 0, szFuncNameJapanese );
			}
			_lwrite( hFile, szLine, strlen( szLine ) );
			break;
		}
		return;
	}
#if 0
	}
#endif
	wsprintf( szLine, "CMacro::GetFuncInfoByID()に、バグがあるのでエラーが出ましたぁぁぁぁぁぁあああ\r\n" );
	_lwrite( hFile, szLine, strlen( szLine ) );
}

/*!	Macroコマンドを、CEditViewのHandleCommandに引き渡す。

	Index: 機能ID
	*Argument[]: 引数
	ArgSize:引数の数
*/
void CMacro::HandleCommand( CEditView* pcEditView, const int Index,	const char* Argument[], const int ArgSize )
{
	switch (Index) 
	{
	case F_CHAR:		//	文字入力。数値は文字コード
	case F_GOLINETOP:	//	行頭に移動。数値は、0x0（デフォルト）、0x1（空白を無視して先頭に移動）、0x2（未定義）、0x4（選択して移動）、0x8（改行単位で先頭に移動：未実装）
		//	一つ目の引数が数値。
		pcEditView->HandleCommand( Index, FALSE, atoi(Argument[0]), 0, 0, 0 );
		break;
	case F_INSTEXT:		//	テキスト挿入
	case F_ADDTAIL:		//	この操作はキーボード操作では存在しないので保存することができない？
		//	一つ目の引数が文字列。
		//	ただし2つ目の引数は文字数。
		{
			int len = strlen(Argument[0]);
			pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], len, 0, 0 );	//	標準
		}
		break;
	/* 一つ目、二つ目とも引数は数値 */
	case F_JUMP:		//	指定行へジャンプ（ただしPL/SQLコンパイルエラー行へのジャンプは未対応）
		//	Argument[0]へジャンプ。オプションはArgument[1]に。
		//		******** 以下「行番号の単位」 ********
		//		0x00	折り返し単位の行番号
		//		0x01	改行単位の行番号
		//		**************************************
		//		0x02	PL/SQLコンパイルエラー行を処理する
		//		未定義	テキストの□行目をブロックの1行目とする
		//		未定義	検出されたPL/SQLパッケージのブロックから選択
		{
			pcEditView->m_pcEditDoc->m_cDlgJump.m_nLineNum = atoi(Argument[0]);	//ジャンプ先
			LPARAM lFlag = atoi(Argument[1]);
			pcEditView->m_pShareData->m_bLineNumIsCRLF = lFlag & 0x01 ? 1 : 0;
			pcEditView->m_pcEditDoc->m_cDlgJump.m_bPLSQL = lFlag & 0x02 ? 1 : 0;
			pcEditView->HandleCommand( Index, FALSE, 0, 0, 0, 0 );	//	標準
		}
		break;
	/*	一つ目の引数は文字列、二つ目の引数は数値	*/
	case F_BOOKMARK_PATTERN:	//2002.02.08 hor
	case F_SEARCH_NEXT:
	case F_SEARCH_PREV:
		//	Argument[0]を検索。オプションはArgument[1]に。
		//	Argument[1]:
		//		0x01	単語単位で探す
		//		0x02	英大文字と小文字を区別する
		//		0x04	正規表現
		//		0x08	見つからないときにメッセージを表示
		//		0x10	検索ダイアログを自動的に閉じる
		//		0x20	先頭（末尾）から再検索する
		//	各値をShareDataに設定してコマンドを発行し、ShareDataの値を元に戻す。
		{
			if( 0 < lstrlen( Argument[0] ) ){
				/* 正規表現 */
				if( pcEditView->m_pShareData->m_Common.m_bRegularExp && !CheckRegexpSyntax( Argument[0], NULL, true ) ){
					break;
				}

				/* 検索文字列 */
				CShareData::getInstance()->AddToSearchKeyArr( (const char*)Argument[0] );
			}
			//	設定値バックアップ
			//	マクロパラメータ→設定値変換
			LPARAM lFlag = atoi(Argument[1]);
			pcEditView->m_pShareData->m_Common.m_bWordOnly			= lFlag & 0x01 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bLoHiCase			= lFlag & 0x02 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bRegularExp		= lFlag & 0x04 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bNOTIFYNOTFOUND	= lFlag & 0x08 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bAutoCloseDlgFind	= lFlag & 0x10 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bSearchAll			= lFlag & 0x20 ? 1 : 0;

			//	コマンド発行
		//	pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], 0, 0, 0);
			pcEditView->HandleCommand( Index, FALSE, 0, 0, 0, 0);
		}
		break;
	case F_EXECCOMMAND:
		//	Argument[0]を実行。オプションはArgument[1]に。
		//	Argument[1]:
		//		次の数値の和。
		//		0x01	標準出力を得る
		{
			LPARAM lFlag = atoi(Argument[1]);
			pcEditView->m_pShareData->m_bGetStdout = lFlag & 0x01 ? 1 : 0;
		//	pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], (LPARAM)atoi(Argument[1]), 0, 0);
			pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], 0, 0, 0);
		}
		break;
	/* はじめの2つの引数は文字列。3つ目は数値 */
	case F_REPLACE:
	case F_REPLACE_ALL:
		//	Argument[0]を、Argument[1]に置換。オプションはArgument[2]に（入れる予定）
		//	Argument[2]:
		//		次の数値の和。
		//		0x001	単語単位で探す
		//		0x002	英大文字と小文字を区別する
		//		0x004	正規表現
		//		0x008	見つからないときにメッセージを表示
		//		0x010	検索ダイアログを自動的に閉じる
		//		0x020	先頭（末尾）から再検索する
		//		0x040	クリップボードから貼り付ける
		//		******** 以下「置換範囲」 ********
		//		0x000	ファイル全体
		//		0x080	選択範囲
		//		**********************************
		//		******** 以下「置換対象」 ********
		//		0x000	見つかった文字列と置換
		//		0x100	見つかった文字列の前に挿入
		//		0x200	見つかった文字列の後に追加
		//		**********************************
		//	各値をShareDataに設定してコマンドを発行し、ShareDataの値を元に戻す。
		{
			if( 0 < lstrlen( Argument[0] ) ){
				/* 正規表現 */
				if( pcEditView->m_pShareData->m_Common.m_bRegularExp && !CheckRegexpSyntax( Argument[0], NULL, true ) ){
					break;
				}

				/* 検索文字列 */
				CShareData::getInstance()->AddToSearchKeyArr( (const char*)Argument[0] );
			}
			if( 0 < lstrlen( Argument[1] ) ){
				/* 検索文字列 */
				CShareData::getInstance()->AddToReplaceKeyArr( (const char*)Argument[1] );
			}
			LPARAM lFlag = atoi(Argument[2]);
			pcEditView->m_pShareData->m_Common.m_bWordOnly			= lFlag & 0x01 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bLoHiCase			= lFlag & 0x02 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bRegularExp		= lFlag & 0x04 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bNOTIFYNOTFOUND	= lFlag & 0x08 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bAutoCloseDlgFind	= lFlag & 0x10 ? 1 : 0;
			pcEditView->m_pShareData->m_Common.m_bSearchAll			= lFlag & 0x20 ? 1 : 0;
			pcEditView->m_pcEditDoc->m_cDlgReplace.m_nPaste			= lFlag & 0x40 ? 1 : 0;	//	CShareDataに入れなくていいの？
//			pcEditView->m_pShareData->m_Common.m_bSelectedArea		= 0;	//	lFlag & 0x80 ? 1 : 0;
			if (Index == F_REPLACE) {
				//	置換する時は選べない
				pcEditView->m_pShareData->m_Common.m_bSelectedArea	= 0;
			}
			else if (Index == F_REPLACE_ALL) {
				//	全置換の時は選べる？
				pcEditView->m_pShareData->m_Common.m_bSelectedArea	= lFlag & 0x80 ? 1 : 0;
			}
			pcEditView->m_pcEditDoc->m_cDlgReplace.m_nReplaceTarget	= lFlag >> 8;	//	8bitシフト（0x100で割り算）
			//	コマンド発行
			pcEditView->HandleCommand( Index, FALSE, 0, 0, 0, 0);
		}
		break;
	case F_GREP:
		//	Argument[0]	検索文字列
		//	Argument[1]	検索対象にするファイル名
		//	Argument[2]	検索対象にするフォルダ名
		//	Argument[3]:
		//		次の数値の和。
		//		0x01	サブフォルダからも検索する
		//		0x02	この編集中のテキストから検索する（未実装）
		//		0x04	英大文字と英小文字を区別する
		//		0x08	正規表現
		//		0x10	文字コード自動判別
		//		******** 以下「結果出力」 ********
		//		0x00	該当行
		//		0x20	該当部分
		//		**********************************
		//		******** 以下「出力形式」 ********
		//		0x00	ノーマル
		//		0x40	ファイル毎
		//		**********************************
		{
			//	常に外部ウィンドウに。
			/*======= Grepの実行 =============*/
			/* Grep結果ウィンドウの表示 */
			char	pCmdLine[1024];
			char	pOpt[64];
			int		nDataLen;
			CMemory cmWork1;	cmWork1.SetDataSz( Argument[0] );	cmWork1.Replace( "\"", "\"\"" );	//	検索文字列
			CMemory cmWork2;	cmWork2.SetDataSz( Argument[1] );	cmWork2.Replace( "\"", "\"\"" );	//	ファイル名
			CMemory cmWork3;	cmWork3.SetDataSz( Argument[2] );	cmWork3.Replace( "\"", "\"\"" );	//	フォルダ名
			
			LPARAM lFlag = atoi(Argument[3]);
			/*
			|| -GREPMODE -GKEY="1" -GFILE="*.*;*.c;*.h" -GFOLDER="c:\" -GOPT=S
			*/
			wsprintf( pCmdLine, "-GREPMODE -GKEY=\"%s\" -GFILE=\"%s\" -GFOLDER=\"%s\"",
				cmWork1.GetPtr( &nDataLen ),
				cmWork2.GetPtr( &nDataLen ),
				cmWork3.GetPtr( &nDataLen )
			);
			pOpt[0] = '\0';
			if( lFlag & 0x01 ){	/* サブフォルダからも検索する */
				strcat( pOpt, "S" );
			}
		//	if( lFlag & 0x02 ){	/* この編集中のテキストから検索する */
		//
		//	}
			if( lFlag & 0x04 ){	/* 英大文字と英小文字を区別する */
				strcat( pOpt, "L" );
			}
			if( lFlag & 0x08 ){	/* 正規表現 */
				strcat( pOpt, "R" );
			}
			if( lFlag & 0x10 ){	/* 文字コード自動判別 */
				strcat( pOpt, "K" );
			}
			if( lFlag & 0x20 ){	/* 行を出力するか該当部分だけ出力するか */
				strcat( pOpt, "P" );
			}
			if( lFlag & 0x40 ){	/* Grep: 出力形式 */
				strcat( pOpt, "2" );
			}
			else {
				strcat( pOpt, "1" );
			}
			if( 0 < lstrlen( pOpt ) ){
				strcat( pCmdLine, " -GOPT=" );
				strcat( pCmdLine, pOpt );
			}
			/* 新規編集ウィンドウの追加 ver 0 */
			CEditApp::OpenNewEditor( pcEditView->m_hInstance, pcEditView->m_pShareData->m_hwndTray, pCmdLine, 0, FALSE );
			/*======= Grepの実行 =============*/
			/* Grep結果ウィンドウの表示 */
		}
		break;
	case F_FILEOPEN:
		//	Argument[0]を開く。
		{
			pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], 0, 0, 0);
		}
		break;
	case F_FILESAVEAS:
		//	Argument[0]を開く。
		{
			pcEditView->m_pcEditDoc->m_nCharCode = atoi(Argument[1]);
			switch (atoi(Argument[2])){
			case 0:
				pcEditView->m_pcEditDoc->m_cSaveLineCode = EOL_NONE;
				break;
			case 1:
				pcEditView->m_pcEditDoc->m_cSaveLineCode = EOL_CRLF;
				break;
			case 2:
				pcEditView->m_pcEditDoc->m_cSaveLineCode = EOL_LF;
				break;
			case 3:
				pcEditView->m_pcEditDoc->m_cSaveLineCode = EOL_CR;
				break;
			}
			pcEditView->HandleCommand( Index, FALSE, (LPARAM)Argument[0], 0, 0, 0);
		}
		break;
	default:
		//	引数なし。
		pcEditView->HandleCommand( Index, FALSE, 0, 0, 0, 0 );	//	標準
		break;
	}
}

/*[EOF]*/
