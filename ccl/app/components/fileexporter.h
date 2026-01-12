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
// Filename    : ccl/app/components/fileexporter.h
// Description : Component for exporting object as files
//
//************************************************************************************************

#ifndef _ccl_fileexporter_h
#define _ccl_fileexporter_h

#include "ccl/app/component.h"

#include "ccl/app/utilities/batchoperation.h"

#include "ccl/public/collections/unknownlist.h"

namespace CCL {

interface IFilePromise;
interface IExportFilter;

//************************************************************************************************
// FileExporter
/** Manages exporting objects as files. */
//************************************************************************************************

class FileExporter: public Component
{
public:
	FileExporter ();

	PROPERTY_OBJECT (Url, destFolder, DestFolder)

	bool addSourceItem (IUnknown* item, IUnknown* context = nullptr);
	bool addFilePromise (IFilePromise* filePromise, StringRef destFileName = nullptr); ///< filePromise is shared!
	bool isAnyAsync () const;

	void changeDestFolders (UrlRef folder);		///< changes folder for all previously added items
	Iterator* getDestPaths ();					///< iterates through all destination urls

	bool run (StringRef progressTitle = nullptr);		///< creates progress dialog

protected:
	BatchOperation batchOperation;

	class ExportTask;
	struct ExportTaskToDestPath;
};

//************************************************************************************************
// ExportAlternative
/** Helper class that collects file promises of an export filter. */
//************************************************************************************************

class ExportAlternative: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ExportAlternative, Object)

	ExportAlternative (IExportFilter* filter = nullptr);

	FileType getFileType () const;
	const UnknownList& getFilePromises () const { return filePromises; }

	bool makeFilePromises (IUnknown* object, IUnknown* context);	///< adds promises created by export filter
	void addFilePromise (IFilePromise* promise);					///< takes ownership

	PROPERTY_SHARED_AUTO (IExportFilter, filter, Filter)

protected:
	UnknownList filePromises;
};

} // namespace CCL

#endif // _ccl_fileexporter_h
