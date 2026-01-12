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
// Filename    : ccl/gui/views/viewdecorator.cpp
// Description : View Decorator
//
//************************************************************************************************

#include "ccl/gui/views/viewdecorator.h"
#include "ccl/gui/theme/thememanager.h"

using namespace CCL;

//************************************************************************************************
// ViewDecorator
//************************************************************************************************

ViewDecorator::ViewDecorator (View* contentView, StringID decorFormName, IUnknown* decorController)
: contentView (contentView),
  decorFormName (decorFormName)
{
	setDecorController (decorController);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ViewDecorator::getDecorArguments ()
{
	return decorArguments;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamList& ViewDecorator::getParamList ()
{
	return paramList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewDecorator::decorateView (ITheme& theme)
{
	IUnknown* controller = getDecorController () ? getDecorController () : asUnknown ();

	View* view = unknown_cast<View> (theme.createView (getDecorFormName (), controller, &decorArguments));
	if(!view)
	{
		ITheme* appTheme = ThemeManager::instance ().getApplicationTheme ();
		if(appTheme && appTheme != &theme)
			view = unknown_cast<View> (appTheme->createView (getDecorFormName (), controller, &decorArguments));
	}
		
	return view ? view : contentView.as_plain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewDecorator::getProperty (Variant& var, MemberID propertyId) const
{
	MutableCString arrayKey;
	if(propertyId.getBetween (arrayKey, "hasParam[", "]"))
	{
		var = findParameter (arrayKey) != nullptr;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ViewDecorator::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Content" && contentView)
	{
		contentView->release ();
		return contentView.detach ();
	}
	return nullptr;
}
