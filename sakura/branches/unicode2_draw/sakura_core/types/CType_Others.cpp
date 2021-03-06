#include "stdafx.h"
#include "types/CType.h"

#define IMPLEMENT_CTYPE_OTHER(CLASS_NAME, TYPE_NAME) \
void CLASS_NAME::InitTypeConfigImp(STypeConfig* pType) \
{ \
	_tcscpy( pType->m_szTypeName, _T(#TYPE_NAME) ); \
	_tcscpy( pType->m_szTypeExts, _T("") ); \
}

IMPLEMENT_CTYPE_OTHER(CType_Other1, 設定17)
IMPLEMENT_CTYPE_OTHER(CType_Other2, 設定18)
IMPLEMENT_CTYPE_OTHER(CType_Other3, 設定19)
IMPLEMENT_CTYPE_OTHER(CType_Other4, 設定20)
IMPLEMENT_CTYPE_OTHER(CType_Other5, 設定21)
IMPLEMENT_CTYPE_OTHER(CType_Other6, 設定22)
IMPLEMENT_CTYPE_OTHER(CType_Other7, 設定23)
IMPLEMENT_CTYPE_OTHER(CType_Other8, 設定24)
IMPLEMENT_CTYPE_OTHER(CType_Other9, 設定25)
IMPLEMENT_CTYPE_OTHER(CType_Other10, 設定26)
IMPLEMENT_CTYPE_OTHER(CType_Other11, 設定27)
IMPLEMENT_CTYPE_OTHER(CType_Other12, 設定28)
IMPLEMENT_CTYPE_OTHER(CType_Other13, 設定29)
IMPLEMENT_CTYPE_OTHER(CType_Other14, 設定30)

