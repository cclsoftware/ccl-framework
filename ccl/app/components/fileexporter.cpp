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
// Filename    : ccl/app/components/fileexporter.cpp
// Description : Component for exporting object as files
//
//************************************************************************************************

#include "ccl/app/components/fileexporter.h"

#include "ccl/base/objectconverter.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileExporter")
	XSTRING (Exporting, "Exporting")
	XSTRING (ExportingX, "Exporting %(1)")
	XSTRING (Exported, "Exported")
END_XSTRINGS

//************************************************************************************************
// ExportAlternative
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ExportAlternative, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExportAlternative::ExportAlternative (IExportFilter* filter)
{
	setFilter (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExportAlternative::makeFilePromises (IUnknown* object, IUnknown* context)
{
	return filter && filter->makeFilePromises (filePromises, object, context) != 0 && !filePromises.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExportAlternative::addFilePromise (IFilePromise* promise)
{
	filePromises.add (promise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileType ExportAlternative::getFileType () const
{
	FileType fileType;
	if(filter && filter->getFileType (fileType) && fileType.isValid ())
		return fileType;
	else
	{
		ForEachUnknown (filePromises, unk)
			UnknownPtr<IFilePromise> promise (unk);
			if(promise->getFileType (fileType) && fileType.isValid ())
				break;
		EndFor
	}
	return fileType;
}

//************************************************************************************************
// FileExporter::ExportTask
//************************************************************************************************

class FileExporter::ExportTask: public BatchOperation::Task
{
public:
	// Task
	String getProgressText () override;
	bool prepare () override;
	bool perform (IProgressNotify* progress) override;
	void onFinished () override;
	void onCanceled () override;

	PROPERTY_SHARED_AUTO (IFilePromise, filePromise, FilePromise)
};

//************************************************************************************************
// FileExporter
//************************************************************************************************

FileExporter::FileExporter ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::addSourceItem (IUnknown* item, IUnknown* context)
{
	// create (possible multiple) file promises for this item
	UnknownList filePromises;
	if(ObjectConverter::instance ().makeFilePromises (filePromises, item, context))
	{
		// add a task for each file to be created
		ForEachUnknown (filePromises, unk)
			UnknownPtr<IFilePromise> filePromise (unk);
			if(filePromise)
				addFilePromise (filePromise);
		EndFor
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::addFilePromise (IFilePromise* filePromise, StringRef destFileName)
{
	String fileName;
	FileType fileType;
	if(filePromise->getFileName (fileName) && filePromise->getFileType (fileType))
	{
		if(!destFileName.isEmpty ())
			fileName = destFileName;
		if(fileName.isEmpty ())
			fileName = XSTR (Exported);

		fileName = LegalFileName (fileName);

		Url path (destFolder);
		path.descend (fileName);

		// replace a presumed extension only if it matches a known file type
		tbool replaceExtension = System::GetFileTypeRegistry ().getFileTypeByUrl (path) != nullptr;
		path.setFileType (fileType, replaceExtension);

		// add export task
		ExportTask* task = NEW ExportTask;
		task->setFilePromise (filePromise);
		task->setDestPath (path);

		batchOperation.addTask (task);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::isAnyAsync () const
{
	ForEach (batchOperation, ExportTask, task)
		if(task->getFilePromise ()->isAsync ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExporter::changeDestFolders (UrlRef folder)
{
	ForEach (batchOperation, ExportTask, task)
		String fileName;
		task->getDestPath ().getName (fileName, true);

		Url path (folder);
		path.descend (fileName);
		task->setDestPath (path);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct FileExporter::ExportTaskToDestPath
{
	static Object* resolveObject (Object* obj)
	{
		FileExporter::ExportTask* task = reinterpret_cast<FileExporter::ExportTask*> (obj);
		return task ? const_cast<Url*> (&task->getDestPath ()) : nullptr;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* FileExporter::getDestPaths ()
{
	return NEW ResolvingIterator<ExportTaskToDestPath> (batchOperation.newIterator ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::run (StringRef progressTitle)
{
	return batchOperation.run (progressTitle.isEmpty () ? XSTR (Exporting) : progressTitle);
}

//************************************************************************************************
// FileExporter::ExportTask
//************************************************************************************************

String FileExporter::ExportTask::getProgressText ()
{
	return buildTextFromFileName (XSTR (ExportingX), getDestPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::ExportTask::prepare ()
{
	destPath.makeUnique ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExporter::ExportTask::perform (IProgressNotify* progress)
{
	return filePromise->createFile (getDestPath (), progress) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExporter::ExportTask::onFinished ()
{
	if(succeeded () && !filePromise->isAsync ())
		File (getDestPath ()).signalCreated ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExporter::ExportTask::onCanceled ()
{
	if(!filePromise->isAsync ())
		System::GetFileSystem ().removeFile (getDestPath ());
}
