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
// Filename    : ccl/app/presets/simplepreset.cpp
// Description : Simple Preset
//
//************************************************************************************************

#include "ccl/app/presets/simplepreset.h"
#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetsystem.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/xmltree.h"

#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// SimplePreset
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimplePreset, Preset)

//////////////////////////////////////////////////////////////////////////////////////////////////

SimplePreset::SimplePreset (UrlRef path, IAttributeList* metaInfo)
: path (path),
  metaInfo (metaInfo)
{
	if(metaInfo)
	{
		metaInfo->retain ();
		name = PresetMetaAttributes (*metaInfo).getTitle ();
	}
	else
		path.getName (name, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SimplePreset::~SimplePreset ()
{
	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::isReadOnly () const
{
	ASSERT (!readOnly ())
	return System::GetFileSystem ().isWriteProtected (path); // can be deleted by user
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API SimplePreset::getMetaInfo () const
{
	if(metaInfo == nullptr)
	{
		metaInfo = NEW Attributes;

		// We need a minimum set of meta information here
		// to identify ourselves inside the preset system
		PresetMetaAttributes attr (*metaInfo);
		attr.setCategory (getCategory ());
		attr.setClassName (getClassName ());
		attr.setTitle (getPresetName ());
	}
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::getUrl (IUrl& url) const
{
	url.assign (path);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::store (IUnknown* target)
{
	UnknownPtr<IStorable> storable (target);
	if(storable == nullptr)
		return false;

	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	if(stream == nullptr)
		return false;

	return storable->save (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::restore (IUnknown* target) const
{
	UnknownPtr<IStorable> storable (target);
	if(storable == nullptr)
		return false;

	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	if(stream == nullptr)
		return false;

	return storable->load (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::toDescriptor (IPresetDescriptor& descriptor) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePreset::fromDescriptor (IPresetDescriptor& descriptor)
{
	setName (descriptor.getPresetName ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimplePreset::queryInterface (UIDRef iid, void** ptr)
{
	// support query for IUrl to make IConvertFilters work even if url was converted to preset before
	if(CCL::ccl_iid<IUrl> ().equals (iid))
	{
		AutoPtr<Url> pathCopy (NEW Url (path));
		return pathCopy->queryInterface (iid, ptr);		
	}
	
	return SuperClass::queryInterface (iid, ptr);
}


//************************************************************************************************
// SimpleXmlPreset
//************************************************************************************************

SimpleXmlPreset::SimpleXmlPreset (UrlRef path, IAttributeList* metaInfo)
: SimplePreset (path, metaInfo)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API SimpleXmlPreset::getMetaInfo () const
{
	if(metaInfo == nullptr)
	{
		metaInfo = NEW Attributes;
		AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
		if(stream)
		{
			XmlTreeParser parser;
			parser.parse (*stream);
			XmlNode* rootNode = parser.getRoot ();
			if(rootNode && rootNode->getNameCString () == getRootName ())
			{
				UID cid;
				if(cid.fromCString (rootNode->getAttributeCString (getIDAttributeName ())))
				{
					PresetMetaAttributes metaAttribs (*metaInfo);
					metaAttribs.setTitle (getPresetName ());

					if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
						metaAttribs.assign (*description);
				}
			}
		}
	}
	return metaInfo;
}

//************************************************************************************************
// SimplePresetHandler
//************************************************************************************************

static const String kUserPresetFolder (CCLSTR ("User Presets"));

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef SimplePresetHandler::getUserPresetFolderName ()
{
	return kUserPresetFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimplePresetHandler::getFactoryFolder (IUrl& path, StringRef subFolder)
{
	PresetPackageHandler::instance ().getFactoryRootFolder (path);
	path.descend (kUserPresetFolder, IUrl::kFolder);
	if(!subFolder.isEmpty ())
		path.descend (subFolder, IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (SimplePresetHandler, PresetHandler)

ObjectArray SimplePresetHandler::simplePresetHandlers;

//////////////////////////////////////////////////////////////////////////////////////////////////

SimplePresetHandler* SimplePresetHandler::findHandler (const IAttributeList& metaInfo)
{
	String category (PresetMetaAttributes (ccl_const_cast (metaInfo)).getCategory ());

	for(auto handler : iterate_as<SimplePresetHandler> (simplePresetHandlers))
		if(handler->getPresetCategory () == category)
		{
			ASSERT (!metaInfo.contains (Meta::kClassID))
			return handler;
		}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SimplePresetHandler::SimplePresetHandler (const FileType& fileType, int flags)
: flags (flags),
  fileType (fileType),
  presetCategory (CCLSTR ("SimplePreset")),
  presetClassName (CCLSTR ("Preset"))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimplePresetHandler::registerSelf ()
{
	System::GetPresetFileRegistry ().addHandler (this);

	// collect all instances for lookup by category in findHandler
	// note: only works inside the same module - could be extended via new methods in IPresetFileRegistry & IPresetFileHandler
	simplePresetHandlers.add (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SimplePresetHandler::getFlags ()
{
	return flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API SimplePresetHandler::getFileType ()
{
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePresetHandler::canHandle (IUnknown* target)
{
	UnknownPtr<IStorable> storable (target);
	FileType format;
	return storable && storable->getFormat (format) && format == getFileType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SimplePresetHandler::finishPath (IUrl& url)
{
	ASSERT(!presetFolderName.isEmpty ())
	url.descend (kUserPresetFolder, Url::kFolder);
	url.descend (presetFolderName, Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePresetHandler::getSubFolder (String& subFolder, IAttributeList& metaInfo)
{
	if(!presetFolderName.isEmpty ())
	{
		subFolder = kUserPresetFolder;
		subFolder << Url::strPathChar << presetFolderName;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePresetHandler::getWriteLocation (IUrl& url, IAttributeList* metaInfo)
{
	if(!presetFolderName.isEmpty ())
	{
		Url path (PresetPackageHandler::instance ().getPrimaryRootFolder ());
		finishPath (path);
		url.assign (path);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimplePresetHandler::getReadLocation (IUrl& url, IAttributeList* metaInfo, int index)
{
	if(!presetFolderName.isEmpty ())
	{
		if(index == 0)
			return getWriteLocation (url, metaInfo);

		if(index == 1) // factory location
		{
			Url path;
			PresetPackageHandler::instance ().getFactoryRootFolder (path);
			finishPath (path);
			url.assign (path);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SimplePreset* SimplePresetHandler::newPreset (UrlRef url, IAttributeList* metaInfo)
{
	SimplePreset* preset = NEW SimplePreset (url, metaInfo);
	preset->setCategory (presetCategory);
	preset->setClassName (presetClassName);
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API SimplePresetHandler::openPreset (UrlRef url, IPresetDescriptor* descriptor)
{
	SimplePreset* preset = newPreset (url);
	if(descriptor)
		preset->fromDescriptor (*descriptor);
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API SimplePresetHandler::createPreset (UrlRef url, IAttributeList& metaInfo)
{
	return newPreset (url, &metaInfo);
}
