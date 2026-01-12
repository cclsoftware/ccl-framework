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
// Filename    : ccl/extras/extensions/icontentserver.h
// Description : Content Server Interface
//
//************************************************************************************************

#ifndef _ccl_icontentserver_h
#define _ccl_icontentserver_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IAsyncOperation;
interface IUnknownList;
interface IFileDescriptor;

namespace Web {
interface IWebCredentials; 
interface ITransfer; }

namespace Install {

class Manifest;

//************************************************************************************************
// ContentDefinition
//************************************************************************************************

struct ContentDefinition
{
	String title;
	String productId;
	String contentId;
	String version;
	tbool isExtension;

	explicit ContentDefinition (StringRef title = nullptr,
								StringRef productId = nullptr,
								StringRef contentId = nullptr,
								StringRef version = nullptr,
								tbool isExtension = false)
	: title (title),
	  productId (productId),
	  contentId (contentId),
	  version (version),
	  isExtension (isExtension)
	{}
};

//************************************************************************************************
// IContentServer
//************************************************************************************************

interface IContentServer: IUnknown
{
	enum UsageHint
	{
		kVersionCheck,
		kVersionCheckInBackground,
		kContentDownload
	};

	enum Options
	{
		kSuppressErrors = 1<<0,
		kSuppressLogin = 1<<1
	};

	enum UserContentOptions
	{
		kSkipPurchasedContent = 1<<2,
		kSkipSubscriptionContent = 1<<3
	};

	/** Get server title for display. */
	virtual StringRef getServerTitle () const = 0;
	
	/** Get alternative action url and title from the server response. */
	virtual void getContentVersionAction (IUrl& url, String& title, StringRef versionString) = 0;

	/** Request credentials, could be saved credentials or prompt depending on hint and options. */
	virtual IUnknown* requestCredentials (UsageHint hint, int options = 0) = 0;

	/** Request credentials (async call). */
	virtual IAsyncOperation* requestCredentialsAsync (UsageHint hint, int options = 0) = 0;

	/** Get URL to download given content package. */
	virtual void getContentURL (IUrl& url, StringRef productId, StringRef contentId, tbool isExtension, IUnknown* credentials) = 0;

	/** Get URL to download a product icon. */
	virtual void getIconURL (IUrl& url, StringRef productId, IUnknown* credentials) = 0;

	/** Create credentials for content download (optional). */
	virtual Web::IWebCredentials* createCredentialsForURL (IUnknown* credentials) = 0;

	/**	Get version of given content package, possibly using cached version information. 
		Response: version number string or error message. */
	virtual IAsyncOperation* getContentVersion (const ContentDefinition& definition, IUnknown* credentials) = 0;

	/** Purge the version cache. The next call to getContentVersion will fetch version information from server. */
	virtual void purgeVersionCache () = 0;

	/** Get error message from version check response. */
	virtual String getContentVersionError (StringRef versionString) = 0;

	/** Get list of content purchased by user. */
	virtual bool requestUserContentList (Manifest& manifest, int options = 0) = 0;
	
	/** Get existing license data. */
	virtual String getLicenseData (StringRef licenseId) = 0;

	/** Request new license data from server. */
	virtual String requestLicenseData (StringRef licenseId) = 0;

	/** Check if user can save backups on server. */
	virtual bool isUserBackupFeatureAvailable () const = 0;

	/** Get file size limit for backup (-1 means no limit). */
	virtual int64 getMaximumBackupFileSize () const = 0;

	/** Get list of backups available on server (IFileDescriptor). */
	virtual bool requestUserBackupList (IUnknownList& backups, int options = 0) = 0;

	/** Get URL to download backup. */
	virtual void getBackupURL (IUrl& url, const IFileDescriptor& descriptor, IUnknown* credentials) = 0;

	/** Create transfer to upload backup. */
	virtual Web::ITransfer* createUploadForBackup (UrlRef localFile, IUnknown* credentials) = 0;

	DECLARE_IID (IContentServer)
};

} // namespace Install
} // namespace CCL

#endif // _ccl_icontentserver_h

