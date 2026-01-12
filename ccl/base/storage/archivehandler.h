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
// Filename    : ccl/base/storage/archivehandler.h
// Description : Archive Handler
//
//************************************************************************************************

#ifndef _ccl_archivehandler_h
#define _ccl_archivehandler_h

#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/archive.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ipackagefile.h"

namespace CCL {

interface IStorable;
class ArchiveSaveTask;

//************************************************************************************************
// ArchiveHandler
/** Helper class to load/save objects from/to a structured storage. */
//************************************************************************************************

class ArchiveHandler: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ArchiveHandler, Object)

	ArchiveHandler (IFileSystem& fileSystem, StringID saveType = nullptr);
	~ArchiveHandler ();

	PROPERTY_MUTABLE_CSTRING (saveType, SaveType)
	PROPERTY_SHARED_AUTO (IProgressNotify, progress, Progress)
	PROPERTY_SHARED_AUTO (IPackageFile, sourcePackage, SourcePackage)

	Attributes& getContext ();

	/** Get handler associated with storage object. */
	static ArchiveHandler* getHandler (const Storage& storage);

	/** Get file system. */
	IFileSystem& getFileSystem ();

	/** Open stream from archive. */
	IStream* openStream (StringRef path, int mode);

	/** Load object state from XML stream. */
	bool loadItem (StringRef path, Archive::ObjectID name, Object& item, int xmlFlags = 0);

	/** Load external object state. */
	bool loadStream (StringRef path, IStorable& item);

	/** Copy data to memory stream. */
	IStream* copyData (StringRef path);

	/** Add archive task in save mode (takes ownership!). */
	bool addSaveTask (StringRef path, ArchiveSaveTask* task, int* attributes = nullptr);

	/** Add task using IStorable (shared!). */
	bool addSaveTask (StringRef path, IStorable& item, StringID debugName = nullptr, int* attributes = nullptr);

	/** Add task using IStream (shared!). */
	bool addSaveTask (StringRef path, IStream& data, int* attributes = nullptr);

	/** Add task using Object (shared!), will result in XML. */
	bool addSaveTask (StringRef path, Archive::ObjectID name, Object& item, int xmlFlags = 0);

	/** Add task copying an item from another package. */
	bool addCopyTask (IPackageFile* sourcePackage, StringRef path, StringRef destPath = nullptr);

	/** Add task as array element in save mode (takes ownership!). Returns index in array. */
	int addArrayItemTask (StringRef path, ArchiveSaveTask* task);

	/** Open array element stream from archive. */
	IStream* openArrayItem (StringRef path, int index);

protected:
	IFileSystem& fileSystem;
	Attributes context;
	static ArchiveHandler* toplevelHandler;

	class ArrayTocItem;
	class ArrayTask;
	class ArrayStream;
};

//************************************************************************************************
// ArchiveSaveTask
//************************************************************************************************

class ArchiveSaveTask: public Object,
					   public IPackageItemWriter
{
public:
	DECLARE_CLASS_ABSTRACT (ArchiveSaveTask, Object)

	CLASS_INTERFACE (IPackageItemWriter, Object)
};

} // namespace CCL

#endif // _ccl_archivehandler_h
