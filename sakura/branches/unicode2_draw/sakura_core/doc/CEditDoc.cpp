/*!	@file
	@brief �����֘A���̊Ǘ�

	@author Norio Nakatani
	@date	1998/03/13 �쐬
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2000, genta
	Copyright (C) 2001, genta, YAZAKI, jepro, novice, asa-o, MIK,
	Copyright (C) 2002, YAZAKI, hor, genta, aroka, frozen, Moca, MIK
	Copyright (C) 2003, MIK, genta, ryoji, Moca, zenryaku, naoh, wmlhq
	Copyright (C) 2004, genta, novice, Moca, MIK, zenryaku
	Copyright (C) 2005, genta, naoh, FILE, Moca, ryoji, D.S.Koba, aroka
	Copyright (C) 2006, genta, ryoji, aroka
	Copyright (C) 2007, ryoji, maru

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/

#include "stdafx.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>	// Apr. 03, 2003 genta
#include <io.h>
#include "doc/CEditDoc.h"
#include "debug/Debug.h"
#include "func/Funccode.h"
#include "debug/CRunningTimer.h"
#include "charset/charcode.h"
#include <DLGS.H>
#include "env/CShareData.h"
#include "window/CEditWnd.h"
#include "sakura_rc.h"
#include "global.h"
#include "outline/CFuncInfoArr.h" /// 2002/2/3 aroka
#include "CMarkMgr.h"///
#include "doc/CDocLine.h" /// 2002/2/3 aroka
#include "CPrintPreview.h"
#include "dlg/CDlgFileUpdateQuery.h"
#include <assert.h> /// 2002/11/2 frozen
#include "CClipboard.h"
#include "doc/CLayout.h"	// 2007.08.22 ryoji �ǉ�
#include "mem/CMemoryIterator.h"	// 2007.08.22 ryoji �ǉ�
#include "charset/CCodeMediator.h"
#include "util/file.h"
#include "util/window.h"
#include "util/string_ex2.h"
#include "util/format.h"
#include "util/module.h"
#include "CEditApp.h"
#include "util/other_util.h"
#include "env/CSakuraEnvironment.h"
#include "CNormalProcess.h"
#include "CControlTray.h"
#include "docplus/CModifyManager.h"
#include "CCodeChecker.h"

#define IDT_ROLLMOUSE	1

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                        �����Ɣj��                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
/*!
	@note
		m_pcEditWnd �̓R���X�g���N�^���ł͎g�p���Ȃ����ƁD

	@date 2000.05.12 genta ���������@�ύX
	@date 2002.2.17 YAZAKI CShareData�̃C���X�^���X�́ACProcess�ɂЂƂ���̂݁B
	@date 2002.01.14 YAZAKI ����v���r���[��CPrintPreview�ɓƗ����������Ƃɂ��ύX
	@date 2004.06.21 novice �^�O�W�����v�@�\�ǉ�
*/
CEditDoc::CEditDoc(CEditApp* pcApp)
: m_pcEditWnd(pcApp->m_pcEditWnd)
, m_nCommandExecNum( 0 )			/* �R�}���h���s�� */
, m_cDocFile(this)
, m_cDocOutline(this)
, m_cDocType(this)
, m_cDocEditor(this)
, m_cDocFileOperation(this)
{
	MY_RUNNINGTIMER( cRunningTimer, "CEditDoc::CEditDoc" );

	// ���C�A�E�g�Ǘ����̏�����
	m_cLayoutMgr.Create( this, &m_cDocLineMgr );

	// ���C�A�E�g���̕ύX
	STypeConfig& ref = m_cDocType.GetDocumentAttribute();
	m_cLayoutMgr.SetLayoutInfo(
		TRUE,
		ref
	);

	//	�����ۑ��̐ݒ�	//	Aug, 21, 2000 genta
	m_cAutoSaveAgent.ReloadAutoSaveParam();

	//$$ CModifyManager �C���X�^���X�𐶐�
	CModifyManager::Instance();

	//$$ CCodeChecker �C���X�^���X�𐶐�
	CCodeChecker::Instance();
}


