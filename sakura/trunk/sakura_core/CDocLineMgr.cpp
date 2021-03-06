/*!	@file
	@brief 行データの管理

	@author Norio Nakatani
	@date 1998/03/05  新規作成
	@date 2001/06/23 N.Nakatani 単語単位で検索する機能を実装
	@date 2001/06/23 N.Nakatani WhereCurrentWord()変更 WhereCurrentWord_2をコールするようにした
	@date 2005/09/25 D.S.Koba GetSizeOfCharで書き換え
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta, ao
	Copyright (C) 2001, genta, jepro, hor
	Copyright (C) 2002, hor, aroka, MIK, Moca, genta, frozen, Azumaiya, YAZAKI
	Copyright (C) 2003, Moca, ryoji, genta, かろと
	Copyright (C) 2004, genta, Moca
	Copyright (C) 2005, D.S.Koba, ryoji, かろと
	Copyright (C) 2008, syat
	Copyright (C) 2010, Moca

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "StdAfx.h"
#include <stdio.h>
#include <io.h>
#include <commctrl.h>
#include "CDocLineMgr.h"
#include "Debug.h"
#include "charcode.h"
//	Jun. 26, 2001 genta	正規表現ライブラリの差し替え
#include "CBregexp.h"
#include "global.h"
#include "etc_uty.h"
#include "file.h"
#include "CRunningTimer.h"

//	May 15, 2000 genta
#include "CEol.h"
#include "CDocLine.h"// 2002/2/10 aroka ヘッダ整理
#include "CMemory.h"// 2002/2/10 aroka

#include "CFileWrite.h" //2002/05/22 Frozen
#include "CFileLoad.h" // 2002/08/30 Moca
#include "CShareData.h" // 2010/03/03 Moca

#include "my_icmp.h" // Nov. 29, 2002 genta/moca





CDocLineMgr::CDocLineMgr()
{
	Init();
}

CDocLineMgr::~CDocLineMgr()
{
	Empty();
}





void CDocLineMgr::Init()
{
	m_pDocLineTop = NULL;
	m_pDocLineBot = NULL;
	m_nLines = 0;
	m_nPrevReferLine = 0;
	m_pCodePrevRefer = NULL;
	m_bIsDiffUse = false;	/* DIFF使用中 */	//@@@ 2002.05.25 MIK
	return;
}

//! 全ての行を削除する
void CDocLineMgr::Empty()
{
	CDocLine* pDocLine;
	CDocLine* pDocLineNext;
	pDocLine = m_pDocLineTop;
	while( pDocLine ){
		pDocLineNext = pDocLine->m_pNext;
		delete pDocLine;
		pDocLine = pDocLineNext;
	}
}


const char* CDocLineMgr::GetLineStr( int nLine, int* pnLineLen )
{
	CDocLine* pDocLine;
	pDocLine = GetLine( nLine );
	if( NULL == pDocLine ){
		*pnLineLen = 0;
		return NULL;
	}
	// 2002/2/10 aroka CMemory のメンバ変数に直接アクセスしない(inline化されているので速度的な問題はない)
	return pDocLine->m_cLine.GetStringPtr( pnLineLen );
}


/*!
	指定された行番号の文字列と改行コードを除く長さを取得
	
	@author Moca
	@date 2003.06.22
*/
const char* CDocLineMgr::GetLineStrWithoutEOL( int nLine, int* pnLineLen )
{
	const CDocLine* pDocLine = GetLine( nLine );
	if( NULL == pDocLine ){
		*pnLineLen = 0;
		return NULL;
	}
	*pnLineLen = pDocLine->GetLengthWithoutEOL();
	return pDocLine->m_cLine.GetStringPtr();
}

/*!
	指定された番号の行へのポインタを返す

	@param nLine [in] 行番号
	@return 行オブジェクトへのポインタ。該当行がない場合はNULL。
*/
CDocLine* CDocLineMgr::GetLine( int nLine )
{
	int nCounter;
	CDocLine* pDocLine;
	if( 0 == m_nLines ){
		return NULL;
	}
	// 2004.03.28 Moca nLineが負の場合のチェックを追加
	if( 0 > nLine || nLine >= m_nLines ){
		return NULL;
	}
	// 2004.03.28 Moca m_pCodePrevReferより、Top,Botのほうが近い場合は、そちらを利用する
	int nPrevToLineNumDiff = abs( m_nPrevReferLine - nLine );
	if( m_pCodePrevRefer == NULL
	  || nLine < nPrevToLineNumDiff
	  || m_nLines - nLine < nPrevToLineNumDiff
	){
		if( m_pCodePrevRefer == NULL ){
			MY_RUNNINGTIMER( cRunningTimer, "CDocLineMgr::GetLine() 	m_pCodePrevRefer == NULL" );
		}

		if( nLine < (m_nLines / 2) ){
			nCounter = 0;
			pDocLine = m_pDocLineTop;
			while( pDocLine ){
				if( nLine == nCounter ){
					m_nPrevReferLine = nLine;
					m_pCodePrevRefer = pDocLine;
					m_pDocLineCurrent = pDocLine->m_pNext;
					return pDocLine;
				}
				pDocLine = pDocLine->m_pNext;
				nCounter++;
			}
		}
		else{
			nCounter = m_nLines - 1;
			pDocLine = m_pDocLineBot;
			while( NULL != pDocLine ){
				if( nLine == nCounter ){
					m_nPrevReferLine = nLine;
					m_pCodePrevRefer = pDocLine;
					m_pDocLineCurrent = pDocLine->m_pNext;
					return pDocLine;
				}
				pDocLine = pDocLine->m_pPrev;
				nCounter--;
			}
		}

	}
	else{
		if( nLine == m_nPrevReferLine ){
			m_nPrevReferLine = nLine;
			m_pDocLineCurrent = m_pCodePrevRefer->m_pNext;
			return m_pCodePrevRefer;
		}
		else if( nLine > m_nPrevReferLine ){
			nCounter = m_nPrevReferLine + 1;
			pDocLine = m_pCodePrevRefer->m_pNext;
			while( NULL != pDocLine ){
				if( nLine == nCounter ){
					m_nPrevReferLine = nLine;
					m_pCodePrevRefer = pDocLine;
					m_pDocLineCurrent = pDocLine->m_pNext;
					return pDocLine;
				}
				pDocLine = pDocLine->m_pNext;
				++nCounter;
			}
		}
		else{
			nCounter = m_nPrevReferLine - 1;
			pDocLine = m_pCodePrevRefer->m_pPrev;
			while( NULL != pDocLine ){
				if( nLine == nCounter ){
					m_nPrevReferLine = nLine;
					m_pCodePrevRefer = pDocLine;
					m_pDocLineCurrent = pDocLine->m_pNext;
					return pDocLine;
				}
				pDocLine = pDocLine->m_pPrev;
				nCounter--;
			}
		}
	}
	return NULL;
}




/*! 順アクセスモード：先頭行を得る

	@param pnLineLen [out] 行の長さが返る。
	@return 1行目の先頭へのポインタ。
	データが1行もないときは、長さ0、ポインタNULLが返る。

*/
const char* CDocLineMgr::GetFirstLinrStr( int* pnLineLen )
{
	char* pszLine;
	if( 0 == m_nLines ){
		pszLine = NULL;
		*pnLineLen = 0;
	}else{
		pszLine = m_pDocLineTop->m_cLine.GetStringPtr( pnLineLen );

		m_pDocLineCurrent = m_pDocLineTop->m_pNext;
	}
	return (const char*)pszLine;
}





/*!
	順アクセスモード：次の行を得る

	@param pnLineLen [out] 行の長さが返る。
	@return 次行の先頭へのポインタ。
	GetFirstLinrStr()が呼び出されていないとNULLが返る

*/
const char* CDocLineMgr::GetNextLinrStr( int* pnLineLen )
{
	char* pszLine;
	if( NULL == m_pDocLineCurrent ){
		pszLine = NULL;
		*pnLineLen = 0;
	}else{
		pszLine = m_pDocLineCurrent->m_cLine.GetStringPtr( pnLineLen );

		m_pDocLineCurrent = m_pDocLineCurrent->m_pNext;
	}
	return (const char*)pszLine;
}

/*!
	末尾に行を追加

	@version 1.5

	@param pData [in] 追加する文字列へのポインタ
	@param nDataLen [in] 文字列の長さ
	@param cEol [in] 行末コード

*/
void CDocLineMgr::AddLineStrX( const char* pData, int nDataLen, CEol cEol )
{
	CDocLine* pDocLine;
	if( 0 == m_nLines ){
		m_pDocLineBot = m_pDocLineTop = new CDocLine;
		m_pDocLineTop->m_pPrev = NULL;
		m_pDocLineTop->m_pNext = NULL;

		m_pDocLineTop->m_cEol = cEol;	/* 改行コードの種類 */
		m_pDocLineTop->m_cLine.SetString( pData, nDataLen );
	}else{
		pDocLine = new CDocLine;
		pDocLine->m_pPrev = m_pDocLineBot;
		pDocLine->m_pNext = NULL;

		pDocLine->m_cEol = cEol;	/* 改行コードの種類 */
		pDocLine->m_cLine.SetString( pData, nDataLen );
		m_pDocLineBot->m_pNext = pDocLine;
		m_pDocLineBot = pDocLine;
	}
	++m_nLines;
}

