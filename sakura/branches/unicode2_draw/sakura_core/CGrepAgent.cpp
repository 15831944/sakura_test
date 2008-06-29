#include "stdafx.h"
#include "CGrepAgent.h"
#include "util/window.h"
#include "debug/CRunningTimer.h"
#include "dlg/CDlgCancel.h"
#include "COpeBlk.h"
#include "util/module.h"
#include "io/CFileLoad.h"
#include "charset/CCodeMediator.h"
#include "parse/CWordParse.h"

CGrepAgent::CGrepAgent()
: m_bGrepMode( false )			/* Grep���[�h�� */
, m_bGrepRunning( false )		/* Grep������ */
{
}

ECallbackResult CGrepAgent::OnBeforeClose()
{
	//GREP�������͏I���ł��Ȃ�
	if( m_bGrepRunning ){
		// �A�N�e�B�u�ɂ���
		ActivateFrameWindow( CEditWnd::Instance()->GetHwnd() );	//@@@ 2003.06.25 MIK
		TopInfoMessage(
			CEditWnd::Instance()->GetHwnd(),
			_T("Grep�̏������ł��B\n")
		);
		return CALLBACK_INTERRUPT;
	}
	return CALLBACK_CONTINUE;
}



// Grep���s
DWORD CGrepAgent::DoGrep(
	CEditView*				pcViewDst,
	const CNativeW*			pcmGrepKey,
	const CNativeT*			pcmGrepFile,
	const CNativeT*			pcmGrepFolder,
	BOOL					bGrepSubFolder,
	const SSearchOption&	sSearchOption,
	ECodeType				nGrepCharSet,	// 2002/09/21 Moca �����R�[�h�Z�b�g�I��
	BOOL					bGrepOutputLine,
	int						nGrepOutputStyle
)
{
#ifdef _DEBUG
	CRunningTimer cRunningTimer( "CEditView::DoGrep" );
#endif

	this->m_bGrepRunning = true;


	int			nDummy;
	int			nHitCount = 0;
	wchar_t		szKey[_MAX_PATH];
	TCHAR		szFile[_MAX_PATH];
	CDlgCancel	cDlgCancel;
	HWND		hwndCancel;
	int			nCharChars;
	//	Jun. 27, 2001 genta	���K�\�����C�u�����̍����ւ�
	CBregexp	cRegexp;
	CNativeW	cmemMessage;
	int			nWork;
	int*		pnKey_CharCharsArr;
	pnKey_CharCharsArr = NULL;

	/*
	|| �o�b�t�@�T�C�Y�̒���
	*/
	cmemMessage.AllocStringBuffer( 4000 );

	pcViewDst->m_bDoing_UndoRedo		= true;


	/* �A���h�D�o�b�t�@�̏��� */
	if( NULL != pcViewDst->m_pcOpeBlk ){	/* ����u���b�N */
//@@@2002.2.2 YAZAKI NULL����Ȃ��Ɛi�܂Ȃ��̂ŁA�Ƃ肠�����R�����g�B��NULL�̂Ƃ��́Anew COpeBlk����B
//		while( NULL != m_pcOpeBlk ){}
//		delete m_pcOpeBlk;
//		m_pcOpeBlk = NULL;
	}
	else {
		pcViewDst->m_pcOpeBlk = new COpeBlk;
	}

	pcViewDst->m_bCurSrchKeyMark = true;								/* ����������̃}�[�N */
	wcscpy( pcViewDst->m_szCurSrchKey, pcmGrepKey->GetStringPtr() );	/* ���������� */
	pcViewDst->m_sCurSearchOption.bRegularExp = sSearchOption.bRegularExp;		/* �����^�u��  1==���K�\�� */
	pcViewDst->m_sCurSearchOption.bLoHiCase   = sSearchOption.bLoHiCase;			/* �����^�u��  1==�p�啶���������̋�� */
	/* ���K�\�� */

	//	From Here Jun. 27 genta
	/*
		Grep���s���ɓ������Č����E��ʐF�����p���K�\���o�b�t�@��
		����������D�����Grep�������ʂ̐F�������s�����߁D

		Note: �����ŋ�������͍̂Ō�̌���������ł�����
		Grep�Ώۃp�^�[���ł͂Ȃ����Ƃɒ���
	*/
	if( pcViewDst->m_sCurSearchOption.bRegularExp ){
		//	Jun. 27, 2001 genta	���K�\�����C�u�����̍����ւ�
		if( !InitRegexp( pcViewDst->GetHwnd(), pcViewDst->m_CurRegexp, true ) ){
			return 0;
		}

		/* �����p�^�[���̃R���p�C�� */
		int nFlag = 0x00;
		nFlag |= pcViewDst->m_sCurSearchOption.bLoHiCase ? 0x01 : 0x00;
		pcViewDst->m_CurRegexp.Compile( pcViewDst->m_szCurSrchKey, nFlag );
	}
	//	To Here Jun. 27 genta

//�܂� m_bCurSrchWordOnly = GetDllShareData().m_Common.m_bWordOnly;	/* �����^�u��  1==�P��̂݌��� */

//	cDlgCancel.Create( G_AppInstance(), m_hwndParent );
//	hwndCancel = cDlgCancel.Open( MAKEINTRESOURCE(IDD_GREPRUNNING) );
	hwndCancel = cDlgCancel.DoModeless( G_AppInstance(), pcViewDst->m_hwndParent, IDD_GREPRUNNING );

	::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, 0, FALSE );
	::DlgItem_SetText( hwndCancel, IDC_STATIC_CURFILE, _T(" ") );	// 2002/09/09 Moca add
	::CheckDlgButton( hwndCancel, IDC_CHECK_REALTIMEVIEW, GetDllShareData().m_Common.m_sSearch.m_bGrepRealTimeView );	// 2003.06.23 Moca

	wcscpy( szKey, pcmGrepKey->GetStringPtr() );

	wcscpy( CAppMode::Instance()->m_szGrepKey, szKey );
	this->m_bGrepMode = true;

	//	2007.07.22 genta
	//	�o�[�W�����ԍ��擾�̂��߁C������O�̕��ֈړ�����
	if( sSearchOption.bRegularExp ){
		if( !InitRegexp( pcViewDst->GetHwnd(), cRegexp, true ) ){
			return 0;
		}
		/* �����p�^�[���̃R���p�C�� */
		int nFlag = 0x00;
		nFlag |= sSearchOption.bLoHiCase ? 0x01 : 0x00;
		if( !cRegexp.Compile( szKey, nFlag ) ){
			return 0;
		}
	}else{
		/* ���������̏�� */
		CSearchAgent::CreateCharCharsArr(
			szKey,
			wcslen( szKey ),
			&pnKey_CharCharsArr
		);
	}

