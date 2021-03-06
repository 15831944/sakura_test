#pragma once

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           基底                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

struct DispPos;
#include "DispPos.h"

struct SColorInfo{
	CEditView*				pcView;
	const wchar_t*			pLine;
	CLogicInt				nLineLen;
	int						nBgn;
	CLogicInt				nPos;
	int						nCOMMENTMODE;
	int						nCOMMENTMODE_OLD;
	int						nCOMMENTEND;
	int						nCOMMENTEND_OLD;
	int						nColorIndex;
	BOOL					bSearchStringMode;
	bool					bKeyWordTop;
};

struct SDrawStrategyInfo{
	SDrawStrategyInfo() : sDispPosBegin(0,0) {}

	CEditView*	pcView;
	HDC			hdc;
	CLogicInt	nPos;
	int			nBgn;
	LPCWSTR		pLine;
	CLogicInt	nLineLen;
	DispPos*	pDispPos;

	EColorIndexType	nCOMMENTMODE;
	int				nCOMMENTEND;

	bool			bSearchStringMode;
	bool			bSearchFlg;
	CLogicInt		nSearchStart;
	CLogicInt		nSearchEnd;
	DispPos			sDispPosBegin;
	bool			bKeyWordTop;
	CLogicInt		nCharChars;	//###########いらないだろうなぁ
	int					nCOMMENTEND_OLD;	//#############怪しい
	EColorIndexType		nCOMMENTMODE_OLD;	//#############怪しい

	//! 現在位置の直前まで描画
	bool DrawToHere()
	{
		bool bRet = false;
		SDrawStrategyInfo* pInfo = this;
		if( pInfo->pDispPos->GetDrawPos().y >= pInfo->pcView->GetTextArea().GetAreaTop() ){
			/* テキスト表示 */
			pInfo->pcView->GetTextDrawer().DispText(
				pInfo->hdc,
				pInfo->pDispPos,
				&pInfo->pLine[pInfo->nBgn],
				pInfo->nPos - pInfo->nBgn
			);
			bRet = true;
		}
		pInfo->nBgn = pInfo->nPos;
		return bRet;
	}

	//! 色の切り替え
	void ChangeColor(EColorIndexType eNewColor)
	{
		SDrawStrategyInfo* pInfo = this;
		pInfo->nCOMMENTMODE = eNewColor;
		if( !pInfo->bSearchStringMode ){
			pInfo->pcView->SetCurrentColor( pInfo->hdc, pInfo->nCOMMENTMODE );
		}
	}

};

class CDrawStrategy{
public:
	virtual ~CDrawStrategy(){}
	//! 色切り替え開始を検出したら、その直前までの描画を行い、さらに色設定を行う。
//	virtual bool BeginColor(SDrawStrategyInfo* pInfo) = 0;
};


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                          具体化                             //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //



class CDraw_URL : public CDrawStrategy{
public:
	virtual bool BeginColor(SDrawStrategyInfo* pInfo);
	virtual bool GetColorIndexImp(SColorInfo* pInfo);
};

class CDraw_Numeric : public CDrawStrategy{
public:
	virtual bool BeginColor(SDrawStrategyInfo* pInfo);
	virtual bool GetColorIndexImp(SColorInfo* pInfo);
};

class CDraw_KeywordSet : public CDrawStrategy{
public:
	virtual bool BeginColor(SDrawStrategyInfo* pInfo);
	virtual bool GetColorIndexImp(SColorInfo* pInfo);
};


class CDraw_CtrlCode : public CDrawStrategy{
public:
	virtual bool BeginColor(SDrawStrategyInfo* pInfo);
	virtual bool EndColor(SDrawStrategyInfo* pInfo);
};



//共通

class CDraw_ColorEnd : public CDrawStrategy{
public:
	virtual bool EndColor(SDrawStrategyInfo* pInfo);
};

class CDraw_Line : public CDrawStrategy{
public:
	virtual bool EndColor(SDrawStrategyInfo* pInfo);
};
