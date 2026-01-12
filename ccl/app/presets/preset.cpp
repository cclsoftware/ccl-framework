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
// Filename    : ccl/app/presets/preset.cpp
// Description : Preset
//
//************************************************************************************************

#include "ccl/app/presets/preset.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/app/presetmetainfo.h"

using namespace CCL;

//************************************************************************************************
// Preset
//************************************************************************************************

DEFINE_CLASS (Preset, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Preset::Preset (StringRef name, int flags, IUnknown* _data)
: name (name),
  flags (flags),
  data (nullptr)
{
	if(_data)
		setData (_data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Preset::~Preset ()
{
	if(data)
		data->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Preset::checkName (IAttributeList& metaInfo)
{
	// check title against filename
	PresetMetaAttributes metaAttribs (metaInfo);
	String title (metaAttribs.getTitle ());
	if(name != title)
	{
		// check if filename is just the "legalized" title
		if(!title.isEmpty () && name.contains (CCLSTR ("_")))
		{
			LegalFileName validTitle (title);
			if(name == validTitle)
			{
				name = title;
				return;
			}
		}
		metaAttribs.setTitle (name); // user has renamed the file: override title
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Preset::setNameFromMetaInfo ()
{
	if(IAttributeList* metaInfo = getMetaInfo ())
	{
		PresetMetaAttributes metaAttr (*metaInfo);
		name = metaAttr.getTitle ();
		if(name.isEmpty ())
			name = metaAttr.getClassName ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Preset::isReadOnly () const
{
	return readOnly ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Preset::isModified () const
{
	return modified ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Preset::getPresetName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Preset::getUserData () const
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Preset::setUserData (IUnknown* _data)
{
	setData (_data);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Preset::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "presetName")
	{
		var = getPresetName ();
		var.share ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// PresetHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetHandler, Object)

