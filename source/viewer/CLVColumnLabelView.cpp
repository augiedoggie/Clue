//ColumnLabelView class source file


/******************************************************************************************************
**** PROJECT HEADER FILES
******************************************************************************************************/
//#define DEBUG 1

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _MENU_H
#include <Menu.h>
#endif

#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif

#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif

#ifndef _REGION_H
#include <Region.h>
#endif


/******************************************************************************************************
**** PROJECT HEADER FILES
******************************************************************************************************/
#ifndef _CLV_COLUMN_LABEL_VIEW_H_
#include "CLVColumnLabelView.h"
#endif

#ifndef _CLV_COLUMN_LIST_VIEW_H_
#include "ColumnListView.h"
#endif

#ifndef _CLV_COLUMN_H_
#include "CLVColumn.h"
#endif

static const unsigned char kCursorEastWest[] = {
	16, 1, 2, 2,
	//pixel color data
	0, 0,
	0, 0,
	56, 0,
	36, 0,
	36, 0,
	19, 224,
	18, 92,
	9, 42,
	8, 1,
	60, 1,
	76, 73,
	66, 133,
	49, 255,
	12, 133,
	2, 72,
	1, 0,
	//now comes the mask
	0, 0,
	0, 0,
	56, 0,
	60, 0,
	60, 0,
	31, 224,
	31, 252,
	15, 254,
	15, 255,
	63, 255,
	127, 255,
	127, 255,
	63, 255,
	15, 255,
	3, 254,
	1, 248
};