/*!
	ファイルを読み込んで格納する（分割読み込みテスト版）
	@version	2.0
	@note	Windows用にコーディングしてある
	@param	nFlags [in]			bit 0: MIME Encodeされたヘッダをdecodeするかどうか
	@param	hWndParent [in]		親ウィンドウのハンドル
	@param	hwndProgress [in]	Progress barのウィンドウハンドル
	@retval	TRUE	正常読み込み
	@retval	FALSE	エラー(またはユーザによるキャンセル?)
	@date	2002/08/30 Moca 旧ReadFileを元に作成 ファイルアクセスに関する部分をCFileLoadで行う
	@date	2003/07/26 ryoji BOMの状態の取得を追加
*/
int CDocLineMgr::ReadFile( const char* pszPath, HWND hWndParent, HWND hwndProgress, ECodeType nCharCode, CFileTime* pcFileTime, int nFlags, bool* pbBomExist)
{
	DEBUG_TRACE( _T("pszPath=[%s]\n"), pszPath );
	MY_RUNNINGTIMER( cRunningTimer, "CDocLineMgr::ReadFile" );
	int			nRetVal = TRUE;
	int			nLineNum = 0;
	//	May 15, 2000 genta
	CEol cEol;
	CFileLoad cfl; 	//	2002/08/30 Moca Add
	const char*	pLine;
	int			nLineLen;

	/* 既存データのクリア */
	Empty();
	Init();

	/* 処理中のユーザー操作を可能にする */
	if( !::BlockingHook( NULL ) ){
		return FALSE;
	}

	try{
	// ファイルを開く
	// ファイルを閉じるにはFileCloseメンバ又はデストラクタのどちらかで処理できます
	//	Jul. 28, 2003 ryoji BOMパラメータ追加
	cfl.FileOpen( pszPath, nCharCode, nFlags, pbBomExist );

	/* ファイル時刻の取得 */
	FILETIME	FileTime;
	if( cfl.GetFileTime( NULL, NULL, &FileTime ) ){
		pcFileTime->SetFILETIME(FileTime);
	}

	// ファイルサイズチェック(ANSI版)
	if( CShareData::getInstance()->GetShareData()->m_Common.m_sFile.m_bAlertIfLargeFile ){
		int nFileMBSize = CShareData::getInstance()->GetShareData()->m_Common.m_sFile.m_nAlertFileSize;
		// m_Common.m_nAlertFileSize はMB単位
		if( cfl.GetFileSize() >> 20 >= nFileMBSize ){
			int nRet = MYMESSAGEBOX( hWndParent,
				MB_ICONQUESTION | MB_YESNO | MB_TOPMOST,
				GSTR_APPNAME,
				"ファイルサイズが%dMB以上あります。開きますか？",
				nFileMBSize );
			if( nRet != IDYES ){
				return FALSE;
			}
		}
	}
	
	if( NULL != hwndProgress ){
		::SendMessage( hwndProgress, PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );
		::SendMessage( hwndProgress, PBM_SETPOS, 0, 0 );
	}

	// ReadLineはファイルから 文字コード変換された1行を読み出します
	// エラー時はthrow CError_FileRead を投げます
	while( NULL != ( pLine = cfl.ReadLine( &nLineLen, &cEol ) ) ){
		++nLineNum;
		AddLineStrX( pLine, nLineLen, cEol );
		if( NULL != hwndProgress && 0 == ( nLineNum % 512 ) ){
			::SendMessage( hwndProgress, PBM_SETPOS, cfl.GetPercent(), 0 );
			/* 処理中のユーザー操作を可能にする */
			if( !::BlockingHook( NULL ) ){
				return FALSE;
			}
		}
	}

	// ファイルをクローズする
	cfl.FileClose();
	} // try
	catch( CError_FileOpen ){
		nRetVal = FALSE;
		if( !fexist( pszPath )){
			// ファイルがない
			ErrorMessage(hWndParent,
				_T("%s\nというファイルを開けません。\nファイルが存在しません。"),	//Mar. 24, 2001 jepro 若干修正
				pszPath
			 );
		}
		else if( -1 == _taccess( pszPath, 4 )){
			// 読み込みアクセス権がない
			ErrorMessage(
				hWndParent,
				_T("\'%s\'\nというファイルを開けません。\n読み込みアクセス権がありません。"),
				pszPath
			 );
		}
//		else if( ファイルサイズ > 2GB ){
//			nRetVal = FALSE;
//			::MYMESSAGEBOX(
//				hWndParent, MB_OK | MB_ICONSTOP, GSTR_APPNAME,
//				_("\'%s\'\nというファイルを開けません。\n2GB以上のファイルサイズは開けません。"),
//				pszPath
//			 );
//		}
		else{
			ErrorMessage(
				hWndParent,
				_T("\'%s\'\nというファイルを開けません。\n他のアプリケーションで使用されている可能性があります。"),
				pszPath
			 );
		}
	}
	catch( CError_FileRead ){
		nRetVal = FALSE;
		ErrorMessage(
			hWndParent,
			_T("\'%s\'というファイルの読み込み中にエラーが発生しました。\nファイルの読み込みを中止します。"),
			pszPath
		 );
		/* 既存データのクリア */
		Empty();
		Init();
	} // 例外処理終わり

	if( NULL != hwndProgress ){
		::SendMessage( hwndProgress, PBM_SETPOS, 0, 0 );
	}
	/* 処理中のユーザー操作を可能にする */
	if( !::BlockingHook( NULL ) ){
		return FALSE;
	}

	/* 行変更状態をすべてリセット */
	ResetAllModifyFlag();
	return nRetVal;
}




/*! バッファ内容をファイルに書き出す (テスト用)

	@note Windows用にコーディングしてある
	@date 2003.07.26 ryoji BOM引数追加
*/
int CDocLineMgr::WriteFile(
	const char* pszPath,
	HWND hWndParent,
	HWND hwndProgress,
	int nCharCode,
	CFileTime* pFileTime,
	CEol cEol,				//!< [in]	使用する改行コード
	bool bBomExist			//!< [in]	ファイル先頭にBOMを付けるか
)
{
	const char*		pLine;
	int				nLineLen;
	int				nRetVal;
	int				nLineNumber;
	int				nWriteLen;
	CMemory			cmemBuf;
	CDocLine*		pCDocLine;

	/*
	|| バッファサイズの調整
	*/
	cmemBuf.AllocStringBuffer( 32000 );

	if( NULL != hwndProgress ){
		::SendMessage( hwndProgress, PBM_SETRANGE, 0, MAKELPARAM( 0, 100 ) );
		::SendMessage( hwndProgress, PBM_SETPOS, 0, 0 );
	}

	nRetVal = TRUE;

	try
	{
		{//この括弧に対応する閉じ括弧でfileのデストラクタが呼び出され、ファイルが閉じられます。 
			CFileWrite file(pszPath);// 2002/05/22 Frozen

			//	Jul. 26, 2003 ryoji bBomExitによってBOMを付けるかどうかを決める
			if (bBomExist) {
				switch( nCharCode ){
				case CODE_UNICODE:
					file.Write("\xff\xfe",sizeof(char)*2);
					break;
				case CODE_UNICODEBE:
					file.Write( "\xfe\xff", sizeof(char) * 2 );
					break;
				case CODE_UTF8: // 2003.05.04 Moca BOMの間違いを訂正
					file.Write( "\xef\xbb\xbf", sizeof(char) * 3 );
					break;
				default:
					//	genta ここに来るのはバグだ
					;
				}
			}

			nLineNumber = 0;
			pCDocLine = m_pDocLineTop;

			while( NULL != pCDocLine ){
				++nLineNumber;
				pLine = pCDocLine->m_cLine.GetStringPtr( &nLineLen );


				if( NULL != hwndProgress && 0 < m_nLines && 0 == ( nLineNumber % 1024 ) ){
					::SendMessage( hwndProgress, PBM_SETPOS, nLineNumber * 100 / m_nLines , 0 );
					// 処理中のユーザー操作を可能にする
					if( !::BlockingHook( NULL ) ){
						return FALSE;
					}
				}

				nWriteLen = nLineLen - pCDocLine->m_cEol.GetLen();
				cmemBuf.SetString( "" );
				if( 0 < nWriteLen ){
					cmemBuf.SetString( pLine, nWriteLen );

					// 書き込み時のコード変換
					switch( nCharCode ){
					case CODE_UNICODE:
						/* SJIS→Unicodeコード変換 */
						cmemBuf.SJISToUnicode();
						break;
					case CODE_UTF8:	/* UTF-8 */
						/* SJIS→UTF-8コード変換 */
						cmemBuf.SJISToUTF8();
						break;
					case CODE_UTF7:	/* UTF-7 */
						/* SJIS→UTF-7コード変換 */
						cmemBuf.SJISToUTF7();
						break;
					case CODE_EUC:
						/* SJIS→EUCコード変換 */
						cmemBuf.SJISToEUC();
						break;
					case CODE_JIS:
						/* SJIS→JISコード変換 */
						cmemBuf.SJIStoJIS();
						break;
					case CODE_UNICODEBE:
						/* SJIS→UnicodeBEコード変換 */
						cmemBuf.SJISToUnicodeBE();
						break;
					case CODE_SJIS:
					default:
						break;
					}
				}
				if( EOL_NONE != pCDocLine->m_cEol ){

// 2002/05/09 Frozen ここから
					if( nCharCode == CODE_UNICODE ){
						if( cEol==EOL_NONE )
							cmemBuf.AppendString( pCDocLine->m_cEol.GetUnicodeValue(), pCDocLine->m_cEol.GetLen()*sizeof(wchar_t));
						else
							cmemBuf.AppendString( cEol.GetUnicodeValue(), cEol.GetLen()*sizeof(wchar_t));
					}else if( nCharCode == CODE_UNICODEBE ){
						/* UnicodeBE の改行コード設定 Moca, 2002/05/26 */
						if( cEol == EOL_NONE ) /*  */
							cmemBuf.AppendString( pCDocLine->m_cEol.GetUnicodeBEValue(), pCDocLine->m_cEol.GetLen()*sizeof(wchar_t) );
						else
							cmemBuf.AppendString( cEol.GetUnicodeBEValue(), cEol.GetLen()*sizeof(wchar_t) );
					}else{
						if( cEol == EOL_NONE )
							cmemBuf.AppendString(pCDocLine->m_cEol.GetValue(),pCDocLine->m_cEol.GetLen());
						else
							cmemBuf.AppendString(cEol.GetValue(),cEol.GetLen());
					}
// 2002/05/09 Frozen ここまで
				}
				if( 0 < cmemBuf.GetStringLength() )
					file.Write(cmemBuf.GetStringPtr(),sizeof(char)*cmemBuf.GetStringLength());//2002/05/22 Frozen gotoの次の}までをこの一行で置き換え


				pCDocLine = pCDocLine->m_pNext;

			}
		}//この括弧でCFileWriteのデストラクタが呼び出され、ファイルが閉じられます。

		/* 更新後のファイル時刻の取得
		 * CloseHandle前ではFlushFileBuffersを呼んでもタイムスタンプが更新
		 * されないことがある。
		 */

		// 2005.10.20 ryoji FindFirstFileを使うように変更（ファイルがロックされていてもタイムスタンプ取得可能）
		CFileTime cftime;
		if( GetLastWriteTimestamp( pszPath, &cftime )){
			*pFileTime = cftime;
		}

	}
	catch(CError_FileOpen){
		ErrorMessage(
			hWndParent,
			_T("\'%s\'\nファイルを保存できません。\n")
			_T("パスが存在しないか、他のアプリケーションで使用されている可能性があります。"),
			pszPath
		);
		nRetVal = FALSE;
	}
	catch(CError_FileWrite){
		nRetVal = FALSE;
	}

	if( NULL != hwndProgress ){
		::SendMessage( hwndProgress, PBM_SETPOS, 0, 0 );
		/* 処理中のユーザー操作を可能にする */
		if( !::BlockingHook( NULL ) ){
			return FALSE;
		}
	}

	return nRetVal;
}




