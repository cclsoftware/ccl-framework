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
// Filename    : ccl/gui/skin/skinregistry.h
// Description : Skin Registry
//
//************************************************************************************************

#ifndef _ccl_skinregistry_h
#define _ccl_skinregistry_h

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/stringdictionary.h"

namespace CCL {

class Url;
class View;
class SkinWizard;

interface IAttributeList;

/** If enabled, skin paths can be overwritten by property file on GUI designer system. This is used in release build. */
#define SKIN_DEVELOPMENT_LOCATIONS_ENABLED (RELEASE && CCL_PLATFORM_DESKTOP)

/** Framework skin identifier. */
extern const CString kFrameworkSkinID;

//************************************************************************************************
// FormReference
//************************************************************************************************

struct FormReference
{
	MutableCString id;
	MutableCString scope;
	MutableCString name;

	FormReference (StringRef path = nullptr);

	void getPath (MutableCString& path) const;
};

//************************************************************************************************
// SkinOverlay
//************************************************************************************************

class SkinOverlay: public Object
{
public:
	PROPERTY_OBJECT (FormReference, target, Target)
	PROPERTY_OBJECT (FormReference, source, Source)
};

//************************************************************************************************
// SkinRegistry
//************************************************************************************************

class SkinRegistry: public Object,
					public Singleton<SkinRegistry>
{
public:
	~SkinRegistry ();

	struct ImportContext
	{
		ImportContext (StringID originalId);
		~ImportContext ();
	};

	void loadDevelopmentLocations ();
	bool getDevelopmentLocation (Url& path, StringRef skinID) const;

	void addSearchLocation (UrlRef folder); ///< add a global location where skins files (e.g. for imports) can be found
	void getSearchLocations (Container& folderUrls); ///< adds Url objects to container

	void addSkin (SkinWizard* skin);
	void removeSkin (SkinWizard* skin);
	
	SkinWizard* getSkin (StringID skinID) const;	
	SkinWizard* getModuleSkin (ModuleRef module) const;
	SkinWizard* getApplicationSkin () const;

	SkinOverlay* addOverlay (StringRef target, StringRef source);
	void removeOverlay (SkinOverlay* overlay);

	View* createView (StringID path, IUnknown* controller, IAttributeList* arguments) const;
	View* createView (const FormReference& reference, IUnknown* controller, IAttributeList* arguments) const;

protected:
	ObjectArray skins;
	ObjectArray overlays;
	MutableCString currentImportId;
	StringDictionary developmentLocations;
	String developmentProfileName;
	ObjectArray searchLocations;

	friend class Singleton<SkinRegistry>;
	SkinRegistry ();

	void resolveID (FormReference& r) const;
};

} // namespace CCL

#endif // _ccl_skinregistry_h
