/*!	@file
	@brief ImageList�̎�舵��

	@author genta
	@date Oct. 11, 2000 genta
*/
/*
	Copyright (C) 2000-2001, genta
	Copyright (C) 2000, jepro
	Copyright (C) 2001, GAE, jepro
	Copyright (C) 2003, Moca, genta, wmlhq
	Copyright (C) 2007, ryoji

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "stdafx.h"
#include "CImageListMgr.h"
#include "sakura_rc.h"
#include "CRunningTimer.h"
#include "etc_uty.h"
#include "CShareData.h"

//  2010/06/29 syat MAX_X, MAX_Y�̒l��CommonSettings.h�Ɉړ�
//	Jul. 21, 2003 genta ���ł��g���̂Ŋ֐��̊O�ɏo����
//	Oct. 21, 2000 JEPRO �ݒ�
const int MAX_X = MAX_TOOLBAR_ICON_X;
const int MAX_Y = MAX_TOOLBAR_ICON_Y;	//2002.01.17

/*!	�̈���w��F�œh��Ԃ�

	@author Nakatani
*/
static void FillSolidRect( HDC hdc, int x, int y, int cx, int cy, COLORREF clr)
{
//	ASSERT_VALID(this);
//	ASSERT(m_hDC != NULL);

	RECT rect;
	::SetBkColor( hdc, clr );
	::SetRect( &rect, x, y, x + cx, y + cy );
	::ExtTextOut( hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL );
}

//	Destructor
CImageListMgr::~CImageListMgr()
{
	//	2003.07.21 Image List�̑���ɕ`��pbitmap�����
	if( m_hIconBitmap != NULL ){
		DeleteObject( m_hIconBitmap );
	}
}

