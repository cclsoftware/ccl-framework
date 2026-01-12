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
// Filename    : ccl/public/system/ifileutilities.h
// Description : File Utilities Interface
//
//************************************************************************************************

#ifndef _ccl_ifileutilities_h
#define _ccl_ifileutilities_h

#include "ccl/public/base/datetime.h"
#include "ccl/public/text/textencoding.h"

namespace CCL {

class FileType;
interface IStream;
interface IMemoryStream;
interface IFileSystem;
interface IProgressNotify;
interface IFileHandler;
interface IFileDescriptor;
interface IFileTypeClassifier;
interface IUnknownIterator;

//************************************************************************************************
// IFileUtilities
/** File-related utilities. Access via System::GetFileUtilities().
	\ingroup ccl_system */
//************************************************************************************************

interface IFileUtilities: IUnknown
{
	/** Make filename unique by appending a numeric suffix (e.g "Filename(2).xxx")*/
	virtual void CCL_API makeUniqueFileName (IFileSystem& fileSystem, IUrl& path, tbool forceSuffix = false) = 0;

	/** Replace invalid characters in filename. */
	virtual void CCL_API makeValidFileName (String& fileName) = 0;

	/** Append current date & time to a filename. */
	virtual void CCL_API appendDateTime (String& fileName) = 0;

	/** Scan date & time from a filename that was created using appendDateTime. Optionally returns the text surrounding the date & time string. */
	virtual tbool CCL_API scanDateTime (DateTime& time, StringRef fileName, String* prefix = nullptr, String* suffix = nullptr) = 0;

	/** Generate unique subfolder name in temporary folder. */
	virtual UrlRef CCL_API makeUniqueTempFolder (IUrl& tempFolder) = 0;

	/** Generate unique file name in temporary folder. */
	virtual UrlRef CCL_API makeUniqueTempFile (IUrl& tempFile, StringRef name) = 0;

	/** Copy source to destination stream. */
	virtual tbool CCL_API copyStream (IStream& destStream, IStream& srcStream, IProgressNotify* progress = nullptr, int64 maxBytesToCopy = -1) = 0;

	/** Create sub-section of incoming stream. */
	virtual IStream* CCL_API createSectionStream (IStream& inStream, int64 offset, int64 size, tbool writeMode = false) = 0;

	/** Create seekable stream, retains incoming stream or creates memory copy. */
	virtual IStream* CCL_API createSeekableStream (IStream& inStream, tbool writeMode = false) = 0;

	/** Create buffered stream for incoming stream. */
	virtual IStream* CCL_API createBufferedStream (IStream& inStream, int bufferSize = -1) = 0;

	/** Create copy of stream in memory. */
	virtual IMemoryStream* CCL_API createStreamCopyInMemory (IStream& inStream, IMemoryStream* destStream = nullptr) = 0;

	enum StringStreamFlags
	{
		kSuppressByteOrderMark = 1<<0
	};
	/** Create IStream from string. */
	virtual IStream* CCL_API createStringStream (StringRef string, TextEncoding encoding, int flags = 0) = 0;

	/** Try to translate a native path inside a mounted package folder to a portable "package://" url; returns 0 if not applicable. */
	virtual IUrl* CCL_API translatePathInMountedFolder (UrlRef path) = 0;

	DECLARE_IID (IFileUtilities)
};

DEFINE_IID (IFileUtilities, 0x1a021959, 0xa7b6, 0x4e05, 0xbd, 0x43, 0x4a, 0x0, 0x42, 0x3c, 0x17, 0xa7)

//************************************************************************************************
// IFileTypeIterator
/** File type iterator.
	\ingroup ccl_system */
//************************************************************************************************

interface IFileTypeIterator: IUnknown
{
	/** Get next file type. */
	virtual const FileType* CCL_API nextFileType () = 0;

	DECLARE_IID (IFileTypeIterator)
};

DEFINE_IID (IFileTypeIterator, 0x5BCF2E11, 0x27C8, 0x472B, 0xB6, 0xEF, 0xFA, 0x74, 0x2A, 0xC4, 0x40, 0x68)

//************************************************************************************************
// IFileTypeRegistry
/**	File type registry. Access via System::GetFileTypeRegistry().

	Threading Policy: 
	The current implementation is NOT thread-safe! It must be called from the main thread only.

	\ingroup ccl_system */
//************************************************************************************************

interface IFileTypeRegistry: IUnknown
{
	/** Get default built-in file type by symbolic identifier (see filetype.h). */
	virtual const FileType& CCL_API getDefaultFileType (int which) const = 0;

