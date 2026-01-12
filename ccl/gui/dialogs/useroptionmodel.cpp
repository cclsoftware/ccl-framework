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
// Filename    : ccl/gui/dialogs/useroptionmodel.cpp
// Description : User Option Model
//
//************************************************************************************************

#include "ccl/gui/dialogs/useroptionmodel.h"

#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/controls/tabview.h"

#include "ccl/app/params.h"
#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/framework/ialert.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("UserOption")
	XSTRING (General, "General")
	XSTRING (AskSaveChanges, "You have made changes to the setup. Do you want to apply the changes?")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

static IUserOption* findLastSelected (IUserOptionList& optionList)
{
	StringRef lastSelected (optionList.getLastSelected ());
	if(lastSelected.isEmpty ())
		return nullptr;

	IUserOption* selected = nullptr;
	for(int i = 0; i < optionList.countOptions (); i++)
	{
		IUserOption* option = optionList.getOption (i);
		if(option->getName () == lastSelected)
		{
			selected = option;
			break;
		}
	}
	return selected;
}

//************************************************************************************************
// OptionItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OptionItem, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionItem::OptionItem (StringRef title)
: ObjectNode (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionRoot* OptionItem::getRoot ()
{
	OptionItem* parent = getParentNode<OptionItem> ();
	if(parent)
		return parent->getRoot ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionItem* OptionItem::getItem (int index) const
{
	return (OptionItem*)getChildren ().at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* OptionItem::getIcon () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionItem::opened ()
{
	ForEach (getChildren (), OptionItem, item)
		item->opened ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionItem::closed ()
{
	ForEach (getChildren (), OptionItem, item)
		item->closed ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionItem::needsApply () const
{
	ForEach (getChildren (), OptionItem, item)
		if(item->needsApply ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionItem::apply ()
{
	ForEach (getChildren (), OptionItem, item)
		item->apply ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionItem::createView (const Rect& bounds)
{
	return nullptr;
}

//************************************************************************************************
// OptionRoot
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OptionRoot, OptionItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionRoot::OptionRoot (StringRef title)
: OptionItem (title),
  selected (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionRoot* OptionRoot::getRoot ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::categorize (String& category, String& page, StringRef title)
{
	if(title.contains (IUserOption::strSeparator))
	{
		category = title.subString (0, title.index (IUserOption::strSeparator));
		page = title.subString (title.lastIndex (IUserOption::strSeparator) + 1);
	}
	else
	{
		category = title;
		page.empty ();
	}

	if(category.isEmpty ())
		category = XSTR (General);
	if(page.isEmpty ())
		page = XSTR (General);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::build (IUserOptionList& optionList)
{
	setTitle (optionList.getTitle ());
	setHelpIdentifier (optionList.getName ());

	for(int i = 0; i < optionList.countOptions (); i++)
	{
		IUserOption* option = optionList.getOption (i);

		// determine category and page by option title
		String categoryTitle, pageTitle;
		categorize (categoryTitle, pageTitle, option->getTitle ());

		// add option to page
		OptionCategory& category = getCategory (categoryTitle);
		OptionPage& page = category.getPage (pageTitle);
		page.addOption (option);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::restoreSelected (IUserOptionList& optionList)
{
	OptionPage* selectedPage = nullptr;

	IUserOption* selectedOption = findLastSelected (optionList);
	if(selectedOption != nullptr)
	{
		// find page for option
		ForEach (getChildren (), OptionCategory, category)
			ForEach (*category, OptionPage, page)
				if(page->contains (selectedOption))
				{
					selectedPage = page;
					break;
				}
			EndFor
			if(selectedPage)
				break;
		EndFor
	}

	if(selectedPage)
		selectPage (selectedPage);
	else
		select ((OptionCategory*)getItem (0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::storeSelected (IUserOptionList& optionList)
{
	String lastSelected;

	if(OptionPage* page = getSelectedPage ())
		if(IUserOption* option = page->getFirstOption ())
			lastSelected = option->getName ();
	
	optionList.setLastSelected (lastSelected);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory& OptionRoot::getCategory (StringRef title)
{
	OptionCategory* category = findChildNode<OptionCategory> (title);
	if(category == nullptr)
		addChild (category = NEW OptionCategory (title));
	return *category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::select (OptionCategory* category)
{
	if(category != selected)
	{
		// check apply state
		if(optionView && needsApply ())
		{
			if(Alert::ask (XSTR (AskSaveChanges)) == Alert::kYes)
				apply ();
		}

		// switch category
		selected = category;
		updateView ();

		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory* OptionRoot::getSelected () const
{
	return selected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OptionRoot::getSelectedIndex () const
{
	if(selected == nullptr)
		return -1;
	return getChildren ().index (selected);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionPage* OptionRoot::getSelectedPage () const
{
	return selected ? selected->getSelected () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::selectPage (OptionPage* page)
{
	OptionCategory* category = page ? page->getCategory () : nullptr;
	select (category);
	if(category)
		category->select (page);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionRoot::needsApply () const
{
	if(selected)
		return selected->needsApply ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::apply ()
{
	if(selected)
		selected->apply ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionRoot::createView (const Rect& bounds)
{
	ASSERT (optionView == nullptr)
	optionView = NEW View (bounds);
	updateView ();
	return optionView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionRoot::getOptionView () const
{
	return optionView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionRoot::updateView ()
{
	if(optionView == nullptr)
		return;

	optionView->removeAll ();

	if(selected)
	{
		Rect bounds;
		optionView->getClientRect (bounds);

		View* view = selected->createView (bounds);
		ASSERT (view != nullptr)
		if(view)
			optionView->addView (view);
	}
}

//************************************************************************************************
// OptionCategory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OptionCategory, OptionItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory::OptionCategory (StringRef title)
: OptionItem (title),
  tabParam (NEW ListParam)
{
	tabParam->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory::~OptionCategory ()
{
	tabParam->removeObserver (this);
	tabParam->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* OptionCategory::getIcon () const
{
	ForEach (getChildren (), OptionItem, item)
		IImage* icon = item->getIcon ();
		if(icon)
			return icon;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionPage& OptionCategory::getPage (StringRef title)
{
	OptionPage* page = findChildNode<OptionPage> (title);
	if(page == nullptr)
	{
		addChild (page = NEW OptionPage (title));
		if(UnknownPtr<IListParameter> tabListParam = tabParam->asUnknown ())
			tabListParam->appendString (title);
	}
	return *page;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionCategory::select (OptionPage* page)
{
	int index = getChildren ().index (page);
	ASSERT (index != -1)
	tabParam->setValue (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionPage* OptionCategory::getSelected () const
{
	int index = tabParam->getValue ();
	return (OptionPage*)getItem (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionCategory::createView (const Rect& bounds)
{
	if(countChildren () == 1)
	{
		OptionItem* first = getItem (0);
		return first->createView (bounds);
	}
	else
	{
		View* tabView = NEW TabView (bounds, tabParam);
		tabView->setSizeMode (View::kAttachAll);

		ForEach (getChildren (), OptionItem, item)
			Rect clientRect (0, 0, bounds.getWidth (), bounds.getHeight ());
			BoxLayoutView* frame = NEW BoxLayoutView (clientRect, Styles::kVertical);
			frame->setSizeMode (View::kAttachAll);
			frame->setTitle (item->getTitle ());
			
			clientRect.contract (frame->getMargin ());
			clientRect.moveTo (Point ());
			View* view = item->createView (clientRect);
			ASSERT (view != nullptr)
			if(view)
				frame->addView (view);

			tabView->addView (frame);
		EndFor

		return tabView;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OptionCategory::notify (ISubject* subject, MessageRef msg)
{
	if(subject == tabParam && msg == kChanged)
	{
		// delegate to root
		OptionRoot* root = getRoot ();
		if(root)
			root->signal (Message (kChanged));	
	}
}

//************************************************************************************************
// OptionPage
//************************************************************************************************

DEFINE_CLASS_HIDDEN (OptionPage, OptionItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionPage::OptionPage (StringRef title)
: OptionItem (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionPage::~OptionPage ()
{
	VectorForEach (options, IUserOption*, option)
		ISubject::removeObserver (option, this);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory* OptionPage::getCategory ()
{
	return getParentNode<OptionCategory> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionPage::addOption (IUserOption* option)
{
	options.add (option);
	ISubject::addObserver (option, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserOption* OptionPage::getFirstOption () const
{
	return options.at (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionPage::contains (IUserOption* option) const
{
	return options.contains (option);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* OptionPage::getIcon () const
{
	VectorForEach (options, IUserOption*, option)
		IImage* icon = option->getIcon ();
		if(icon)
			return icon;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionPage::opened ()
{
	VectorForEach (options, IUserOption*, option)
		option->opened ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionPage::closed ()
{
	VectorForEach (options, IUserOption*, option)
		option->closed ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionPage::needsApply () const
{
	VectorForEach (options, IUserOption*, option)
		if(option->needsApply ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OptionPage::apply ()
{
	VectorForEach (options, IUserOption*, option)
		if(option->needsApply ())
			option->apply ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API OptionPage::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		// delegate to root
		OptionRoot* root = getRoot ();
		if(root)
			root->signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionPage::createOptionView (IUserOption* option)
{
	UnknownPtr<IViewFactory> factory (option);
	View* view = factory ? unknown_cast<View> (factory->createView ("Options", Variant (), Rect ())) : nullptr;
	ASSERT (view != nullptr)
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* OptionPage::createView (const Rect& bounds)
{
	if(options.count () == 1)
	{
		View* view = createOptionView (options.at (0));
		if(view)
		{
			view->setSizeMode (View::kAttachAll);
			view->setSize (bounds);
		}
		return view;
	}
	else
	{
		BoxLayoutView* pageView = NEW BoxLayoutView (bounds, StyleFlags (Styles::kVertical, Styles::kLayoutUnifySizes));
		pageView->setMargin (0);
		pageView->setTitle (getTitle ());
		pageView->setSizeMode (View::kAttachAll);

		VectorForEach (options, IUserOption*, option)
			View* view = createOptionView (option);
			if(view)
			{
				view->setSizeMode (View::kAttachLeft|View::kAttachRight);
				Rect rect (0, 0, bounds.getWidth (), view->getHeight ());
				view->setSize (rect);
				pageView->addView (view);
			}
		EndFor

		return pageView;
	}
}