/*
	@brief Image List�̍쐬
	
	���\�[�X�܂��̓t�@�C������bitmap��ǂݍ����
	�`��p�ɕێ�����D
	
	@param hInstance [in] bitmap���\�[�X�����C���X�^���X
	@param hWnd [in] ���g�p
	
	@date 2003.07.21 genta ImageList�̍\�z�͍s��Ȃ��D�����bitmap�����̂܂ܕێ�����D
*/
bool CImageListMgr::Create(HINSTANCE hInstance, HWND hWnd)
{
	MY_RUNNINGTIMER( cRunningTimer, "CImageListMgr::Create" );
	if( m_hIconBitmap != NULL ){	//	���ɍ\�z�ς݂Ȃ疳������
		return true;
	}

	HBITMAP	hRscbmp;	//	���\�[�X����ǂݍ��񂾂ЂƂ����܂��Bitmap
	HBITMAP	hFOldbmp;	//	SetObject�œ���ꂽ1�O�̃n���h����ێ�����
	HDC		dcFrom;		//	�`��p
	int		nRetPos;	//	�㏈���p
	m_cx = m_cy  = 16;

	nRetPos = 0;
	do {
		//	From Here 2001.7.1 GAE
		//	2001.7.1 GAE ���\�[�X�����[�J���t�@�C��(sakura�f�B���N�g��) my_icons.bmp ����ǂ߂�悤��
		// 2007.05.19 ryoji �ݒ�t�@�C���D��ɕύX
		TCHAR szPath[_MAX_PATH];
		GetInidirOrExedir( szPath, FN_TOOL_BMP );
		hRscbmp = (HBITMAP)::LoadImage( NULL, szPath, IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS );

		if( hRscbmp == NULL ) {	// ���[�J���t�@�C���̓ǂݍ��ݎ��s���̓��\�[�X����擾
			//	���̃u���b�N���͏]���̏���
			//	���\�[�X����Bitmap��ǂݍ���
			//	2003.09.29 wmlhq ���ɂ���ăA�C�R�����Ԃ��
			//hRscbmp = ::LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_MYTOOL ) );
			hRscbmp = (HBITMAP)::LoadImage( hInstance, MAKEINTRESOURCE( IDB_MYTOOL ), IMAGE_BITMAP, 0, 0,
				LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS  );
			if( hRscbmp == NULL ){
				//	Oct. 4, 2003 genta �G���[�R�[�h�ǉ�
				//	����I���Ɠ����R�[�h����dcFrom��s���ɉ�����Ă��܂�
				nRetPos = 2;
				break;
			}
		}
		//	To Here 2001.7.1 GAE
		m_hIconBitmap = hRscbmp;

		//	���ߐF�𓾂邽�߂�DC�Ƀ}�b�v����
		//	2003.07.21 genta ���ߐF�𓾂�ȊO�̖ړI�ł͎g��Ȃ��Ȃ���
		dcFrom = CreateCompatibleDC(0);	//	�]�����p
		if( dcFrom == NULL ){
			nRetPos = 1;
			break;
		}

		//	�܂�bitmap��dc��map����
		//	�������邱�Ƃ�CreateCompatibleBitmap��
		//	hRscbmp�Ɠ����`����bitmap������D
		//	�P��CreateCompatibleDC(0)�Ŏ擾����dc��
		//	�X�N���[����DC�ɑ΂���CreateCompatibleBitmap��
		//	�g���ƃ��m�N��Bitmap�ɂȂ�D
		hFOldbmp = (HBITMAP)SelectObject( dcFrom, hRscbmp );
		if( hFOldbmp == NULL ){
			nRetPos = 4;
			break;
		}

		m_cTrans = GetPixel( dcFrom, 0, 0 );//	�擾�����摜��(0,0)�̐F��w�i�F�Ƃ��Ďg��
		
		//	2003.07.21 genta
		//	ImageList�ւ̓o�^�����͓��R�΂�����폜
		
		//	���͂⏈���Ƃ͖��֌W�����C��w�̂��߂ɃR�����g�̂ݎc���Ă�����
		//---------------------------------------------------------
		//	Bitmap��MemoryDC��Assign����Ă���Ԃ�bitmap�n���h����
		//	�g���Ă�������bitmap���擾�ł��Ȃ��D
		//	�܂�CDC�ւ̕`�施�߂𔭍s���Ă����̏��Bitmap��
		//	���f�����킯�ł͂Ȃ��D
		//	Bitmap��DC������O���ď��߂ē��e�̕ۏ؂��ł���

		//	DC��map/unmap�����x�ɑ傫���e�����邽�߁C
		//	������Bitmap������Ĉꊇ�o�^����悤�ɕύX
		//	����ɂ����250msec���炢���x�����P�����D
		//---------------------------------------------------------

	}while(0);	//	1�񂵂��ʂ�Ȃ�

	//	�㏈��
	switch( nRetPos ){
	case 0:
		//	Oct. 4, 2003 genta hRscBmp��dcFrom����؂藣���Ă����K�v������
		//	�A�C�R���`��ύX���ɉ߂��č폜����Ă���
		SelectObject( dcFrom, hFOldbmp );
	case 4:
		DeleteDC( dcFrom );
	case 2:
	case 1:
		//	2003.07.21 genta hRscbmp�� m_hIconBitmap �Ƃ��ăI�u�W�F�N�g��
		//	���������ێ������̂ŉ�����Ă͂Ȃ�Ȃ�
		break;
	}

	return nRetPos == 0;
}