/* データの削除 */
/*
|| 指定行内の文字しか削除できません
|| データ変更によって影響のあった、変更前と変更後の行の範囲を返します
|| この情報をもとに、レイアウト情報などを更新してください。
||
	@date 2002/03/24 YAZAKI bUndo削除
*/
void CDocLineMgr::DeleteData_CDocLineMgr(
	int			nLine,
	int			nDelPos,
	int			nDelLen,
	int*		pnModLineOldFrom,	//!< 影響のあった変更前の行(from)
	int*		pnModLineOldTo,		//!< 影響のあった変更前の行(to)
	int*		pnDelLineOldFrom,	//!< 削除された変更前論理行(from)
	int*		pnDelLineOldNum,	//!< 削除された行数
	CMemory*	cmemDeleted			//!< [out] 削除されたデータ
)
{
#ifdef _DEBUG
	CRunningTimer cRunningTimer( "CDocLineMgr::DeleteData" );
#endif
	CDocLine*	pDocLine;
	CDocLine*	pDocLine2;
	char*		pData;
	int			nDeleteLength;
	char*		pLine;
	int			nLineLen;
	char*		pLine2;
	int			nLineLen2;

	*pnModLineOldFrom = nLine;	/* 影響のあった変更前の行(from) */
	*pnModLineOldTo = nLine;	/* 影響のあった変更前の行(to) */
	*pnDelLineOldFrom = 0;		/* 削除された変更前論理行(from) */
	*pnDelLineOldNum = 0;		/* 削除された行数 */
//	cmemDeleted.SetData( "", lstrlen( "" ) );
	cmemDeleted->SetString( "" );

	pDocLine = GetLine( nLine );
	if( NULL == pDocLine ){
		return;
	}

	pDocLine->SetModifyFlg(true);		/* 変更フラグ */

	pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );

	if( nDelPos >= nLineLen ){
		return;
	}
	/* 「改行」を削除する場合は、次の行と連結する */
//	if( ( nDelPos == nLineLen -1 && ( pLine[nDelPos] == CR || pLine[nDelPos] == LF ) )
//	 || nDelPos + nDelLen >= nLineLen
	if( ( EOL_NONE != pDocLine->m_cEol && nDelPos == nLineLen - pDocLine->m_cEol.GetLen() )
	 || ( EOL_NONE != pDocLine->m_cEol && nDelPos + nDelLen >  nLineLen - pDocLine->m_cEol.GetLen() )
	 || ( EOL_NONE == pDocLine->m_cEol && nDelPos + nDelLen >= nLineLen - pDocLine->m_cEol.GetLen() )
	){
		/* 実際に削除するバイト数 */
		nDeleteLength = nLineLen - nDelPos;

		/* 削除されるデータ */
		cmemDeleted->SetString( &pLine[nDelPos], nDeleteLength );

		/* 次の行の情報 */
		pDocLine2 = pDocLine->m_pNext;
		if( !pDocLine2 ){
			pData = new char[nLineLen + 1];
			if( nDelPos > 0 ){
				memcpy( pData, pLine, nDelPos );
			}
			if( 0 < nLineLen - ( nDelPos + nDeleteLength ) ){
				memcpy(
					pData + nDelPos,
					pLine + nDelPos + nDeleteLength,
					nLineLen - ( nDelPos + nDeleteLength )
				);
			}
			pData[ nLineLen - nDeleteLength ] = '\0';
			/* 改行コードの情報を更新 */
			pDocLine->m_cEol.SetType( EOL_NONE );

			if( 0 < nLineLen - nDeleteLength ){
				pDocLine->m_cLine.SetString( pData, nLineLen - nDeleteLength );
			}else{
				// 行の削除
				// 2004.03.18 Moca 関数を使う
				DeleteNode( pDocLine );
				pDocLine = NULL;
				*pnDelLineOldFrom = nLine;	/* 削除された変更前論理行(from) */
				*pnDelLineOldNum = 1;		/* 削除された行数 */
			}
			delete [] pData;
		}
		else{
			*pnModLineOldTo = nLine + 1;	/* 影響のあった変更前の行(to) */
			pLine2 = pDocLine2->m_cLine.GetStringPtr( &nLineLen2 );
			pData = new char[nLineLen + nLineLen2 + 1];
			if( nDelPos > 0 ){
				memcpy( pData, pLine, nDelPos );
			}
			if( 0 < nLineLen - ( nDelPos + nDeleteLength ) ){
				memcpy(
					pData + nDelPos,
					pLine + nDelPos + nDeleteLength,
					nLineLen - ( nDelPos + nDeleteLength )
				);
			}
			/* 次の行のデータを連結 */
			memcpy( pData + (nLineLen - nDeleteLength), pLine2, nLineLen2 );
			pData[ nLineLen - nDeleteLength + nLineLen2 ] = '\0';
			pDocLine->m_cLine.SetString( pData, nLineLen - nDeleteLength + nLineLen2 );
			/* 改行コードの情報を更新 */
			pDocLine->m_cEol = pDocLine2->m_cEol;

			/* 次の行を削除 && 次次行とのリストの連結*/
			// 2004.03.18 Moca DeleteNode を使う
			DeleteNode( pDocLine2 );
			pDocLine2 = NULL;
			*pnDelLineOldFrom = nLine + 1;	/* 削除された変更前論理行(from) */
			*pnDelLineOldNum = 1;			/* 削除された行数 */
			delete [] pData;
		}
	}
	else{
		/* 実際に削除するバイト数 */
		nDeleteLength = nDelLen;

		/* 削除されるデータ */
		cmemDeleted->SetString( &pLine[nDelPos], nDeleteLength );

		pData = new char[nLineLen + 1];
		if( nDelPos > 0 ){
			memcpy( pData, pLine, nDelPos );
		}
		if( 0 < nLineLen - ( nDelPos + nDeleteLength ) ){
			memcpy(
				pData + nDelPos,
				pLine + nDelPos + nDeleteLength,
				nLineLen - ( nDelPos + nDeleteLength )
			);
		}
		pData[ nLineLen - nDeleteLength ] = '\0';
		if( 0 < nLineLen - nDeleteLength ){
			pDocLine->m_cLine.SetString( pData, nLineLen - nDeleteLength );
		}
		delete [] pData;
	}
}





