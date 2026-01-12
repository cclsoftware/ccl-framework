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
// Filename    : ccl/app/presets/presetdescriptor.h
// Description : Preset Descriptor
//
//************************************************************************************************

#ifndef _ccl_presetdescriptor_h
#define _ccl_presetdescriptor_h

#include "ccl/base/storage/persistence/dataitem.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/base/istream.h"

namespace CCL {

interface IAttributeList;

//************************************************************************************************
// PresetDescriptor
/** Lightweight preset description that can be stored in a DataStore. */
//************************************************************************************************

class PresetDescriptor: public DataItem,
						public IPresetDescriptor
{
public:
	DECLARE_CLASS (PresetDescriptor, DataItem)
	DECLARE_PROPERTY_NAMES (PresetDescriptor)

	PresetDescriptor ();

	void initWithPreset (const IPreset& preset, UrlRef url, const DateTime& date);
	void applySubFolder (IPreset& preset);

	PROPERTY_STRING (category, Category)
	PROPERTY_STRING (classID, ClassID)
	PROPERTY_STRING (vendor, Vendor)
	PROPERTY_STRING (creator, Creator)
	PROPERTY_STRING (generator, Generator)
	PROPERTY_STRING (subFolder, SubFolder)

	void assignMetaInfo (IAttributeList& metaInfo);
	bool hasData () const;

	// IPersistentObject
	void CCL_API storeMembers (Persistence::IObjectState& state) const override;
	void CCL_API restoreMembers (Persistence::IObjectState& state) override;

	// IPresetDescriptor
	StringRef CCL_API getPresetName () override;
	IStream* CCL_API getData () override;

	CLASS_INTERFACE (IPresetDescriptor, DataItem)

protected:
	AutoPtr<IStream> data;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool PresetDescriptor::hasData () const { return data != nullptr; }

} // namespace CCL

#endif // _ccl_presetdescriptor_h
