#pragma once

#include "CConvert.h"


//!半角にできるものは全部半角に変換
class CConvert_ToHankaku : public CConvert{
public:
	bool DoConvert(CNativeW* pcData);
};


enum EToHankakuMode{
	TO_KATAKANA	= 0x01, //!< カタカナに影響アリ
	TO_HIRAGANA	= 0x02, //!< ひらがなに影響アリ
	TO_EISU		= 0x04, //!< 英数字に影響アリ
};
