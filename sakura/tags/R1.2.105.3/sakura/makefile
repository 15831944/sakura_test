# Makefile for Sakura Editor
# 	Output - sakurabc.exe
# 	Compiler - borland C++ free compiler 5.5.1
# 	Base Version - Release 1.2.104.1 (2001 Dec 19)
# 	Usage - 'make -f sakurabc.mak [Option]'
# 	Option - '-DDEBUG'	'-DTRACE'
# 	Target - all(default) tall tool
#

## CPP Options
CPPFLAGS=-I..\sakura_core  -DWINVER=0x400 -D_WIN32_WINNT=0 -D_WIN32_IE=0x300 -M
RFLAGS=-dDS_SETFOREGROUND=0x200L
WARNINGS=-wamb -wamp -wbbf -wcln -wdef -wncf -wnak -wnod -wsig -wstu -wstv -wucp -wuse

## Optimizing Options	// DEBUG,TRACE,RELEASE
!ifdef DEBUG
OPTFLAGS=-Od -v -y -N -D_DEBUG $(WARNINGS)
!else
OPTFLAGS=-5 -O1 -Octv
!ifdef TRACE
OPTFLAGS=-v -y $(OPTFLAGS) $(WARNINGS)
!else
OPTFLAGS=$(OPTFLAGS) -DNDEBUG -w-8004  -w-8057
!endif
!endif

#DEF=EXPORT_SAKURA_DLL


## Directory and files
OUTDIR=..\sakura
SRCDIR=..\sakura_core
TOOLDIR=..\btool

OBJS =\
	$(SRCDIR)\CAutoSave.obj \
	$(SRCDIR)\CCommandLine.obj \
	$(SRCDIR)\CControlProcess.obj \
	$(SRCDIR)\CDialog.obj \
	$(SRCDIR)\CDicMgr.obj \
	$(SRCDIR)\CDlgAbout.obj \
	$(SRCDIR)\CDlgCancel.obj \
	$(SRCDIR)\CDlgCompare.obj \
	$(SRCDIR)\CDlgDebug.obj \
	$(SRCDIR)\CDlgExec.obj \
	$(SRCDIR)\CDlgFind.obj \
	$(SRCDIR)\CDlgFuncList.obj \
	$(SRCDIR)\CDlgGrep.obj \
	$(SRCDIR)\CDlgInput1.obj \
	$(SRCDIR)\CDlgJump.obj \
	$(SRCDIR)\CDlgOpenFile.obj \
	$(SRCDIR)\CDlgPrintPage.obj \
	$(SRCDIR)\CDlgPrintSetting.obj \
	$(SRCDIR)\CDlgProperty.obj \
	$(SRCDIR)\CDlgReplace.obj \
	$(SRCDIR)\CDlgTypeList.obj \
	$(SRCDIR)\CDllHandler.obj \
	$(SRCDIR)\CDocLine.obj \
	$(SRCDIR)\CDocLineMgr.obj \
	$(SRCDIR)\CDocLineMgr_New.obj \
	$(SRCDIR)\CDropTarget.obj \
	$(SRCDIR)\CEditApp.obj \
	$(SRCDIR)\CEditDoc.obj \
	$(SRCDIR)\CEditDoc_new.obj \
	$(SRCDIR)\CEditView.obj \
	$(SRCDIR)\CEditView_Command.obj \
	$(SRCDIR)\CEditView_Command_New.obj \
	$(SRCDIR)\CEditView_New.obj \
	$(SRCDIR)\CEditView_New2.obj \
	$(SRCDIR)\CEditView_New3.obj \
	$(SRCDIR)\CEditWnd.obj \
	$(SRCDIR)\CEol.obj \
	$(SRCDIR)\CFuncInfo.obj \
	$(SRCDIR)\CFuncInfoArr.obj \
	$(SRCDIR)\CFuncKeyWnd.obj \
	$(SRCDIR)\CFuncLookup.obj \
	$(SRCDIR)\CHokanMgr.obj \
	$(SRCDIR)\CHtmlHelp.obj \
	$(SRCDIR)\CImageListMgr.obj \
	$(SRCDIR)\CKeyBind.obj \
	$(SRCDIR)\CKeyMacroMgr.obj \
	$(SRCDIR)\CKeyWordSetMgr.obj \
	$(SRCDIR)\CLayout.obj \
	$(SRCDIR)\CLayoutMgr.obj \
	$(SRCDIR)\CLayoutMgr_New.obj \
	$(SRCDIR)\CLayoutMgr_New2.obj \
	$(SRCDIR)\CMacro.obj \
	$(SRCDIR)\CMarkMgr.obj \
	$(SRCDIR)\CMemory.obj \
	$(SRCDIR)\CMenuDrawer.obj \
	$(SRCDIR)\CMRU.obj \
	$(SRCDIR)\CMRUFolder.obj \
	$(SRCDIR)\CNormalProcess.obj \
	$(SRCDIR)\COpe.obj \
	$(SRCDIR)\COpeBlk.obj \
	$(SRCDIR)\COpeBuf.obj \
	$(SRCDIR)\CPrint.obj \
	$(SRCDIR)\CPrintPreview.obj \
	$(SRCDIR)\CProcess.obj \
	$(SRCDIR)\CProcessFactory.obj \
	$(SRCDIR)\CProfile.obj \
	$(SRCDIR)\CPropComBackup.obj \
	$(SRCDIR)\CPropComCustmenu.obj \
	$(SRCDIR)\CPropComEdit.obj \
	$(SRCDIR)\CPropComFile.obj \
	$(SRCDIR)\CPropComFormat.obj \
	$(SRCDIR)\CPropComGrep.obj \
	$(SRCDIR)\CPropComHelper.obj \
	$(SRCDIR)\CPropComKeybind.obj \
	$(SRCDIR)\CPropComKeyword.obj \
	$(SRCDIR)\CPropCommon.obj \
	$(SRCDIR)\CPropComMacro.obj \
	$(SRCDIR)\CPropComToolbar.obj \
	$(SRCDIR)\CPropComUrl.obj \
	$(SRCDIR)\CPropComWin.obj \
	$(SRCDIR)\CPropTypes.obj \
	$(SRCDIR)\CPropTypesRegex.obj \
	$(SRCDIR)\CRunningTimer.obj \
	$(SRCDIR)\CRegexKeyword.obj \
	$(SRCDIR)\CShareData_new.obj \
	$(SRCDIR)\CShareData.obj \
	$(SRCDIR)\CShareData_new2.obj \
	$(SRCDIR)\CSMacroMgr.obj \
	$(SRCDIR)\CSplitBoxWnd.obj \
	$(SRCDIR)\CSplitterWnd.obj \
	$(SRCDIR)\CTipWnd.obj \
	$(SRCDIR)\CWnd.obj \
	$(SRCDIR)\CWaitCursor.obj \
	$(SRCDIR)\CBregexp.obj \
	$(SRCDIR)\Debug.obj \
	$(SRCDIR)\etc_uty.obj \
	$(SRCDIR)\Funccode.obj \
	$(SRCDIR)\global.obj \
	$(SRCDIR)\my_icmp.obj \
	$(SRCDIR)\StdAfx.obj \
	$(SRCDIR)\WinMain.obj \

