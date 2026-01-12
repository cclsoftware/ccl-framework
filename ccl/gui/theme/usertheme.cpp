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
// Filename    : ccl/gui/theme/usertheme.cpp
// Description : UserTheme class
//
//************************************************************************************************

#include "ccl/gui/theme/usertheme.h"

#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skinmodel.h"

#include "ccl/gui/views/view.h"
#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/storage/iattributelist.h"

using namespace CCL;

//************************************************************************************************
// UserTheme
//************************************************************************************************

const FileType& UserTheme::getFileType ()
{
	return SkinWizard::getSkinFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (UserTheme, Theme)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserTheme::UserTheme (StringID themeID, ITranslationTable* table, ModuleRef module)
{
	// we always need a translation table!
	AutoPtr<ITranslationTable> tempTable;
	if(table == nullptr)
	{
		tempTable = System::CreateTranslationTable ();
		table = tempTable;
	}

	skinWizard = NEW SkinWizard (themeID, this, table);
	skinWizard->setModuleReference (module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserTheme::~UserTheme ()
{
	skinWizard->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserTheme::queryInterface (UIDRef iid, void** ptr)
{
	// make skin model accessible
	if(iid == ccl_iid<ISkinModel> ())
		return skinWizard->getModel ().queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API UserTheme::getThemeID () const
{
	return skinWizard->getSkinID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef UserTheme::getModuleReference () const
{
	return skinWizard->getModuleReference ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserTheme::load (UrlRef path)
{
	return skinWizard->loadSkin (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserTheme::reload (bool keepImages)
{
	return skinWizard->reloadSkin (keepImages);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UserTheme::getResource (StringID name)
{
	return static_cast<IObject*> (skinWizard->getRoot ().getResource (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API UserTheme::getGradient (StringID name)
{
	return skinWizard->getRoot ().getGradient (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API UserTheme::getImage (StringID name)
{
	// always lookup images from root, independent of selected scope!
	return skinWizard->getRoot ().getImage (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API UserTheme::createView (StringID name, IUnknown* controller, IAttributeList* arguments)
{
	// apply outer zoom factor (e.g. from the wizard for another module)
	float zoomFactor = ThemeSelector::currentTheme ? ThemeSelector::currentTheme->getZoomFactor () : getZoomFactor ();
	Theme::ZoomFactorScope zoomScope (*this, zoomFactor);

	// copy variables from another skin
	Attributes outerVariables;
	if(ThemeSelector::currentTheme && ThemeSelector::currentTheme != this)
		ThemeSelector::currentTheme->getVariables (outerVariables);

	ThemeSelector selector (*this);
	
	SkinArgumentScope scope1 (*skinWizard, &outerVariables);
	SkinArgumentScope scope2 (*skinWizard, arguments); // push arguments
	return skinWizard->createView (name, controller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTheme::getVariables (IAttributeList& list) const
{
	skinWizard->getVariables (list);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserTheme::setZoomFactor (float factor)
{
	skinWizard->setZoomFactor (factor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float UserTheme::getZoomFactor () const
{
	return skinWizard->getZoomFactor ();
}
