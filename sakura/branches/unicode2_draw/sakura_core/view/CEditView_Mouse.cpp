#include "stdafx.h"
#include "CEditView.h"
#include "util/input.h"
#include "types/CTypeSupport.h"
#include "util/os.h"
#include "parse/CWordParse.h"
#include "COpeBlk.h"
#include "view/colors/CColorStrategy.h"
#include "CClipboard.h"
#include "doc/CLayout.h"

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                      �}�E�X�C�x���g                         //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

/* �}�E�X���{�^������ */
void CEditView::OnLBUTTONDOWN( WPARAM fwKeys, int _xPos , int _yPos )
{
	CMyPoint ptMouse(_xPos,_yPos);

	if( m_bHokan ){
		m_pcEditDoc->m_pcEditWnd->m_cHokanMgr.Hide();
		m_bHokan = FALSE;
	}

	//isearch 2004.10.22 isearch���L�����Z������
	if (m_nISearchMode > 0 ){
		ISearchExit();
	}

	CNativeW	cmemCurText;
	const wchar_t*	pLine;
	CLogicInt		nLineLen;

	CLayoutRange sRange;

	CLogicInt	nIdx;
	int			nWork;
	BOOL		tripleClickMode = FALSE;	// 2007.10.02 nasukoji	�g���v���N���b�N�ł��邱�Ƃ�����
	int			nFuncID = 0;				// 2007.12.02 nasukoji	�}�E�X���N���b�N�ɑΉ�����@�\�R�[�h

	if( m_pcEditDoc->m_cLayoutMgr.GetLineCount() == 0 ){
		return;
	}
	if( !GetCaret().ExistCaretFocus() ){ //�t�H�[�J�X���Ȃ��Ƃ�
		return;
	}

	/* ����Tip���N������Ă��� */
	if( 0 == m_dwTipTimer ){
		/* ����Tip������ */
		m_cTipWnd.Hide();
		m_dwTipTimer = ::GetTickCount();	/* ����Tip�N���^�C�}�[ */
	}
	else{
		m_dwTipTimer = ::GetTickCount();		/* ����Tip�N���^�C�}�[ */
	}

	// 2007.12.02 nasukoji	�g���v���N���b�N���`�F�b�N
	tripleClickMode = CheckTripleClick(ptMouse);

	if(tripleClickMode){
		// �}�E�X���g���v���N���b�N�ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[5]�ɓ����Ă���
		nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_TRIPLECLICK].m_nFuncCodeArr[getCtrlKeyState()];
		if( 0 == nFuncID ){
			tripleClickMode = 0;	// ���蓖�ċ@�\�����̎��̓g���v���N���b�N OFF
		}
	}else{
		m_dwTripleClickCheck = 0;	// �g���v���N���b�N�`�F�b�N OFF
	}

	/* ���݂̃}�E�X�J�[�\���ʒu�����C�A�E�g�ʒu */
	CLayoutPoint ptNew;
	GetTextArea().ClientToLayout(ptMouse, &ptNew);

	// OLE�ɂ��h���b�O & �h���b�v���g��
	// 2007.12.02 nasukoji	�g���v���N���b�N���̓h���b�O���J�n���Ȃ�
	if( !tripleClickMode && GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DragDrop ){
		if( GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DropSource ){		/* OLE�ɂ��h���b�O���ɂ��邩 */
			/* �s�I���G���A���h���b�O���� */
			if( ptMouse.x < GetTextArea().GetAreaLeft() - GetTextMetrics().GetHankakuDx() ){
				goto normal_action;
			}
			/* �w��J�[�\���ʒu���I���G���A���ɂ��邩 */
			if( 0 == IsCurrentPositionSelected(ptNew) ){
				/* �I��͈͂̃f�[�^���擾 */
				if( GetSelectedData( &cmemCurText, FALSE, NULL, FALSE, GetDllShareData().m_Common.m_sEdit.m_bAddCRLFWhenCopy ) ){
					DWORD dwEffects;
					int nOpe = m_pcEditDoc->m_cDocEditor.m_cOpeBuf.GetCurrentPointer();
					int nTickDrag = ::GetTickCount();
					m_pcEditWnd->SetDragSourceView( this );
					CDataObject data( cmemCurText.GetStringPtr(), cmemCurText.GetStringLength(), GetSelectionInfo().IsBoxSelecting() );
					dwEffects = data.DragDrop( TRUE, DROPEFFECT_COPY | DROPEFFECT_MOVE );
					m_pcEditWnd->SetDragSourceView( NULL );
					if( m_pcEditDoc->m_cDocEditor.m_cOpeBuf.GetCurrentPointer() == nOpe ){	// �h�L�������g�ύX�Ȃ����H	// 2007.12.09 ryoji
						m_pcEditWnd->SetActivePane( m_nMyIndex );
						if( ::GetTickCount() - nTickDrag <= ::GetDoubleClickTime() ){	// �Z���ԂȂ�N���b�N�Ƃ݂Ȃ�
							// �N���b�N�ʒu�ɃJ�[�\���ړ�����
							if( GetSelectionInfo().IsTextSelected() ){	/* �e�L�X�g���I������Ă��邩 */
								/* ���݂̑I��͈͂��I����Ԃɖ߂� */
								GetSelectionInfo().DisableSelectArea( TRUE );
							}

//@@@ 2002.01.08 YAZAKI �t���[�J�[�\��OFF�ŕ����s�I�����A�s�̌����N���b�N����Ƃ����ɃL�����b�g���u����Ă��܂��o�O�C��
							/* �J�[�\���ړ��B */
							if( ptMouse.y >= GetTextArea().GetAreaTop() && ptMouse.y < GetTextArea().GetAreaBottom() ){
								if( ptMouse.x >= GetTextArea().GetAreaLeft() && ptMouse.x < GetTextArea().GetAreaRight() ){
									GetCaret().MoveCursorToClientPoint( ptMouse );
								}
								else if( ptMouse.x < GetTextArea().GetAreaLeft() ){
									GetCaret().MoveCursorToClientPoint( CMyPoint(GetTextArea().GetDocumentLeftClientPointX(), ptMouse.y) );
								}
							}
						}else if( DROPEFFECT_MOVE == dwEffects ){
							// �ړ��͈͂��폜����
							// �h���b�v�悪�ړ����������������h�L�������g�ɂ����܂ŕύX������
							// ���h���b�v��͊O���̃E�B���h�E�ł���
							if( NULL == m_pcOpeBlk ){
								m_pcOpeBlk = new COpeBlk;
							}

							// �I��͈͂��폜
							DeleteData( TRUE );

							// �A���h�D�o�b�t�@�̏���
							if( NULL != m_pcOpeBlk ){
								if( 0 < m_pcOpeBlk->GetNum() ){
									m_pcEditDoc->m_cDocEditor.m_cOpeBuf.AppendOpeBlk( m_pcOpeBlk );
								}else{
									delete m_pcOpeBlk;
								}
								m_pcOpeBlk = NULL;
							}
						}
					}
				}
				return;
			}
		}
	}

