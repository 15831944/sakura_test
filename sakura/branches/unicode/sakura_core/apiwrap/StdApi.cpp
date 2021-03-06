#include "stdafx.h"
#include "StdApi.h"
#include "charcode.h"
#include <vector>
using namespace std;

//デバッグ用。
//VistaだとExtTextOutの結果が即反映されない。この関数を用いると即反映されるので、
//デバッグ時ステップ実行する際に便利になる。ただし、当然重くなる。
#ifdef _DEBUG
#define DEBUG_SETPIXEL(hdc) SetPixel(hdc,-1,-1,0); //SetPixelをすると、結果が即反映される。
#else
#define DEBUG_SETPIXEL(hdc)
#endif

/*!
	ワイド文字列からマルチバイト文字列を生成する。
	マルチバイト文字列のために新しいメモリ領域が確保されるので、
	使い終わったらDestroyMbStringを呼ぶこと！

	@retval 変換されたACHAR文字列
*/
static ACHAR* CreateMbString(
	const WCHAR*	pWideString,	//!< [in]  元のWCHAR文字列
	int				nWideLength,	//!< [in]  元のWCHAR文字列の長さ。文字単位。
	int*			pnMbLength		//!< [out] 変換されたACHAR文字列の長さの受け取り先。文字単位。
)
{
	//必要な領域サイズを取得
	int nNewLen=WideCharToMultiByte(
		CP_ACP,
		0,
		pWideString,
		nWideLength,
		NULL,
		0,
		NULL,
		NULL
	);

	//領域を確保
	ACHAR* buf=new ACHAR[nNewLen+1];

	//変換
	nNewLen = WideCharToMultiByte(
		CP_ACP,
		0,
		pWideString,
		nWideLength,
		buf,
		nNewLen,
		NULL,
		NULL
	);
	buf[nNewLen]='\0';

	//結果
	if(pnMbLength)*pnMbLength=nNewLen;
	return buf;
}

/*!
	CreateMbString で確保したマルチバイト文字列を解放する
*/
static void DestroyMbString(ACHAR* pMbString)
{
	delete[] pMbString;
}



namespace ApiWrap{



	/*!
		MakeSureDirectoryPathExists の UNICODE 版。
		szDirPath で指定されたすべてのディレクトリを作成します。
		ディレクトリの記述は、ルートから開始します。

		@param DirPath
			有効なパス名を指定する、null で終わる文字列へのポインタを指定します。
			パスの最後のコンポーネントがファイル名ではなくディレクトリである場合、
			文字列の最後に円記号（\）を記述しなければなりません。 

		@returns
			関数が成功すると、TRUE が返ります。
			関数が失敗すると、FALSE が返ります。

		@note
			指定された各ディレクトリがまだ存在しない場合、それらのディレクトリを順に作成します。
			一部のディレクトリのみを作成した場合、この関数は FALSE を返します。

		@author
			kobake

		@date
			2007.10.15
	*/
	BOOL MakeSureDirectoryPathExistsW(LPCWSTR szDirPath)
	{
		const wchar_t* p=szDirPath-1;
		while(1){
			p=wcschr(p+1,L'\\');
			if(!p)break; //'\\'を走査し終わったので終了

			//先頭からpまでの部分文字列 -> szBuf
			wchar_t szBuf[_MAX_PATH];
			wcsncpy_s(szBuf,_countof(szBuf),szDirPath,p-szDirPath);
			szBuf[p-szDirPath]=L'\0';

			//存在するか
			int nAcc = _waccess(szBuf,0);
			if(nAcc==0)continue; //存在するなら、次へ

			//ディレクトリ作成
			int nDir = _wmkdir(szBuf);
			if(nDir==-1)return FALSE; //エラーが発生したので、FALSEを返す
		}
		return TRUE;
	}




	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//              W系描画API (ANSI版でも利用可能)                //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

