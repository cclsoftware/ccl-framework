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
// Filename    : ccl/app/components/eulacomponent.h
// Description : EULA Component
//
//************************************************************************************************

#include "ccl/app/components/eulacomponent.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum EULAComponentTags
	{
		kEULAText = 100
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("EULA")
	XSTRING (TOSTitle, "%(1) Terms of Service")
	XSTRING (EULATitle, "%(1) End User License Agreement")
END_XSTRINGS

//************************************************************************************************
// EULAComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EULAComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

CString EULAComponent::getAcceptedAttributeID ()
{
	MutableCString acceptedId ("accepted-");
	acceptedId += System::GetLocaleManager ().getLanguage ();
	return acceptedId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EULAComponent::EULAComponent (StringID _formName)
: Component (CCLSTR ("EULA")),
  formName (_formName.isEmpty () ? "EULADialog" : _formName)
{
	paramList.addString ("EULAText", Tag::kEULAText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EULAComponent::startup (const IUrl* defaultPath)
{
	Url licenseFolder;
	if(defaultPath && !defaultPath->isEmpty ())
		licenseFolder.assign (*defaultPath);
	else
	{
		System::GetSystem ().getLocation (licenseFolder, System::kAppDeploymentFolder);
		licenseFolder.descend ("license", Url::kFolder);
	}

	Url path (licenseFolder);
	path.descend ("EULA.txt");
	LocalizedUrl::localize (path, CCLSTR ("EULA")); // try to find a localized EULA

	return run (CCLSTR ("EULA"), path, RootComponent::instance ().getApplicationTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EULAComponent::run (StringRef id, UrlRef path, StringRef title, AgreementType type)
{
	if(checkAcceptedAndLoadText (id, path))
		return true;

	String dialogTitle = getDialogTitle (title, type);
	if(runDialog (dialogTitle) != DialogResult::kOkay)
		return false;

	StringID acceptedAttributeId = getAcceptedAttributeID ();
	Settings::instance ().getAttributes (id).set (acceptedAttributeId, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* EULAComponent::runAsync (StringRef id, UrlRef path, StringRef title, AgreementType type)
{
	if(checkAcceptedAndLoadText (id, path))
		return AsyncOperation::createCompleted (DialogResult::kOkay);

	AutoPtr<AsyncSequence> asyncs = NEW AsyncSequence ();
	asyncs->add ([&, this] () -> IAsyncOperation*
	{
		String dialogTitle = getDialogTitle (title, type);
		return runDialogAsync (dialogTitle);
	});

	asyncs->then ([id] (IAsyncOperation& operation)
	{
		int result = operation.getResult ().asInt ();
		if(result == DialogResult::kOkay || result == DialogResult::kApply)
		{
			StringID acceptedAttributeId = getAcceptedAttributeID ();
			Settings::instance ().getAttributes (id).set (acceptedAttributeId, true);
		}
	});

	return return_shared<IAsyncOperation> (asyncs->start ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int EULAComponent::runDialog (StringRef title)
{
	IView* view = createDialogView (title);
	if(view == nullptr)
		return DialogResult::kCancel;

	return DialogBox ()->runDialog (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* EULAComponent::runDialogAsync (StringRef title)
{
	IView* view = createDialogView (title);
	if(view == nullptr)
		return nullptr;

	return DialogBox ()->runDialogAsync (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EULAComponent::checkAcceptedAndLoadText (StringRef id, UrlRef path)
{
	StringID acceptedAttributeId = getAcceptedAttributeID ();
	Attributes& a = Settings::instance ().getAttributes (id);
	bool firstRun = !a.getBool (acceptedAttributeId);
	if(!firstRun)
		return true;

	String text = TextUtils::loadString (path);
	IParameter* textParam = paramList.byTag (Tag::kEULAText);
	textParam->fromString (text);
	if(text.isEmpty ())
	{
		a.set (acceptedAttributeId, true);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EULAComponent::getDialogTitle (StringRef title, AgreementType type) const
{
	if(type == kCustom)
		return title;

	String formatString (type == kTOS ? XSTR (TOSTitle) : XSTR (EULATitle));
	return String ().appendFormat (formatString, title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* EULAComponent::createDialogView (StringRef title)
{
	ITheme* theme = getTheme ();
	IView* view = theme->createView (formName, asUnknown ());
	if(!view)
	{
		ITheme* theme2 = System::GetThemeManager ().getApplicationTheme ();
		if(theme2 && theme2 != theme)
			view = theme2->createView (formName, asUnknown ());
	}

	ASSERT (view)
	if(!view)
		return nullptr;

	ViewBox (view).setTitle (title);

	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API EULAComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "EULATextClient")
	{
		String text;
		IParameter* textParam = paramList.byTag (Tag::kEULAText);
		textParam->toString (text);

		// determine font
		Font font;
		if(UnknownPtr<ISkinCreateArgs> args = data.asUnknown ())
			if(IVisualStyle* visualStyle = args->getVisualStyleForElement ())
				font = visualStyle->getTextFont ();

		Rect rect (bounds);
		Font::measureText (rect, bounds.getWidth (), text, font);
		rect.bottom += 100;

		ControlBox textBox (CCL::ClassID::TextBox, textParam, rect, StyleFlags (Styles::kTransparent, Styles::kTextBoxAppearanceMultiLine));
		return textBox;
	}
	return nullptr;
}
