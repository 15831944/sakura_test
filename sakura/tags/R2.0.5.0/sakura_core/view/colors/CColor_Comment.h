/*
	Copyright (C) 2008, kobake

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
#ifndef SAKURA_CCOLOR_COMMENT_5CBF888D_32A3_4713_8967_D6D49F6E569F_H_
#define SAKURA_CCOLOR_COMMENT_5CBF888D_32A3_4713_8967_D6D49F6E569F_H_

#include "view/colors/CColorStrategy.h"


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                        行コメント                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

class CColor_LineComment : public CColorStrategy{
public:
	virtual EColorIndexType GetStrategyColor() const{ return COLORIDX_COMMENT; }
	virtual void InitStrategyStatus(){}
	virtual bool BeginColor(const CStringRef& cStr, int nPos);
	virtual bool EndColor(const CStringRef& cStr, int nPos);
};


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                    ブロックコメント１                       //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

class CColor_BlockComment : public CColorStrategy{
public:
	CColor_BlockComment(int nType) : m_nType(nType), m_nCOMMENTEND(0) { }
	virtual EColorIndexType GetStrategyColor() const{ return (EColorIndexType)(COLORIDX_BLOCK1 + m_nType); }
	virtual void InitStrategyStatus(){ m_nCOMMENTEND = 0; }
	virtual bool BeginColor(const CStringRef& cStr, int nPos);
	virtual bool EndColor(const CStringRef& cStr, int nPos);
private:
	int m_nType; //0 or 1 (コメント種)
	int m_nCOMMENTEND;
};

#endif /* SAKURA_CCOLOR_COMMENT_5CBF888D_32A3_4713_8967_D6D49F6E569F_H_ */
/*[EOF]*/
