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
// Filename    : ccl/public/network/web/iwebfileservice.h
// Description : Web File Service Interface
//
//************************************************************************************************

#ifndef _ccl_iwebfileservice_h
#define _ccl_iwebfileservice_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

class FileType;
interface ISearcher;
interface ISearchDescription;
interface IFileDescriptor;
interface ITriggerAction;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web File Service Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to web file service. */
	DEFINE_STRINGID (kWebFiles, "CCL.WebFiles")

	/** Volumes have changed. arg[0]: volume name; arg[1]: type of change (optional, e.g. "mounted", "unmounted") */
	DEFINE_STRINGID (kVolumesChanged, "VolumesChanged")
		DEFINE_STRINGID (kVolumeChangeMounted, "mounted")
		DEFINE_STRINGID (kVolumeChangeUnmounted, "unmounted")

	/** Directory has changed (upload via transfer manager or file operation) args[0]: WebFS URL (IUrl) */
	DEFINE_STRINGID (kDirectoryChanged, "DirectoryChanged")

	/** Volume information has changed. arg[0]: volume name */
	DEFINE_STRINGID (kVolumeInfoChanged, "VolumeInfoChanged")

	/** Reveal given volume arg[0]: volume name */
	DEFINE_STRINGID (kRevealVolume, "RevealVolume")
}

namespace Web {

interface IWebCredentials;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Web File Service Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Meta
{
	// *** Messages ***
	/** arg[0]: tresult */
	DEFINE_STRINGID (kGetDirectoryCompleted, "getDirectoryCompleted")

	/** arg[0]: tresult */
	DEFINE_STRINGID (kFileTaskCompleted, "fileTaskCompleted")
}

interface ITransfer;
interface IFileTask;
interface IRemoteSession;

//************************************************************************************************
// IWebFileService
/**	Interface to mount server volumes into the virtual file system.

	Threading Policy:
	File servers can be mounted/unmounted by the main thread only,
	otherwise the methods will fail with kResultWrongThread!
*/
//************************************************************************************************

interface IWebFileService: IUnknown
{
	static const CCL::String kProtocol; ///< Web File Service URL protocol

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Volumes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Mount file server. */
	virtual tresult CCL_API mountFileServer (UrlRef serverUrl, StringRef name, StringRef label,
											 IWebCredentials* credentials = nullptr, StringRef type = nullptr,
											 IUnknown* serverHandler = nullptr) = 0;

	/** Unmount file server. */
	virtual tresult CCL_API unmountFileServer (StringRef name, tbool deferred = false) = 0;

	/** Remount file server with new credentials, and optionally new URL. */
	virtual tresult CCL_API remountFileServer (StringRef name, IWebCredentials* newCredentials, const IUrl* newUrl = nullptr) = 0;

	/** Check if file server is already mounted. */
	virtual tbool CCL_API isMounted (UrlRef serverUrl, IWebCredentials* credentials = nullptr) = 0;

	/** Translate URL on server to its equivalent in WebFS. */
	virtual tresult CCL_API translateServerUrl (IUrl& webfsUrl, UrlRef serverUrl,
												IWebCredentials* credentials = nullptr) = 0;

	/** Translate URL in WebFS to real URL on server. */
	virtual tresult CCL_API translateWebfsUrl (IUrl& serverUrl, UrlRef webfsUrl) = 0;

	/** Unmount all file servers and exit threads. */
	virtual tresult CCL_API terminate () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Items
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get handler for given volume (can be null, must be released otherwise). */
	virtual tresult CCL_API openHandler (UrlRef webfsUrl, UIDRef iid, void** object) = 0;

	template <class Interface> Interface* openHandler (UrlRef webfsUrl)
	{ Interface* iface = nullptr; openHandler (webfsUrl, CCL::ccl_iid<Interface> (), (void**)&iface); return iface; }

	/** Open file descriptor for given location (can be null, must be released otherwise). */
	virtual IFileDescriptor* CCL_API openFileItem (UrlRef webfsUrl) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// File Tasks
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get directory listing. Main thread must request in background with async = true. */
	virtual tresult CCL_API requestDirectory (IObserver* observer, UrlRef webfsUrl, tbool async = true) = 0;

	/** Discard cached directory listing at given location. */
	virtual tresult CCL_API discardDirectory (UrlRef webfsUrl, tbool async = true) = 0;

	/** Schedule file task to be performed in background. Task is shared. */
	virtual tresult CCL_API scheduleTask (IObserver* observer, UrlRef webfsUrl, IFileTask* task) = 0;

	/** Cancel asynchronous operation. */
	virtual tresult CCL_API cancelOperation (IObserver* observer) = 0;

	/** Open independent remote session to access given WebFS volume. This call might block! */
	virtual IRemoteSession* CCL_API openSession (UrlRef webfsUrl) = 0;

	/** Create searcher for WebFS volume. */
	virtual ISearcher* CCL_API createSearcher (ISearchDescription& description) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Uploads/Downloads
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create transfer object for downloading given WebFS file. */
	virtual tresult CCL_API createDownload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath) = 0;

	/** Create transfer object for uploading file to given WebFS folder. */
	virtual tresult CCL_API createUpload (ITransfer*& transfer, UrlRef webfsUrl, UrlRef localPath) = 0;

	/** Create trigger action for kDirectoryChanged signal. */
	virtual ITriggerAction* CCL_API createDirectoryChangedAction (UrlRef webfsUrl) = 0;

	DECLARE_IID (IWebFileService)
};

DEFINE_IID (IWebFileService, 0x28915aa, 0xed87, 0x4ea3, 0xa6, 0xb0, 0x68, 0xb1, 0xfb, 0x19, 0x2d, 0x3f)

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebfileservice_h