	/*!
		ANSI版でも使えるExtTextOutW_AnyBuild。
		文字数制限1024半角文字。(文字間隔配列を1024半角文字分しか用意していないため)
	*/
	BOOL ExtTextOutW_AnyBuild(
		HDC				hdc,
		int				x,
		int				y,
		UINT			fuOptions,
		const RECT*		lprc,
		LPCWSTR			lpwString,
		UINT			cbCount,
		const int*		lpDx
	)
	{
		BOOL ret;
#ifdef _UNICODE
		ret=::ExtTextOut(hdc,x,y,fuOptions,lprc,lpwString,cbCount,lpDx);
#else
		if(lpwString==NULL || *lpwString==L'\0')return FALSE;
		if(cbCount>1024)return FALSE;

		int nNewLength=0;
		//ANSI文字列を生成
		ACHAR* pNewString = CreateMbString(
			lpwString,
			cbCount==-1?wcslen(lpwString):cbCount,
			&nNewLength
		);

		//文字間隔配列を生成
		int nHankakuDx;
		const int* lpDxNew=NULL;
		if(lpDx){
			if(WCODE::isHankaku(lpwString[0]))nHankakuDx=lpDx[0];
			else nHankakuDx=lpDx[0]/2;
			static int aDx[1024]={0}; //1024半角文字まで
			if(aDx[0]!=nHankakuDx){
				for(int i=0;i<_countof(aDx);i++){
					aDx[i]=nHankakuDx;
				}
			}
			lpDxNew=aDx;
		}

		//APIコール
		ret=::ExtTextOut(hdc,x,y,fuOptions,lprc,pNewString,nNewLength,lpDxNew);

		//後始末
		DestroyMbString(pNewString);
#endif
		DEBUG_SETPIXEL(hdc);
		return ret;
	}

	BOOL TextOutW_AnyBuild(
		HDC		hdc,
		int		nXStart,
		int		nYStart,
		LPCWSTR	lpwString,
		int		cbString
	)
	{
		BOOL ret;
#ifdef _UNICODE
		ret=::TextOut(hdc,nXStart,nYStart,lpwString,cbString);
#else
		int nNewLength=0;
		ACHAR* pNewString = CreateMbString(
			lpwString,
			cbString==-1?wcslen(lpwString):cbString,
			&nNewLength
		);
		ret=::TextOut(hdc,nXStart,nYStart,pNewString,nNewLength);
		DestroyMbString(pNewString);
#endif
		DEBUG_SETPIXEL(hdc);
		return ret;
	}

	LPWSTR CharNextW_AnyBuild(
		LPCWSTR lpsz
	)
	{
		//$$ サロゲートペア無視
		if(*lpsz)return const_cast<LPWSTR>(lpsz+1);
		else return const_cast<LPWSTR>(lpsz);
	}

	LPWSTR CharPrevW_AnyBuild(
		LPCWSTR lpszStart,
		LPCWSTR lpszCurrent
	)
	{
		//$$ サロゲートペア無視
		if(lpszCurrent>lpszStart)return const_cast<LPWSTR>(lpszCurrent-1);
		else return const_cast<LPWSTR>(lpszStart);
	}

#if 1
	BOOL GetTextExtentPoint32W_AnyBuild(
		HDC		hdc, 
		LPCWSTR	lpString, 
		int		cbString, 
		LPSIZE	lpSize
	)
	{
		vector<char> buf;
		wcstombs_vector(lpString,cbString,&buf);
		return GetTextExtentPoint32A(
			hdc,
			&buf[0],
			buf.size()-1,
			lpSize
		);
	}
#endif

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
	//             その他W系API (ANSI版でも利用可能)               //
	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

	int LoadStringW_AnyBuild(
		HINSTANCE	hInstance,
		UINT		uID,
		LPWSTR		lpBuffer,
		int			nBufferCount	//!< バッファのサイズ。文字単位。
	)
	{
#ifdef _UNICODE
		return ::LoadStringW(hInstance, uID, lpBuffer, nBufferCount);
#else
		//まずはACHARでロード
		int nTmpCnt = nBufferCount*2+2;
		ACHAR* pTmp = new ACHAR[nTmpCnt];
		int ret=LoadStringA(hInstance, uID, pTmp, nTmpCnt);

		//WCHARに変換
		mbstowcs2(lpBuffer, pTmp, nBufferCount);
		int ret2=wcslen(lpBuffer);

		//後始末
		delete[] pTmp;

		//結果
		return ret2;
#endif
	}

}
