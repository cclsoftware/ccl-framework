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
// Filename    : ccl/app/presets/presetfile.cpp
// Description : Preset File
//
//************************************************************************************************

#include "ccl/app/presets/presetfile.h"
#include "ccl/app/presets/presetsystem.h"
#include "ccl/app/utilities/pathclassifier.h"
#include "ccl/app/component.h"

#include "ccl/base/development.h"
#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/binaryarchive.h"

#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/storage/istorage.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (Preset, "Preset")
	XSTRING (Presets, "Presets")
END_XSTRINGS

static FileType PresetFileType (XSTR_REF (Preset).getKey (), "preset", CCL_MIME_TYPE "-preset");
static const String kUnknownFolder (CCLSTR ("(Unknown Vendor)"));

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (PresetPackageHandler, kSetupLevel)
{
	if(System::IsInMainAppModule ())
	{
		PresetFileType.setDescription (XSTR (Preset));
		System::GetPresetFileRegistry ().addHandler (return_shared (&PresetPackageHandler::instance ()), true);
		System::GetPresetFileRegistry ().addHandler (return_shared (&PresetLocationHandler::instance ()));
		System::GetFileTypeRegistry ().updateFileType (PresetFileType);
	}
	return true;
}

//************************************************************************************************
// PresetPackageHandler
//************************************************************************************************

