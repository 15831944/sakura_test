/*!	@file
	@brief CEditViewクラスの補完関連コマンド処理系関数群

	@author genta
	@date	2005/01/10 作成
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, jepro
	Copyright (C) 2001, asa-o
	Copyright (C) 2003, Moca
	Copyright (C) 2004, Moca
	Copyright (C) 2005, genta
	Copyright (C) 2006, rastiv
	Copyright (C) 2008, syat, nasukoji
	Copyright (C) 2010, syat

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "StdAfx.h"
#include "CEditView.h"
#include "CEditApp.h"
#include "CEditDoc.h"
#include "Debug.h"
#include "etc_uty.h"
#include "charcode.h"  // 2006.06.28 rastiv
#include "CDocLineMgr.h"	// 2008.10.29 syat
#include "CEditWnd.h"
#include "my_icmp.h"		// 2008.10.29 syat
#include "sakura_rc.h"

/*!
	@brief コマンド受信前補完処理
	
	補完ウィンドウの非表示

	@date 2005.01.10 genta 関数化
*/
void CEditView::PreprocessCommand_hokan( int nCommand )
{
	/* 補完ウィンドウが表示されているとき、特別な場合を除いてウィンドウを非表示にする */
	if( m_bHokan ){
		if( nCommand != F_HOKAN		//	補完開始・終了コマンド
		 && nCommand != F_CHAR		//	文字入力
		 && nCommand != F_IME_CHAR	//	漢字入力
		 ){
			m_pcEditWnd->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
	}
}

/*!
	コマンド実行後補完処理

	@author Moca
	@date 2005.01.10 genta 関数化
*/
void CEditView::PostprocessCommand_hokan(void)
{
	if( m_pShareData->m_Common.m_sHelper.m_bUseHokan && !m_bExecutingKeyMacro ){ /* キーボードマクロの実行中 */
		CMemory	cmemData;

		/* カーソル直前の単語を取得 */
		if( 0 < GetLeftWord( &cmemData, 100 ) ){
			ShowHokanMgr( cmemData, FALSE );
		}else{
			if( m_bHokan ){
				m_pcEditWnd->m_cHokanMgr.Hide();
				m_bHokan = FALSE;
			}
		}
	}
}

/*!	補完ウィンドウを表示する
	ウィンドウを表示した後は、HokanMgrに任せるので、ShowHokanMgrの知るところではない。
	
	@param cmemData [in] 補完する元のテキスト 「Ab」などがくる。
	@param bAutoDecided [in] 候補が1つだったら確定する

	@date 2005.01.10 genta CEditView_Commandから移動
*/
void CEditView::ShowHokanMgr( CMemory& cmemData, BOOL bAutoDecided )
{
	/* 補完対象ワードリストを調べる */
	CMemory		cmemHokanWord;
	int			nKouhoNum;
	POINT		poWin;
	/* 補完ウィンドウの表示位置を算出 */
	poWin.x = m_nViewAlignLeft
			 + (m_ptCaretPos.x - m_nViewLeftCol)
			  * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace );
	poWin.y = m_nViewAlignTop
			 + (m_ptCaretPos.y - m_nViewTopLine)
			  * ( m_pcEditDoc->GetDocumentAttribute().m_nLineSpace + m_nCharHeight );
	::ClientToScreen( m_hWnd, &poWin );
	poWin.x -= (
		cmemData.GetStringLength()
		 * ( m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace )
	);

	/*	補完ウィンドウを表示
		ただし、bAutoDecided == TRUEの場合は、補完候補が1つのときは、ウィンドウを表示しない。
		詳しくは、Search()の説明を参照のこと。
	*/
	CMemory* pcmemHokanWord;
	if ( bAutoDecided ){
		pcmemHokanWord = &cmemHokanWord;
	}
	else {
		pcmemHokanWord = NULL;
	}

	/* 入力補完ウィンドウ作成 */
	// 以前はエディタ起動時に作成していたが必要になってからここで作成するようにした。
	// エディタ起動時だとエディタ可視化の途中になぜか不可視の入力補完ウィンドウが一時的にフォアグラウンドになって、
	// タブバーに新規タブが追加されるときのタブ切替でタイトルバーがちらつく（一瞬非アクティブ表示になるのがはっきり見える）ことがあった。
	// ※ Vista/7 の特定の PC でだけのちらつきか？ 該当 PC 以外の Vista/7 PC でもたまに微妙に表示が乱れた感じになる程度の症状が見られたが、それらが同一原因かどうかは不明。
	if( !m_pcEditWnd->m_cHokanMgr.m_hWnd ){
		m_pcEditWnd->m_cHokanMgr.DoModeless(
			m_pcEditDoc->m_hInstance,
			m_hWnd,
			(LPARAM)this
		);
		::SetFocus( m_hWnd );	//エディタにフォーカスを戻す
	}
	nKouhoNum = m_pcEditWnd->m_cHokanMgr.Search(
		&poWin,
		m_nCharHeight,
		m_nCharWidth + m_pcEditDoc->GetDocumentAttribute().m_nColumnSpace,
		cmemData.GetStringPtr(),
		m_pcEditDoc->GetDocumentAttribute().m_szHokanFile,
		m_pcEditDoc->GetDocumentAttribute().m_bHokanLoHiCase,
		m_pcEditDoc->GetDocumentAttribute().m_bUseHokanByFile, // 2003.06.22 Moca
		pcmemHokanWord
	);
	/* 補完候補の数によって動作を変える */
	if (nKouhoNum <= 0) {				//	候補無し
		if( m_bHokan ){
			m_pcEditWnd->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
			// 2003.06.25 Moca 失敗してたら、ビープ音を出して補完終了。
			ErrorBeep();
		}
	}
	else if( bAutoDecided && nKouhoNum == 1){ //	候補1つのみ→確定。
		if( m_bHokan ){
			m_pcEditWnd->m_cHokanMgr.Hide();
			m_bHokan = FALSE;
		}
		// 2004.05.14 Moca CHokanMgr::Search側で改行を削除するようにし、直接書き換えるのをやめた

		Command_WordDeleteToStart();
		Command_INSTEXT( true, cmemHokanWord.GetStringPtr(), cmemHokanWord.GetStringLength(), TRUE );
	}
	else {
		m_bHokan = TRUE;
	}
	
	//	補完終了。
	if ( !m_bHokan ){
		m_pShareData->m_Common.m_sHelper.m_bUseHokan = FALSE;	//	入力補完終了の知らせ
	}
}


