#pragma once

#include "view/colors/CColorStrategy.h"



class CColor_RegexKeyword : public CColorStrategy{
public:
	virtual EColorIndexType BeginColor(SColorStrategyInfo* pInfo);
	virtual bool EndColor(SColorStrategyInfo* pInfo);
};
