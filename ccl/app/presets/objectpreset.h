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
// Filename    : ccl/app/presets/objectpreset.h
// Description : Object Preset
//
//************************************************************************************************

#ifndef _ccl_objectpreset_h
#define _ccl_objectpreset_h

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/app/presets/preset.h"

#include "ccl/public/storage/istorage.h"

namespace CCL {

interface IClassDescription;

//************************************************************************************************
// ObjectPreset
/** Transfers preset data between two objects that implement IStorable or are CCL::Objects. */
//************************************************************************************************

class ObjectPreset: public Preset
{
public:
	DECLARE_CLASS (ObjectPreset, Preset)

	ObjectPreset (const IClassDescription* description = nullptr);	///< takes meta info from class description (no IStorable)
	ObjectPreset (IPresetMediator& presetMediator);				///< mediator provides meta info and storable object
	ObjectPreset (IAttributeList& _metaInfo);					///< copies provided meta info
	~ObjectPreset ();

	bool isOnlyClass () const;
	IUnknown* getSource () const;

	PROPERTY_OBJECT (Url, url, PresetUrl)

	// Preset
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;

private:
	IAttributeList* metaInfo;
	AutoPtr<IStorable> sourceStorable;
	AutoPtr<Object> sourceObject;
	bool onlyClass;
};

//************************************************************************************************
// ObjectPresetCollection
/** Transfers preset collections between objects that implement IPresetCollector. */
//************************************************************************************************

class ObjectPresetCollection: public Preset,
							  public IPresetCollection
{
public:
	DECLARE_CLASS (ObjectPresetCollection, Preset)

	ObjectPresetCollection (IPresetCollector& collector);
	~ObjectPresetCollection ();

	// IPreset
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API restore (IUnknown* target) const override;

	// IPresetCollection
	int CCL_API countPresets () override;
	IPreset* CCL_API openPreset (int index) override;
	IPreset* CCL_API openPreset (const IStringDictionary& parameters) override;
	IPreset* CCL_API createPreset (IAttributeList& metaInfo) override;
	IStream* CCL_API openStream (StringRef path, int mode) override;

	CLASS_INTERFACE (IPresetCollection, Preset)

private:
	IAttributeList* metaInfo;
	ObjectArray presets;
	ObjectArray streams;
	class Stream;

	ObjectPresetCollection ();
};

//************************************************************************************************
// PresetScriptUtils
//************************************************************************************************

class PresetScriptUtils
{
public:
	static IPreset* createPresetFromArgument (VariantRef arg);
};

} // namespace CCL

#endif // _ccl_objectpreset_h
