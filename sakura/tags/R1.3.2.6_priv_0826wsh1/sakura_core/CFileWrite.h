/*! 
 * @file CFileWrite.h
 * @brief ファイル書き込み用クラス
 *
 * @author Frozen
 * @date 2000/05/22
 *
 */
#ifndef FILE_WRITE_H
#define FILE_WRITE_H

//! ファイルのオープンに失敗したときに送出する例外
class CError_FileOpen
{
};
//! ファイルの書き込みに失敗したときに送出する例外
class CError_FileWrite
{
};

//! ファイル書き込みクラス（Cのライブラリ関数を使用して実装）
class CFileWrite
{
public:
	CFileWrite(const TCHAR* pszPath);
	~CFileWrite();
	inline void Write(const void* buffer,size_t size)
	{
		if(::fwrite(buffer,1,size,m_hFile)!=size)
			throw CError_FileWrite();
	}
private:
	FILE* m_hFile;
	DWORD m_dwFileAttribute;
	TCHAR* m_pszPath;
	bool m_ChangeAttribute;	
};

#endif // FILE_WRITE_H