/*! �r�b�g�}�b�v�̕\�� �D�F�𓧖��`��

	@author Nakatani
	@date 2003.07.21 genta �ȑO��CMenuDrawer���ړ]����
	@date 2003.08.27 Moca �w�i�͓��ߏ����ɕύX���AcolBkColor���폜
*/
void CImageListMgr::MyBitBlt(
	HDC drawdc, 
	int nXDest, 
	int nYDest, 
	int nWidth, 
	int nHeight, 
	HBITMAP bmp, 
	int nXSrc, 
	int nYSrc
) const
{
	COLORREF colToTransParent = m_cTrans;	/* BMP�̒��̓����ɂ���F */
//	HBRUSH	brShadow, brHilight;
	HDC		hdcMask;
	HBITMAP bmpMask;
	HBITMAP bmpMaskOld;
	HDC		hdcMem;
	HBITMAP	bmpMemOld;
	HDC		hdcMem2;
	HBITMAP bmpMem2;
	HBITMAP bmpMem2Old;
	// create a monochrome memory DC
	hdcMask = CreateCompatibleDC(0);
	bmpMask = CreateCompatibleBitmap( hdcMask, nWidth, nHeight);
	bmpMaskOld = (HBITMAP)SelectObject( hdcMask, bmpMask);
	/* ���r�b�g�}�b�v�pDC */
	hdcMem = ::CreateCompatibleDC( drawdc );
	bmpMemOld = (HBITMAP)::SelectObject( hdcMem, bmp );
	/* ��ƗpDC */
	hdcMem2 = ::CreateCompatibleDC( drawdc );
	bmpMem2 = CreateCompatibleBitmap( drawdc, nWidth, nHeight);
	bmpMem2Old = (HBITMAP)SelectObject( hdcMem2, bmpMem2);

	// build a mask
//	2003.09.04 Moca bmpMask��bmp�̓]������傫���������Ȃ̂ŕs�v
//	PatBlt( hdcMask, 0, 0, nWidth, nHeight, WHITENESS);
	SetBkColor( hdcMem, colToTransParent );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCCOPY);

	/* �}�X�N�`��(�����ɂ��Ȃ��������������`��) */
	::SetBkColor( drawdc, RGB( 255, 255, 255 ) /* colBkColor */ ); // 2003.08.27 Moca �����@�ύX
	::SetTextColor( drawdc, RGB( 0, 0, 0 ) );
	// 2003.08.27 Moca �����@�ύX
	::BitBlt( drawdc, nXDest, nYDest, nWidth, nHeight, hdcMask, 0, 0, SRCAND /* SRCCOPY */ ); 

	/* �r�b�g�}�b�v�`��(�����ɂ���F���������ă}�X�N��OR�`��) */
	::SetBkColor( hdcMem2, colToTransParent/*RGB( 0, 0, 0 )*/ );
	::SetTextColor( hdcMem2, RGB( 0, 0, 0 ) );
	::BitBlt( hdcMem2, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCCOPY );
	::BitBlt( hdcMem2, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCINVERT/*SRCPAINT*/ );
	::BitBlt( drawdc, nXDest, nYDest, nWidth, nHeight, hdcMem2,  0, 0, /*SRCCOPY*/SRCPAINT );

	::SelectObject( hdcMask, bmpMaskOld );
	::DeleteObject( bmpMask );
	::DeleteDC( hdcMask );
	::SelectObject( hdcMem, bmpMemOld );
	::DeleteDC( hdcMem );
	::SelectObject( hdcMem2, bmpMem2Old );
	::DeleteObject( bmpMem2 );
	::DeleteDC( hdcMem2 );
	return;
}

