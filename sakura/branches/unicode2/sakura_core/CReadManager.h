#pragma once

#include "CDocListener.h"
class CDocLineMgr;

class CReadManager : public CProgressSubject{
public:
	//	Nov. 12, 2000 genta 引数追加
	//	Jul. 26, 2003 ryoji BOM引数追加
	int ReadFile_To_CDocLineMgr(
		CDocLineMgr*		pcDocLineMgr,
		const SLoadInfo&	sLoadInfo,
		SFileInfo*			pFileInfo
	);
};
