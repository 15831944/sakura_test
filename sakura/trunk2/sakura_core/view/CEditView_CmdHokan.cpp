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

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "stdafx.h"
#include "sakura_rc.h"
#include "view/CEditView.h"
#include "doc/CEditDoc.h"
#include "debug/Debug.h"
#include "charset/charcode.h"  // 2006.06.28 rastiv
#include "window/CEditWnd.h"
#include "parse/CWordParse.h"

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
		 && nCommand != F_WCHAR		//	文字入力
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
	if( GetDllShareData().m_Common.m_sHelper.m_bUseHokan && !m_bExecutingKeyMacro ){ /* キーボードマクロの実行中 */
		CNativeW	cmemData;

		/* カーソル直前の単語を取得 */
		if( 0 < GetParser().GetLeftWord( &cmemData, 100 ) ){
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
void CEditView::ShowHokanMgr( CNativeW& cmemData, BOOL bAutoDecided )
{
	/* 補完対象ワードリストを調べる */
	CNativeW	cmemHokanWord;
	int			nKouhoNum;
	POINT		poWin;
	/* 補完ウィンドウの表示位置を算出 */
	poWin.x = GetTextArea().GetAreaLeft()
			 + (Int)(GetCaret().GetCaretLayoutPos().GetX2() - GetTextArea().GetViewLeftCol())
			  * GetTextMetrics().GetHankakuDx();
	poWin.y = GetTextArea().GetAreaTop()
			 + (Int)(GetCaret().GetCaretLayoutPos().GetY2() - GetTextArea().GetViewTopLine())
			  * GetTextMetrics().GetHankakuDy();
	this->ClientToScreen( &poWin );
	poWin.x -= (
		cmemData.GetStringLength()
		 * GetTextMetrics().GetHankakuDx()
	);

	/*	補完ウィンドウを表示
		ただし、bAutoDecided == TRUEの場合は、補完候補が1つのときは、ウィンドウを表示しない。
		詳しくは、Search()の説明を参照のこと。
	*/
	CNativeW* pcmemHokanWord;
	if ( bAutoDecided ){
		pcmemHokanWord = &cmemHokanWord;
	}
	else {
		pcmemHokanWord = NULL;
	}
	nKouhoNum = m_pcEditWnd->m_cHokanMgr.CHokanMgr::Search(
		&poWin,
		GetTextMetrics().GetHankakuHeight(),
		GetTextMetrics().GetHankakuDx(),
		cmemData.GetStringPtr(),
		m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_szHokanFile,
		m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_bHokanLoHiCase,
		m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_bUseHokanByFile, // 2003.06.22 Moca
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
//		pszKouhoWord = cmemHokanWord.GetPtr( &nKouhoWordLen );
//		pszKouhoWord[nKouhoWordLen] = L'\0';
		GetCommander().Command_WordDeleteToStart();
		GetCommander().Command_INSTEXT( TRUE, cmemHokanWord.GetStringPtr(), cmemHokanWord.GetStringLength(), TRUE );
	}
	else {
		m_bHokan = TRUE;
	}
	
	//	補完終了。
	if ( !m_bHokan ){
		GetDllShareData().m_Common.m_sHelper.m_bUseHokan = FALSE;	//	入力補完終了の知らせ
	}
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
	const wchar_t*	pszKey,			//!< [in]
	BOOL			bHokanLoHiCase,	//!< [in] 英大文字小文字を同一視する
	CNativeW**		ppcmemKouho,	//!< [in/out] 候補
	int				nKouhoNum,		//!< [in] ppcmemKouhoのすでに入っている数
	int				nMaxKouho		//!< [in] Max候補数(0==無制限)
){
	const int nKeyLen = wcslen( pszKey );
	int nLines = m_pcEditDoc->m_cDocLineMgr.GetLineCount();
	int j, nWordLen, nLineLen, nRet, nCharSize;

	const wchar_t* pszLine;
	const wchar_t* word;

	CLogicPoint ptCur = GetCaret().GetCaretLogicPos(); //物理カーソル位置
	bool bKeyStartWithMark;			//キーが記号で始まるか
	bool bWordStartWithMark;		//候補が記号で始まるか

	// キーの先頭が記号(#$@\)かどうか判定
	bKeyStartWithMark = ( wcschr( L"$@#\\", pszKey[0] ) != NULL ? true : false );

	for( CLogicInt i = CLogicInt(0); i < nLines; i++  ){
		pszLine = CDocReader(m_pcEditDoc->m_cDocLineMgr).GetLineStrWithoutEOL( i, &nLineLen );

		for( j = 0; j < nLineLen; j += nCharSize ){
			nCharSize = CNativeW::GetSizeOfChar( pszLine, nLineLen, j );

			// 半角記号は候補に含めない
			if ( pszLine[j] < 0x00C0 && !IS_KEYWORD_CHAR( pszLine[j] ) )continue;

			// キーの先頭が記号以外の場合、記号で始まる単語は候補からはずす
			if( !bKeyStartWithMark && wcschr( L"$@#\\", pszLine[j] ) != NULL )continue;

			// 候補単語の開始位置を求める
			word = pszLine + j;
			bWordStartWithMark = ( wcschr( L"$@#\\", pszLine[j] ) != NULL ? true : false );

			// 文字種類取得
			ECharKind kindPre = CWordParse::WhatKindOfChar( pszLine, nLineLen, j );	// 文字種類取得

			// 全角記号は候補に含めない
			if ( kindPre == CK_ZEN_SPACE || kindPre == CK_ZEN_NOBASU || kindPre == CK_ZEN_DAKU ||
				 kindPre == CK_ZEN_KIGO  || kindPre == CK_ZEN_SKIGO )continue;

			// 候補単語の終了位置を求める
			nWordLen = nCharSize;
			for( j += nCharSize; j < nLineLen; j += nCharSize ){
				nCharSize = CNativeW::GetSizeOfChar( pszLine, nLineLen, j );

				// 半角記号は含めない
				if ( pszLine[j] < 0x00C0 && !IS_KEYWORD_CHAR( pszLine[j] ) )break;

				// 文字種類取得
				ECharKind kindCur = CWordParse::WhatKindOfChar( pszLine, nLineLen, j );
				// 全角記号は候補に含めない
				if ( kindCur == CK_ZEN_SPACE || kindCur == CK_ZEN_KIGO || kindCur == CK_ZEN_SKIGO ){
					break;
				}

				// 文字種類が変わったら単語の切れ目とする
				ECharKind kindMerge = CWordParse::WhatKindOfTwoChars( kindPre, kindCur );
				if ( kindMerge == CK_NULL ) {	// kindPreとkindCurが別種
					if( kindCur == CK_HIRA ) {
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

			// CDicMgr等の制限により長すぎる単語は無視する
			if( nWordLen > 1020 ){
				continue;
			}
			if( nKeyLen > nWordLen )continue;

			// キーと比較する
			if( bHokanLoHiCase ){
				nRet = auto_memicmp( pszKey, word, nKeyLen );
			}else{
				nRet = auto_memcmp( pszKey, word, nKeyLen );
			}
			if( nRet!=0 )continue;

			// カーソル位置の単語は候補からはずす
			if( ptCur.y == i && ptCur.x <= j && j - nWordLen + nCharSize <= ptCur.x ){	// 2008.11.09 syat 修正
				continue;
			}

			if( NULL == *ppcmemKouho ){
				*ppcmemKouho = new CNativeW;
				(*ppcmemKouho)->SetString( word, nWordLen );
				(*ppcmemKouho)->AppendString( L"\n" );
				++nKouhoNum;
			}
			else{
				// 重複していたら追加しない
				int nLen;
				const wchar_t* ptr = (*ppcmemKouho)->GetStringPtr( &nLen );
				int nPosKouho;
				nRet = 1;
				// 2008.07.25 nasukoji	大文字小文字を同一視の場合でも候補の振るい落としは完全一致で見る
				if( nWordLen < nLen ){
					if( L'\n' == ptr[nWordLen] && 0 == auto_memcmp( ptr, word, nWordLen )  ){
						nRet = 0;
					}else{
						int nPosKouhoMax = nLen - nWordLen - 1;
						for( nPosKouho = 1; nPosKouho < nPosKouhoMax; nPosKouho++ ){
							if( ptr[nPosKouho] == L'\n' ){
								if( ptr[nPosKouho + nWordLen + 1] == L'\n' ){
									if( 0 == auto_memcmp( &ptr[nPosKouho + 1], word, nWordLen) ){
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
				//2007.10.17 kobake メモリリークしてました。修正。
				(*ppcmemKouho)->AppendString( word, nWordLen );
				(*ppcmemKouho)->AppendString( L"\n" );
				++nKouhoNum;
			}
			if( 0 != nMaxKouho && nMaxKouho <= nKouhoNum ){
				return nKouhoNum;
			}
			
		}
	}
	return nKouhoNum;
}