/*! ���j���[�A�C�R���̒W�F�\��

	@author Nakatani
	
	@date 2003.07.21 genta �ȑO��CMenuDrawer���ړ]����
	@date 2003.08.27 Moca �w�i�F�͓��ߏ�������
*/
void CImageListMgr::DitherBlt2( HDC drawdc, int nXDest, int nYDest, int nWidth, 
                        int nHeight, HBITMAP bmp, int nXSrc, int nYSrc) const
{
	HDC		hdcMask;
	HBITMAP	bmpMask;
	HBITMAP	bmpMaskOld;
	HDC		hdcMem;
	HBITMAP	bmpMemOld;
	HDC		hdcMem2;
	HBITMAP bmpMem2;
	HBITMAP bmpMem2Old;

	//COLORREF colToTransParent = RGB( 192, 192, 192 );	/* BMP�̒��̓����ɂ���F */
	COLORREF colToTransParent = m_cTrans;

	// create a monochrome memory DC
	hdcMask = CreateCompatibleDC(0);
	bmpMask = CreateCompatibleBitmap( hdcMask, nWidth, nHeight);
	bmpMaskOld = (HBITMAP)SelectObject( hdcMask, bmpMask);

	hdcMem = CreateCompatibleDC(0);
	bmpMemOld = (HBITMAP)SelectObject( hdcMem, bmp);

	//	Jul. 21, 2003 genta
	//	hdcMem�ɏ������ނƌ���bitmap��j�󂵂Ă��܂�
	hdcMem2 = ::CreateCompatibleDC( drawdc );
	bmpMem2 = CreateCompatibleBitmap( drawdc, nWidth, nHeight);
	bmpMem2Old = (HBITMAP)SelectObject( hdcMem2, bmpMem2);

	// build a mask
	//	2003.09.04 Moca bmpMask��bmp�̓]������傫���������Ȃ̂ŕs�v
	//PatBlt( hdcMask, 0, 0, nWidth, nHeight, WHITENESS);
	SetBkColor( hdcMem, colToTransParent );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCCOPY);
	SetBkColor( hdcMem, RGB( 255, 255, 255 ) );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCPAINT);

	// Copy the image from the toolbar into the memory DC
	// and draw it (grayed) back into the toolbar.
    //SK: Looks better on the old shell
	// 2003.08.29 Moca �����@��ύX
	COLORREF coltxOld = ::SetTextColor( drawdc, RGB(0, 0, 0) );
	COLORREF colbkOld = ::SetBkColor( drawdc, RGB(255, 255, 255) );
	::SetBkColor( hdcMem2, RGB(0, 0, 0));
	::SetTextColor( hdcMem2, ::GetSysColor( COLOR_BTNHILIGHT ) );
	::BitBlt( hdcMem2, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCCOPY );
	::BitBlt( drawdc, nXDest+1, nYDest+1, nWidth, nHeight, hdcMask, 0, 0, SRCAND );
	::BitBlt( drawdc, nXDest+1, nYDest+1, nWidth, nHeight, hdcMem2, 0, 0, SRCPAINT);
	::SetTextColor( hdcMem2, ::GetSysColor( COLOR_BTNSHADOW ) );
	::BitBlt( hdcMem2, 0, 0, nWidth, nHeight, hdcMask, 0, 0, SRCCOPY );
	::BitBlt( drawdc, nXDest, nYDest, nWidth, nHeight, hdcMask, 0, 0, SRCAND );
	::BitBlt( drawdc, nXDest, nYDest, nWidth, nHeight, hdcMem2, 0, 0, SRCPAINT);
	::SetTextColor( drawdc, coltxOld );
	::SetBkColor( drawdc, colbkOld );

	// reset DCs
	SelectObject( hdcMask, bmpMaskOld);
	DeleteDC( hdcMask );

	SelectObject( hdcMem, bmpMemOld);
	DeleteDC( hdcMem );

	//	Jul. 21, 2003 genta
	::SelectObject( hdcMem2, bmpMem2Old );
	::DeleteObject( bmpMem2 );
	::DeleteDC( hdcMem2 );

	DeleteObject( bmpMask );
	return;

}

/*! @brief �A�C�R���̕`��

	�w�肳�ꂽDC�̎w�肳�ꂽ���W�ɃA�C�R����`�悷��D

	@param index [in] �`�悷��A�C�R���ԍ�
	@param dc [in] �`�悷��Device Context
	@param x [in] �`�悷��X���W
	@param y [in] �`�悷��Y���W
	@param fstyle [in] �`��X�^�C��
	@param bgColor [in] �w�i�F(���������̕`��p)

	@note �`��X�^�C���Ƃ��ėL���Ȃ̂́CILD_NORMAL, ILD_MASK
	
	@date 2003.07.21 genta �Ǝ��`�惋�[�`�����g��
	@date 2003.08.30 genta �w�i�F���w�肷�������ǉ�
	@date 2003.09.06 genta Moca����̔w�i�F���ߏ����ɔ����C�w�i�F�����폜
	@date 2007.11.02 ryoji �A�C�R���ԍ������̏ꍇ�͕`�悵�Ȃ�
*/
bool CImageListMgr::Draw(int index, HDC dc, int x, int y, int fstyle ) const
{
	if( m_hIconBitmap == NULL )
		return false;
	if( index < 0 )
		return false;

	if( fstyle == ILD_MASK ){
		DitherBlt2( dc, x, y, cx(), cy(), m_hIconBitmap,
		( index % MAX_X ) * cx(), ( index / MAX_X ) * cy());
	}
	else {
		MyBitBlt( dc, x, y, cx(), cy(), m_hIconBitmap,
		( index % MAX_X ) * cx(), ( index / MAX_X ) * cy());
	}
	return true;
}

/*!	�A�C�R������Ԃ�

	@date 2003.07.21 genta ���������ŊǗ�����K�v������D
*/
int CImageListMgr::Count() const
{
	return MAX_X * MAX_Y;
}
/*[EOF]*/