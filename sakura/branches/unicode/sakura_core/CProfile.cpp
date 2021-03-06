/*!	@file
	@brief INIファイル入出力

	@author D.S.Koba
	@date 2003-10-21 D.S.Koba メンバ関数の名前と引数をそのままにしてメンバ変数，関数の中身を書き直し
	@date 2004-01-10 D.S.Koba 返値をBOOLからboolへ変更。IOProfileDataを型別の関数に分け，引数を減らす
	@date 2006-02-11 D.S.Koba 読み込み/書き出しを引数でなく，メンバで判別
	@date 2006-02-12 D.S.Koba IOProfileDataの中身の読み込みと書き出しを関数に分ける
*/
/*
	Copyright (C) 2003, D.S.Koba
	Copyright (C) 2004, D.S.Koba, MIK, genta
	Copyright (C) 2006, D.S.Koba, ryoji

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/
#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "CProfile.h"
#include "Debug.h"
#include "io/CTextStream.h"
using namespace std;

/*! Profileを初期化
	
	@date 2003-10-21 D.S.Koba STLで書き直す
*/
void CProfile::Init( void )
{
	m_strProfileName = _T("");
	m_ProfileData.clear();
	m_bRead = true;
	return;
}

/*!
	sakura.iniの1行を処理する．

	1行の読み込みが完了するごとに呼ばれる．
	
	@param line [in] 読み込んだ行
*/
void CProfile::ReadOneline(
	const wstring& line
)
{
	//	空行を読み飛ばす
	if( line.empty() )
		return;

	//コメント行を読みとばす
	if( 0 == line.compare( 0, 2, LTEXT("//") ))
		return;

	// セクション取得
	//	Jan. 29, 2004 genta compare使用
	if( line.compare( 0, 1, LTEXT("[") ) == 0 
			&& line.find( LTEXT("=") ) == line.npos
			&& line.find( LTEXT("]") ) == ( line.size() - 1 ) ) {
		Section Buffer;
		Buffer.strSectionName = line.substr( 1, line.size() - 1 - 1 );
		m_ProfileData.push_back( Buffer );
	}
	// エントリ取得
	else if( !m_ProfileData.empty() ) {	//最初のセクション以前の行のエントリは無視
		wstring::size_type idx = line.find( LTEXT("=") );
		if( line.npos != idx ) {
			m_ProfileData.back().mapEntries.insert( PAIR_STR_STR( line.substr(0,idx), line.substr(idx+1) ) );
		}
	}
}

/*! Profileをファイルから読み出す
	
	@param pszProfileName [in] ファイル名

	@retval true  成功
	@retval false 失敗

	@date 2003-10-21 D.S.Koba STLで書き直す
	@date 2003-10-26 D.S.Koba ReadProfile()から分離
	@date 2004-01-29 genta stream使用をやめてCライブラリ使用に．
	@date 2004-01-31 genta 行の解析の方を別関数にしてReadFileをReadProfileに
		
*/
bool CProfile::ReadProfile( const TCHAR* pszProfileName )
{
	m_strProfileName = pszProfileName;

	CTextInputStream in(m_strProfileName.c_str());
	if(!in){
		return false;
	}

	try{
		while( in ){
			//1行読込
			wstring line=in.ReadLineW();

			//解析
			ReadOneline(line);
		}
	}
	catch( ... ){
		return false;
	}

	return true;
}


/*! Profileをファイルへ書き出す
	
	@param pszProfileName [in] ファイル名(NULL=最後に読み書きしたファイル)
	@param pszComment [in] コメント文(NULL=コメント省略)

	@retval true  成功
	@retval false 失敗

	@date 2003-10-21 D.S.Koba STLで書き直す
	@date 2004-01-28 D.S.Koba ファイル書き込み部を分離
*/
bool CProfile::WriteProfile(
	const TCHAR* pszProfileName,
	const WCHAR* pszComment
)
{
	if( pszProfileName!=NULL ) {
		m_strProfileName = pszProfileName;
	}
    
	std::vector< wstring > vecLine;
	if( NULL != pszComment ) {
		vecLine.push_back( LTEXT("//") + wstring( pszComment ) );
		vecLine.push_back( LTEXT("") );
	}
	std::vector< Section >::iterator iter;
	std::vector< Section >::iterator iterEnd = m_ProfileData.end();
	MAP_STR_STR::iterator mapiter;
	MAP_STR_STR::iterator mapiterEnd;
	for( iter = m_ProfileData.begin(); iter != iterEnd; iter++ ) {
		//セクション名を書き込む
		vecLine.push_back( LTEXT("[") + iter->strSectionName + LTEXT("]") );
		mapiterEnd = iter->mapEntries.end();
		for( mapiter = iter->mapEntries.begin(); mapiter != mapiterEnd; mapiter++ ) {
			//エントリを書き込む
			vecLine.push_back( mapiter->first + LTEXT("=") + mapiter->second );
		}
		vecLine.push_back( LTEXT("") );
	}

	return WriteFile( m_strProfileName, vecLine );
}

