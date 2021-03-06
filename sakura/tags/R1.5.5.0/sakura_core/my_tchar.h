/*
	my_icmp/my_stringのShift_JISとUnicodeを切り替えるための定義
*/

#ifndef _MY_TCHAR_H__
#define _MY_TCHAR_H__
 
/* include files */
#include "my_icmp.h"
// #include "my_string.h"

	
/* ************ _UNICODE ************ */
#ifdef _UNICODE

 /* my_icmp */
#define  my_tmemicmp my_wmemicmp
#define  my_tcsicmp  my_wcsicmp
#define  my_tcsnicmp my_wcsnicmp

 /* CRT */
#ifndef _tmemcpy
#define _tmemcpy     wmemcpy
#endif

 
/* ********** MBCS or CBCS ********** */
#else

 /*  my_icmp */
#define  my_tmemicmp my_memicmp
#define  my_tcsicmp  my_stricmp
#define  my_tcsnicmp my_strnicmp

 /* CRT */
#ifndef _tmemcpy
#define _tmemcpy     memcpy
#endif

#endif

#endif /* _MY_TCHAR_H__ */
/*[EOF]*/
