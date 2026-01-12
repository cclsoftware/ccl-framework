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
// Filename    : ccl/system/filemanager.h
// Description : File Manager
//
//************************************************************************************************

#ifndef _ccl_filemanager_h
#define _ccl_filemanager_h

#include "ccl/base/signalsource.h"
#include "ccl/base/singleton.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/system/ifilemanager.h"

namespace CCL {

interface IFileItemProvider;

//************************************************************************************************
// FileManager
/** Base class for platform-specific implementations. */
//************************************************************************************************

class FileManager: public Object,
			       public IFileManager,
				   public ExternalSingleton<FileManager>
{
public:
	DECLARE_CLASS_ABSTRACT (FileManager, Object)

	void signalFileCreated (UrlRef url, bool defer = false);
	void signalFileRemoved (UrlRef url, bool defer = false);
	void signalFileChanged (UrlRef url, bool defer = false);
	void signalFileMoved (UrlRef oldUrl, UrlRef newUrl, bool defer = false);

	// IFileManager
	tresult CCL_API addWatchedLocation (UrlRef url, int flags = kDeep) override;
	tresult CCL_API removeWatchedLocation (UrlRef url) override;
	tresult CCL_API setFileUsed (UrlRef url, tbool state) override;
	tresult CCL_API setFileWriting (UrlRef url, tbool state) override;
	IAsyncOperation* CCL_API triggerFileUpdate (UrlRef url) override;
	tbool CCL_API getFileDisplayString (String& string, UrlRef url, int type) const override;
	StringID CCL_API getFileLocationType (UrlRef url) const override;
	void CCL_API terminate () override;

	CLASS_INTERFACE (IFileManager, Object)

protected:
	SignalSource signalSource;
	ObjectList watchedUrls; // UrlItem
	ObjectList usedUrls; // UrlItem

	class UrlItem: public Object
	{
	public:
		DECLARE_CLASS_ABSTRACT (UrlItem, Object)

		UrlItem (UrlRef url)
		: url (url)
		{}

		Url url;
		int useCount = 0;		
		int flags = 0;

		static const int kWriting = 1 << 16;
		PROPERTY_FLAG (flags, kWriting, writing)
	};

	FileManager ();

	tresult setWatchedLocation (UrlRef url, bool state, int flags);

	UrlItem* getItemFromList (ObjectList& list, UrlRef url, bool create);
	bool buildDisplayPath (Url& displayPath, UrlRef url, IFileItemProvider& provider) const;

	// to be implemented by derived class:
	virtual tresult startWatching (UrlRef url, int flags);
	virtual tresult stopWatching (UrlRef url);
	virtual tresult startUsing (UrlRef url);
	virtual tresult stopUsing (UrlRef url);
	virtual void setWriting (UrlRef url, bool state);
};

} // namespace CCL

#endif // _ccl_filemanager_h