normal_action:;

	// ALT�L�[��������Ă���A���g���v���N���b�N�łȂ�		// 2007.11.15 nasukoji	�g���v���N���b�N�Ή�
	if( GetKeyState_Alt() &&( ! tripleClickMode)){
		if( GetSelectionInfo().IsTextSelected() ){	/* �e�L�X�g���I������Ă��邩 */
			/* ���݂̑I��͈͂��I����Ԃɖ߂� */
			GetSelectionInfo().DisableSelectArea( TRUE );
		}
		if( ptMouse.y >= GetTextArea().GetAreaTop()  && ptMouse.y < GetTextArea().GetAreaBottom() ){
			if( ptMouse.x >= GetTextArea().GetAreaLeft() && ptMouse.x < GetTextArea().GetAreaRight() ){
				GetCaret().MoveCursorToClientPoint( ptMouse );
			}
			else if( ptMouse.x < GetTextArea().GetAreaLeft() ){
				GetCaret().MoveCursorToClientPoint( CMyPoint(GetTextArea().GetDocumentLeftClientPointX(), ptMouse.y) );
			}else{
				return;
			}
		}
		GetSelectionInfo().m_ptMouseRollPosOld = ptMouse;	// �}�E�X�͈͑I��O��ʒu(XY���W)
		/*
		m_nMouseRollPosXOld = xPos;		// �}�E�X�͈͑I��O��ʒu(X���W)
		m_nMouseRollPosYOld = yPos;		// �}�E�X�͈͑I��O��ʒu(Y���W)
		*/

		/* �͈͑I���J�n & �}�E�X�L���v�`���[ */
		GetSelectionInfo().SelectBeginBox();

		::SetCapture( GetHwnd() );
		GetCaret().HideCaret_( GetHwnd() ); // 2002/07/22 novice
		/* ���݂̃J�[�\���ʒu����I�����J�n���� */
		GetSelectionInfo().BeginSelectArea( );
		GetCaret().m_cUnderLine.CaretUnderLineOFF( TRUE );
		GetCaret().m_cUnderLine.Lock();
		if( ptMouse.x < GetTextArea().GetAreaLeft() ){
			/* �J�[�\�����ړ� */
			GetCommander().Command_DOWN( TRUE, FALSE );
		}
	}
	else{
		/* �J�[�\���ړ� */
		if( ptMouse.y >= GetTextArea().GetAreaTop() && ptMouse.y < GetTextArea().GetAreaBottom() ){
			if( ptMouse.x >= GetTextArea().GetAreaLeft() && ptMouse.x < GetTextArea().GetAreaRight() ){
			}
			else if( ptMouse.x < GetTextArea().GetAreaLeft() ){
			}
			else{
				return;
			}
		}
		else if( ptMouse.y < GetTextArea().GetAreaTop() ){
			//	���[���N���b�N
			return;
		}
		else {
			return;
		}

		/* �}�E�X�̃L���v�`���Ȃ� */
		GetSelectionInfo().m_ptMouseRollPosOld = ptMouse;	// �}�E�X�͈͑I��O��ʒu(XY���W)
		
		/* �͈͑I���J�n & �}�E�X�L���v�`���[ */
		GetSelectionInfo().SelectBeginNazo();
		::SetCapture( GetHwnd() );
		GetCaret().HideCaret_( GetHwnd() ); // 2002/07/22 novice


		if(tripleClickMode){		// 2007.11.15 nasukoji	�g���v���N���b�N����������
			// 1�s�I���łȂ��ꍇ�͑I�𕶎��������
			// �g���v���N���b�N��1�s�I���łȂ��Ă��N�A�h���v���N���b�N��L���Ƃ���
			if(F_SELECTLINE != nFuncID){
				OnLBUTTONUP( fwKeys, ptMouse.x, ptMouse.y );	// �����ō��{�^���A�b�v�������Ƃɂ���

				if( GetSelectionInfo().IsTextSelected() )		// �e�L�X�g���I������Ă��邩
					GetSelectionInfo().DisableSelectArea( TRUE );	// ���݂̑I��͈͂��I����Ԃɖ߂�
			}

			// �P��̓r���Ő܂�Ԃ���Ă���Ɖ��̍s���I������Ă��܂����Ƃւ̑Ώ�
			GetCaret().MoveCursorToClientPoint( ptMouse );	// �J�[�\���ړ�

			// �R�}���h�R�[�h�ɂ�鏈���U�蕪��
			// �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
			::SendMessage( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ), (LPARAM)NULL );

			// 1�s�I���łȂ��ꍇ�͂����Ŕ�����i���̑I���R�}���h�̎����ƂȂ邩���j
			if(F_SELECTLINE != nFuncID)
				return;

			// �I��������̂������i[EOF]�݂̂̍s�j���͒ʏ�N���b�N�Ɠ�������
			if(( ! GetSelectionInfo().IsTextSelected() )&&
			   ( GetCaret().GetCaretLogicPos().y >= m_pcEditDoc->m_cDocLineMgr.GetLineCount() ))
			{
				GetSelectionInfo().BeginSelectArea();				// ���݂̃J�[�\���ʒu����I�����J�n����
				GetSelectionInfo().m_bBeginLineSelect = FALSE;		// �s�P�ʑI�� OFF
			}
		}else
		/* �I���J�n���� */
		/* SHIFT�L�[��������Ă����� */
		if(GetKeyState_Shift()){
			if( GetSelectionInfo().IsTextSelected() ){		/* �e�L�X�g���I������Ă��邩 */
				if( GetSelectionInfo().IsBoxSelecting() ){	/* ��`�͈͑I�� */
					/* ���݂̑I��͈͂��I����Ԃɖ߂� */
					GetSelectionInfo().DisableSelectArea( TRUE );

					/* ���݂̃J�[�\���ʒu����I�����J�n���� */
					GetSelectionInfo().BeginSelectArea( );
				}
				else{
				}
			}
			else{
				/* ���݂̃J�[�\���ʒu����I�����J�n���� */
				GetSelectionInfo().BeginSelectArea( );
			}

			/* �J�[�\���ړ� */
			if( ptMouse.y >= GetTextArea().GetAreaTop() && ptMouse.y < GetTextArea().GetAreaBottom() ){
				if( ptMouse.x >= GetTextArea().GetAreaLeft() && ptMouse.x < GetTextArea().GetAreaRight() ){
					GetCaret().MoveCursorToClientPoint( ptMouse );
				}
				else if( ptMouse.x < GetTextArea().GetAreaLeft() ){
					GetCaret().MoveCursorToClientPoint( CMyPoint(GetTextArea().GetDocumentLeftClientPointX(), ptMouse.y) );
				}
			}
		}
		else{
			if( GetSelectionInfo().IsTextSelected() ){	/* �e�L�X�g���I������Ă��邩 */
				/* ���݂̑I��͈͂��I����Ԃɖ߂� */
				GetSelectionInfo().DisableSelectArea( TRUE );
			}
			/* �J�[�\���ړ� */
			if( ptMouse.y >= GetTextArea().GetAreaTop() && ptMouse.y < GetTextArea().GetAreaBottom() ){
				if( ptMouse.x >= GetTextArea().GetAreaLeft() && ptMouse.x < GetTextArea().GetAreaRight() ){
					GetCaret().MoveCursorToClientPoint( ptMouse );
				}
				else if( ptMouse.x < GetTextArea().GetAreaLeft() ){
					GetCaret().MoveCursorToClientPoint( CMyPoint(GetTextArea().GetDocumentLeftClientPointX(), ptMouse.y) );
				}
			}
			/* ���݂̃J�[�\���ʒu����I�����J�n���� */
			GetSelectionInfo().BeginSelectArea( );
		}


		/******* ���̎��_�ŕK�� true == GetSelectionInfo().IsTextSelected() �̏�ԂɂȂ� ****:*/
		if( !GetSelectionInfo().IsTextSelected() ){
			WarningMessage( GetHwnd(), _T("�o�O���Ă�") );
			return;
		}

		int	nWorkRel;
		nWorkRel = IsCurrentPositionSelected(
			GetCaret().GetCaretLayoutPos()	// �J�[�\���ʒu
		);


		/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
		GetSelectionInfo().ChangeSelectAreaByCurrentCursor( GetCaret().GetCaretLayoutPos() );


		// CTRL�L�[��������Ă���A���g���v���N���b�N�łȂ�		// 2007.11.15 nasukoji	�g���v���N���b�N�Ή�
		if( GetKeyState_Control() &&( ! tripleClickMode)){
			GetSelectionInfo().m_bBeginWordSelect = TRUE;		/* �P��P�ʑI�� */
			if( !GetSelectionInfo().IsTextSelected() ){
				/* ���݈ʒu�̒P��I�� */
				if ( GetCommander().Command_SELECTWORD() ){
					GetSelectionInfo().m_sSelectBgn = GetSelectionInfo().m_sSelect;
				}
			}else{

				/* �I��̈�`�� */
				GetSelectionInfo().DrawSelectArea();


				/* �w�肳�ꂽ���ɑΉ�����s�̃f�[�^���̈ʒu�𒲂ׂ� */
				const CLayout* pcLayout;
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr(
					GetSelectionInfo().m_sSelect.GetFrom().GetY2(),
					&nLineLen,
					&pcLayout
				);
				if( NULL != pLine ){
					nIdx = LineColmnToIndex( pcLayout, GetSelectionInfo().m_sSelect.GetFrom().GetX2() );
					/* ���݈ʒu�̒P��͈̔͂𒲂ׂ� */
					int nWhareResult = m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
						GetSelectionInfo().m_sSelect.GetFrom().GetY2(),
						nIdx,
						&sRange,
						NULL,
						NULL
					);
					if( nWhareResult ){
						// �w�肳�ꂽ�s�̃f�[�^���̈ʒu�ɑΉ����錅�̈ʒu�𒲂ׂ�B
						// 2007.10.15 kobake ���Ƀ��C�A�E�g�P�ʂȂ̂ŕϊ��͕s�v
						/*
						pLine            = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetFrom().GetY2(), &nLineLen, &pcLayout );
						sRange.SetFromX( LineIndexToColmn( pcLayout, sRange.GetFrom().x ) );
						pLine            = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetTo().GetY2(), &nLineLen, &pcLayout );
						sRange.SetToX( LineIndexToColmn( pcLayout, sRange.GetTo().x ) );
						*/

						nWork = IsCurrentPositionSelected(
							sRange.GetFrom()	// �J�[�\���ʒu
						);
						if( -1 == nWork || 0 == nWork ){
							GetSelectionInfo().m_sSelect.SetFrom(sRange.GetFrom());
							if( 1 == nWorkRel ){
								GetSelectionInfo().m_sSelectBgn = sRange;
							}
						}
					}
				}
				pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( GetSelectionInfo().m_sSelect.GetTo().GetY2(), &nLineLen, &pcLayout );
				if( NULL != pLine ){
					nIdx = LineColmnToIndex( pcLayout, GetSelectionInfo().m_sSelect.GetTo().GetX2() );
					/* ���݈ʒu�̒P��͈̔͂𒲂ׂ� */
					if( m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
						GetSelectionInfo().m_sSelect.GetTo().GetY2(), nIdx, &sRange, NULL, NULL )
					){
						// �w�肳�ꂽ�s�̃f�[�^���̈ʒu�ɑΉ����錅�̈ʒu�𒲂ׂ�
						// 2007.10.15 kobake ���Ƀ��C�A�E�g�P�ʂȂ̂ŕϊ��͕s�v
						/*
						pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetFrom().GetY2(), &nLineLen, &pcLayout );
						sRange.SetFromX( LineIndexToColmn( pcLayout, sRange.GetFrom().x ) );
						pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetTo().GetY2(), &nLineLen, &pcLayout );
						sRange.SetToX( LineIndexToColmn( pcLayout, sRange.GetTo().x ) );
						*/

						nWork = IsCurrentPositionSelected(sRange.GetFrom());
						if( -1 == nWork || 0 == nWork ){
							GetSelectionInfo().m_sSelect.SetTo(sRange.GetFrom());
						}
						if( 1 == IsCurrentPositionSelected(sRange.GetTo()) ){
							GetSelectionInfo().m_sSelect.SetTo(sRange.GetTo());
						}
						if( -1 == nWorkRel || 0 == nWorkRel ){
							GetSelectionInfo().m_sSelectBgn=sRange;
						}
					}
				}

				if( 0 < nWorkRel ){

				}
				/* �I��̈�`�� */
				GetSelectionInfo().DrawSelectArea();
			}
		}
		if( ptMouse.x < GetTextArea().GetAreaLeft() ){
			/* ���݂̃J�[�\���ʒu����I�����J�n���� */
			GetSelectionInfo().m_bBeginLineSelect = TRUE;

			// 2002.10.07 YAZAKI �܂�Ԃ��s���C���f���g���Ă���Ƃ��ɑI�������������o�O�̑΍�
			// �P�s����ʕ����������ƍ��E�ɃX�N���[�����Ă�������������Ȃ�̂Ō�őS�̂��ĕ`��	// 2008.05.20 ryoji
			bool bDrawSwitchOld = GetDrawSwitch();
			bool bDrawAfter = false;
			if( bDrawSwitchOld ){
				const CLayout* pcLayout = m_pcEditDoc->m_cLayoutMgr.SearchLineByLayoutY( GetCaret().GetCaretLayoutPos().GetY2() );
				if( pcLayout ){
					CLayoutInt nColumn = LineIndexToColmn( pcLayout, CLogicInt(pcLayout->GetLengthWithoutEOL()) );
					bDrawAfter = (nColumn + CLayoutInt(SCROLLMARGIN_RIGHT) >= GetTextArea().m_nViewColNum);
					if( bDrawAfter ){
						SetDrawSwitch( false );
					}
				}
			}
			GetCommander().Command_GOLINEEND( TRUE, FALSE );
			GetCommander().Command_RIGHT( true, false, false );
			if( bDrawSwitchOld && bDrawAfter ){
				SetDrawSwitch( true );
				Redraw();
			}

			//	Apr. 14, 2003 genta
			//	�s�ԍ��̉����N���b�N���ăh���b�O���J�n����Ƃ��������Ȃ�̂��C��
			//	�s�ԍ����N���b�N�����ꍇ�ɂ�GetSelectionInfo().ChangeSelectAreaByCurrentCursor()�ɂ�
			//	GetSelectionInfo().m_sSelect.GetTo().x/GetSelectionInfo().m_sSelect.GetTo().y��-1���ݒ肳��邪�A���
			//	GetCommander().Command_GOLINEEND(), Command_RIGHT()�ɂ���čs�I�����s����B
			//	�������L�����b�g�������ɂ���ꍇ�ɂ̓L�����b�g���ړ����Ȃ��̂�
			//	GetSelectionInfo().m_sSelect.GetTo().x/GetSelectionInfo().m_sSelect.GetTo().y��-1�̂܂܎c���Ă��܂��A���ꂪ
			//	���_�ɐݒ肳��邽�߂ɂ��������Ȃ��Ă����B
			//	�Ȃ̂ŁA�͈͑I�����s���Ă��Ȃ��ꍇ�͋N�_�����̐ݒ���s��Ȃ��悤�ɂ���
			if( GetSelectionInfo().IsTextSelected() ){
				GetSelectionInfo().m_sSelectBgn.SetTo( GetSelectionInfo().m_sSelect.GetTo() );
			}
		}
		else{
			/* URL���N���b�N���ꂽ��I�����邩 */
			//	Sep. 7, 2003 genta URL�̋����\��OFF�̎���URL�͕��ʂ̕����Ƃ��Ĉ���
			if( CTypeSupport(this,COLORIDX_URL).IsDisp() &&
				TRUE == GetDllShareData().m_Common.m_sEdit.m_bSelectClickedURL ){

				CLogicRange cUrlRange;	//URL�͈�
				// �J�[�\���ʒu��URL���L��ꍇ�̂��͈̔͂𒲂ׂ�
				bool bIsUrl = IsCurrentPositionURL(
					GetCaret().GetCaretLayoutPos(),	// �J�[�\���ʒu
					&cUrlRange,						// URL�͈�
					NULL							// URL�󂯎���
				);
				if( bIsUrl ){
					/* ���݂̑I��͈͂��I����Ԃɖ߂� */
					GetSelectionInfo().DisableSelectArea( TRUE );

					/*
					  �J�[�\���ʒu�ϊ�
					  �����ʒu(�s������̃o�C�g���A�܂�Ԃ������s�ʒu)
					  �����C�A�E�g�ʒu(�s������̕\�����ʒu�A�܂�Ԃ�����s�ʒu)
						2002/04/08 YAZAKI �����ł��킩��₷���B
					*/
					CLayoutRange sRangeB;
					m_pcEditDoc->m_cLayoutMgr.LogicToLayout( cUrlRange, &sRangeB );
					/*
					m_pcEditDoc->m_cLayoutMgr.LogicToLayout( CLogicPoint(nUrlIdxBgn          , nUrlLine), sRangeB.GetFromPointer() );
					m_pcEditDoc->m_cLayoutMgr.LogicToLayout( CLogicPoint(nUrlIdxBgn + nUrlLen, nUrlLine), sRangeB.GetToPointer() );
					*/

					GetSelectionInfo().m_sSelectBgn = sRangeB;
					GetSelectionInfo().m_sSelect = sRangeB;

					/* �I��̈�`�� */
					GetSelectionInfo().DrawSelectArea();
				}
			}
		}
	}
}


