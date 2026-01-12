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
// Filename    : ccl/app/presets/memorypreset.cpp
// Description : Memory Preset
//
//************************************************************************************************

#include "ccl/app/presets/memorypreset.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/protocolhandler.h"

#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// PresetCategory
//************************************************************************************************

class PresetCategory: public Object
{
public:
	DECLARE_CLASS (PresetCategory, Object)

	PresetCategory (StringRef name = nullptr);
	~PresetCategory ();

	PROPERTY_STRING (name, Name)

	void addPreset (MemoryPreset* preset);
	bool removePreset (StringRef name);
	MemoryPreset* getPreset (StringRef name) const;
	Iterator* newIterator () { return presets.newIterator (); }

	IFileSystem* getFileSystem ();

	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

private:
	ObjectList presets;
	IFileSystem* fileSys;
};

static const FileType MemoryPresetType ("Memory Preset", "mempreset", CCL_MIME_TYPE "-memorypreset");

//************************************************************************************************
// MemoryPresetIterator
//************************************************************************************************

class MemoryPresetIterator: public Object,
							public IFileIterator
{
public:
	MemoryPresetIterator (PresetCategory& category, int mode = IFileIterator::kAll)
	: iterator (category.newIterator ())
	{
		// todo: mode
		current.setUrl (nullptr, IUrl::kFile);
		current.setProtocol (CCLSTR ("preset"));
		current.setHostName (category.getName ());
	}

	// IFileIterator
	const IUrl* CCL_API next () override
	{
		MemoryPreset* preset = ccl_cast<MemoryPreset> (iterator->next ());
		if(preset)
		{
			current.setPath (preset->getPresetName ());
			current.setFileType (MemoryPresetType);
			return &current;
		}
		return nullptr;
	}

	CLASS_INTERFACE (IFileIterator, Object)

private:
	Url current;
	AutoPtr<Iterator> iterator;
};

//************************************************************************************************
// PresetFileSystem
//************************************************************************************************

class PresetFileSystem: public Unknown,
						public AbstractFileSystem
{
public:
	PresetFileSystem (PresetCategory& category)
	: category (category)
	{}

	// IFileSystem
	IFileIterator* CCL_API newIterator (UrlRef url, int mode = IFileIterator::kAll) override
	{
		if(url.getPath ().isEmpty () && url.getProtocol () == CCLSTR ("preset"))
			return NEW MemoryPresetIterator (category, mode);
		return nullptr;
	}

	tbool CCL_API removeFile (UrlRef url, int mode = 0) override
	{
		ASSERT (url.getProtocol () == CCLSTR ("preset"))
		return category.removePreset (url.getPath ());
	}

	CLASS_INTERFACE (IFileSystem, Unknown)

private:
	PresetCategory& category;
};

//************************************************************************************************
// MemoryPresetProtocol
//************************************************************************************************

class MemoryPresetProtocol: public Object,
							public Singleton<MemoryPresetProtocol>
{
public:
	class Handler: public ProtocolHandler
	{
	public:

		StringRef CCL_API getProtocol () const override
		{
			static const String protocol = CCLSTR ("preset");
			return protocol;
		}

		IFileSystem* CCL_API getMountPoint (StringRef name) override
		{
			PresetCategory* category = MemoryPresetHandler::instance ().getCategory (name, false);
			return category ? category->getFileSystem () : nullptr;
		}
	};

	MemoryPresetProtocol ()
	: handler (NEW Handler)
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		ASSERT (registry != nullptr)
		if(registry)
			registry->registerProtocol (handler);
	}

	~MemoryPresetProtocol ()
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		if(registry)
			registry->unregisterProtocol (handler);
	}

protected:
	AutoPtr<Handler> handler;
};

} // namespace CCL

using namespace CCL;

DEFINE_SINGLETON (MemoryPresetProtocol)

//************************************************************************************************
// PresetCategory
//************************************************************************************************