/*!	データの挿入

	@date 2002/03/24 YAZAKI bUndo削除
*/
void CDocLineMgr::InsertData_CDocLineMgr(
	int			nLine,
	int			nInsPos,
	const char*	pInsData,
	int			nInsDataLen,
	int*		pnInsLineNum,	// 挿入によって増えた行の数
	int*		pnNewLine,		// 挿入された部分の次の位置の行
	int*		pnNewPos		// 挿入された部分の次の位置のデータ位置
)
{
	CDocLine*	pDocLine;
	CDocLine*	pDocLineNew;
	char*		pLine;
	int			nLineLen;
	CMemory		cmemPrevLine;
	CMemory		cmemCurLine;
	CMemory		cmemNextLine;
	int			nAllLinesOld = m_nLines;
	int			nCount;

	//	May 15, 2000 genta
	CEol 		cEOLType;
	CEol 		cEOLTypeNext;

	bool		bBookMarkNext;	// 2001.12.03 hor 挿入によるマーク行の制御

	*pnNewLine = nLine;	/* 挿入された部分の次の位置の行 */
	//	Jan. 25, 2004 genta
	//	挿入文字列長が0の場合に最後までpnNewPosが設定されないので
	//	初期値として0ではなく開始位置と同じ値を入れておく．
	*pnNewPos  = nInsPos;		/* 挿入された部分の次の位置のデータ位置 */

	/* 挿入データを行終端で区切った行数カウンタ */
	nCount = 0;
	*pnInsLineNum = 0;
	pDocLine = GetLine( nLine );
	if( !pDocLine ){
		/* ここでNULLが帰ってくるということは、*/
		/* 全テキストの最後の次の行を追加しようとしていることを示す */
		cmemPrevLine.SetString( "" );
		cmemNextLine.SetString( "" );
		cEOLTypeNext.SetType( EOL_NONE );
		bBookMarkNext=false;	// 2001.12.03 hor
	}
	else{
		pDocLine->SetModifyFlg(true);		/* 変更フラグ */

		pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );
		cmemPrevLine.SetString( pLine, nInsPos );
		cmemNextLine.SetString( &pLine[nInsPos], nLineLen - nInsPos );

		cEOLTypeNext = pDocLine->m_cEol;
		bBookMarkNext= pDocLine->IsBookmarked();	// 2001.12.03 hor
	}

	int	nBgn = 0;
	int	nPos = 0;
	for( nPos = 0; nPos < nInsDataLen; ){
		if( pInsData[nPos] == '\n' || pInsData[nPos] == '\r' ){
			/* 行終端子の種類を調べる */
			cEOLType.SetTypeByString( &pInsData[nPos], nInsDataLen - nPos );
			/* 行終端子も含めてテキストをバッファに格納 */
			cmemCurLine.SetString( &pInsData[nBgn], nPos - nBgn + cEOLType.GetLen() );
			nBgn = nPos + cEOLType.GetLen();
			nPos = nBgn;
			if( NULL == pDocLine ){
				pDocLineNew = new CDocLine;

				/* 挿入データを行終端で区切った行数カウンタ */
				if( 0 == nCount ){
					if( NULL == m_pDocLineTop ){
						m_pDocLineTop = pDocLineNew;
					}
					pDocLineNew->m_pPrev = m_pDocLineBot;
					if( NULL != m_pDocLineBot ){
						m_pDocLineBot->m_pNext = pDocLineNew;
					}
					m_pDocLineBot = pDocLineNew;
					pDocLineNew->m_pNext = NULL;
					pDocLineNew->m_cLine.SetNativeData( &cmemPrevLine );
					pDocLineNew->m_cLine += cmemCurLine;

					pDocLineNew->m_cEol = cEOLType;							/* 改行コードの種類 */
				}
				else{
					if( NULL != m_pDocLineBot ){
						m_pDocLineBot->m_pNext = pDocLineNew;
					}
					pDocLineNew->m_pPrev = m_pDocLineBot;
					m_pDocLineBot = pDocLineNew;
					pDocLineNew->m_pNext = NULL;
					pDocLineNew->m_cLine.SetNativeData( &cmemCurLine );

					pDocLineNew->m_cEol = cEOLType;							/* 改行コードの種類 */
				}
				pDocLine = NULL;
				++m_nLines;
			}
			else{
				/* 挿入データを行終端で区切った行数カウンタ */
				if( 0 == nCount ){
					pDocLine->m_cLine.SetNativeData( &cmemPrevLine );
					pDocLine->m_cLine += cmemCurLine;

					pDocLine->m_cEol = cEOLType;						/* 改行コードの種類 */

					// 2001.12.13 hor
					// 行頭で改行したら元の行のマークを新しい行に移動する
					// それ以外なら元の行のマークを維持して新しい行にはマークを付けない
					if(nInsPos==0){
						pDocLine->SetBookMark(false);
					}
					else{
						bBookMarkNext=false;
					}

					pDocLine = pDocLine->m_pNext;
				}
				else{
					pDocLineNew = new CDocLine;
					pDocLineNew->m_pPrev = pDocLine->m_pPrev;
					pDocLineNew->m_pNext = pDocLine;
					pDocLine->m_pPrev->m_pNext = pDocLineNew;
					pDocLine->m_pPrev = pDocLineNew;
					pDocLineNew->m_cLine.SetNativeData( &cmemCurLine );
					pDocLineNew->m_cEol = cEOLType;							/* 改行コードの種類 */
					++m_nLines;
				}
			}

			/* 挿入データを行終端で区切った行数カウンタ */
			++nCount;
			++(*pnNewLine);	/* 挿入された部分の次の位置の行 */
		}
		else{
			++nPos;
		}
	}

	if( 0 < nPos - nBgn || 0 < cmemNextLine.GetStringLength() ){
		cmemCurLine.SetString( &pInsData[nBgn], nPos - nBgn );
		cmemCurLine += cmemNextLine;
		if( NULL == pDocLine ){
			pDocLineNew = new CDocLine;
			/* 挿入データを行終端で区切った行数カウンタ */
			if( 0 == nCount ){
				if( NULL == m_pDocLineTop ){
					m_pDocLineTop = pDocLineNew;
				}
				pDocLineNew->m_pPrev = m_pDocLineBot;
				if( NULL != m_pDocLineBot ){
					m_pDocLineBot->m_pNext = pDocLineNew;
				}
				m_pDocLineBot = pDocLineNew;
				pDocLineNew->m_pNext = NULL;
				pDocLineNew->m_cLine.SetNativeData( &cmemPrevLine );
				pDocLineNew->m_cLine += cmemCurLine;

				pDocLineNew->m_cEol = cEOLTypeNext;							/* 改行コードの種類 */

			}
			else{
				if( NULL != m_pDocLineBot ){
					m_pDocLineBot->m_pNext = pDocLineNew;
				}
				pDocLineNew->m_pPrev = m_pDocLineBot;
				m_pDocLineBot = pDocLineNew;
				pDocLineNew->m_pNext = NULL;
				pDocLineNew->m_cLine.SetNativeData( &cmemCurLine );

				pDocLineNew->m_cEol = cEOLTypeNext;							/* 改行コードの種類 */

			}
			pDocLine = NULL;
			++m_nLines;
			*pnNewPos = nPos - nBgn;	/* 挿入された部分の次の位置のデータ位置 */
		}
		else{
			/* 挿入データを行終端で区切った行数カウンタ */
			if( 0 == nCount ){
				pDocLine->m_cLine.SetNativeData( &cmemPrevLine );
				pDocLine->m_cLine += cmemCurLine;

				pDocLine->m_cEol = cEOLTypeNext;						/* 改行コードの種類 */

				pDocLine = pDocLine->m_pNext;
				*pnNewPos = cmemPrevLine.GetStringLength() + nPos - nBgn;		/* 挿入された部分の次の位置のデータ位置 */
			}
			else{
				pDocLineNew = new CDocLine;
				pDocLineNew->m_pPrev = pDocLine->m_pPrev;
				pDocLineNew->m_pNext = pDocLine;
				pDocLine->m_pPrev->m_pNext = pDocLineNew;
				pDocLine->m_pPrev = pDocLineNew;
				pDocLineNew->m_cLine.SetNativeData( &cmemCurLine );

				pDocLineNew->m_cEol = cEOLTypeNext;							/* 改行コードの種類 */
				pDocLineNew->SetBookMark(bBookMarkNext);	// 2001.12.03 hor ブックマークを復元


				++m_nLines;
				*pnNewPos = nPos - nBgn;	/* 挿入された部分の次の位置のデータ位置 */
			}
		}
	}
	*pnInsLineNum = m_nLines - nAllLinesOld;
}

/* 現在位置の単語の範囲を調べる */
// 2001/06/23 N.Nakatani WhereCurrentWord()変更 WhereCurrentWord_2をコールするようにした
bool CDocLineMgr::WhereCurrentWord(
	int			nLineNum,
	int			nIdx,
	int*		pnIdxFrom,
	int*		pnIdxTo,
	CMemory*	pcmcmWord,
	CMemory*	pcmcmWordLeft
)
{
	*pnIdxFrom = nIdx;
	*pnIdxTo = nIdx;
	CDocLine*	pDocLine = GetLine( nLineNum );
	if( NULL == pDocLine ){
		return false;
	}

	int			nLineLen;
	const char*	pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );

	/* 現在位置の単語の範囲を調べる */
	return CDocLineMgr::WhereCurrentWord_2( pLine, nLineLen, nIdx, pnIdxFrom, pnIdxTo, pcmcmWord, pcmcmWordLeft );
}



