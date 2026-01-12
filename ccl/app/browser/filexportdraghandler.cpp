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
// Filename    : ccl/app/browser/filexportdraghandler.cpp
// Description : Drag handler for exporting objects as files by dragging them onto a DirectoryNode
//
//************************************************************************************************

#include "ccl/app/browser/filexportdraghandler.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/components/fileexporter.h"

#include "ccl/base/objectconverter.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/ifileitem.h"

namespace CCL {
namespace Browsable {

//************************************************************************************************
// FileExportDraghandler::Entry
//************************************************************************************************

class FileExportDraghandler::Entry: public Object
{
public:
	Entry (IFilePromise* fp, IExportFilter* f = nullptr) 
	{ setFilePromise (fp); setFilter (f); }

	PROPERTY_SHARED_AUTO (IFilePromise, filePromise, FilePromise)
	PROPERTY_SHARED_AUTO (IExportFilter, filter, Filter)
};

//************************************************************************************************
// FileExportDraghandler::Alternative
//************************************************************************************************

class FileExportDraghandler::Alternative: public ExportAlternative
{
public:
	Alternative ()
	: spriteIndex (-1) {}

	PROPERTY_STRING (title, Title)
	PROPERTY_VARIABLE (int, spriteIndex, SpriteIndex)
};

} // namespace Browsable
} // namespace CCL

using namespace CCL;
using namespace Browsable;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileExport")
	XSTRING (Export, "Export")
	XSTRING (ExportTo, "Export to \"%(1)\"")
	XSTRING (Download, "Download")
	XSTRING (DownloadTo, "Download to \"%(1)\"")
END_XSTRINGS