CEditDoc::~CEditDoc()
{
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                        �����Ɣj��                           //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

void CEditDoc::Clear()
{
	// �t�@�C���̔r�����b�N����
	m_cDocFileOperation.DoFileUnlock();

	// �����݋֎~�̃N���A
	m_cDocLocker.Clear();

	// �A���h�D�E���h�D�o�b�t�@�̃N���A
	m_cDocEditor.m_cOpeBuf.ClearAll();

	// �e�L�X�g�f�[�^�̃N���A
	m_cDocLineMgr.DeleteAllLine();

	// �t�@�C���p�X�ƃA�C�R���̃N���A
	SetFilePathAndIcon( _T("") );

	// �t�@�C���̃^�C���X�^���v�̃N���A
	m_cDocFile.m_sFileInfo.cFileTime.ClearFILETIME();

	// �u��{�v�̃^�C�v�ʐݒ��K�p
	m_cDocType.SetDocumentType( CDocTypeManager().GetDocumentTypeOfPath( m_cDocFile.GetFilePath() ), true );
	STypeConfig& ref = m_cDocType.GetDocumentAttribute();
	m_cLayoutMgr.SetLayoutInfo(
		TRUE,
		ref
	);
}

/* �����f�[�^�̃N���A */
void CEditDoc::InitDoc()
{
	CAppMode::Instance()->SetViewMode(false);	// �r���[���[�h $$ ����OnClearDoc��p�ӂ�����
	wcscpy( CAppMode::Instance()->m_szGrepKey, L"" );	//$$

	CEditApp::Instance()->m_pcGrepAgent->m_bGrepMode = false;	/* Grep���[�h */	//$$����
	m_cAutoReloadAgent.m_eWatchUpdate = WU_QUERY; // Dec. 4, 2002 genta �X�V�Ď����@ $$

	// 2005.06.24 Moca �o�O�C��
	//	�A�E�g�v�b�g�E�B���h�E�Łu����(����)�v���s���Ă��A�E�g�v�b�g�E�B���h�E�̂܂�
	if( CAppMode::Instance()->IsDebugMode() ){
		CAppMode::Instance()->SetDebugModeOFF();
	}

//	Sep. 10, 2002 genta
//	�A�C�R���ݒ�̓t�@�C�����ݒ�ƈ�̉��̂��߂�������͍폜

	Clear();

	/* �ύX�t���O */
	m_cDocEditor.SetModified(false,false);	//	Jan. 22, 2002 genta

	/* �����R�[�h��� */
	m_cDocFile.m_sFileInfo.eCharCode = CODE_DEFAULT;
	m_cDocFile.m_sFileInfo.bBomExist = false;	//	Jul. 26, 2003 ryoji

	//	May 12, 2000
	m_cDocEditor.m_cNewLineCode.SetType( EOL_CRLF );
	
	//	Oct. 2, 2005 genta �}�����[�h
	m_cDocEditor.SetInsMode( GetDllShareData().m_Common.m_sGeneral.m_bIsINSMode );
}

/* �S�r���[�̏������F�t�@�C���I�[�v��/�N���[�Y�����ɁA�r���[������������ */
void CEditDoc::InitAllView( void )
{
	int		i;

	m_nCommandExecNum = 0;	/* �R�}���h���s�� */
	/* �擪�փJ�[�\�����ړ� */
	for( i = 0; i < 4; ++i ){
		//	Apr. 1, 2001 genta
		// �ړ������̏���
		m_pcEditWnd->m_pcEditViewArr[i]->m_cHistory->Flush();

		/* ���݂̑I��͈͂��I����Ԃɖ߂� */
		m_pcEditWnd->m_pcEditViewArr[i]->GetSelectionInfo().DisableSelectArea( FALSE );

		m_pcEditWnd->m_pcEditViewArr[i]->OnChangeSetting();
		m_pcEditWnd->m_pcEditViewArr[i]->GetCaret().MoveCursor( CLayoutPoint(0, 0), TRUE );
		m_pcEditWnd->m_pcEditViewArr[i]->GetCaret().m_nCaretPosX_Prev = CLayoutInt(0);
	}

	return;
}



/////////////////////////////////////////////////////////////////////////////
//
//	CEditDoc::Create
//	BOOL Create(HINSTANCE hInstance, HWND hwndParent)
//
//	����
//	  �E�B���h�E�̍쐬��
//
//	@date Sep. 29, 2001 genta �}�N���N���X��n���悤��
//	@date 2002.01.03 YAZAKI m_tbMyButton�Ȃǂ�CShareData����CMenuDrawer�ֈړ��������Ƃɂ��C���B
/////////////////////////////////////////////////////////////////////////////
BOOL CEditDoc::Create(
	CImageListMgr* pcIcons
 )
{
	MY_RUNNINGTIMER( cRunningTimer, "CEditDoc::Create" );

	//	Oct. 2, 2001 genta
	m_cFuncLookup.Init( GetDllShareData().m_Common.m_sMacro.m_MacroTable, &GetDllShareData().m_Common );


	MY_TRACETIME( cRunningTimer, "End: PropSheet" );

	return TRUE;
}



// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           �ݒ�                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*!
	�t�@�C�����̐ݒ�
	
	�t�@�C������ݒ肷��Ɠ����ɁC�E�B���h�E�A�C�R����K�؂ɐݒ肷��D
	
	@param szFile [in] �t�@�C���̃p�X��
	
	@author genta
	@date 2002.09.09
*/
void CEditDoc::SetFilePathAndIcon(const TCHAR* szFile)
{
	TCHAR szWork[MAX_PATH];
	if( ::GetLongFileName( szFile, szWork ) ){
		szFile = szWork;
	}
	m_cDocFile.SetFilePath(szFile);
	m_cDocType.SetDocumentIcon();
}




// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           ����                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

//! �h�L�������g�̕����R�[�h���擾
ECodeType CEditDoc::GetDocumentEncoding() const
{
	return m_cDocFile.m_sFileInfo.eCharCode;
}

//! �h�L�������g�̕����R�[�h��ݒ�
void CEditDoc::SetDocumentEncoding(ECodeType eCharCode)
{
	if(!IsValidCodeType(eCharCode))return; //�����Ȕ͈͂��󂯕t���Ȃ�

	m_cDocFile.m_sFileInfo.eCharCode = eCharCode;
}

void CEditDoc::GetSaveInfo(SSaveInfo* pSaveInfo) const
{
	pSaveInfo->cFilePath = this->m_cDocFile.GetFilePath(),
	pSaveInfo->eCharCode = this->m_cDocFile.m_sFileInfo.eCharCode;
	pSaveInfo->cEol      = this->m_cDocEditor.m_cNewLineCode; //�ҏW�����s�R�[�h��ۑ������s�R�[�h�Ƃ��Đݒ�
	pSaveInfo->bBomExist = this->m_cDocFile.m_sFileInfo.bBomExist;
}


/* �ҏW�t�@�C�������i�[ */
void CEditDoc::GetEditInfo(
	EditInfo* pfi	//!< [out]
) const
{
	//�t�@�C���p�X
	_tcscpy(pfi->m_szPath, m_cDocFile.GetFilePath());

	//�\����
	pfi->m_nViewTopLine = m_pcEditWnd->GetActiveView().GetTextArea().GetViewTopLine();	/* �\����̈�ԏ�̍s(0�J�n) */
	pfi->m_nViewLeftCol = m_pcEditWnd->GetActiveView().GetTextArea().GetViewLeftCol();	/* �\����̈�ԍ��̌�(0�J�n) */

	//�L�����b�g�ʒu
	pfi->m_ptCursor.Set(m_pcEditWnd->GetActiveView().GetCaret().GetCaretLogicPos());

	//�e����
	pfi->m_bIsModified = m_cDocEditor.IsModified();			/* �ύX�t���O */
	pfi->m_nCharCode = m_cDocFile.m_sFileInfo.eCharCode;	/* �����R�[�h��� */

	//GREP���[�h
	pfi->m_bIsGrep = CEditApp::Instance()->m_pcGrepAgent->m_bGrepMode;
	wcscpy( pfi->m_szGrepKey, CAppMode::Instance()->m_szGrepKey );

	//�f�o�b�O���j�^ (�A�E�g�v�b�g�E�C���h�E) ���[�h
	pfi->m_bIsDebug = CAppMode::Instance()->IsDebugMode();
}


//	From Here Aug. 14, 2000 genta
//
//	�����������֎~����Ă��邩�ǂ���
//	�߂�l: true: �֎~ / false: ����
//
bool CEditDoc::IsModificationForbidden( EFunctionCode nCommand )
{
	//	�r���[���[�h�ł��㏑���֎~�ł��Ȃ����
	if( !CAppMode::Instance()->IsViewMode() && m_cDocLocker.IsDocWritable() )
		return false; // ��ɏ�����������

	//	�㏑���֎~���[�h�̏ꍇ
	//	�b��Case��: ���ۂɂ͂����ƌ����̗ǂ����@���g���ׂ�
	switch( nCommand ){
	//	�t�@�C��������������R�}���h�͎g�p�֎~
	case F_WCHAR:
	case F_IME_CHAR:
	case F_DELETE:
	case F_DELETE_BACK:
	case F_WordDeleteToEnd:
	case F_WordDeleteToStart:
	case F_WordDelete:
	case F_WordCut:
	case F_LineDeleteToStart:
	case F_LineDeleteToEnd:
	case F_LineCutToStart:
	case F_LineCutToEnd:
	case F_DELETE_LINE:
	case F_CUT_LINE:
	case F_DUPLICATELINE:
	case F_INDENT_TAB:
	case F_UNINDENT_TAB:
	case F_INDENT_SPACE:
	case F_UNINDENT_SPACE:
	case F_CUT:
	case F_PASTE:
	case F_INS_DATE:
	case F_INS_TIME:
	case F_CTRL_CODE_DIALOG:	//@@@ 2002.06.02 MIK
	case F_INSTEXT_W:
	case F_ADDTAIL_W:
	case F_PASTEBOX:
	case F_REPLACE_DIALOG:
	case F_REPLACE:
	case F_REPLACE_ALL:
	case F_CODECNV_EMAIL:
	case F_CODECNV_EUC2SJIS:
	case F_CODECNV_UNICODE2SJIS:
	case F_CODECNV_UNICODEBE2SJIS:
	case F_CODECNV_SJIS2JIS:
	case F_CODECNV_SJIS2EUC:
	case F_CODECNV_UTF82SJIS:
	case F_CODECNV_UTF72SJIS:
	case F_CODECNV_SJIS2UTF7:
	case F_CODECNV_SJIS2UTF8:
	case F_CODECNV_AUTO2SJIS:
	case F_TOLOWER:
	case F_TOUPPER:
	case F_TOHANKAKU:
	case F_TOHANKATA:				// 2002/08/29 ai
	case F_TOZENEI:					// 2001/07/30 Misaka
	case F_TOHANEI:
	case F_TOZENKAKUKATA:
	case F_TOZENKAKUHIRA:
	case F_HANKATATOZENKATA:
	case F_HANKATATOZENHIRA:
	case F_TABTOSPACE:
	case F_SPACETOTAB:  //---- Stonee, 2001/05/27
	case F_HOKAN:
	case F_CHGMOD_INS:
	case F_LTRIM:		// 2001.12.03 hor
	case F_RTRIM:		// 2001.12.03 hor
	case F_SORT_ASC:	// 2001.12.11 hor
	case F_SORT_DESC:	// 2001.12.11 hor
	case F_MERGE:		// 2001.12.11 hor
	case F_UNDO:		// 2007.10.12 genta
	case F_REDO:		// 2007.10.12 genta
		return true;
	}
	return false;	//	�f�t�H���g�ŏ�����������
}
//	To Here Aug. 14, 2000 genta


// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           ���                              //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*! @brief ���̃E�B���h�E�ŐV�����t�@�C�����J���邩

	�V�����E�B���h�E���J�����Ɍ��݂̃E�B���h�E���ė��p�ł��邩�ǂ����̃e�X�g���s���D
	�ύX�ς݁C�t�@�C�����J���Ă���CGrep�E�B���h�E�C�A�E�g�v�b�g�E�B���h�E�̏ꍇ�ɂ�
	�ė��p�s�D

	@author Moca
	@date 2005.06.24 Moca
*/
bool CEditDoc::IsAcceptLoad() const
{
	if(m_cDocEditor.IsModified())return false;
	if(m_cDocFile.GetFilePathClass().IsValidPath())return false;
	if(CEditApp::Instance()->m_pcGrepAgent->m_bGrepMode)return false;
	if(CAppMode::Instance()->IsDebugMode())return false;
	return true;
}





// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                         �C�x���g                            //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/*! �R�}���h�R�[�h�ɂ�鏈���U�蕪��

	@param[in] nCommand MAKELONG( �R�}���h�R�[�h�C���M�����ʎq )

	@date 2006.05.19 genta ���16bit�ɑ��M���̎��ʎq������悤�ɕύX
	@date 2007.06.20 ryoji �O���[�v���ŏ��񂷂�悤�ɕύX
*/
BOOL CEditDoc::HandleCommand( EFunctionCode nCommand )
{
	//	May. 19, 2006 genta ���16bit�ɑ��M���̎��ʎq������悤�ɕύX�����̂�
	//	����16�r�b�g�݂̂����o��
	switch( LOWORD( nCommand )){
	case F_PREVWINDOW:	//�O�̃E�B���h�E
		{
			int nPane = m_pcEditWnd->m_cSplitterWnd.GetPrevPane();
			if( -1 != nPane ){
				m_pcEditWnd->SetActivePane( nPane );
			}else{
				CControlTray::ActiveNextWindow();
			}
		}
		return TRUE;
	case F_NEXTWINDOW:	//���̃E�B���h�E
		{
			int nPane = m_pcEditWnd->m_cSplitterWnd.GetNextPane();
			if( -1 != nPane ){
				m_pcEditWnd->SetActivePane( nPane );
			}
			else{
				CControlTray::ActivePrevWindow();
			}
		}
		return TRUE;
	default:
		return m_pcEditWnd->GetActiveView().GetCommander().HandleCommand( nCommand, TRUE, 0, 0, 0, 0 );
	}
}

/*! �r���[�ɐݒ�ύX�𔽉f������

	@date 2004.06.09 Moca ���C�A�E�g�č\�z����Progress Bar��\������D
*/
void CEditDoc::OnChangeSetting()
{
	int			i;
	HWND		hwndProgress = NULL;

	CEditWnd*	pCEditWnd = m_pcEditWnd;	//	Sep. 10, 2002 genta

	//pCEditWnd->m_CFuncKeyWnd.Timer_ONOFF( FALSE ); // 20060126 aroka

	if( NULL != pCEditWnd ){
		hwndProgress = pCEditWnd->m_cStatusBar.GetProgressHwnd();
		//	Status Bar���\������Ă��Ȃ��Ƃ���m_hwndProgressBar == NULL
	}

	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_SHOW );
	}

	/* �t�@�C���̔r�����[�h�ύX */
	if( m_cDocFile.GetShareMode() != GetDllShareData().m_Common.m_sFile.m_nFileShareMode ){
		/* �t�@�C���̔r�����b�N���� */
		m_cDocFileOperation.DoFileUnlock();
		/* �t�@�C���̔r�����b�N */
		m_cDocFileOperation.DoFileLock();
	}

	/* ���L�f�[�^�\���̂̃A�h���X��Ԃ� */
	CFileNameManager::Instance()->TransformFileName_MakeCache();

	// �������
	m_cDocType.SetDocumentType( CDocTypeManager().GetDocumentTypeOfPath( m_cDocFile.GetFilePath() ), false );

	CLogicPoint* posSaveAry = m_pcEditWnd->SavePhysPosOfAllView();

	/* ���C�A�E�g���̍쐬 */
	const STypeConfig& ref = m_cDocType.GetDocumentAttribute();
	CProgressSubject* pOld = CEditApp::Instance()->m_pcVisualProgress->CProgressListener::Listen(&m_cLayoutMgr);
	m_cLayoutMgr.SetLayoutInfo(true,ref);
	CEditApp::Instance()->m_pcVisualProgress->CProgressListener::Listen(pOld);

	/* �r���[�ɐݒ�ύX�𔽉f������ */
	for( i = 0; i < 4; ++i ){
		m_pcEditWnd->m_pcEditViewArr[i]->OnChangeSetting();
	}
	m_pcEditWnd->RestorePhysPosOfAllView( posSaveAry );
	if( hwndProgress ){
		::ShowWindow( hwndProgress, SW_HIDE );
	}
}

