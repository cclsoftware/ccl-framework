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
// Filename    : ccl/app/presets/presetfileregistry.cpp
// Description : Preset File Registry
//
//************************************************************************************************

#include "ccl/app/presets/presetfileregistry.h"
#include "ccl/app/presets/presetsystem.h"

#include "ccl/app/presets/presetfileprimitives.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/message.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// PresetFileRegistry::FileHandler
//************************************************************************************************

class PresetFileRegistry::FileHandler: public Object,
									   public IFileHandler
{
public:
	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;
	State CCL_API getState (IFileDescriptor& descriptor) override;
	tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) override;

	CLASS_INTERFACE (IFileHandler, Object)
};

static PresetFileRegistry::FileHandler thePresetFileHandler;

//************************************************************************************************
// PresetFileRegistry::FileHandler
//************************************************************************************************

tbool CCL_API PresetFileRegistry::FileHandler::openFile (UrlRef path)
{
	if(IPreset* preset = System::GetPresetManager ().openPreset (path))
	{
		System::GetPresetManager ().onPresetCreated (path, *preset); // adds to preset store

		// broadcast the desire to have the preset opened
		SignalSource (Signals::kPresetManager).signal (Message (Signals::kOpenPreset, preset));

		preset->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileHandler::State CCL_API PresetFileRegistry::FileHandler::getState (IFileDescriptor& descriptor)
{
	FileType fileType;
	descriptor.getFileType (fileType);
	if(System::GetPresetManager ().supportsFileType (fileType))
	{
		Attributes metaInfo;
		descriptor.getMetaInfo (metaInfo);

		String name;
		descriptor.getFileName (name);
		int index = name.lastIndex (CCLSTR ("."));
		if(index != -1)
			name.truncate (index);

		if(System::GetPresetManager ().presetExists (&metaInfo, name))
			return kCanUpdate;

		return kCanInstall;
	}

	return kNotHandled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetFileRegistry::FileHandler::getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor)
{
	FileType fileType;
	descriptor.getFileType (fileType);
	if(System::GetPresetManager ().supportsFileType (fileType))
	{
		Attributes metaInfo;
		descriptor.getMetaInfo (metaInfo);
		if(PresetFilePrimitives::getWriteLocation (dst, fileType, &metaInfo))
			return true;
	}
	return false;
}

//************************************************************************************************
// PresetFileRegistry
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetFileRegistry, Component)
DEFINE_COMPONENT_SINGLETON (PresetFileRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFileRegistry::registerFileHandler ()
{
	System::GetFileTypeRegistry ().registerHandler (&thePresetFileHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFileRegistry::unregisterFileHandler ()
{
	System::GetFileTypeRegistry ().unregisterHandler (&thePresetFileHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFileRegistry::PresetFileRegistry ()
: Component ("PresetFileRegistry"),
  defaultHandler (nullptr),
  handlerLock (System::CreateAdvancedLock (ClassID::ReadWriteLock))
{
	subFolderPrefixes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFileRegistry::~PresetFileRegistry ()
{
	ASSERT (handlers.isEmpty ())

	handlerLock->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Threading::ILockable* CCL_API PresetFileRegistry::getLock () const
{
	return handlerLock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetFileRegistry::initialize (IUnknown* context)
{
	// Workaround for now: Plug-ins only allowed in main module!
	bool pluginsEnabled = System::IsInMainAppModule ();

	// instantiate all handlers
	if(pluginsEnabled)
	{
		ForEachPlugInClass (PLUG_CATEGORY_PRESETFILEHANDLER, desc)
			IPresetFileHandler* handler = ccl_new<IPresetFileHandler> (desc.getClassID ());
			ASSERT (handler)
			if(handler)
				addHandler (handler);
		EndFor
	}

	return Component::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetFileRegistry::terminate ()
{
	Vector<IPresetFileHandler*> releaseList;

	{
		Threading::AutoLock autoLock (*this, Threading::ILockable::kWrite);
		releaseList.copyVector (handlers);
		handlers.removeAll ();
	}

	VectorForEach (releaseList, IPresetFileHandler*, handler)
		releaseHandler (handler);
	EndFor

	return Component::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFileRegistry::releaseHandler (IPresetFileHandler* handler)
{
	ISubject::removeObserver (handler, this);

	// hmm... check if it is an internal object
	Object* obj = unknown_cast<Object> (handler);
	if(obj)
		obj->release ();
	else
		ccl_release (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetFileRegistry::addHandler (IPresetFileHandler* handler, tbool isDefault)
{
	Threading::AutoLock autoLock (*this, Threading::ILockable::kWrite);

	ASSERT (!handlers.contains (handler))
	if(isDefault)
		handlers.insertAt (0, handler);
	else
		handlers.add (handler);

	ISubject::addObserver (handler, this);

	// register filetypes
	FileType fileType;
	int index = 0;
	while((fileType = handler->getFileType (index++)).isValid ())
		if(!System::GetFileTypeRegistry ().getFileTypeByExtension (fileType.getExtension ()))
			System::GetFileTypeRegistry ().registerFileType (fileType);

	if(isDefault)
		defaultHandler = handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PresetFileRegistry::countHandlers () const
{
	return handlers.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getHandler (int index) const
{
	return handlers.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getDefaultHandler () const
{
	return handlers.isEmpty () ? 0 : handlers.at (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getHandlerForTarget (IUnknown* target) const
{
	VectorForEach (handlers, IPresetFileHandler*, handler)
		if(handler->canHandle (target))
			return handler;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getHandlerForFile (UrlRef url) const
{
	return getHandlerForFileType (url.getFileType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getHandlerForFileType (const FileType& fileType) const
{
	if(!fileType.isValid ()) // ignore empty file type
		return nullptr;

	for(auto handler : handlers)
	{
		FileType ft;
		int index = 0;
		while((ft = handler->getFileType (index++)).isValid ())
			if(ft == fileType)
				return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPresetFileHandler* CCL_API PresetFileRegistry::getHandlerForMimeType (StringID _mimeType) const
{
	String mimeType (_mimeType);
	if(mimeType.isEmpty ())
		return nullptr;

	for(auto handler : handlers)
	{
		FileType fileType;
		int index = 0;
		while((fileType = handler->getFileType (index++)).isValid ())
			if(fileType.getMimeType () == mimeType)
				return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetFileRegistry::collectFileTypes (IFileTypeFilter& fileTypes, IUnknown* target, int requiredHandlerFlags) const
{
	auto addTypesForHandler = [&] (IPresetFileHandler& handler)
	{
		FileType fileType;
		int index = 0;
		while((fileType = handler.getFileType (index++)).isValid ())
			fileTypes.addFileType (fileType);
	};

	if(target == nullptr && requiredHandlerFlags == 0)
	{
		for(auto handler : handlers)
			addTypesForHandler (*handler);
	}
	else
	{
		bool addDefaultHandler = false;

		for(auto handler : handlers)
			if((handler->getTargetFlags (target) & requiredHandlerFlags) == requiredHandlerFlags)
			{
				if(handler == defaultHandler)
				{
					if(requiredHandlerFlags & (IPresetFileHandler::kCanImport|IPresetFileHandler::kCanExport))
						addDefaultHandler = true;  // default handler should be last when collecting im/export formats
				}
				else
					addTypesForHandler (*handler);
			}

		if(addDefaultHandler)
			addTypesForHandler (*defaultHandler);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetFileRegistry::setSubFolderPrefix (UrlRef location, StringRef subFolder)
{
	ASSERT (!findSubFolderPrefixLocation (location))
	subFolderPrefixes.add (NEW UrlWithTitle (location, subFolder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API PresetFileRegistry::getSubFolderPrefix (UrlRef url) const
{
	UrlWithTitle* prefixLocation = findSubFolderPrefixLocation (url);
	return prefixLocation  ? prefixLocation->getTitle () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlWithTitle* PresetFileRegistry::findSubFolderPrefixLocation (UrlRef url) const
{
	return static_cast<UrlWithTitle*> (subFolderPrefixes.findIf ([&] (Object* obj)
	{
		auto* prefixLocation = static_cast<UrlWithTitle*> (obj);
		return prefixLocation->contains (url) || prefixLocation->isEqualUrl (url);
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetFileRegistry::notify (ISubject* s, MessageRef msg)
{
	if(msg == IPresetFileHandler::kPresetLocationsChanged)
	{
		UnknownPtr<IPresetFileHandler> handler (s);
		if(handler)
			System::GetPresetManager ().scanPresets (true);;
	}
}
