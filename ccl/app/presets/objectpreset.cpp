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
// Filename    : ccl/app/presets/objectpreset.cpp
// Description : Object Preset
//
//************************************************************************************************

#include "ccl/app/presets/objectpreset.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// ObjectPreset
//************************************************************************************************

DEFINE_CLASS (ObjectPreset, Preset)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPreset::ObjectPreset (IPresetMediator& presetMediator)
: Preset (String::kEmpty, kReadOnly),
  metaInfo (NEW Attributes),
  onlyClass (false)
{
	presetMediator.getPresetMetaInfo (*metaInfo);
	presetMediator.storePreset (*this);
	setNameFromMetaInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPreset::ObjectPreset (IAttributeList& _metaInfo)
: Preset (String::kEmpty, kReadOnly),
  metaInfo (NEW Attributes),
  onlyClass (false)
{
	metaInfo->copyFrom (_metaInfo);
	setNameFromMetaInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPreset::ObjectPreset (const IClassDescription* description)
: Preset (String::kEmpty, kReadOnly),
  metaInfo (NEW Attributes),
  onlyClass (true)
{
	if(description)
	{
		PresetMetaAttributes (*metaInfo).assign (*description);
		setNameFromMetaInfo ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPreset::~ObjectPreset ()
{
	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectPreset::isOnlyClass () const
{
	return onlyClass; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ObjectPreset::getSource () const
{
	if(sourceStorable)
		return sourceStorable;
	else if(sourceObject)
		return sourceObject->asUnknown ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API ObjectPreset::getMetaInfo () const
{
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectPreset::getUrl (IUrl& url) const
{
	url.assign (this->url);
	return !url.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectPreset::store (IUnknown* target)
{
	UnknownPtr<IStorable> storable (target);
	if(storable)
	{
		// keep the storable interface for a later restore
		sourceStorable.share (storable);
	}
	else if(Object* object = unknown_cast<Object> (target))
	{
		// keep the object pointer for a later restore
		sourceObject.share (object);
	}
	else
		return false;

	onlyClass = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectPreset::restore (IUnknown* target) const
{
	if(sourceStorable)
	{
		UnknownPtr<IStorable> targetStorable (target);
		if(targetStorable)
		{
			// transfer the preset data through a memory stream
			MemoryStream memStream;
			if(sourceStorable->save (memStream))
			{
				memStream.rewind ();
				return targetStorable->load (memStream);
			}
		}
	}
	else if(sourceObject)
	{
		if(Object* targetObject = unknown_cast<Object> (target))
		{
			// transfer the preset data through an attribute list
			Attributes a;
			Storage storage (a);
			if(sourceObject->save (storage))
				return targetObject->load (storage);
		}
	}
	else
		return onlyClass;

	return false;
}


//************************************************************************************************
// ObjectPresetCollection::Stream
//************************************************************************************************

class ObjectPresetCollection::Stream: public Object
{
public:
	Stream (StringRef p)
	: path (p),
	  stream (NEW MemoryStream)
	{}
	String path;
	AutoPtr<MemoryStream> stream;
};

//************************************************************************************************
// ObjectPresetCollection
//************************************************************************************************

DEFINE_CLASS (ObjectPresetCollection, Preset)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPresetCollection::ObjectPresetCollection (IPresetCollector& collector)
: Preset (String::kEmpty, kReadOnly),
  metaInfo (NEW Attributes)
{
	presets.objectCleanup ();
	streams.objectCleanup ();

	UnknownPtr<IPresetMediator> mediator (&collector);
	if(mediator)
		mediator->getPresetMetaInfo (*metaInfo);

	setNameFromMetaInfo ();

	collector.save (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPresetCollection::ObjectPresetCollection ()
: Preset (String::kEmpty, kReadOnly),
  metaInfo (nullptr)
{
	presets.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectPresetCollection::~ObjectPresetCollection ()
{
	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API ObjectPresetCollection::getMetaInfo () const
{
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ObjectPresetCollection::countPresets ()
{
	return presets.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API ObjectPresetCollection::openPreset (int index)
{
	ObjectPreset* preset = (ObjectPreset*)presets.at (index);
	if(preset)
		preset->retain ();
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API ObjectPresetCollection::openPreset (const IStringDictionary& parameters)
{
	CCL_NOT_IMPL ("ObjectPresetCollection: Open preset with parameters not implemented!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API ObjectPresetCollection::createPreset (IAttributeList& metaInfo)
{
	ObjectPreset* preset = NEW ObjectPreset (metaInfo);
	presets.add (preset);
	preset->retain ();
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectPresetCollection::restore (IUnknown* target) const
{
	UnknownPtr<IPresetCollector> collector (target);
	if(collector)
		return collector->load (*const_cast<ObjectPresetCollection*> (this));
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API ObjectPresetCollection::openStream (StringRef path, int mode)
{
	Stream* stream = nullptr;
	for(int i = 0; i < streams.count (); i++)
	{
		Stream* s = (Stream*)streams.at (i);
		if(s->path == path)
		{
			stream = s;
			break;
		}
	}

	if(mode & (IStream::kWriteMode | IStream::kCreate))
	{
		if(stream == nullptr)
		{
			stream = NEW Stream (path);
			streams.add (stream);
		}
	}

	if(auto result = stream ? stream->stream : nullptr)
	{
		result->rewind ();
		result->retain ();
		return result;
	}

	return nullptr;	
}

//************************************************************************************************
// PresetScriptUtils
//************************************************************************************************

IPreset* PresetScriptUtils::createPresetFromArgument (VariantRef arg)
{
	bool isPresetCollection = false;

	// 1.) preset object
	UnknownPtr<IPreset> preset (arg.asUnknown ());
	if(!preset)
	{
		// 2.) class ID: create object preset
		UIDBytes cid = CCL::Boxed::UID::fromVariant (arg);
		if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
		{
			AutoPtr<ObjectPreset> objectPreset = NEW ObjectPreset (description);
			preset = objectPreset->asUnknown ();
		}
	}
	return preset.detach ();
}
