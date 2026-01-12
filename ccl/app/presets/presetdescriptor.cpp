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
// Filename    : ccl/app/presets/presetdescriptor.cpp
// Description : Preset Descriptor
//
//************************************************************************************************

#include "ccl/app/presets/presetdescriptor.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/base/memorystream.h"

using namespace CCL;
using namespace Persistence;

//************************************************************************************************
// PresetDescriptor
//************************************************************************************************

DEFINE_CLASS (PresetDescriptor, DataItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (PresetDescriptor)
	DEFINE_PROPERTY_TYPE ("category",	ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("classID",	ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("vendor",		ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("creator",	ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("generator",	ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("subFolder",	ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("data",		ITypeInfo::kBlob)
END_PROPERTY_NAMES (PresetDescriptor)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetDescriptor::PresetDescriptor ()
: data (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetDescriptor::initWithPreset (const IPreset& preset, UrlRef url, const DateTime& modifiedTime)
{
	IAttributeList* metaInfo = preset.getMetaInfo ();
	ASSERT (metaInfo)

	setLastModified (modifiedTime);
	assignMetaInfo (*metaInfo);
	this->url = url;

	preset.toDescriptor (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetDescriptor::applySubFolder (IPreset& preset)
{
	// re-apply subFolder
	if(!getSubFolder ().isEmpty ())
		if(IAttributeList* metaInfo = preset.getMetaInfo ())
			PresetMetaAttributes (*metaInfo).setSubFolder (getSubFolder ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetDescriptor::storeMembers (IObjectState& state) const
{
	SuperClass::storeMembers (state);

	state.set ("category", category);
	state.set ("classID", classID);
	state.set ("vendor", vendor);
	state.set ("creator", creator);
	state.set ("generator", generator);
	state.set ("subFolder", subFolder);
	state.set ("data", Variant (data));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetDescriptor::restoreMembers (IObjectState& state)
{
	SuperClass::restoreMembers (state);

	category = state.get ("category");
	classID = state.get ("classID");
	vendor = state.get ("vendor");
	creator = state.get ("creator");
	generator = state.get ("generator");
	subFolder = state.get ("subFolder");

	UnknownPtr<IStream> stream (state.get ("data").asUnknown ());
	data.share (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetDescriptor::assignMetaInfo (IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttribs (metaInfo);

	title = metaAttribs.getTitle ();

	category = metaAttribs.getCategory ();
	classID = metaAttribs.getString (Meta::kClassID);
	vendor = metaAttribs.getVendor ();
	creator = metaAttribs.getCreator ();
	generator = metaAttribs.getGenerator ();
	subFolder = metaAttribs.getSubFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API PresetDescriptor::getPresetName ()
{
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API PresetDescriptor::getData ()
{
	if(!data)
		data = NEW MemoryStream;
	return data;
}
