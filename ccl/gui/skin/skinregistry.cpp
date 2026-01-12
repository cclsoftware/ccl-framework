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
// Filename    : ccl/gui/skin/skinregistry.cpp
// Description : Skin Registry
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/gui/skin/skinregistry.h"
#include "ccl/gui/skin/skinwizard.h"

#include "ccl/gui/views/view.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/propertyfile.h"
#include "ccl/base/development.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

const CString CCL::kFrameworkSkinID = "cclgui";

CCL_KERNEL_INIT_LEVEL (SkinManager, kFrameworkLevelSecond - 1)
{
	// SkinRegistry must be created before ThemeManager to avoid shutdown issues.
	SkinRegistry::instance ();
	return true;
}

//************************************************************************************************
// FormReference
//************************************************************************************************

FormReference::FormReference (StringRef _path)
{
	if(_path.isEmpty ())
		return;

	Url path (_path);
	this->id = path.getHostName ();

	String pathName;
	path.getPathName (pathName);
	this->scope = pathName;

	String name;
	path.getName (name);
	this->name = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FormReference::getPath (MutableCString& path) const
{
	path.empty ();

	path += "://";
	path += id;
	path += "/";

	if(!scope.isEmpty ())
	{
		path += scope;
		path += "/";
	}

	path += name;
}

//************************************************************************************************
// SkinRegistry::ImportContext
//************************************************************************************************

SkinRegistry::ImportContext::ImportContext (StringID originalId)
{
	SkinRegistry::instance ().currentImportId = originalId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinRegistry::ImportContext::~ImportContext ()
{
	SkinRegistry::instance ().currentImportId.empty ();
}

//************************************************************************************************
// SkinRegistry
//************************************************************************************************

DEFINE_SINGLETON (SkinRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinRegistry::SkinRegistry ()
{
	searchLocations.objectCleanup (true);

	// skins in ccl framework folder
	Url frameworkSkins;
	GET_DEVELOPMENT_FOLDER_LOCATION (frameworkSkins, CCL_FRAMEWORK_DIRECTORY, "skins")
	addSearchLocation (frameworkSkins);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinRegistry::~SkinRegistry ()
{
	ASSERT (skins.isEmpty () == true)
	ASSERT (overlays.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::loadDevelopmentLocations ()
{
	Url baseFolder;
	System::GetSystem ().getLocation (baseFolder, System::kUserDocumentFolder);

	Url path (baseFolder);
	path.descend ("ccl-skin-development.properties");
	Java::PropertyFile branchFile;
	branchFile.loadFromFile (path);
	developmentProfileName = branchFile.getProperties ().lookupValue ("profile");
	if(!developmentProfileName.isEmpty ())
	{
		path = baseFolder;
		path.descend (developmentProfileName, Url::kFolder);
		path.descend ("skins.properties");

		Java::PropertyFile skinsFile;
		if(skinsFile.loadFromFile (path))
			developmentLocations.copyFrom (skinsFile.getProperties ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinRegistry::getDevelopmentLocation (Url& path, StringRef skinID) const
{
	CCL_PRINT ("Get skin development location for ")
	CCL_PRINTLN (skinID)

	StringRef pathString = developmentLocations.lookupValue (skinID);
	if(!pathString.isEmpty ())
	{
		path.fromDisplayString (pathString, Url::kFolder);

		Url baseFolder;
		System::GetSystem ().getLocation (baseFolder, System::kUserDocumentFolder);
		baseFolder.descend (developmentProfileName, Url::kFolder);
		path.makeAbsolute (baseFolder);

		return File (path).exists ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::addSearchLocation (UrlRef folder)
{
	if(!folder.isEmpty ())
		searchLocations.addOnce (Url (folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::getSearchLocations (Container& folderUrls)
{
	folderUrls.add (searchLocations, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::addSkin (SkinWizard* skin)
{
	skins.add (skin);

	// activate existing overlays from previously loaded skins
	ArrayForEachFast (overlays, SkinOverlay, overlay)
		if(overlay->getTarget ().id == skin->getSkinID ())
			skin->addOverlay (overlay);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::removeSkin (SkinWizard* skin)
{
	skins.remove (skin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinWizard* SkinRegistry::getSkin (StringID skinID) const
{
	if(skinID.isEmpty () || skinID == IObjectTable::kHostApp)
	{
		return getApplicationSkin ();
	}
	else
	{
		ArrayForEachFast (skins, SkinWizard, skin)
			if(skin->getSkinID () == skinID)
				return skin;
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinWizard* SkinRegistry::getModuleSkin (ModuleRef module) const
{
	ASSERT (module != nullptr)
	if(module == nullptr)
		return nullptr;

	ArrayForEachFast (skins, SkinWizard, skin)
		if(skin->getModuleReference () == module)
			return skin;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinWizard* SkinRegistry::getApplicationSkin () const
{
	if(SkinWizard* mainModuleSkin = getModuleSkin (System::GetMainModuleRef ()))
		return mainModuleSkin;

	// if the main module is not ccl-based, fallback to the first skin
	ArrayForEachFast (skins, SkinWizard, skin)
		if(skin->getSkinID () != kFrameworkSkinID)
			return skin;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::resolveID (FormReference& r) const
{
	if(r.id == ISkinContext::kImportID)
		r.id = currentImportId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinOverlay* SkinRegistry::addOverlay (StringRef target, StringRef source)
{
	SkinOverlay* overlay = NEW SkinOverlay;
	FormReference targetReference (target);
	FormReference sourceReference (source);

	// resolve skin identifiers on import
	resolveID (targetReference);
	resolveID (sourceReference);

	overlay->setTarget (targetReference);
	overlay->setSource (sourceReference);
	overlays.add (overlay);

	// activate overlay
	SkinWizard* skin = getSkin (overlay->getTarget ().id);
	if(skin)
		skin->addOverlay (overlay);

	return overlay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinRegistry::removeOverlay (SkinOverlay* overlay)
{
	// deactivate overlay
	SkinWizard* skin = getSkin (overlay->getTarget ().id);
	if(skin)
		skin->removeOverlay (overlay);

	overlays.remove (overlay);
	overlay->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinRegistry::createView (StringID _path, IUnknown* controller, IAttributeList* arguments) const
{
	Url path (String (_path), Url::kFile);

	MutableCString skinID (path.getHostName ());
	SkinWizard* skin = getSkin (skinID);
	if(skin == nullptr)
	{
		CCL_PRINT ("Skin not found : ")
		CCL_PRINTLN (path.getHostName ())
		return nullptr;
	}

	MutableCString formName ("/");
	formName += path.getPath ();

	// apply outer zoom factor (e.g. from the wizard for another skin)
	float zoomFactor = ThemeSelector::currentTheme ? ThemeSelector::currentTheme->getZoomFactor () : skin->getZoomFactor ();
	Theme::ZoomFactorScope zoomScope (*skin->getTheme (), zoomFactor);

	// copy variables from another skin
	Attributes outerVariables;
	if(ThemeSelector::currentTheme && ThemeSelector::currentTheme != skin->getTheme ())
		ThemeSelector::currentTheme->getVariables (outerVariables);

	ThemeSelector selector (skin->getTheme ());

	SkinArgumentScope scope1 (*skin, &outerVariables);
	SkinArgumentScope scope2 (*skin, arguments); // push arguments
	return skin->createView (formName, controller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinRegistry::createView (const FormReference& reference, IUnknown* controller, IAttributeList* arguments) const
{
	MutableCString path;
	reference.getPath (path);
	return createView (path, controller, arguments);
}

