/*
	2008.05.18 kobake CShareData から分離
*/

#include "stdafx.h"
#include "CFileNameManager.h"
#include "util/module.h"
#include "util/os.h"
#include <shlobj.h> //CSIDL_PROFILE等
#include "util/shell.h"
#include "CCommandLine.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                      ファイル名管理                         //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //


/*!	共有データの設定に従ってパスを縮小表記に変換する
	@param pszSrc   [in]  ファイル名
	@param pszDest  [out] 変換後のファイル名の格納先
	@param nDestLen [in]  終端のNULLを含むpszDestのTCHAR単位の長さ _MAX_PATH まで
	@date 2003.01.27 Moca 新規作成
	@note 連続して呼び出す場合のため、展開済みメタ文字列をキャッシュして高速化している。
*/
LPTSTR CFileNameManager::GetTransformFileNameFast( LPCTSTR pszSrc, LPTSTR pszDest, int nDestLen )
{
	int i;
	TCHAR szBuf[_MAX_PATH + 1];

	if( -1 == m_nTransformFileNameCount ){
		TransformFileName_MakeCache();
	}

	if( 0 < m_nTransformFileNameCount ){
		GetFilePathFormat( pszSrc, pszDest, nDestLen,
			m_szTransformFileNameFromExp[0],
			m_pShareData->m_Common.m_sFileName.m_szTransformFileNameTo[m_nTransformFileNameOrgId[0]]
		);
		for( i = 1; i < m_nTransformFileNameCount; i++ ){
			_tcscpy( szBuf, pszDest );
			GetFilePathFormat( szBuf, pszDest, nDestLen,
				m_szTransformFileNameFromExp[i],
				m_pShareData->m_Common.m_sFileName.m_szTransformFileNameTo[m_nTransformFileNameOrgId[i]] );
		}
	}else{
		// 変換する必要がない コピーだけする
		_tcsncpy( pszDest, pszSrc, nDestLen - 1 );
		pszDest[nDestLen - 1] = '\0';
	}
	return pszDest;
}

/*!	展開済みメタ文字列のキャッシュを作成・更新する
	@retval 有効な展開済み置換前文字列の数
	@date 2003.01.27 Moca 新規作成
	@date 2003.06.23 Moca 関数名変更
*/
int CFileNameManager::TransformFileName_MakeCache( void ){
	int i;
	int nCount = 0;
	for( i = 0; i < m_pShareData->m_Common.m_sFileName.m_nTransformFileNameArrNum; i++ ){
		if( L'\0' != m_pShareData->m_Common.m_sFileName.m_szTransformFileNameFrom[i][0] ){
			if( ExpandMetaToFolder( m_pShareData->m_Common.m_sFileName.m_szTransformFileNameFrom[i],
			 m_szTransformFileNameFromExp[nCount], _MAX_PATH ) ){
				// m_szTransformFileNameToとm_szTransformFileNameFromExpの番号がずれることがあるので記録しておく
				m_nTransformFileNameOrgId[nCount] = i;
				nCount++;
			}
		}
	}
	m_nTransformFileNameCount = nCount;
	return nCount;
}


/*!	ファイル・フォルダ名を置換して、簡易表示名を取得する
	@date 2002.11.27 Moca 新規作成
	@note 大小文字を区別しない。nDestLenに達したときは後ろを切り捨てられる
*/
LPCTSTR CFileNameManager::GetFilePathFormat( LPCTSTR pszSrc, LPTSTR pszDest, int nDestLen, LPCTSTR pszFrom, LPCTSTR pszTo )
{
	int i, j;
	int nSrcLen;
	int nFromLen, nToLen;
	int nCopy;

	nSrcLen  = _tcslen( pszSrc );
	nFromLen = _tcslen( pszFrom );
	nToLen   = _tcslen( pszTo );

	nDestLen--;

	for( i = 0, j = 0; i < nSrcLen && j < nDestLen; i++ ){
#if defined(_MBCS)
		if( 0 == strnicmp( &pszSrc[i], pszFrom, nFromLen ) )
#else
		if( 0 == _tcsncicmp( &pszSrc[i], pszFrom, nFromLen ) )
#endif
		{
			nCopy = __min( nToLen, nDestLen - j );
			memcpy( &pszDest[j], pszTo, nCopy * sizeof( TCHAR ) );
			j += nCopy;
			i += nFromLen - 1;
		}else{
#if defined(_MBCS)
// SJIS 専用処理
			if( _IS_SJIS_1( (unsigned char)pszSrc[i] ) && i + 1 < nSrcLen && _IS_SJIS_2( (unsigned char)pszSrc[i + 1] ) ){
				if( j + 1 < nDestLen ){
					pszDest[j] = pszSrc[i];
					j++;
					i++;
				}else{
					// SJISの先行バイトだけコピーされるのを防ぐ
					break;// goto end_of_func;
				}
			}
#endif
			pszDest[j] = pszSrc[i];
			j++;
		}
	}
// end_of_func:;
	pszDest[j] = '\0';
	return pszDest;
}


