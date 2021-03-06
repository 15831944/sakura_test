#pragma once

#include "doc/CDocListener.h"

class CBackupAgent : public CDocListenerEx{
public:
	ECallbackResult OnPreBeforeSave(SSaveInfo* pSaveInfo);

protected:
	int MakeBackUp( const TCHAR* target_file );								//!< バックアップの作成
	bool FormatBackUpPath( TCHAR* szNewPath, const TCHAR* target_file );	//!< バックアップパスの作成 2005.11.21 aroka
};
