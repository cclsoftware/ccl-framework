//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/layout/perspectiveswitcher.cpp
// Description : Perspective Switcher
//
//************************************************************************************************

#include "ccl/gui/layout/perspectiveswitcher.h"
#include "ccl/gui/layout/workspace.h"

#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/itemviews/listview.h"
#include "ccl/gui/commands.h"

using namespace CCL;

//************************************************************************************************
// PerspectiveListModel
//************************************************************************************************

PerspectiveListModel::PerspectiveListModel (Workspace* workspace)
{
	ForEach (*workspace, Perspective, p)
		if(p->getActivator ())
			addPerspective (p);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveListModel::addPerspective (Perspective* perspective)
{
	int64 lastActivated = perspective->getLastActivated ();

	ArrayForEach (perspectives, Perspective, p)
		if(p->getLastActivated () <= lastActivated)
		{
			perspectives.insertAt (__iter, perspective);
			return;
		}
	EndFor
	perspectives.add (perspective);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPerspectiveActivator* PerspectiveListModel::getPerspective (int index)
{
	return ((Perspective*)perspectives.at (index))->getActivator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveListModel::activateFocusPerspective ()
{
	if(IPerspectiveActivator* p = getPerspective (getFocusIndex ()))
		p->activatePerspective ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveListModel::setFocusIndex (int index)
{
	if(IItemView* itemView = getItemView ())
		itemView->setFocusItem ((index + perspectives.count ()) % perspectives.count ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PerspectiveListModel::getFocusIndex ()
{
	ItemIndex focusItem;
	if(IItemView* itemView = getItemView ())
		if(itemView->getFocusItem (focusItem))
			return focusItem.getIndex ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveListModel::init (Perspective* current, int increment)
{
	int index = 0;
	ForEach (perspectives, Perspective, p)
		if(p == current)
		{
			setFocusIndex (index + increment);
			return;
		}
		index++;
	EndFor

	setFocusIndex (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveListModel::increment (int increment)
{
	setFocusIndex (getFocusIndex () + increment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PerspectiveListModel::countFlatItems ()
{
	return perspectives.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PerspectiveListModel::getItemTitle (String& title, ItemIndexRef index)
{
	if(IPerspectiveActivator* p = getPerspective (index.getIndex ()))
		title = p->getPerspectiveTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API PerspectiveListModel::getItemIcon (ItemIndexRef index)
{
	if(IPerspectiveActivator* p = getPerspective (index.getIndex ()))
		return p->getPerspectiveIcon ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PerspectiveListModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (40);  // kIcon
	list.addColumn (calculateTextWidth ()); // kTitle
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PerspectiveListModel::calculateTextWidth ()
{
	int w = 0;
	Font font;

	ForEach (perspectives, Perspective, p)
		if(IPerspectiveActivator* activator = p->getActivator ())
		{
			font.isBold (true);
			ccl_lower_limit (w, Font::getStringWidth (activator->getPerspectiveTitle (), font) + 4);

			font.isBold (false);
			ccl_lower_limit (w, Font::getStringWidth (activator->getPerspectiveDescription (), font));
		}
	EndFor
	return w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PerspectiveListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	IPerspectiveActivator* activator = getPerspective (index.getIndex ());
	if(!activator)
		return false;

	switch(column)
	{
	case kIcon :
		if(IImage* icon = activator->getPerspectiveIcon ())
		{
			Rect src (0, 0, icon->getWidth (), icon->getHeight ());
			Rect iconRect (src);
			iconRect.center (info.rect);
			info.graphics.drawImage (icon, src, iconRect);
		}
		break;

	case kTitle :
		{
			String title (activator->getPerspectiveTitle ());
			if(!title.isEmpty ())
			{
				String description (activator->getPerspectiveDescription ());

				Font font (info.style.font);
				font.isBold (true);

				Rect textRect (info.rect);
				textRect.top += 2;
				textRect.bottom -= 2;
				if(description.isEmpty ())
					info.graphics.drawString (textRect, title, font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
				else
				{
					textRect.setHeight (textRect.getHeight ()/2);
					info.graphics.drawString (textRect, title, font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);

					font.isBold (false);
					textRect.offset (0, textRect.getHeight ());
					info.graphics.drawString (textRect, description, font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
				}
			}
		}
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PerspectiveListModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	// prevent invoking another switcher instance via the command handled by WorkspaceSystem
	if(msg.category == "View" && (msg.name == "Next Perspective" || msg.name == "Previous Perspective"))
		return true;

	return false;
}

//************************************************************************************************
// PerspectiveSwitcher
//************************************************************************************************

PerspectiveSwitcher::PerspectiveSwitcher (Workspace* workspace)
: workspace (workspace),
  startIncrement (1),
  mainKey (VKey::kUnknown),
  modifierKey (VKey::kUnknown)
{
	perspectiveListModel = NEW PerspectiveListModel (workspace);

	// continue using the key that triggered the command
	KeyEvent* key = CommandTable::instance ().lookupKeyEvent (Command ("View", "Next Perspective"));
	if(key)
	{
		mainKey = key->vKey;
		modifierKey = getModifierKeyCode (key->state.getModifiers ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VirtualKey PerspectiveSwitcher::getModifierKeyCode (int modifier)
{
	// assuming that only one modifier is set
	switch(modifier)
	{
	case KeyState::kShift:		return VKey::kShift;
	case KeyState::kCommand:	return VKey::kCommand;
	case KeyState::kOption:		return VKey::kOption;
	case KeyState::kControl:	return VKey::kControl;
	}
	return VKey::kUnknown;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveSwitcher::run (bool next)
{
	if(perspectiveListModel->countFlatItems () < 2)
		return;

	startIncrement = next ? 1 : -1;

	View* parent = workspace->getWorkspaceView ();

	PopupSizeInfo sizeInfo (parent, PopupSizeInfo::kHCenter|PopupSizeInfo::kVCenter);
	PopupSelector popupSelector;
	popupSelector.setTheme (parent->getTheme ());
	popupSelector.popup (this, sizeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* PerspectiveSwitcher::getItemModel ()
{
	return perspectiveListModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* PerspectiveSwitcher::getVisualStyle (Theme& theme)
{
	if(visualStyle)
		return visualStyle;

	return theme.getStandardStyle (ThemePainter::kPerspectiveSwitcherStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PerspectiveSwitcher::onItemViewCreated ()
{
	ListViewPopup::onItemViewCreated ();

	if(View* listView = unknown_cast<View> (itemView))
		listView->autoSize ();

	perspectiveListModel->init (workspace->getCurrentPerspective (), startIncrement);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PerspectiveSwitcher::onKeyDown (const KeyEvent& event)
{
	if(event.vKey == mainKey)
	{
		perspectiveListModel->increment (event.state.isSet (KeyState::kShift) ? -1 : 1);
		return kIgnore;
	}
	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API PerspectiveSwitcher::onKeyUp (const KeyEvent& event)
{
	if(event.vKey == modifierKey)
		if(hasPopupResult ())
			return kOkay;

	return SuperClass::onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PerspectiveSwitcher::onPopupClosed (Result result)
{
	ListViewPopup::onPopupClosed (result);

	if(result == IPopupSelectorClient::kOkay)
		perspectiveListModel->activateFocusPerspective ();
}