/*!	�g���v���N���b�N�̃`�F�b�N
	@brief �g���v���N���b�N�𔻒肷��
	
	2��ڂ̃N���b�N����3��ڂ̃N���b�N�܂ł̎��Ԃ��_�u���N���b�N���Ԉȓ��ŁA
	�����̎��̃N���b�N�ʒu�̂��ꂪ�V�X�e�����g���b�N�iX:SM_CXDOUBLECLK,
	Y:SM_CYDOUBLECLK�j�̒l�i�s�N�Z���j�ȉ��̎��g���v���N���b�N�Ƃ���B
	
	@param[in] xPos		�}�E�X�N���b�NX���W
	@param[in] yPos		�}�E�X�N���b�NY���W
	@return		�g���v���N���b�N�̎���TRUE��Ԃ�
	�g���v���N���b�N�łȂ�����FALSE��Ԃ�

	@note	m_dwTripleClickCheck��0�łȂ����Ƀ`�F�b�N���[�h�Ɣ��肷�邪�APC��
			�A���ғ����Ă���ꍇ49.7�����ɃJ�E���^��0�ɂȂ�ׁA�킸���ȉ\��
			�ł��邪�g���v���N���b�N������ł��Ȃ���������B
			�s�ԍ��\���G���A�̃g���v���N���b�N�͒ʏ�N���b�N�Ƃ��Ĉ����B
	
	@date 2007.11.15 nasukoji	�V�K�쐬
*/
BOOL CEditView::CheckTripleClick( CMyPoint ptMouse )
{

	// �g���v���N���b�N�`�F�b�N�L���łȂ��i�������Z�b�g����Ă��Ȃ��j
	if(! m_dwTripleClickCheck)
		return FALSE;

	BOOL result = FALSE;

	// �O��N���b�N�Ƃ̃N���b�N�ʒu�̂�����Z�o
	CMyPoint dpos( GetSelectionInfo().m_ptMouseRollPosOld.x - ptMouse.x,
				   GetSelectionInfo().m_ptMouseRollPosOld.y - ptMouse.y );

	if(dpos.x < 0)
		dpos.x = -dpos.x;	// ��Βl��

	if(dpos.y < 0)
		dpos.y = -dpos.y;	// ��Βl��

	// �s�ԍ��\���G���A�łȂ��A���N���b�N�v���X����_�u���N���b�N���Ԉȓ��A
	// ���_�u���N���b�N�̋��e����s�N�Z���ȉ��̂���̎��g���v���N���b�N�Ƃ���
	//	2007.10.12 genta/dskoba �V�X�e���̃_�u���N���b�N���x�C���ꋖ�e�ʂ��擾
	if( (ptMouse.x >= GetTextArea().GetAreaLeft())&&
		(::GetTickCount() - m_dwTripleClickCheck <= GetDoubleClickTime() )&&
		(dpos.x <= GetSystemMetrics(SM_CXDOUBLECLK) ) &&
		(dpos.y <= GetSystemMetrics(SM_CYDOUBLECLK)))
	{
		result = TRUE;
	}else{
		m_dwTripleClickCheck = 0;	// �g���v���N���b�N�`�F�b�N OFF
	}
	
	return result;
}

/* �}�E�X�E�{�^������ */
void CEditView::OnRBUTTONDOWN( WPARAM fwKeys, int xPos , int yPos )
{
	/* ���݂̃}�E�X�J�[�\���ʒu�����C�A�E�g�ʒu */

	CLayoutPoint ptNew;
	GetTextArea().ClientToLayout(CMyPoint(xPos,yPos), &ptNew);
	/*
	ptNew.x = GetTextArea().GetViewLeftCol() + (xPos - GetTextArea().GetAreaLeft()) / GetTextMetrics().GetHankakuDx();
	ptNew.y = GetTextArea().GetViewTopLine() + (yPos - GetTextArea().GetAreaTop()) / GetTextMetrics().GetHankakuDy();
	*/
	/* �w��J�[�\���ʒu���I���G���A���ɂ��邩 */
	if( 0 == IsCurrentPositionSelected(
		ptNew		// �J�[�\���ʒu
		)
	){
		return;
	}
	OnLBUTTONDOWN( fwKeys, xPos , yPos );
	return;
}