DEFINE_CLASS (PresetCategory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetCategory::PresetCategory (StringRef name)
: name (name),
  fileSys (nullptr)
{
	presets.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetCategory::~PresetCategory ()
{
	if(fileSys)
		fileSys->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetCategory::load (const Storage& storage)
{
	Attributes& attr = storage.getAttributes ();
	attr.get (name, "name");
	while(MemoryPreset* preset = attr.unqueueObject<MemoryPreset> ("presets"))
		addPreset (preset);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetCategory::save (const Storage& storage) const
{
	Attributes& attr = storage.getAttributes ();
	attr.set ("name", name);
	ForEach (presets, MemoryPreset, p)
		attr.queue ("presets", p);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryPreset* PresetCategory::getPreset (StringRef name) const
{
	ForEach (presets, MemoryPreset, p)
		if(p->getName () == name)
			return p;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetCategory::addPreset (MemoryPreset* preset)
{
	presets.add (preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetCategory::removePreset (StringRef name)
{
	if(MemoryPreset* preset = getPreset (name))
	{
		presets.remove (preset);
		preset->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* PresetCategory::getFileSystem ()
{
	if(!fileSys)
		fileSys = NEW PresetFileSystem (*this);
	return fileSys;
}

//************************************************************************************************
// MemoryPreset
//************************************************************************************************

DEFINE_CLASS (MemoryPreset, Preset)

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryPreset::MemoryPreset (IAttributeList* _metaInfo)
{
	if(Attributes* attribs = unknown_cast<Attributes> (_metaInfo))
	{
		if(PackageInfo* pkgInfo = ccl_cast<PackageInfo> (attribs))
			metaInfo.share (pkgInfo);
		else
		{
			metaInfo = NEW PackageInfo;
			metaInfo->copyFrom (*attribs);
		}
	}
	else
		metaInfo = NEW PackageInfo;

	name = PresetMetaAttributes (*metaInfo).getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryPreset::load (const Storage& storage)
{
	Attributes& attr = storage.getAttributes ();

	if(Attributes* pkgAttribs = attr.getAttributes ("metaInfo"))
		metaInfo->load (Storage (*pkgAttribs));

	data.share (attr.getObject<Attributes> ("data"));

	name = PresetMetaAttributes (*metaInfo).getTitle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MemoryPreset::save (const Storage& storage) const
{
	Attributes& attr = storage.getAttributes ();

	AutoPtr<Attributes> pkgAttribs (NEW Attributes);
	if(metaInfo->save (Storage (*pkgAttribs)))
		attr.set ("metaInfo", pkgAttribs, Attributes::kShare);

	attr.set ("data", data, Attributes::kShare);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API MemoryPreset::getMetaInfo () const
{
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPreset::getUrl (IUrl& url) const
{
	if(metaInfo)
	{
		MemoryPresetHandler::makeCategoryUrl (url, metaInfo);
		url.descend (getName ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPreset::store (IUnknown* target)
{
	data = NEW Attributes;

	Object* object = unknown_cast<Object> (target);
	if(object)
		return object->save (Storage (*data));

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPreset::restore (IUnknown* target) const
{
	Object* object = unknown_cast<Object> (target);
	if(object && data)
		return object->load (Storage (*data));

	return false;
}

//************************************************************************************************
// MemoryPresetHandler
//************************************************************************************************

DEFINE_UNMANAGED_SINGLETON (MemoryPresetHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryPresetHandler::MemoryPresetHandler ()
: settings (nullptr)
{
	MemoryPresetProtocol::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryPresetHandler::~MemoryPresetHandler ()
{
	if(settings)
	{
		settings->flush ();
		settings->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& MemoryPresetHandler::getSettings ()
{
	if(!settings)
	{
		settings = NEW XmlSettings (CCLSTR ("Presets"));
		settings->restore ();
	}
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryPresetHandler::makeCategoryUrl (IUrl& url, StringRef category)
{
	url.setUrl (nullptr, IUrl::kFolder);
	url.setProtocol (CCLSTR ("preset"));
	url.setHostName (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MemoryPresetHandler::makeCategoryUrl (IUrl& url, IAttributeList* metaInfo)
{
	// get category from  metaInfo
	ASSERT (metaInfo)
	{
		PresetMetaAttributes metaAttributes (*metaInfo);
		String category (metaAttributes.getCategory ());
		ASSERT (!category.isEmpty ())
		if(category.isEmpty ())
			category = CCLSTR ("(Unknown Category)");

		makeCategoryUrl (url, category);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPresetHandler::getWriteLocation (IUrl& url, IAttributeList* metaInfo)
{
	makeCategoryUrl (url, metaInfo);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPresetHandler::getReadLocation (IUrl& url, IAttributeList* metaInfo, int index)
{
	if(index == 0)
		return getWriteLocation (url, metaInfo);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API MemoryPresetHandler::getFileType ()
{
	return MemoryPresetType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MemoryPresetHandler::canHandle (IUnknown* target)
{
	return unknown_cast<Object> (target) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MemoryPresetHandler::getFlags ()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetCategory* MemoryPresetHandler::getCategory (StringRef categoryName, bool create)
{
	Settings::Section* section = getSettings ().getSection (categoryName, create);
	if(section)
	{
		Attributes& attribs = section->getAttributes ();
		PresetCategory* category = attribs.getObject<PresetCategory> ("category");
		if(!category && create)
		{
			category = NEW PresetCategory (categoryName);
			attribs.set ("category", category, Attributes::kOwns);
		}
		return category;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API MemoryPresetHandler::openPreset (UrlRef url, IPresetDescriptor* descriptor)
{
	if(url.getProtocol () == CCLSTR ("preset"))
		if(PresetCategory* category = getCategory (url.getHostName (), false))
		{
			if(MemoryPreset* preset = category->getPreset (url.getPath ()))
			{
				preset->retain ();
				return preset;
			}
		}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API MemoryPresetHandler::createPreset (UrlRef url, IAttributeList& metaInfo)
{
	MemoryPreset* preset = NEW MemoryPreset (&metaInfo);

	PresetCategory* category = getCategory (url.getHostName (), true);
	category->addPreset (preset);
	preset->retain ();
	return preset;
}
