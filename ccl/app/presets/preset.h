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
// Filename    : ccl/app/presets/preset.h
// Description : Preset
//
//************************************************************************************************

#ifndef _ccl_preset_h
#define _ccl_preset_h

#include "ccl/base/object.h"

#include "ccl/public/app/ipreset.h"

namespace CCL {

//************************************************************************************************
// Preset
/** Base class for presets. */
//************************************************************************************************

class Preset: public Object,
			  public AbstractPreset
{
public:
	DECLARE_CLASS (Preset, Object)

	Preset (StringRef name = nullptr, int flags = 0, IUnknown* data = nullptr);
	~Preset ();

	enum Flags
	{
		kReadOnly = 1<<0,
		kModified = 1<<1
	};

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, kReadOnly, readOnly)
	PROPERTY_FLAG (flags, kModified, modified)
	PROPERTY_STRING (name, Name)
	PROPERTY_SHARED (IUnknown, data, Data)

	// IPreset
	tbool CCL_API isReadOnly () const override;
	tbool CCL_API isModified () const override;
	StringRef CCL_API getPresetName () const override;
	IUnknown* CCL_API getUserData () const override;
	tbool CCL_API setUserData (IUnknown* data) override;

	CLASS_INTERFACE (IPreset, Object)

protected:
	void checkName (IAttributeList& metaInfo); ///< check title against filename
	void setNameFromMetaInfo ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// PresetHandler
/** Base class for preset handlers. */
//************************************************************************************************

class PresetHandler: public Object,
					 public AbstractPresetFileHandler
{
public:
	DECLARE_CLASS (PresetHandler, Object)

	CLASS_INTERFACE (IPresetFileHandler, Object)
};

} // namespace CCL

#endif // _ccl_preset_h