/* �}�E�X�E�{�^���J�� */
void CEditView::OnRBUTTONUP( WPARAM fwKeys, int xPos , int yPos )
{
	if( GetSelectionInfo().IsMouseSelecting() ){	/* �͈͑I�� */
		/* �}�E�X���{�^���J���̃��b�Z�[�W���� */
		OnLBUTTONUP( fwKeys, xPos, yPos );
	}


	int		nIdx;
	int		nFuncID;
// novice 2004/10/10
	/* Shift,Ctrl,Alt�L�[��������Ă����� */
	nIdx = getCtrlKeyState();
	/* �}�E�X�E�N���b�N�ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[1]�ɓ����Ă��� */
	nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_RIGHT].m_nFuncCodeArr[nIdx];
	if( nFuncID != 0 ){
		/* �R�}���h�R�[�h�ɂ�鏈���U�蕪�� */
		//	May 19, 2006 genta �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
		::PostMessageCmd( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ),  (LPARAM)NULL );
	}
//	/* �E�N���b�N���j���[ */
//	GetCommander().Command_MENU_RBUTTON();
	return;
}


// novice 2004/10/11 �}�E�X���{�^���Ή�
/*!
	�}�E�X���{�^�����������Ƃ��̏���

	@param fwKeys [in] first message parameter
	@param xPos [in] �}�E�X�J�[�\��X���W
	@param yPos [in] �}�E�X�J�[�\��Y���W
	@date 2004.10.11 novice �V�K�쐬
*/
void CEditView::OnMBUTTONDOWN( WPARAM fwKeys, int xPos , int yPos )
{
	int		nIdx;
	int		nFuncID;

	/* Shift,Ctrl,Alt�L�[��������Ă����� */
	nIdx = getCtrlKeyState();
	/* �}�E�X���T�C�h�{�^���ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[2]�ɓ����Ă��� */
	nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_CENTER].m_nFuncCodeArr[nIdx];
	if( nFuncID != 0 ){
		/* �R�}���h�R�[�h�ɂ�鏈���U�蕪�� */
		//	May 19, 2006 genta �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
		::PostMessageCmd( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ),  (LPARAM)NULL );
	}
}


// novice 2004/10/10 �}�E�X�T�C�h�{�^���Ή�
/*!
	�}�E�X���T�C�h�{�^�����������Ƃ��̏���

	@param fwKeys [in] first message parameter
	@param xPos [in] �}�E�X�J�[�\��X���W
	@param yPos [in] �}�E�X�J�[�\��Y���W
	@date 2004.10.10 novice �V�K�쐬
	@date 2004.10.11 novice �}�E�X���{�^���Ή��̂��ߕύX
*/
void CEditView::OnXLBUTTONDOWN( WPARAM fwKeys, int xPos , int yPos )
{
	int		nIdx;
	int		nFuncID;

	/* Shift,Ctrl,Alt�L�[��������Ă����� */
	nIdx = getCtrlKeyState();
	/* �}�E�X���T�C�h�{�^���ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[3]�ɓ����Ă��� */
	nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_LEFTSIDE].m_nFuncCodeArr[nIdx];
	if( nFuncID != 0 ){
		/* �R�}���h�R�[�h�ɂ�鏈���U�蕪�� */
		//	May 19, 2006 genta �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
		::PostMessageCmd( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ),  (LPARAM)NULL );
	}

	return;
}


/*!
	�}�E�X�E�T�C�h�{�^���������Ƃ��̏���

	@param fwKeys [in] first message parameter
	@param xPos [in] �}�E�X�J�[�\��X���W
	@param yPos [in] �}�E�X�J�[�\��Y���W
	@date 2004.10.10 novice �V�K�쐬
	@date 2004.10.11 novice �}�E�X���{�^���Ή��̂��ߕύX
*/
void CEditView::OnXRBUTTONDOWN( WPARAM fwKeys, int xPos , int yPos )
{
	int		nIdx;
	int		nFuncID;

	/* Shift,Ctrl,Alt�L�[��������Ă����� */
	nIdx = getCtrlKeyState();
	/* �}�E�X�E�T�C�h�{�^���ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[4]�ɓ����Ă��� */
	nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_RIGHTSIDE].m_nFuncCodeArr[nIdx];
	if( nFuncID != 0 ){
		/* �R�}���h�R�[�h�ɂ�鏈���U�蕪�� */
		//	May 19, 2006 genta �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
		::PostMessageCmd( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ),  (LPARAM)NULL );
	}

	return;
}

