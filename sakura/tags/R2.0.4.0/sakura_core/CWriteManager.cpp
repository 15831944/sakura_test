#include "stdafx.h"
#include "CWriteManager.h"
#include <list>
#include "doc/CDocLineMgr.h"
#include "doc/CDocLine.h"
#include "CEditApp.h" // CAppExitException
#include "window/CEditWnd.h"
#include "charset/CCodeFactory.h"
#include "charset/CCodeBase.h"
#include "io/CIoBridge.h"
#include "io/CBinaryStream.h"
#include "util/window.h"


/*! バッファ内容をファイルに書き出す (テスト用)

	@note Windows用にコーディングしてある
	@date 2003.07.26 ryoji BOM引数追加
*/
EConvertResult CWriteManager::WriteFile_From_CDocLineMgr(
	const CDocLineMgr&	pcDocLineMgr,	//!< [in]
	const SSaveInfo&	sSaveInfo		//!< [in]
)
{
	EConvertResult		nRetVal = RESULT_COMPLETE;
	std::auto_ptr<CCodeBase> pcCodeBase( CCodeFactory::CreateCodeBase(sSaveInfo.eCharCode,0) );

	try
	{
		//ファイルオープン
		CBinaryOutputStream out(sSaveInfo.cFilePath,true);

		//BOM出力
		if(sSaveInfo.bBomExist){
			CMemory cBom;
			pcCodeBase->GetBom(&cBom);
			out.Write(cBom.GetRawPtr(),cBom.GetRawLength());
		}

		//各行出力
		int			nLineNumber = 0;
		CDocLine*	pcDocLine = pcDocLineMgr.GetDocLineTop();
		while( pcDocLine ){
			++nLineNumber;

			//経過通知
			if(pcDocLineMgr.GetLineCount()>0 && nLineNumber%1024==0){
				NotifyProgress(nLineNumber * 100 / pcDocLineMgr.GetLineCount());
				// 処理中のユーザー操作を可能にする
				if( !::BlockingHook( NULL ) ){
					throw CAppExitException(); //中断検出
				}
			}

			//1行出力 -> cmemOutputBuffer
			CMemory cmemOutputBuffer;
			if( 0 < pcDocLine->GetLengthWithoutEOL() ){
				CNativeW cstrSrc( pcDocLine->GetPtr(), pcDocLine->GetLengthWithoutEOL() );

				// 書き込み時のコード変換 cstrSrc -> cmemOutputBuffer
				EConvertResult e = CIoBridge::ImplToFile(
					cstrSrc,
					&cmemOutputBuffer,
					sSaveInfo.eCharCode
				);
				if(e==RESULT_LOSESOME){
					if(nRetVal==RESULT_COMPLETE)nRetVal=RESULT_LOSESOME;
				}
			}

			//改行出力 -> cmemOutputBuffer
			if( pcDocLine->GetEol() != EOL_NONE ){
				CMemory cEolMem;
				pcCodeBase->GetEol(&cEolMem,sSaveInfo.cEol!=EOL_NONE?sSaveInfo.cEol:pcDocLine->GetEol());
				cmemOutputBuffer.AppendRawData(cEolMem.GetRawPtr(),cEolMem.GetRawLength());
			}

			//ファイルに出力 cmemOutputBuffer -> fp
			out.Write(cmemOutputBuffer.GetRawPtr(), cmemOutputBuffer.GetRawLength());

			//次の行へ
			pcDocLine = pcDocLine->GetNextLine();
		}

		//ファイルクローズ
		out.Close();
	}
	catch(CError_FileOpen){ //########### 現時点では、この例外が発生した場合は正常に動作できない
		ErrorMessage(
			CEditWnd::Instance()->GetHwnd(),
			_T("\'%ts\'\n")
			_T("ファイルを保存できません。\n")
			_T("パスが存在しないか、他のアプリケーションで使用されている可能性があります。"),
			sSaveInfo.cFilePath.c_str()
		);
		nRetVal = RESULT_FAILURE;
	}
	catch(CError_FileWrite){
		nRetVal = RESULT_FAILURE;
	}
	catch(CAppExitException){
		//中断検出
		return RESULT_FAILURE;
	}

	return nRetVal;
}