//2002.02.08 Grep�A�C�R�����傫���A�C�R���Ə������A�C�R����ʁX�ɂ���B
	HICON	hIconBig, hIconSmall;
	//	Dec, 2, 2002 genta �A�C�R���ǂݍ��ݕ��@�ύX
	hIconBig   = GetAppIcon( G_AppInstance(), ICON_DEFAULT_GREP, FN_GREP_ICON, false );
	hIconSmall = GetAppIcon( G_AppInstance(), ICON_DEFAULT_GREP, FN_GREP_ICON, true );

	//	Sep. 10, 2002 genta
	//	CEditWnd�ɐV�݂����֐����g���悤��
	CEditWnd*	pCEditWnd = CEditWnd::Instance();	//	Sep. 10, 2002 genta
	pCEditWnd->SetWindowIcon( hIconSmall, ICON_SMALL );
	pCEditWnd->SetWindowIcon( hIconBig, ICON_BIG );

	TCHAR szPath[_MAX_PATH];
	_tcscpy( szPath, pcmGrepFolder->GetStringPtr() );
	nDummy = _tcslen( szPath );

	/* �t�H���_�̍Ōオ�u���p����'\\'�v�łȂ��ꍇ�́A�t������ */
	nCharChars = &szPath[nDummy] - CNativeT::GetCharPrev( szPath, nDummy, &szPath[nDummy] );
	if( 1 == nCharChars && szPath[nDummy - 1] == _T('\\') ){
	}else{
		_tcscat( szPath, _T("\\") );
	}
	_tcscpy( szFile, pcmGrepFile->GetStringPtr() );

	nWork = wcslen( szKey ); // 2003.06.10 Moca ���炩���ߒ������v�Z���Ă���

	/* �Ō�Ƀe�L�X�g��ǉ� */
	CNativeW	cmemWork;
	cmemMessage.AppendString( L"\r\n����������  " );
	if( 0 < nWork ){
		CNativeW cmemWork2;
		cmemWork2.SetString( szKey );
		if( pcViewDst->m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_nStringType == 0 ){	/* �������؂�L���G�X�P�[�v���@  0=[\"][\'] 1=[""][''] */
			cmemWork2.Replace( L"\\", L"\\\\" );
			cmemWork2.Replace( L"\'", L"\\\'" );
			cmemWork2.Replace( L"\"", L"\\\"" );
		}else{
			cmemWork2.Replace( L"\'", L"\'\'" );
			cmemWork2.Replace( L"\"", L"\"\"" );
		}
		cmemWork.AppendString( L"\"" );
		cmemWork.AppendNativeData( cmemWork2 );
		cmemWork.AppendString( L"\"\r\n" );
	}else{
		cmemWork.AppendString( L"�u�t�@�C�������v\r\n" );
	}
	cmemMessage += cmemWork;



	cmemMessage.AppendString( L"�����Ώ�   " );
	cmemWork.SetStringT( szFile );
	if( pcViewDst->m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_nStringType == 0 ){	/* �������؂�L���G�X�P�[�v���@  0=[\"][\'] 1=[""][''] */
	}else{
	}
	cmemMessage += cmemWork;




	cmemMessage.AppendString( L"\r\n" );
	cmemMessage.AppendString( L"�t�H���_   " );
	cmemWork.SetStringT( szPath );
	if( pcViewDst->m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_nStringType == 0 ){	/* �������؂�L���G�X�P�[�v���@  0=[\"][\'] 1=[""][''] */
	}else{
	}
	cmemMessage += cmemWork;
	cmemMessage.AppendString( L"\r\n" );

	const wchar_t*	pszWork;
	if( bGrepSubFolder ){
		pszWork = L"    (�T�u�t�H���_������)\r\n";
	}else{
		pszWork = L"    (�T�u�t�H���_���������Ȃ�)\r\n";
	}
	cmemMessage.AppendString( pszWork );

	if( 0 < nWork ){ // 2003.06.10 Moca �t�@�C�������̏ꍇ�͕\�����Ȃ� // 2004.09.26 �������C��
		if( sSearchOption.bWordOnly ){
		/* �P��P�ʂŒT�� */
			cmemMessage.AppendString( L"    (�P��P�ʂŒT��)\r\n" );
		}

		if( sSearchOption.bLoHiCase ){
			pszWork = L"    (�p�啶������������ʂ���)\r\n";
		}else{
			pszWork = L"    (�p�啶������������ʂ��Ȃ�)\r\n";
		}
		cmemMessage.AppendString( pszWork );

		if( sSearchOption.bRegularExp ){
			//	2007.07.22 genta : ���K�\�����C�u�����̃o�[�W�������o�͂���
			cmemMessage.AppendString( L"    (���K�\��:" );
			cmemMessage.AppendStringT( cRegexp.GetVersionT() );
			cmemMessage.AppendString( L")\r\n" );
		}
	}

	if( CODE_AUTODETECT == nGrepCharSet ){
		cmemMessage.AppendString( L"    (�����R�[�h�Z�b�g�̎�������)\r\n" );
	}else if(IsValidCodeType(nGrepCharSet)){
		cmemMessage.AppendString( L"    (�����R�[�h�Z�b�g�F" );
		cmemMessage.AppendStringT( CCodeTypeName(nGrepCharSet).Normal() );
		cmemMessage.AppendString( L")\r\n" );
	}

	if( 0 < nWork ){ // 2003.06.10 Moca �t�@�C�������̏ꍇ�͕\�����Ȃ� // 2004.09.26 �������C��
		if( bGrepOutputLine ){
		/* �Y���s */
			pszWork = L"    (��v�����s���o��)\r\n";
		}else{
			pszWork = L"    (��v�����ӏ��̂ݏo��)\r\n";
		}
		cmemMessage.AppendString( pszWork );
	}


	cmemMessage.AppendString( L"\r\n\r\n" );
	pszWork = cmemMessage.GetStringPtr( &nWork );
//@@@ 2002.01.03 YAZAKI Grep����̓J�[�\����Grep���O�̈ʒu�ɓ�����
	CLayoutInt tmp_PosY_Layout = pcViewDst->m_pcEditDoc->m_cLayoutMgr.GetLineCount();
	if( 0 < nWork ){
		pcViewDst->GetCommander().Command_ADDTAIL( pszWork, nWork );
	}
	//	2007.07.22 genta �o�[�W�������擾���邽�߂ɁC
	//	���K�\���̏���������ֈړ�


	/* �\������ON/OFF */
	// 2003.06.23 Moca ���ʐݒ�ŕύX�ł���悤��
//	SetDrawSwitch(false);
	pcViewDst->SetDrawSwitch(0 != GetDllShareData().m_Common.m_sSearch.m_bGrepRealTimeView);


	int nGrepTreeResult = DoGrepTree(
		pcViewDst,
		&cDlgCancel,
		hwndCancel,
		szKey,
		pnKey_CharCharsArr,
		szFile,
		szPath,
		bGrepSubFolder,
		sSearchOption,
		nGrepCharSet,
		bGrepOutputLine,
		nGrepOutputStyle,
		&cRegexp,
		0,
		&nHitCount
	);
	if( -1 == nGrepTreeResult ){
		const wchar_t* p = L"���f���܂����B\r\n";
		pcViewDst->GetCommander().Command_ADDTAIL( p, wcslen( p ) );
	}
	auto_sprintf( szPath, _T("%d ����������܂����B\r\n"), nHitCount );
	pcViewDst->GetCommander().Command_ADDTAIL( to_wchar(szPath), -1 );