/* �}�E�X�ړ��̃��b�Z�[�W���� */
void CEditView::OnMOUSEMOVE( WPARAM fwKeys, int _xPos , int _yPos )
{
	CMyPoint ptMouse(_xPos,_yPos);

	CLayoutInt	nScrollRowNum;
	POINT		po;
	const wchar_t*	pLine;
	CLogicInt		nLineLen;

	CLogicInt	nIdx;
	int			nWorkF;
	int			nWorkT;

	CLayoutRange sRange;
	CLayoutRange sSelectBgn_Old; // �͈͑I��(���_)
	CLayoutRange sSelect_Old;
	CLayoutRange sSelect;

	sSelectBgn_Old = GetSelectionInfo().m_sSelectBgn;
	sSelect_Old    = GetSelectionInfo().m_sSelect;

	if( !GetSelectionInfo().IsMouseSelecting() ){	/* �͈͑I�� */
		::GetCursorPos( &po );
		//	2001/06/18 asa-o: �⊮�E�B���h�E���\������Ă��Ȃ�
		if(!m_bHokan){
			/* ����Tip���N������Ă��� */
			if( 0 == m_dwTipTimer ){
				if( (m_poTipCurPos.x != po.x || m_poTipCurPos.y != po.y ) ){
					/* ����Tip������ */
					m_cTipWnd.Hide();
					m_dwTipTimer = ::GetTickCount();	/* ����Tip�N���^�C�}�[ */
				}
			}else{
				m_dwTipTimer = ::GetTickCount();		/* ����Tip�N���^�C�}�[ */
			}
		}
		/* ���݂̃}�E�X�J�[�\���ʒu�����C�A�E�g�ʒu */
		CLayoutPoint ptNew;
		GetTextArea().ClientToLayout(ptMouse, &ptNew);

		CLogicRange	cUrlRange;	//URL�͈�

		/* �I���e�L�X�g�̃h���b�O���� */
		if( m_bDragMode ){
			if( GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DragDrop ){	/* OLE�ɂ��h���b�O & �h���b�v���g�� */
				/* ���W�w��ɂ��J�[�\���ړ� */
				nScrollRowNum = GetCaret().MoveCursorToClientPoint( ptMouse );
			}
		}
		else{
			/* �s�I���G���A? */
			if( ptMouse.x < GetTextArea().GetAreaLeft() || ptMouse.y < GetTextArea().GetAreaTop() ){	//	2002/2/10 aroka
				/* ���J�[�\�� */
				if( ptMouse.y >= GetTextArea().GetAreaTop() )
					::SetCursor( ::LoadCursor( G_AppInstance(), MAKEINTRESOURCE( IDC_CURSOR_RVARROW ) ) );
				else
					::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );
			}
			else if( GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DragDrop	/* OLE�ɂ��h���b�O & �h���b�v���g�� */
			 && GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DropSource /* OLE�ɂ��h���b�O���ɂ��邩 */
			 && 0 == IsCurrentPositionSelected(						/* �w��J�[�\���ʒu���I���G���A���ɂ��邩 */
				ptNew	// �J�[�\���ʒu
				)
			){
				/* ���J�[�\�� */
				::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );
			}
			/* �J�[�\���ʒu��URL���L��ꍇ */
			//	Sep. 7, 2003 genta URL�̋����\��OFF�̎���URL�`�F�b�N���s��Ȃ�
			else if( CTypeSupport(this,COLORIDX_URL).IsDisp() &&
				IsCurrentPositionURL(
					ptNew,			// �J�[�\���ʒu
					&cUrlRange,		// URL�͈�
					NULL			// URL�󂯎���
				)
			){
				/* ��J�[�\�� */
				::SetCursor( ::LoadCursor( G_AppInstance(), MAKEINTRESOURCE( IDC_CURSOR_HAND ) ) );
			}else{
				//migemo isearch 2004.10.22
				if( m_nISearchMode > 0 ){
					if (m_nISearchDirection == 1){
						::SetCursor( ::LoadCursor( G_AppInstance(),MAKEINTRESOURCE(IDC_CURSOR_ISEARCH_F)));
					}else{
						::SetCursor( ::LoadCursor( G_AppInstance(),MAKEINTRESOURCE(IDC_CURSOR_ISEARCH_B)));
					}
				}else
				/* �A�C�r�[�� */
				::SetCursor( ::LoadCursor( NULL, IDC_IBEAM ) );
			}
		}
		return;
	}
	::SetCursor( ::LoadCursor( NULL, IDC_IBEAM ) );
	if( GetSelectionInfo().IsBoxSelecting() ){	/* ��`�͈͑I�� */
		/* ���W�w��ɂ��J�[�\���ړ� */
		nScrollRowNum = GetCaret().MoveCursorToClientPoint( ptMouse );
		/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
		GetSelectionInfo().ChangeSelectAreaByCurrentCursor( GetCaret().GetCaretLayoutPos() );
		GetSelectionInfo().m_ptMouseRollPosOld = ptMouse; // �}�E�X�͈͑I��O��ʒu(XY���W)
	}
	else{
		/* ���W�w��ɂ��J�[�\���ړ� */
		if(( ptMouse.x < GetTextArea().GetAreaLeft() || m_dwTripleClickCheck )&& GetSelectionInfo().m_bBeginLineSelect ){	// �s�P�ʑI��
			// 2007.11.15 nasukoji	������̍s�I�������}�E�X�J�[�\���̈ʒu�̍s���I�������悤�ɂ���
			CMyPoint nNewPos(0, ptMouse.y);

			// 1�s�̍���
			int nLineHeight = GetTextMetrics().GetHankakuHeight() + m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_nLineSpace;

			// �I���J�n�s�ȉ��ւ̃h���b�O����1�s���ɃJ�[�\�����ړ�����
			if( GetTextArea().GetViewTopLine() + (ptMouse.y - GetTextArea().GetAreaTop()) / nLineHeight >= GetSelectionInfo().m_sSelectBgn.GetTo().y)
				nNewPos.y += nLineHeight;

			// �J�[�\�����ړ�
			nNewPos.x = GetTextArea().GetAreaLeft() - Int(GetTextArea().GetViewLeftCol()) * ( GetTextMetrics().GetHankakuWidth() + m_pcEditDoc->m_cDocType.GetDocumentAttribute().m_nColmSpace );
			nScrollRowNum = GetCaret().MoveCursorToClientPoint( nNewPos );

			// 2.5�N���b�N�ɂ��s�P�ʂ̃h���b�O
			if( m_dwTripleClickCheck ){
				// �I���J�n�s�ȏ�Ƀh���b�O����
				if( GetCaret().GetCaretLayoutPos().GetY() <= GetSelectionInfo().m_sSelectBgn.GetTo().y ){
					GetCommander().Command_GOLINETOP( TRUE, 0x09 );		// ���s�P�ʂ̍s���ֈړ�
				}else{
					CLayoutPoint ptCaret;

					CLogicPoint ptCaretPrevLog(0, GetCaret().GetCaretLogicPos().y);

					// �I���J�n�s��艺�ɃJ�[�\�������鎞��1�s�O�ƕ����s�ԍ��̈Ⴂ���`�F�b�N����
					// �I���J�n�s�ɃJ�[�\�������鎞�̓`�F�b�N�s�v
					if( GetCaret().GetCaretLayoutPos().GetY() > GetSelectionInfo().m_sSelectBgn.GetTo().y ){
						// 1�s�O�̕����s���擾����
						m_pcEditDoc->m_cLayoutMgr.LayoutToLogic( CLayoutPoint(CLayoutInt(0), GetCaret().GetCaretLayoutPos().GetY() - 1), &ptCaretPrevLog );
					}

					// �O�̍s�Ɠ��������s
					if( ptCaretPrevLog.y == GetCaret().GetCaretLogicPos().y ){
						// 1�s��̕����s���烌�C�A�E�g�s�����߂�
						m_pcEditDoc->m_cLayoutMgr.LogicToLayout( CLogicPoint(0, GetCaret().GetCaretLogicPos().y + 1), &ptCaret );

						// �J�[�\�������̕����s���ֈړ�����
						nScrollRowNum = GetCaret().MoveCursor( ptCaret, TRUE );
					}
				}
			}
		}else{
			nScrollRowNum = GetCaret().MoveCursorToClientPoint( ptMouse );
		}
		GetSelectionInfo().m_ptMouseRollPosOld = ptMouse; // �}�E�X�͈͑I��O��ʒu(XY���W)

		/* CTRL�L�[��������Ă����� */
//		if( GetKeyState_Control() ){
		if( !GetSelectionInfo().m_bBeginWordSelect ){
			/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
			GetSelectionInfo().ChangeSelectAreaByCurrentCursor( GetCaret().GetCaretLayoutPos() );
		}else{
			/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX(�e�X�g�̂�) */
			GetSelectionInfo().ChangeSelectAreaByCurrentCursorTEST(
				GetCaret().GetCaretLayoutPos(),
				&sSelect
			);
			/* �I��͈͂ɕύX�Ȃ� */
			if( sSelect_Old == sSelect ){
				GetSelectionInfo().ChangeSelectAreaByCurrentCursor(
					GetCaret().GetCaretLayoutPos()
				);
				return;
			}
			const CLayout* pcLayout;
			if( NULL != ( pLine = m_pcEditDoc->m_cLayoutMgr.GetLineStr( GetCaret().GetCaretLayoutPos().GetY2(), &nLineLen, &pcLayout ) ) ){
				nIdx = LineColmnToIndex( pcLayout, GetCaret().GetCaretLayoutPos().GetX2() );
				/* ���݈ʒu�̒P��͈̔͂𒲂ׂ� */
				int nResult=m_pcEditDoc->m_cLayoutMgr.WhereCurrentWord(
					GetCaret().GetCaretLayoutPos().GetY2(),
					nIdx,
					&sRange,
					NULL,
					NULL
				);
				if( nResult ){
					// �w�肳�ꂽ�s�̃f�[�^���̈ʒu�ɑΉ����錅�̈ʒu�𒲂ׂ�
					// 2007.10.15 kobake ���Ƀ��C�A�E�g�P�ʂȂ̂ŕϊ��͕s�v
					/*
					pLine     = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetFrom().GetY2(), &nLineLen, &pcLayout );
					sRange.SetFromX( LineIndexToColmn( pcLayout, sRange.GetFrom().x ) );
					pLine     = m_pcEditDoc->m_cLayoutMgr.GetLineStr( sRange.GetTo().GetY2(), &nLineLen, &pcLayout );
					sRange.SetToX( LineIndexToColmn( pcLayout, sRange.GetTo().x ) );
					*/

					nWorkF = IsCurrentPositionSelectedTEST(
						sRange.GetFrom(), //�J�[�\���ʒu
						sSelect
					);
					nWorkT = IsCurrentPositionSelectedTEST(
						sRange.GetTo(),	// �J�[�\���ʒu
						sSelect
					);
					if( -1 == nWorkF ){
						/* �n�_���O���Ɉړ��B���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
						GetSelectionInfo().ChangeSelectAreaByCurrentCursor( sRange.GetFrom() );
					}
					else if( 1 == nWorkT ){
						/* �I�_������Ɉړ��B���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
						GetSelectionInfo().ChangeSelectAreaByCurrentCursor( sRange.GetTo() );
					}
					else if( sSelect_Old.GetFrom() == sSelect.GetFrom() ){
						/* �n�_�����ύX���O���ɏk�����ꂽ */
						/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
						GetSelectionInfo().ChangeSelectAreaByCurrentCursor( sRange.GetTo() );
					}
					else if( sSelect_Old.GetTo()==sSelect.GetTo() ){
						/* �I�_�����ύX������ɏk�����ꂽ */
						/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
						GetSelectionInfo().ChangeSelectAreaByCurrentCursor( sRange.GetFrom() );
					}
				}else{
					/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
					GetSelectionInfo().ChangeSelectAreaByCurrentCursor( GetCaret().GetCaretLayoutPos() );
				}
			}else{
				/* ���݂̃J�[�\���ʒu�ɂ���đI��͈͂�ύX */
				GetSelectionInfo().ChangeSelectAreaByCurrentCursor( GetCaret().GetCaretLayoutPos() );
			}
		}
	}
	return;
}
//m_dwTipTimerm_dwTipTimerm_dwTipTimer




/* �}�E�X�z�C�[���̃��b�Z�[�W���� */
LRESULT CEditView::OnMOUSEWHEEL( WPARAM wParam, LPARAM lParam )
{
	WORD	fwKeys;
	short	zDelta;
	short	xPos;
	short	yPos;
	int		i;
	int		nScrollCode;
	int		nRollLineNum;

	fwKeys = LOWORD(wParam);			// key flags
	zDelta = (short) HIWORD(wParam);	// wheel rotation
	xPos = (short) LOWORD(lParam);		// horizontal position of pointer
	yPos = (short) HIWORD(lParam);		// vertical position of pointer
//	MYTRACE_A( "CEditView::DispatchEvent() WM_MOUSEWHEEL fwKeys=%xh zDelta=%d xPos=%d yPos=%d \n", fwKeys, zDelta, xPos, yPos );

	if( 0 < zDelta ){
		nScrollCode = SB_LINEUP;
	}else{
		nScrollCode = SB_LINEDOWN;
	}

	/* �}�E�X�z�C�[���ɂ��X�N���[���s�������W�X�g������擾 */
	nRollLineNum = 6;

	/* ���W�X�g���̑��݃`�F�b�N */
	// 2006.06.03 Moca ReadRegistry �ɏ�������
	unsigned int uDataLen;	// size of value data
	TCHAR szValStr[256];
	uDataLen = _countof(szValStr) - 1;
	if( ReadRegistry( HKEY_CURRENT_USER, _T("Control Panel\\desktop"), _T("WheelScrollLines"), szValStr, uDataLen ) ){
		nRollLineNum = ::_ttoi( szValStr );
	}

	if( -1 == nRollLineNum ){/* �u1��ʕ��X�N���[������v */
		nRollLineNum = (Int)GetTextArea().m_nViewRowNum;	// �\����̍s��
	}
	else{
		if( nRollLineNum < 1 ){
			nRollLineNum = 1;
		}
		if( nRollLineNum > 30 ){	//@@@ YAZAKI 2001.12.31 10��30�ցB
			nRollLineNum = 30;
		}
	}
	for( i = 0; i < nRollLineNum; ++i ){
		//	Sep. 11, 2004 genta �����X�N���[���s��
		CLayoutInt line;

		if( nScrollCode == SB_LINEUP ){
			line = ScrollAtV( GetTextArea().GetViewTopLine() - CLayoutInt(1) );
		}else{
			line = ScrollAtV( GetTextArea().GetViewTopLine() + CLayoutInt(1) );
		}
		SyncScrollV( line );
	}
	return 0;
}