## Targets
all : tool tall


## Tool for Borland C++
tool : $(TOOLDIR)\mrc2brc.exe

$(TOOLDIR)\mrc2brc.exe : $(TOOLDIR)\mrc2brc.obj $(TOOLDIR)\RcCvt.obj
	$(CC) $(CPPFLAGS) $(OPTFLAGS) -WC -e$(TOOLDIR)\$. $**

## Editor includes sakura_core
tall : $(SRCDIR)\sakurabc.rc sakura_rc.exe
	brc32 $(RFLAGS) $** 
	move /y sakura_rc.exe sakura.exe
	move /y sakura_rc.map sakura.map


sakura_rc.exe : $(OBJS)
	$(CC) $(CPPFLAGS) $(OPTFLAGS) -W -e$(OUTDIR)\$. $** 

$(SRCDIR)\sakurabc.rc : $(SRCDIR)\sakura_rc.rc
	$(TOOLDIR)\mrc2brc	$** $:\$. > $(TOOLDIR)\mrc2brc.log


.cpp.obj :
	$(CC) $(CPPFLAGS) $(OPTFLAGS) -o$@ -c $<

cleanall : clean
	erase $(TOOLDIR)\mrc2brc.exe
	erase $(TOOLDIR)\mrc2brc.map
	erase $(TOOLDIR)\mrc2brc.tds
	erase $(TOOLDIR)\mrc2brc.log
	erase $(TOOLDIR)\*.obj

clean : clobber
	erase $(OUTDIR)\*.exe

clobber :
	erase $(SRCDIR)\sakurabc.res
	erase $(SRCDIR)\sakurabc.rc
	erase $(SRCDIR)\*.obj
	erase $(OUTDIR)\*.map
	erase $(OUTDIR)\*.tds

#	echo "*$* :$: .$. &$& @$@ **$** ?$?"


