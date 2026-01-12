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
// Filename    : core/portable/gui/corealertbox.cpp
// Description : Alert box
//
//************************************************************************************************

#include "corealertbox.h"
#include "corecontrols.h"
#include "coreviewbuilder.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// AlertView
//************************************************************************************************

class AlertView: public ContainerView,
				 public ViewController,
				 public IParamObserver
{
public:
	BEGIN_CORE_CLASS ('AleV', AlertView)
		ADD_CORE_CLASS_ (ViewController)
		ADD_CORE_CLASS_ (IParamObserver)
	END_CORE_CLASS (ContainerView)

	AlertView (const AlertBox& alertBox, AlertID id, IAlertCompletionHandler* handler);
	~AlertView ();

	enum Tags { kFirstTag = 1, kSecondTag, kCountTag };

	PROPERTY_VARIABLE (AlertID, id, ID)
	PROPERTY_POINTER (IAlertCompletionHandler, handler, Handler)

	// ViewController
	View* createView (CStringPtr type) override;
	void* getObjectForView (CStringPtr name, CStringPtr type) override;

	// IParamObserver
	void paramChanged (Parameter* p, int msg) override;

protected:
	AlertDescription description;
	NumericParam firstButton;
	NumericParam secondButton;
	NumericParam buttonCount;

	static const ParamInfo firstButtonInfo;
	static const ParamInfo secondButtonInfo;
	static const ParamInfo buttonCountInfo;
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// AlertBox
//************************************************************************************************

RootView* AlertBox::theRootView = nullptr;
void AlertBox::setRootView (RootView* rootView)
{
	theRootView = rootView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr AlertBox::getButtonTitle (int result)
{
	switch(result)
	{
	case kYes : return "Yes";
	case kNo : return "No";
	case kOk : return "OK";
	case kCancel : return "Cancel";
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::initButtons (int firstResult, int secondResult)
{
	setFirstResult (firstResult);
	setFirstButton (getButtonTitle (firstResult));
	setSecondResult (secondResult);
	setSecondButton (getButtonTitle (secondResult));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::runAsync (AlertID id, IAlertCompletionHandler* handler)
{
	ASSERT (theRootView != nullptr)
	if(theRootView)
	{
		AlertView* alertView = NEW AlertView (*this, id, handler);

		// center in root view
		Rect alertSize;
		alertView->getClientRect (alertSize);
		Rect rootSize;
		theRootView->getClientRect (rootSize);
		alertSize.center (rootSize);
		alertView->setSize (alertSize);

		theRootView->setModalView (alertView);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::showOk (CStringPtr text, CStringPtr secondaryText, AlertID id, IAlertCompletionHandler* handler)
{
	AlertBox alert;
	alert.setText (text);
	alert.setSecondaryText (secondaryText);
	alert.initButtons (AlertBox::kOk);
	alert.runAsync (id, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::showOkCancel (CStringPtr text, CStringPtr secondaryText, AlertID id, IAlertCompletionHandler* handler)
{
	AlertBox alert;
	alert.setText (text);
	alert.setSecondaryText (secondaryText);
	alert.initButtons (AlertBox::kOk, AlertBox::kCancel);
	alert.runAsync (id, handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::showYesNo (CStringPtr text, CStringPtr secondaryText, AlertID id, IAlertCompletionHandler* handler)
{
	AlertBox alert;
	alert.setText (text);
	alert.setSecondaryText (secondaryText);
	alert.initButtons (AlertBox::kYes, AlertBox::kNo);
	alert.runAsync (id, handler);
}

//************************************************************************************************
// AlertView
//************************************************************************************************

const ParamInfo AlertView::firstButtonInfo = PARAM_TOGGLE (AlertView::kFirstTag, "firstButton", 0, "", 0);
const ParamInfo AlertView::secondButtonInfo = PARAM_TOGGLE (AlertView::kSecondTag, "secondButton", 0, "", 0);
const ParamInfo AlertView::buttonCountInfo = PARAM_INT (AlertView::kCountTag, "buttonCount", 0, 1, 0, "", nullptr, 0, 0);

AlertView::AlertView (const AlertBox& alertBox, AlertID id, IAlertCompletionHandler* handler)
: id (id),
  handler (handler),
  description (alertBox),
  firstButton (firstButtonInfo),
  secondButton (secondButtonInfo),
  buttonCount (buttonCountInfo)
{
	wantsFocus (true);
	wantsTouch (true);

	firstButton.setController (this);
	secondButton.setController (this);
	buttonCount.setValue (ParamValue (description.getSecondButton ().isEmpty () ? 0 : 1));

	ViewBuilder::instance ().buildView (*this, "Standard.AlertBox", this);

	// assign text and button titles
	if(View* view = findView (ViewNameFilter ("text")))
	{
		// is it a Label or a MultiLineLabel?
		if(Label* label = core_cast<Label> (view))
			label->setTitle (description.getText ());
		else if(MultiLineLabel* label = core_cast<MultiLineLabel> (view))
			label->setTitle (description.getText ());
		else
		{
			ASSERT (false); // what type of View is it??
		}
	}
	if(View* secondaryLabel = findView (ViewNameFilter ("secondaryText")))
	{
		// is it a Label or a MultiLineLabel?
		if(Label* label = core_cast<Label> (secondaryLabel))
			label->setTitle (description.getSecondaryText ());
		else if(MultiLineLabel* label = core_cast<MultiLineLabel> (secondaryLabel))
			label->setTitle (description.getSecondaryText ());
		else
		{
			ASSERT (false); // what type of View is it??
		}
	}
	if(Button* button = core_cast<Button> (findView (ViewNameFilter ("firstButton"))))
		button->setTitle (description.getFirstButton ());
	if(!description.getSecondButton ().isEmpty ())
		if(Button* button = core_cast<Button> (findView (ViewNameFilter ("secondButton"))))
			button->setTitle (description.getSecondButton ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertView::~AlertView ()
{
	removeAll (); // remove before parameter dtor!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* AlertView::createView (CStringPtr type)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* AlertView::getObjectForView (CStringPtr _name, CStringPtr type)
{
	if(ConstString (type) == kParamType)
	{
		ConstString name (_name);
		if(name == "firstButton")
			return &firstButton;
		if(name == "secondButton")
			return &secondButton;
		if(name == "buttonCount")
			return &buttonCount;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertView::paramChanged (Parameter* p, int msg)
{
	if(msg == Parameter::kEdit)
	{
		if(p->getTag () == kFirstTag || p->getTag () == kSecondTag)
		{
			// if the operation takes some time, don't let them click again.
			firstButton.enable (false);
			secondButton.enable (false);

			if(handler)
			{
				int result = p->getTag () == kFirstTag ? description.getFirstResult () : description.getSecondResult ();
				handler->onAlertCompleted (id, result);
			}

			if(RootView* rootView = getRootView ())
			{
				ASSERT (rootView->getModalView () == this)
				rootView->resetModalViewDeferred ();
			}
		}
	}
}