/* �}�E�X���{�^���J���̃��b�Z�[�W���� */
void CEditView::OnLBUTTONUP( WPARAM fwKeys, int xPos , int yPos )
{
//	MYTRACE_A( "OnLBUTTONUP()\n" );
	CMemory		cmemBuf, cmemClip;

	/* �͈͑I���I�� & �}�E�X�L���v�`���[����� */
	if( GetSelectionInfo().IsMouseSelecting() ){	/* �͈͑I�� */
		/* �}�E�X �L���v�`������� */
		::ReleaseCapture();
		GetCaret().ShowCaret_( GetHwnd() ); // 2002/07/22 novice

		GetSelectionInfo().SelectEnd();

		if( GetSelectionInfo().m_sSelect.IsOne() ){
			/* ���݂̑I��͈͂��I����Ԃɖ߂� */
			GetSelectionInfo().DisableSelectArea( TRUE );

			// �Ί��ʂ̋����\��	// 2007.10.18 ryoji
			DrawBracketPair( false );
			SetBracketPairPos( true );
			DrawBracketPair( true );
		}
		GetCaret().m_cUnderLine.UnLock();
	}
	return;
}





// �}�E�X���{�^���_�u���N���b�N
// 2007.01.18 kobake IsCurrentPositionURL�d�l�ύX�ɔ����A�����̏�������
void CEditView::OnLBUTTONDBLCLK( WPARAM fwKeys, int _xPos , int _yPos )
{
	CMyPoint ptMouse(_xPos,_yPos);

	CLogicRange		cUrlRange;	// URL�͈�
	std::wstring	wstrURL;
	const wchar_t*	pszMailTo = L"mailto:";

	// 2007.10.06 nasukoji	�N�A�h���v���N���b�N���̓`�F�b�N���Ȃ�
	if(! m_dwTripleClickCheck){
		/* �J�[�\���ʒu��URL���L��ꍇ�̂��͈̔͂𒲂ׂ� */
		//	Sep. 7, 2003 genta URL�̋����\��OFF�̎���URL�`�F�b�N���s��Ȃ�
		if( CTypeSupport(this,COLORIDX_URL).IsDisp()
			&&
			IsCurrentPositionURL(
				GetCaret().GetCaretLayoutPos(),	// �J�[�\���ʒu
				&cUrlRange,				// URL�͈�
				&wstrURL				// URL�󂯎���
			)
		){
			std::wstring wstrOPEN;

			// URL���J��
		 	// ���݈ʒu�����[���A�h���X�Ȃ�΁ANULL�ȊO�ƁA���̒�����Ԃ�
			if( IsMailAddress( wstrURL.c_str(), wstrURL.length(), NULL ) ){
				wstrOPEN = pszMailTo + wstrURL;
			}
			else{
				if( wcsnicmp( wstrURL.c_str(), L"ttp://", 6 ) == 0 ){	//�}�~URL
					wstrOPEN = L"h" + wstrURL;
				}
				else if( wcsnicmp( wstrURL.c_str(), L"tp://", 5 ) == 0 ){	//�}�~URL
					wstrOPEN = L"ht" + wstrURL;
				}
				else{
					wstrOPEN = wstrURL;
				}
			}
			::ShellExecute( NULL, _T("open"), to_tchar(wstrOPEN.c_str()), NULL, NULL, SW_SHOW );
			return;
		}

		/* GREP�o�̓��[�h�܂��̓f�o�b�O���[�h ���� �}�E�X���{�^���_�u���N���b�N�Ń^�O�W�����v �̏ꍇ */
		//	2004.09.20 naoh �O���R�}���h�̏o�͂���Tagjump�ł���悤��
		if( (CEditApp::Instance()->m_pcGrepAgent->m_bGrepMode || CAppMode::Instance()->IsDebugMode()) && GetDllShareData().m_Common.m_sSearch.m_bGTJW_LDBLCLK ){
			/* �^�O�W�����v�@�\ */
			GetCommander().Command_TAGJUMP();
			return;
		}
	}

// novice 2004/10/10
	/* Shift,Ctrl,Alt�L�[��������Ă����� */
	int	nIdx = getCtrlKeyState();

	/* �}�E�X���N���b�N�ɑΉ�����@�\�R�[�h��m_Common.m_pKeyNameArr[?]�ɓ����Ă��� 2007.11.15 nasukoji */
	EFunctionCode	nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[
		m_dwTripleClickCheck ? MOUSEFUNCTION_QUADCLICK : MOUSEFUNCTION_DOUBLECLICK
	].m_nFuncCodeArr[nIdx];
	if(m_dwTripleClickCheck){
		// ��I����Ԃɂ����㍶�N���b�N�������Ƃɂ���
		// ���ׂđI���̏ꍇ�́A3.5�N���b�N���̑I����ԕێ��ƃh���b�O�J�n����
		// �͈͕ύX�̂��߁B
		// �N�A�h���v���N���b�N�@�\�����蓖�Ă��Ă��Ȃ��ꍇ�́A�_�u���N���b�N
		// �Ƃ��ď������邽�߁B
		if( GetSelectionInfo().IsTextSelected() )		// �e�L�X�g���I������Ă��邩
			GetSelectionInfo().DisableSelectArea( TRUE );		// ���݂̑I��͈͂��I����Ԃɖ߂�

		if(! nFuncID){
			m_dwTripleClickCheck = 0;	// �g���v���N���b�N�`�F�b�N OFF
			nFuncID = GetDllShareData().m_Common.m_sKeyBind.m_pKeyNameArr[MOUSEFUNCTION_DOUBLECLICK].m_nFuncCodeArr[nIdx];
			OnLBUTTONDOWN( fwKeys, ptMouse.x , ptMouse.y );	// �J�[�\�����N���b�N�ʒu�ֈړ�����
		}
	}

	if( nFuncID != 0 ){
		/* �R�}���h�R�[�h�ɂ�鏈���U�蕪�� */
		//	May 19, 2006 genta �}�E�X����̃��b�Z�[�W��CMD_FROM_MOUSE����ʃr�b�g�ɓ���đ���
		::SendMessageCmd( ::GetParent( m_hwndParent ), WM_COMMAND, MAKELONG( nFuncID, CMD_FROM_MOUSE ),  (LPARAM)NULL );
	}

	// 2007.10.06 nasukoji	�N�A�h���v���N���b�N���������Ŕ�����
	if(m_dwTripleClickCheck){
		m_dwTripleClickCheck = 0;	// �g���v���N���b�N�`�F�b�N OFF�i����͒ʏ�N���b�N�j
		return;
	}

	// 2007.11.06 nasukoji	�_�u���N���b�N���P��I���łȂ��Ă��g���v���N���b�N��L���Ƃ���
	// 2007.10.02 nasukoji	�g���v���N���b�N�`�F�b�N�p�Ɏ������擾
	m_dwTripleClickCheck = ::GetTickCount();

	// �_�u���N���b�N�ʒu�Ƃ��ċL��
	GetSelectionInfo().m_ptMouseRollPosOld = ptMouse;	// �}�E�X�͈͑I��O��ʒu(XY���W)

	/*	2007.07.09 maru �@�\�R�[�h�̔����ǉ�
		�_�u���N���b�N����̃h���b�O�ł͒P��P�ʂ͈̔͑I��(�G�f�B�^�̈�ʓI����)�ɂȂ邪
		���̓���́A�_�u���N���b�N���P��I����O��Ƃ������́B
		�L�[���蓖�Ă̕ύX�ɂ��A�_�u���N���b�N���P��I���̂Ƃ��ɂ� GetSelectionInfo().m_bBeginWordSelect = TRUE
		�ɂ���ƁA�����̓��e�ɂ���Ă͕\�������������Ȃ�̂ŁA�����Ŕ�����悤�ɂ���B
	*/
	if(F_SELECTWORD != nFuncID) return;

	/* �͈͑I���J�n & �}�E�X�L���v�`���[ */
	GetSelectionInfo().SelectBeginWord();

	if( GetDllShareData().m_Common.m_sView.m_bFontIs_FIXED_PITCH ){	/* ���݂̃t�H���g�͌Œ蕝�t�H���g�ł��� */
		/* ALT�L�[��������Ă����� */
		if( GetKeyState_Alt() ){
			GetSelectionInfo().SetBoxSelect(true);	/* ��`�͈͑I�� */
		}
	}
	::SetCapture( GetHwnd() );
	GetCaret().HideCaret_( GetHwnd() ); // 2002/07/22 novice
	if( GetSelectionInfo().IsTextSelected() ){
		/* �펞�I��͈͈͂̔� */
		GetSelectionInfo().m_sSelectBgn.SetTo( GetSelectionInfo().m_sSelect.GetTo() );
	}
	else{
		/* ���݂̃J�[�\���ʒu����I�����J�n���� */
		GetSelectionInfo().BeginSelectArea( );
	}

	return;
}




// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //
//                           D&D                               //
// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- //

STDMETHODIMP CEditView::DragEnter( LPDATAOBJECT pDataObject, DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect )
{
#ifdef _DEBUG
	MYTRACE_A( "CEditView::DragEnter()\n" );
#endif
	//�uOLE�ɂ��h���b�O & �h���b�v���g���v�I�v�V�����������̏ꍇ�ɂ̓h���b�v���󂯕t���Ȃ�
	if(!GetDllShareData().m_Common.m_sEdit.m_bUseOLE_DragDrop)return E_UNEXPECTED;

	//�r���[���[�h�̏ꍇ�̓h���b�v���󂯕t���Ȃ�
	if(CAppMode::Instance()->IsViewMode())return E_UNEXPECTED;

	//�㏑���֎~�̏ꍇ�̓h���b�v���󂯕t���Ȃ�
	if(!m_pcEditDoc->m_cDocLocker.IsDocWritable())return E_UNEXPECTED;


	if( pDataObject == NULL || pdwEffect == NULL )
		return E_INVALIDARG;

	CLIPFORMAT cf;
	cf = GetAvailableClipFormat( pDataObject );
	if( cf == 0 )
		return E_INVALIDARG;

	/* �������A�N�e�B�u�y�C���ɂ��� */
	m_pcEditWnd->SetActivePane( m_nMyIndex );

	// ���݂̃J�[�\���ʒu���L������	// 2007.12.09 ryoji
	m_ptCaretPos_DragEnter = GetCaret().GetCaretLayoutPos();
	m_nCaretPosX_Prev_DragEnter = GetCaret().m_nCaretPosX_Prev;

	// �h���b�O�f�[�^�͋�`��
	m_bDragBoxData = IsDataAvailable( pDataObject, ::RegisterClipboardFormat( _T("MSDEVColumnSelect") ) );

	/* �I���e�L�X�g�̃h���b�O���� */
	_SetDragMode( TRUE );

	DragOver( dwKeyState, pt, pdwEffect );
	return S_OK;
}

STDMETHODIMP CEditView::DragOver( DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect )
{
#ifdef _DEBUG
	MYTRACE_A( "CEditView::DragOver()\n" );
#endif

	/* �}�E�X�ړ��̃��b�Z�[�W���� */
	::ScreenToClient( GetHwnd(), (LPPOINT)&pt );
	OnMOUSEMOVE( dwKeyState, pt.x , pt.y );

	if ( pdwEffect == NULL )
		return E_INVALIDARG;

	*pdwEffect = TranslateDropEffect( dwKeyState, pt, *pdwEffect );

	return S_OK;
}

STDMETHODIMP CEditView::DragLeave( void )
{
#ifdef _DEBUG
	MYTRACE_A( "CEditView::DragLeave()\n" );
#endif
	/* �I���e�L�X�g�̃h���b�O���� */
	_SetDragMode( FALSE );

	// DragEnter���̃J�[�\���ʒu�𕜌�	// 2007.12.09 ryoji
	// ���͈͑I�𒆂̂Ƃ��ɑI��͈͂ƃJ�[�\������������ƕς�����
	GetCaret().MoveCursor( m_ptCaretPos_DragEnter, FALSE );
	GetCaret().m_nCaretPosX_Prev = m_nCaretPosX_Prev_DragEnter;
	RedrawAll();

	// ��A�N�e�B�u���͕\����Ԃ��A�N�e�B�u�ɖ߂�	// 2007.12.09 ryoji
	if( ::GetActiveWindow() == NULL )
		OnKillFocus();

	return S_OK;
}