//@@@ 2001.06.23 N.Nakatani
/*!
	@brief 現在位置の単語の範囲を調べる staticメンバ
	@author N.Nakatani
	@retval true	成功 現在位置のデータは「単語」と認識する。
	@retval false	失敗 現在位置のデータは「単語」とは言いきれない気がする。
*/
bool CDocLineMgr::WhereCurrentWord_2(
	const char*	pLine,			//!< [in]  調べるメモリ全体の先頭アドレス
	int			nLineLen,		//!< [in]  調べるメモリ全体の有効長
	int			nIdx,			//!< [out] 調査開始地点:pLineからの相対的な位置
	int*		pnIdxFrom,		//!< [out] 単語が見つかった場合は、単語の先頭インデックスを返す。
	int*		pnIdxTo,		//!< [out] 単語が見つかった場合は、単語の終端の次のバイトの先頭インデックスを返す。
	CMemory*	pcmcmWord,		//!< [out] 単語が見つかった場合は、現在単語を切り出して指定されたCMemoryオブジェクトに格納する。情報が不要な場合はNULLを指定する。
	CMemory*	pcmcmWordLeft	//!< [out] 単語が見つかった場合は、現在単語の左に位置する単語を切り出して指定されたCMemoryオブジェクトに格納する。情報が不要な場合はNULLを指定する。
)
{
	*pnIdxFrom = nIdx;
	*pnIdxTo = nIdx;

	if( NULL == pLine ){
		return false;
	}
	if( nIdx >= nLineLen ){
		return false;
	}

	// 現在位置の文字の種類によっては選択不能
	if( pLine[nIdx] == CR || pLine[nIdx] == LF ){
		return false;
	}

	// 現在位置の文字の種類を調べる
	int	nCharKind = WhatKindOfChar( (char*)pLine, nLineLen, nIdx );

	// 文字種類が変わるまで前方へサーチ
	int	nIdxNext = nIdx;
	int	nCharChars = &pLine[nIdxNext] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nIdxNext] );
	while( nCharChars > 0 ){
		int	nIdxNextPrev = nIdxNext;
		nIdxNext -= nCharChars;
		int	nCharKindNext = WhatKindOfChar( (char*)pLine, nLineLen, nIdxNext );

		int nCharKindMerge = WhatKindOfTwoChars( nCharKindNext, nCharKind );
		if( nCharKindMerge == CK_NULL ){
			nIdxNext = nIdxNextPrev;
			break;
		}
		nCharKind = nCharKindMerge;
		nCharChars = &pLine[nIdxNext] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nIdxNext] );
	}
	*pnIdxFrom = nIdxNext;

	if( NULL != pcmcmWordLeft ){
		pcmcmWordLeft->SetString( &pLine[*pnIdxFrom], nIdx - *pnIdxFrom );
	}

	// 文字種類が変わるまで後方へサーチ
	nIdxNext = nIdx;
	nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxNext ); // 2005-09-02 D.S.Koba GetSizeOfChar
	while( nCharChars > 0 ){
		nIdxNext += nCharChars;
		int	nCharKindNext = WhatKindOfChar( (char*)pLine, nLineLen, nIdxNext );

		int nCharKindMerge = WhatKindOfTwoChars( nCharKindNext, nCharKind );
		if( nCharKindMerge == CK_NULL ){
			break;
		}
		nCharKind = nCharKindMerge;
		nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxNext ); // 2005-09-02 D.S.Koba GetSizeOfChar
	}
	*pnIdxTo = nIdxNext;

	if( NULL != pcmcmWord ){
		pcmcmWord->SetString( &pLine[*pnIdxFrom], *pnIdxTo - *pnIdxFrom );
	}
	return true;
}



/*!	次の単語の先頭を探す
	pLine（長さ：nLineLen）の文字列から単語を探す。
	探し始める位置はnIdxで指定。方向は後方に限定。単語の両端で止まらない（関係ないから）
*/
int CDocLineMgr::SearchNextWordPosition(
	const char* pLine,
	int			nLineLen,
	int			nIdx,		//	桁数
	int*		pnColumnNew,	//	見つかった位置
	BOOL		bStopsBothEnds	//	単語の両端で止まる
)
{
	/* 文字種類が変わるまで後方へサーチ */
	/* 空白とタブは無視する */

	/* 現在位置の文字の種類を調べる */
	int nCharKind = WhatKindOfChar( pLine, nLineLen, nIdx );

	int nIdxNext = nIdx;
	// 2005-09-02 D.S.Koba GetSizeOfChar
	int nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxNext );
	while( nCharChars > 0 ){
		nIdxNext += nCharChars;
		int nCharKindNext = WhatKindOfChar( pLine, nLineLen, nIdxNext );
		/* 空白とタブは無視する */
		if( nCharKindNext == CK_TAB || nCharKindNext == CK_SPACE ){
			if ( bStopsBothEnds && nCharKind != nCharKindNext ){
				*pnColumnNew = nIdxNext;
				return TRUE;
			}
			nCharKind = nCharKindNext;
		}
		else {
			int nCharKindMerge = WhatKindOfTwoChars( nCharKind, nCharKindNext );
			if( nCharKindMerge == CK_NULL ){
				*pnColumnNew = nIdxNext;
				return TRUE;
			}
			nCharKind = nCharKindMerge;
		}
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( pLine, nLineLen, nIdxNext );
	}
	return FALSE;
}




// 現在位置の左右の単語の先頭位置を調べる
int CDocLineMgr::PrevOrNextWord(
	int			nLineNum,		//	行数
	int			nIdx,			//	桁数
	int*		pnColumnNew,	//	見つかった位置
	BOOL		bLEFT,			//	TRUE:前方（左）へ向かう。FALSE:後方（右）へ向かう。
	BOOL		bStopsBothEnds	//	単語の両端で止まる
)
{
	CDocLine*	pDocLine;
	int			nCharKind;
	int			nCharKindNext;
	int			nIdxNext;
	int			nIdxNextPrev;
	int			nCharChars;
	int			nCount;
	pDocLine = GetLine( nLineNum );
	if( NULL == pDocLine ){
		return FALSE;
	}

	int			nLineLen;
	const char*		pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );

	// ABC D[EOF]となっていたときに、Dの後ろにカーソルを合わせ、単語の左端に移動すると、Aにカーソルがあうバグ修正。YAZAKI
	if( nIdx >= nLineLen ){
		if (bLEFT && nIdx == nLineLen){
		}
		else {
			nIdx = nLineLen - 1;
		}
	}
	/* 現在位置の文字の種類によっては選択不能 */
	if( !bLEFT && ( pLine[nIdx] == CR || pLine[nIdx] == LF ) ){
		return FALSE;
	}
	/* 前の単語か？後ろの単語か？ */
	if( bLEFT ){
		/* 現在位置の文字の種類を調べる */
		nCharKind = WhatKindOfChar( pLine, nLineLen, nIdx );
		if( nIdx == 0 ){
			return FALSE;
		}

		/* 文字種類が変わるまで前方へサーチ */
		/* 空白とタブは無視する */
		nCount = 0;
		nIdxNext = nIdx;
		nCharChars = &pLine[nIdxNext] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nIdxNext] );
		while( nCharChars > 0 ){
			nIdxNextPrev = nIdxNext;
			nIdxNext -= nCharChars;
			nCharKindNext = WhatKindOfChar( pLine, nLineLen, nIdxNext );

			int nCharKindMerge = WhatKindOfTwoChars( nCharKindNext, nCharKind );
			if( nCharKindMerge == CK_NULL ){
				/* サーチ開始位置の文字が空白またはタブの場合 */
				if( nCharKind == CK_TAB	|| nCharKind == CK_SPACE ){
					if ( bStopsBothEnds && nCount ){
						nIdxNext = nIdxNextPrev;
						break;
					}
					nCharKindMerge = nCharKindNext;
				}else{
					if( nCount == 0){
						nCharKindMerge = nCharKindNext;
					}else{
						nIdxNext = nIdxNextPrev;
						break;
					}
				}
			}
			nCharKind = nCharKindMerge;
			nCharChars = &pLine[nIdxNext] - CMemory::MemCharPrev( pLine, nLineLen, &pLine[nIdxNext] );
			++nCount;
		}
		*pnColumnNew = nIdxNext;
	}else{
		CDocLineMgr::SearchNextWordPosition(pLine, nLineLen, nIdx, pnColumnNew, bStopsBothEnds);
	}
	return TRUE;
}




