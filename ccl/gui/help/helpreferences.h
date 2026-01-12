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
// Filename    : ccl/gui/help/helpreferences.h
// Description : Help References
//
//************************************************************************************************

#ifndef _ccl_helpreferences_h
#define _ccl_helpreferences_h

#include "ccl/public/gui/framework/ihelpmanager.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/storableobject.h"

namespace CCL {

class HelpCatalog;

//************************************************************************************************
// HelpReference
//************************************************************************************************

class HelpReference: public Object
{
public:
	DECLARE_CLASS (HelpReference, Object)

	HelpReference (StringRef helpid = nullptr);

	PROPERTY_POINTER (HelpCatalog, catalog, Catalog)
	PROPERTY_STRING (helpid, HelpIdentifier)
	PROPERTY_STRING (filename, FileName)
	PROPERTY_STRING (destination, Destination)

	// Object
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// HelpCatalog
//************************************************************************************************

class HelpCatalog: public StorableObject,
				   public IHelpCatalog
{
public:
	DECLARE_CLASS (HelpCatalog, StorableObject)

	HelpCatalog ();

	PROPERTY_SHARED_AUTO (Url, path, Path)
	PROPERTY_VARIABLE (int, priority, Priority)
	PROPERTY_BOOL (primary, Primary)
	PROPERTY_STRING (contentLanguage, ContentLanguage) ///< comma-separated list of languages
	PROPERTY_BOOL (quickHelp, QuickHelp)

	void setCategory (StringID category);
	void addShared (const HelpCatalog& other);
	void setDefaultReference (const HelpReference& reference);

	const HelpReference* lookup (StringRef helpid) const;
	const HelpReference& getDefaultReference () const;

	#if DEBUG
	void dump ();
	#endif

	// IHelpCatalog
	StringRef CCL_API getTitle () const override;
	StringID CCL_API getCategory () const override;

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IHelpCatalog, StorableObject)

protected:
	mutable String title;
	MutableCString category;
	PackageInfo attributes;
	HelpReference defaultReference;
	ObjectArray references;
};

} // namespace CCL

#endif // _ccl_helpreferences_h
