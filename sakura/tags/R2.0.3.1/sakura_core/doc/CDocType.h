/*
	ドキュメント種別の管理

	2008.01〜03 kobake 作成
*/
#pragma once

#include "types/CType.h" // CTypeConfig
#include "env/CDocTypeManager.h"

class CDocType{
public:
	//生成と破棄
	CDocType(CEditDoc* pcDoc);
	
	//ロック機能	//	Nov. 29, 2000 genta 設定の一時変更時に拡張子による強制的な設定変更を無効にする
	void LockDocumentType(){ m_nSettingTypeLocked = true; }
	void UnlockDocumentType(){ m_nSettingTypeLocked = false; }
	bool GetDocumentLockState(){ return m_nSettingTypeLocked; }
	
	// 文書種別の設定と取得		// Nov. 23, 2000 genta
	void SetDocumentType(CTypeConfig type, bool force, bool bTypeOnly = false);	//!< 文書種別の設定
	CTypeConfig GetDocumentType() const					//!< 文書種別の取得
	{
		return m_nSettingType;
	}
	STypeConfig& GetDocumentAttribute() const						//!< 文書種別の詳細情報
	{
		return CDocTypeManager().GetTypeSetting(m_nSettingType);
	}

	// 拡張機能
	void SetDocumentIcon();	//アイコンの設定	//Sep. 10, 2002 genta

private:
	CEditDoc*		m_pcDocRef;
	CTypeConfig	m_nSettingType;
	bool			m_nSettingTypeLocked;		//!< 文書種別の一時設定状態
};
