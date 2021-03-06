/*!	@file
	@brief ImageListの取り扱い

	@author genta
	@date Oct. 11, 2000 genta
*/
/*
	Copyright (C) 2000-2001, genta
	Copyright (C) 2000, jepro
	Copyright (C) 2001, GAE, jepro
	Copyright (C) 2003, Moca, genta, wmlhq

	This source code is designed for sakura editor.
	Please contact the copyright holders to use this code for other purpose.
*/
#include "stdafx.h"
#include "CImageListMgr.h"
#include "sakura_rc.h"
#include "CRunningTimer.h"
#include "etc_uty.h"

//	Jul. 21, 2003 genta 他でも使うので関数の外に出した
//	Oct. 21, 2000 JEPRO 設定
const int MAX_X = 32;
const int MAX_Y = 12;	//2002.01.17

/*!	領域を指定色で塗りつぶす

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
	//	2003.07.21 Image Listの代わりに描画用bitmapを解放
	if( m_hIconBitmap != NULL ){
		DeleteObject( m_hIconBitmap );
	}
}

/*
	@brief Image Listの作成
	
	リソースまたはファイルからbitmapを読み込んで
	描画用に保持する．
	
	@param hInstance [in] bitmapリソースを持つインスタンス
	@param hWnd [in] 未使用
	
	@date 2003.07.21 genta ImageListの構築は行わない．代わりにbitmapをそのまま保持する．
*/
bool CImageListMgr::Create(HINSTANCE hInstance, HWND hWnd)
{
	MY_RUNNINGTIMER( cRunningTimer, "CImageListMgr::Create" );
	if( m_hIconBitmap != NULL ){	//	既に構築済みなら無視する
		return true;
	}

	HBITMAP	hRscbmp;	//	リソースから読み込んだひとかたまりのBitmap
	HBITMAP	hFOldbmp;	//	SetObjectで得られた1つ前のハンドルを保持する
	HDC		dcFrom;		//	描画用
	int		nRetPos;	//	後処理用
	m_cx = m_cy  = 16;

	nRetPos = 0;
	do {
		//	From Here 2001.7.1 GAE
		//	2001.7.1 GAE リソースをローカルファイル(sakuraディレクトリ) my_icons.bmp から読めるように
		char	szPath[_MAX_PATH], szExeDrive[_MAX_DRIVE], szExeDir[_MAX_DIR];

		// sakura.exe のパスを取得
		::GetModuleFileName( ::GetModuleHandle(NULL), szPath, sizeof(szPath) );

		// (sakuraディレクトリ) my_icons.bmp の文字列を作成
		_splitpath( szPath, szExeDrive, szExeDir, NULL, NULL );
		//	@@TEST@@
		wsprintf( szPath, "%s%s" FN_TOOL_BMP, szExeDrive, szExeDir );	//Jul.04, 2001 JEPRO toolbar.bmp→my_icons.bmp に名前変更
		hRscbmp = (HBITMAP)::LoadImage( NULL, szPath, IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS );

		if( hRscbmp == NULL ) {	// ローカルファイルの読み込み失敗時はリソースから取得
			//	このブロック内は従来の処理
			//	リソースからBitmapを読み込む
			//	2003.09.29 wmlhq 環境によってアイコンがつぶれる
			//hRscbmp = ::LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_MYTOOL ) );
			hRscbmp = (HBITMAP)::LoadImage( hInstance, MAKEINTRESOURCE( IDB_MYTOOL ), IMAGE_BITMAP, 0, 0,
				LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS  );
			if( hRscbmp == NULL ){
				//	Oct. 4, 2003 genta エラーコード追加
				//	正常終了と同じコードだとdcFromを不正に解放してしまう
				nRetPos = 2;
				break;
			}
		}
		//	To Here 2001.7.1 GAE
		m_hIconBitmap = hRscbmp;

		//	透過色を得るためにDCにマップする
		//	2003.07.21 genta 透過色を得る以外の目的では使わなくなった
		dcFrom = CreateCompatibleDC(0);	//	転送元用
		if( dcFrom == NULL ){
			nRetPos = 1;
			break;
		}

		//	まずbitmapをdcにmapする
		//	こうすることでCreateCompatibleBitmapで
		//	hRscbmpと同じ形式のbitmapを作れる．
		//	単にCreateCompatibleDC(0)で取得したdcや
		//	スクリーンのDCに対してCreateCompatibleBitmapを
		//	使うとモノクロBitmapになる．
		hFOldbmp = (HBITMAP)SelectObject( dcFrom, hRscbmp );
		if( hFOldbmp == NULL ){
			nRetPos = 4;
			break;
		}

		m_cTrans = GetPixel( dcFrom, 0, 0 );//	取得した画像の(0,0)の色を背景色として使う
		
		//	2003.07.21 genta
		//	ImageListへの登録部分は当然ばっさり削除
		
		//	もはや処理とは無関係だが，後学のためにコメントのみ残しておこう
		//---------------------------------------------------------
		//	BitmapがMemoryDCにAssignされている間はbitmapハンドルを
		//	使っても正しいbitmapが取得できない．
		//	つまり，DCへの描画命令を発行してもその場でBitmapに
		//	反映されるわけではない．
		//	BitmapをDCから取り外して初めて内容の保証ができる

		//	DCのmap/unmapが速度に大きく影響するため，
		//	横長のBitmapを作って一括登録するように変更
		//	これによって250msecくらい速度が改善される．
		//---------------------------------------------------------

	}while(0);	//	1回しか通らない

	//	後処理
	switch( nRetPos ){
	case 0:
		//	Oct. 4, 2003 genta hRscBmpをdcFromから切り離しておく必要がある
		//	アイコン描画変更時に過って削除されていた
		SelectObject( dcFrom, hFOldbmp );
	case 4:
		DeleteDC( dcFrom );
	case 2:
	case 1:
		//	2003.07.21 genta hRscbmpは m_hIconBitmap としてオブジェクトと
		//	同じだけ保持されるので解放してはならない
		break;
	}

	return nRetPos == 0;
}