/*!	%MYDOC%などのパラメータ指定を実際のパス名に変換する

	@param pszSrc  [in]  変換前文字列
	@param pszDes  [out] 変換後文字列
	@param nDesLen [in]  pszDesのNULLを含むTCHAR単位の長さ
	@retval true  正常に変換できた
	@retval false バッファが足りなかった，またはエラー。pszDesは不定
	@date 2002.11.27 Moca 作成開始
*/
bool CFileNameManager::ExpandMetaToFolder( LPCTSTR pszSrc, LPTSTR pszDes, int nDesLen )
{
	LPCTSTR ps;
	LPTSTR  pd, pd_end;

#define _USE_META_ALIAS
#ifdef _USE_META_ALIAS
	struct MetaAlias{
		LPTSTR szAlias;
		int nLenth;
		LPTSTR szOrig;
	};
	static const MetaAlias AliasList[] = {
		{  _T("COMDESKTOP"), 10, _T("Common Desktop") },
		{  _T("COMMUSIC"), 8, _T("CommonMusic") },
		{  _T("COMVIDEO"), 8, _T("CommonVideo") },
		{  _T("MYMUSIC"),  7, _T("My Music") },
		{  _T("MYVIDEO"),  7, _T("Video") },
		{  _T("COMPICT"),  7, _T("CommonPictures") },
		{  _T("MYPICT"),   6, _T("My Pictures") },
		{  _T("COMDOC"),   6, _T("Common Documents") },
		{  _T("MYDOC"),    5, _T("Personal") },
		{ NULL, 0 , NULL }
	};
#endif

	pd_end = pszDes + ( nDesLen - 1 );
	for( ps = pszSrc, pd = pszDes; _T('\0') != *ps; ps++ ){
		if( pd_end <= pd ){
			if( pd_end == pd ){
				*pd = _T('\0');
			}
			return false;
		}

		if( _T('%') != *ps ){
			*pd = *ps;
			pd++;
			continue;
		}

		// %% は %
		if( _T('%') == ps[1] ){
			*pd = _T('%');
			pd++;
			ps++;
			continue;
		}

		if( _T('\0') != ps[1] ){
			TCHAR szMeta[_MAX_PATH];
			TCHAR szPath[_MAX_PATH + 1];
			int   nMetaLen;
			int   nPathLen;
			bool  bFolderPath;
			LPCTSTR  pStr;
			ps++;
			// %SAKURA%
			if( 0 == auto_strnicmp( _T("SAKURA%"), ps, 7 ) ){
				// exeのあるフォルダ
				GetExedir( szPath );
				nMetaLen = 6;
			}
			// %SAKURADATA%	// 2007.06.06 ryoji
			else if( 0 == auto_strnicmp( _T("SAKURADATA%"), ps, 11 ) ){
				// iniのあるフォルダ
				GetInidir( szPath );
				nMetaLen = 10;
			}
			// メタ文字列っぽい
			else if( NULL != (pStr = _tcschr( ps, _T('%') ) )){
				nMetaLen = pStr - ps;
				if( nMetaLen < _MAX_PATH ){
					auto_memcpy( szMeta, ps, nMetaLen );
					szMeta[nMetaLen] = _T('\0');
				}
				else{
					*pd = _T('\0');
					return false;
				}
#ifdef _USE_META_ALIAS
				// メタ文字列がエイリアス名なら書き換える
				const MetaAlias* pAlias;
				for( pAlias = &AliasList[0]; nMetaLen < pAlias->nLenth; pAlias++ )
					; // 読み飛ばす
				for( ; nMetaLen == pAlias->nLenth; pAlias++ ){
					if( 0 == auto_stricmp( pAlias->szAlias, szMeta ) ){
						_tcscpy( szMeta, pAlias->szOrig );
						break;
					}
				}
#endif
				// 直接レジストリで調べる
				szPath[0] = _T('\0');
				bFolderPath = ReadRegistry( HKEY_CURRENT_USER,
					_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
					szMeta, szPath, _countof( szPath ) );
				if( false == bFolderPath || _T('\0') == szPath[0] ){
					bFolderPath = ReadRegistry( HKEY_LOCAL_MACHINE,
						_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
						szMeta, szPath, _countof( szPath ) );
				}
				if( false == bFolderPath || _T('\0') == szPath[0] ){
					pStr = _tgetenv( szMeta );
					// 環境変数
					if( NULL != pStr ){
						nPathLen = _tcslen( pStr );
						if( nPathLen < _MAX_PATH ){
							_tcscpy( szPath, pStr );
						}else{
							*pd = _T('\0');
							return false;
						}
					}
					// 未定義のメタ文字列は 入力された%...%を，そのまま文字として処理する
					else if(  pd + ( nMetaLen + 2 ) < pd_end ){
						*pd = _T('%');
						auto_memcpy( &pd[1], ps, nMetaLen );
						pd[nMetaLen + 1] = _T('%');
						pd += nMetaLen + 2;
						ps += nMetaLen;
						continue;
					}else{
						*pd = _T('\0');
						return false;
					}
				}
			}else{
				// %...%の終わりの%がない とりあえず，%をコピー
				*pd = _T('%');
				pd++;
				ps--; // 先にps++してしまったので戻す
				continue;
			}

			// ロングファイル名にする
			nPathLen = _tcslen( szPath );
			LPTSTR pStr2 = szPath;
			if( nPathLen < _MAX_PATH && 0 != nPathLen ){
				if( FALSE != GetLongFileName( szPath, szMeta ) ){
					pStr2 = szMeta;
				}
			}

			// 最後のフォルダ区切り記号を削除する
			// [A:\]などのルートであっても削除
			for(nPathLen = 0; pStr2[nPathLen] != _T('\0'); nPathLen++ ){
#ifdef _MBCS
				if( _IS_SJIS_1( (unsigned char)pStr2[nPathLen] ) && _IS_SJIS_2( (unsigned char)pStr2[nPathLen + 1] ) ){
					// SJIS読み飛ばし
					nPathLen++; // 2003/01/17 sui
				}else
#endif
				if( _T('\\') == pStr2[nPathLen] && _T('\0') == pStr2[nPathLen + 1] ){
					pStr2[nPathLen] = _T('\0');
					break;
				}
			}

			if( pd + nPathLen < pd_end && 0 != nPathLen ){
				auto_memcpy( pd, pStr2, nPathLen );
				pd += nPathLen;
				ps += nMetaLen;
			}else{
				*pd = _T('\0');
				return false;
			}
		}else{
			// 最後の文字が%だった
			*pd = *ps;
			pd++;
		}
	}
	*pd = _T('\0');
	return true;
}




