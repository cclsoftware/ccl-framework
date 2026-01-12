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
// Filename    : ccl/app/presets/presetcollection.cpp
// Description : Preset Collection
//
//************************************************************************************************

#include "ccl/app/presets/presetcollection.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/utilities/pathclassifier.h"

#include "ccl/app/component.h" // for IsInMainAppModule()

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/binaryarchive.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// PresetSubFile
//************************************************************************************************

class PresetSubFile: public Preset
{
public:
	PresetSubFile (PresetCollection& collection, PresetPart& part);
	~PresetSubFile ();

	// Preset
	IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getUrl (IUrl& url) const override;
	tbool CCL_API store (IUnknown* target) override;
	tbool CCL_API restore (IUnknown* target) const override;

protected:
	PresetCollection& collection;
	PresetPart& part;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (PresetCollection, "Preset Collection")
END_XSTRINGS

static FileType PresetCollectionType (XSTR_REF (PresetCollection).getKey (), "multipreset", CCL_MIME_TYPE "-multipreset");

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetCollectionHandler, kSetupLevel)
{
	if(System::IsInMainAppModule ())
	{
		PresetCollectionType.setDescription (XSTR (PresetCollection));
		System::GetPresetFileRegistry ().addHandler (return_shared (&PresetCollectionHandler::instance ()));
		System::GetFileTypeRegistry ().updateFileType (PresetCollectionType);
	}
	return true;
}

//************************************************************************************************
// PresetPart
//************************************************************************************************

DEFINE_CLASS (PresetPart, PersistentAttributes)

//************************************************************************************************
// PresetPartList
//************************************************************************************************

