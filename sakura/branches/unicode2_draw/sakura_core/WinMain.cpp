/*!	@file
	@brief Entry Point

	@author Norio Nakatani
	@date	1998/03/13 �쐬
	@date	2001/06/26 genta ���[�h�P�ʂ�Grep�̂��߂̃R�}���h���C�������ǉ�
	@date	2002/01/08 aroka �����̗���𐮗��A���g�p�R�[�h���폜
	@date	2002/01/18 aroka ����聕�����[�X
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2001, genta
	Copyright (C) 2002, aroka

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#include "stdafx.h"
#include <windows.h>
#include "CProcessFactory.h"
#include "CProcess.h"
#include "debug/CRunningTimer.h"

/*!
	Windows Entry point

	1�ڂ̃G�f�B�^�v���Z�X�̏ꍇ�́A���̃v���Z�X�̓R���g���[���v���Z�X��
	�Ȃ�A�V�����G�f�B�^�v���Z�X���N������B�����łȂ��Ƃ��̓G�f�B�^�v���Z�X
	�ƂȂ�B

	�R���g���[���v���Z�X��CControlProcess�N���X�̃C���X�^���X�����A
	�G�f�B�^�v���Z�X��CNormalProcess�N���X�̃C���X�^���X�����B
*/
int WINAPI _tWinMain(
	HINSTANCE	hInstance,		//!< handle to current instance
	HINSTANCE	hPrevInstance,	//!< handle to previous instance
	LPTSTR		lpCmdLine,		//!< pointer to command line
	int			nCmdShow		//!< show state of window
)
{
	MY_RUNNINGTIMER(cRunningTimer, "WinMain" );
	setlocale( LC_ALL, "Japanese" ); //2007.08.16 kobake �ǉ�

	//�J�����
	DEBUG_TRACE(_T("-- -- WinMain -- --\n"));
	DEBUG_TRACE(_T("sizeof(DLLSHAREDATA) = %d\n"),sizeof(DLLSHAREDATA));

	//�v���Z�X�̐����ƃ��b�Z�[�W���[�v
	CProcessFactory aFactory;
	CProcess *process = 0;
	try{
		process = aFactory.Create( hInstance, lpCmdLine );
		MY_TRACETIME( cRunningTimer, "ProcessObject Created" );
	}
	catch(...){
	}
	if( 0 != process ){
		process->Run();
		delete process;
	}
	return 0;
}