//	GetCommander().Command_GOFILEEND( FALSE );
#ifdef _DEBUG
	auto_sprintf( szPath, _T("��������: %d�~���b\r\n"), cRunningTimer.Read() );
	pcViewDst->GetCommander().Command_ADDTAIL( to_wchar(szPath), -1 );
#endif

	pcViewDst->GetCaret().MoveCursor( CLayoutPoint(CLayoutInt(0), tmp_PosY_Layout), TRUE );	//	�J�[�\����Grep���O�̈ʒu�ɖ߂��B

	cDlgCancel.CloseDialog( 0 );

	/* �A�N�e�B�u�ɂ��� */
	ActivateFrameWindow( CEditWnd::Instance()->GetHwnd() );


	/* �A���h�D�o�b�t�@�̏��� */
	if( NULL != pcViewDst->m_pcOpeBlk ){
		if( 0 < pcViewDst->m_pcOpeBlk->GetNum() ){	/* ����̐���Ԃ� */
			/* ����̒ǉ� */
			pcViewDst->m_pcEditDoc->m_cDocEditor.m_cOpeBuf.AppendOpeBlk( pcViewDst->m_pcOpeBlk );
		}
		else{
			delete pcViewDst->m_pcOpeBlk;
		}
		pcViewDst->m_pcOpeBlk = NULL;
	}

	//	Apr. 13, 2001 genta
	//	Grep���s��̓t�@�C����ύX�����̏�Ԃɂ���D
	pcViewDst->m_pcEditDoc->m_cDocEditor.SetModified(false,false);

	this->m_bGrepRunning = false;
	pcViewDst->m_bDoing_UndoRedo = false;

	if( NULL != pnKey_CharCharsArr ){
		delete [] pnKey_CharCharsArr;
		pnKey_CharCharsArr = NULL;
	}
//	if( NULL != pnKey_CharUsedArr ){
//		delete [] pnKey_CharUsedArr;
//		pnKey_CharUsedArr = NULL;
//	}

	/* �\������ON/OFF */
	pcViewDst->SetDrawSwitch(true);

	/* �t�H�[�J�X�ړ����̍ĕ`�� */
	pcViewDst->RedrawAll();

	return nHitCount;
}



/*
 * SORTED_LIST_BSEARCH
 *   ���X�g�̒T����bsearch���g���܂��B
 *   �w�肵�Ȃ��ꍇ�́A���`�T���ɂȂ�܂��B
 * SORTED_LIST
 *   ���X�g��qsort���܂��B
 *
 * �����F
 *   ���`�T���ł�qsort���g���A�������r�̑召�֌W���t�]�����Ƃ���ŒT����
 *   �ł��؂�Ώ����͑�����������܂���B
 */
//#define SORTED_LIST
//#define SORTED_LIST_BSEARCH

#ifdef SORTED_LIST_BSEARCH
#define SORTED_LIST
#endif

#ifdef SORTED_LIST
typedef int (* COMP)(const void *, const void *);

/*!
	qsort�p��r�֐�
	����a,b�͕�����ւ̃|�C���^�̃|�C���^�ł��邱�Ƃɒ��ӁB
	
	@param a [in] ��r������ւ̃|�C���^�̃|�C���^(list)
	@param b [in] ��r������ւ̃|�C���^�̃|�C���^(list)
	@return ��r����
*/
int grep_compare_pp(const void* a, const void* b)
{
	return _tcscmp( *((const TCHAR**)a), *((const TCHAR**)b) );
}

/*!
	bsearch�p��r�֐�
	����b�͕�����ւ̃|�C���^�̃|�C���^�ł��邱�Ƃɒ��ӁB
	
	@param a [in] ��r������ւ̃|�C���^(key)
	@param b [in] ��r������ւ̃|�C���^�̃|�C���^(list)
	@return ��r����
*/
int grep_compare_sp(const void* a, const void* b)
{
	return _tcscmp( (const TCHAR*)a, *((const TCHAR**)b) );
}
#endif

