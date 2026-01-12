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
// Filename    : ccl/public/network/web/iwebfileclient.h
// Description : Web File Client Interface
//
//************************************************************************************************

#ifndef _ccl_iwebfileclient_h
#define _ccl_iwebfileclient_h

#include "ccl/public/base/datetime.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
	
interface IProgressNotify;
interface IStringDictionary;

namespace Web {
	
//////////////////////////////////////////////////////////////////////////////////////////////////
// Permissions
//////////////////////////////////////////////////////////////////////////////////////////////////
	
namespace Permission 
{
	DEFINE_ENUM (Permission)
	{
		kRead   = 1<<0,
		kModify = 1<<1,
		kDelete = 1<<2
	};
}

//************************************************************************************************
// IWebFileClient
/** Client interface for web based file systems protocol. */
//************************************************************************************************

interface IWebFileClient: IUnknown
{
	/** Server information. */
	struct ServerInfo
	{
		enum Flags 
		{
			kCanCreateFolders = 1<<8,
			kCanUploadFiles = 1<<9,
			kCanModifySpecific = 1<<10 ///< flags need to be checked per folder
		};

		int64 bytesTotal;
		int64 bytesFree;
		int64 maxContentLength;
		int flags;
		
		ServerInfo ()
		: flags (0),
		  bytesTotal (0),
		  bytesFree (0),
		  maxContentLength (0)
		{}
	};
	
	/** Directory entry. */
	struct DirEntry
	{
		enum Flags
		{
			kCanDownload = Permission::kRead,
			kCanRename = Permission::kModify,
			kCanDelete = Permission::kDelete,
			kCanUpload = 1<<3,
			kCanCreateFolder = 1<<4,
			kShared = 1<<5,
			kVersioned = 1<<6,
			kSharable = 1<<7,
			kAcceptsChildrenOnly = 1<<8,
			kCanMove = 1<<9
		};

		String name;
		DateTime creationDate;
		DateTime modifiedDate;
		String contentType;
		int64 contentLength;
		tbool directory;
		int flags;

		DirEntry ()
		: contentLength (0),
		  directory (false),
		  flags (0)
		{}
	};
	
	/** Directory iterator. */
	interface IDirIterator: IUnknown
	{
		/** Get next directory entry. */
		virtual const DirEntry* CCL_API getEntry (int index) const = 0;
		
		/** Get object associated with entry (can be null). */
		virtual IUnknown* CCL_API getObject (int index) const = 0;
	
		DECLARE_IID (IDirIterator)
	};

	/** Return server information in the info struct. */
	virtual tresult CCL_API getServerInfo (StringRef remotePath, ServerInfo& info) = 0;
	
	/** Return file information in the info struct. */
	virtual tresult CCL_API getFileInfo (StringRef remotePath, DirEntry& info) = 0;
	
	/** Make a directory at the given location. */
	virtual tresult CCL_API makeDirectory (String& resultPath, StringRef remotePath, StringRef name) = 0;
	
	/** Open a directory for reading. Iterator must be released by caller. */
	virtual IDirIterator* CCL_API openDirectory (StringRef remotePath, IProgressNotify* progress) = 0;

	/** Delete the resource are the given location. This is a deep delete. */
	virtual tresult CCL_API deleteResource (StringRef remotePath) = 0;
	
	/** Copy the resource to the new location. This is a deep copy. */
	virtual tresult CCL_API copyResource (String& resultPath, StringRef sourcePath, StringRef destPath) = 0;
	
	/** Move the resource to the new location (newName is optional. If it is set, the destPath is the new parent path)*/
	virtual tresult CCL_API moveResource (String& resultPath, StringRef sourcePath, StringRef destPath, StringRef newName = String::kEmpty) = 0;

	/** Upload local file to new remote ressource. */
	virtual tresult CCL_API uploadResource (String& resultPath, IStream& localStream, StringRef remotePath,
											StringRef fileName, StringID contentType, IProgressNotify* progress) = 0;

	DECLARE_IID (IWebFileClient)
};

DEFINE_IID (IWebFileClient, 0x5dda5c32, 0xd396, 0x4206, 0x9f, 0x2e, 0xb, 0xa, 0x2b, 0xbd, 0xc7, 0x63)
DEFINE_IID (IWebFileClient::IDirIterator, 0x7f28c25b, 0x33c4, 0x4d8a, 0xa2, 0x1e, 0xce, 0xb4, 0x9f, 0xe3, 0x4b, 0x0)

//************************************************************************************************
// IWebFileSearchClient
//************************************************************************************************

interface IWebFileSearchClient: IUnknown
{
	typedef IWebFileClient::DirEntry ResultEntry;
	typedef IWebFileClient::IDirIterator IResultIterator;

	DECLARE_STRINGID_MEMBER (kSearchTerms) ///< identifier of search terms in query
	DECLARE_STRINGID_MEMBER (kPaginationOffset) ///< identifier of pagination offset in query

	virtual IResultIterator* CCL_API search (StringRef remotePath, const IStringDictionary& query, IProgressNotify* progress) = 0;

	DECLARE_IID (IWebFileSearchClient)
};

DEFINE_IID (IWebFileSearchClient, 0xe9b6f6be, 0x85b7, 0x493e, 0x91, 0x6a, 0x2b, 0x16, 0xe, 0xa7, 0x1, 0x2c)
DEFINE_STRINGID_MEMBER (IWebFileSearchClient, kSearchTerms, "searchTerms")
DEFINE_STRINGID_MEMBER (IWebFileSearchClient, kPaginationOffset, "paginationOffset")

} // namespace Web
} // namespace CCL

#endif // _ccl_iwebfileclient_h