/*! ビットマップの表示 灰色を透明描画

	@author Nakatani
	@date 2003.07.21 genta 以前のCMenuDrawerより移転復活
	@date 2003.08.27 Moca 背景は透過処理に変更し、colBkColorを削除
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
	COLORREF colToTransParent = m_cTrans;	/* BMPの中の透明にする色 */
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
	/* 元ビットマップ用DC */
	hdcMem = ::CreateCompatibleDC( drawdc );
	bmpMemOld = (HBITMAP)::SelectObject( hdcMem, bmp );
	/* 作業用DC */
	hdcMem2 = ::CreateCompatibleDC( drawdc );
	bmpMem2 = CreateCompatibleBitmap( drawdc, nWidth, nHeight);
	bmpMem2Old = (HBITMAP)SelectObject( hdcMem2, bmpMem2);

	// build a mask
//	2003.09.04 Moca bmpMaskとbmpの転送する大きさが同じなので不要
//	PatBlt( hdcMask, 0, 0, nWidth, nHeight, WHITENESS);
	SetBkColor( hdcMem, colToTransParent );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCCOPY);

	/* マスク描画(透明にしない部分だけ黒く描画) */
	::SetBkColor( drawdc, RGB( 255, 255, 255 ) /* colBkColor */ ); // 2003.08.27 Moca 作画方法変更
	::SetTextColor( drawdc, RGB( 0, 0, 0 ) );
	// 2003.08.27 Moca 作画方法変更
	::BitBlt( drawdc, nXDest, nYDest, nWidth, nHeight, hdcMask, 0, 0, SRCAND /* SRCCOPY */ ); 

	/* ビットマップ描画(透明にする色を黒くしてマスクとOR描画) */
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

/*! メニューアイコンの淡色表示

	@author Nakatani
	
	@date 2003.07.21 genta 以前のCMenuDrawerより移転復活
	@date 2003.08.27 Moca 背景色は透過処理する
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

	//COLORREF colToTransParent = RGB( 192, 192, 192 );	/* BMPの中の透明にする色 */
	COLORREF colToTransParent = m_cTrans;

	// create a monochrome memory DC
	hdcMask = CreateCompatibleDC(0);
	bmpMask = CreateCompatibleBitmap( hdcMask, nWidth, nHeight);
	bmpMaskOld = (HBITMAP)SelectObject( hdcMask, bmpMask);

	hdcMem = CreateCompatibleDC(0);
	bmpMemOld = (HBITMAP)SelectObject( hdcMem, bmp);

	//	Jul. 21, 2003 genta
	//	hdcMemに書き込むと元のbitmapを破壊してしまう
	hdcMem2 = ::CreateCompatibleDC( drawdc );
	bmpMem2 = CreateCompatibleBitmap( drawdc, nWidth, nHeight);
	bmpMem2Old = (HBITMAP)SelectObject( hdcMem2, bmpMem2);

	// build a mask
	//	2003.09.04 Moca bmpMaskとbmpの転送する大きさが同じなので不要
	//PatBlt( hdcMask, 0, 0, nWidth, nHeight, WHITENESS);
	SetBkColor( hdcMem, colToTransParent );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCCOPY);
	SetBkColor( hdcMem, RGB( 255, 255, 255 ) );
	BitBlt( hdcMask, 0, 0, nWidth, nHeight, hdcMem, nXSrc,nYSrc, SRCPAINT);

	// Copy the image from the toolbar into the memory DC
	// and draw it (grayed) back into the toolbar.
    //SK: Looks better on the old shell
	// 2003.08.29 Moca 作画方法を変更
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

/*! @brief アイコンの描画

	指定されたDCの指定された座標にアイコンを描画する．

	@param index [in] 描画するアイコン番号
	@param dc [in] 描画するDevice Context
	@param x [in] 描画するX座標
	@param y [in] 描画するY座標
	@param fstyle [in] 描画スタイル
	@param bgColor [in] 背景色(透明部分の描画用)

	@note 描画スタイルとして有効なのは，ILD_NORMAL, ILD_MASK
	
	@date 2003.07.21 genta 独自描画ルーチンを使う
	@date 2003.08.30 genta 背景色を指定する引数を追加
	@date 2003.09.06 genta Mocaさんの背景色透過処理に伴い，背景色引数削除
*/
bool CImageListMgr::Draw(int index, HDC dc, int x, int y, int fstyle ) const
{
	if( m_hIconBitmap == NULL )
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

/*!	アイコン数を返す

	@date 2003.07.21 genta 個数を自分で管理する必要がある．
*/
int CImageListMgr::Count() const
{
	return MAX_X * MAX_Y;
}
/*[EOF]*/
