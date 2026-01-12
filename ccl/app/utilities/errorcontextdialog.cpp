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
// Filename    : ccl/app/utilities/errorcontextdialog.cpp
// Description : Error Context List Dialog
//
//************************************************************************************************

#include "ccl/app/utilities/errorcontextdialog.h"

#include "ccl/app/component.h"
#include "ccl/app/controls/listviewmodel.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/ierrorhandler.h"

namespace CCL {
namespace Alert {

//************************************************************************************************
// ErrorContextDialog
//************************************************************************************************

class ErrorContextDialog: public Component
{
public:
	ErrorContextDialog ();

	bool run (CCL::IErrorContext* errorContext, StringRef headerText, StringRef question = nullptr, bool deep = false);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

private:
	AutoPtr<ListViewModel> eventList;

	enum Tags
	{
		kHeaderText = 'head',
		kQuestion = 'ques'
	};

	void addErrorEvents (ListViewModel& eventList, const IErrorContext& errorContext, bool deep = false);
};

} // namespace Alert
} // namespace CCL

using namespace CCL;
using namespace Alert;

//************************************************************************************************
// ErrorContextDialog
//************************************************************************************************

ErrorContextDialog::ErrorContextDialog ()
{
	paramList.addString ("headerText", kHeaderText);
	paramList.addString ("questionText", kQuestion);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ErrorContextDialog::getProperty (Variant& var, MemberID propertyId) const
{
	String string;
	if(propertyId == "hasHeader")
	{
		paramList.byTag (kHeaderText)->toString (string);
		var = string.isEmpty () == false;
		return true;
	}
	if(propertyId == "hasQuestion")
	{
		paramList.byTag (kQuestion)->toString (string);
		var = string.isEmpty () == false;
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextDialog::addErrorEvents (ListViewModel& eventList, const IErrorContext& errorContext, bool deep)
{
	int eventCount = errorContext.getEventCount ();
	for(int i = 0; i < eventCount; i++)
	{
		ListViewItem* item = NEW ListViewItem;
		item->setTitle (errorContext.getEvent (i).message);
		eventList.addItem (item);
	}
	if(deep)
	{
		int childCount = errorContext.getChildCount ();
		for(int j = 0; j < childCount; j++)
		{
			if(IErrorContext * childContext = errorContext.getChild (j))
				addErrorEvents (eventList, *childContext, deep);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool ErrorContextDialog::run (IErrorContext* errorContext, StringRef headerText, StringRef question, bool deep)
{
	if(eventList == nullptr && errorContext != nullptr)
	{
		eventList = NEW ListViewModel;
		addErrorEvents (*eventList, *errorContext, deep);
		addObject ("eventList", eventList->asUnknown ());
	}

	paramList.byTag (kHeaderText)->setValue (headerText);
	paramList.byTag (kQuestion)->setValue (question);

	ITheme* theme = ViewBox::getModuleTheme ();
	ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();

	IView* dialogView = theme->createView ("CCL/EventListDialog", this->asUnknown ());
	if(dialogView == nullptr && theme2 != theme)
		dialogView = theme2->createView ("CCL/EventListDialog", this->asUnknown ());

	if(dialogView)
		return DialogBox ()->runDialog (dialogView) == DialogResult::kOkay;

	return false;
}

//************************************************************************************************
// Alert
//************************************************************************************************

bool Alert::showErrorContextList (IErrorContext* errorContext, StringRef text, StringRef question, bool deep)
{
	AutoPtr<ErrorContextDialog> dlg = NEW ErrorContextDialog ();
	return dlg->run (errorContext, text, question, deep);
}
