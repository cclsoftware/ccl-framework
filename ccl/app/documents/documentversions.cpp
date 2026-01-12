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
// Filename    : ccl/app/documents/documentversions.cpp
// Description : Document Version Management
//
//************************************************************************************************

#include "ccl/app/documents/documentversions.h"
#include "ccl/app/documents/documentmetainfo.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/documents/documentmanager.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/base/buffer.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// DocumentVersionSelector::HistoryList
//************************************************************************************************

class DocumentVersionSelector::HistoryList: public ItemModel
{
public:
	HistoryList ();
	~HistoryList ();

	enum Columns
	{
		kIcon,
		kTitle,
		kDescription,
		kAge,
		kDate,
		kNumColumns
	};

	void rebuild (Document& document);
	const IUrl* getFocusPath ()			{ return focusPath; }
	int count () const					{ return entries.count (); }

	// ItemModel
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API getSortColumnID (MutableCString& columnID, tbool& upwards) override;
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	DECLARE_STRINGID_MEMBER (kOpenVersion)

protected:
	ObjectArray entries;
	const IUrl* focusPath;
	int selectIndex;

	DocumentDescription* resolve (ItemIndexRef index) const { return (DocumentDescription*)entries.at (index.getIndex ()); }
	void onSortColumnChanged (StringID columnID, tbool upwards);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace HistoryListColumns
{
	// column ids
	static const CStringPtr kTitle = "title";
	static const CStringPtr kDescription = "descr";
	static const CStringPtr kAge = "age";
	static const CStringPtr kDate = "date";
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (Title, "Title")
	XSTRING (Description, "Description")
	XSTRING (Original, "Original");
	XSTRING (Age, "Age")
	XSTRING (Date, "Date")
	XSTRING (NoOtherVersionsOfXWereSaved, "No other versions of \"%(1)\" were saved.")
	XSTRING (RestoringVersion_DoYouWantToReplaceVersionOrDiscard, "Restoring version \"%(1)\".\n\nDo you want to replace the saved version \"%(2)\" with the current state, or discard the current state?")
	XSTRING (Replace, "Replace")
	XSTRING (Discard, "Discard")
END_XSTRINGS

//************************************************************************************************
// DocumentDescription
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDescription::assign (UrlRef _path, bool isVersion)
{
	setPath (Url (_path));

	UrlDisplayString fileName (path, Url::kStringDisplayName);

	icon = FileIcons::instance ().createIcon (path);

	auto scanDateTime = [] (DateTime& date, StringRef fileName, String& prefix, String& suffix)
		{
			int length = fileName.length ();
			enum { kDateTimePatternLength = 15 }; // e.g. "20170907-161201"

			// search backwards for a reasonable date start
			for(int dateStart = length - kDateTimePatternLength; dateStart >= 0; dateStart--)
			{
				// date must start with a number and be preceded by space as delimiter
				if(Unicode::isDigit (fileName.at (dateStart))
					&& (dateStart == 0 || fileName.at (dateStart - 1) == ' ')
					&& System::GetFileUtilities ().scanDateTime (date, fileName.subString (dateStart), &prefix, &suffix))
					{
						prefix.insert (0, fileName.subString (0, dateStart));
						prefix.trimWhitespace ();
						return true;
					}
			}
			return false;
		};

	String prefix, suffix;
	if(isVersion && scanDateTime (date, fileName, prefix, suffix))
	{
		title = prefix;
		description = suffix;
	}
	else
	{
		FileInfo fileInfo;
		if(System::GetFileSystem ().getFileInfo (fileInfo, path))
			date = fileInfo.modifiedTime;
	}

	if(title.isEmpty ())
		title = fileName;

	if(isVersion)
	{
		String packageDescription;
		PackageInfo info;
		if(info.loadFromPackage (path))
		{
			DocumentMetaInfo metaInfo (info);
			title = metaInfo.getTitle ();
			packageDescription = DocumentVersions::getDisplayDescription (metaInfo);
		}

		if(!packageDescription.isEmpty ())
		{
			// append autosave suffixes to description from package (ignore other suffixes)
			if(description == DocumentVersions::strDocumentSnapshotSuffix || description == DocumentVersions::strAutosaveSnapshotSuffix)
				description = packageDescription << " " << description;
			else
				description = packageDescription;
		}
	}

	if(date != DateTime ())
	{
		age = Format::TimeAgo::print (date);
		dateString = Format::DateTime::print (date, Format::DateTime::kFriendlyDateTime);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentDescription::getSummary () const
{
	String summary;
	StringBuilder writer (summary);
	writer.setItemSeparator (CCLSTR (" - "));
	writer.addItem (title);
	if(!description.isEmpty ())
		writer.addItem (description);
	if(!age.isEmpty ())
		writer.addItem (age);
	if(!dateString.isEmpty ())
		writer.addItem (dateString);
	return summary;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentDescription::isAutoSave () const
{
	return getDescription ().endsWith (DocumentVersions::strAutosaveSnapshotSuffix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentDescription::compare (const Object& obj) const
{
	if(const DocumentDescription* other = ccl_cast<DocumentDescription> (&obj))
	{
		if(isAutoSave () != other->isAutoSave ()) // autosave last
			return isAutoSave () ? 1 : -1;

		return int(other->getDate ().toOrdinal () - date.toOrdinal ());
	}
	else
		return Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentDescription::toString (String& string, int flags) const
{
	string = getTitle ();
	return true;
}

#if 0 // not used anymore
//************************************************************************************************
// DocumentVersions::HistoryChecker
//************************************************************************************************

class DocumentVersions::HistoryChecker
{
public:
	HistoryChecker (DocumentVersions& versions, UrlRef fileToBeRestored);

	void checkDuplicateInHistory (StringRef currentDescription);

	PROPERTY_AUTO_POINTER (Url, existingHistoryPath, ExistingHistoryPath)

	PROPERTY_FLAG (flags, 1 << 0, mustCopyToHistory)
	PROPERTY_FLAG (flags, 1 << 1, mustRemoveHistoryDuplicate)
	PROPERTY_FLAG (flags, 1 << 2, historyChecked)

private:
	DocumentVersions& versions;
	UrlRef fileToBeRestored;
	AutoPtr<Url> existingHistoryFile;
	int flags;

	static bool isSameFile (UrlRef path1, UrlRef path2);
	static bool isSameContent (const File& file1, const File& file2);
};

//************************************************************************************************
// DocumentVersions::HistoryChecker
//************************************************************************************************

DocumentVersions::HistoryChecker::HistoryChecker (DocumentVersions& versions, UrlRef fileToBeRestored)
: versions (versions),
  fileToBeRestored (fileToBeRestored),
  flags (0)
{
	mustCopyToHistory (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::HistoryChecker::checkDuplicateInHistory (StringRef currentDescription)
{
	historyChecked (true);

	ObjectList history;
	history.objectCleanup (true);
	versions.buildHistory (history);

	ForEach (history, DocumentDescription, dd)
		if(dd->getDescription () == currentDescription)
		{
			existingHistoryPath = NEW Url (dd->getPath ());
			if(isSameFile (*existingHistoryPath, versions.documentPath))
			{
				mustCopyToHistory (false); // found the same file in the history, no need to copy or ask
				break;
			}
			// otherwise continue, another file might be the same
		}
	EndFor

	if(mustCopyToHistory () && existingHistoryPath)
	{
		// the found file in the history is different: ask user which one he wants to keep
		String question;

		DocumentDescription restoreDescription;
		restoreDescription.assign (fileToBeRestored, true);

		question.appendFormat (XSTR (RestoringVersion_DoYouWantToReplaceVersionOrDiscard), restoreDescription.getDescription (), currentDescription);

		enum Answers { kAnswerReplace = Alert::kFirstButton, kAnswerDiscard = Alert::kSecondButton };
		int answer = Alert::ask (question, XSTR (Replace), XSTR (Discard));

		if(answer == kAnswerReplace)
			mustRemoveHistoryDuplicate (true);
		else if(answer == kAnswerDiscard)
			mustCopyToHistory (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::HistoryChecker::isSameFile (UrlRef path1, UrlRef path2)
{
	File file1 (path1);
	File file2 (path2);

	FileInfo info1, info2;
	if(file1.getInfo (info1) && file2.getInfo (info2))
	{
		// must have same size
		if(info1.fileSize == info2.fileSize)
		{
			// accept same modification date, otherwise check content
			if(info1.modifiedTime == info2.modifiedTime)
				return true;

			#if 0
			// this does not work for package files, where each contained file gets the current date on save. todo: compare package directory and contents of contained files
			return isSameContent (file1, file2);
			#endif
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::HistoryChecker::isSameContent (const File& file1, const File& file2)
{
	AutoPtr<IStream> stream1 = file1.open ();
	AutoPtr<IStream> stream2 = file2.open ();
	if(!stream1 || !stream2)
		return false;

	stream1 = System::GetFileUtilities ().createBufferedStream (*stream1);
	stream2 = System::GetFileUtilities ().createBufferedStream (*stream2);

	const uint32 kBufferSize = 1024;
	Buffer buffer1 (kBufferSize, false);
	Buffer buffer2 (kBufferSize, false);

	while(true)
	{
		int bytesRead1 = stream1->read (buffer1, buffer1.getSize ());
		int bytesRead2 = stream2->read (buffer2, buffer2.getSize ());
		if(bytesRead1 != bytesRead2 || bytesRead1 < 0)
			return false;

		if(bytesRead1 == 0)
			return true;

		if(memcmp (buffer1, buffer2, bytesRead1) != 0)
			return false;
	}
	return true;
}
#endif

//************************************************************************************************
// DocumentVersions
//************************************************************************************************

bool DocumentVersions::supported = true;
const String DocumentVersions::strDocumentSnapshotSuffix (CCLSTR ("(Before Autosave)"));
const String DocumentVersions::strAutosaveSnapshotSuffix (CCLSTR ("(Autosaved)"));

MutableCString DocumentVersions::sortColumnID;
bool DocumentVersions::sortUpwards = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::isSupported () 
{ 
	return supported;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::setSupported (bool state)
{
	supported = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static DEFINE_ARRAY_COMPARE (SortByTitleDescription, DocumentDescription, lhs, rhs)
	// 1. autoSave last, 2. title, 3. description
	int c = DocumentVersions::compareAutoSave (*lhs, *rhs);
	if(c == 0)
		c = lhs->getTitle ().compareWithOptions (rhs->getTitle (), Text::kIgnoreCase|Text::kCompareNumerically);
	if(c == 0)
		c = lhs->getDescription ().compare (rhs->getDescription ());
	return c;
}

static DEFINE_ARRAY_COMPARE (SortByDescription, DocumentDescription, lhs, rhs)
	// 1. description, 2. autoSave last
	int c = lhs->getDescription ().compareWithOptions (rhs->getDescription (), Text::kIgnoreCase|Text::kCompareNumerically);
	if(c == 0)
		c = DocumentVersions::compareAutoSave (*lhs, *rhs);
	return c;
}

static DEFINE_ARRAY_COMPARE (SortByAge, DocumentDescription, lhs, rhs)
	return int(lhs->getDate ().toOrdinal () - rhs->getDate ().toOrdinal ());
}

static DEFINE_ARRAY_COMPARE (SortByDate, DocumentDescription, lhs, rhs)
	return int(rhs->getDate ().toOrdinal () - lhs->getDate ().toOrdinal ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DocumentVersions::compareAutoSave (DocumentDescription& lhs, DocumentDescription& rhs)
{
	if(lhs.isAutoSave () != rhs.isAutoSave ()) // autosave last
		return lhs.isAutoSave () ? 1 : -1;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Core::VectorCompareFunction* DocumentVersions::getCompareFunction (StringID id)
{
	if(id == HistoryListColumns::kTitle)
		return SortByTitleDescription;
	else if(id == HistoryListColumns::kDescription)
		return SortByDescription;
	else if(id == HistoryListColumns::kAge)
		return SortByAge;
	else
		return SortByDate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::getSortOrder (MutableCString& columnID, tbool& upwards)
{
	if(sortColumnID.isEmpty ())
	{
		Settings::instance ().getAttributes ("DocumentVersions").getCString (sortColumnID, "sortColumn");
		Settings::instance ().getAttributes ("DocumentVersions").getBool (sortUpwards, "sortUpwards");
		if(sortColumnID.isEmpty ())
			sortColumnID = HistoryListColumns::kDate;
	}
	columnID = sortColumnID;
	upwards = sortUpwards;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::setSortOrder (StringID columnID, tbool upwards)
{
	sortColumnID = columnID;
	sortUpwards = upwards;
	Settings::instance ().getAttributes ("DocumentVersions").set ("sortColumn", sortColumnID);
	Settings::instance ().getAttributes ("DocumentVersions").set ("sortUpwards", sortUpwards);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::sortDescriptions (ObjectArray& descriptions)
{
	MutableCString sortColumnID;
	tbool sortUpwards;
	getSortOrder (sortColumnID, sortUpwards);

	descriptions.sort (DocumentVersions::getCompareFunction (sortColumnID));
	if(sortUpwards)
		descriptions.reverse ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentVersions::DocumentVersions (UrlRef documentPath)
: documentPath (documentPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef DocumentVersions::getHistoryFolderName ()
{
	return CCLSTR ("History");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::getHistoryFolder (Url& path)
{
	path = Url (documentPath);
	path.ascend ();
	path.descend (getHistoryFolderName (), IUrl::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::makeHistoryPath (Url& path, const String* suffix, bool withTimeStamp)
{
	String name;
	documentPath.getName (name, false);

	makeHistoryPath (path, name, suffix, withTimeStamp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::makeHistoryPath (Url& path, StringRef baseName, const String* suffix, bool withTimeStamp)
{
	String name (baseName);
	if(withTimeStamp)
		System::GetFileUtilities ().appendDateTime (name);
	if(suffix && !suffix->isEmpty ())
	{
		if(!suffix->startsWith (" "))
			name << " ";
		name.append (*suffix);
	}
	System::GetFileUtilities ().makeValidFileName (name);

	Url folder;
	getHistoryFolder (folder);

	path = folder;
	path.descend (name, IUrl::kFile);
	path.setExtension (documentPath.getFileType ().getExtension (), false);

	int index = name.lastIndex (")");
	if(index == name.length () - 1)
	{
		// variation of makeUniqueFileName: add counter inside brackets to avoid confusion of counter with version name
		if(System::GetFileSystem ().fileExists (path))
		{
			name.truncate (index);
			name << "-";
			int counter = 1;
			do
			{
				path = folder;
				path.descend (String (name) << counter++ << ")", IUrl::kFile);
				path.setExtension (documentPath.getFileType ().getExtension (), false);
			} while(System::GetFileSystem ().fileExists (path));
		}
	}
	else
		System::GetFileUtilities ().makeUniqueFileName (System::GetFileSystem (), path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::makeVersionPath (Url& path)
{
	String fileName (makeVersionFileName (documentPath));
	makeHistoryPath (path, fileName, nullptr, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url DocumentVersions::makeVersionPathInDocumentFolder (UrlRef sourcePath)
{
	// make filenname from title + description of the existing "source" document file
	String newName (makeVersionFileName (sourcePath, false));

	String extension;
	documentPath.getExtension (extension);

	Url path (documentPath);
	path.setName (newName, Url::kFile);
	path.setExtension (extension, false);
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentVersions::makeVersionFileName (UrlRef path, bool forceDescription)
{
	// "Title (Description)"
	String fileName;
	AutoPtr<IAttributeList> metaAttribs (createMetaAttribs (path));
	if(metaAttribs)
	{
		DocumentMetaInfo metaInfo (*metaAttribs);
		String title (metaInfo.getTitle ());
		String description (getDisplayDescription (metaInfo, forceDescription));
		if(!description.isEmpty ())
			description.truncate (50).trimWhitespace ();

		fileName = title;
		fileName.trimWhitespace ();

		if(description.startsWith ("("))
			fileName << " " << description;
		else if(!description.isEmpty ())
			fileName << " (" << description << ")";
	}
	else
		path.getName (fileName, false);

	System::GetFileUtilities ().makeValidFileName (fileName);
	return fileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DocumentVersions::getDisplayDescription (const DocumentMetaInfo& metaInfo, bool forceDescription)
{
	String description (metaInfo.getDescription ());
	if(forceDescription && description.isEmpty ())
		description = String ("(") << XSTR (Original) << ")";
	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::appendOriginalSuffix (Url& path)
{
	String fileName, ext;
	path.getName (fileName, false);
	path.getExtension (ext);

	path.setName (fileName << " (" << XSTR (Original) << ")");
	path.setExtension (ext, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* DocumentVersions::createMetaAttribs (UrlRef path)
{
	DocumentManager& manager (DocumentManager::instance ());
	if(Document* document = manager.findDocument (path))
	{
		UnknownPtr<IAttributeList> metaAttribs (document->getMetaInfo ());
		return metaAttribs.detach ();
	}
	else
	{
		AutoPtr<PackageInfo> packageInfo (NEW PackageInfo);
		if(packageInfo->loadFromPackage (path))
			return packageInfo.detach ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::buildHistory (Container& list, int timeout)
{
	Url historyFolder;
	getHistoryFolder (historyFolder);

	String documentExtension;
	documentPath.getExtension (documentExtension);

	int64 endTime = timeout > 0 ? System::GetSystemTicks () + timeout : -1;
	bool result = true;

	ForEachFile (System::GetFileSystem ().newIterator (historyFolder, IFileIterator::kFiles), p)
		if(p->isFile ())
		{
			String extension;
			p->getExtension (extension);
			if(extension == documentExtension)
			{
				DocumentDescription* entry = NEW DocumentDescription;
				entry->assign (*p, true);
				list.add (entry);

				if(endTime > 0 && System::GetSystemTicks () >= endTime)
				{
					result = false;
					CCL_WARN ("Document version history timeout exceeded.\n", 0)
					break;
				}
			}
		}
	EndFor

	if(ObjectArray* arr = ccl_cast<ObjectArray> (&list))
		sortDescriptions  (*arr);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::copyOldFormatToHistory (const Document& doc)
{
	// add the generator string of the old document (includes the version number)
	String generator;
	UnknownPtr<IAttributeList> metaAttribs (doc.getMetaInfo ());
	if(metaAttribs)
		generator = DocumentMetaInfo (*metaAttribs).getGenerator ();

	ASSERT (!generator.isEmpty ())
	if(generator.isEmpty ())
		generator = "old format";

	generator = String (" (") << generator << ")";

	Url historyPath;
	makeHistoryPath (historyPath, &generator, false);
	return System::GetFileSystem ().copyFile (historyPath, doc.getPath ()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::moveDocumentVersionToHistory ()
{
	Url historyPath;
	makeVersionPath (historyPath);
	return System::GetFileSystem ().moveFile (historyPath, documentPath) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::moveDocumentToHistory (const IUrl* docFile, const String* suffix)
{
	if(isSupported () == false)
		return false;
	
	UrlRef source = docFile ? *docFile : documentPath;

	Url historyPath;
	makeHistoryPath (historyPath, suffix);
	return System::GetFileSystem ().moveFile (historyPath, source) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::purgeOldest (StringRef _description, int numFilesToKeep)
{
	String description (_description);
	description.trimWhitespace ();

	AutoPtr<Url> oldestFile;
	DateTime oldestDate;

	Url historyFolder;
	getHistoryFolder (historyFolder);

	String documentExtension;
	documentPath.getExtension (documentExtension);

	int numFound = 0;
	DocumentDescription docDescription;
	ForEachFile (System::GetFileSystem ().newIterator (historyFolder, IFileIterator::kFiles), p)
		if(p->isFile ())
		{
			String extension;
			p->getExtension (extension);
			if(extension == documentExtension)
			{
				String fileName;
				p->getName (fileName, false);
				if(fileName.endsWith (description))
				{
					docDescription.assign (*p, true);
					numFound++;

					if(!oldestFile || docDescription.getDate () < oldestDate)
					{
						oldestFile = NEW Url (*p);
						oldestDate = docDescription.getDate ();
					}
				}
			}
		}
	EndFor

	if(oldestFile && numFound > numFilesToKeep)
	{
		System::GetFileSystem ().removeFile (*oldestFile);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentVersions::restoreDocumentVersion (UrlRef historyFile)
{
	DocumentManager& manager (DocumentManager::instance ());

	if(Document* document = manager.findDocument (documentPath))
	{
		// force first save (without asking, will be moved to history anyway; otherwise closeDocument could remove the "empty" document folder)
		if(!System::GetFileSystem ().fileExists (documentPath))
			manager.saveDocument (document);

		if(!manager.closeDocument (document))
			return false;
	}

	// move document to history
	if(moveDocumentVersionToHistory ())
	{
		// this is the actual "restore": replace document file with history file
		Url newDocumentPath (makeVersionPathInDocumentFolder (historyFile));

		// move history file to document folder
		if(System::GetFileSystem ().copyFile (newDocumentPath, historyFile))
			System::GetFileSystem ().removeFile (historyFile);

		onActiveVersionChanged (documentPath, newDocumentPath);

		manager.openDocument (newDocumentPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersions::onActiveVersionChanged (UrlRef oldDocumentPath, UrlRef newDocumentPath)
{
	DocumentManager& manager (DocumentManager::instance ());

	// adjust recent file list
	bool wasPinned = manager.getRecentPaths ().isPathPinned (oldDocumentPath);
	if(manager.getRecentPaths ().removeRecentPath (oldDocumentPath))
	{
		manager.getRecentPaths ().setRecentPath (newDocumentPath);
		manager.getRecentPaths ().setPathPinned (newDocumentPath, wasPinned);
	}
}

//************************************************************************************************
// DocumentVersionSelector
//************************************************************************************************

DocumentVersionSelector::DocumentVersionSelector ()
: historyList (NEW HistoryList ()),
  document (nullptr)
{
	historyList->addObserver (this);
	addObject ("historyList", historyList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentVersionSelector::~DocumentVersionSelector ()
{
	historyList->removeObserver (this);
	historyList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersionSelector::runDialog (Document& document)
{
	SharedPtr<Document> holder (&document);
	this->document = &document;

	historyList->rebuild (document);

	if(historyList->count () == 0)
	{
		String text;
		text.appendFormat (XSTR (NoOtherVersionsOfXWereSaved), document.getTitle ());
		Alert::info (text);
	}
	else
	{
		IView* view = getTheme ()->createView ("CCL/RestoreDocumentVersion", this->asUnknown ());
		ASSERT (view)
		if(view)
		{
			int result = DialogBox ()->runDialog (view, Styles::kWindowCombinedStyleDialog, Styles::kDialogOkCancel);

			if(result == DialogResult::kOkay)
				if(const IUrl* path = historyList->getFocusPath ())
					DocumentVersions (document.getPath ()).restoreDocumentVersion (*path);
		}
	}
	this->document = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersionSelector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == HistoryList::kOpenVersion && document)
	{
		if(IWindow* window = System::GetDesktop ().getWindowByOwner (asUnknown ()))
			window->close ();

		UnknownPtr<IUrl> url (msg[0]);
		if(url && document)
			DocumentVersions (document->getPath ()).restoreDocumentVersion (*url);
	}
	else
		Component::notify (subject, msg);
}

//************************************************************************************************
// DocumentVersionSelector::HistoryList
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (DocumentVersionSelector::HistoryList, kOpenVersion, "openVersion")

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentVersionSelector::HistoryList::HistoryList ()
: focusPath (nullptr),
  selectIndex (-1)
{
	entries.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentVersionSelector::HistoryList::~HistoryList ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersionSelector::HistoryList::rebuild (Document& document)
{
	entries.removeAll ();
	focusPath = nullptr;

	DocumentVersions (document.getPath ()).buildHistory (entries);

	// for intial selection, find version with same description as current document
	DocumentDescription currentDescription;
	currentDescription.assign (document.getPath (), true);

	selectIndex = -1;
	int i = 0;
	ForEach (entries, DocumentDescription, dd)
		if(dd->getDescription () == currentDescription.getDescription ())
		{
			selectIndex = i;
			break;
		}
		i++;
	EndFor

	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (24);  // kIcon
	list.addColumn (160, XSTR (Title),			HistoryListColumns::kTitle, 50,			IColumnHeaderList::kSizable|IColumnHeaderList::kCanFit|IColumnHeaderList::kSortable); // kTitle
	list.addColumn (140, XSTR (Description),	HistoryListColumns::kDescription, 90,	IColumnHeaderList::kSizable|IColumnHeaderList::kCanFit|IColumnHeaderList::kSortable); // kDescription
	list.addColumn (80, XSTR (Age),				HistoryListColumns::kAge, 40,			IColumnHeaderList::kSizable|IColumnHeaderList::kCanFit|IColumnHeaderList::kSortable); // kAge
	list.addColumn (250, XSTR (Date),			HistoryListColumns::kDate, 100,			IColumnHeaderList::kSizable|IColumnHeaderList::kCanFit|IColumnHeaderList::kSortable); // kDate
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersionSelector::HistoryList::onSortColumnChanged (StringID columnID, tbool upwards)
{
	MutableCString sortColumnID;
	tbool sortUpwards;
	DocumentVersions::getSortOrder (sortColumnID, sortUpwards);

	if(columnID != sortColumnID || upwards != sortUpwards)
	{
		DocumentVersions::setSortOrder (columnID, upwards);
		DocumentVersions::sortDescriptions (entries);

		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::getSortColumnID (MutableCString& columnID, tbool& upwards)
{
	DocumentVersions::getSortOrder (columnID, upwards);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DocumentVersionSelector::HistoryList::countFlatItems ()
{
	return entries.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::getItemTitle (String& title, ItemIndexRef index)
{
	if(DocumentDescription* entry = resolve (index))
	{
		title = entry->getTitle ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info)
{
	if(DocumentDescription* entry = resolve (index))
	{
		switch(column)
		{
		case kIcon :
			if(IImage* icon = entry->getIcon ())
				size = icon->getWidth (), icon->getHeight ();
			return true;

		case kTitle :
			Font::measureString (size, entry->getTitle (), info.font);
			return true;

		case kDescription :
			Font::measureString (size, entry->getDescription (), info.font);
			return true;

		case kAge :
			Font::measureString (size, entry->getAge (), info.font);
			return true;

		case kDate :
			Font::measureString (size, entry->getDateString (), info.font);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::drawCell (ItemIndexRef index, int column, const DrawInfo& drawInfo)
{
	DocumentDescription* entry = resolve (index);
	if(!entry)
		return false;

	switch(column)
	{
	case kIcon :
		if(IImage* icon = entry->getIcon ())
			drawIcon (drawInfo, icon);
		break;

	case kTitle :
		drawTitle (drawInfo, entry->getTitle ());
		break;

	case kDescription :
		drawTitle (drawInfo, entry->getDescription ());
		break;

	case kAge :
		drawTitle (drawInfo, entry->getAge ());
		break;

	case kDate :
		drawTitle (drawInfo, entry->getDateString ());
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentVersionSelector::HistoryList::viewAttached (IItemView* itemView)
{
	ItemModel::viewAttached (itemView);

	(NEW Message ("select"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::onItemFocused (ItemIndexRef index)
{
	focusPath = nullptr;

	ItemIndex focusItem;
	if(IItemView* itemView = getItemView ())
		if(itemView->getFocusItem (focusItem))
			if(DocumentDescription* entry = resolve (focusItem))
				focusPath = &entry->getPath ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentVersionSelector::HistoryList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	DocumentDescription* entry = resolve (index);
	if(entry)
		signal (Message (kOpenVersion, (const_cast<Url&> (entry->getPath ()).asUnknown ())));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentVersionSelector::HistoryList::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IColumnHeaderList::kSortColumnChanged)
	{
		MutableCString columnID (msg[0].asString ());
		bool upwards = msg[1].asBool ();
		onSortColumnChanged (columnID, upwards);
	}
	else if(msg == "select")
	{
		if(IItemView* itemView = getItemView ())
			if(selectIndex >= 0)
				itemView->setFocusItem (selectIndex);
	}
}
