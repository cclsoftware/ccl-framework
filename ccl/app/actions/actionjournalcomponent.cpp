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
// Filename    : ccl/app/actions/actionjournalcomponent.cpp
// Description : Action Journal Component
//
//************************************************************************************************

#include "ccl/app/actions/actionjournalcomponent.h"
#include "ccl/app/actions/actionjournal.h"
#include "ccl/app/actions/action.h"

#include "ccl/app/controls/listviewmodel.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/ibitmapfilter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/usercontrolbase.h"

#include "ccl/public/system/formatter.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ActionListModel
//************************************************************************************************

class ActionListModel: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (ActionListModel, ListViewModel)

	ActionListModel (ActionJournalComponent& component, ActionJournal& journal);
	~ActionListModel ();

	void setUndoPosition (int position);

	enum Columns
	{
		kIndex,
		kIndicator,
		kTime,
		kDescription,
		kDetails
	};

	class ActionItem: public ListViewItem
	{
	public:
		ActionItem (): iconChecked (false) {}
		PROPERTY_BOOL (iconChecked, IconChecked)
		PROPERTY_SHARED_AUTO (Action, action, Action)
		PROPERTY_STRING (details, Details)
	};

	// ListViewModel
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API viewDetached (IItemView* itemView) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;

protected:
	ActionJournalComponent& component;
	ActionJournal& journal;
	bool observerEnabled;
	int undoCount;
	Pen linePen;
	bool colorizeIcons;
	Font textFont;
	bool initDone;
	AutoPtr<IBitmapPainter> bitmapPainter;

	void viewVisible (bool state);
	void enableObserver (bool state);
	ActionItem* createItem (Action* action) const;

	class UndoMouseHandler;
};

//************************************************************************************************
// ActionListModel::UndoMouseHandler
//************************************************************************************************