/*! @brief Grep���s

	@date 2001.06.27 genta	���K�\�����C�u�����̍����ւ�
	@date 2003.06.23 Moca   �T�u�t�H���_���t�@�C���������̂��t�@�C�����T�u�t�H���_�̏��ɕύX
	@date 2003.06.23 Moca   �t�@�C��������""����菜���悤��
	@date 2003.03.27 �݂�   ���O�t�@�C���w��̓����Əd�������h�~�̒ǉ��D
		�啔�����ύX���ꂽ���߁C�ʂ̕ύX�_�L���͖����D
*/
int CGrepAgent::DoGrepTree(
	CEditView*				pcViewDst,
	CDlgCancel*				pcDlgCancel,		//!< [in] Cancel�_�C�A���O�ւ̃|�C���^
	HWND					hwndCancel,			//!< [in] Cancel�_�C�A���O�̃E�B���h�E�n���h��
	const wchar_t*			pszKey,				//!< [in] �����p�^�[��
	int*					pnKey_CharCharsArr,	//!< [in] ������z��(2byte/1byte)�D�P�������񌟍��Ŏg�p�D
	const TCHAR*			pszFile,			//!< [in] �����Ώۃt�@�C���p�^�[��(!�ŏ��O�w��)
	const TCHAR*			pszPath,			//!< [in] �����Ώۃp�X
	BOOL					bGrepSubFolder,		//!< [in] TRUE: �T�u�t�H���_���ċA�I�ɒT������ / FALSE: ���Ȃ�
	const SSearchOption&	sSearchOption,		//!< [in] �����I�v�V����
	ECodeType				nGrepCharSet,		//!< [in] �����R�[�h�Z�b�g (0:�����F��)�`
	BOOL					bGrepOutputLine,	//!< [in] TRUE: �q�b�g�s���o�� / FALSE: �q�b�g�������o��
	int						nGrepOutputStyle,	//!< [in] �o�͌`�� 1: Normal, 2: WZ��(�t�@�C���P��)
	CBregexp*				pRegexp,			//!< [in] ���K�\���R���p�C���f�[�^�B���ɃR���p�C������Ă���K�v������
	int						nNest,				//!< [in] �l�X�g���x��
	int*					pnHitCount			//!< [i/o] �q�b�g���̍��v
)
{
	::DlgItem_SetText( hwndCancel, IDC_STATIC_CURPATH, pszPath );

	const TCHAR EXCEPT_CHAR = _T('!');	//���O���ʎq
	const TCHAR* WILDCARD_DELIMITER = _T(" ;,");	//���X�g�̋�؂�
	const TCHAR* WILDCARD_ANY = _T("*.*");	//�T�u�t�H���_�T���p

	int		nWildCardLen;
	int		nPos;
	BOOL	result;
	int		i;
	WIN32_FIND_DATA w32fd;
	CNativeW		cmemMessage;
	int				nHitCountOld;
	const wchar_t*	pszWork;
	int				nWork = 0;
	nHitCountOld = -100;

	//����̑Ώ�
	HANDLE handle      = INVALID_HANDLE_VALUE;


	/*
	 * ���X�g�̏�����(������ւ̃|�C���^�����X�g�Ǘ�����)
	 */
	int checked_list_size = 256;	//�m�ۍς݃T�C�Y
	int checked_list_count = 0;	//�o�^��
	TCHAR** checked_list = (TCHAR**)malloc( sizeof( TCHAR* ) * checked_list_size );
	if( ! checked_list ) return FALSE;	//�������m�ێ��s


	/*
	 * ���O�t�@�C����o�^����B
	 */
	nPos = 0;
	TCHAR* pWildCard = _tcsdup( pszFile );	//���C���h�J�[�h���X�g��Ɨp
	if( ! pWildCard ) goto error_return;	//�������m�ێ��s
	nWildCardLen = _tcslen( pWildCard );
	TCHAR*	token;
	while( NULL != (token = my_strtok<TCHAR>( pWildCard, nWildCardLen, &nPos, WILDCARD_DELIMITER )) )	//�g�[�N�����ɌJ��Ԃ��B
	{
		//���O�t�@�C���w��łȂ����H
		if( EXCEPT_CHAR != token[0] ) continue;

		//�_�u���R�[�e�[�V�����������A��΃p�X�����쐬����B
		TCHAR* p;
		TCHAR* q;
		p = q = ++token;
		while( *p )
		{
			if( *p != _T('"') ) *q++ = *p;
			p++;
		}
		*q = _T('\0');
		TCHAR* currentPath = NULL;	//���ݒT�����̃p�X
		currentPath = new TCHAR[ _tcslen( pszPath ) + _tcslen( token ) + 1 ];
		if( ! currentPath ) goto error_return;	//�������m�ێ��s
		_tcscpy( currentPath, pszPath );
		_tcscat( currentPath, token );

		//�t�@�C���̗�����J�n����B
		handle = FindFirstFile( currentPath, &w32fd );
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
		while( result )
		{
			if( ! (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )	//�t�H���_�łȂ��ꍇ
			{
				//�`�F�b�N�ς݃��X�g�ɓo�^����B
				if( checked_list_count >= checked_list_size )
				{
					checked_list_size += 256;
					TCHAR** p = (TCHAR**)realloc( checked_list, sizeof( TCHAR* ) * checked_list_size );
					if( ! p ) goto error_return;	//�������m�ێ��s
					checked_list = p;
				}
				checked_list[ checked_list_count ] = _tcsdup( w32fd.cFileName );
				checked_list_count++;
			}

			//���̃t�@�C���𗅗񂷂�B
			result = FindNextFile( handle, &w32fd );
		}
		//�n���h�������B
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
		delete [] currentPath;
		currentPath = NULL;
	}
	free( pWildCard );
	pWildCard = NULL;


	/*
	 * �J�����g�t�H���_�̃t�@�C����T������B
	 */
	nPos = 0;
	pWildCard = _tcsdup( pszFile );
	if( ! pWildCard ) goto error_return;	//�������m�ێ��s
	nWildCardLen = _tcslen( pWildCard );
	while( NULL != (token = my_strtok<TCHAR>( pWildCard, nWildCardLen, &nPos, WILDCARD_DELIMITER )) )	//�g�[�N�����ɌJ��Ԃ��B
	{
		//���O�t�@�C���w�肩�H
		if( EXCEPT_CHAR == token[0] ) continue;

		//�_�u���R�[�e�[�V�����������A��΃p�X�����쐬����B
		TCHAR* p;
		TCHAR* q;
		p = q = token;
		while( *p )
		{
			if( *p != _T('"') ) *q++ = *p;
			p++;
		}
		*q = _T('\0');
		TCHAR* currentPath = NULL;	//���ݒT�����̃p�X
		currentPath = new TCHAR[ _tcslen( pszPath ) + _tcslen( token ) + 1 ];
		if( ! currentPath ) goto error_return;
		_tcscpy( currentPath, pszPath );
		_tcscat( currentPath, token );

		//�t�@�C���̗�����J�n����B
#ifdef SORTED_LIST
		//�\�[�g
		qsort( checked_list, checked_list_count, sizeof( WCHAR* ), (COMP)grep_compare_pp_w );
#endif
		int current_checked_list_count = checked_list_count;	//�O��܂ł̃��X�g�̐�
		handle = FindFirstFile( currentPath, &w32fd );
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
		while( result )
		{
			/* �������̃��[�U�[������\�ɂ��� */
			if( !::BlockingHook( pcDlgCancel->GetHwnd() ) ){
				goto cancel_return;
			}
			/* ���f�{�^�������`�F�b�N */
			if( pcDlgCancel->IsCanceled() ){
				goto cancel_return;
			}

			/* �\���ݒ���`�F�b�N */
			pcViewDst->SetDrawSwitch(0 != ::IsDlgButtonChecked( pcDlgCancel->GetHwnd(), IDC_CHECK_REALTIMEVIEW ));

			if( ! (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )	//�t�H���_�łȂ��ꍇ
			{
				/*
				 * ���X�g�ɂ��邩���ׂ�B
				 * ����T�����̃t�@�C�����m���d�����邱�Ƃ͂Ȃ��̂ŁA
				 * �O��܂ł̃��X�g(current_checked_list_count)���猟������΂悢�B
				 */
#ifdef SORTED_LIST_BSEARCH
				if( ! bsearch( w32fd.cFileName, checked_list, current_checked_list_count, sizeof( WCHAR* ), (COMP)grep_compare_sp ) )
#else
				bool found = false;
				TCHAR** ptr = checked_list;
				for( i = 0; i < current_checked_list_count; i++, ptr++ )
				{
#ifdef SORTED_LIST
					int n = wcscmp( *ptr, w32fd.cFileName );
					if( 0 == n )
					{
						found = true; 
						break;
					}
					else if( n > 0 )	//�T���ł��؂�
					{
						break;
					}
#else
					if( 0 == _tcscmp( *ptr, w32fd.cFileName ) )
					{
						found = true; 
						break;
					}
#endif
				}
				if( ! found )
#endif
				{
					//�`�F�b�N�ς݃��X�g�ɓo�^����B
					if( checked_list_count >= checked_list_size )
					{
						checked_list_size += 256;
						TCHAR** p = (TCHAR**)realloc( checked_list, sizeof( TCHAR* ) * checked_list_size );
						if( ! p ) goto error_return;	//�������m�ێ��s
						checked_list = p;
					}
					checked_list[ checked_list_count ] = _tcsdup( w32fd.cFileName );
					checked_list_count++;


					//GREP���s�I
					::DlgItem_SetText( hwndCancel, IDC_STATIC_CURFILE, w32fd.cFileName );

					TCHAR* currentFile = new TCHAR[ _tcslen( pszPath ) + _tcslen( w32fd.cFileName ) + 1 ];
					if( ! currentFile ) goto error_return;	//�������m�ێ��s
					_tcscpy( currentFile, pszPath );
					_tcscat( currentFile, w32fd.cFileName );
					/* �t�@�C�����̌��� */
					int nRet = DoGrepFile(
						pcViewDst,
						pcDlgCancel,
						hwndCancel,
						pszKey,
						pnKey_CharCharsArr,
						w32fd.cFileName,
						sSearchOption,
						nGrepCharSet,
						bGrepOutputLine,
						nGrepOutputStyle,
						pRegexp,
						pnHitCount,
						currentFile,
						cmemMessage
					);
					delete currentFile;
					currentFile = NULL;

					// 2003.06.23 Moca ���A���^�C���\���̂Ƃ��͑��߂ɕ\��
					if( pcViewDst->GetDrawSwitch() ){
						if( LTEXT('\0') != pszKey[0] ){
							// �f�[�^�����̂Ƃ��t�@�C���̍��v���ő�10MB�𒴂�����\��
							nWork += ( w32fd.nFileSizeLow + 1023 ) / 1024;
						}
						if( *pnHitCount - nHitCountOld && 
							( *pnHitCount < 20 || 10000 < nWork ) ){
							nHitCountOld = -100; // ���\��
						}
					}
					if( *pnHitCount - nHitCountOld  >= 10 ){
						/* ���ʏo�� */
						pszWork = cmemMessage.GetStringPtr( &nWork );
						if( 0 < nWork ){
							pcViewDst->GetCommander().Command_ADDTAIL( pszWork, nWork );
							pcViewDst->GetCommander().Command_GOFILEEND( FALSE );
							/* ���ʊi�[�G���A���N���A */
							cmemMessage.SetString( L"" );
							nWork = 0;
						}
						nHitCountOld = *pnHitCount;
					}
					if( -1 == nRet ){
						goto cancel_return;
					}
				}
			}

			//���̃t�@�C���𗅗񂷂�B
			result = FindNextFile( handle, &w32fd );
		}
		//�n���h�������B
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
		delete [] currentPath;
		currentPath = NULL;
	}
	free( pWildCard );
	pWildCard = NULL;

	for( i = 0; i < checked_list_count; i++ )
	{
		free( checked_list[ i ] );
	}
	free( checked_list );
	checked_list = NULL;
	checked_list_count = 0;
	checked_list_size = 0;


	/*
	 * �T�u�t�H���_����������B
	 */
	if( bGrepSubFolder ){
		TCHAR* subPath     = NULL;
		subPath = new TCHAR[ _tcslen( pszPath ) + _tcslen( WILDCARD_ANY ) + 1 ];
		if( ! subPath ) goto error_return;	//�������m�ێ��s
		_tcscpy( subPath, pszPath );
		_tcscat( subPath, WILDCARD_ANY );
		handle = FindFirstFile( subPath, &w32fd );
		result = (INVALID_HANDLE_VALUE != handle) ? TRUE : FALSE;
		while( result )
		{
			//�T�u�t�H���_�̒T�����ċA�Ăяo���B
			/* �������̃��[�U�[������\�ɂ��� */
			if( !::BlockingHook( pcDlgCancel->GetHwnd() ) ){
				goto cancel_return;
			}
			/* ���f�{�^�������`�F�b�N */
			if( pcDlgCancel->IsCanceled() ){
				goto cancel_return;
			}
			/* �\���ݒ���`�F�b�N */
			pcViewDst->SetDrawSwitch(0 != ::IsDlgButtonChecked( pcDlgCancel->GetHwnd(), IDC_CHECK_REALTIMEVIEW ));

			if( (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//�t�H���_�̏ꍇ
			 && 0 != _tcscmp( w32fd.cFileName, _T("."))
			 && 0 != _tcscmp( w32fd.cFileName, _T("..")) )
			{
				//�t�H���_�����쐬����B
				TCHAR* currentPath = NULL;	//���ݒT�����̃p�X
				currentPath = new TCHAR[ _tcslen( pszPath ) + _tcslen( w32fd.cFileName ) + 2 ];
				if( ! currentPath ) goto error_return;	//�������m�ێ��s
				_tcscpy( currentPath, pszPath );
				_tcscat( currentPath, w32fd.cFileName );
				_tcscat( currentPath, _T("\\") );

				int nGrepTreeResult = DoGrepTree(
					pcViewDst,
					pcDlgCancel,
					hwndCancel,
					pszKey,
					pnKey_CharCharsArr,
					pszFile,
					currentPath,
					bGrepSubFolder,
					sSearchOption,
					nGrepCharSet,
					bGrepOutputLine,
					nGrepOutputStyle,
					pRegexp,
					nNest + 1,
					pnHitCount
				);
				if( -1 == nGrepTreeResult ){
					goto cancel_return;
				}
				::DlgItem_SetText( hwndCancel, IDC_STATIC_CURPATH, pszPath );	//@@@ 2002.01.10 add �T�u�t�H���_����߂��Ă�����...

				delete [] currentPath;
				currentPath = NULL;
			}

			//���̃t�@�C���𗅗񂷂�B
			result = FindNextFile( handle, &w32fd );
		}
		//�n���h�������B
		if( INVALID_HANDLE_VALUE != handle )
		{
			FindClose( handle );
			handle = INVALID_HANDLE_VALUE;
		}
		delete [] subPath;
		subPath = NULL;
	}

	::DlgItem_SetText( hwndCancel, IDC_STATIC_CURFILE, LTEXT(" ") );	// 2002/09/09 Moca add
	/* ���ʏo�� */
	pszWork = cmemMessage.GetStringPtr( &nWork );
	if( 0 < nWork ){
		pcViewDst->GetCommander().Command_ADDTAIL( pszWork, nWork );
		pcViewDst->GetCommander().Command_GOFILEEND( FALSE );
		/* ���ʊi�[�G���A���N���A */
		cmemMessage.SetString( LTEXT("") );
	}

	return 0;


cancel_return:;
error_return:;
	/*
	 * �G���[���͂��ׂĂ̊m�ۍς݃��\�[�X���������B
	 */
	if( INVALID_HANDLE_VALUE != handle ) FindClose( handle );

	if( pWildCard ) free( pWildCard );
//	if( currentPath ) delete [] currentPath;
//	if( subPath ) delete [] subPath;

	if( checked_list )
	{
		for( i = 0; i < checked_list_count; i++ )
		{
			free( checked_list[ i ] );
		}
		free( checked_list );
	}

	/* ���ʏo�� */
	pszWork = cmemMessage.GetStringPtr( &nWork );
	if( 0 < nWork )
	{
		pcViewDst->GetCommander().Command_ADDTAIL( pszWork, nWork );
		pcViewDst->GetCommander().Command_GOFILEEND( FALSE );
		/* ���ʊi�[�G���A���N���A */
		cmemMessage.SetString( LTEXT("") );
	}

	return -1;
}




/*!	@brief Grep���ʂ��\�z����

	@param pWork [out] Grep�o�͕�����D�[���ȃ������̈��\�ߊm�ۂ��Ă������ƁD
		�Œ��� �{��2000 byte�{�t�@�C���� _MAX_PATH byte�{�s�E���ʒu�\���̒������K�v�D
		�t�@�C���P�ʏo�͂̏ꍇ�͖{��2500 byte + _MAX_PATH + �s�E���ʒu�\���̒������K�v�D
		

	pWork�͏[���ȃ������̈�������Ă���R�g
	@date 2002/08/29 Moca �o�C�i���[�f�[�^�ɑΉ� pnWorkLen �ǉ�
*/
void CGrepAgent::SetGrepResult(
	/* �f�[�^�i�[�� */
	wchar_t*	pWork,
	int*		pnWorkLen,			/*!< [out] Grep�o�͕�����̒��� */
	/* �}�b�`�����t�@�C���̏�� */
	const TCHAR*		pszFullPath,	/*!< [in] �t���p�X */
	const TCHAR*		pszCodeName,	/*!< [in] �����R�[�h���D" [SJIS]"�Ƃ� */
	/* �}�b�`�����s�̏�� */
	int			nLine,				/*!< [in] �}�b�`�����s�ԍ�(1�`) */
	int			nColm,				/*!< [in] �}�b�`�������ԍ�(1�`) */
	const wchar_t*	pCompareData,	/*!< [in] �s�̕����� */
	int			nLineLen,			/*!< [in] �s�̕�����̒��� */
	int			nEolCodeLen,		/*!< [in] EOL�̒��� */
	/* �}�b�`����������̏�� */
	const wchar_t*	pMatchData,		/*!< [in] �}�b�`���������� */
	int			nMatchLen,			/*!< [in] �}�b�`����������̒��� */
	/* �I�v�V���� */
	BOOL		bGrepOutputLine,	/*!< [in] 0: �Y�������̂�, !0: �Y���s */
	int			nGrepOutputStyle	/*!< [in] 1: Normal, 2: WZ��(�t�@�C���P��) */
)
{

	int nWorkLen = 0;
	const wchar_t * pDispData;
	int k;
	bool bEOL = true;
	int nMaxOutStr;

	/* �m�[�}�� */
	if( 1 == nGrepOutputStyle ){
		nWorkLen = ::auto_sprintf( pWork, L"%ls(%d,%d)%ls: ", pszFullPath, nLine, nColm, pszCodeName );
		nMaxOutStr = 2000; // 2003.06.10 Moca �ő咷�ύX
	}
	/* WZ�� */
	else if( 2 == nGrepOutputStyle ){
		nWorkLen = ::auto_sprintf( pWork, L"�E(%6d,%-5d): ", nLine, nColm );
		nMaxOutStr = 2500; // 2003.06.10 Moca �ő咷�ύX
	}

	/* �Y���s */
	if( bGrepOutputLine ){
		pDispData = pCompareData;
		k = nLineLen - nEolCodeLen;
		if( nMaxOutStr < k ){
			k = nMaxOutStr; // 2003.06.10 Moca �ő咷�ύX
		}
	}
	/* �Y������ */
	else{
		pDispData = pMatchData;
		k = nMatchLen;
		if( nMaxOutStr < k ){
			k = nMaxOutStr; // 2003.06.10 Moca �ő咷�ύX
		}
		// �Y�������ɉ��s���܂ޏꍇ�͂��̉��s�R�[�h�����̂܂ܗ��p����(���̍s�ɋ�s�����Ȃ�)
		// 2003.06.10 Moca k==0�̂Ƃ��Ƀo�b�t�@�A���_�[�������Ȃ��悤��
		if( 0 < k && (pMatchData[ k - 1 ] == L'\r' || pMatchData[ k - 1 ] == L'\n') ){
			bEOL = false;
		}
	}

	auto_memcpy( &pWork[nWorkLen], pDispData, k );
	nWorkLen += k;
	if( bEOL ){
		auto_memcpy( &pWork[nWorkLen], L"\r\n", 2 );
		nWorkLen = nWorkLen + 2;
	}
	*pnWorkLen = nWorkLen;
}

/*!
	Grep���s (CFileLoad���g�����e�X�g��)

	@param pcDlgCancel		[in] Cancel�_�C�A���O�ւ̃|�C���^
	@param hwndCancel		[in] Cancel�_�C�A���O�̃E�B���h�E�n���h��
	@param pszKey			[in] �����p�^�[��
	@param pnKey_CharCharsArr	[in] ������z��(2byte/1byte)�D�P�������񌟍��Ŏg�p�D
	@param pszFile			[in] �����Ώۃt�@�C����(�\���p)
	@param bGrepLoHiCase	[in] TRUE: �啶���������̋�ʂ��� / FALSE: ����
	@param bGrepRegularExp	[in] TRUE: �����p�^�[���͐��K�\�� / FALSE: ������
	@param nGrepCharSet		[in] �����R�[�h�Z�b�g (0:�����F��)�`
	@param bGrepOutputLine	[in] TRUE: �q�b�g�s���o�� / FALSE: �q�b�g�������o��
	@param bWordOnly		[in] TRUE: �P��P�ʂň�v�𔻒f / FALSE: �����ɂ���v����
	@param nGrepOutputStyle	[in] �o�͌`�� 1: Normal, 2: WZ��(�t�@�C���P��)
	@param pRegexp			[in] ���K�\���R���p�C���f�[�^�B���ɃR���p�C������Ă���K�v������
	@param pnHitCount		[i/o] �q�b�g���̍��v�D���X�̒l�Ɍ��������������Z���ĕԂ��D
	@param pszFullPath		[in] �����Ώۃt�@�C���p�X

	@retval -1 GREP�̃L�����Z��
	@retval ����ȊO �q�b�g��(�t�@�C���������̓t�@�C����)

	@date 2002/08/30 Moca CFileLoad���g�����e�X�g��
	@date 2004/03/28 genta �s�v�Ȉ���nNest, bGrepSubFolder, pszPath���폜
*/
int CGrepAgent::DoGrepFile(
	CEditView*				pcViewDst,
	CDlgCancel*				pcDlgCancel,
	HWND					hwndCancel,
	const wchar_t*			pszKey,
	int*					pnKey_CharCharsArr,
	const TCHAR*			pszFile,
	const SSearchOption&	sSearchOption,
	ECodeType				nGrepCharSet,
	BOOL					bGrepOutputLine,
	int						nGrepOutputStyle,
	CBregexp*				pRegexp,		//	Jun. 27, 2001 genta	���K�\�����C�u�����̍����ւ�
	int*					pnHitCount,
	const TCHAR*			pszFullPath,
	CNativeW&				cmemMessage
)
{
	int		nHitCount;
//	char	szLine[16000];
	wchar_t	szWork[3000]; // ������ SetGrepResult() ���Ԃ���������i�[�ł���T�C�Y���K�v
	wchar_t	szWork0[_MAX_PATH + 100];
	int		nLine;
	int		nWorkLen;
	const wchar_t*	pszRes; // 2002/08/29 const�t��
	ECodeType	nCharCode;
	const wchar_t*	pCompareData; // 2002/08/29 const�t��
	int		nColm;
	BOOL	bOutFileName;
	bOutFileName = FALSE;
	CEol	cEol;
	int		nEolCodeLen;
	CFileLoad	cfl;
	int		nOldPercent = 0;

	int	nKeyKen = wcslen( pszKey );

	//	�����ł͐��K�\���R���p�C���f�[�^�̏������͕s�v

	const TCHAR*	pszCodeName; // 2002/08/29 const�t��
	pszCodeName = _T("");
	nHitCount = 0;
	nLine = 0;

	/* ���������������[���̏ꍇ�̓t�@�C���������Ԃ� */
	// 2002/08/29 �s���[�v�̑O���炱���Ɉړ�
	if( 0 == nKeyKen ){
		if( CODE_AUTODETECT == nGrepCharSet ){
			// 2003.06.10 Moca �R�[�h���ʏ����������Ɉړ��D
			// ���ʃG���[�ł��t�@�C�����ɃJ�E���g���邽��
			// �t�@�C���̓��{��R�[�h�Z�b�g����
			nCharCode = CCodeMediator::CheckKanjiCodeOfFile( pszFullPath );
			if( -1 == nCharCode ){
				pszCodeName = _T("  [(DetectError)]");
			}else{
				pszCodeName = CCodeTypeName(nCharCode).Bracket();
			}
		}
		if( 1 == nGrepOutputStyle ){
		/* �m�[�}�� */
			auto_sprintf( szWork0, L"%ts%ts\r\n", pszFullPath, pszCodeName );
		}else{
		/* WZ�� */
			auto_sprintf( szWork0, L"��\"%ts\"%ts\r\n", pszFullPath, pszCodeName );
		}
		cmemMessage.AppendString( szWork0 );
		++(*pnHitCount);
		::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
		return 1;
	}


	try{
	// �t�@�C�����J��
	// FileClose�Ŗ����I�ɕ��邪�A���Ă��Ȃ��Ƃ��̓f�X�g���N�^�ŕ���
	// 2003.06.10 Moca �����R�[�h���菈����FileOpen�ōs��
	nCharCode = cfl.FileOpen( pszFullPath, nGrepCharSet, 0 );
	if( CODE_AUTODETECT == nGrepCharSet ){
		pszCodeName = CCodeTypeName(nCharCode).Bracket();
	}
	auto_sprintf( szWork0, L"��\"%ts\"%ts\r\n", pszFullPath, pszCodeName );
//	/* �������̃��[�U�[������\�ɂ��� */
	if( !::BlockingHook( pcDlgCancel->GetHwnd() ) ){
		return -1;
	}
	/* ���f�{�^�������`�F�b�N */
	if( pcDlgCancel->IsCanceled() ){
		return -1;
	}

	/* ���������������[���̏ꍇ�̓t�@�C���������Ԃ� */
	// 2002/08/29 �t�@�C���I�[�v���̎�O�ֈړ�

	// ���� : cfl.ReadLine �� throw ����\��������
	CNativeW cUnicodeBuffer;
	while( RESULT_FAILURE != cfl.ReadLine( &cUnicodeBuffer, &cEol ) )
	{
		const wchar_t*	pLine = cUnicodeBuffer.GetStringPtr();
		int		nLineLen = cUnicodeBuffer.GetStringLength();

		nEolCodeLen = cEol.GetLen();
		++nLine;
		pCompareData = pLine;

		/* �������̃��[�U�[������\�ɂ��� */
		if( !::BlockingHook( pcDlgCancel->GetHwnd() ) ){
			return -1;
		}
		if( 0 == nLine % 64 ){
			/* ���f�{�^�������`�F�b�N */
			if( pcDlgCancel->IsCanceled() ){
				return -1;
			}
			//	2003.06.23 Moca �\���ݒ���`�F�b�N
			pcViewDst->SetDrawSwitch(0 != ::IsDlgButtonChecked( pcDlgCancel->GetHwnd(), IDC_CHECK_REALTIMEVIEW ));
			// 2002/08/30 Moca �i�s��Ԃ�\������(5MB�ȏ�)
			if( 5000000 < cfl.GetFileSize() ){
				int nPercent = cfl.GetPercent();
				if( 5 <= nPercent - nOldPercent ){
					nOldPercent = nPercent;
					::auto_sprintf( szWork, L"%ls (%3d%%)", pszFile, nPercent );
					::DlgItem_SetText( hwndCancel, IDC_STATIC_CURFILE, szWork );
				}
			}
		}

		/* ���K�\������ */
		if( sSearchOption.bRegularExp ){
			int nColmPrev = 0;

			//	Jun. 21, 2003 genta ���[�v����������
			//	�}�b�`�ӏ���1�s���畡�����o����P�[�X��W���ɁC
			//	�}�b�`�ӏ���1�s����1�������o����ꍇ���O�P�[�X�ƂƂ炦�C
			//	���[�v�p���E�ł��؂����(bGrepOutputLine)���t�ɂ����D
			//	Jun. 27, 2001 genta	���K�\�����C�u�����̍����ւ�
			// From Here 2005.03.19 ����� ���͂�BREGEXP�\���̂ɒ��ڃA�N�Z�X���Ȃ�
			while( pRegexp->Match( pCompareData, nLineLen, 0 ) ){

					//	�p�^�[������
					nColm = pRegexp->GetIndex() + 1;
					int matchlen = pRegexp->GetMatchLen();

					/* Grep���ʂ��AszWork�Ɋi�[���� */
					SetGrepResult(
						szWork,
						&nWorkLen,
						pszFullPath,
						pszCodeName,
						nLine,
						nColm + nColmPrev,
						pCompareData,
						nLineLen,
						nEolCodeLen,
						pCompareData + nColm - 1,
						matchlen,
						bGrepOutputLine,
						nGrepOutputStyle
					);
					// To Here 2005.03.19 ����� ���͂�BREGEXP�\���̂ɒ��ڃA�N�Z�X���Ȃ�
					if( 2 == nGrepOutputStyle ){
					/* WZ�� */
						if( !bOutFileName ){
							cmemMessage.AppendString( szWork0 );
							bOutFileName = TRUE;
						}
					}
					cmemMessage.AppendString( szWork, nWorkLen );
					++nHitCount;
					++(*pnHitCount);
					if( 0 == ( (*pnHitCount) % 16 ) || *pnHitCount < 16 ){
						::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
					}
					//	Jun. 21, 2003 genta �s�P�ʂŏo�͂���ꍇ��1������Ώ\��
					if ( bGrepOutputLine ) {
						break;
					}
					//	�T���n�߂�ʒu��␳
					//	2003.06.10 Moca �}�b�`����������̌�납�玟�̌������J�n����
					//	nClom : �}�b�`�ʒu
					//	matchlen : �}�b�`����������̒���
					int nPosDiff = nColm + matchlen;
					if( matchlen != 0 ){
						nPosDiff--;
					}
					pCompareData += nPosDiff;
					nLineLen -= nPosDiff;
					nColmPrev += nPosDiff;
			}
		}
		/* �P��̂݌��� */
		else if( sSearchOption.bWordOnly ){
			/*
				2002/02/23 Norio Nakatani
				�P��P�ʂ�Grep�������I�Ɏ����B�P���WhereCurrentWord()�Ŕ��ʂ��Ă܂��̂ŁA
				�p�P���C/C++���ʎq�Ȃǂ̌��������Ȃ�q�b�g���܂��B

				2002/03/06 YAZAKI
				Grep�ɂ����������B
				WhereCurrentWord�ŒP��𒊏o���āA���̒P�ꂪ������Ƃ����Ă��邩��r����B
			*/
			CLogicInt nNextWordFrom = CLogicInt(0);
			CLogicInt nNextWordFrom2;
			CLogicInt nNextWordTo2;
			// Jun. 26, 2003 genta ���ʂ�while�͍폜
			while( CWordParse::WhereCurrentWord_2( pCompareData, CLogicInt(nLineLen), nNextWordFrom, &nNextWordFrom2, &nNextWordTo2 , NULL, NULL ) ){
				if( nKeyKen == nNextWordTo2 - nNextWordFrom2 ){
					// const char* pData = pCompareData;	// 2002/2/10 aroka CMemory�ύX , 2002/08/29 Moca pCompareData��const���ɂ��s�v?
					/* 1==�啶���������̋�� */
					if( (!sSearchOption.bLoHiCase && 0 == auto_memicmp( &(pCompareData[nNextWordFrom2]) , pszKey, nKeyKen ) ) ||
						(sSearchOption.bLoHiCase && 0 ==	 auto_memcmp( &(pCompareData[nNextWordFrom2]) , pszKey, nKeyKen ) )
					){
						/* Grep���ʂ��AszWork�Ɋi�[���� */
						SetGrepResult(
							szWork, &nWorkLen,
							pszFullPath, pszCodeName,
							//	Jun. 25, 2002 genta
							//	���ʒu��1�n�܂�Ȃ̂�1�𑫂��K�v������
							nLine, nNextWordFrom2 + 1, pCompareData, nLineLen, nEolCodeLen,
							&(pCompareData[nNextWordFrom2]), nKeyKen,
							bGrepOutputLine, nGrepOutputStyle
						);
						if( 2 == nGrepOutputStyle ){
						/* WZ�� */
							if( !bOutFileName ){
								cmemMessage.AppendString( szWork0 );
								bOutFileName = TRUE;
							}
						}

						cmemMessage.AppendString( szWork, nWorkLen );
						++nHitCount;
						++(*pnHitCount);
						//	May 22, 2000 genta
						// if( 0 == ( (*pnHitCount) % 16 ) ){
							::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
						// }
					}
				}
				/* ���݈ʒu�̍��E�̒P��̐擪�ʒu�𒲂ׂ� */
				if( !CWordParse::SearchNextWordPosition( pCompareData, CLogicInt(nLineLen), nNextWordFrom, &nNextWordFrom, FALSE ) ){
					break;	//	���̒P�ꂪ�����B
				}
			}
		}
		else {
			/* �����񌟍� */
			int nColmPrev = 0;
			//	Jun. 21, 2003 genta ���[�v����������
			//	�}�b�`�ӏ���1�s���畡�����o����P�[�X��W���ɁC
			//	�}�b�`�ӏ���1�s����1�������o����ꍇ���O�P�[�X�ƂƂ炦�C
			//	���[�v�p���E�ł��؂����(bGrepOutputLine)���t�ɂ����D
			while(1){
				pszRes = CSearchAgent::SearchString(
					pCompareData,
					nLineLen,
					0,
					pszKey,
					nKeyKen,
					pnKey_CharCharsArr,
					sSearchOption.bLoHiCase
				);
				if(!pszRes)break;

				nColm = pszRes - pCompareData + 1;

				/* Grep���ʂ��AszWork�Ɋi�[���� */
				SetGrepResult(
					szWork, &nWorkLen,
					pszFullPath, pszCodeName,
					nLine, nColm + nColmPrev, pCompareData, nLineLen, nEolCodeLen,
					pszRes, nKeyKen,
					bGrepOutputLine, nGrepOutputStyle
				);
				if( 2 == nGrepOutputStyle ){
				/* WZ�� */
					if( !bOutFileName ){
						cmemMessage.AppendString( szWork0 );
						bOutFileName = TRUE;
					}
				}

				cmemMessage.AppendString( szWork, nWorkLen );
				++nHitCount;
				++(*pnHitCount);
				//	May 22, 2000 genta
				if( 0 == ( (*pnHitCount) % 16 ) || *pnHitCount < 16 ){
					::SetDlgItemInt( hwndCancel, IDC_STATIC_HITCOUNT, *pnHitCount, FALSE );
				}
				
				//	Jun. 21, 2003 genta �s�P�ʂŏo�͂���ꍇ��1������Ώ\��
				if ( bGrepOutputLine ) {
					break;
				}
				//	�T���n�߂�ʒu��␳
				//	2003.06.10 Moca �}�b�`����������̌�납�玟�̌������J�n����
				//	nClom : �}�b�`�ʒu
				//	matchlen : �}�b�`����������̒���
				int nPosDiff = nColm += nKeyKen - 1;
				pCompareData += nPosDiff;
				nLineLen -= nPosDiff;
				nColmPrev += nPosDiff;
			}
		}
	}

	// �t�@�C���𖾎��I�ɕ��邪�A�����ŕ��Ȃ��Ƃ��̓f�X�g���N�^�ŕ��Ă���
	cfl.FileClose();
	} // try
	catch( CError_FileOpen ){
		auto_sprintf( szWork, L"file open error [%ts]\r\n", pszFullPath );
		pcViewDst->GetCommander().Command_ADDTAIL( szWork, wcslen( szWork ) );
		return 0;
	}
	catch( CError_FileRead ){
		auto_sprintf( szWork, L"CEditView::DoGrepFile() �t�@�C���̓ǂݍ��ݒ��ɃG���[���������܂����B\r\n");
		pcViewDst->GetCommander().Command_ADDTAIL( szWork, wcslen( szWork ) );
	} // ��O�����I���

	return nHitCount;
}