DEFINE_CLASS (PresetPartList, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPartList::PresetPartList ()
{
	parts.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetPartList::countParts () const
{
	return parts.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPart* PresetPartList::getPart (int index) const
{
	return (PresetPart*)parts.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPartList::addPart (PresetPart* part)
{
	parts.add (part);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetPartList::getPartIndex (const PresetPart* part) const
{
	return parts.index (part);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PresetPartList::newIterator () const
{
	return parts.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPartList::loadFromHandler (ArchiveHandler& handler)
{
	return handler.loadItem (CCLSTR ("presetparts.xml"), "PresetParts", *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPartList::saveWithHandler (ArchiveHandler& handler)
{
	return handler.addSaveTask (CCLSTR ("presetparts.xml"), "PresetParts", *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPartList::load (const Storage& storage)
{
	storage.getAttributes ().unqueue (parts, nullptr, ccl_typeid<PresetPart> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPartList::save (const Storage& storage) const
{
	return storage.getAttributes ().queue (nullptr, parts);
}

//************************************************************************************************
// PresetCollection
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetCollection, PresetFile)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetCollection::PresetCollection (UrlRef url, PackageInfo* metaInfo)
: PresetFile (url, metaInfo),
  parts (nullptr),
  currentHandler (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetCollection::~PresetCollection ()
{
	if(parts)
		parts->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPartList& PresetCollection::getParts () const
{
	if(parts == nullptr)
		parts = NEW PresetPartList;
	return *parts;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PresetCollection::countPresets ()
{
	readMetaInfo ();
	return getParts ().countParts ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetCollection::openPreset (int index)
{
	readMetaInfo ();
	PresetPart* part = getParts ().getPart (index);
	ASSERT (part != nullptr)
	return part ? NEW PresetSubFile (*this, *part) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetCollection::openPreset (const IStringDictionary& parameters)
{
	CCL_NOT_IMPL ("Open preset with parameters not implemented!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetCollection::createPreset (IAttributeList& metaInfo)
{
	ASSERT (currentHandler != nullptr) // must be called inside store()!
	if(currentHandler == nullptr)
		return nullptr;

	PresetPart* part = NEW PresetPart;
	static_cast<IAttributeList*> (part)->copyFrom (metaInfo);
	getParts ().addPart (part);
	return NEW PresetSubFile (*this, *part);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PresetCollection::getMimeType () const
{
	return PresetCollectionType.getMimeType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API PresetCollection::openStream (StringRef path, int mode)
{
	if(mode & (IStream::kWriteMode | IStream::kCreate))
	{
		ASSERT (currentHandler)
		if(currentHandler)
		{
			MemoryStream* stream = NEW MemoryStream;
			currentHandler->addSaveTask (path, *stream);
			return stream;
		}
	}
	else
	{
		if(currentHandler)
			return currentHandler->openStream (path, mode);
		else
		{
			AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (url);
			ASSERT (packageFile != nullptr)
			if(!packageFile)
				return nullptr;

			IPackageFile::Closer packageFileCloser (*packageFile);
			IFileSystem* fileSystem = packageFile->getFileSystem ();
			ASSERT (fileSystem != nullptr)

			ProgressNotifyScope baseProgressScope (progress);
			ArchiveHandler archiveHandler (*fileSystem);
			archiveHandler.setProgress (progress);

			return archiveHandler.copyData (path);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetCollection::storeContent (ArchiveHandler& handler, IUnknown* target)
{
	UnknownPtr<IPresetCollector> collector (target);
	if(!collector)
		return false;

	ScopedVar<ArchiveHandler*> handlerScope (currentHandler, &handler);

	// collect presets
	if(!collector->save (*this))
		return false;

	// save part list
	if(!getParts ().saveWithHandler (handler))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetCollection::restoreContent (ArchiveHandler& handler, IUnknown* target) const
{
	UnknownPtr<IPresetCollector> collector (target);
	if(!collector)
		return false;

	ScopedVar<ArchiveHandler*> handlerScope (currentHandler, &handler);

	// load part list
	if(getParts ().countParts () == 0)
		if(!getParts ().loadFromHandler (handler))
			return false;

	// restore presets
	if(!collector->load (const_cast<PresetCollection&> (*this)))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetCollection::readMetaInfo () const
{
	if(metaInfo == nullptr)
	{
		ASSERT (parts == nullptr)

		ASSERT (!url.isEmpty ())
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (url);
		if(packageFile)
		{
			IFileSystem* fileSystem = packageFile->getFileSystem ();
			ASSERT (fileSystem != nullptr)
			ArchiveHandler archiveHandler (*fileSystem);

			metaInfo = NEW PackageInfo;
			metaInfo->loadFromHandler (archiveHandler);

			parts = NEW PresetPartList;
			parts->loadFromHandler (archiveHandler);
		}

		if(metaInfo)
			const_cast<PresetCollection*> (this)->checkName (*metaInfo);
	}
	return &getInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollection::toDescriptor (IPresetDescriptor& descriptor) const
{
	readMetaInfo ();
	if(IStream* stream = descriptor.getData ())
	{
		Attributes a;
		if(metaInfo)
			a.set ("metaInfo", metaInfo, Attributes::kShare);
		if(parts)
			a.set ("parts", parts, Attributes::kShare);

		BinaryArchive archive (*stream);
		archive.saveAttributes (myClass ().getPersistentName (), a);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollection::fromDescriptor (IPresetDescriptor& descriptor)
{
	if(IStream* stream = descriptor.getData ())
	{
		stream->rewind ();

		Attributes a;
		BinaryArchive archive (*stream);
		if(archive.loadAttributes (myClass ().getPersistentName (), a))
		{
			take_shared<PackageInfo> (metaInfo, a.getObject<PackageInfo> ("metaInfo"));
			take_shared<PresetPartList> (parts, a.getObject<PresetPartList> ("parts"));

			setName (descriptor.getPresetName ());
			ASSERT (metaInfo != nullptr && descriptor.getPresetName () == (PresetMetaAttributes (*metaInfo).getTitle ()))
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// PresetArchiver
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (PresetArchiver, PresetCollection)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetArchiver::PresetArchiver (ArchiveHandler& archiveHandler, IAttributeList* additionalAttributes)
: PresetCollection (Url::kEmpty, AutoPtr<PackageInfo> (NEW PackageInfo)),
  archiveHandler (archiveHandler),
  additionalAttributes (additionalAttributes)
{
	safe_release (metaInfo); // needed in ctor to avoid base class behavior, but should be loaded from archive later!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetArchiver::store (IUnknown* target)
{
	// setup meta info
	setupMetaInfo ();
	if(additionalAttributes)
		getInfo ().addFrom (*additionalAttributes);

	// save content
	if(!storeContent (archiveHandler, target))
		return false;

	// save meta info
	if(!getInfo ().saveWithHandler (archiveHandler))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetArchiver::restore (IUnknown* target) const
{
	// load meta info
	if(!getInfo ().loadFromHandler (archiveHandler))
		return false;

	// load content
	if(!restoreContent (archiveHandler, target))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetArchiver::readMetaInfo () const
{
	if(metaInfo == nullptr)
		getInfo ().loadFromHandler (archiveHandler);

	if(parts == nullptr)
		getParts ().loadFromHandler (archiveHandler);

	return &getInfo ();
}

//************************************************************************************************
// PresetSubFile
//************************************************************************************************

PresetSubFile::PresetSubFile (PresetCollection& collection, PresetPart& part)
: collection (collection),
  part (part)
{
	readOnly (true); // can not be deleted by user

	collection.retain ();

	// TODO: name is empty here???
	setName (PresetMetaAttributes (part).getTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetSubFile::~PresetSubFile ()
{
	collection.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API PresetSubFile::getMetaInfo () const
{
	return &part;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSubFile::getUrl (IUrl& url) const
{
	tbool result = collection.getUrl (url);
	int presetIndex = collection.getParts ().getPartIndex (&part);
	PresetUrl::setSubPresetIndex (url, presetIndex);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSubFile::store (IUnknown* target)
{
	UnknownPtr<IStorable> storable (target);
	if(!storable)
		return false;

	// must be called inside PresetCollection::store()!
	ArchiveHandler* handler = collection.currentHandler;
	ASSERT (handler != nullptr)
	if(handler == nullptr)
		return false;

	FileType dataFileType;
	if(!storable->getFormat (dataFileType))
		dataFileType.setExtension ("bin");

	PresetMetaAttributes metaAttribs (part);

	// check if caller already prepared data filename
	String dataFileName = metaAttribs.getDataFile ();
	if(dataFileName.isEmpty ())
		dataFileName = String () << "Data/data";

	if(dataFileName.lastIndex (CCLSTR (".")) == -1)
		dataFileName << CCLSTR (".") << dataFileType.getExtension ();

	Url dataPath;
	dataPath.setPath (dataFileName);
	dataPath.makeUnique (handler->getFileSystem ());
	dataFileName = dataPath.getPath ();

	// setup additional meta info
	metaAttribs.setDataFile (dataFileName);
	metaAttribs.setDataMimeType (dataFileType.getMimeType ());
	// TODO: user name, generator???

	// save data stream
	int fileAttributes = PresetFile::getPackageItemAttributesForType (dataFileType);
	if(!handler->addSaveTask (dataFileName, *storable, "Sub Preset Data", &fileAttributes))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetSubFile::restore (IUnknown* target) const
{
	UnknownPtr<IStorable> storable (target);
	if(!storable)
		return false;

	ArchiveHandler* handler = collection.currentHandler;
	if(handler == nullptr) // reopen package file of collection
	{
		Url url;
		collection.getUrl (url);
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (url);
		ASSERT (packageFile != nullptr)
		if(!packageFile)
			return false;

		IPackageFile::Closer packageFileCloser (*packageFile);
		IFileSystem* fileSystem = packageFile->getFileSystem ();
		ASSERT (fileSystem != nullptr)

		ProgressNotifyScope baseProgressScope (collection.progress);
		ArchiveHandler archiveHandler (*fileSystem);
		archiveHandler.setProgress (collection.progress);

		// load data stream
		String dataFileName (PresetMetaAttributes (part).getDataFile ());
		if(!archiveHandler.loadStream (dataFileName, *storable))
			return false;
	}
	else
	{
		// load data stream
		String dataFileName (PresetMetaAttributes (part).getDataFile ());
		if(!handler->loadStream (dataFileName, *storable))
			return false;
	}

	return true;
}

//************************************************************************************************
// PresetCollectionHandler
//************************************************************************************************

DEFINE_SINGLETON (PresetCollectionHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API PresetCollectionHandler::getFileType ()
{
	return PresetCollectionType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollectionHandler::getWriteLocation (IUrl& url, IAttributeList* metaInfo)
{
	return PresetPackageHandler::instance ().getWriteLocation (url, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollectionHandler::getReadLocation (IUrl& url, IAttributeList* metaInfo, int index)
{
	return PresetPackageHandler::instance ().getReadLocation (url, metaInfo, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollectionHandler::getSubFolder (String& subFolder, IAttributeList& metaInfo)
{
	return PresetPackageHandler::instance ().getSubFolder (subFolder, metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetCollectionHandler::canHandle (IUnknown* target)
{
	return UnknownPtr<IPresetCollector> (target).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PresetCollectionHandler::getFlags ()
{
	return kCanImport|kCanExport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetCollectionHandler::openPreset (UrlRef url, IPresetDescriptor* descriptor)
{
	if(PathClassifier::isCompressedFile (url)) // package must be seekable!
		return nullptr;

	PresetCollection* preset = NEW PresetCollection (url);
	if(descriptor)
		preset->fromDescriptor (*descriptor);
	else
	{
		int presetIndex = PresetUrl::getSubPresetIndex (url);
		if(presetIndex >= 0)
		{
			IPreset* subPreset = preset->openPreset (presetIndex);
			preset->release ();
			return subPreset;
		}
	}
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetCollectionHandler::createPreset (UrlRef url, IAttributeList& metaInfo)
{
	PackageInfo* presetMetaInfo = unknown_cast<PackageInfo> (&metaInfo);
	AutoPtr<PackageInfo> packageInfo;
	if(presetMetaInfo == nullptr) // can be from another component...
	{
		packageInfo = NEW PackageInfo (metaInfo);
		presetMetaInfo = packageInfo;
	}

	return NEW PresetCollection (url, presetMetaInfo);
}