class ActionListModel::UndoMouseHandler: public AbstractMouseHandler,
										 public Unknown
{
public:
	UndoMouseHandler (ActionListModel& actionlist)
	 : actionlist (actionlist)
	{
		 autoScrollV (true);
	}

	void onBegin () override
	{
		onMove (0);
	}

	bool onMove (int moveFlags) override
	{
		ItemIndex itemIndex;
		IItemView* itemView = actionlist.getItemView ();
		if(itemView->findItem (itemIndex, current.where))
		{
			int index = itemIndex.getIndex ();
			Rect itemRect;
			itemView->getItemRect (itemRect, itemIndex);
			if(current.where.y > itemRect.getCenter ().y)
				index++;

			actionlist.setUndoPosition (index);
		}
		return true;
	}

	CLASS_INTERFACE (IMouseHandler, Unknown)

private:
	ActionListModel& actionlist;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static IImage* createTextIcon (StringRef text, FontRef font)
{
	IImage* image = GraphicsFactory::createShapeImage ();

	Rect size;
	Font::measureString (size, text, font);

	AutoPtr<IGraphics> g = GraphicsFactory::createShapeBuilder (image);
	size.right += 4;
	size.bottom += 4;
	g->fillRect (size, SolidBrush (Colors::kWhite));
	g->drawString (size, text, font, SolidBrush (Colors::kBlack), Alignment::kCenter);

	return image;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ActionListModel
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ActionListModel, ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionListModel::ActionListModel (ActionJournalComponent& component, ActionJournal& journal)
: component (component),
  journal (journal),
  observerEnabled (false),
  undoCount (0),
  colorizeIcons (false),
  initDone (false)
{
	journal.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionListModel::~ActionListModel ()
{
	enableObserver (false);
	cancelSignals ();

	journal.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionListModel::enableObserver (bool state)
{
	if(observerEnabled != state)
	{
		if(observerEnabled)
			journal.removeObserver (this);
		observerEnabled = state;
		if(observerEnabled)
			journal.addObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionListModel::viewVisible (bool state)
{
	if(state)
	{
		ASSERT (undoCount == 0)
		undoCount = 0;

		// make items
		IterForEachReverse (journal.newUndoIterator (), Action, action)
			ActionItem* item = createItem (action);
			items.add (item);
			undoCount++;
		EndFor

		IterForEach (journal.newRedoIterator (), Action, action)
			ActionItem* item = createItem (action);
			items.add (item);
		EndFor

		if(getItemView () && !items.isEmpty ())
		{
			int index = undoCount - 1; // make last undo item visible
			if(items.count () > undoCount)
				index++; // also show one redo item to make the separator more obvious

			(NEW Message ("makeItemVisible", index))->post (this);
		}

		enableObserver (true);
	}
	else
	{
		enableObserver (false);

		// remove items
		items.removeAll ();
		undoCount = 0;
	}

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActionListModel::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &journal)
	{
		int scrollToIndex = -1;
		if(msg == ActionJournal::kExecuted)
		{
			Action* action = unknown_cast<Action> (msg[0].asUnknown ());
			ASSERT (action != nullptr)

			// redo remove all
			for(int i = undoCount; i < items.count (); i++)
			{
				Object* item = items.at (i);
				items.removeAt (i);
				item->release ();
				i--;
			}

			// undo +1
			ActionItem* newItem = createItem (action);
			items.add (newItem);
			undoCount++;

			scrollToIndex = undoCount-1;
		}
		else if(msg == ActionJournal::kUndone)
		{
			ASSERT (undoCount > 0)
			undoCount--;
		}
		else if(msg == ActionJournal::kRedone)
		{
			undoCount++;
		}
		else if(msg == ActionJournal::kRemovedAll)
		{
			items.removeAll ();
			undoCount = 0;
		}
		else if(msg == ActionJournal::kUndoReduced)
		{
			// undo remove first
			ASSERT (!items.isEmpty ())
			if(Object* first = items.at (0))
			{
				items.removeAt (0);
				first->release ();
			}
		}
		else if(msg == ActionJournal::kMerged)
		{
			Action* action = unknown_cast<Action> (msg[0].asUnknown ());
			for(int i = 0; i < items.count (); i++)
			{
				ActionItem* item = (ActionItem*)items.at (i);
				if(item->getAction () == action)
				{
					item->setTitle (action->getDescription ());
					item->setDetails (action->getDetailedDescription ());
					signal (Message (IItemModel::kItemModified, i));
					return;
				}
			}
		}
		else if(msg == ActionJournal::kSquashed)
		{
			if(getItemView ())
			{
				// rebuild list
				viewVisible (false);
				viewVisible (true);				
			}		
			return;
		}

		signal (Message (kChanged));

		if(scrollToIndex != -1)
			if(getItemView ())
				getItemView ()->makeItemVisible (ItemIndex (scrollToIndex));
	}
	else if(msg == "makeItemVisible")
	{
		if(getItemView ())
			getItemView ()->makeItemVisible (ItemIndex (msg[0].asInt ()));
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionListModel::ActionItem* ActionListModel::createItem (Action* action) const
{
	ActionItem* item = NEW ActionItem;
	item->setAction (action);
	item->setTitle (action->getDescription ());
	item->setEnabled (true);
	item->setDetails (action->getDetailedDescription ());
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActionListModel::viewAttached (IItemView* itemView)
{
	SuperClass::viewAttached (itemView);
	viewVisible (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActionListModel::viewDetached (IItemView* itemView)
{
	viewVisible (false);
	SuperClass::viewDetached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionListModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (30);  // kIndex
	list.addColumn (24);  // kIndicator
	list.addColumn (60);  // kTime
	list.addColumn (200); // kDescription
	list.addColumn (300); // kDetails
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(!initDone)
	{
		colorizeIcons = ViewBox (info.view).getVisualStyle ().getMetric<bool> ("colorizeIcons", false);
		linePen = Pen (ViewBox (info.view).getTheme ().getThemeColor (ThemeElements::kSelectionColor), 3);
		textFont = ViewBox (info.view).getVisualStyle ().getTextFont ();
		initDone = true;

		if(colorizeIcons)
			bitmapPainter = ccl_new<IBitmapPainter> (ClassID::BitmapPainter);
	}

	ActionItem* item = (ActionItem*)resolve (index);
	if(!item)
		return false;

	int i = index.getIndex ();

	switch(column)
	{
	case kIndicator :
		if(!item->isIconChecked ())
		{
			AutoPtr<IUnknown> icon (item->getAction ()->createIcon ());
			item->setIcon (UnknownPtr<IImage> (icon));
			item->setIconChecked (true);
		}

		if(IImage* icon = item->getIcon ())
		{
			bool disabled = i >= undoCount;
			ClipSetter cs (info.graphics, info.rect);

			bool iconDrawn = false;
			if(colorizeIcons)
			{
				Variant isTemplate;
				UnknownPtr<IObject> (icon)->getProperty (isTemplate, IImage::kIsTemplate);
				if(isTemplate.asBool ())
				{
					Rect src (0, 0, icon->getWidth (), icon->getHeight ());
					Rect dst (src);
					dst.center (info.rect);

					ASSERT (bitmapPainter != nullptr)
					bitmapPainter->drawColorized (info.graphics, icon, src, dst, info.style.getTextBrush (!disabled).getColor ());
					iconDrawn = true;
				}
			}

			if(iconDrawn == false)
				drawIcon (info, icon, !disabled, false); // don't stretch the image!
		}
		break;

	case kIndex :
	case kTime:
	case kDetails :
		{
			String text;
			switch(column)
			{
			case kIndex   : text << i + 1; break;
			case kTime    : text << Format::DateTime::print (item->getAction ()->getTime (), Format::DateTime::kTime); break;
			case kDetails : text << item->getDetails (); break;
			}

			if(i >= undoCount)
				drawTitle (info, text, false, column == kDetails ? Font::kItalic : 0);
			else
				drawTitle (info, text);
		}
		break;

	case kDescription:
		{
			int fontStyle = 0;
			bool enabled = true;

			if(i >= undoCount)
			{
				fontStyle = Font::kItalic;
				enabled = false;
			}

			if(item->getAction ()->isDragable ())
				fontStyle |= Font::kUnderline;

			drawTitle (info, item->getTitle (), enabled, fontStyle);
		}
		break;
	}

	// separator between undo & redo stack
	if(i == undoCount - 1)
		info.graphics.drawLine (info.rect.getLeftBottom (), info.rect.getRightBottom (),linePen);
	else if(i == 0 && undoCount == 0)
		info.graphics.drawLine (info.rect.getLeftTop (), info.rect.getRightTop (), linePen);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ActionListModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	ActionItem* item = (ActionItem*)resolve (index);
	if(!item)
		return false;

	if(auto mouseEvent = info.editEvent.as<MouseEvent> ())
	{
		if(column == kIndicator)
		{
			// detect drag
			if(ViewBox (info.view).detectDrag (*mouseEvent))
			{
				getItemView ()->selectAll (false);
				getItemView ()->selectItem (index, true);
			}
			return false; // will drag selected items
		}
		else
			// create mouse handler
			getItemView ()->beginMouseHandler (NEW UndoMouseHandler (*this), *mouseEvent);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionListModel::setUndoPosition (int position)
{
	if(position >= undoCount) // redo
	{
		while(undoCount < position)
			if(!journal.redo ())
				break;
	}
	else if(position < undoCount)
	{
		while(undoCount > position)
			if(!journal.undo ())
				break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ActionListModel::getItemIcon (ItemIndexRef index)
{
	ActionItem* item = (ActionItem*)resolve (index);
	if(!item)
		return nullptr;

	if(item->getIcon () == nullptr)
	{
		ASSERT (initDone == true)
		AutoPtr<IImage> icon = createTextIcon (item->getTitle (), textFont);
		item->setIcon (icon);
	}
	return item->getIcon ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ActionListModel::createDragSessionData (ItemIndexRef index)
{
	ActionItem* item = (ActionItem*)resolve (index);
	if(item)
	{
		ASSERT (item->getAction ())
		return item->getAction ()->createDragObject ();
	}
	return nullptr;
}

//************************************************************************************************
// ActionJournalComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ActionJournalComponent, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournalComponent::ActionJournalComponent (ActionJournal& journal, StringRef name)
: Component (name.isEmpty () ? String ("ActionJournal") : name),
  journal (journal)
{
	actionList = NEW ActionListModel (*this, journal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActionJournalComponent::~ActionJournalComponent ()
{
	actionList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ActionJournalComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "actionList")
		return ccl_as_unknown (actionList);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActionJournalComponent::runDialog ()
{
	IView* view = getTheme ()->createView ("CCL/ActionJournalDialog", this->asUnknown ());
	ASSERT (view != nullptr)
	if(view)
		DialogBox ()->runDialog (view, Styles::kWindowCombinedStyleDialog, Styles::kCloseButton);
}
