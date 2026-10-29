//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/fileselector.wasm.cpp
// Description : WebAssembly File Selector Implementation
//
//************************************************************************************************

#include "ccl/platform/wasm/gui/systemevent.wasm.h"

#include "ccl/gui/dialogs/fileselector.h"

#include "ccl/base/asyncoperation.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, createFileInput, (CStringPtr elementId, CStringPtr uid, CStringPtr accept, bool multiple),
{
	try
	{
		const id = UTF8ToString (elementId);
		const uidString = UTF8ToString (uid);
		let input = document.getElementById (id);

		// Remove existing input if present
		if(input)
			input.remove ();

		input = document.createElement ('input');
		input.type = 'file';
		input.id = id;
		input.style.display = 'none';

		if(accept)
		{
			const acceptString = UTF8ToString (accept);
			if(acceptString.length > 0)
				input.accept = acceptString;
		}

		if(multiple)
			input.multiple = true;

		input.addEventListener ('change', () =>
		{
			const files = input.files;
			if(files && files.length > 0)
			{
				CCL.onJSEvent ('fileselected', id, files.length);

				// Create unique directory for this selector using just the UID
				const selectorDir = '/tmp/' + uidString + '/';
				CCL.makeDirectory (selectorDir);

				for(let i = 0; i < files.length; i++)
				{
					const file = files[i];
					const path = selectorDir + file.name;

					CCL.readFileToFS (file, path)
						.then (() => CCL.onJSEvent ('fileavailable', id, i, path))
						.catch (() => CCL.onJSEvent ('error', id, i));
				}
			}
			else
				CCL.onJSEvent ('cancelled', id);
		});

		input.addEventListener ('cancel', () =>
		{
			CCL.onJSEvent ('cancelled', id);
		});

		document.body.appendChild (input);
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, openFileInput, (CStringPtr elementId),
{
	try
	{
		const id = UTF8ToString (elementId);
		const input = document.getElementById (id);
		if(!input)
			return false;

		input.click ();
		return true;
	}
	catch(e)
	{
		return false;
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removeFileInput, (CStringPtr elementId),
{
	try
	{
		const id = UTF8ToString (elementId);
		const input = document.getElementById (id);
		if(input)
			input.remove ();
	}
	catch(e)
	{}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (bool, downloadFile, (CStringPtr filename, CStringPtr sourcePath),
{
	try
	{
		const path = UTF8ToString (sourcePath);
		const data = FS.readFile (path);
		const blob = new Blob ([data]);
		CCL.downloadBlob (blob, UTF8ToString (filename));
		return true;
	}
	catch(e)
	{
		return false;
	}
});

} // namespace Wasm

//************************************************************************************************
// WasmFileSelector
//************************************************************************************************

class WasmFileSelector: public NativeFileSelector,
						public SystemEventHandler
{
public:
	DECLARE_CLASS (WasmFileSelector, NativeFileSelector)

	WasmFileSelector ();
	~WasmFileSelector ();

	// NativeFileSelector
	bool runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window) override;
	int CCL_API getSaveBehavior () const override;
	void CCL_API setSaveContent (UrlRef url) override;

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

protected:
	SharedPtr<AsyncOperation> operation;
	MutableCString selectorId;
	AutoPtr<Url> saveContentUrl;
	int expectedFileCount;
	int receivedFileCount;

	void initializeSelector ();
	void cleanupSelector ();
	void getElementId (MutableCString& elementId) const;
	void updateOperationState ();
	void onCancelled ();
	void onFileSelected (int count);
	void onFileAvailable (int index, StringRef path);
	void onError (int index);
};

//************************************************************************************************
// WasmFolderSelector
//************************************************************************************************

class WasmFolderSelector: public NativeFolderSelector
{
public:
	DECLARE_CLASS (WasmFolderSelector, NativeFolderSelector)

	// NativeFolderSelector
	bool runPlatformSelector (StringRef title, IWindow* window) override;
	IAsyncOperation* runPlatformSelectorAsync (StringRef title, IWindow* window) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmFileSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (WasmFileSelector, NativeFileSelector, "FileSelector")
DEFINE_CLASS_UID (WasmFileSelector, 0xacfd316a, 0x371d, 0x4ba2, 0x9b, 0x7e, 0x45, 0xce, 0xc8, 0x7a, 0x2c, 0xbf) // ClassID::FileSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmFileSelector::WasmFileSelector ()
: expectedFileCount (0),
  receivedFileCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmFileSelector::~WasmFileSelector ()
{
	cleanupSelector ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::initializeSelector ()
{
	UID uid;
	uid.generate ();
	uid.toCString (selectorId, UID::kCompact);

	MutableCString elementId;
	getElementId (elementId);
	SystemEventDelegate::instance ().registerEventHandler (elementId, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::cleanupSelector ()
{
	SystemEventDelegate::instance ().unregisterEventHandler (*this);
	if(!selectorId.isEmpty ())
	{
		MutableCString elementId;
		getElementId (elementId);
		removeFileInput (elementId);
		selectorId.empty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::getElementId (MutableCString& elementId) const
{
	elementId = "fileselector";
	elementId.append (selectorId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::onCancelled ()
{
	if(operation)
	{
		cleanupSelector ();
		operation->setStateDeferred (IAsyncOperation::kFailed);
		operation.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmFileSelector::runPlatformSelector (int type, StringRef title, int filterIndex, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WasmFileSelector::getSaveBehavior () const
{
	return kSaveNeedsContent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmFileSelector::setSaveContent (UrlRef url)
{
	if(!url.isEmpty ())
		saveContentUrl = NEW Url (url);
	else
		saveContentUrl = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WasmFileSelector::runPlatformSelectorAsync (int type, StringRef title, int filterIndex, IWindow* window)
{
	initializeSelector ();

	String acceptString;
	int filterCount = countFilters ();
	if(filterCount > 0)
	{
		for(int i = 0; i < filterCount; i++)
		{
			const FileType* filter = getFilter (i);
			if(i > 0)
				acceptString << ",";
			acceptString << "." << filter->getExtension ();
		}
	}

	MutableCString accept (acceptString, Text::kUTF8);
	bool multiple = (type == kOpenMultipleFiles);

	if(type == kSaveFile)
	{
		if(!saveContentUrl.isValid () || saveContentUrl->isEmpty ())
			return AsyncOperation::createFailed ();

		String fileName (getInitialFileName ());
		if(fileName.isEmpty ())
			saveContentUrl->getName (fileName, true);
		if(fileName.isEmpty ())
			fileName = "download";

		if(countFilters () > 0 && filterIndex >= 0 && filterIndex < countFilters ())
		{
			const FileType* filter = getFilter (filterIndex);
			if(!fileName.endsWith (filter->getExtension ()))
				fileName.append (".").append (filter->getExtension ());
		}

		MutableCString sourcePath (UrlFullString (*saveContentUrl, true), Text::kUTF8);
		MutableCString fileNameUtf8 (fileName, Text::kUTF8);

		if(downloadFile (fileNameUtf8, sourcePath))
		{
			Url* result = NEW Url;
			result->fromDisplayString (fileName, IUrl::kFile);
			paths.add (result);
			return AsyncOperation::createCompleted (true);
		}
		else
			return AsyncOperation::createFailed ();
	}
	if(type == kOpenFile || type == kOpenMultipleFiles)
	{
		MutableCString elementId;
		getElementId (elementId);

		if(!createFileInput (elementId, selectorId, accept, multiple))
			return AsyncOperation::createFailed ();

		operation = NEW AsyncOperation;

		if(!openFileInput (elementId))
		{
			cleanupSelector ();
			operation->setState (IAsyncOperation::kFailed);
		}
	}

	return return_shared<IAsyncOperation> (operation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmFileSelector::handleEvent (SystemEvent& e)
{
	if(e.eventId == "fileselected")
		onFileSelected (e.arg1.asInt ());
	else if(e.eventId == "fileavailable")
		onFileAvailable (e.arg1.asInt (), e.arg2.asString ());
	else if(e.eventId == "error")
		onError (e.arg1.asInt ());
	else if(e.eventId == "cancelled")
		onCancelled ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::onFileSelected (int count)
{
	expectedFileCount = count;
	receivedFileCount = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::onFileAvailable (int index, StringRef path)
{
	if(!path.isEmpty ())
	{
		AutoPtr<Url> result = NEW Url;
		if(result->fromDisplayString (path, IUrl::kFile))
			paths.add (result.detach ());
	}

	receivedFileCount++;
	updateOperationState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::onError (int index)
{
	receivedFileCount++;
	updateOperationState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmFileSelector::updateOperationState ()
{
	if(operation && receivedFileCount >= expectedFileCount)
	{
		cleanupSelector ();
		operation->setResult (!paths.isEmpty ());
		operation->setStateDeferred (paths.isEmpty () ? IAsyncOperation::kFailed : IAsyncOperation::kCompleted);
		operation.release ();
	}
}

//************************************************************************************************
// WasmFolderSelector
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (WasmFolderSelector, NativeFolderSelector, "FolderSelector")
DEFINE_CLASS_UID (WasmFolderSelector, 0x898fbf4d, 0x15d, 0x4754, 0x93, 0xa, 0xf1, 0x7a, 0xa7, 0x0, 0x82, 0xfc) // ClassID::FolderSelector

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmFolderSelector::runPlatformSelector (StringRef title, IWindow* window)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WasmFolderSelector::runPlatformSelectorAsync (StringRef title, IWindow* window)
{
	// TODO: Opening a browser file selector allowing to choose a folder on the host file system doesn't make much sense.
	// We wouldn't be able to write to a host file system path.
	// Instead, we could display a custom folder selector allowing to choose/create new folders in the persistent IndexedDB file system.
	return AsyncOperation::createFailed ();
}
