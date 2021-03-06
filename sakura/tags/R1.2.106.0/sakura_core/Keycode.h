//	$Id$
/*!	@file
	@brief キーコード定義

	@author Norio Nakatani
	$Revision$
*/
/*
	Copyright (C) 1998-2001, Norio Nakatani

	This source code is designed for sakura editor.
	Please contact the copyright holder to use this code for other purpose.
*/

#ifndef _KEYCODE_H_
#define _KEYCODE_H_


#include <windows.h>
#include "funccode.h"

#define _SHIFT	0x00000001
#define _CTRL	0x00000002
#define _ALT	0x00000004

/* キーコードの定義 */
/* マウス操作 */
#define	KEY_LBUTTONCLK	 VK_LBUTTON
#define	KEY_LBUTTONDBLCLK 0x00000200
#define	KEY_RBUTTONCLK	 VK_RBUTTON
#define	KEY_RBUTTONDBLCLK 0x00000400



///////////////////////////////////////////////////////////////////////
#endif /* _KEYCODE_H_ */


/*[EOF]*/