/*!	入力補完
	Ctrl+Spaceでここに到着。
	CEditView::m_bHokan： 現在補完ウィンドウが表示されているかを表すフラグ。
	m_Common.m_sHelper.m_bUseHokan：現在補完ウィンドウが表示されているべきか否かをあらわすフラグ。

    @date 2001/06/19 asa-o 英大文字小文字を同一視する
                     候補が1つのときはそれに確定する
	@date 2001/06/14 asa-o 参照データ変更
	                 開くプロパティシートをタイプ別に変更
	@date 2000/09/15 JEPRO [Esc]キーと[x]ボタンでも中止できるように変更
	@date 2005/01/10 genta CEditView_Commandから移動
*/
void CEditView::Command_HOKAN( void )
{
	if(!m_pShareData->m_Common.m_sHelper.m_bUseHokan){
		m_pShareData->m_Common.m_sHelper.m_bUseHokan = TRUE;
	}
retry:;
	/* 補完候補一覧ファイルが設定されていないときは、設定するように促す。 */
	// 2003.06.22 Moca ファイル内から検索する場合には補完ファイルの設定は必須ではない
	if( m_pcEditDoc->GetDocumentAttribute().m_bUseHokanByFile == false &&
		_T('\0') == m_pcEditDoc->GetDocumentAttribute().m_szHokanFile[0]
	){
		ConfirmBeep();
		if( IDYES == ::MYMESSAGEBOX( NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_TOPMOST, GSTR_APPNAME,
			_T("補完候補一覧ファイルが設定されていません。\n今すぐ設定しますか?")
		) ){
			/* タイプ別設定 プロパティシート */
			if( !CEditApp::getInstance()->OpenPropertySheetTypes( 2, m_pcEditDoc->GetDocumentType() ) ){
				return;
			}
			goto retry;
		}
	}

	CMemory		cmemData;
	/* カーソル直前の単語を取得 */
	if( 0 < GetLeftWord( &cmemData, 100 ) ){
		ShowHokanMgr( cmemData, TRUE );
	}else{
		InfoBeep(); //2010.04.03 Error→Info
		SendStatusMessage(_T("補完対象がありません")); // 2010.05.29 ステータスで表示
		m_pShareData->m_Common.m_sHelper.m_bUseHokan = FALSE;	//	入力補完終了のお知らせ
	}
	return;
}


