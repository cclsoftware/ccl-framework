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
// Filename    : ccl/public/storage/iurl.h
// Description : URL interface
//
//************************************************************************************************

#ifndef _ccl_iurl_h
#define _ccl_iurl_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/unknown.h"

namespace CCL {

class FileType;

//************************************************************************************************
// IUrl
/** URL interface
	\ingroup base_io  */
//************************************************************************************************

interface IUrl: IUnknown
{
	/** URL types. */
	enum Types
	{
		kFile = 1,			///< URL describes a file location
		kFolder,			///< URL describes a folder location

		kDetect = 0,		///< type should be detected ("folder/" or "file")
		kIgnore = -1,		///< type argument should be ignored

		kMaxLength = 2048	///< maximum URL length
	};

	/** Normalization options. */
	enum NormalizationFlags
	{
		kRemoveDotSegments = 1<<0	///< remove any segments with ".." and "."
	};

	/** URL string representations. */
	enum StringTypes
	{
		kStringNativePath,  ///< platform style, not beautified
		kStringDisplayPath, ///< full beautified path 
		kStringDisplayName  ///< name only (without extension)
	};

	/** Clone URL object. */
	virtual void CCL_API clone (IUrl*& url) const = 0;

	/** Assign from other URL reference. */
	virtual void CCL_API assign (UrlRef url) = 0;

	/** Check two URLs for equality. */
	virtual tbool CCL_API isEqualUrl (UrlRef url, tbool withParameters = true) const = 0;

	/** Check if URL is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Get URL type (file or folder). */
	virtual int CCL_API getType () const = 0;

	/** Get full URL string (e.g. "file://localhost/folder1/folder2/file.ext"). */
	virtual void CCL_API getUrl (String& url, tbool withParameters = false) const = 0;

	/** Set full URL string and type. */
	virtual void CCL_API setUrl (StringRef url, int type = kFile) = 0;

	/** Get protocol string (e.g. "file"). */
	virtual StringRef CCL_API getProtocol () const = 0;

	/** Set protocol string. */
	virtual void CCL_API setProtocol (StringRef protocol) = 0;

	/** Get host string (e.g. "localhost"). */
	virtual StringRef CCL_API getHostName () const = 0;

	/** Set host string. */
	virtual void CCL_API setHostName (StringRef name) = 0;

	/** Get path string (folder + file name, e.g. "folder1/folder2/file.ext"). */
	virtual StringRef CCL_API getPath () const = 0;

	/** Set path string (folder + file name). */
	virtual void CCL_API setPath (StringRef path, int type = kIgnore) = 0;

	/** Get path name without file name (e.g. "folder1/folder2"). */
	virtual void CCL_API getPathName (String& pathName) const = 0;

	/** Get file name with or without extension (e.g. "file" or "file.ext"). */
	virtual void CCL_API getName (String& name, tbool withExtension = true) const = 0;

	/** Set file name and extension. */
	virtual void CCL_API setName (StringRef name, int type = kIgnore) = 0;

	/** Get extension string (e.g. "ext") */
	virtual tbool CCL_API getExtension (String& ext) const = 0;

	/** Set extension string, either by appending or replacing the existing one (e.g. "file.new" or "file.old.new"). */
	virtual void  CCL_API setExtension (StringRef ext, tbool replace = true) = 0;

	/** Get file type (extension + description + MIME type). Not all fields are guaranteed to be valid. */
	virtual const FileType& CCL_API getFileType () const = 0;

	/** Set extension with file type, either by appending or replacing the existing one. */
	virtual void CCL_API setFileType (const FileType& type, tbool replaceExtension = true) = 0;

	/** Check if URL is a root path (e.g. path is empty or "/" or "C:"). */
	virtual tbool CCL_API isRootPath () const = 0;

	/** Check if URL is a native path using "file" protocol (e.g. "file:///C:/folder/file.ext"). */
	virtual tbool CCL_API isNativePath () const = 0;

	/** Convert to native path string in UTF-16 enconding (e.g. "C:\folder\file.ext" or "Volumes/folder/file.ext"). */
	virtual tbool CCL_API toNativePath (uchar* pathBuffer, int bufferSize) const = 0;

	/** Assign native path. On Windows a path string like "C:\..." will be mapped to "file:///C:/...". */
	virtual tbool CCL_API fromNativePath (const uchar* pathBuffer, int type = kFile) = 0;

	/** Convert to POSIX-style path ('/' as separator) in UTF-8 encoding. */
	virtual tbool CCL_API toPOSIXPath (char* pathBuffer, int bufferSize) const = 0;

	/** Assign from POSIX-style path. */
	virtual tbool CCL_API fromPOSIXPath (const char* pathBuffer, int type = kFile) = 0;
	
	/** Get beautified string for display respecting the platform conventions. */
	virtual tbool CCL_API toDisplayString (String& displayString, int which = kStringNativePath) const = 0;

	/** Assign path from native display string, following platform conventions. */
	virtual tbool CCL_API fromDisplayString (StringRef displayString, int type = kFile) = 0;

	/** Check if it is an absolute path. */
	virtual tbool CCL_API isAbsolute () const = 0;

	/** Check if it is a relative path. */
	virtual tbool CCL_API isRelative () const = 0;

	/** Make relative URL absolute to base URL. */
	virtual tbool CCL_API makeAbsolute (UrlRef baseUrl) = 0;