/*! 単語検索

	@date 2003.05.22 かろと 行頭処理など見直し
	@date 2005.11.26 かろと \rや.が\r\nにヒットしないように
*/
/* 見つからない場合は０を返す */
int CDocLineMgr::SearchWord(
	int						nLineNum,		//!< 検索開始行
	int						nIdx, 			//!< 検索開始位置
	const char*				pszPattern,		//!< 検索条件
	ESearchDirection		eDirection,		//!< 検索方向
	const SSearchOption&	sSearchOption,	//!< 検索オプション
	int*					pnLineNum, 		//!< マッチ行
	int*					pnIdxFrom, 		//!< マッチ位置from
	int*					pnIdxTo,  		//!< マッチ位置to
	CBregexp*				pRegexp			//!< [in] 正規表現コンパイルデータ。既にコンパイルされている必要がある
)
{
	CDocLine*	pDocLine;
	int			nLinePos;
	int			nIdxPos;
	int			nIdxPosOld;
	const char*	pLine;
	int			nLineLen;
	char*		pszRes;
	int			nHitTo;
	int			nHitPos;
	int			nHitPosOld;
	int			nRetVal = 0;
	int*		pnKey_CharCharsArr;
	//	Jun. 10, 2003 Moca
	//	lstrlenを毎回呼ばずにnPatternLenを使うようにする
	const int	nPatternLen = lstrlen( pszPattern );	//2001/06/23 N.Nakatani
	pnKey_CharCharsArr = NULL;
	/* 検索条件の情報 */
	CDocLineMgr::CreateCharCharsArr(
		(const unsigned char *)pszPattern,
		nPatternLen,
		&pnKey_CharCharsArr
	);

	//正規表現
	if( sSearchOption.bRegularExp ){
		nLinePos = nLineNum;		// 検索行＝検索開始行
		pDocLine = GetLine( nLinePos );
		// 後方検索
		if( eDirection == SEARCH_BACKWARD ){
			//
			// 後方(↑)検索(正規表現)
			//
			nHitTo = nIdx;				// 検索開始位置
			nIdxPos = 0;
			while( NULL != pDocLine ){
				pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );
				nHitPos		= -1;	// -1:この行でマッチ位置なし
				while( 1 ){
					nHitPosOld = nHitPos;
					nIdxPosOld = nIdxPos;
					// 長さ０でマッチしたので、この位置で再度マッチしないように、１文字進める
					if (nIdxPos == nHitPos) {
						// 2005-09-02 D.S.Koba GetSizeOfChar
						nIdxPos += (CMemory::GetSizeOfChar( pLine, nLineLen, nIdxPos ) == 2 ? 2 : 1);
					}
					if (	nIdxPos <= pDocLine->GetLengthWithoutEOL() 
						&&	pRegexp->Match( pLine, nLineLen, nIdxPos ) ){
						// 検索にマッチした！
						nHitPos = pRegexp->GetIndex();
						nIdxPos = pRegexp->GetLastIndex();
						if( nHitPos >= nHitTo ){
							// マッチしたのは、カーソル位置以降だった
							// すでにマッチした位置があれば、それを返し、なければ前の行へ
							break;
						}
					} else {
						// マッチしなかった
						// すでにマッチした位置があれば、それを返し、なければ前の行へ
						break;
					}
				}

				if ( -1 != nHitPosOld ) {
					// この行でマッチした位置が存在するので、この行で検索終了
					*pnIdxFrom = nHitPosOld;			// マッチ位置from
					*pnIdxTo = nIdxPosOld;				// マッチ位置to
					break;
				} else {
					// この行でマッチした位置が存在しないので、前の行を検索へ
					nLinePos--;
					pDocLine = pDocLine->m_pPrev;
					nIdxPos = 0;
					if( NULL != pDocLine ){
						nHitTo = pDocLine->m_cLine.GetStringLength() + 1;		// 前の行のNULL文字(\0)にもマッチさせるために+1 2003.05.16 かろと 
					}
				}
			}
		}
		// 前方検索
		else {
			//
			// 前方検索(正規表現)
			//
			nIdxPos = nIdx;
			while( NULL != pDocLine ){
				pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );
				if(		nIdxPos <= pDocLine->GetLengthWithoutEOL() 
					&&	pRegexp->Match( pLine, nLineLen, nIdxPos ) ){
					// マッチした
					*pnIdxFrom = pRegexp->GetIndex();					// マッチ位置from
					*pnIdxTo = pRegexp->GetLastIndex();					// マッチ位置to
					break;
				}
				++nLinePos;
				pDocLine = pDocLine->m_pNext;
				nIdxPos = 0;
			}
		}
		//
		// 正規表現検索の後処理
		if ( pDocLine != NULL ) {
			// マッチした行がある
			*pnLineNum = nLinePos;				// マッチ行
			nRetVal = 1;
			// レイアウト行では改行文字内の位置を表現できないため、マッチ開始位置を補正
			if (*pnIdxFrom > pDocLine->GetLengthWithoutEOL()) {
				// \r\n改行時に\nにマッチすると置換できない不具合となるため
				// 改行文字内でマッチした場合、改行文字の始めからマッチしたことにする
				*pnIdxFrom = pDocLine->GetLengthWithoutEOL();
			}
		}
	}
	//単語のみ検索
	else if( sSearchOption.bWordOnly ){
		/*
			2001/06/23 Norio Nakatani
			単語単位の検索を試験的に実装。単語はWhereCurrentWord()で判別してますので、
			英単語やC/C++識別子などの検索条件ならヒットします。
		*/

		// 前方検索
		if( eDirection == SEARCH_BACKWARD ){
			nLinePos = nLineNum;
			pDocLine = GetLine( nLinePos );
			int nNextWordFrom;
			int nNextWordFrom2;
			int nNextWordTo2;
			int nWork;
			nNextWordFrom = nIdx;
			while( NULL != pDocLine ){
				if( PrevOrNextWord( nLinePos, nNextWordFrom, &nWork, TRUE, FALSE ) ){
					nNextWordFrom = nWork;
					if( WhereCurrentWord( nLinePos, nNextWordFrom, &nNextWordFrom2, &nNextWordTo2 , NULL, NULL ) ){
						if( nPatternLen == nNextWordTo2 - nNextWordFrom2 ){
							const char* pData = pDocLine->m_cLine.GetStringPtr();	// 2002/2/10 aroka CMemory変更
							/* 1==大文字小文字の区別 */
							if( (!sSearchOption.bLoHiCase && 0 == my_memicmp( &(pData[nNextWordFrom2]) , pszPattern, nPatternLen ) ) ||
								(sSearchOption.bLoHiCase && 0 == memcmp( &(pData[nNextWordFrom2]) , pszPattern, nPatternLen ) )
							){
								*pnLineNum = nLinePos;	// マッチ行
								*pnIdxFrom = nNextWordFrom2;	// マッチ位置from
								*pnIdxTo = *pnIdxFrom + nPatternLen;	// マッチ位置to
								nRetVal = 1;
								goto end_of_func;
							}
						}
						continue;
					}
				}
				/* 前の行を見に行く */
				nLinePos--;
				pDocLine = pDocLine->m_pPrev;
				if( NULL != pDocLine ){
					nNextWordFrom = pDocLine->m_cLine.GetStringLength() - pDocLine->m_cEol.GetLen();
					if( 0 > nNextWordFrom ){
						nNextWordFrom = 0;
					}
				}
			}
		}
		// 後方検索
		else{
			nLinePos = nLineNum;
			pDocLine = GetLine( nLinePos );
			int nNextWordFrom;

			int nNextWordFrom2;
			int nNextWordTo2;
			nNextWordFrom = nIdx;
			while( NULL != pDocLine ){
				if( WhereCurrentWord( nLinePos, nNextWordFrom, &nNextWordFrom2, &nNextWordTo2 , NULL, NULL ) ){
					if( nPatternLen == nNextWordTo2 - nNextWordFrom2 ){
						const char* pData = pDocLine->m_cLine.GetStringPtr();	// 2002/2/10 aroka CMemory変更
						/* 1==大文字小文字の区別 */
						if( (!sSearchOption.bLoHiCase && 0 == my_memicmp( &(pData[nNextWordFrom2]) , pszPattern, nPatternLen ) ) ||
							(sSearchOption.bLoHiCase && 0 == memcmp( &(pData[nNextWordFrom2]) , pszPattern, nPatternLen ) )
						){
							*pnLineNum = nLinePos;	// マッチ行
							*pnIdxFrom = nNextWordFrom2;	// マッチ位置from
							*pnIdxTo = *pnIdxFrom + nPatternLen;	// マッチ位置to
							nRetVal = 1;
							goto end_of_func;
						}
					}
					/* 現在位置の左右の単語の先頭位置を調べる */
					if( PrevOrNextWord( nLinePos, nNextWordFrom, &nNextWordFrom, FALSE, FALSE ) ){
						continue;
					}
				}
				/* 次の行を見に行く */
				nLinePos++;
				pDocLine = pDocLine->m_pNext;
				nNextWordFrom = 0;
			}
		}

		nRetVal = 0;
		goto end_of_func;
	}
	//普通の検索 (正規表現でも単語単位でもない)
	else{
		// 後方検索
		if( eDirection == SEARCH_BACKWARD ){
			nLinePos = nLineNum;
			nHitTo = nIdx;

			nIdxPos = 0;
			pDocLine = GetLine( nLinePos );
			while( NULL != pDocLine ){
				pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );
				nHitPos = -1;
				while( 1 ){
					nHitPosOld = nHitPos;
					nIdxPosOld = nIdxPos;
					pszRes = SearchString(
						(const unsigned char *)pLine,
						nLineLen,
						nIdxPos,
						(const unsigned char *)pszPattern,
						nPatternLen,
						pnKey_CharCharsArr,
						sSearchOption.bLoHiCase
					);
					if( NULL != pszRes ){
						nHitPos = pszRes - pLine;
						nIdxPos = nHitPos + nPatternLen;	// マッチ文字列長進めるように変更 2005.10.28 Karoto
						if( nHitPos >= nHitTo ){
							if( -1 != nHitPosOld ){
								*pnLineNum = nLinePos;							// マッチ行
								*pnIdxFrom = nHitPosOld;						// マッチ位置from
 								*pnIdxTo = nIdxPosOld;							// マッチ位置to/
								nRetVal = 1;
								goto end_of_func;
							}else{
								break;
							}
						}
					}else{
						if( -1 != nHitPosOld ){
							*pnLineNum = nLinePos;							// マッチ行
							*pnIdxFrom = nHitPosOld;						// マッチ位置from
							*pnIdxTo = nIdxPosOld;							// マッチ位置to
							nRetVal = 1;
							goto end_of_func;
						}else{
							break;
						}
					}
				}
				nLinePos--;
				pDocLine = pDocLine->m_pPrev;
				nIdxPos = 0;
				if( NULL != pDocLine ){
					nHitTo = pDocLine->m_cLine.GetStringLength();
				}
			}
			nRetVal = 0;
			goto end_of_func;
		}
		// 前方検索
		else{
			nIdxPos = nIdx;
			nLinePos = nLineNum;
			pDocLine = GetLine( nLinePos );
			while( NULL != pDocLine ){
				pLine = pDocLine->m_cLine.GetStringPtr( &nLineLen );
				pszRes = SearchString(
					(const unsigned char *)pLine,
					nLineLen,
					nIdxPos,
					(const unsigned char *)pszPattern,
					nPatternLen,
					pnKey_CharCharsArr,
					sSearchOption.bLoHiCase
				);
				if( NULL != pszRes ){
					*pnLineNum = nLinePos;							// マッチ行
					*pnIdxFrom = pszRes - pLine;					// マッチ位置from
					*pnIdxTo = *pnIdxFrom + nPatternLen;	// マッチ位置to
					nRetVal = 1;
					goto end_of_func;
				}
				++nLinePos;
				pDocLine = pDocLine->m_pNext;
				nIdxPos = 0;
			}
			nRetVal = 0;
			goto end_of_func;
		}
	}
end_of_func:;
	if( NULL != pnKey_CharCharsArr ){
		delete [] pnKey_CharCharsArr;
		pnKey_CharCharsArr = NULL;
	}

	return nRetVal;
}