/**
	構成設定ファイルからiniファイル名を取得する

	sakura.exe.iniからsakura.iniの格納フォルダを取得し、フルパス名を返す

	@param[out] pszPrivateIniFile マルチユーザ用のiniファイルパス
	@param[out] pszIniFile EXE基準のiniファイルパス

	@author ryoji
	@date 2007.09.04 ryoji 新規作成
	@date 2008.05.05 novice GetModuleHandle(NULL)→NULLに変更
*/
void CFileNameManager::GetIniFileNameDirect( LPTSTR pszPrivateIniFile, LPTSTR pszIniFile )
{
	TCHAR szPath[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFname[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	::GetModuleFileName(
		NULL,
		szPath, _countof(szPath)
	);
	_tsplitpath( szPath, szDrive, szDir, szFname, szExt );

	auto_snprintf( pszIniFile, _MAX_PATH - 1, _T("%ts%ts%ts%ts"), szDrive, szDir, szFname, _T(".ini") );
	pszIniFile[_MAX_PATH - 1] = _T('\0');

	// マルチユーザ用のiniファイルパス
	//		exeと同じフォルダに置かれたマルチユーザ構成設定ファイル（sakura.exe.ini）の内容
	//		に従ってマルチユーザ用のiniファイルパスを決める
	pszPrivateIniFile[0] = _T('\0');
	if( COsVersionInfo().IsWin2000_or_later() ){
		auto_snprintf( szPath, _MAX_PATH - 1, _T("%ts%ts%ts%ts"), szDrive, szDir, szFname, _T(".exe.ini") );
		szPath[_MAX_PATH - 1] = _T('\0');
		int nEnable = ::GetPrivateProfileInt(_T("Settings"), _T("MultiUser"), 0, szPath );
		if( nEnable ){
			int nFolder = ::GetPrivateProfileInt(_T("Settings"), _T("UserRootFolder"), 0, szPath );
			switch( nFolder ){
			case 1:
				nFolder = CSIDL_PROFILE;			// ユーザのルートフォルダ
				break;
			case 2:
				nFolder = CSIDL_PERSONAL;			// ユーザのドキュメントフォルダ
				break;
			case 3:
				nFolder = CSIDL_DESKTOPDIRECTORY;	// ユーザのデスクトップフォルダ
				break;
			default:
				nFolder = CSIDL_APPDATA;			// ユーザのアプリケーションデータフォルダ
				break;
			}
			::GetPrivateProfileString(_T("Settings"), _T("UserSubFolder"), _T("sakura"), szDir, _MAX_DIR, szPath );
			if( szDir[0] == _T('\0') )
				::lstrcpy( szDir, _T("sakura") );
			if( GetSpecialFolderPath( nFolder, szPath ) ){
				auto_snprintf( pszPrivateIniFile, _MAX_PATH - 1, _T("%ts\\%ts\\%ts%ts"), szPath, szDir, szFname, _T(".ini") );
				pszPrivateIniFile[_MAX_PATH - 1] = _T('\0');
			}
		}
	}
	return;
}

/**
	iniファイル名の取得

	共有データからsakura.iniの格納フォルダを取得し、フルパス名を返す
	（共有データ未設定のときは共有データ設定を行う）

	@param[out] pszIniFileName iniファイル名（フルパス）
	@param[in] bRead true: 読み込み / false: 書き込み

	@author ryoji
	@date 2007.05.19 ryoji 新規作成
*/
void CFileNameManager::GetIniFileName( LPTSTR pszIniFileName, BOOL bRead/*=FALSE*/ )
{
	if( !m_pShareData->m_sFileNameManagement.m_IniFolder.m_bInit ){
		m_pShareData->m_sFileNameManagement.m_IniFolder.m_bInit = true;			// 初期化済フラグ
		m_pShareData->m_sFileNameManagement.m_IniFolder.m_bReadPrivate = false;	// マルチユーザ用iniからの読み出しフラグ
		m_pShareData->m_sFileNameManagement.m_IniFolder.m_bWritePrivate = false;	// マルチユーザ用iniへの書き込みフラグ

		GetIniFileNameDirect( m_pShareData->m_sFileNameManagement.m_IniFolder.m_szPrivateIniFile, m_pShareData->m_sFileNameManagement.m_IniFolder.m_szIniFile );
		if( m_pShareData->m_sFileNameManagement.m_IniFolder.m_szPrivateIniFile[0] != _T('\0') ){
			m_pShareData->m_sFileNameManagement.m_IniFolder.m_bReadPrivate = true;
			m_pShareData->m_sFileNameManagement.m_IniFolder.m_bWritePrivate = true;
			if( CCommandLine::Instance()->IsNoWindow() && CCommandLine::Instance()->IsWriteQuit() )
				m_pShareData->m_sFileNameManagement.m_IniFolder.m_bWritePrivate = false;

			// マルチユーザ用のiniフォルダを作成しておく
			if( m_pShareData->m_sFileNameManagement.m_IniFolder.m_bWritePrivate ){
				TCHAR szPath[_MAX_PATH];
				TCHAR szDrive[_MAX_DRIVE];
				TCHAR szDir[_MAX_DIR];
				_tsplitpath( m_pShareData->m_sFileNameManagement.m_IniFolder.m_szPrivateIniFile, szDrive, szDir, NULL, NULL );
				auto_snprintf( szPath, _MAX_PATH - 1, _T("%ts\\%ts"), szDrive, szDir );
				szPath[_MAX_PATH - 1] = _T('\0');
				MakeSureDirectoryPathExistsT( szPath );
			}
		}
	}

	bool bPrivate = bRead? m_pShareData->m_sFileNameManagement.m_IniFolder.m_bReadPrivate: m_pShareData->m_sFileNameManagement.m_IniFolder.m_bWritePrivate;
	::lstrcpy( pszIniFileName, bPrivate? m_pShareData->m_sFileNameManagement.m_IniFolder.m_szPrivateIniFile: m_pShareData->m_sFileNameManagement.m_IniFolder.m_szIniFile );
}

