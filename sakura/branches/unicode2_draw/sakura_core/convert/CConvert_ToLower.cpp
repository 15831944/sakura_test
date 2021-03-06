#include "stdafx.h"
#include "CConvert_ToLower.h"

//! 小文字へ変換
bool CConvert_ToLower::DoConvert(CNativeW* pcData)
{
	WCHAR* p = pcData->GetStringPtr();
	WCHAR* end = p + pcData->GetStringLength();
	while(p < end){
		WCHAR& c=*p;
		// A-Z → a-z
		if(c>=0x0041 && c<=0x005A){
			c=0x0061+(c-0x0041);
		}
		// Ａ-Ｚ → ａ-ｚ
		else if( c>=0xFF21 && c<=0xFF3A){
			c=0xFF41+(c-0xFF21);
		}
		// ギリシャ文字変換
		else if( c>=0x0391 && c<=0x03A9){
			c=0x03B1+(c-0x0391);
		}
		// ロシア文字変換
		else if( c>=0x0410 && c<=0x042F){
			c=0x0430+(c-0x0410);
		}
		p++;
	}
	return true;
}