/******************************************************************************************************
**** FUNCTION DEFINITIONS
******************************************************************************************************/
CLVColumnLabelView :: CLVColumnLabelView
(
	BRect Bounds
	,	ColumnListView* parent
	,	const BFont* Font
)
	:	BView(Bounds, "CLVColumnLabelView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS)
	,	fFontAscent(0.0)
	,	fDisplayList(NULL)
	,	fColumnClicked(NULL)
	,	fPreviousMousePos(0.0, 0.0)
	,	fMouseClickedPos(0.0, 0.0)
	,	fColumnDragging(false)
	,	fColumnResizing(false)
	,	fDragGroups(10)
	,	fDragGroup(0)
	,	fTheDragGroup(NULL)
	,	fTheShownGroupBefore(NULL)
	,	fTheShownGroupAfter(NULL)
	,	fSnapGroupBefore(0)
	,	fSnapGroupAfter(0)
	,	fDragBoxMouseHoldOffset(0.0)
	,	fResizeMouseHoldOffset(0.0)
	,	fDragBoxWidth(0.0)
	,	fPrevDragOutlineLeft(0.0)
	,	fPrevDragOutlineRight(0.0)
	,	fSnapMin(0.0)
	,	fSnapMax(0.0)
	,	fParent(parent)
	,	m_WatchMouse(false)
{
	SetFont(Font);
	SetViewColor(BeBackgroundGrey);
	SetLowColor(BeBackgroundGrey);
	SetHighColor(Black);
	fDisplayList = &fParent->fColumnDisplayList;
	font_height FontAttributes;
	Font->GetHeight(&FontAttributes);
	fFontAscent = ceil(FontAttributes.ascent);
}


CLVColumnLabelView::~CLVColumnLabelView()
{
	int32 NumberOfGroups = fDragGroups.CountItems();
	CLVDragGroup* DragGroup(NULL);

	for (int32 Counter = 0; Counter < NumberOfGroups; Counter++) {
		DragGroup = (CLVDragGroup*) fDragGroups.RemoveItem(int32(0));
		delete DragGroup;
	}
}


void CLVColumnLabelView::Draw(BRect update_rect)
{
	BRect ViewBounds = Bounds();

	//Draw each column label in turn
	float ColumnBegin = 0.0;
	float ColumnEnd = -1.0;
	bool MergeWithLeft = false;
	int32 NumberOfColumns = fDisplayList->CountItems();
	BPoint Start, Stop;
	for (int32 ColumnDraw = 0; ColumnDraw < NumberOfColumns; ColumnDraw++) {
		CLVColumn* ThisColumn = (CLVColumn*)fDisplayList->ItemAt(ColumnDraw);
		if (ThisColumn->IsShown()) {
			//Figure out where this column is
			ColumnBegin = ThisColumn->fColumnBegin;
			ColumnEnd = ThisColumn->fColumnEnd;
			//Start by figuring out if this column will merge with a shown column to the right
			bool MergeWithRight = false;
			if (ThisColumn->fFlags & CLV_MERGE_WITH_RIGHT) {
				for (int32 ColumnCounter = ColumnDraw+1; ColumnCounter < NumberOfColumns;
					 ColumnCounter++) {
					CLVColumn* NextColumn = (CLVColumn*)fDisplayList->ItemAt(ColumnCounter);
					if (NextColumn->IsShown()) {
						//The next column is shown
						MergeWithRight = true;
						break;
					} else if (!(NextColumn->fFlags & CLV_MERGE_WITH_RIGHT))
						//The next column is not shown and doesn't pass on the merge
						break;
				}
			}
			if (update_rect.Intersects(BRect(ColumnBegin, ViewBounds.top, ColumnEnd,
											 ViewBounds.bottom))) {
				//Need to draw this column
				BeginLineArray(4);
				//Top line
				Start.Set(ColumnBegin, ViewBounds.top);
				Stop.Set(ColumnEnd - 1.0, ViewBounds.top);
				if (MergeWithRight && !(ThisColumn == fColumnClicked && fColumnResizing))
					Stop.x = ColumnEnd;
				AddLine(Start, Stop, BeHighlight);
				//Left line
				if (!MergeWithLeft)
					AddLine(BPoint(ColumnBegin, ViewBounds.top+1.0),
							BPoint(ColumnBegin, ViewBounds.bottom), BeHighlight);
				//Bottom line
				Start.Set(ColumnBegin+1.0, ViewBounds.bottom);
				if (MergeWithLeft)
					Start.x = ColumnBegin;
				Stop.Set(ColumnEnd - 1.0, ViewBounds.bottom);
				if (MergeWithRight && !(ThisColumn == fColumnClicked && fColumnResizing))
					Stop.x = ColumnEnd;
				AddLine(Start, Stop, BeShadow);
				//Right line
				if (ThisColumn == fColumnClicked && fColumnResizing)
					AddLine(BPoint(ColumnEnd, ViewBounds.top), BPoint(ColumnEnd, ViewBounds.bottom),
							BeFocusBlue);
				else if (!MergeWithRight)
					AddLine(BPoint(ColumnEnd, ViewBounds.top), BPoint(ColumnEnd, ViewBounds.bottom),
							BeShadow);
				EndLineArray();

				//Add the label
				//Limit the clipping region to the interior of the box
				BRect TextRect(ColumnBegin+1.0, ViewBounds.top+1.0, ColumnEnd - 1.0,
							   ViewBounds.bottom - 1.0);
				BRegion TextRegion;
				TextRegion.Include(TextRect);
				ConstrainClippingRegion(&TextRegion);

				bool focus;
				bool sort_key;
				if (ThisColumn == fColumnClicked && !fColumnResizing)
					focus = true;
				else
					focus = false;
				if (fParent->fSortKeyList.HasItem(ThisColumn) && ThisColumn->fSortMode != NoSort)
					sort_key = true;
				else
					sort_key = false;

				ThisColumn->DrawColumnHeader(this, TextRect, sort_key, focus, fFontAscent);

				//Restore the clipping region
				ConstrainClippingRegion(NULL);
			}
			//Set MergeWithLeft flag for the next column to the appropriate state
			MergeWithLeft = MergeWithRight;
		}
	}

	//Add highlight and shadow to the region after the columns if necessary
	if (ColumnEnd < ViewBounds.right) {
		ColumnBegin = ColumnEnd+1.0;
		if (update_rect.Intersects(BRect(ColumnEnd+1.0, ViewBounds.top, ViewBounds.right,
										 ViewBounds.bottom))) {
			BeginLineArray(3);
			//Top line
			AddLine(BPoint(ColumnBegin, ViewBounds.top), BPoint(ViewBounds.right, ViewBounds.top),
					BeHighlight);
			//Left line
			AddLine(BPoint(ColumnBegin, ViewBounds.top+1.0), BPoint(ColumnBegin, ViewBounds.bottom),
					BeHighlight);
			//Bottom line
			Start.Set(ColumnBegin+1.0, ViewBounds.bottom);
			if (MergeWithLeft)
				Start.x = ColumnBegin;
			Stop.Set(ViewBounds.right, ViewBounds.bottom);
			AddLine(Start, Stop, BeShadow);
			EndLineArray();
		}
	}

	//Draw the dragging box if necessary
	if (fColumnClicked && fColumnDragging) {
		float DragOutlineLeft = fPreviousMousePos.x - fDragBoxMouseHoldOffset;
		float GroupBegin = ((CLVDragGroup*)fDragGroups.ItemAt(fDragGroup))->GroupBegin;
		if (DragOutlineLeft < GroupBegin && fSnapGroupBefore == -1)
			DragOutlineLeft = GroupBegin;
		if (DragOutlineLeft > GroupBegin && fSnapGroupAfter == -1)
			DragOutlineLeft = GroupBegin;
		float DragOutlineRight = DragOutlineLeft + fDragBoxWidth;
		BeginLineArray(4);
		AddLine(BPoint(DragOutlineLeft, ViewBounds.top), BPoint(DragOutlineRight,
				ViewBounds.top), BeFocusBlue);
		AddLine(BPoint(DragOutlineLeft, ViewBounds.bottom), BPoint(DragOutlineRight,
				ViewBounds.bottom), BeFocusBlue);
		AddLine(BPoint(DragOutlineLeft, ViewBounds.top+1.0), BPoint(DragOutlineLeft,
				ViewBounds.bottom - 1.0), BeFocusBlue);
		AddLine(BPoint(DragOutlineRight, ViewBounds.top+1.0), BPoint(DragOutlineRight,
				ViewBounds.bottom - 1.0), BeFocusBlue);
		EndLineArray();
		fPrevDragOutlineLeft = DragOutlineLeft;
		fPrevDragOutlineRight = DragOutlineRight;
	}
}


void
CLVColumnLabelView :: MouseDown
(
	BPoint Point
)
{
	//Only pay attention to primary mouse button
	BPoint MousePos;
	uint32 Buttons;
	GetMouse(&MousePos, &Buttons);
	if (B_PRIMARY_MOUSE_BUTTON == Buttons) {
		BRect ViewBounds = Bounds();

		//Make sure no other column was already clicked.  If so, just discard the old one and redraw the
		//view
		if (fColumnClicked != NULL) {
			Invalidate();
			fColumnClicked = NULL;
		}

		//Find the column that the user clicked, if any
		bool GrabbedResizeTab = false;
		int32 NumberOfColumns = fDisplayList->CountItems();
		int32 ColumnFind;
		CLVColumn* ThisColumn = NULL;
		for (ColumnFind = 0; ColumnFind < NumberOfColumns; ColumnFind++) {
			ThisColumn = (CLVColumn*)fDisplayList->ItemAt(ColumnFind);
			if (ThisColumn->IsShown()) {
				float ColumnBegin = ThisColumn->fColumnBegin;
				float ColumnEnd = ThisColumn->fColumnEnd;
				if (Point.x >= ColumnBegin && Point.x <= ColumnEnd) {
					//User clicked in this column
					if (Point.x <= ColumnBegin+2.0) {
						//User clicked the resize tab preceding this column
						for (ColumnFind--; ColumnFind >= 0; ColumnFind--) {
							ThisColumn = (CLVColumn*)fDisplayList->ItemAt(ColumnFind);
							if (ThisColumn->IsShown()) {
								GrabbedResizeTab = true;
								break;
							}
						}
					} else if (Point.x >= ColumnEnd - 2.0) {
						//User clicked the resize tab for (after) this column
						GrabbedResizeTab = true;
					} else {
						//The user clicked in this column
						fColumnClicked = (CLVColumn*)fDisplayList->ItemAt(ColumnFind);
						fColumnResizing = false;
						fPreviousMousePos = Point;
						fMouseClickedPos = Point;
						fColumnDragging = false;
						SetSnapMinMax();
						fDragBoxMouseHoldOffset = Point.x -
						((CLVDragGroup*)fDragGroups.ItemAt(fDragGroup))->GroupBegin;
						Invalidate(BRect(ColumnBegin+1.0, ViewBounds.top+1.0, ColumnEnd - 1.0,
										 ViewBounds.bottom - 1.0));

						//Start watching the mouse
						m_WatchMouse = true;
					}
					break;
				}
			}
		}
		if (GrabbedResizeTab) {
			//The user grabbed a resize tab.  See if resizing of this column is allowed
			if (!(ThisColumn->fFlags & CLV_NOT_RESIZABLE)) {
				fColumnClicked = (CLVColumn*)fDisplayList->ItemAt(ColumnFind);
				fColumnResizing = true;
				fPreviousMousePos = Point;
				fMouseClickedPos = Point;
				fColumnDragging = false;
				fResizeMouseHoldOffset = Point.x - fColumnClicked->fColumnEnd;
				Invalidate(BRect(fColumnClicked->fColumnEnd, ViewBounds.top, ThisColumn->fColumnEnd,
								 ViewBounds.bottom));

				//Start watching the mouse
				m_WatchMouse = true;
			}
		}
	} else if (B_SECONDARY_MOUSE_BUTTON == Buttons) {
		int32 NumberOfColumns = fDisplayList->CountItems();
		int32 ColumnFind;
		CLVColumn* ThisColumn(NULL);
		BPopUpMenu* popupmenu(new BPopUpMenu("ColumnSelect", false, false));
		float offset(0.0);
		float tmpfw(0.0);

		for (ColumnFind = 0; ColumnFind < NumberOfColumns; ColumnFind++) {
			ThisColumn = (CLVColumn*) fDisplayList->ItemAt(ColumnFind);
			BMessage* msg(NULL);
			if (ThisColumn->IsShown())
				msg = new BMessage('1112');
			else
				msg = new BMessage('1111');
			msg->AddPointer("CLVColumn", (void*) ThisColumn);
			BMenuItem* menuitem(new BMenuItem(ThisColumn->GetLabel(), msg));
			menuitem->SetTarget(this);   //send to ColumnListView who has the control to show/hide
			menuitem->SetMarked(ThisColumn->IsShown());
			popupmenu->AddItem(menuitem);
			tmpfw = menuitem->Frame().Width();
			PRINT(("menuitem-frame-width = %0.f.\n", tmpfw));
			PRINT((" frame = %0.f, %0.f.\n", menuitem->Frame().left, menuitem->Frame().right));
			if (tmpfw > offset)
				offset = tmpfw;
		}

		popupmenu->ResizeToPreferred();
#ifdef DEBUG
		BRect rc(popupmenu->Frame());
		PRINT(("popupmenu-width = %0.f, %0.f\n", rc.left, rc.right));
#endif
		ConvertToScreen(&Point);
		Point.x -= offset;
		PRINT(("offset = %0.f\n", offset));
		popupmenu->Go(Point, true, false, true);
	}

	if (m_WatchMouse) {
		fPreviousMousePos = Point;
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}
}



void
CLVColumnLabelView :: MouseMoved
(
	BPoint Point
	,	uint32 transit
	,	const BMessage* message
)
{
	//BPoint MousePos (Point);
	//message->FindPoint("where",&MousePos);
	//uint32 Buttons;
	//message->FindInt32 ("buttons", (int32 *) &Buttons);
	//uint32 Modifiers;
	//message->FindInt32 ("modifiers", (int32 *) &Modifiers);
	BRect ViewBounds(Bounds());
	if (m_WatchMouse) {
		//Mouse is still held down
		if (!fColumnResizing) {
			//User is clicking or dragging
			if ((Point.x < fMouseClickedPos.x - 2.0 || Point.x > fMouseClickedPos.x + 2.0) &&
				!fColumnDragging) {
				//User is initiating a drag
				if (fTheDragGroup->Flags & CLV_NOT_MOVABLE) {
					//Not allowed to drag this column - terminate the click
					Invalidate(BRect(fColumnClicked->fColumnBegin, ViewBounds.top,
									 fColumnClicked->fColumnEnd, ViewBounds.bottom));
					fColumnClicked = NULL;
				} else {
					//Actually initiate a drag
					fColumnDragging = true;
					fPrevDragOutlineLeft = -1.0;
					fPrevDragOutlineRight = -1.0;
				}
			}
			//Now deal with dragging
			if (fColumnDragging) {
				//User is dragging
				if (Point.x < fPreviousMousePos.x || Point.x > fPreviousMousePos.x) {
					//Mouse moved since I last checked
					ViewBounds = Bounds();
					bool ColumnSnapped;
					do {
						//Live dragging of columns
						ColumnSnapped = false;
						float ColumnsUpdateLeft = 0.0;
						float ColumnsUpdateRight = 0.0;
						float MainViewUpdateLeft = 0.0;
						float MainViewUpdateRight = 0.0;
						CLVColumn* LastSwapColumn = NULL;
						if (fSnapMin != -1.0 && Point.x < fSnapMin) {
							//Shift the group left
							ColumnsUpdateLeft = fTheShownGroupBefore->GroupBegin;
							ColumnsUpdateRight = fTheDragGroup->GroupEnd;
							MainViewUpdateLeft = ColumnsUpdateLeft;
							MainViewUpdateRight = ColumnsUpdateRight;
							LastSwapColumn = fTheShownGroupBefore->LastColumnShown;
							if (fTheDragGroup->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							else if (fTheShownGroupBefore->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							ShiftDragGroup(fSnapGroupBefore);
							ColumnSnapped = true;
						}
						if (fSnapMax != -1.0 && Point.x > fSnapMax) {
							//Shift the group right
							ColumnsUpdateLeft = fTheDragGroup->GroupBegin;
							ColumnsUpdateRight = fTheShownGroupAfter->GroupEnd;
							MainViewUpdateLeft = ColumnsUpdateLeft;
							MainViewUpdateRight = ColumnsUpdateRight;
							LastSwapColumn = fTheDragGroup->LastColumnShown;
							if (fTheDragGroup->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							else if (fTheShownGroupAfter->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							ShiftDragGroup(fSnapGroupAfter+1);
							ColumnSnapped = true;
						}
						if (ColumnSnapped) {
							//Redraw the snapped column labels
							Invalidate(BRect(ColumnsUpdateLeft, ViewBounds.top, ColumnsUpdateRight,
											 ViewBounds.bottom));
							BRect MainViewBounds(fParent->Bounds());
							//Modify MainViewUpdateRight if more columns are pushed by expanders
							if (LastSwapColumn->fFlags & CLV_EXPANDER ||
								(LastSwapColumn->fPushedByExpander && (LastSwapColumn->fFlags&
										CLV_PUSH_PASS))) {
								int32 NumberOfColumns = fDisplayList->CountItems();
								for (int32 ColumnCounter = fDisplayList->IndexOf(LastSwapColumn)+1;
									 ColumnCounter < NumberOfColumns; ColumnCounter++) {
									CLVColumn* ThisColumn =
									(CLVColumn*)fDisplayList->ItemAt(ColumnCounter);
									if (ThisColumn->IsShown()) {
										if (ThisColumn->fPushedByExpander)
											MainViewUpdateRight = ThisColumn->fColumnEnd;
										else
											break;
									}
								}
							}
							fParent->Invalidate(BRect(MainViewUpdateLeft, MainViewBounds.top,
													  MainViewUpdateRight, MainViewBounds.bottom));
						}
					} while (ColumnSnapped);
					//Erase and redraw the drag rectangle but not the interior to avoid label flicker
					float Min(fPrevDragOutlineLeft);
					float Max(fPrevDragOutlineRight);
					float Min2(Point.x - fDragBoxMouseHoldOffset);
					float GroupBegin(((CLVDragGroup*) fDragGroups.ItemAt(fDragGroup))->GroupBegin);
					if (Min2 < GroupBegin && fSnapGroupBefore == -1)
						Min2 = GroupBegin;
					if (Min2 > GroupBegin && fSnapGroupAfter == -1)
						Min2 = GroupBegin;
					float Max2(Min2 + fDragBoxWidth);
					float Temp(0.0);
					if (Min2 < Min || Min == -1.0) {
						Temp = Min2; Min2 = Min; Min = Temp;
					}
					if (Max2 > Max || Max == -1.0) {
						Temp = Max2; Max2 = Max; Max = Temp;
					}
					Invalidate(BRect(Min, ViewBounds.top + 1.0, Min, ViewBounds.bottom - 1.0));
					if (Min2 != -1.0)
						Invalidate(BRect(Min2, ViewBounds.top + 1.0, Min2, ViewBounds.bottom - 1.0));
					Invalidate(BRect(Max, ViewBounds.top+1.0, Max, ViewBounds.bottom - 1.0));
					if (Max2 != -1.0)
						Invalidate(BRect(Max2, ViewBounds.top + 1.0, Max2, ViewBounds.bottom - 1.0));
					Invalidate(BRect(Min, ViewBounds.top, Max, ViewBounds.top));
					Invalidate(BRect(Min, ViewBounds.bottom, Max, ViewBounds.bottom));
				}
			}
		} else {
			//User is resizing the column
			if (Point.x < fPreviousMousePos.x || Point.x>fPreviousMousePos.x) {
				float NewWidth(Point.x - fResizeMouseHoldOffset - fColumnClicked->fColumnBegin);
				if (NewWidth < fColumnClicked->fMinWidth)
					NewWidth = fColumnClicked->fMinWidth;
				if (NewWidth != fColumnClicked->fWidth)
					fColumnClicked->SetWidth(NewWidth);
			}
		}
		fPreviousMousePos = Point;
	} else {
		uint32 buttons(0);
		BPoint cursor;
		GetMouse(&cursor, &buttons, false);
		if (buttons) {
			PRINT((" early return\n"));
			return;
		}

		if (B_ENTERED_VIEW == transit || B_INSIDE_VIEW == transit) {
			PRINT(("transit == Entered or Inside\n"));
			//Find the column that the mouse is over, if any
			bool OverResizeTab(false);
			int32 NumberOfColumns = fDisplayList->CountItems();
			int32 ColumnFind;
			CLVColumn* ThisColumn = NULL;
			for (ColumnFind = 0; ColumnFind < NumberOfColumns; ColumnFind++) {
				ThisColumn = (CLVColumn*)fDisplayList->ItemAt(ColumnFind);
				if (ThisColumn->IsShown()) {
					float ColumnBegin = ThisColumn->fColumnBegin;
					float ColumnEnd = ThisColumn->fColumnEnd;
					if (Point.x >= ColumnBegin && Point.x <= ColumnEnd) {
						//User clicked in this column
						if (Point.x <= ColumnBegin + 2.0) {
							//User clicked the resize tab preceding this column
							for (ColumnFind--; ColumnFind >= 0; ColumnFind--) {
								ThisColumn = (CLVColumn*) fDisplayList->ItemAt(ColumnFind);
								if (ThisColumn->IsShown()) {
									OverResizeTab = true;
									break;
								}
							}
						} else if (Point.x >= ColumnEnd - 2.0) {
							//User clicked the resize tab for (after) this column
							OverResizeTab = true;
						}
						break;
					}
				}
			}
			if (OverResizeTab) {
				PRINT((" OverResizeTab = true.\n"));
				be_app->SetCursor(kCursorEastWest);
			} else {
				PRINT((" OverResizeTab = false.\n"));
				be_app->SetCursor(B_HAND_CURSOR);
			}
		} else {
			PRINT((" just setcursor to hand.\n"));
			be_app->SetCursor(B_HAND_CURSOR);
		}
	}
}


void
CLVColumnLabelView :: MouseUp
(
	BPoint point
)
{
	PRINT(("CLVColumnLabelView :: MouseUp\n"));
	if (m_WatchMouse) {
		BRect ViewBounds(Bounds());
		uint32 Modifiers(0);
		uint32 ColumnFlags(fColumnClicked->Flags());
		m_WatchMouse = false;
		//Mouse button was released
		if (!fColumnDragging && !fColumnResizing) {
			//Column was clicked
			if (CLV_SORT_KEYABLE & ColumnFlags) {
				//The column is a "sortable" column
				if (!(B_SHIFT_KEY & Modifiers)) {
					//The user wants to select it as the main sorting column
					if (fParent->fSortKeyList.ItemAt(0) == fColumnClicked)
						//The column was already selected; switch sort modes
						fParent->ReverseSortMode(fParent->fColumnList.IndexOf(fColumnClicked));
					else
						//The user selected this column for sorting
						fParent->SetSortKey(fParent->fColumnList.IndexOf(fColumnClicked));
				} else {
					//The user wants to add it as a secondary sorting column
					if (fParent->fSortKeyList.HasItem(fColumnClicked))
						//The column was already selected; switch sort modes
						fParent->ReverseSortMode(fParent->fColumnList.IndexOf(fColumnClicked));
					else
						//The user selected this column for sorting
						fParent->AddSortKey(fParent->fColumnList.IndexOf(fColumnClicked));
				}
			}
		} else if (fColumnDragging) {
			//Column was dragging; erase the drag box but not the interior to avoid label flicker
			Invalidate(BRect(fPrevDragOutlineLeft, ViewBounds.top + 1.0, fPrevDragOutlineLeft, ViewBounds.bottom - 1.0));
			Invalidate(BRect(fPrevDragOutlineRight, ViewBounds.top + 1.0, fPrevDragOutlineRight, ViewBounds.bottom - 1.0));
			Invalidate(BRect(fPrevDragOutlineLeft, ViewBounds.top, fPrevDragOutlineRight, ViewBounds.top));
			Invalidate(BRect(fPrevDragOutlineLeft, ViewBounds.bottom, fPrevDragOutlineRight, ViewBounds.bottom));
		} else {
			//Column was resizing; erase the drag tab
			Invalidate(BRect(fColumnClicked->fColumnEnd, ViewBounds.top, fColumnClicked->fColumnEnd, ViewBounds.bottom));
			be_app->SetCursor(B_HAND_CURSOR);
		}
		//Unhighlight the label and forget the column
		Invalidate(BRect(fColumnClicked->fColumnBegin + 1.0, ViewBounds.top + 1.0,
						 fColumnClicked->fColumnEnd - 1.0, ViewBounds.bottom - 1.0));
		fColumnClicked = NULL;
		fColumnDragging = false;
		fColumnResizing = false;
	} else {
		PRINT(("  call MouseMoved\n"));
		MouseMoved(point, B_INSIDE_VIEW, NULL);
	}
}


void
CLVColumnLabelView :: MessageReceived
(
	BMessage* message
)
{
	CLVColumn* TheColumn(NULL);

	switch (message->what) {
		case '1111':
			message->FindPointer("CLVColumn", (void**) &TheColumn);
			if (TheColumn)
				TheColumn->SetShown(true);
			break;
		case '1112':
			message->FindPointer("CLVColumn", (void**) &TheColumn);
			if (TheColumn)
				TheColumn->SetShown(false);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
	return;
//	if(message->what != MW_MOUSE_MOVED && message->what != MW_MOUSE_DOWN && message->what != MW_MOUSE_UP)
//		BView::MessageReceived(message);
//	else if(fColumnClicked != NULL)
//	{
	BPoint MousePos;
	message->FindPoint("where", &MousePos);
	uint32 Buttons;
	message->FindInt32("buttons", (int32*)&Buttons);
	uint32 Modifiers;
	message->FindInt32("modifiers", (int32*)&Modifiers);
	BRect ViewBounds;
	ViewBounds = Bounds();
//		uint32 ColumnFlags = fColumnClicked->Flags();
	if (Buttons == B_PRIMARY_MOUSE_BUTTON) {
		//Mouse is still held down
		if (!fColumnResizing) {
			//User is clicking or dragging
			if ((MousePos.x<fMouseClickedPos.x - 2.0 || MousePos.x>fMouseClickedPos.x+2.0) &&
				!fColumnDragging) {
				//User is initiating a drag
				if (fTheDragGroup->Flags & CLV_NOT_MOVABLE) {
					//Not allowed to drag this column - terminate the click
					Invalidate(BRect(fColumnClicked->fColumnBegin, ViewBounds.top,
									 fColumnClicked->fColumnEnd, ViewBounds.bottom));
					fColumnClicked = NULL;
				} else {
					//Actually initiate a drag
					fColumnDragging = true;
					fPrevDragOutlineLeft = -1.0;
					fPrevDragOutlineRight = -1.0;
				}
			}

			//Now deal with dragging
			if (fColumnDragging) {
				//User is dragging
				if (MousePos.x<fPreviousMousePos.x || MousePos.x>fPreviousMousePos.x) {
					//Mouse moved since I last checked
					ViewBounds = Bounds();

					bool ColumnSnapped;
					do {
						//Live dragging of columns
						ColumnSnapped = false;
						float ColumnsUpdateLeft = 0.0;
						float ColumnsUpdateRight = 0.0;
						float MainViewUpdateLeft = 0.0;
						float MainViewUpdateRight = 0.0;
						CLVColumn* LastSwapColumn = NULL;
						if (fSnapMin != -1.0 && MousePos.x < fSnapMin) {
							//Shift the group left
							ColumnsUpdateLeft = fTheShownGroupBefore->GroupBegin;
							ColumnsUpdateRight = fTheDragGroup->GroupEnd;
							MainViewUpdateLeft = ColumnsUpdateLeft;
							MainViewUpdateRight = ColumnsUpdateRight;
							LastSwapColumn = fTheShownGroupBefore->LastColumnShown;
							if (fTheDragGroup->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							else if (fTheShownGroupBefore->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							ShiftDragGroup(fSnapGroupBefore);
							ColumnSnapped = true;
						}
						if (fSnapMax != -1.0 && MousePos.x > fSnapMax) {
							//Shift the group right
							ColumnsUpdateLeft = fTheDragGroup->GroupBegin;
							ColumnsUpdateRight = fTheShownGroupAfter->GroupEnd;
							MainViewUpdateLeft = ColumnsUpdateLeft;
							MainViewUpdateRight = ColumnsUpdateRight;
							LastSwapColumn = fTheDragGroup->LastColumnShown;
							if (fTheDragGroup->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							else if (fTheShownGroupAfter->LastColumnShown->fFlags & CLV_MERGE_WITH_RIGHT)
								ColumnsUpdateRight += 1.0;
							ShiftDragGroup(fSnapGroupAfter+1);
							ColumnSnapped = true;
						}
						if (ColumnSnapped) {
							//Redraw the snapped column labels
							Invalidate(BRect(ColumnsUpdateLeft, ViewBounds.top, ColumnsUpdateRight,
											 ViewBounds.bottom));
							BRect MainViewBounds = fParent->Bounds();
							//Modify MainViewUpdateRight if more columns are pushed by expanders
							if (LastSwapColumn->fFlags & CLV_EXPANDER ||
								(LastSwapColumn->fPushedByExpander && (LastSwapColumn->fFlags&
										CLV_PUSH_PASS))) {
								int32 NumberOfColumns = fDisplayList->CountItems();
								for (int32 ColumnCounter = fDisplayList->IndexOf(LastSwapColumn)+1;
									 ColumnCounter < NumberOfColumns; ColumnCounter++) {
									CLVColumn* ThisColumn =
									(CLVColumn*)fDisplayList->ItemAt(ColumnCounter);
									if (ThisColumn->IsShown()) {
										if (ThisColumn->fPushedByExpander)
											MainViewUpdateRight = ThisColumn->fColumnEnd;
										else
											break;
									}
								}
							}
							fParent->Invalidate(BRect(MainViewUpdateLeft, MainViewBounds.top,
													  MainViewUpdateRight, MainViewBounds.bottom));
						}
					} while (ColumnSnapped);
					//Erase and redraw the drag rectangle but not the interior to avoid label flicker
					float Min = fPrevDragOutlineLeft;
					float Max = fPrevDragOutlineRight;
					float Min2 = MousePos.x - fDragBoxMouseHoldOffset;
					float GroupBegin = ((CLVDragGroup*)fDragGroups.ItemAt(fDragGroup))->GroupBegin;
					if (Min2 < GroupBegin && fSnapGroupBefore == -1)
						Min2 = GroupBegin;
					if (Min2 > GroupBegin && fSnapGroupAfter == -1)
						Min2 = GroupBegin;
					float Max2 = Min2 + fDragBoxWidth;
					float Temp;
					if (Min2 < Min || Min == -1.0)
					{Temp = Min2; Min2 = Min; Min = Temp;}
					if (Max2 > Max || Max == -1.0)
					{Temp = Max2; Max2 = Max; Max = Temp;}
					Invalidate(BRect(Min, ViewBounds.top+1.0, Min, ViewBounds.bottom - 1.0));
					if (Min2 != -1.0)
						Invalidate(BRect(Min2, ViewBounds.top+1.0, Min2, ViewBounds.bottom - 1.0));
					Invalidate(BRect(Max, ViewBounds.top+1.0, Max, ViewBounds.bottom - 1.0));
					if (Max2 != -1.0)
						Invalidate(BRect(Max2, ViewBounds.top+1.0, Max2, ViewBounds.bottom - 1.0));
					Invalidate(BRect(Min, ViewBounds.top, Max, ViewBounds.top));
					Invalidate(BRect(Min, ViewBounds.bottom, Max, ViewBounds.bottom));
				}
			}
		} else {
			//User is resizing the column
			if (MousePos.x<fPreviousMousePos.x || MousePos.x>fPreviousMousePos.x) {
				float NewWidth = MousePos.x - fResizeMouseHoldOffset - fColumnClicked->fColumnBegin;
				if (NewWidth < fColumnClicked->fMinWidth)
					NewWidth = fColumnClicked->fMinWidth;
				if (NewWidth != fColumnClicked->fWidth)
					fColumnClicked->SetWidth(NewWidth);
			}
		}
	}
	/*		else if(Buttons == 0)
			{
				//Mouse button was released
				if(!fColumnDragging && !fColumnResizing)
				{
					//Column was clicked
					if(ColumnFlags&CLV_SORT_KEYABLE)
					{
						//The column is a "sortable" column
						if(!(Modifiers&B_SHIFT_KEY))
						{
							//The user wants to select it as the main sorting column
							if(fParent->fSortKeyList.ItemAt(0) == fColumnClicked)
								//The column was already selected; switch sort modes
								fParent->ReverseSortMode(fParent->fColumnList.IndexOf(fColumnClicked));
							else
								//The user selected this column for sorting
								fParent->SetSortKey(fParent->fColumnList.IndexOf(fColumnClicked));
						}
						else
						{
							//The user wants to add it as a secondary sorting column
							if(fParent->fSortKeyList.HasItem(fColumnClicked))
								//The column was already selected; switch sort modes
								fParent->ReverseSortMode(fParent->fColumnList.IndexOf(fColumnClicked));
							else
								//The user selected this column for sorting
								fParent->AddSortKey(fParent->fColumnList.IndexOf(fColumnClicked));
						}
					}
				}
				else if(fColumnDragging)
				{
					//Column was dragging; erase the drag box but not the interior to avoid label flicker
					Invalidate(BRect(fPrevDragOutlineLeft,ViewBounds.top+1.0,
						fPrevDragOutlineLeft,ViewBounds.bottom-1.0));
					Invalidate(BRect(fPrevDragOutlineRight,ViewBounds.top+1.0,
						fPrevDragOutlineRight,ViewBounds.bottom-1.0));
					Invalidate(BRect(fPrevDragOutlineLeft,ViewBounds.top,
						fPrevDragOutlineRight,ViewBounds.top));
					Invalidate(BRect(fPrevDragOutlineLeft,ViewBounds.bottom,
						fPrevDragOutlineRight,ViewBounds.bottom));
				}
				else
					//Column was resizing; erase the drag tab
					Invalidate(BRect(fColumnClicked->fColumnEnd,ViewBounds.top,fColumnClicked->fColumnEnd,
						ViewBounds.bottom));
				//Unhighlight the label and forget the column
				Invalidate(BRect(fColumnClicked->fColumnBegin+1.0,ViewBounds.top+1.0,
					fColumnClicked->fColumnEnd-1.0,ViewBounds.bottom-1.0));
				fColumnClicked = NULL;
				fColumnDragging = false;
				fColumnResizing = false;
			}
	*/		else {
		//Unused button combination
		//Unhighlight the label and forget the column
		Invalidate(BRect(fColumnClicked->fColumnBegin+1.0, ViewBounds.top+1.0,
						 fColumnClicked->fColumnEnd - 1.0, ViewBounds.bottom - 1.0));
		fColumnClicked = NULL;
		fColumnDragging = false;
		fColumnResizing = false;
	}
	fPreviousMousePos = MousePos;
//	}
}


void CLVColumnLabelView::ShiftDragGroup(int32 NewPos)
//Shift the drag group into a new position
{
	int32 NumberOfGroups = fDragGroups.CountItems();
	int32 GroupCounter;
	CLVDragGroup* ThisGroup;
	int32 NumberOfColumnsInGroup;
	int32 ColumnCounter;
	BList NewDisplayList;

	//Copy the groups up to the new position
	for (GroupCounter = 0; GroupCounter < NewPos; GroupCounter++) {
		if (GroupCounter != fDragGroup) {
			ThisGroup = (CLVDragGroup*)fDragGroups.ItemAt(GroupCounter);
			NumberOfColumnsInGroup = ThisGroup->GroupStopDispListIndex -
			ThisGroup->GroupStartDispListIndex + 1;
			for (ColumnCounter = ThisGroup->GroupStartDispListIndex; ColumnCounter <=
				 ThisGroup->GroupStopDispListIndex; ColumnCounter++)
				NewDisplayList.AddItem(fDisplayList->ItemAt(ColumnCounter));
		}
	}
	//Copy the group into the new position
	ThisGroup = (CLVDragGroup*)fDragGroups.ItemAt(fDragGroup);
	NumberOfColumnsInGroup = ThisGroup->GroupStopDispListIndex - ThisGroup->GroupStartDispListIndex + 1;
	for (ColumnCounter = ThisGroup->GroupStartDispListIndex; ColumnCounter <=
		 ThisGroup->GroupStopDispListIndex; ColumnCounter++)
		NewDisplayList.AddItem(fDisplayList->ItemAt(ColumnCounter));
	//Copy the rest of the groups, but skip the dragging group
	for (GroupCounter = NewPos; GroupCounter < NumberOfGroups; GroupCounter++) {
		if (GroupCounter != fDragGroup) {
			ThisGroup = (CLVDragGroup*)fDragGroups.ItemAt(GroupCounter);
			NumberOfColumnsInGroup = ThisGroup->GroupStopDispListIndex -
			ThisGroup->GroupStartDispListIndex + 1;
			for (ColumnCounter = ThisGroup->GroupStartDispListIndex; ColumnCounter <=
				 ThisGroup->GroupStopDispListIndex; ColumnCounter++)
				NewDisplayList.AddItem(fDisplayList->ItemAt(ColumnCounter));
		}
	}

	//Set the new order
	*fDisplayList = NewDisplayList;

	//Update columns and drag groups
	fParent->UpdateColumnSizesDataRectSizeScrollBars();
	UpdateDragGroups();
	SetSnapMinMax();

	//Inform the program that the display order changed
	int32* NewOrder = fParent->DisplayOrder();
	fParent->DisplayOrderChanged(NewOrder);
	delete[] NewOrder;
}


void CLVColumnLabelView::UpdateDragGroups()
{
	//Make a copy of the DragGroups list.  Use it to store the CLVDragGroup's for recycling
	BList TempList(fDragGroups);
	fDragGroups.MakeEmpty();
	int32 NumberOfColumns = fDisplayList->CountItems();
	bool ContinueGroup = false;
	CLVDragGroup* CurrentGroup = NULL;
	for (int32 Counter = 0; Counter < NumberOfColumns; Counter++) {
		CLVColumn* CurrentColumn = (CLVColumn*)fDisplayList->ItemAt(Counter);
		if (!ContinueGroup) {
			//Recycle or obtain a new CLVDragGroup
			CurrentGroup = (CLVDragGroup*)TempList.RemoveItem(int32(0));
			if (CurrentGroup == NULL)
				CurrentGroup = new CLVDragGroup;
			//Add the CLVDragGroup to the DragGroups list
			fDragGroups.AddItem(CurrentGroup);
			//Set up the new DragGroup
			CurrentGroup->GroupStartDispListIndex = Counter;
			CurrentGroup->GroupStopDispListIndex = Counter;
			CurrentGroup->Flags = 0;
			if (CurrentColumn->IsShown()) {
				CurrentGroup->GroupBegin = CurrentColumn->fColumnBegin;
				CurrentGroup->GroupEnd = CurrentColumn->fColumnEnd;
				CurrentGroup->LastColumnShown = CurrentColumn;
				CurrentGroup->Shown = true;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING)
					CurrentGroup->AllLockBeginning = true;
				else
					CurrentGroup->AllLockBeginning = false;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_END)
					CurrentGroup->AllLockEnd = true;
				else
					CurrentGroup->AllLockEnd = false;
			} else {
				CurrentGroup->GroupBegin = -1.0;
				CurrentGroup->GroupEnd = -1.0;
				CurrentGroup->LastColumnShown = NULL;
				CurrentGroup->Shown = false;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING)
					CurrentGroup->AllLockBeginning = true;
				else
					CurrentGroup->AllLockBeginning = false;
				if (CurrentColumn->fFlags & CLV_LOCK_AT_END)
					CurrentGroup->AllLockEnd = true;
				else
					CurrentGroup->AllLockEnd = false;
			}
		} else {
			//Add this column to the current DragGroup
			CurrentGroup->GroupStopDispListIndex = Counter;
			if (CurrentColumn->IsShown()) {
				if (CurrentGroup->GroupBegin == -1.0)
					CurrentGroup->GroupBegin = CurrentColumn->fColumnBegin;
				CurrentGroup->GroupEnd = CurrentColumn->fColumnEnd;
				CurrentGroup->LastColumnShown = CurrentColumn;
				CurrentGroup->Shown = true;
			}
			if (!(CurrentColumn->fFlags & CLV_LOCK_AT_BEGINNING))
				CurrentGroup->AllLockBeginning = false;
			if (!(CurrentColumn->fFlags & CLV_LOCK_AT_END))
				CurrentGroup->AllLockEnd = false;
		}
		CurrentGroup->Flags |= CurrentColumn->fFlags & (CLV_NOT_MOVABLE|CLV_LOCK_AT_BEGINNING|
				CLV_LOCK_AT_END);
		//See if I should add more columns to this group
		if (CurrentColumn->fFlags & CLV_LOCK_WITH_RIGHT)
			ContinueGroup = true;
		else
			ContinueGroup = false;
	}
	//If any unused groups remain in TempList, delete them
	while ((CurrentGroup = (CLVDragGroup*)TempList.RemoveItem(int32(0))) != NULL)
		delete CurrentGroup;
}


void CLVColumnLabelView::SetSnapMinMax()
{
	//Find the column group that the user is dragging and the shown group before it
	int32 NumberOfGroups = fDragGroups.CountItems();
	int32 ColumnCount;
	fDragGroup = -1;
	fTheShownGroupBefore = NULL;
	fSnapGroupBefore = -1;
	CLVDragGroup* ThisGroup;
	int32 GroupCounter;
	for (GroupCounter = 0; GroupCounter < NumberOfGroups; GroupCounter++) {
		ThisGroup = (CLVDragGroup*)fDragGroups.ItemAt(GroupCounter);
		for (ColumnCount = ThisGroup->GroupStartDispListIndex; ColumnCount <=
			 ThisGroup->GroupStopDispListIndex; ColumnCount++)
			if (fDisplayList->ItemAt(ColumnCount) == fColumnClicked) {
				fDragGroup = GroupCounter;
				fTheDragGroup = ThisGroup;
				break;
			}
		if (fDragGroup != -1)
			break;
		else if (ThisGroup->Shown) {
			fTheShownGroupBefore = ThisGroup;
			fSnapGroupBefore = GroupCounter;
		}
	}

	//Find the position of shown group after the one that the user is dragging
	fTheShownGroupAfter = NULL;
	fSnapGroupAfter = -1;
	for (GroupCounter = fDragGroup+1; GroupCounter < NumberOfGroups; GroupCounter++) {
		ThisGroup = (CLVDragGroup*)fDragGroups.ItemAt(GroupCounter);
		if (ThisGroup->Shown) {
			fTheShownGroupAfter = ThisGroup;
			fSnapGroupAfter = GroupCounter;
			break;
		}
	}

	//See if it can actually snap in the given direction
	if (fSnapGroupBefore != -1) {
		if (fTheShownGroupBefore->Flags & CLV_LOCK_AT_BEGINNING)
			if (!fTheDragGroup->AllLockBeginning)
				fSnapGroupBefore = -1;
		if (fTheDragGroup->Flags & CLV_LOCK_AT_END)
			if (!fTheShownGroupBefore->AllLockEnd)
				fSnapGroupBefore = -1;
	}
	if (fSnapGroupAfter != -1) {
		if (fTheShownGroupAfter->Flags & CLV_LOCK_AT_END)
			if (!fTheDragGroup->AllLockEnd)
				fSnapGroupAfter = -1;
		if (fTheDragGroup->Flags & CLV_LOCK_AT_BEGINNING)
			if (!fTheShownGroupAfter->AllLockBeginning)
				fSnapGroupAfter = -1;
	}

	//Find the minumum and maximum positions for the group to snap
	fSnapMin = -1.0;
	fSnapMax = -1.0;
	fDragBoxWidth = fTheDragGroup->GroupEnd - fTheDragGroup->GroupBegin;
	if (fSnapGroupBefore != -1) {
		fSnapMin = fTheShownGroupBefore->GroupBegin + fDragBoxWidth;
		if (fSnapMin > fTheShownGroupBefore->GroupEnd)
			fSnapMin = fTheShownGroupBefore->GroupEnd;
	}
	if (fSnapGroupAfter != -1) {
		fSnapMax = fTheShownGroupAfter->GroupEnd - fDragBoxWidth;
		if (fSnapMax < fTheShownGroupAfter->GroupBegin)
			fSnapMax = fTheShownGroupAfter->GroupBegin;
	}
}