DEFINE_SINGLETON (PresetPackageHandler)
const String PresetPackageHandler::kPresetFolder (CCLSTR ("Presets"));
const LocalString& PresetPackageHandler::getPlural () { return XSTR_REF (Presets); }
const CCL::UID PresetPackageHandler::catListId (0x5c75b5d8, 0xd2cd, 0x4f40, 0xb1, 0x3d, 0x38, 0x9e, 0x7b, 0x10, 0xcf, 0xb4);

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::addCategorySubfolder (StringRef category, StringRef subFolderName)
{
	String test;
	if(getCategorySubfolder (category, test) == false)
	{
		UnknownPtr<IAttributeList> attrList = System::GetObjectTable ().getObjectByID (catListId);
		if(attrList == nullptr)
		{
			attrList = AutoPtr<IAttributeList> (ccl_new<IAttributeList> (ClassID::Attributes));
			if(attrList)
				System::GetObjectTable ().registerObject (attrList, catListId, "PresetCategoryList");
		}

		if(attrList)
		{
			MutableCString attrId (category);
			attrList->setAttribute (attrId, subFolderName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPackageHandler::getCategorySubfolder (StringRef category, String& subFolderName)
{
	UnknownPtr<IAttributeList> attrList = System::GetObjectTable ().getObjectByID (catListId);
	if(attrList)
	{
		MutableCString attrId (category);
		AttributeReadAccessor acc (*attrList);
		return acc.getString (subFolderName, attrId);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPackageHandler::PresetPackageHandler ()
: systemSink (*NEW SignalSink (Signals::kSystemInformation))
{
	systemSink.setObserver (this);
	systemSink.enable (true);

	rootFolders.objectCleanup (true);

	Url factoryPresetRoot;
	getFactoryRootFolder (factoryPresetRoot);
	addRootFolder (factoryPresetRoot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPackageHandler::~PresetPackageHandler ()
{
	systemSink.enable (false);
	delete &systemSink;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef PresetPackageHandler::getFactoryRootFolder (IUrl& folder)
{
	System::GetSystem ().getLocation (folder, System::kAppDeploymentFolder);
	folder.descend (kPresetFolder, IUrl::kFolder);
	return folder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::addRootFolder (UrlRef folder)
{
	Threading::ScopedLock scopedLock (lock);

	rootFolders.add (NEW Url (folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::removeAllRootFolders ()
{
	Threading::ScopedLock scopedLock (lock);
	rootFolders.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetPackageHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kContentLocationChanged)
	{
		UnknownPtr<IUrl> path (msg[0]);
		ASSERT (path != nullptr)

		Url rootFolder (*path);
		rootFolder.descend (kPresetFolder, Url::kFolder);
		if(!System::GetFileSystem ().fileExists (rootFolder))
			System::GetFileSystem ().createFolder (rootFolder); // ensure preset folder exists

		setPrimaryRootFolder (rootFolder);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::setPrimaryRootFolder (UrlRef folder)
{
	Threading::ScopedLock scopedLock (lock);

	// primaryFolder either shares an element of rootFolders or a separate object
	primaryFolder.share (ccl_cast<Url> (rootFolders.findEqual (Url (folder))));
	if(!primaryFolder)
		primaryFolder = NEW Url (folder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef PresetPackageHandler::getPrimaryRootFolder ()
{
	Threading::ScopedLock scopedLock (lock);

	if(!primaryFolder)
	{
		// build default folder
		Url defaultFolder;
		System::GetSystem ().getLocation (defaultFolder, System::kUserContentFolder);
		defaultFolder.descend (kPresetFolder, Url::kFolder);
		setPrimaryRootFolder (defaultFolder);
	}
	return *primaryFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::removeTrailingDot (String& name)
{
	// windows doesn't allow folders ending with '.'
	int lastIndex = name.length () - 1;
	if(name.at (lastIndex) == '.')
		name.truncate (lastIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::finishPath (IUrl& url, IAttributeList* metaInfo)
{
	if(metaInfo)
	{
		PresetMetaAttributes metaAttributes (*metaInfo);

		String subFolderName;
		if(getCategorySubfolder (metaAttributes.getCategory (), subFolderName))
		{
			removeTrailingDot (subFolderName);
			url.descend (subFolderName, IUrl::kFolder);
		}
		else
		{
			LegalFileName vendor (metaAttributes.getVendor ());
			if(vendor.isEmpty ())
				vendor = kUnknownFolder;
			else
				removeTrailingDot (vendor);

			url.descend (vendor, IUrl::kFolder);
		}

		LegalFileName className (metaAttributes.getClassName ());
		if(!className.isEmpty ())
		{
			removeTrailingDot (className);
			url.descend (className, IUrl::kFolder);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::setAlternativeVendorName (StringRef vendorName, StringRef alternativeName)
{
	ASSERT (vendorName != alternativeName)
	alternativeVendors.setEntry (vendorName, alternativeName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetPackageHandler::getSubFolder (String& subFolder, IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttributes (metaInfo);
	adjustMetaInfo (metaAttributes);

	return getSubFolderInternal (subFolder, metaAttributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPackageHandler::getAlternativeSubFolder (String& subFolder, IAttributeList& metaInfo)
{
	PresetMetaAttributes metaAttributes (metaInfo);
	String alternativeClassName;
	adjustMetaInfo (metaAttributes, &alternativeClassName);

	String categoryFolder;
	if(!getCategorySubfolder (metaAttributes.getCategory (), categoryFolder))
	{
		String vendor (metaAttributes.getVendor ());
		StringRef alternativeVendor (alternativeVendors.lookupValue (vendor));

		const String* vendorName = alternativeVendor.isEmpty () ? nullptr : &alternativeVendor;
		const String* className = alternativeClassName.isEmpty () ? nullptr : &alternativeClassName;
		if(className || vendorName)
			return getSubFolderInternal (subFolder, metaAttributes, &alternativeVendor, className);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetPackageHandler::adjustMetaInfo (PresetMetaAttributes& metaAttributes, String* alternativeClassName) const
{
	UID classID;
	if(metaAttributes.getClassID (classID))
		if(const IClassDescription* classDesc = System::GetPlugInManager ().getClassDescription (classID))
		{
			// Assign registered class information in case preset meta info is outdated.
			metaAttributes.assign (*classDesc);

			if(alternativeClassName)
			{
				Variant altName;
				if(classDesc->getClassAttribute (altName, Meta::kAlternativeClassName))
					*alternativeClassName = altName;
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetPackageHandler::getSubFolderInternal (String& subFolder, const PresetMetaAttributes& metaAttributes, const String* vendorName, const String* className) const
{
	if(getCategorySubfolder (metaAttributes.getCategory (), subFolder))
	{
		removeTrailingDot (subFolder);
	}
	else
	{
		subFolder = String::kEmpty;

		LegalFileName vendor (vendorName ? *vendorName : metaAttributes.getVendor ());
		if(vendor.isEmpty ())
			vendor = kUnknownFolder;
		else
			removeTrailingDot (vendor);

		if(subFolder.isEmpty ())
			subFolder = vendor;
		else
			subFolder << Url::strPathChar << vendor;
	}

	LegalFileName legalClassName (className ? *className : metaAttributes.getClassName ());
	if(!legalClassName.isEmpty ())
	{
		removeTrailingDot (legalClassName);
		subFolder << Url::strPathChar << legalClassName;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetPackageHandler::getWriteLocation (IUrl& url, IAttributeList* metaInfo)
{
	url.assign (getPrimaryRootFolder ());
	finishPath (url, metaInfo);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetPackageHandler::getReadLocation (IUrl& url, IAttributeList* metaInfo, int index)
{
	if(index == 0)
		return getWriteLocation (url, metaInfo);

	Threading::ScopedLock scopedLock (lock);

	Url* root = ccl_cast<Url> (rootFolders.at (index - 1));
	if(root)
	{
		url.assign (*root);
		finishPath (url, metaInfo);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API PresetPackageHandler::getFileType ()
{
	return PresetFileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetPackageHandler::canHandle (IUnknown* target)
{
	return UnknownPtr<IStorable> (target).isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PresetPackageHandler::getFlags ()
{
	return kCanImport|kCanExport;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetPackageHandler::openPreset (UrlRef url, IPresetDescriptor* descriptor)
{
	if(PathClassifier::isCompressedFile (url)) // package must be seekable!
		return nullptr;

	PresetFile* preset = NEW PresetFile (url);
	if(descriptor)
		preset->fromDescriptor (*descriptor);
	return preset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API PresetPackageHandler::createPreset (UrlRef url, IAttributeList& metaInfo)
{
	PackageInfo* presetMetaInfo = unknown_cast<PackageInfo> (&metaInfo);
	AutoPtr<PackageInfo> packageInfo;
	if(presetMetaInfo == nullptr) // can be from another component...
	{
		packageInfo = NEW PackageInfo (metaInfo);
		presetMetaInfo = packageInfo;
	}
	PresetFile* presetFile = NEW PresetFile (url, presetMetaInfo);
	return presetFile;
}

//************************************************************************************************
// PresetFile
//************************************************************************************************

IPackageFile* PresetFile::createPackageForSave (UrlRef path)
{
	return System::GetPackageHandler ().createPackage (path, ClassID::ZipFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetFile::getPackageItemAttributesForType (const FileType& fileType)
{
	int fileAttributes = IPackageItem::kCompressed;
	// no compression for binary preset formats
	if(fileType.isHumanReadable () == false)
	{
		SOFT_ASSERT (!fileType.getMimeType ().isEmpty (), "MIME type not set!\n")
		fileAttributes = 0;
	}
	return fileAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFile::registerExtraContentHandler (IExtraContentHandler* handler)
{
	extraContentHandlers.add (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vector<PresetFile::IExtraContentHandler*> PresetFile::extraContentHandlers;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PresetFile, Preset)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFile::PresetFile (UrlRef url, PackageInfo* _metaInfo)
: url (url),
  metaInfo (_metaInfo)
{
	if(metaInfo)
	{
		metaInfo->retain ();
		name = PresetMetaAttributes (*metaInfo).getTitle ();
	}
	else
		url.getName (name, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFile::~PresetFile ()
{
	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::isReadOnly () const
{
	if(url.isEmpty ())
		return true;
	return System::GetFileSystem ().isWriteProtected (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API PresetFile::getMetaInfo () const
{
	return readMetaInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::getUrl (IUrl& url) const
{
	url.assign (this->url);
	return !url.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PresetFile::getMimeType () const
{
	return PresetFileType.getMimeType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageInfo& PresetFile::getInfo () const
{
	if(metaInfo == nullptr)
		metaInfo = NEW PackageInfo;
	return *metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFile::setupMetaInfo ()
{
	PresetMetaAttributes metaAttribs (getInfo ());
	metaAttribs.setMimeType (getMimeType ());
	metaAttribs.setCreator (RootComponent::instance ().getCreatorName ());
	metaAttribs.setGenerator (RootComponent::instance ().getGeneratorName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::store (IUnknown* target)
{
	ASSERT (!url.isEmpty ())
	AutoPtr<IPackageFile> packageFile = createPackageForSave (url);
	ASSERT (packageFile != nullptr)
	packageFile->setOption (PackageOption::kCompressed, true);
	if(!packageFile->create ())
		return false;

	IFileSystem* fileSystem = packageFile->getFileSystem ();
	ASSERT (fileSystem != nullptr)

	ProgressNotifyScope baseProgressScope (progress);
	ArchiveHandler archiveHandler (*fileSystem);
	archiveHandler.setProgress (progress);
	IPackageFile::Closer packageFileCloser (*packageFile);

	// setup meta info
	setupMetaInfo ();

	// save content
	if(!storeContent (archiveHandler, target))
		return false;

	// save extra content
	VectorForEach (extraContentHandlers, IExtraContentHandler*, extraHandler)
		extraHandler->storeExtraContent (url, archiveHandler, target);	
	EndFor

	// save meta info
	if(!getInfo ().saveWithHandler (archiveHandler))
		return false;

	ProgressNotifyScope flushProgressScope (baseProgressScope);
	if(!packageFile->flush (progress))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFile::storeContent (ArchiveHandler& archiveHandler, IUnknown* target)
{
	UnknownPtr<IStorable> storable (target);
	if(!storable)
		return false;

	FileType dataFileType;
	if(!storable->getFormat (dataFileType))
		dataFileType.setExtension ("bin");

	String dataFileName (CCLSTR ("data."));
	dataFileName.append (dataFileType.getExtension ());

	// setup additional meta info
	PresetMetaAttributes metaAttribs (*metaInfo);
	metaAttribs.setDataFile (dataFileName);
	metaAttribs.setDataMimeType (dataFileType.getMimeType ());

	// save data stream
	int fileAttributes = getPackageItemAttributesForType (dataFileType);
	if(!archiveHandler.addSaveTask (dataFileName, *storable, "Preset Data", &fileAttributes))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::restore (IUnknown* target) const
{
	ASSERT (!url.isEmpty ())
	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (url);
	ASSERT (packageFile != nullptr)
	if(!packageFile)
		return false;

	IPackageFile::Closer packageFileCloser (*packageFile);
	IFileSystem* fileSystem = packageFile->getFileSystem ();
	ASSERT (fileSystem != nullptr)

	ProgressNotifyScope baseProgressScope (progress);
	ArchiveHandler archiveHandler (*fileSystem);
	archiveHandler.setProgress (progress);

	// load meta info
	if(!getInfo ().loadFromHandler (archiveHandler))
		return false;
	if(metaInfo) // see readMetaInfo
		const_cast<PresetFile*> (this)->checkName (*metaInfo);

	// load content
	if(!restoreContent (archiveHandler, target))
		return false;

	// load extra content
	VectorForEach (extraContentHandlers, IExtraContentHandler*, extraHandler)
		extraHandler->restoreExtraContent (url, archiveHandler, target);	
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFile::restoreContent (ArchiveHandler& archiveHandler, IUnknown* target) const
{
	UnknownPtr<IStorable> storable (target);
	if(!storable)
		return false;

	// load data stream
	String dataFileName (PresetMetaAttributes (*metaInfo).getDataFile ());
	if(!archiveHandler.loadStream (dataFileName, *storable))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* PresetFile::readMetaInfo () const
{
	if(metaInfo == nullptr)
	{
		ASSERT (!url.isEmpty ())
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().openPackage (url);
		if(packageFile)
		{
			IFileSystem* fileSystem = packageFile->getFileSystem ();
			ASSERT (fileSystem != nullptr)
			ArchiveHandler archiveHandler (*fileSystem);

			metaInfo = NEW PackageInfo;
			metaInfo->loadFromHandler (archiveHandler);
		}

		if(metaInfo)
			const_cast<PresetFile*> (this)->checkName (*metaInfo);
	}
	return &getInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::toDescriptor (IPresetDescriptor& descriptor) const
{
	readMetaInfo ();
	if(metaInfo)
	{
		if(IStream* stream = descriptor.getData ())
		{
			BinaryArchive archive (*stream);
			archive.saveAttributes (myClass ().getPersistentName (), *metaInfo);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFile::fromDescriptor (IPresetDescriptor& descriptor)
{
	if(IStream* stream = descriptor.getData ())
	{
		stream->rewind ();

		AutoPtr<PackageInfo> info = NEW PackageInfo;
		BinaryArchive archive (*stream);
		if(archive.loadAttributes (myClass ().getPersistentName (), *info))
		{
			take_shared<PackageInfo> (metaInfo, info);

			setName (descriptor.getPresetName ());
			ASSERT (metaInfo != nullptr && descriptor.getPresetName () == (PresetMetaAttributes (*metaInfo).getTitle ()))
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// PresetLocationHandler
//************************************************************************************************

DEFINE_SINGLETON (PresetLocationHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetLocationHandler::PresetLocationHandler ()
{
	presetLocations.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetLocationHandler::addLocation (UrlRef path)
{
	if(!presetLocations.contains (Url (path)))
		presetLocations.add (NEW Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetLocationHandler::removeLocation (UrlRef path)
{
	for(Url* url : iterate_as<Url> (presetLocations))
		if(path.isEqualUrl (*url))
		{
			presetLocations.remove (url);
			url->release ();
			break;
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetLocationHandler::locationsChanged ()
{
	signal (Message (kPresetLocationsChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetLocationHandler::getReadLocation (IUrl& outUrl, IAttributeList* metaInfo, int index)
{
	if(index >= 0 && index < presetLocations.count ())
	{
		Url* url = (Url*)presetLocations.at (index);
		outUrl.assign (*url);
		return true;
	}
	return false;
}
