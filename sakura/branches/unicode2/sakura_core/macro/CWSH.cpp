/*!	@file
	@brief WSH Handler

	@author 鬼
	@date 2002年4月28日
*/
/*
	Copyright (C) 2002, 鬼, genta
	Copyright (C) 2003, FILE
	Copyright (C) 2004, genta
	Copyright (C) 2005, FILE, zenryaku

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.

*/

#include "stdafx.h"
#include "CWSH.h"
#include "CWSHManager.h"
#include "CMacroFactory.h"
#include "CMacro.h"
#include "macro/CSMacroMgr.h"
#include "view/CEditView.h"
#include "doc/CEditDoc.h"
#include "OleTypes.h"
#include "io/CTextStream.h"
#include "util/os.h"

//スクリプトに渡されるオブジェクトの情報
class CInterfaceObjectTypeInfo: public ImplementsIUnknown<ITypeInfo>
{
private:
	CInterfaceObject *m_Object;
	TYPEATTR m_TypeAttr;
public:
	CInterfaceObjectTypeInfo(CInterfaceObject *AObject);

	virtual HRESULT STDMETHODCALLTYPE GetTypeAttr(
					/* [out] */ TYPEATTR __RPC_FAR *__RPC_FAR *ppTypeAttr)
	{
#ifdef TEST
		cout << "GetTypeAttr" << endl;
#endif
		*ppTypeAttr = &m_TypeAttr;
		return S_OK;
	}
        
	virtual HRESULT STDMETHODCALLTYPE GetTypeComp( 
					/* [out] */ ITypeComp __RPC_FAR *__RPC_FAR *ppTComp)
	{
#ifdef TEST
		cout << "GetTypeComp" << endl;
#endif
		return E_NOTIMPL;
	}
        
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetFuncDesc( 
				/* [in] */ UINT index,
				/* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetVarDesc( 
	    /* [in] */ UINT index,
	    /* [out] */ VARDESC __RPC_FAR *__RPC_FAR *ppVarDesc)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetNames( 
	    /* [in] */ MEMBERID memid,
	    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
	    /* [in] */ UINT cMaxNames,
	    /* [out] */ UINT __RPC_FAR *pcNames);

