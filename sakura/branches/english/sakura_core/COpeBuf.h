/*!	@file
	@brief �A���h�D�E���h�D�o�b�t�@

	@author Norio Nakatani
	@date 1998/06/09 �V�K�쐬
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani
	Copyright (C) 2002, aroka
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

class COpeBuf;

#ifndef _COPEBUF_H_
#define _COPEBUF_H_

/* �A���h�D�o�b�t�@�p ����R�[�h */
enum enumOPECODE {
	OPE_INSERT		= 1,
	OPE_DELETE		= 2,
	OPE_MOVECARET	= 3,
};


#include "global.h"
class COpeBlk;/// 2002/2/10 aroka




/*-----------------------------------------------------------------------
�N���X�̐錾
-----------------------------------------------------------------------*/
/*!
	@brief �A���h�D�E���h�D�o�b�t�@
*/
class SAKURA_CORE_API COpeBuf {
	public:
		COpeBuf();	/* COpeBuf�N���X�\�z */
		~COpeBuf();	/* COpeBuf�N���X���� */
		void ClearAll( void );	/* �S�v�f�̃N���A */
		int AppendOpeBlk( COpeBlk* );	/* ����u���b�N�̒ǉ� */
		int	IsEnableUndo( void );	/* Undo�\�ȏ�Ԃ� */
		int	IsEnableRedo( void );	/* Redo�\�ȏ�Ԃ� */
		COpeBlk* DoUndo( int* );	/* ���݂�Undo�Ώۂ̑���u���b�N��Ԃ� */
		COpeBlk* DoRedo( int* );	/* ���݂�Redo�Ώۂ̑���u���b�N��Ԃ� */
		void SetNoModified( void );	/* ���݈ʒu�Ŗ��ύX�ȏ�ԂɂȂ������Ƃ�ʒm */
		int GetCurrentPointer( void ) const { return m_nCurrentPointer; }	/* ���݈ʒu��Ԃ� */	// 2007.12.09 ryoji

		void DUMP( void );	/* �ҏW����v�f�u���b�N�̃_���v */
	private:
		int			 m_nCOpeBlkArrNum;	/* ����u���b�N�̐� */
		COpeBlk**	m_ppCOpeBlkArr;	/* ����u���b�N�̔z�� */
		int			m_nCurrentPointer;	/* ���݈ʒu */
		int			m_nNoModifiedIndex;	/* ���ύX�ȏ�ԂɂȂ����ʒu */
};



///////////////////////////////////////////////////////////////////////
#endif /* _COPEBUF_H_ */


/*[EOF]*/