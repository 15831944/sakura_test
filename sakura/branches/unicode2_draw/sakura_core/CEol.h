/*! @file
	@brief End of Line��ʂ̊Ǘ�

	@author genta
	@date 2000/5/15 �V�K�쐬
*/
/*
	Copyright (C) 2000-2001, genta
	Copyright (C) 2002, frozen, Moca

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

#ifndef _CEOL_H_
#define _CEOL_H_

#include "global.h"

// 2002/09/22 Moca EOL_CRLF_UNICODE��p�~
/* �s�I�[�q�̎�� */
SAKURA_CORE_API enum enumEOLType {
	EOL_NONE,			//!< 
	EOL_CRLF,			//!< 0d0a
	EOL_LF,				//!< 0a
	EOL_CR,				//!< 0d
	EOL_CODEMAX,		//
	EOL_UNKNOWN = -1	//
};
typedef enumEOLType EEolType;

#define EOL_TYPE_NUM	5

/* �s�I�[�q�̔z�� */
SAKURA_CORE_API extern const EEolType gm_pnEolTypeArr[EOL_TYPE_NUM];

#include "SakuraBasis.h"

/*!
	@brief �s���̉��s�R�[�h���Ǘ�

	�Ǘ��Ƃ͌����Ă��I�u�W�F�N�g�����邱�Ƃň��S�ɐݒ���s������֘A���̎擾��
	�I�u�W�F�N�g�ɑ΂��郁�\�b�h�ōs���邾�������A�O���[�o���ϐ��ւ̎Q�Ƃ�
	�N���X�����ɕ����߂邱�Ƃ��ł���̂ł���Ȃ�ɈӖ��͂���Ǝv���B
*/
class SAKURA_CORE_API CEol{
public:
	//�R���X�g���N�^�E�f�X�g���N�^
	CEol(){ m_eEolType = EOL_NONE;; }
	CEol( EEolType t ){ SetType(t); }

	//��r
	bool operator==( EEolType t ) const { return GetType() == t; }
	bool operator!=( EEolType t ) const { return GetType() != t; }

	//���
	const CEol& operator=( const CEol& t ){ m_eEolType = t.m_eEolType; return *this; }

	//�^�ϊ�
	operator EEolType() const { return GetType(); }

	//�ݒ�
	bool SetType( EEolType t);	//	Type�̐ݒ�
	void SetTypeByString( const wchar_t* pszData, int nDataLen );
	void SetTypeByString( const char* pszData, int nDataLen );

	//�擾
	EEolType		GetType()	const{ return m_eEolType; }		//!< ���݂�Type���擾
	CLogicInt		GetLen()	const;	//!< ���݂�EOL�����擾�B�����P�ʁB
	const TCHAR*	GetName()	const;	//!< ���݂�EOL�̖��̎擾
	const wchar_t*	GetValue2()	const;	//!< ���݂�EOL������擪�ւ̃|�C���^���擾
	//#####

	bool IsValid() const
	{
		return m_eEolType>=EOL_CRLF && m_eEolType<EOL_CODEMAX;
	}


private:
	EEolType	m_eEolType;	//!< ���s�R�[�h�̎��
};


#endif