/*! �t�@�C�������Ƃ���MRU�o�^ & �ۑ��m�F �� �ۑ����s

	@retval TRUE: �I�����ėǂ� / FALSE: �I�����Ȃ�
*/
BOOL CEditDoc::OnFileClose()
{
	int			nRet;
	int			nBool;

	//�N���[�Y���O����
	ECallbackResult eBeforeCloseResult = NotifyBeforeClose();
	if(eBeforeCloseResult==CALLBACK_INTERRUPT)return FALSE;


	// �f�o�b�O���j�^���[�h�̂Ƃ��͕ۑ��m�F���Ȃ�
	if(CAppMode::Instance()->IsDebugMode())return TRUE;

	//�e�L�X�g���ύX����Ă��Ȃ��ꍇ�͕ۑ��m�F���Ȃ�
	if(!m_cDocEditor.IsModified())return TRUE;

	//GREP���[�h�ŁA���A�uGREP���[�h�ŕۑ��m�F���邩�v��OFF��������A�ۑ��m�F���Ȃ�
	if( CEditApp::Instance()->m_pcGrepAgent->m_bGrepMode ){
		if( !GetDllShareData().m_Common.m_sSearch.m_bGrepExitConfirm ){
			return TRUE;
		}
	}

	// -- -- �ۑ��m�F -- -- //
	
	/* �E�B���h�E���A�N�e�B�u�ɂ��� */
	HWND	hwndMainFrame = CEditWnd::Instance()->GetHwnd();
	ActivateFrameWindow( hwndMainFrame );
	if( CAppMode::Instance()->IsViewMode() ){	/* �r���[���[�h */
		::MessageBeep( MB_ICONQUESTION );
		nRet = ::MYMESSAGEBOX(
			hwndMainFrame,
			MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
			GSTR_APPNAME,
			_T("%ts\n�͕ύX����Ă��܂��B ����O�ɕۑ����܂����H\n\n�r���[���[�h�ŊJ���Ă���̂ŁA���O��t���ĕۑ�����΂����Ǝv���܂��B\n"),
			m_cDocFile.GetFilePathClass().IsValidPath() ? m_cDocFile.GetFilePath() : _T("�i����j")
		);
		switch( nRet ){
		case IDYES:
			nBool = m_cDocFileOperation.FileSaveAs();	// 2006.12.30 ryoji
			return nBool;
		case IDNO:
			return TRUE;
		case IDCANCEL:
		default:
			return FALSE;
		}
	}
	else{
		::MessageBeep( MB_ICONQUESTION );
		nRet = ::MYMESSAGEBOX(
			hwndMainFrame,
			MB_YESNOCANCEL | MB_ICONQUESTION | MB_TOPMOST,
			GSTR_APPNAME,
			_T("%ts\n�͕ύX����Ă��܂��B ����O�ɕۑ����܂����H"),
			m_cDocFile.GetFilePathClass().IsValidPath() ? m_cDocFile.GetFilePath() : _T("�i����j")
		);
		switch( nRet ){
		case IDYES:
			if( m_cDocFile.GetFilePathClass().IsValidPath() ){
				nBool = m_cDocFileOperation.FileSave();	// 2006.12.30 ryoji
			}
			else{
				nBool = m_cDocFileOperation.FileSaveAs();	// 2006.12.30 ryoji
			}
			return nBool;
		case IDNO:
			return TRUE;
		case IDCANCEL:
		default:
			return FALSE;
		}
	}
}