//************************************************************************************************
// Browsable::FileExportDraghandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileExportDraghandler, FileDraghandlerBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileExportDraghandler::FileExportDraghandler (IView* view, Browser* browser)
: FileDraghandlerBase (view, browser),
  anyAsync (false),
  alternativeIndex (0),
  lastModifiers (0)
{
	entries.objectCleanup (true);
	alternatives.objectCleanup (true);

	canTryParentFolders (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileExportDraghandler::Alternative* FileExportDraghandler::getAlternative (IExportFilter* filter)
{
	ArrayForEach (alternatives, Alternative, a)
		if(a->getFilter () == filter)
			return a;
	EndFor

	Alternative* a = NEW Alternative;
	a->setFilter (filter);

	FileType fileType;
	filter->getFileType (fileType);
	a->setTitle (fileType.getDescription ());

	alternatives.add (a);
	return a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExportDraghandler::selectAlternative (int index)
{
	if(Alternative* selected = (Alternative*)alternatives.at (index))
	{
		alternativeIndex = index;

		if(sprite)
		{
			ArrayForEach (alternatives, Alternative, a)
				String text = makeAlternativeTitle (a->getTitle (), a == selected);
				spriteBuilder.replaceItemText (*sprite, a->getSpriteIndex (), text);
			EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FileExportDraghandler::makeAlternativeTitle (StringRef title, bool selected)
{
	if(selected)
		return String () << "[x] " << title;
	else
		return String () << "[  ] " << title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileExportDraghandler::Alternative* FileExportDraghandler::getSelectedAlternative ()
{
	if(!alternatives.isEmpty ())
		if(Alternative* a = (Alternative*)alternatives.at (alternativeIndex))
			return a;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExportDraghandler::makeEntries (IUnknown* item, IUnknown* context)
{
	// check if object already is a file promise
	if(UnknownPtr<IFilePromise> fp = item)
	{
		if(fp->isAsync ())
			anyAsync = true;
		entries.add (NEW Entry (fp));
		return true;
	}

	// try filters
	ObjectArray candidates;
	ListForEach (ObjectConverter::instance ().getExporters (), IExportFilter*, filter)
		UnknownList filePromises;
		if(filter->makeFilePromises (filePromises, item, context))
		{
			ForEachUnknown (filePromises, unk)
				UnknownPtr<IFilePromise> filePromise (unk);
				if(filePromise)
				{
					if(filePromise->isAsync ())
						anyAsync = true;
					candidates.add (NEW Entry (filePromise, filter));
				}
			EndFor
		}
	EndFor

	if(candidates.isEmpty ())
		return false;

	// TODO: check for different identity strings?
	ArrayForEach (candidates, Entry, entry)
		String identity (entry->getFilter ()->getIdentity ());
		if(identity.isEmpty ())
			entries.add (entry);
		else
		{
			Alternative* a = getAlternative (entry->getFilter ());
			a->addFilePromise (return_shared (entry->getFilePromise ()));
			entry->release ();
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExportDraghandler::addToSprite (IFilePromise& promise)
{
	String fileName;
	promise.getFileName (fileName);
	fileName = LegalFileName (fileName);

	FileType fileType;
	promise.getFileType (fileType);
	
	String dotExt;
	dotExt << "." << fileType.getExtension ();
	if(!fileName.endsWith (dotExt, false)) // could be part of file name already
		fileName << dotExt;

	AutoPtr<IImage> icon = FileIcons::instance ().createIcon (fileType);

	spriteBuilder.addItem (icon, fileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileExportDraghandler::checkDataTarget (IDataTarget* dataTarget, IDragSession* session)
{
	ASSERT (dataTarget && session)
	if(dataTarget)
	{
		bool dataTargetValid = dataTarget->canInsertData (data, session);
		if(dataTargetValid == false)
		{
			// second chance ask the data target to accept promised files
			UnknownList promises;

			auto addPromisedUrl = [&promises, this] (IFilePromise& promise)
			{
				String fileName;
				if(promise.getFileName (fileName))
				{
					Url promisePath (destFolder);
					promisePath.descend (fileName);
					FileType promiseType;
					if(promise.getFileType (promiseType))
						promisePath.setFileType (promiseType, true);
					promises.add (ccl_as_unknown (NEW Url (promisePath)));
				}
			};

			ArrayForEach (entries, Entry, entry)
				IFilePromise* promise = entry->getFilePromise ();
				if(promise)
					addPromisedUrl (*promise);
			EndFor

			if(Alternative* alternative = getSelectedAlternative ())
			{
				ForEachUnknown (alternative->getFilePromises (), unk)
					UnknownPtr<IFilePromise> promise (unk);
					if(promise)
						addPromisedUrl (*promise);
				EndFor
			}

			if(promises.isEmpty () == false)
				dataTargetValid = dataTarget->canInsertData (promises, session);
		}

		return dataTargetValid;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FileExportDraghandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	if(makeEntries (&item, context))
	{
		item.retain ();
		return &item;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileExportDraghandler::finishPrepare ()
{
	int itemCount = 0;
	spriteBuilder.addHeader (anyAsync ? XSTR (Download) : XSTR (Export));
	itemCount++;

	ArrayForEach (entries, Entry, entry)
		addToSprite (*entry->getFilePromise ());
		itemCount++;
	EndFor

	if(alternatives.count () == 1)
	{
		Alternative* a = (Alternative*)alternatives.at (0);
		ForEachUnknown (a->getFilePromises (), unk)
			UnknownPtr<IFilePromise> promise (unk);
			addToSprite (*promise);
			itemCount++;
		EndFor

	}
	else if(alternatives.count () >= 2)
	{
		int index = 0;
		ArrayForEach (alternatives, Alternative, a)
			a->setSpriteIndex (itemCount);
			String title = makeAlternativeTitle (a->getTitle (), index == alternativeIndex);
			spriteBuilder.addHeader (title);
			itemCount++;

			ForEachUnknown (a->getFilePromises (), unk)
				UnknownPtr<IFilePromise> promise (unk);
				addToSprite (*promise);
				itemCount++;
			EndFor

			index++;
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileExportDraghandler::dragOver (const DragEvent& event)
{
	SuperClass::dragOver (event);

	String pattern;
	if(anyAsync)
		pattern = targetNode ? XSTR (DownloadTo) : XSTR (Download);
	else
		pattern = targetNode ? XSTR (ExportTo) : XSTR (Export);

	if(sprite)
		spriteBuilder.replaceItemText (*sprite, 0, makeTitleWithDestFolder (pattern));

	// TEST: select alternative via modifier change
	uint32 modifiers = event.keys.getModifiers ();
	if(modifiers != lastModifiers)
	{
		lastModifiers = modifiers;
		if(modifiers && alternatives.count () >= 2)
		{
			int index = alternativeIndex + 1;
			if(index >= alternatives.count ())
				index = 0;
			selectAlternative (index);
		}
	}

	event.session.setResult (targetNode ? IDragSession::kDropCopyReal : IDragSession::kDropNone);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileExportDraghandler::afterDrop (const DragEvent& event)
{
	SuperClass::afterDrop (event);

	FileExporter exporter;
	exporter.setDestFolder (destFolder);

	ArrayForEach (entries, Entry, entry)
		exporter.addFilePromise (entry->getFilePromise ());
	EndFor

	if(Alternative* a = getSelectedAlternative ())
		ForEachUnknown (a->getFilePromises (), unk)
			UnknownPtr<IFilePromise> promise (unk);
			exporter.addFilePromise (promise);			
		EndFor

	bool result = exporter.run ();

	// notify target node
	if(result && targetNode && anyAsync == false)
		notifyTargetNode (exporter.getDestPaths ());

	return true;
}