/* 検索条件の情報(キー文字列の全角か半角かの配列)作成 */
void CDocLineMgr::CreateCharCharsArr(
	const unsigned char*	pszPattern,
	int						nSrcLen,
	int**					ppnCharCharsArr
)
{
	int		i;
	int*	pnCharCharsArr;
	pnCharCharsArr = new int[nSrcLen];
	for( i = 0; i < nSrcLen; /*i++*/ ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		pnCharCharsArr[i] = CMemory::GetSizeOfChar( (const char *)pszPattern, nSrcLen, i );
		if( 0 == pnCharCharsArr[i] ){
			pnCharCharsArr[i] = 1;
		}
		if( 2 == pnCharCharsArr[i] ){
			pnCharCharsArr[i + 1] = pnCharCharsArr[i];
		}
		i+= pnCharCharsArr[i];
	}
	*ppnCharCharsArr = pnCharCharsArr;
	return;
}


/* 文字列検索 */
char* CDocLineMgr::SearchString(
		const unsigned char*	pLine,
		int						nDesLen,
		int						nIdxPos,
		const unsigned char*	pszPattern,
		int						nSrcLen,
		int*					pnCharCharsArr,
//		int*					pnCharUsedArr,
		int						bLoHiCase
)
{
	if( nDesLen < nSrcLen ){
		return NULL;
	}
	if( 0 >= nSrcLen || 0 >= nDesLen){
		return NULL;
	}

	int	nPos;
	int	i;
	int	j;
	int	nWork;
	int	nCharChars;
	int	nCharChars1;
	int	nCharChars2;
	int	nCompareTo;

	/* 線形探索 */
	nCompareTo = nDesLen - nSrcLen;	//	Mar. 4, 2001 genta
	for( nPos = nIdxPos; nPos <= nCompareTo; /*nPos++*/ ){
		// 2005-09-02 D.S.Koba GetSizeOfChar
		nCharChars = CMemory::GetSizeOfChar( (const char *)pLine, nDesLen, nPos );
		nCharChars1 = nCharChars;
		for( i = 0; i < nSrcLen; /*i++*/ ){
			if( NULL != pnCharCharsArr ){
				nCharChars2 = pnCharCharsArr[i];
			}else{
				// 2005-09-02 D.S.Koba GetSizeOfChar
				nCharChars2 = CMemory::GetSizeOfChar( (const char *)pszPattern, nSrcLen, i );
			}
			if( nCharChars1 != nCharChars2 ){
				break;
			}
			if( 0 == nCharChars1 ){
				nWork =1;
			}else{
				nWork = nCharChars1;
			}
			//	From Here Mar. 4, 2001 genta
			if( !bLoHiCase && nWork == 1 ){	//	英大小文字の同一視
				if( toupper( pLine[nPos + i] ) != toupper( pszPattern[i] ) )
					break;
			}
			else {
				for( j = 0; j < nWork; ++j ){
					if( pLine[nPos + i + j] != pszPattern[i + j] ){
						break;
					}
				}
				if( j < nWork ){
					break;
				}
			}
			//	To Here
			if( 0 == nCharChars2 ){
				++i;
			}else{
				i+= nCharChars2;
			}
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars1 = CMemory::GetSizeOfChar( (const char *)pLine, nDesLen, nPos + i );
		}
		if( i >= nSrcLen ){
			return (char *)&pLine[nPos];
		}
		if( 0 == nCharChars ){
			++nPos;
		}else{
			nPos+= nCharChars;
		}
	}
	return NULL;
}



//! 現在位置の文字の種類を調べる
int	CDocLineMgr::WhatKindOfChar(
	const char*	pData,
	int		pDataLen,
	int		nIdx
)
{
	int		nCharChars;
	WORD	wChar;
	// 2005-09-02 D.S.Koba GetSizeOfChar
	nCharChars = CMemory::GetSizeOfChar( pData, pDataLen, nIdx );
	if( nCharChars == 0 ){
		return CK_NULL;	/* NULL 0x0<=c<=0x0 */
	}
	else if( nCharChars == 1 ){	/* 半角文字 */
		if( pData[nIdx] == CR ){
			return CK_CR;	/* CR = 0x0d */
		}
		if( pData[nIdx] == LF ){
			return CK_LF;	/* LF = 0x0a */
		}
		if( pData[nIdx] == TAB ){
			return CK_TAB;	/* タブ 0x9<=c<=0x9 */
		}
		if( pData[nIdx] == SPACE ){
			return CK_SPACE;	/* 半角のスペース 0x20<=c<=0x20 */
		}
		if( pData[nIdx] >= 0 && __iscsym( pData[nIdx] ) ){	// 2008.10.29 syat 条件追加
			return CK_CSYM;	/* 半角の英字、アンダースコア、数字のいずれか */
		}
		if( pData[nIdx] >= (char)0xa1 && pData[nIdx] <= (char)0xdf ){ // Mar. 30, 2003 genta fd->df
			return CK_KATA;	/* 半角のカタカナ 0xA1<=c<=0xFD */
		}
		return CK_ETC;	/* 半角のその他 */

	}
	else if( nCharChars == 2 ){	/* 全角文字 */
		//<< 2002/03/28 Azumaiya
		// 判定条件部分の比較回数を少なくして最適化。
		wChar =  MAKEWORD(pData[nIdx + 1], pData[nIdx]);
//		MYTRACE( _T("wChar=%0xh\n"), wChar );
		if( wChar == (WORD)0x8140 ){
			return CK_MBC_SPACE;	/* 2バイトのスペース */
		}
		if( wChar == (WORD)0x815B ){
			return CK_MBC_NOVASU;	/* 伸ばす記号 0x815B<=c<=0x815B 'ー' */
		}
//		if( wChar == (WORD)0x8151 ||								/* 0x8151<=c<=0x8151 全角アンダースコア */
//			(wChar >= (WORD)0x824F && wChar <= (WORD)0x8258 ) ||	/* 0x824F<=c<=0x8258 全角数字 */
//			(wChar >= (WORD)0x8260 && wChar <= (WORD)0x8279 ) ||	/* 0x8260<=c<=0x8279 全角英大文字 */
//			(wChar >= (WORD)0x8281 && wChar <= (WORD)0x829a )		/* 0x8281<=c<=0x829a 全角英小文字 */
//		){
		if (
			(WORD)wChar == 0x8151 ||			/* 0x8151<=c<=0x8151 全角アンダースコア */
			(WORD)(wChar - 0x824F) <= 0x09 ||	/* 0x824F<=c<=0x8258 全角数字 */
			(WORD)(wChar - 0x8260) <= 0x19 ||	/* 0x8260<=c<=0x8279 全角英大文字 */
			(WORD)(wChar - 0x8281) <= 0x19 		/* 0x8281<=c<=0x829a 全角英小文字 */
		   ){
			return CK_MBC_CSYM;	/* 2バイトの英字、アンダースコア、数字のいずれか */
		}
		if( (WORD)(wChar - 0x814a) <= 0x01 ){ /* ゛゜ 全角濁点 */
			return CK_MBC_DAKU;	/* 2バイトの濁点 */
		}
		if( (WORD)(wChar - 0x8152) <= 0x01 ){ /* ヽヾ カタカナ踊り字 */
			return CK_MBC_KATA;	/* 2バイトのカタカナ */
		}
		if( (WORD)(wChar - 0x8154) <= 0x01 ){ /* ゝゞ ひらがな踊り字 */
			return CK_MBC_HIRA;	/* 2バイトのひらがな */
		}
		if( (WORD)(wChar - 0x8157) <= 0x03 ){ /* 仝々〆〇 漢字とみなせる字 */
			return CK_MBC_ETC;	/* 2バイトの漢字 */
		}
//		if( wChar >= (WORD)0x8140 && wChar <= (WORD)0x81FD ){
		if( (WORD)(wChar - 0x8140) <= 0xBD ){ /* 0x8140<=c<=0x81FD 2バイトの記号 */
			return CK_MBC_KIGO;	/* 2バイトの記号 */
		}
//		if( wChar >= (WORD)0x829F && wChar <= (WORD)0x82F1 ){
		if( (WORD)(wChar - 0x829F) <= 0x52 ){	/* 0x829F<=c<=0x82F1 2バイトのひらがな */
			return CK_MBC_HIRA;	/* 2バイトのひらがな */
		}
//		if( wChar >= (WORD)0x8340 && wChar <= (WORD)0x8396 ){
		if( (WORD)(wChar - 0x8340) <= 0x56 ){	/* 0x8340<=c<=0x8396 2バイトのカタカナ */
			return CK_MBC_KATA;	/* 2バイトのカタカナ */
		}
//		if( wChar >= (WORD)0x839F && wChar <= (WORD)0x83D6 ){
		if( (WORD)(wChar - 0x839F) <= 0x37 ){	/* 0x839F<=c<=0x83D6 2バイトのギリシャ文字 */
			return CK_MBC_GIRI;	/* 2バイトのギリシャ文字 */
		}
//		if( ( wChar >= (WORD)0x8440 && wChar <= (WORD)0x8460 ) ||	/* 0x8440<=c<=0x8460 全角ロシア文字大文字 */
//			( wChar >= (WORD)0x8470 && wChar <= (WORD)0x8491 ) ){	/* 0x8470<=c<=0x8491 全角ロシア文字小文字 */
		if(
			(WORD)(wChar - 0x8440) <= 0x20 ||	/* 0x8440<=c<=0x8460 全角ロシア文字大文字 */
			(WORD)(wChar - 0x8470) <= 0x21		/* 0x8470<=c<=0x8491 全角ロシア文字小文字 */
		   ){
			return CK_MBC_ROS;	/* 2バイトのロシア文字: */
		}
//		if( wChar >= (WORD)0x849F && wChar <= (WORD)0x879C ){
		if( (WORD)(wChar - 0x849F) <= 0x02FD ){	/* 0x849F<=c<=0x879C 2バイトの特殊記号 */
			return CK_MBC_SKIGO;	/* 2バイトの特殊記号 */
		}
		return CK_MBC_ETC;	/* 2バイトのその他(漢字など) */
		//>> 2002/03/28 Azumaiya
	}
	else{
		return CK_NULL;	/* NULL 0x0<=c<=0x0 */
	}
}