	/** Make absolute URL relative to given base URL. */
	virtual tbool CCL_API makeRelative (UrlRef baseUrl) = 0;

	/** Ascend one directory level (e.g. "folder1/folder2" becomes "folder1"). */
	virtual tbool CCL_API ascend () = 0;

	/** Descend one directory level appending given name and type. */
	virtual tbool CCL_API descend (StringRef name, int type = kFile) = 0;

	/** Normalize URL with given options. */
	virtual void CCL_API normalize (int flags) = 0;

	/** Access to dictionary with (decoded) parameters. */
	virtual IStringDictionary& CCL_API getParameters () const = 0;

	/** Get parameters as URL-encoded string. */
	virtual void CCL_API getParameters (String& params) const = 0;

	/** Set parameters from URL-encoded string. */
	virtual void CCL_API setParameters (StringRef params) = 0;

	/** Check if URL has parameters. */
	virtual tbool CCL_API hasParameters () const = 0;

	DECLARE_IID (IUrl)

//////////////////////////////////////////////////////////////////////////////////////////////////
	
	inline bool isFile () const		{ return getType () == kFile; }
	inline bool isFolder () const	{ return getType () == kFolder; }
	inline IUrl& operator= (UrlRef url) { assign (url); return *this; }
};

DEFINE_IID (IUrl, 0xbfca729d, 0x5097, 0x4b38, 0x9f, 0x15, 0x52, 0x13, 0xf9, 0x79, 0x0, 0xc4)

//************************************************************************************************
// IUrlFilter
/** URL filter used as callback interface for file operations.
	\ingroup base_io  */
//************************************************************************************************

interface IUrlFilter: IUnknown
{
	/** Return true to include given URL. */
	virtual tbool CCL_API matches (UrlRef url) const = 0;

	DECLARE_IID (IUrlFilter)
};

DEFINE_IID (IUrlFilter, 0xe87eefd2, 0xe062, 0x4ce5, 0x98, 0x83, 0xc1, 0xa2, 0x6e, 0xea, 0xe4, 0x3d)

//************************************************************************************************
// IFileTypeFilter
/** Filter using a list of allowed filetypes.
	\ingroup base_io  */
//************************************************************************************************

interface IFileTypeFilter: IUnknown
{
	/** Add filetype. */
	virtual void CCL_API addFileType (const FileType& type) = 0;

	/** Get number of filetypes. */
	virtual int CCL_API countFileTypes () const = 0;

	/** Get filetype at index. */
	virtual const FileType& CCL_API getFileType (int index) const = 0;

	/** Return true to include given filetype. */
	virtual tbool CCL_API matches (const FileType& fileType) const = 0;

	DECLARE_IID (IFileTypeFilter)
};

DEFINE_IID (IFileTypeFilter, 0xBAE218AC, 0xEFE3, 0x4E57, 0xAA, 0x3E, 0x30, 0xE8, 0x02, 0x8B, 0xC0, 0xC0)

//************************************************************************************************
// UrlFilter
/** URL filter that matches any url.
	\ingroup base_io  */
//************************************************************************************************

class UrlFilter: public Unknown,
				 public IUrlFilter
{
public:
	// IUrlFilter
	tbool CCL_API matches (UrlRef url) const override { return true; }

	CLASS_INTERFACE (IUrlFilter, Unknown)
};

//************************************************************************************************
// NativePath
/** Helper class for converting URL to native path.
	\ingroup base_io  */
//************************************************************************************************

struct NativePath
{
	uchar path[IUrl::kMaxLength];

	NativePath ()
	{ path[0] = 0; }

	NativePath (UrlRef url)
	{ url.toNativePath (path, IUrl::kMaxLength); }

	bool isValid () const			{ return path[0] != 0; }
	int size () const				{ return sizeof(path)/sizeof(uchar); }
	operator const uchar* () const	{ return path; }
	operator uchar* ()				{ return path; }
};

//************************************************************************************************
// POSIXPath
/** Helper class for converting URL to POSIX-style path.
	\ingroup base_io  */
//************************************************************************************************

struct POSIXPath
{
	char path[IUrl::kMaxLength];

	POSIXPath ()
	{ path[0] = 0; }

	POSIXPath (UrlRef url)
	{ url.toPOSIXPath (path, IUrl::kMaxLength); }

	bool isValid () const			{ return path[0] != 0; }
	int size () const				{ return sizeof(path); }
	operator const char* () const	{ return path; }
	operator char* ()				{ return path; }
};

//************************************************************************************************
// UrlDisplayString
/** \ingroup base_io */
//************************************************************************************************

class UrlDisplayString: public String
{
public:
	UrlDisplayString (UrlRef url, int mode = IUrl::kStringNativePath) { url.toDisplayString (*this, mode); }
};

//************************************************************************************************
// UrlFullString
//	\ingroup base_io */
//************************************************************************************************

class UrlFullString: public String
{
public:
	UrlFullString (UrlRef url, bool withParameters = false) { url.getUrl (*this, withParameters); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Predefined Url parameter keys. */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace UrlParameter
{
	/** Display name of the chosen file (overrides the Url's filename, e.g. in an encoded url scheme). */
	DEFINE_STRINGID (kDisplayName, "DisplayName")
	
	/**	Package identifier of the file */
	DEFINE_STRINGID (kPackageID, "PackageID")
}

} // namespace CCL

#endif // _ccl_iurl_h