	virtual HRESULT STDMETHODCALLTYPE GetRefTypeOfImplType( 
	    /* [in] */ UINT index,
	    /* [out] */ HREFTYPE __RPC_FAR *pRefType)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetImplTypeFlags( 
	    /* [in] */ UINT index,
	    /* [out] */ INT __RPC_FAR *pImplTypeFlags)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
	    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
	    /* [in] */ UINT cNames,
	    /* [size_is][out] */ MEMBERID __RPC_FAR *pMemId)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke( 
	    /* [in] */ PVOID pvInstance,
	    /* [in] */ MEMBERID memid,
	    /* [in] */ WORD wFlags,
	    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
	    /* [out] */ VARIANT __RPC_FAR *pVarResult,
	    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
	    /* [out] */ UINT __RPC_FAR *puArgErr)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDocumentation( 
	    /* [in] */ MEMBERID memid,
	    /* [out] */ BSTR __RPC_FAR *pBstrName,
	    /* [out] */ BSTR __RPC_FAR *pBstrDocString,
	    /* [out] */ DWORD __RPC_FAR *pdwHelpContext,
	    /* [out] */ BSTR __RPC_FAR *pBstrHelpFile)
	{
		//	Feb. 08, 2004 genta
		//	とりあえず全部NULLを返す (情報無し)
		pBstrName = NULL;
		pBstrDocString = NULL;
		pdwHelpContext = NULL;
		pBstrHelpFile = NULL;
		return S_OK ;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDllEntry( 
	    /* [in] */ MEMBERID memid,
	    /* [in] */ INVOKEKIND invKind,
	    /* [out] */ BSTR __RPC_FAR *pBstrDllName,
	    /* [out] */ BSTR __RPC_FAR *pBstrName,
	    /* [out] */ WORD __RPC_FAR *pwOrdinal)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetRefTypeInfo( 
	    /* [in] */ HREFTYPE hRefType,
	    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE AddressOfMember( 
	    /* [in] */ MEMBERID memid,
	    /* [in] */ INVOKEKIND invKind,
	    /* [out] */ PVOID __RPC_FAR *ppv)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateInstance( 
	    /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
	    /* [in] */ REFIID riid,
	    /* [iid_is][out] */ PVOID __RPC_FAR *ppvObj)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMops( 
	    /* [in] */ MEMBERID memid,
	    /* [out] */ BSTR __RPC_FAR *pBstrMops)
	{
		return E_NOTIMPL;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetContainingTypeLib( 
	    /* [out] */ ITypeLib __RPC_FAR *__RPC_FAR *ppTLib,
	    /* [out] */ UINT __RPC_FAR *pIndex)
	{
		return E_NOTIMPL;
	}
        
	virtual /* [local] */ void STDMETHODCALLTYPE ReleaseTypeAttr( 
					/* [in] */ TYPEATTR __RPC_FAR *pTypeAttr)
	{
	}
        
	virtual /* [local] */ void STDMETHODCALLTYPE ReleaseFuncDesc( 
					/* [in] */ FUNCDESC __RPC_FAR *pFuncDesc)
	{
	}
        
	virtual /* [local] */ void STDMETHODCALLTYPE ReleaseVarDesc(
				/* [in] */ VARDESC __RPC_FAR *pVarDesc)
	{
	}
};

//IActiveScriptSite, IActiveScriptSiteWindow
/*!
	@date Sep. 15, 2005 FILE IActiveScriptSiteWindow実装．
		マクロでMsgBoxを使用可能にする．
*/
class CWSHSite: public IActiveScriptSite, public IActiveScriptSiteWindow
{
private:
	CWSHClient *m_Client;
	ITypeInfo *m_TypeInfo;
	ULONG m_RefCount;
public:
	CWSHSite(CWSHClient *AClient): m_RefCount(0), m_Client(AClient)
	{
	}

	virtual ULONG _stdcall AddRef() {
		return ++m_RefCount;
	}

	virtual ULONG _stdcall Release() {
		if(--m_RefCount == 0)
		{
			delete this;
			return 0;
		}
		return m_RefCount;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
	    /* [in] */ REFIID iid,
	    /* [out] */ void ** ppvObject)
	{
		*ppvObject = NULL;

		if(iid == IID_IActiveScriptSiteWindow){
			*ppvObject = static_cast<IActiveScriptSiteWindow*>(this);
			++m_RefCount;
			return S_OK;
		}

		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetLCID( 
	    /* [out] */ LCID *plcid) 
	{ 
#ifdef TEST
		cout << "GetLCID" << endl;
#endif
		return E_NOTIMPL; //システムデフォルトを使用
	};

	virtual HRESULT STDMETHODCALLTYPE GetItemInfo( 
	    /* [in] */ LPCOLESTR pstrName,
	    /* [in] */ DWORD dwReturnMask,
	    /* [out] */ IUnknown **ppiunkItem,
	    /* [out] */ ITypeInfo **ppti) 
	{
#ifdef TEST
		wcout << L"GetItemInfo:" << pstrName << endl;
#endif
		//	Nov. 10, 2003 FILE Win9Xでは、[lstrcmpiW]が無効のため、[_wcsicmp]に修正
		if(_wcsicmp(pstrName, L"Editor") == 0)
		{
			if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
			{
				(*ppiunkItem) = m_Client->m_InterfaceObject;
				(*ppiunkItem)->AddRef();
			}
			if(dwReturnMask & SCRIPTINFO_ITYPEINFO)
			{
				m_Client->m_InterfaceObject->GetTypeInfo(0, 0, ppti);
			}
			return S_OK;
		}
		return TYPE_E_ELEMENTNOTFOUND;
	};

	virtual HRESULT STDMETHODCALLTYPE GetDocVersionString( 
	    /* [out] */ BSTR *pbstrVersion) 
	{ 
#ifdef TEST
		cout << "GetDocVersionString" << endl;
#endif
		return E_NOTIMPL; 
	};

	virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate( 
	    /* [in] */ const VARIANT *pvarResult,
	    /* [in] */ const EXCEPINFO *pexcepinfo) 
	{ 
#ifdef TEST
		cout << "OnScriptTerminate" << endl;
#endif
		return S_OK; 
	};

	virtual HRESULT STDMETHODCALLTYPE OnStateChange( 
	    /* [in] */ SCRIPTSTATE ssScriptState) 
	{ 
#ifdef TEST
		cout << "OnStateChange" << endl;
#endif
		return S_OK; 
	};

	//	Nov. 3, 2002 鬼
	//	エラー行番号表示対応
	virtual HRESULT STDMETHODCALLTYPE OnScriptError(
	  /* [in] */ IActiveScriptError *pscripterror)
	{ 
		EXCEPINFO Info;
		if(pscripterror->GetExceptionInfo(&Info) == S_OK)
		{
			DWORD Context;
			ULONG Line;
			LONG Pos;
			if(pscripterror->GetSourcePosition(&Context, &Line, &Pos) == S_OK)
			{
				wchar_t *Message = new wchar_t[SysStringLen(Info.bstrDescription) + 128];
				//	Nov. 10, 2003 FILE Win9Xでは、[wsprintfW]が無効のため、[auto_sprintf]に修正
				const wchar_t* szDesc=Info.bstrDescription;
				auto_sprintf(Message, L"[Line %d] %ls", Line + 1, szDesc);
				SysReAllocString(&Info.bstrDescription, Message);
				delete[] Message;
			}
			m_Client->Error(Info.bstrDescription, Info.bstrSource);
			SysFreeString(Info.bstrSource);
			SysFreeString(Info.bstrDescription);
			SysFreeString(Info.bstrHelpFile);
		}
		return S_OK;
	};

	virtual HRESULT STDMETHODCALLTYPE OnEnterScript() {
#ifdef TEST
		cout << "OnEnterScript" << endl;
#endif
		return S_OK; 
	};

	virtual HRESULT STDMETHODCALLTYPE OnLeaveScript() {
#ifdef TEST
		cout << "OnLeaveScript" << endl;
#endif
		return S_OK; 
	};

	//	Sep. 15, 2005 FILE IActiveScriptSiteWindow実装
	virtual HRESULT __stdcall GetWindow(
	    /* [out] */ HWND *phwnd)
	{
		*phwnd = CEditWnd::Instance()->m_cSplitterWnd.GetHwnd();
		return S_OK;
	}

	//	Sep. 15, 2005 FILE IActiveScriptSiteWindow実装
	virtual HRESULT __stdcall EnableModeless(
	    /* [in] */ BOOL fEnable)
	{
		return S_OK;
	};
};

//implementation

CInterfaceObjectTypeInfo::CInterfaceObjectTypeInfo(CInterfaceObject *AObject)
				: ImplementsIUnknown<ITypeInfo>(), m_Object(AObject)
{ 
	ZeroMemory(&m_TypeAttr, sizeof(m_TypeAttr));
	m_TypeAttr.cImplTypes = 0; //親クラスのITypeInfoの数
	m_TypeAttr.cFuncs = m_Object->m_Methods.size();
}

HRESULT STDMETHODCALLTYPE CInterfaceObjectTypeInfo::GetFuncDesc( 
			/* [in] */ UINT index,
			/* [out] */ FUNCDESC __RPC_FAR *__RPC_FAR *ppFuncDesc)
{
#ifdef TEST
	cout << "GetFuncDesc" << endl;
#endif
	*ppFuncDesc = &(m_Object->m_Methods[index].Desc);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInterfaceObjectTypeInfo::GetNames( 
    /* [in] */ MEMBERID memid,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *rgBstrNames,
    /* [in] */ UINT cMaxNames,
    /* [out] */ UINT __RPC_FAR *pcNames)
{
#ifdef TEST
		cout << "GetNames" << endl;
#endif
	*pcNames = 1;
	if(cMaxNames > 0)
		*rgBstrNames = SysAllocString(m_Object->m_Methods[memid].Name);
	return S_OK;
}

//スクリプトに渡されるオブジェクト
CInterfaceObject::CInterfaceObject(CWSHClient *AOwner): ImplementsIUnknown<IDispatch>(), 
				m_Methods(), m_TypeInfo(NULL), m_Owner(AOwner)
{ 
};

CInterfaceObject::~CInterfaceObject()
{
	if(m_TypeInfo != NULL)
		m_TypeInfo->Release();
}
	

HRESULT STDMETHODCALLTYPE CInterfaceObject::QueryInterface(REFIID iid, void ** ppvObject) 
{
	if(ppvObject == NULL) 
		return E_POINTER;
	else if(IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDispatch))
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
		return E_NOINTERFACE;
}
	
HRESULT STDMETHODCALLTYPE CInterfaceObject::Invoke(
				DISPID dispidMember,
				REFIID riid,
				LCID lcid,
				WORD wFlags,
				DISPPARAMS FAR* pdispparams,
				VARIANT FAR* pvarResult,
				EXCEPINFO FAR* pexcepinfo,
				UINT FAR* puArgErr)
{
	if((unsigned)dispidMember < m_Methods.size())
		return (*m_Methods[dispidMember].Method)(m_Methods[dispidMember].ID, pdispparams, pvarResult, m_Owner->m_Data);
	else
		return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE CInterfaceObject::GetTypeInfo( 
				/* [in] */ UINT iTInfo,
				/* [in] */ LCID lcid,
				/* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
	if(m_TypeInfo == NULL)
	{
		m_TypeInfo = new CInterfaceObjectTypeInfo(this);
		m_TypeInfo->AddRef();
	}
		
	(*ppTInfo) = m_TypeInfo;
	(*ppTInfo)->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CInterfaceObject::GetTypeInfoCount( 
				/* [out] */ UINT __RPC_FAR *pctinfo)
{
	*pctinfo = 1;
	return S_OK;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
HRESULT STDMETHODCALLTYPE CInterfaceObject::GetIDsOfNames(
  REFIID riid,
  OLECHAR FAR* FAR* rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID FAR* rgdispid)
{
	for(unsigned I = 0; I < cNames; ++I)
	{
#ifdef TEST
		wcout << L"GetIDsOfNames:" << rgszNames[I] << endl;
#endif
		for(unsigned J = 0; J < m_Methods.size(); ++J)
		{
			//	Nov. 10, 2003 FILE Win9Xでは、[lstrcmpiW]が無効のため、[_wcsicmp]に修正
			if(_wcsicmp(rgszNames[I], m_Methods[J].Name) == 0)
			{
				rgdispid[I] = J;
				goto Found;
			}
		}
		return DISP_E_UNKNOWNNAME;
		Found:
		;
	}
	return S_OK;		
}

void CInterfaceObject::AddMethod(
	wchar_t*				Name,
	EFunctionCode			ID,
	VARTYPE*				ArgumentTypes,
	int						ArgumentCount,
	VARTYPE					ResultType,
	CInterfaceObjectMethod	Method
)
{
	m_Methods.push_back(CMethodInfo());
	CMethodInfo *Info = &m_Methods[m_Methods.size() - 1];
	ZeroMemory(Info, sizeof(CMethodInfo));
	Info->Desc.invkind = INVOKE_FUNC;
	Info->Desc.cParams = ArgumentCount + 1; //戻り値の分
	Info->Desc.lprgelemdescParam = Info->Arguments;
	//	Nov. 10, 2003 FILE Win9Xでは、[lstrcpyW]が無効のため、[wcscpy]に修正
	wcscpy(Info->Name, Name);
	Info->Method = Method;
	Info->ID = ID;
	for(int I = 0; I < ArgumentCount; ++I)
	{
		Info->Arguments[I].tdesc.vt = ArgumentTypes[ArgumentCount - I - 1];
		Info->Arguments[I].paramdesc.wParamFlags = PARAMFLAG_FIN;
	}
	Info->Arguments[ArgumentCount].tdesc.vt = ResultType;
	Info->Arguments[ArgumentCount].paramdesc.wParamFlags = PARAMFLAG_FRETVAL;
}

CWSHClient::CWSHClient(wchar_t const *AEngine, ScriptErrorHandler AErrorHandler, void *AData): 
				m_Engine(NULL), m_Data(AData), m_OnError(AErrorHandler), m_Valid(false)
{ 
	m_InterfaceObject = new CInterfaceObject(this);
	m_InterfaceObject->AddRef();
	
	CLSID ClassID;
	if(CLSIDFromProgID(AEngine, &ClassID) != S_OK)
		Error(L"指名のスクリプトエンジンが見つかりません");
	else
	{
		if(CoCreateInstance(ClassID, 0, CLSCTX_INPROC_SERVER, IID_IActiveScript, reinterpret_cast<void **>(&m_Engine)) != S_OK)
			Error(L"指名のスクリプトエンジンが作成できません");
		else
		{
			IActiveScriptSite *Site = new CWSHSite(this);
			if(m_Engine->SetScriptSite(Site) != S_OK)
			{
				delete Site;
				Error(L"サイトを登録できません");
			}
			else
			{
				m_Valid = true;
			}
		}
	}
}

CWSHClient::~CWSHClient()
{
	if(m_InterfaceObject != NULL)
		m_InterfaceObject->Release();
	
	if(m_Engine != NULL) 
		m_Engine->Release();
}

void CWSHClient::Execute(wchar_t const *AScript)
{
	IActiveScriptParse *Parser;
	if(m_Engine->QueryInterface(IID_IActiveScriptParse, reinterpret_cast<void **>(&Parser)) != S_OK)
		Error(L"パーサを取得できません");
	else 
	{
		if(Parser->InitNew() != S_OK)
			Error(L"初期化できません");
		else
		{
			if(m_Engine->AddNamedItem(L"Editor", SCRIPTITEM_GLOBALMEMBERS | SCRIPTITEM_ISVISIBLE) != S_OK)
				Error(L"オブジェクトを渡せなかった");
			else
			{
				if(m_Engine->SetScriptState(SCRIPTSTATE_STARTED) != S_OK)
					Error(L"状態変更エラー");
				else
				{
					if(Parser->ParseScriptText(AScript, 0, 0, 0, 0, 0, SCRIPTTEXT_ISVISIBLE, 0, 0) != S_OK)
						Error(L"実行に失敗しました");
				}
			}
		}
		Parser->Release();
	}
	m_Engine->Close();
}

void CWSHClient::Error(BSTR Description, BSTR Source)
{
	if(m_OnError != NULL)
		m_OnError(Description, Source, m_Data);
}

void CWSHClient::Error(wchar_t* Description)
{
	BSTR S = SysAllocString(L"WSH");
	BSTR D = SysAllocString(Description);
	Error(D, S);
	SysFreeString(S);
	SysFreeString(D);
}

/////////////////////////////////////////////
/*!
	マクロコマンドの実行

	@date 2005.06.27 zenryaku 戻り値の受け取りが無くてもエラーにせずに関数を実行する
*/
static HRESULT MacroCommand(EFunctionCode ID, DISPPARAMS *Arguments, VARIANT* Result, void *Data)
{
	CEditView *View = reinterpret_cast<CEditView*>(Data);

	if(ID >= F_FUNCTION_FIRST)
	{
		VARIANT ret; // 2005.06.27 zenryaku 戻り値の受け取りが無くても関数を実行する
		VariantInit(&ret);
		
		bool r = CMacro::HandleFunction(View, ID, Arguments->rgvarg, Arguments->cArgs, ret);
		if(Result) {::VariantCopyInd(Result, &ret);}
		VariantClear(&ret);
		return r ? S_OK : E_FAIL;
	}
	else
	{
		int ArgCount = Arguments->cArgs;
		if(ArgCount > 4) ArgCount = 4;

		//	Nov. 29, 2005 FILE 引数を文字列で取得する
		WCHAR *StrArgs[4] = {NULL, NULL, NULL, NULL};	// 初期化必須
		WCHAR *S = NULL;								// 初期化必須
		Variant varCopy;							// VT_BYREFだと困るのでコピー用
		int Len;
		for(int I = 0; I < ArgCount; ++I)
		{
			if(VariantChangeType(&varCopy.Data, &(Arguments->rgvarg[I]), 0, VT_BSTR) == S_OK)
			{
				Wrap(&varCopy.Data.bstrVal)->GetW(&S, &Len);
			}
			else
			{
				S = new WCHAR[1];
				S[0] = 0;
			}
			StrArgs[ArgCount - I - 1] = S;
		}

		CMacro::HandleCommand(View, ID, const_cast<WCHAR const **>(StrArgs), ArgCount);

		//	Nov. 29, 2005 FILE 配列の破棄なので、[括弧]を追加
		for(int J = 0; J < ArgCount; ++J)
			delete [] StrArgs[J];

		return S_OK;
	}
}

static void MacroError(BSTR Description, BSTR Source, void *Data)
{
	CEditView *View = reinterpret_cast<CEditView*>(Data);
	MessageBox(View->GetHwnd(), to_tchar(Description), to_tchar(Source), MB_ICONERROR);
}


CWSHMacroManager::CWSHMacroManager(std::wstring const AEngineName) : m_EngineName(AEngineName)
{
}

CWSHMacroManager::~CWSHMacroManager()
{
}

void CWSHMacroManager::ReadyCommands(CInterfaceObject *Object, MacroFuncInfo *Info)
{
	while(Info->m_nFuncID != -1)	// Aug. 29, 2002 genta 番人の値が変更されたのでここも変更
	{
		wchar_t FuncName[256];
		wcscpy(FuncName, Info->m_pszFuncName);

		int ArgCount = 0;
		for(int I = 0; I < 4; ++I)
			if(Info->m_varArguments[I] != VT_EMPTY) 
				++ArgCount;
		
		Object->AddMethod(
			FuncName,
			Info->m_nFuncID,
			Info->m_varArguments,
			ArgCount,
			Info->m_varResult,
			MacroCommand
		);
		
		++Info;
	}
}

/*!
	WSHマクロの実行

	@param EditView [in] 操作対象EditView
*/
void CWSHMacroManager::ExecKeyMacro(CEditView *EditView) const
{
	CWSHClient* Engine;
	Engine = new CWSHClient(m_EngineName.c_str(), MacroError, EditView);
	if(Engine->m_Valid)
	{
		ReadyCommands(Engine->m_InterfaceObject, CSMacroMgr::m_MacroFuncInfoArr);
		ReadyCommands(Engine->m_InterfaceObject, CSMacroMgr::m_MacroFuncInfoNotCommandArr);
		
		Engine->Execute(m_Source.c_str());
		
		//EditView->Redraw();
		EditView->GetCaret().ShowEditCaret();
	}
	delete Engine;
}

/*!
	WSHマクロの読み込み

	@param Instance [in] インスタンスハンドル(未使用)
	@param Path		[in] ファイルのパス
*/
BOOL CWSHMacroManager::LoadKeyMacro(HINSTANCE hInstance, const TCHAR* szPath)
{
	//ソース読み込み -> m_Source
	m_Source=L"";
	
	CTextInputStream in(szPath);
	if(!in)
		return FALSE;

	while(in){
		m_Source+=in.ReadLineW()+L"\r\n";
	}
	return TRUE;
}

CMacroManagerBase* CWSHMacroManager::Creator(const TCHAR* FileExt)
{
	TCHAR FileExtWithDot[1024], FileType[1024], EngineName[1024]; //1024を超えたら後は知りません
	
	_tcscpy( FileExtWithDot, _T(".") );
	_tcscat( FileExtWithDot, FileExt );

	if(ReadRegistry(HKEY_CLASSES_ROOT, FileExtWithDot, NULL, FileType, 1024))
	{
		lstrcat(FileType, _T("\\ScriptEngine"));
		if(ReadRegistry(HKEY_CLASSES_ROOT, FileType, NULL, EngineName, 1024))
		{
			wchar_t EngineNameW[1024];
			_tcstowcs(EngineNameW, EngineName, _countof(EngineNameW));
			return new CWSHMacroManager(EngineNameW);
		}
	}
	return NULL;
}

void CWSHMacroManager::declare()
{
	//暫定
	CMacroFactory::Instance()->RegisterCreator(Creator);
}

