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
// Filename    : ccl/extras/web/webpreset.cpp
// Description : Web Preset
//
//************************************************************************************************

#include "ccl/extras/web/webpreset.h"

#include "ccl/app/presets/presetcollection.h"
#include "ccl/app/presets/presetsystem.h"

#include "ccl/app/presets/presetfileprimitives.h"

#include "ccl/base/security/cryptomaterial.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/objectconverter.h"

#include "ccl/system/fileutilities.h"

#include "ccl/public/app/presetmetainfo.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

#define USE_TEMP_FOLDER 1

//************************************************************************************************
// FileDescriptorToWebPresetFilter
//************************************************************************************************

class FileDescriptorToWebPresetFilter: public ConvertFilter
{
public:
	// IConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid = kNullUID) const override;
	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid = kNullUID) const override;
};

//************************************************************************************************
// FileDescriptorToWebPresetFilter
//************************************************************************************************

tbool CCL_API FileDescriptorToWebPresetFilter::canConvert (IUnknown* object, UIDRef cid) const
{
	if(cid == ccl_iid<IPreset> ())
	{
		UnknownPtr<IFileDescriptor> descriptor (object);
		if(descriptor)
		{
			FileType fileType;
			descriptor->getFileType (fileType);
			return System::GetPresetManager ().supportsFileType (fileType);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API FileDescriptorToWebPresetFilter::convert (IUnknown* object, UIDRef cid) const
{
	ASSERT (cid == ccl_iid<IPreset> ())
	UnknownPtr<IFileDescriptor> descriptor (object);
	if(descriptor)
	{
		FileType fileType;
		descriptor->getFileType (fileType);
		if(fileType == PresetCollectionHandler::instance ().getFileType ())
			return ccl_as_unknown (NEW WebPresetCollection (descriptor));
		else
			return ccl_as_unknown (NEW WebPreset (descriptor));
	}

	return nullptr;
}

//************************************************************************************************
// WebPreset
//************************************************************************************************

void WebPreset::registerConvertFilter ()
{
	ObjectConverter::instance ().registerFilter (AutoPtr<ConvertFilter> (NEW FileDescriptorToWebPresetFilter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (WebPreset, Preset)

////////////////////////////////////////////////////////////////////////////////////////////////////

WebPreset::WebPreset (IFileDescriptor* descriptor)
: Preset (String::kEmpty, kReadOnly),
  metaInfo (NEW Attributes)
{
	setFileDescriptor (descriptor);
	ASSERT (fileDescriptor)

	fileDescriptor->getMetaInfo (*metaInfo);
	fileDescriptor->getTitle (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebPreset::~WebPreset ()
{
	if(metaInfo)
		metaInfo->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDataTarget* WebPreset::getDataTarget () const
{
	return dataTarget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebPreset::setDataTarget (IDataTarget* target)
{
	dataTarget = target;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* WebPreset::getProgress () const
{
	return progress;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebPreset::setProgress (IParameter* param)
{
	progress = param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef WebPreset::getDestPath () const
{
	if(destPath.isEmpty ())
	{
		// make download destination path in preset location
		FileType fileType;
		fileDescriptor->getFileType (fileType);

		#if USE_TEMP_FOLDER
		PresetFilePrimitives::getTempLocation (destPath);
		destPath.descend (UIDString::generate (), Url::kFolder);
		#else
		PresetFilePrimitives::getWriteLocation (destPath, fileType, metaInfo);
		#endif

		String fileName;
		fileDescriptor->getFileName (fileName);
		destPath.descend (fileName);
		destPath.setFileType (fileType, true);

		destPath.makeUnique ();
	}
	return destPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API WebPreset::getMetaInfo () const
{
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebPreset::getUrl (IUrl& url) const
{
	UnknownPtr<IDownloadable> sourceInfo (fileDescriptor);
	if(!sourceInfo)
		return false;

	url.assign (sourceInfo->getSourceUrl ());
	return !url.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebPreset::store (IUnknown* target)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebPreset::restore (IUnknown* target) const
{
	UnknownPtr<IDataTarget> dataTarget (target);
	const_cast<WebPreset*> (this)->setDataTarget (dataTarget);
	
	// start transfer of the actual preset
	return PresetTransferHandler::instance ().startTransfer (const_cast<WebPreset*> (this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WebPreset::getTransferState () const
{
	return transfer ? transfer->getState () : ITransfer::kNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WebPreset::updateTransferState ()
{
	if(transfer)
	{
		int state = transfer->getState ();
		if(progress)
		{
			progress->setNormalized ((float)transfer->getProgressValue (), true);

			if(state == ITransfer::kFailed || state == ITransfer::kCanceled)
			{
				progress->enable (false);
				progress->performUpdate ();
			}
		}
		return state;
	}
	return ITransfer::kNone;
}

//************************************************************************************************
// WebPresetCollection
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebPresetCollection, WebPreset)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebPresetCollection::WebPresetCollection (IFileDescriptor* descriptor)
: WebPreset (descriptor),
  parts (nullptr)
{
	String partsFile;
	if(AttributeAccessor (*getMetaInfo ()).getString (partsFile, "Preset:PartsFile"))
	{
		AutoPtr<PresetPartList> presetParts (NEW PresetPartList);

		Security::Crypto::Material partsData;
		partsData.fromBase64 (partsFile);

		XmlArchive archive (partsData.asStream ());
		if(archive.loadObject ("PresetParts", *presetParts))
			parts = presetParts.detach ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebPresetCollection::~WebPresetCollection ()
{
	if(parts)
		parts->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetPartList& WebPresetCollection::getParts () const
{
	if(parts == nullptr)
		parts = NEW PresetPartList;
	return *parts;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WebPresetCollection::countPresets ()
{
	return getParts ().countParts ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API WebPresetCollection::openPreset (int index)
{
	PresetPart* part = getParts ().getPart (index);
	ASSERT (part != nullptr)

	CCL_NOT_IMPL ("WebPresetCollection: openPreset not implemented!")
	return nullptr; // todo
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API WebPresetCollection::openPreset (const IStringDictionary& parameters)
{
	CCL_NOT_IMPL ("WebPresetCollection: Open preset with parameters not implemented!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* CCL_API WebPresetCollection::createPreset (IAttributeList& metaInfo)
{
	CCL_NOT_IMPL ("WebPresetCollection: createPreset not implemented!")
	return nullptr;
}

//************************************************************************************************
// PresetTransferHandler::Finalizer
//************************************************************************************************

PresetTransferHandler::Finalizer::Finalizer (WebPreset* preset)
{
	webPreset.share (preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetTransferHandler::Finalizer::execute (IObject* target)
{
	UnknownPtr<ITransfer> transfer (target);
	ASSERT (transfer != nullptr)
	PresetTransferHandler::instance ().onTransferFinished (webPreset);
}

//************************************************************************************************
// PresetTransferHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetTransferHandler, Object)
DEFINE_SINGLETON (PresetTransferHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetTransferHandler::PresetTransferHandler ()
: lastUpdateTime (0),
  restoring (false)
{
	transferPresets.objectCleanup (true);
	finishedPresets.objectCleanup (true);
	tempFolders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetTransferHandler::~PresetTransferHandler ()
{
	ASSERT (restoring == false)
	ASSERT (transferPresets.isEmpty ())
	ASSERT (finishedPresets.isEmpty ())

	ForEach (tempFolders, Url, folder)
		System::GetFileSystem ().removeFolder (*folder, IFileSystem::kDeleteRecursively);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool PresetTransferHandler::startTransfer (WebPreset* webPreset)
{
	UnknownPtr<IDownloadable> sourceInfo (webPreset->getFileDescriptor ());
	if(!sourceInfo)
		return false;

	// prepare transfer
	AutoPtr<ITransfer> transfer;
	System::GetWebFileService ().createDownload (transfer, sourceInfo->getSourceUrl (), webPreset->getDestPath ());
	if(!transfer)
		return false;

	// add finalizer for restoring the downloaded preset
	transfer->addFinalizer (NEW PresetTransferHandler::Finalizer (webPreset));

	// trigger inital transfer
	if(transfer && transfer->getState () == ITransfer::kNone)
		System::GetTransferManager ().queue (transfer);

	webPreset->setTransfer (transfer);
	webPreset->retain ();
	transferPresets.add (webPreset);

	#if USE_TEMP_FOLDER
	Url* tempFolder = NEW Url (webPreset->getDestPath ());
	tempFolder->ascend ();
	tempFolders.add (tempFolder);
	#endif

	UnknownPtr<IController> controller (webPreset->getDataTarget ());
	webPreset->setProgress (controller ? controller->findParameter ("transferProgress") : nullptr);
	webPreset->updateTransferState ();

	startTimer ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTransferHandler::removeTransfer (WebPreset* webPreset)
{
	#if USE_TEMP_FOLDER
	Url folder (webPreset->getDestPath ());
	folder.ascend ();
	if(Object* tempFolder = tempFolders.findEqual (folder))
	{
		tempFolders.remove (tempFolder);
		tempFolder->release ();
		System::GetFileSystem ().removeFolder (folder, IFileSystem::kDeleteRecursively);
	}
	#endif

	transferPresets.remove (webPreset);
	webPreset->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTransferHandler::onTransferFinished (WebPreset* webPreset)
{
	if(webPreset)
	{
		webPreset->updateTransferState ();
		transferPresets.remove (webPreset);
		finishedPresets.add (webPreset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTransferHandler::onIdleTimer ()
{
	bool idleNeeded = false;

	// monitor transfer state changes
	if(!transferPresets.isEmpty ())
	{
		idleNeeded = true;
		int64 now = System::GetSystemTicks ();
		if(now - lastUpdateTime >= 500)
		{
			lastUpdateTime = now;
			ListForEachObject (transferPresets, WebPreset, p)
				int state = p->updateTransferState ();

				if(state == ITransfer::kFailed || state == ITransfer::kCanceled)
					removeTransfer (p);
			EndFor
		}
	}

	// check for import
	if(!finishedPresets.isEmpty ())
	{
		idleNeeded = true;
		restoreAll ();
	}

	// check if timer can stop
	if(idleNeeded == false)
		stopTimer ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTransferHandler::restoreAll ()
{
	if(restoring) // this method can be reentered!
		return;

	// check for menu loop & progress dialog
	if(System::GetDesktop ().isInMode (IDesktop::kMenuLoopMode|IDesktop::kProgressMode))
		return;

	// check mouse or modifier keys
	KeyState keys;
	System::GetGUI ().getKeyState (keys);
	if(keys.isSet (KeyState::kMouseMask|KeyState::kModifierMask))
		return;

	ScopedVar<bool> scope (restoring, true);

	AutoPtr<WebPreset> preset;
	while(preset = (WebPreset*)finishedPresets.removeFirst ())
	{
		// todo: check if it can be restored now
		if(preset->getTransferState () == ITransfer::kCompleted)
			restorePreset (preset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetTransferHandler::restorePreset (WebPreset* webPreset)
{
	AutoPtr<IPreset> preset (System::GetPresetManager ().openPreset (webPreset->getDestPath ()));
	if(preset)
	{
		#if !USE_TEMP_FOLDER
		// notify manager about new preset file (adds to preset store)
		System::GetPresetManager ().onPresetCreated (webPreset->getDestPath (), *preset);
		#endif

		// feed preset into data target
		if(IDataTarget* dataTarget = webPreset->getDataTarget ())
		{
			UnknownList data;
			data.add (preset, true);
			dataTarget->insertData (data);
		}
	}
}
