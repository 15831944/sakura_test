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
#ifndef SAKURA_CCOLORSTRATEGY_BC7B5956_A0AF_4C9C_9C0E_07FE658028AC9_H_
#define SAKURA_CCOLORSTRATEGY_BC7B5956_A0AF_4C9C_9C0E_07FE658028AC9_H_

// 要先行定義
// #include "view/CEditView.h"
#include "EColorIndexType.h"
#include "CGraphics.h"

class	CEditView;

bool _IsPosKeywordHead(const CStringRef& cStr, int nPos);


//正規表現キーワードのEColorIndexType値を作る関数
inline EColorIndexType ToColorIndexType_RegularExpression(const int& nRegexColorIndex)
{
	return (EColorIndexType)(COLORIDX_REGEX_FIRST + nRegexColorIndex);
}

//正規表現キーワードのEColorIndexType値かどうか
inline bool IsRegularExpression(const EColorIndexType& eColorIndex)
{
	return (eColorIndex >= COLORIDX_REGEX_FIRST && eColorIndex <= COLORIDX_REGEX_LAST);
}

//正規表現キーワードのEColorIndexType値を色番号に戻す関数
inline int ToColorInfoArrIndex_RegularExpression(const EColorIndexType& eRegexColorIndex)
{
	return eRegexColorIndex - COLORIDX_REGEX_FIRST;
}

//EColorIndexType値を色番号に変換する関数
inline int ToColorInfoArrIndex(const EColorIndexType& eColorIndex)
{
	if(eColorIndex>=0 && eColorIndex<COLORIDX_LAST)
		return eColorIndex;
	else if(eColorIndex==COLORIDX_BLOCK1 || eColorIndex==COLORIDX_BLOCK2)
		return COLORIDX_COMMENT;
	else if( IsRegularExpression(eColorIndex) )
		return ToColorInfoArrIndex_RegularExpression(eColorIndex);

	return -1;
}

// カラー名＜＞インデックス番号の変換	//@@@ 2002.04.30
SAKURA_CORE_API int GetColorIndexByName( const TCHAR *name );
SAKURA_CORE_API const TCHAR* GetColorNameByIndex( int index );


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           基底                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

struct DispPos;
class CColorStrategy;
#include "view/DispPos.h"

class CColor_Found;
class CColor_Select;

struct SColorStrategyInfo{
	SColorStrategyInfo() : sDispPosBegin(0,0), pStrategy(NULL), pStrategyFound(NULL), pStrategySelect(NULL), m_colorIdxBackLine(COLORIDX_TEXT) {}

	//参照
	CEditView*	pcView;
	CGraphics	gr;	//(SColorInfoでは未使用)

	//スキャン位置
	LPCWSTR			pLineOfLogic;
	CLogicInt		nPosInLogic;
	CLayoutInt		nLayoutLineNum;

	//描画位置
	DispPos*		pDispPos;
	DispPos			sDispPosBegin;

	//色変え
	CColorStrategy*		pStrategy;
	CColor_Found*		pStrategyFound;
	CColor_Select*		pStrategySelect;
	EColorIndexType		m_colorIdxBackLine;

	//! 色の切り替え
	void ChangeColor(EColorIndexType eNewColor)
	{
		this->pcView->SetCurrentColor( this->gr, eNewColor );
	}
	void ChangeColor2(EColorIndexType eNewColor, EColorIndexType eNewColor2)
	{
		this->pcView->SetCurrentColor3(this->gr, eNewColor, eNewColor2, m_colorIdxBackLine);
	}

	void DoChangeColor(const CStringRef& cLineStr);
	EColorIndexType GetCurrentColor() const;
	EColorIndexType GetCurrentColor2() const;
	EColorIndexType GetCurrentColorBg() const{ return m_colorIdxBackLine; }

	//! 現在のスキャン位置
	CLogicInt GetPosInLogic() const
	{
		return nPosInLogic;
	}
	CLogicInt GetPosInLayout() const;
	const CDocLine* GetDocLine() const;
	const CLayout* GetLayout() const;
};

class CColorStrategy{
public:
	virtual ~CColorStrategy(){}
	//! 色定義
	virtual EColorIndexType GetStrategyColor() const = 0;
	//! 色切り替え開始を検出したら、その直前までの描画を行い、さらに色設定を行う。
	virtual void InitStrategyStatus() = 0;
	virtual bool BeginColor(const CStringRef& cStr, int nPos){ return false; }
	virtual bool EndColor(const CStringRef& cStr, int nPos){ return true; }
	//イベント
	virtual void OnStartScanLogic(){}

	//#######ラップ
	EColorIndexType GetStrategyColorSafe() const{ if(this)return GetStrategyColor(); else return COLORIDX_TEXT; }
};

#include "util/design_template.h"
#include <vector>
class CColor_LineComment;
class CColor_BlockComment;
class CColor_BlockComment;
class CColor_SingleQuote;
class CColor_DoubleQuote;

class CColorStrategyPool : public TSingleton<CColorStrategyPool>{
public:
	//コンストラクタ・デストラクタ
	CColorStrategyPool();
	virtual ~CColorStrategyPool();

	//取得
	CColorStrategy*	GetStrategy(int nIndex) const{ return m_vStrategies[nIndex]; }
	int				GetStrategyCount() const{ return (int)m_vStrategies.size(); }
	CColorStrategy*	GetStrategyByColor(EColorIndexType eColor) const;

	//特定取得
	CColor_Found*   GetFoundStrategy() const{ return m_pcFoundStrategy; }
	CColor_Select*  GetSelectStrategy() const{ return m_pcSelectStrategy; }

	//イベント
	void NotifyOnStartScanLogic();

	/*
	|| 色分け
	*/
	//@@@ 2002.09.22 YAZAKI
	// 2005.11.21 Moca 引用符の色分け情報を引数から除去
	bool CheckColorMODE( CColorStrategy** ppcColorStrategy, int nPos, const CStringRef& cLineStr );

	//ビューの設定・取得
	CEditView* GetCurrentView(void) const{ return m_pcView; }
	void SetCurrentView(CEditView* pcView) { m_pcView = pcView; }

private:
	std::vector<CColorStrategy*>	m_vStrategies;
	CColor_Found*					m_pcFoundStrategy;
	CColor_Select*					m_pcSelectStrategy;

	CColor_LineComment*				m_pcLineComment;
	CColor_BlockComment*			m_pcBlockComment1;
	CColor_BlockComment*			m_pcBlockComment2;
	CColor_SingleQuote*				m_pcSingleQuote;
	CColor_DoubleQuote*				m_pcDoubleQuote;

	CEditView*						m_pcView;
};

#endif /* SAKURA_CCOLORSTRATEGY_BC7B5956_A0AF_4C9C_9C0E_07FE658028AC9_H_ */
/*[EOF]*/
