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
// Filename    : ccl/gui/dialogs/useroptiondialog.cpp
// Description : User Option Dialog
//
//************************************************************************************************

#include "ccl/gui/dialogs/useroptiondialog.h"
#include "ccl/gui/dialogs/useroptionmodel.h"
#include "ccl/gui/dialogs/dialogbuilder.h"

#include "ccl/gui/controls/button.h"

#include "ccl/app/params.h"

using namespace CCL;

//************************************************************************************************
// UserOptionDialog
//************************************************************************************************

DEFINE_CLASS (UserOptionDialog, Object)
DEFINE_CLASS_UID (UserOptionDialog, 0x6e0c7b4, 0x73e, 0x4187, 0xa5, 0x30, 0xa0, 0xdf, 0x8d, 0xf6, 0x5, 0x26)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionDialog::UserOptionDialog ()
: optionRoot (*NEW OptionRoot),
  defaultIcon (FrameworkTheme::instance ().getImage (ThemeNames::kUserOptionIcon)),
  applyButton (nullptr),
  optionHeader (nullptr),
  listParam (NEW ListParam (CSTR ("optionList"))),
  nextListParam (nullptr),
  visibleList (nullptr)
{
	paramList.setController (this);
	paramList.add (listParam);

	optionHeader = paramList.addString (CSTR ("optionHeader"));
	nextListParam = paramList.addParam (CSTR ("nextOptionList"));

	optionRoot.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserOptionDialog::~UserOptionDialog ()
{
	optionRoot.removeObserver (this);
	optionRoot.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserOptionDialog::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (IUserOptionDialog)
	QUERY_INTERFACE (IController)
	QUERY_INTERFACE (IParamObserver)
	QUERY_INTERFACE (IViewFactory)
	QUERY_INTERFACE (IDialogButtonInterest)
	QUERY_INTERFACE (IItemModel)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserOptionDialog::run (IUserOptionList& optionList)
{
	IUserOptionList* lists[1] = {&optionList};
	return run (lists, 1, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserOptionDialog::run (IUserOptionList* lists[], int count, int index)
{
	// add lists
	for(int i = 0; i < count; i++)
	{
		optionLists.add (lists[i]);
		lists[i]->retain (); // keep a reference, in case application restarts from options dialog
		listParam->appendString (lists[i]->getTitle ());
	}

	Theme& theme = FrameworkTheme::instance ();
	View* view = unknown_cast<View> (theme.createView ("UserOptionDialog", this->asUnknown ()));
	ASSERT (view != nullptr)
	if(view)
	{
		IUserOptionList* selected = lists[index];
		ASSERT (selected != nullptr)
		showList (selected);

		DialogBuilder builder;
		builder.setTheme (theme);
		int result = builder.runDialog (view);
		if(result == DialogResult::kOkay)
		{
			if(optionRoot.needsApply ())
				optionRoot.apply ();
		}

		showList (nullptr);
	}

	// remove lists
	VectorForEach (optionLists, IUserOptionList*, list)
		list->release ();
	EndFor
	optionLists.removeAll ();
	listParam->removeAll ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionDialog::showList (IUserOptionList* list)
{
	if(list == visibleList)
		return;

	// cleanup
	if(visibleList)
	{
		// save name of last last selected option
		optionRoot.storeSelected (*visibleList);

		optionRoot.select (nullptr);
		optionRoot.closed ();
		optionRoot.removeAll ();
	}

	visibleList = list;

	// prepare
	if(visibleList)
	{
		optionRoot.build (*visibleList);
		optionRoot.opened ();

		// restore selected option
		optionRoot.restoreSelected (*visibleList);
	}

	// update
	updateApply ();
	updateWindow ();
	updateNextButton ();

	int listIndex = visibleList ? optionLists.index (visibleList) : 0;
	listParam->setValue (listIndex);

	signal (Message (kChanged)); // IItemModel

	if(getItemView ())
		getItemView ()->selectItem (ItemIndex (optionRoot.getSelectedIndex ()), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserOptionList* UserOptionDialog::getNextList () const
{
	int index = visibleList ? optionLists.index (visibleList) : 0;
	if(index + 1 < optionLists.count ())
		index++;
	else
		index = 0;

	return optionLists.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UserOptionDialog::getObject (StringID name, UIDRef classID)
{
	if(name == "OptionList")
		return this->asUnknown ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOptionDialog::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isSimple") // is there a single category only?
	{
		auto isSimple = [&] ()
		{
			if(optionLists.count () != 1)
				return false;

			String lastCategory;
			IUserOptionList* list = optionLists.first ();
			for(int i = 0; i < list->countOptions (); i++)
			{
				String category, page;
				OptionRoot::categorize (category, page, list->getOption (i)->getTitle ());
				if(lastCategory.isEmpty ())
					lastCategory = category;
				else if(category != lastCategory)
					return false;
			}		
			return true;
		};

		var = isSimple ();
		return true;
	}

	if(propertyId == "isMultiple") // are multiple alternative option lists available?
	{
		var = optionLists.count () > 1;
		return true;
	}

	static const CString kOptionName ("optionName");
	if(propertyId.startsWith (kOptionName))
	{
		int64 index = 0;
		CString (propertyId.str () + kOptionName.length ()).getIntValue (index);

		IUserOptionList* list = optionLists.at ((int)index - 1);
		ASSERT (list != nullptr)
		if(list == nullptr)
			return false;

		var = list->getName ();
		var.share ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOptionDialog::paramChanged (IParameter* param)
{
	if(param == listParam)
	{
		int index = listParam->getValue ();
		showList (optionLists.at (index));
	}
	else if(param == nextListParam)
	{
		showList (getNextList ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API UserOptionDialog::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "OptionView")
	{
		return optionRoot.createView (bounds);
	}

	if(name == "OptionListButton")
	{
		nextListButton = NEW Button (Rect (), nextListParam);
		updateNextButton ();
		return nextListButton;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionDialog::updateApply ()
{
	if(applyButton)
		applyButton->enable (optionRoot.needsApply ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionDialog::updateWindow ()
{
	String title;
	String helpid;

	if(OptionCategory* category = optionRoot.getSelected ())
	{
		title = category->getTitle ();
		
		if(OptionPage* page = category->getSelected ())
			if(IUserOption* first = page->getFirstOption ())
				helpid = first->getName ();
	}

	optionHeader->fromString (title);

	if(!helpid.isEmpty ())
		helpid << ";";
	helpid << optionRoot.getHelpIdentifier ();

	if(View* view = optionRoot.getOptionView ())
		if(Window* window = view->getWindow ())
		{
			window->setHelpIdentifier (helpid);
			window->setTitle (optionRoot.getTitle ());
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserOptionDialog::updateNextButton ()
{
	String title;
	IUserOptionList* nextList = getNextList ();
	if(nextList)
	{
		//title << "<< ";
		title << nextList->getTitle ();
	}

	if(nextListButton)
		nextListButton->setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOptionDialog::setDialogButton (IParameter* button, int which)
{
	if(which == DialogResult::kApply)
	{
		applyButton = button;

		updateApply ();

		updateWindow (); // window should exist at this stage!
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOptionDialog::onDialogButtonHit (int which)
{
	if(which == DialogResult::kApply)
	{
		optionRoot.apply ();

		updateApply ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserOptionDialog::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &optionRoot)
	{
		updateApply ();
		updateWindow ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IItemModel
//////////////////////////////////////////////////////////////////////////////////////////////////

OptionCategory* UserOptionDialog::getCategory (ItemIndexRef index)
{
	return (OptionCategory*)optionRoot.getItem (index.getIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UserOptionDialog::countFlatItems ()
{
	return optionRoot.countChildren ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOptionDialog::getItemTitle (String& title, ItemIndexRef index)
{
	OptionCategory* category = getCategory (index);
	if(category == nullptr)
		return false;

	title = category->getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API UserOptionDialog::getItemIcon (ItemIndexRef index)
{
	OptionCategory* category = getCategory (index);
	IImage* icon = category ? category->getIcon () : nullptr;
	return icon ? icon : defaultIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserOptionDialog::onItemFocused (ItemIndexRef index)
{
	optionRoot.select (getCategory (index));
	return true;
}
