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
// Filename    : ccl/public/app/presetmetainfo.h
// Description : Preset Meta Information
//
//************************************************************************************************

#ifndef _ccl_presetmetainfo_h
#define _ccl_presetmetainfo_h

#include "ccl/public/app/ipresetmetainfo.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/metainfo.h"
#include "ccl/public/text/istringdict.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugins/iclassfactory.h"

namespace CCL {

//************************************************************************************************
// PresetMetaAttributes
/**	\ingroup app_inter */
//************************************************************************************************

class PresetMetaAttributes: public AttributeAccessor
{
public:
	PresetMetaAttributes (IAttributeList& attributes)
	: AttributeAccessor (attributes) 
	{}

	// Common Attributes
	METAINFO_ATTRIBUTE_STRING (DataFile, Meta::kPresetDataFile)
	METAINFO_ATTRIBUTE_STRING (DataMimeType, Meta::kPresetDataMimeType)
	METAINFO_ATTRIBUTE_STRING (SubFolder, Meta::kPresetSubFolder)

	METAINFO_ATTRIBUTE_UID    (ClassID, Meta::kClassID)
	METAINFO_ATTRIBUTE_STRING (ClassName, Meta::kClassName)
	METAINFO_ATTRIBUTE_STRING (Vendor, Meta::kClassVendor)
	METAINFO_ATTRIBUTE_STRING (Category, Meta::kClassCategory)
	METAINFO_ATTRIBUTE_STRING (SubCategory, Meta::kClassSubCategory)
	METAINFO_ATTRIBUTE_UID    (AlternativeClassID, Meta::kAlternativeClassID)
	METAINFO_ATTRIBUTE_UID    (FallbackClassID, Meta::kFallbackClassID)

	METAINFO_ATTRIBUTE_STRING (MimeType, Meta::kDocumentMimeType)
	METAINFO_ATTRIBUTE_STRING (Generator, Meta::kDocumentGenerator)
	METAINFO_ATTRIBUTE_STRING (Creator, Meta::kDocumentCreator)
	METAINFO_ATTRIBUTE_STRING (Title, Meta::kDocumentTitle)
	METAINFO_ATTRIBUTE_STRING (Description, Meta::kDocumentDescription)
	METAINFO_ATTRIBUTE_STRING (TypeDescription, Meta::kPresetTypeDescription)
	// todo: date

	void assign (const IClassDescription& description)
	{
		setClassID (description.getClassID ());
		setClassName (description.getName ());
		setCategory (description.getCategory ());
		setSubCategory (description.getSubCategory ());

		Variant vendor;
		if(description.getClassAttribute (vendor, Meta::kClassVendor))
			setVendor (vendor);
		else
			setVendor (description.getModuleVersion ().getVendor ());
	}

	bool isSimilar (const PresetMetaAttributes& other) const
	{
		String classIdString (getString (Meta::kClassID));
		if(!classIdString.isEmpty ())
			return classIdString == other.getString (Meta::kClassID);
		else
			return getCategory () == other.getCategory ();
	}

	String getClassKey () const
	{
		// classID or category
		String classIdString (getString (Meta::kClassID));
		if(!classIdString.isEmpty ())
			return classIdString;
		else
			return getCategory ();
	}
};

//************************************************************************************************
// PresetUrl methods
/**	\ingroup app_inter */
//************************************************************************************************

struct PresetUrl
{
	static void setSubPresetIndex (IUrl& url, int presetIndex)
	{
		url.getParameters ().setEntry ("preset", String () << presetIndex);
	}

	static int getSubPresetIndex (UrlRef url)
	{
		String presetString (url.getParameters ().lookupValue ("preset"));
		int64 presetIndex = 0;
		if(presetString.getIntValue (presetIndex))
			return (int)presetIndex;

		return -1;
	}

	static void removeSubPresetIndex (IUrl& url)
	{
		url.getParameters ().removeEntry ("preset");
	}
};

} // namespace CCL

#endif // _ccl_presetmetainfo_h