/*! ファイルへ書き込む
	
	@retval true  成功
	@retval false 失敗

	@date 2004-01-28 D.S.Koba WriteProfile()から分離
	@date 2004-01-29 genta stream使用をやめてCライブラリ使用に．
*/
bool CProfile::WriteFile(
	const tstring&			strFilename,	//!< [in]  ファイル名
	const vector<wstring>&	vecLine			//!< [out] 文字列格納先
)
{
	CTextOutputStream out(strFilename.c_str());
	if(!out){
		return false;
	}

	for(int i=0;i<(int)vecLine.size();i++){
		// 出力
		out.WriteString(vecLine[i].c_str());
		out.WriteString(L"\n");
	}

	out.Close();

	return true;
}



// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                            Imp                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*! エントリ値をProfileから読み込む
	
	@retval true 成功
	@retval false 失敗

	@date 2003-10-22 D.S.Koba 作成
*/
bool CProfile::GetProfileDataImp(
	const wstring&	strSectionName,	//!< [in] セクション名
	const wstring&	strEntryKey,	//!< [in] エントリ名
	wstring&		strEntryValue	//!< [out] エントリ値
)
{
	wstring strWork;
	std::vector< Section >::iterator iter;
	std::vector< Section >::iterator iterEnd = m_ProfileData.end();
	MAP_STR_STR::iterator mapiter;
	for( iter = m_ProfileData.begin(); iter != iterEnd; iter++ ) {
		if( iter->strSectionName == strSectionName ) {
			mapiter = iter->mapEntries.find( strEntryKey );
			if( iter->mapEntries.end() != mapiter ) {
				strEntryValue = mapiter->second;
				return true;
			}
		}
	}
	return false;
}

/*! エントリをProfileへ書き込む
	
	@retval true  成功
	@retval false 失敗(処理を入れていないのでfalseは返らない)

	@date 2003-10-21 D.S.Koba 作成
*/
bool CProfile::SetProfileDataImp(
	const wstring&	strSectionName,	//!< [in] セクション名
	const wstring&	strEntryKey,	//!< [in] エントリ名
	const wstring&	strEntryValue	//!< [in] エントリ値
)
{
	std::vector< Section >::iterator iter;
	std::vector< Section >::iterator iterEnd = m_ProfileData.end();
	MAP_STR_STR::iterator mapiter;
	MAP_STR_STR::iterator mapiterEnd;
	for( iter = m_ProfileData.begin(); iter != iterEnd; iter++ ) {
		if( iter->strSectionName == strSectionName ) {
			//既存のセクションの場合
			mapiter = iter->mapEntries.find( strEntryKey );
			if( iter->mapEntries.end() != mapiter ) {
				//既存のエントリの場合は値を上書き
				mapiter->second = strEntryValue;
				break;
			}
			else {
				//既存のエントリが見つからない場合は追加
				iter->mapEntries.insert( PAIR_STR_STR( strEntryKey, strEntryValue ) );
				break;
			}
		}
	}
	//既存のセクションではない場合，セクション及びエントリを追加
	if( iterEnd == iter ) {
		Section Buffer;
		Buffer.strSectionName = strSectionName;
		Buffer.mapEntries.insert( PAIR_STR_STR( strEntryKey, strEntryValue ) );
		m_ProfileData.push_back( Buffer );
	}
	return true;
}



void CProfile::DUMP( void )
{
#ifdef _DEBUG
	std::vector< Section >::iterator iter;
	std::vector< Section >::iterator iterEnd = m_ProfileData.end();
	//	2006.02.20 ryoji: MAP_STR_STR_ITER削除時の修正漏れによるコンパイルエラー修正
	MAP_STR_STR::iterator mapiter;
	MAP_STR_STR::iterator mapiterEnd;
	MYTRACE_A( "\n\nCProfile::DUMP()======================" );
	for( iter = m_ProfileData.begin(); iter != iterEnd; iter++ ) {
		MYTRACE_A( "\n■strSectionName=%ls", iter->strSectionName.c_str() );
		mapiterEnd = iter->mapEntries.end();
		for( mapiter = iter->mapEntries.begin(); mapiter != mapiterEnd; mapiter++ ) {
			MYTRACE_A( "\"%ls\" = \"%ls\"\n", mapiter->first.c_str(), mapiter->second.c_str() );
		}
	}
	MYTRACE_A( "========================================\n" );
#endif
	return;
}

/*[EOF]*/