/*!
	編集中データから入力補完キーワードの検索
	CHokanMgrから呼ばれる

	@return 候補数

	@author Moca
	@date 2003.06.25

	@date 2005/01/10 genta  CEditView_Commandから移動
	@date 2007/10/17 kobake 読みやすいようにネストを浅くしました。
*/
int CEditView::HokanSearchByFile(
	const char* 	pszKey,			//!< [in]
	bool			bHokanLoHiCase,	//!< [in] 英大文字小文字を同一視する
	CMemory**		ppcmemKouho,	//!< [in,out] 候補
	int				nKouhoNum,		//!< [in] ppcmemKouhoのすでに入っている数
	int				nMaxKouho		//!< [in] Max候補数(0==無制限)
){
	const int nKeyLen = lstrlen( pszKey );
	int nLines = m_pcEditDoc->m_cDocLineMgr.GetLineCount();
	int i, j, nWordLen, nLineLen, nRet, nCharSize, nWordEnd;

	const char* pszLine;
	const char* word;

	int nCurX = m_ptCaretPos_PHY.x;	//物理カーソル位置
	int nCurY = m_ptCaretPos_PHY.y;	//物理カーソル位置
	bool bKeyStartWithMark;			//キーが記号で始まるか
	bool bWordStartWithMark;		//候補が記号で始まるか

	// キーの先頭が記号(#$@\)かどうか判定
	bKeyStartWithMark = ( IS_KEYWORD_CHAR( pszKey[0] ) == 2 ? true : false );

	for( i = 0; i < nLines; i++ ){
		pszLine = m_pcEditDoc->m_cDocLineMgr.GetLineStrWithoutEOL( i, &nLineLen );

		for( j = 0; j < nLineLen; j += nCharSize ){
			nCharSize = CMemory::GetSizeOfChar( pszLine, nLineLen, j );

			// 半角記号は候補に含めない
			if ( (unsigned char)pszLine[j] < 0x80 && !IS_KEYWORD_CHAR( pszLine[j] ) )continue;

			// キーの先頭が記号以外の場合、記号で始まる単語は候補からはずす
			if( !bKeyStartWithMark && IS_KEYWORD_CHAR( pszLine[j] ) == 2 )continue;

			// 候補単語の開始位置を求める
			word = pszLine + j;
			bWordStartWithMark = ( IS_KEYWORD_CHAR( pszLine[j] ) == 2 ? true : false );

			// 文字種類取得
			int kindPre = CDocLineMgr::WhatKindOfChar( pszLine, nLineLen, j );	// 文字種類取得

			// 全角記号は候補に含めない
			if ( kindPre == CK_MBC_SPACE || kindPre == CK_MBC_NOVASU || kindPre == CK_MBC_DAKU ||
				 kindPre == CK_MBC_KIGO  || kindPre == CK_MBC_SKIGO )continue;

			// 候補単語の終了位置を求める
			nWordLen = nCharSize;
			nWordEnd = 0;
			for( j += nCharSize; j < nLineLen; j += nCharSize ){
				nWordEnd = j;			// ループを抜けた時点で単語の終わりを指す			
				nCharSize = CMemory::GetSizeOfChar( pszLine, nLineLen, j );

				// 半角記号は含めない
				if ( (unsigned char)pszLine[j] < 0x80 && !IS_KEYWORD_CHAR( pszLine[j] ) )break;

				// 文字種類取得
				int kindCur = CDocLineMgr::WhatKindOfChar( pszLine, nLineLen, j );
				// 全角記号は候補に含めない（ただしヽヾゝゞ仝々〆〇ー濁点は許可）
				if ( kindCur == CK_MBC_SPACE || kindCur == CK_MBC_KIGO || kindCur == CK_MBC_SKIGO ){
					break;
				}

				// 文字種類が変わったら単語の切れ目とする
				int kindMerge = CDocLineMgr::WhatKindOfTwoChars( kindPre, kindCur );
				if ( kindMerge == CK_NULL ) {	// kindPreとkindCurが別種
					if( kindCur == CK_MBC_HIRA ) {
						kindMerge = kindCur;		// ひらがななら続行
					}else if( bKeyStartWithMark && bWordStartWithMark && kindPre == CK_ETC ){
						kindMerge = kindCur;		// 記号で始まる単語は制限を緩める
					}else{
						j -= nCharSize;
						break;						// それ以外は単語の切れ目
					}
				}

				kindPre = kindMerge;
				nWordLen += nCharSize;				// 次の文字へ
			}
			if( j >= nLineLen ) nWordEnd = nLineLen;

			// CDicMgr等の制限により長すぎる単語は無視する
			if( nWordLen > 1020 ){
				continue;
			}
			if( nKeyLen > nWordLen ) continue;

			// キーと比較する
			if( bHokanLoHiCase ){
				nRet = my_memicmp( pszKey, word, nKeyLen );		// 2008.10.29 syat memicmpがマルチバイト文字に対し不正な結果をかえすため修正
			}else{
				nRet = memcmp( pszKey, word, nKeyLen );
			}
			if( 0 != nRet ) continue;

			// カーソル位置の単語は候補からはずす
			if( nCurY == i && nCurX <= nWordEnd && nWordEnd - nWordLen <= nCurX ){	// 2010.02.20 syat 修正// 2008.11.09 syat 修正
				continue;
			}

			if( NULL == *ppcmemKouho ){
				*ppcmemKouho = new CMemory;
				(*ppcmemKouho)->SetString( word, nWordLen );
				(*ppcmemKouho)->AppendString( "\n" );
				++nKouhoNum;
			}
			else{
				// 重複していたら追加しない
				int nLen;
				const char* ptr = (*ppcmemKouho)->GetStringPtr( &nLen );
				int nPosKouho;
				nRet = 1;
				// 2008.07.23 nasukoji	大文字小文字を同一視の場合でも候補の振るい落としは完全一致で見る
				if( nWordLen < nLen ){
					if( '\n' == ptr[nWordLen] && 0 == memcmp( ptr, word, nWordLen )  ){
						nRet = 0;
					}else{
						const int nPosKouhoMax = nLen - nWordLen - 1;
						for( nPosKouho = 1; nPosKouho < nPosKouhoMax; nPosKouho++ ){
							if( ptr[nPosKouho] == '\n' ){
								if( ptr[nPosKouho + nWordLen + 1] == '\n' ){
									if( 0 == memcmp( &ptr[nPosKouho + 1], word, nWordLen) ){
										nRet = 0;
										break;
									}else{
										nPosKouho += nWordLen;
									}
								}
							}
						}
					}
				}
				if( 0 == nRet ){
					continue;
				}
				(*ppcmemKouho)->AppendString( word, nWordLen );
				(*ppcmemKouho)->AppendString( "\n", 1 );
				++nKouhoNum;
			}
			if( 0 != nMaxKouho && nMaxKouho <= nKouhoNum ){
				return nKouhoNum;
			}
		}
	}
	return nKouhoNum;
}
/*[EOF]*/
