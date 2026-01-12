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
// Filename    : ccl/system/fileutilities.h
// Description : File Utilities
//
//************************************************************************************************

#ifndef _ccl_fileutilities_h
#define _ccl_fileutilities_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/system/ifileutilities.h"

namespace CCL {

//************************************************************************************************
// FileUtilities
//************************************************************************************************

class FileUtilities: public Object,
					 public IFileUtilities
{
public:
	// IFileUtilities
	void CCL_API makeUniqueFileName (IFileSystem& fileSystem, IUrl& path, tbool forceSuffix = false) override;
	void CCL_API makeValidFileName (String& fileName) override;
	void CCL_API appendDateTime (String& fileName) override;
	tbool CCL_API scanDateTime (DateTime& time, StringRef fileName, String* prefix = nullptr, String* suffix = nullptr) override;
	UrlRef CCL_API makeUniqueTempFolder (IUrl& tempFolder) override;
	UrlRef CCL_API makeUniqueTempFile (IUrl& tempFile, StringRef name) override;
	tbool CCL_API copyStream (IStream& destStream, IStream& srcStream, IProgressNotify* progress = nullptr, int64 maxBytesToCopy = -1) override;
	IStream* CCL_API createSectionStream (IStream& inStream, int64 offset, int64 size, tbool writeMode = false) override;
	IStream* CCL_API createSeekableStream (IStream& inStream, tbool writeMode = false) override;
	IStream* CCL_API createBufferedStream (IStream& inStream, int bufferSize = -1) override;
	IMemoryStream* CCL_API createStreamCopyInMemory (IStream& inStream, IMemoryStream* destStream = nullptr) override;
	IStream* CCL_API createStringStream (StringRef string, TextEncoding encoding, int flags = 0) override;
	IUrl* CCL_API translatePathInMountedFolder (UrlRef path) override;

	CLASS_INTERFACE (IFileUtilities, Object)
};

//************************************************************************************************
// FileTypeRegistry
//************************************************************************************************

class FileTypeRegistry: public Object,
						public IFileTypeRegistry,
						public IFileTypeClassifier,
						public IFileHandler
{
public:
	DECLARE_CLASS (FileTypeRegistry, Object)
	DECLARE_METHOD_NAMES (FileTypeRegistry)

	FileTypeRegistry ();
	~FileTypeRegistry ();

	static FileTypeRegistry& instance ();

	// IFileTypeRegistry
	const FileType& CCL_API getDefaultFileType (int which) const override;
	tresult CCL_API registerFileType (const FileType& fileType) override;
	tresult CCL_API unregisterFileType (const FileType& fileType) override;
	tresult CCL_API updateFileType (const FileType& fileType) override;
	const FileType* CCL_API getFileTypeByUrl (UrlRef path) const override;
	const FileType* CCL_API getFileTypeByExtension (StringRef extension) const override;
	const FileType* CCL_API getFileTypeByMimeType (StringRef mimeType) const override;
	IFileTypeIterator* CCL_API newIterator () const override;
	tresult CCL_API registerHandler (IFileHandler* handler) override;
	tresult CCL_API unregisterHandler (IFileHandler* handler) override;
	IFileHandler& CCL_API getHandlers () override;
	IUnknownIterator* CCL_API newHandlerIterator () const override;
	void CCL_API setFileTypeClassifier (IFileTypeClassifier* classifier) override;

	// IFileTypeClassifier
	tbool CCL_API getFileTypeCategory (String& title, const FileType& fileType) const override;

	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;
	State CCL_API getState (IFileDescriptor& descriptor) override;
	tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) override;

	class FileTypeItem;

	CLASS_INTERFACE3 (IFileTypeRegistry, IFileTypeClassifier, IFileHandler, Object)

protected:
	class HandlerIterator: public Unknown,
						   public IUnknownIterator
	{
	public:
		HandlerIterator (const Vector<IFileHandler*>& handlers)
		: iter (handlers)
		{}

		// IUnknownIterator
		tbool CCL_API done () const override { return iter.done (); }
		IUnknown* CCL_API nextUnknown () override { return iter.next (); }
		CLASS_INTERFACE (IUnknownIterator, Unknown)

	protected:
		VectorIterator<IFileHandler*> iter;
	};

	ObjectArray fileTypes;
	Vector<IFileHandler*> handlers;
	SharedPtr<IFileTypeClassifier> fileTypeClassifier;

	FileTypeItem* findRegisteredType (const FileType& fileType) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_fileutilities_h