//! 二つの文字を結合したものの種類を調べる
int CDocLineMgr::WhatKindOfTwoChars( int kindPre, int kindCur )
{
	if( kindPre == kindCur )return kindCur;			// 同種ならその種別を返す

	// 全角長音・全角濁点は前後の全角ひらがな・全角カタカナに引きずられる
	if( ( kindPre == CK_MBC_NOVASU || kindPre == CK_MBC_DAKU ) &&
		( kindCur == CK_MBC_KATA   || kindCur == CK_MBC_HIRA ) )return kindCur;
	if( ( kindCur == CK_MBC_NOVASU || kindCur == CK_MBC_DAKU ) &&
		( kindPre == CK_MBC_KATA   || kindPre == CK_MBC_HIRA ) )return kindPre;

	if( kindPre == kindCur )return kindCur;			// 同種ならその種別を返す

	return CK_NULL;									// それ以外なら二つの文字は別種
}


/*!	@brief CDocLineMgrDEBUG用

	@date 2004.03.18 Moca
		m_pDocLineCurrentとm_pCodePrevReferがデータチェーンの
		要素を指しているかの検証機能を追加．

*/
void CDocLineMgr::DUMP()
{
#ifdef _DEBUG
	MYTRACE( _T("------------------------\n") );

	CDocLine* pDocLine;
	CDocLine* pDocLineNext;
	CDocLine* pDocLineEnd = NULL;
	pDocLine = m_pDocLineTop;

	// 正当性を調べる
	bool bIncludeCurrent = false;
	bool bIncludePrevRefer = false;
	int nNum = 0;
	if( m_pDocLineTop->m_pPrev != NULL ){
		MYTRACE( _T("error: m_pDocLineTop->m_pPrev != NULL\n"));
	}
	if( m_pDocLineBot->m_pNext != NULL ){
		MYTRACE( _T("error: m_pDocLineBot->m_pNext != NULL\n") );
	}
	while( NULL != pDocLine ){
		if( m_pDocLineCurrent == pDocLine ){
			bIncludeCurrent = true;
		}
		if( m_pCodePrevRefer == pDocLine ){
			bIncludePrevRefer = true;
		}
		if( NULL != pDocLine->m_pNext ){
			if( pDocLine->m_pNext == pDocLine ){
				MYTRACE( _T("error: pDocLine->m_pPrev Invalid value.\n") );
				break;
			}
			if( pDocLine->m_pNext->m_pPrev != pDocLine ){
				MYTRACE( _T("error: pDocLine->m_pNext->m_pPrev != pDocLine.\n") );
				break;
			}
		}else{
			pDocLineEnd = pDocLine;
		}
		pDocLine = pDocLine->m_pNext;
		nNum++;
	}
	
	if( pDocLineEnd != m_pDocLineBot ){
		MYTRACE( _T("error: pDocLineEnd != m_pDocLineBot") );
	}
	
	if( nNum != m_nLines ){
		MYTRACE( _T("error: nNum(%d) != m_nLines(%d)\n"), nNum, m_nLines );
	}
	if( false == bIncludeCurrent && m_pDocLineCurrent != NULL ){
		MYTRACE( _T("error: m_pDocLineCurrent=%08lxh Invalid value.\n"), m_pDocLineCurrent );
	}
	if( false == bIncludePrevRefer && m_pCodePrevRefer != NULL ){
		MYTRACE( _T("error: m_pCodePrevRefer =%08lxh Invalid value.\n"), m_pCodePrevRefer );
	}

	// DUMP
	MYTRACE( _T("m_nLines=%d\n"), m_nLines );
	MYTRACE( _T("m_pDocLineTop=%08lxh\n"), m_pDocLineTop );
	MYTRACE( _T("m_pDocLineBot=%08lxh\n"), m_pDocLineBot );
	pDocLine = m_pDocLineTop;
	while( NULL != pDocLine ){
		pDocLineNext = pDocLine->m_pNext;
		MYTRACE( _T("\t-------\n") );
		MYTRACE( _T("\tthis=%08lxh\n"), pDocLine );
		MYTRACE( _T("\tpPrev; =%08lxh\n"), pDocLine->m_pPrev );
		MYTRACE( _T("\tpNext; =%08lxh\n"), pDocLine->m_pNext );

		MYTRACE( _T("\tm_enumEOLType =%s\n"), pDocLine->m_cEol.GetName() );
		MYTRACE( _T("\tm_nEOLLen =%d\n"), pDocLine->m_cEol.GetLen() );


//		MYTRACE( _T("\t[%s]\n"), (char*)*(pDocLine->m_pLine) );
		MYTRACE( _T("\tpDocLine->m_cLine.GetStringLength()=[%d]\n"), pDocLine->m_cLine.GetStringLength() );
		MYTRACE( _T("\t[%s]\n"), pDocLine->m_cLine.GetStringPtr() );


		pDocLine = pDocLineNext;
	}
	MYTRACE( _T("------------------------\n") );
#endif // _DEBUG
	return;
}

/* 行変更状態をすべてリセット */
/*
  ・変更フラグCDocLineオブジェクト作成時にはTRUEである
  ・変更回数はCDocLineオブジェクト作成時には1である

  ファイルを読み込んだときは変更フラグを FALSEにする
  ファイルを読み込んだときは変更回数を 0にする

  ファイルを上書きした時は変更フラグを FALSEにする
  ファイルを上書きした時は変更回数は変えない

  変更回数はUndoしたときに-1される
  変更回数が0になった場合は変更フラグをFALSEにする
*/
void CDocLineMgr::ResetAllModifyFlag( void )
{
	CDocLine* pDocLine;
	CDocLine* pDocLineNext;
	pDocLine = m_pDocLineTop;
	while( NULL != pDocLine ){
		pDocLineNext = pDocLine->m_pNext;
		pDocLine->SetModifyFlg(false);		/* 変更フラグ */
		pDocLine = pDocLineNext;
	}
}


/* 全行データを返す
	改行コードは、CFLF統一される。
	@retval 全行データ。freeで開放しなければならない。
	@note   Debug版のテストにのみ使用している。
*/
char* CDocLineMgr::GetAllData( int*	pnDataLen )
{
	int			nDataLen;
	char*		pLine;
	int			nLineLen;
	CDocLine* 	pDocLine;

	pDocLine = m_pDocLineTop;
	nDataLen = 0;
	while( NULL != pDocLine ){
		//	Oct. 7, 2002 YAZAKI
		nDataLen += pDocLine->GetLengthWithoutEOL() + 2;	//	\r\nを追加して返すため+2する。
		pDocLine = pDocLine->m_pNext;
	}

	char*	pData;
	pData = (char*)malloc( nDataLen + 1 );
	if( NULL == pData ){
		TopErrorMessage(
			NULL,
			_T("CDocLineMgr::GetAllData()\nメモリ確保に失敗しました。\n%dバイト"),
			nDataLen + 1
		);
		return NULL;
	}
	pDocLine = m_pDocLineTop;

	nDataLen = 0;
	while( NULL != pDocLine ){
		//	Oct. 7, 2002 YAZAKI
		nLineLen = pDocLine->GetLengthWithoutEOL();
		if( 0 < nLineLen ){
			pLine = pDocLine->m_cLine.GetStringPtr();
			memcpy( &pData[nDataLen], pLine, nLineLen );
			nDataLen += nLineLen;
		}
		pData[nDataLen++] = '\r';
		pData[nDataLen++] = '\n';
		pDocLine = pDocLine->m_pNext;
	}
	pData[nDataLen] = '\0';
	*pnDataLen = nDataLen;
	return (char*)pData;
}


/* 行オブジェクトの削除、リスト変更、行数-- */
void CDocLineMgr::DeleteNode( CDocLine* pCDocLine )
{
	m_nLines--;	/* 全行数 */
	if( 0 == m_nLines ){
		/* データがなくなった */
		Init();
	}else{
		if( NULL == pCDocLine->m_pPrev ){
			m_pDocLineTop = pCDocLine->m_pNext;
		}else{
			pCDocLine->m_pPrev->m_pNext = pCDocLine->m_pNext;
		}
		if( NULL == pCDocLine->m_pNext ){
			m_pDocLineBot = pCDocLine->m_pPrev;
		}else{
			pCDocLine->m_pNext->m_pPrev = pCDocLine->m_pPrev;
		}
		if( m_pCodePrevRefer == pCDocLine ){
			m_pCodePrevRefer = pCDocLine->m_pNext;
		}
	}
	delete pCDocLine;

	return;
}



/* 行オブジェクトの挿入、リスト変更、行数++ */
/* pCDocLinePrevの次にpCDocLineを挿入する */
/* NULL==pCDocLinePrevのときリストの先頭に挿入 */
void CDocLineMgr::InsertNode( CDocLine* pCDocLinePrev, CDocLine* pCDocLine )
{
	pCDocLine->m_pPrev = pCDocLinePrev;
	if( NULL != pCDocLinePrev ){
		pCDocLine->m_pNext = pCDocLinePrev->m_pNext;
		pCDocLinePrev->m_pNext = pCDocLine;
	}else{
		pCDocLine->m_pNext = m_pDocLineTop;
		m_pDocLineTop = pCDocLine;
	}
	if( NULL != pCDocLine->m_pNext ){
		pCDocLine->m_pNext->m_pPrev = pCDocLine;
	}else{
		m_pDocLineBot = pCDocLine;
	}
	m_pDocLineTop = pCDocLine;
	m_nLines++;	/* 全行数 */
	return;

}


/*[EOF]*/