STDMETHODIMP CEditView::Drop( LPDATAOBJECT pDataObject, DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect )
{
#ifdef _DEBUG
	MYTRACE_A( "CEditView::Drop()\n" );
#endif
	BOOL		bBoxData;
	BOOL		bMove;
	BOOL		bMoveToPrev;
	RECT		rcSel;
	CNativeW	cmemBuf;
	bool		bBeginBoxSelect_Old;

	CLayoutRange sSelectBgn_Old;
	CLayoutRange sSelect_Old;



	/* �I���e�L�X�g�̃h���b�O���� */
	_SetDragMode( FALSE );

	// ��A�N�e�B�u���͕\����Ԃ��A�N�e�B�u�ɖ߂�	// 2007.12.09 ryoji
	if( ::GetActiveWindow() == NULL )
		OnKillFocus();

	if( pDataObject == NULL || pdwEffect == NULL )
		return E_INVALIDARG;

	CLIPFORMAT cf;
	cf = GetAvailableClipFormat( pDataObject );
	if( cf == 0 )
		return E_INVALIDARG;

	*pdwEffect = TranslateDropEffect( dwKeyState, pt, *pdwEffect );
	if( *pdwEffect == DROPEFFECT_NONE )
		return E_INVALIDARG;

	// �O������̃h���b�v�͈Ȍ�̏����ł̓R�s�[�Ɠ��l�Ɉ���
	CEditView* pcDragSourceView = m_pcEditWnd->GetDragSourceView();
	bMove = (*pdwEffect == DROPEFFECT_MOVE) && pcDragSourceView;
	bBoxData = m_bDragBoxData;

	// �h���b�v�f�[�^�̎擾
	HGLOBAL hData = GetGlobalData( pDataObject, cf );
	if (hData == NULL)
		return E_INVALIDARG;
	LPVOID pData = ::GlobalLock( hData );
	SIZE_T nSize = ::GlobalSize( hData );
	if( cf == CClipboard::GetSakuraFormat() ){
		wchar_t* pszData = (wchar_t*)((BYTE*)pData + sizeof(int));
		cmemBuf.SetString( pszData, wcsnlen( pszData, *(int*)pData ) );
	}else if( cf == CF_UNICODETEXT ){
		cmemBuf.SetString( (wchar_t*)pData, wcsnlen( (wchar_t*)pData, nSize / sizeof(wchar_t) ) );
	}else{
		cmemBuf.SetStringOld( (char*)pData, strnlen( (char*)pData, nSize / sizeof(char) ) );
	}

	// �A���h�D�o�b�t�@�̏���
	if( NULL == m_pcOpeBlk ){
		m_pcOpeBlk = new COpeBlk;
	}

	/* �ړ��̏ꍇ�A�ʒu�֌W���Z�o */
	if( bMove ){
		if( bBoxData ){
			/* 2�_��Ίp�Ƃ����`�����߂� */
			TwoPointToRect(
				&rcSel,
				pcDragSourceView->GetSelectionInfo().m_sSelect.GetFrom(),	// �͈͑I���J�n
				pcDragSourceView->GetSelectionInfo().m_sSelect.GetTo()		// �͈͑I���I��
			);
			++rcSel.bottom;
			if( GetCaret().GetCaretLayoutPos().GetY() >= rcSel.bottom ){
				bMoveToPrev = FALSE;
			}else
			if( GetCaret().GetCaretLayoutPos().GetY() + rcSel.bottom - rcSel.top < rcSel.top ){
				bMoveToPrev = TRUE;
			}else
			if( GetCaret().GetCaretLayoutPos().GetX2() < rcSel.left ){
				bMoveToPrev = TRUE;
			}else{
				bMoveToPrev = FALSE;
			}
		}else{
			if( pcDragSourceView->GetSelectionInfo().m_sSelect.GetFrom().y > GetCaret().GetCaretLayoutPos().GetY() ){
				bMoveToPrev = TRUE;
			}else
			if( pcDragSourceView->GetSelectionInfo().m_sSelect.GetFrom().y == GetCaret().GetCaretLayoutPos().GetY() ){
				if( pcDragSourceView->GetSelectionInfo().m_sSelect.GetFrom().x > GetCaret().GetCaretLayoutPos().GetX2() ){
					bMoveToPrev = TRUE;
				}else{
					bMoveToPrev = FALSE;
				}
			}else{
				bMoveToPrev = FALSE;
			}
		}
	}

	CLayoutPoint ptCaretPos_Old = GetCaret().GetCaretLayoutPos();
	if( !bMove ){
		/* �R�s�[���[�h */
		/* ���݂̑I��͈͂��I����Ԃɖ߂� */
		GetSelectionInfo().DisableSelectArea( TRUE );
	}else{
		bBeginBoxSelect_Old = pcDragSourceView->GetSelectionInfo().IsBoxSelecting();
		sSelectBgn_Old = pcDragSourceView->GetSelectionInfo().m_sSelectBgn;
		sSelect_Old = pcDragSourceView->GetSelectionInfo().m_sSelect;
		if( bMoveToPrev ){
			/* �ړ����[�h & �O�Ɉړ� */
			/* �I���G���A���폜 */
			if( this != pcDragSourceView ){
				pcDragSourceView->GetSelectionInfo().DisableSelectArea( TRUE );
				GetSelectionInfo().DisableSelectArea( TRUE );
				GetSelectionInfo().SetBoxSelect( bBeginBoxSelect_Old );
				GetSelectionInfo().m_sSelectBgn = sSelectBgn_Old;
				GetSelectionInfo().m_sSelect = sSelect_Old;
			}
			DeleteData( TRUE );
			GetCaret().MoveCursor( ptCaretPos_Old, TRUE );
		}else{
			/* ���݂̑I��͈͂��I����Ԃɖ߂� */
			pcDragSourceView->GetSelectionInfo().DisableSelectArea( TRUE );
			if( this != pcDragSourceView )
				GetSelectionInfo().DisableSelectArea( TRUE );
		}
	}
	if( !bBoxData ){	/* ��`�f�[�^ */
		//	2004,05.14 Moca �����ɕ����񒷂�ǉ�

		// �}���O�̃L�����b�g�ʒu���L������
		// �i�L�����b�g���s�I�[���E�̏ꍇ�͖��ߍ��܂��󔒕��������ʒu���V�t�g�j
		CLogicPoint ptCaretLogicPos_Old = GetCaret().GetCaretLogicPos();
		const CLayout* pcLayout;
		CLogicInt nLineLen;
		CLayoutPoint ptCaretLayoutPos_Old = GetCaret().GetCaretLayoutPos();
		if( m_pcEditDoc->m_cLayoutMgr.GetLineStr( ptCaretLayoutPos_Old.GetY2(), &nLineLen, &pcLayout ) ){
			CLayoutInt nLineAllColLen;
			LineColmnToIndex2( pcLayout, ptCaretLayoutPos_Old.GetX2(), &nLineAllColLen );
			if( nLineAllColLen > CLayoutInt(0) ){	// �s�I�[���E�̏ꍇ�ɂ� nLineAllColLen �ɍs�S�̂̕\�������������Ă���
				ptCaretLogicPos_Old.SetX(
					ptCaretLogicPos_Old.GetX2()
					+ (Int)(ptCaretLayoutPos_Old.GetX2() - nLineAllColLen)
				);
			}
		}

		GetCommander().Command_INSTEXT( TRUE, cmemBuf.GetStringPtr(), cmemBuf.GetStringLength(), FALSE );

		// �}���O�̃L�����b�g�ʒu����}����̃L�����b�g�ʒu�܂ł�I��͈͂ɂ���
		CLayoutPoint ptSelectFrom;
		m_pcEditDoc->m_cLayoutMgr.LogicToLayout(
			ptCaretLogicPos_Old,
			&ptSelectFrom
		);
		GetSelectionInfo().m_sSelect.SetFrom( ptSelectFrom );
		GetSelectionInfo().m_sSelect.SetTo( GetCaret().GetCaretLayoutPos() );
	}else{
		// 2004.07.12 Moca �N���b�v�{�[�h�����������Ȃ��悤��
		// TRUE == bBoxSelected
		// FALSE == GetSelectionInfo().IsBoxSelecting()
		/* �\��t���i�N���b�v�{�[�h����\��t���j*/
		GetCommander().Command_PASTEBOX( cmemBuf.GetStringPtr(), cmemBuf.GetStringLength() );
		AdjustScrollBars(); // 2007.07.22 ryoji
		Redraw();
	}
	if( bMove ){
		if( bMoveToPrev ){
		}else{
			/* �ړ����[�h & ���Ɉړ�*/

			// ���݂̑I��͈͂��L������	// 2008.03.26 ryoji
			CLogicRange sSelLogic;
			m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
				GetSelectionInfo().m_sSelect,
				&sSelLogic
			);

			// �ȑO�̑I��͈͂��L������	// 2008.03.26 ryoji
			CLogicRange sDelLogic;
			m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
				sSelect_Old,
				&sDelLogic
			);

			// ���݂̍s�����L������	// 2008.03.26 ryoji
			int nLines_Old = m_pcEditDoc->m_cDocLineMgr.GetLineCount();

			// �ȑO�̑I��͈͂�I������
			GetSelectionInfo().SetBoxSelect( bBeginBoxSelect_Old );
			GetSelectionInfo().m_sSelectBgn = sSelectBgn_Old;
			GetSelectionInfo().m_sSelect = sSelect_Old;

			/* �I���G���A���폜 */
			DeleteData( TRUE );

			// �폜�O�̑I��͈͂𕜌�����	// 2008.03.26 ryoji
			if( !bBoxData ){
				// �폜���ꂽ�͈͂��l�����đI��͈͂𒲐�����
				if( sSelLogic.GetFrom().GetY2() == sDelLogic.GetTo().GetY2() ){	// �I���J�n���폜�����Ɠ���s
					sSelLogic.SetFromX(
						sSelLogic.GetFrom().GetX2()
						- (sDelLogic.GetTo().GetX2() - sDelLogic.GetFrom().GetX2())
					);
				}
				if( sSelLogic.GetTo().GetY2() == sDelLogic.GetTo().GetY2() ){	// �I���I�����폜�����Ɠ���s
					sSelLogic.SetToX(
						sSelLogic.GetTo().GetX2()
						- (sDelLogic.GetTo().GetX2() - sDelLogic.GetFrom().GetX2())
					);
				}
				// Note.
				// (sDelLogic.GetTo().GetY2() - sDelLogic.GetFrom().GetY2()) ���Ǝ��ۂ̍폜�s���Ɠ����ɂȂ�
				// ���Ƃ����邪�A�i�폜�s���|�P�j�ɂȂ邱�Ƃ�����D
				// ��j�t���[�J�[�\���ł̍s�ԍ��N���b�N���̂P�s�I��
				int nLines = m_pcEditDoc->m_cDocLineMgr.GetLineCount();
				sSelLogic.SetFromY( sSelLogic.GetFrom().GetY2() - (nLines_Old - nLines) );
				sSelLogic.SetToY( sSelLogic.GetTo().GetY2() - (nLines_Old - nLines) );

				// ������̑I��͈͂�ݒ肷��
				m_pcEditDoc->m_cLayoutMgr.LogicToLayout(
					sSelLogic,
					&GetSelectionInfo().m_sSelect
				);
				ptCaretPos_Old = GetSelectionInfo().m_sSelect.GetTo();
			}

			// �L�����b�g���ړ�����
			GetCaret().MoveCursor( ptCaretPos_Old, TRUE );
			GetCaret().m_nCaretPosX_Prev = GetCaret().GetCaretLayoutPos().GetX2();

			// �폜�ʒu����ړ���ւ̃J�[�\���ړ����A���h�D����ɒǉ�����	// 2008.03.26 ryoji
			CLogicPoint ptBefore;
			m_pcEditDoc->m_cLayoutMgr.LayoutToLogic(
				GetSelectionInfo().m_sSelect.GetFrom(),
				&ptBefore
			);
			m_pcOpeBlk->AppendOpe(
				new CMoveCaretOpe(
					sDelLogic.GetFrom(),
					GetCaret().GetCaretLogicPos()
				)
			);
		}
	}
	GetSelectionInfo().DrawSelectArea();

	/* �A���h�D�o�b�t�@�̏��� */
	if( NULL != m_pcOpeBlk ){
		if( 0 < m_pcOpeBlk->GetNum() ){	/* ����̐���Ԃ� */
			/* ����̒ǉ� */
			m_pcEditDoc->m_cDocEditor.m_cOpeBuf.AppendOpeBlk( m_pcOpeBlk );
			m_pcEditWnd->RedrawInactivePane();	// ���̃y�C���̕\��	// 2007.07.22 ryoji
		}else{
			delete m_pcOpeBlk;
		}
		m_pcOpeBlk = NULL;
	}

	::GlobalUnlock( hData );
	// 2004.07.12 fotomo/���� �������[���[�N�̏C��
	if( 0 == (GMEM_LOCKCOUNT & ::GlobalFlags(hData)) ){
		::GlobalFree( hData );
	}

	return S_OK;
}

CLIPFORMAT CEditView::GetAvailableClipFormat( LPDATAOBJECT pDataObject )
{
	CLIPFORMAT cf = 0;
	CLIPFORMAT cfSAKURAClip = CClipboard::GetSakuraFormat();

	if( IsDataAvailable(pDataObject, cfSAKURAClip) )
		cf = cfSAKURAClip;
	else if( IsDataAvailable(pDataObject, CF_UNICODETEXT) )
		cf = CF_UNICODETEXT;
	else if( IsDataAvailable(pDataObject, CF_TEXT) )
		cf = CF_TEXT;

	return cf;
}

DWORD CEditView::TranslateDropEffect( DWORD dwKeyState, POINTL pt, DWORD dwEffect )
{
	CEditView* pcDragSourceView = m_pcEditWnd->GetDragSourceView();

	/* ���̃r���[�̃J�[�\�����h���b�O���̑I��͈͓��ɂ��邩 */
	if( pcDragSourceView &&
		!pcDragSourceView->IsCurrentPositionSelected( GetCaret().GetCaretLayoutPos() )
	){
		return DROPEFFECT_NONE;
	};

#if 1
	// �h���b�O�����O���E�B���h�E���ǂ����ɂ���Ď󂯕���ς���
	// ���ėp�e�L�X�g�G�f�B�^�ł͂����炪�嗬���ۂ�
	if( pcDragSourceView ){
#else
	// �h���b�O�����ړ����������ǂ����ɂ���Ď󂯕���ς���
	// ��MS ���i�iMS Office, Visual Studio�Ȃǁj�ł͂����炪�嗬���ۂ�
	if( dwEffect & DROPEFFECT_MOVE ){
#endif
		dwEffect &= GetKeyState_Control()? DROPEFFECT_COPY: DROPEFFECT_MOVE;
	}else{
		dwEffect &= GetKeyState_Shift()? DROPEFFECT_MOVE: DROPEFFECT_COPY;
	}
	return dwEffect;
}

bool CEditView::IsDragSource( void )
{
	return ( this == m_pcEditWnd->GetDragSourceView() );
}