	/** Register file type. */
	virtual tresult CCL_API registerFileType (const FileType& fileType) = 0;

	/** Unregister file type. */
	virtual tresult CCL_API unregisterFileType (const FileType& fileType) = 0;

	/** Update file type description. */
	virtual tresult CCL_API updateFileType (const FileType& fileType) = 0;

	/** Get file type by URL. */
	virtual const FileType* CCL_API getFileTypeByUrl (UrlRef path) const = 0;

	/** Get file type by file extension. */
	virtual const FileType* CCL_API getFileTypeByExtension (StringRef extension) const = 0;

	/** Get file type by MIME type. */
	virtual const FileType* CCL_API getFileTypeByMimeType (StringRef mimeType) const = 0;

	/** Iterate registered file types. */
	virtual IFileTypeIterator* CCL_API newIterator () const = 0;

	/** Register file handler. */
	virtual tresult CCL_API registerHandler (IFileHandler* handler) = 0;

	/** Unregister file handler. */
	virtual tresult CCL_API unregisterHandler (IFileHandler* handler) = 0;

	/** Get interface to combined file handlers. */
	virtual IFileHandler& CCL_API getHandlers () = 0;

	/** Iterate registered file handlers. */
	virtual IUnknownIterator* CCL_API newHandlerIterator () const = 0;

	/** Set external file type classifier. */
	virtual void CCL_API setFileTypeClassifier (IFileTypeClassifier* classifier) = 0;

	DECLARE_IID (IFileTypeRegistry)
};

DEFINE_IID (IFileTypeRegistry, 0x586c95bb, 0x895d, 0x4630, 0xa8, 0x49, 0x9f, 0x9b, 0x53, 0x7d, 0x23, 0x53)

//************************************************************************************************
// IFileHandler
/** Handler interface for opening files.
	\ingroup ccl_system */
//************************************************************************************************

interface IFileHandler: IUnknown
{
	DEFINE_ENUM (State)
	{
		kNotCompatible = -1,		///< file is not compatible
		kNotHandled,			///< file is not recognized
		kInstalled,				///< file is already installed
		kCanInstall,			///< file can be installed
		kCanUpdate				///< file is already installed, but can be updated
	};

	DECLARE_STRINGID_MEMBER (kOpenFile) ///< signal send by built-in file handler, args[0]: IUrl

	/** Open given file. */
	virtual tbool CCL_API openFile (UrlRef path) = 0;

	/** Get state for given file descriptor. */
	virtual State CCL_API getState (IFileDescriptor& descriptor) = 0;

	/** Get default location for given file descriptor. */
	virtual tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) = 0;

	DECLARE_IID (IFileHandler)
};

DEFINE_IID (IFileHandler, 0xe4b52ad0, 0x486b, 0x494c, 0x89, 0xc7, 0xb1, 0x8, 0xb5, 0x33, 0x3c, 0x26)
DEFINE_STRINGID_MEMBER (IFileHandler, kOpenFile, "openFile")

//************************************************************************************************
// AbstractFileHandler
//************************************************************************************************

class AbstractFileHandler: public IFileHandler
{
public:
	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override
	{
		return false;
	}

	State CCL_API getState (IFileDescriptor& descriptor) override
	{
		return kNotHandled;
	}

	tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) override
	{
		return false;
	}
};

//************************************************************************************************
// IFileTypeClassifier
//************************************************************************************************

interface IFileTypeClassifier: IUnknown
{
	/** Get a category string for the given type. */
	virtual tbool CCL_API getFileTypeCategory (String& title, const FileType& fileType) const = 0;

	DECLARE_IID (IFileTypeClassifier)
};

DEFINE_IID (IFileTypeClassifier, 0x5DE43D8A, 0xDC6E, 0x4CFA, 0xBD, 0xA5, 0x91, 0xDF, 0x08, 0x8B, 0xE4, 0x24)

} // namespace CCL

#endif // _ccl_ifileutilities_h
