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
// Filename    : ccl/system/packaging/filetree.cpp
// Description : File Tree
//
//************************************************************************************************

#include "ccl/system/packaging/filetree.h"
#include "ccl/system/packaging/sectionstream.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ifileutilities.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Tree Storage Format
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	+---------------------------------+
	| 'Root'						  |	4 Bytes
	| version						  |	4 Bytes
 +->+---------------------------------+
 |	| child count					  | 4 Bytes
 |	| itemId ('File' or 'Fold')		  | 4 Bytes
 |	+---------------------------------+
 |	| attributes					  |	4 Bytes
 |	| file name						  |	null-terminated UTF-16 or UTF-8 string
 |	+---+-----------------------------+
 |		|
 |		+-->+-------------------------+
 |	    |   | file time				  |	9 Bytes (CompactDateTime format)
 |		|	| file data offset		  |	8 Bytes
 +------+	| file data size		  |	8 Bytes
			| file size on disk		  |	8 Bytes
			+-------------------------+
*/

static DEFINE_FOURCC (kFileID,   'F', 'i', 'l', 'e')
static DEFINE_FOURCC (kFolderID, 'F', 'o', 'l', 'd')
static DEFINE_FOURCC (kRootID,   'R', 'o', 'o', 't')

static inline bool isUTF8FileNameVersion (int version)
{
	return version >= 2;
}

//************************************************************************************************
// CompactDateTime
//************************************************************************************************

struct CompactDateTime // 9 Bytes
{
	int16 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minute;
	uint8 second;
	uint16 milliseconds;

	CompactDateTime ()
	{ memset (this, 0, sizeof(CompactDateTime)); }

	CompactDateTime& operator = (const DateTime& dt)
	{
		year = (int16)dt.getDate ().getYear ();
		month = (uint8)dt.getDate ().getMonth ();
		day = (uint8)dt.getDate ().getDay ();
		hour = (uint8)dt.getTime ().getHour ();
		minute = (uint8)dt.getTime ().getMinute ();
		second = (uint8)dt.getTime ().getSecond ();
		milliseconds = (uint16)dt.getTime ().getMilliseconds ();
		return *this;
	}

	operator DateTime () const
	{
		DateTime dt;
		dt.setDate (Date (year, month, day));
		dt.setTime (Time (hour, minute, second, milliseconds));
		return dt;
	}

	bool serialize (Streamer& s) const
	{
		s.write (year);
		s.write (month);
		s.write (day);
		s.write (hour);
		s.write (minute);
		s.write (second);
		s.write (milliseconds);
		return true;
	}

	bool deserialize (Streamer& s)
	{
		s.read (year);
		s.read (month);
		s.read (day);
		s.read (hour);
		s.read (minute);
		s.read (second);
		s.read (milliseconds);
		return true;
	}
};

//************************************************************************************************
// FileTreeStreamWrapper
//************************************************************************************************

class FileTreeStreamWrapper: public StreamAlias
{
public:
	FileTreeStreamWrapper (IStream* dataStream, IPackageItem* item)
	: StreamAlias (dataStream),
	  item (item)
	{}

	// IStream
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		// make IPackageItem accessible
		if(iid == ccl_iid<IPackageItem> () && item)
			return item->queryInterface (iid, ptr);

		return StreamAlias::queryInterface (iid, ptr);
	}

protected:
	SharedPtr<IPackageItem> item;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FileTreeFileSystem
//************************************************************************************************

FileTreeFileSystem::FileTreeFileSystem ()
: rootItem (nullptr),
  readOnly (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTreeFileSystem::~FileTreeFileSystem ()
{
	if(rootItem)
		rootItem->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTreeFileSystem::isCompressed () const
{
	return compressionType.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTreeFileSystem::isEncrypted  () const
{
	return encryptionType.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootFolderItem& FileTreeFileSystem::getRoot ()
{
	if(!rootItem)
		rootItem = NEW RootFolderItem;
	return *rootItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTreeFileSystem::setRoot (RootFolderItem* newRoot)
{
	take_shared<RootFolderItem> (rootItem, newRoot);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileTreeFileSystem::createFromFolder (UrlRef folderPath, int fileIteratorMode, IUrlFilter* filter,
										   IProgressNotify* progress, FileSystemItem* current)
{
	if(!current)
		current = &getRoot ();

	UnknownPtr<IPackageItemFilter> itemFilter (filter); // optional

	int count = 0;
	ForEachFile (System::GetFileSystem ().newIterator (folderPath, fileIteratorMode), path)
		if(!filter || filter->matches (*path))
		{
			FileSystemItem* newItem = nullptr;

			if(path->isFolder ())
			{
				ASSERT (fileIteratorMode & IFileIterator::kFolders)
				if(fileIteratorMode & IFileIterator::kFolders)
				{
					newItem = NEW FolderItem;
					count += createFromFolder (*path, fileIteratorMode, filter, progress, newItem);
				}
			}
			else if(path->isFile ())
			{
				newItem = NEW FileStreamItem;
				AutoPtr<Url> localPath = NEW Url (*path);
				((FileStreamItem*)newItem)->setLocalPath (localPath);

				// set Compression/Encryption state
				newItem->isCompressed (isCompressed ());
				newItem->isEncrypted (isEncrypted ());
			}

			if(newItem)
			{
				String fileName;
				path->getName (fileName);
				newItem->setName (fileName);
				current->addChild (newItem);

				// apply attributes via item filter
				if(itemFilter)
				{
					int attr = itemFilter->getPackageItemAttributes (*path);
					newItem->isHidden ((attr & IPackageItem::kHidden) != 0);
					newItem->isCompressed ((attr & IPackageItem::kCompressed) != 0);					
					newItem->isEncrypted ((attr & IPackageItem::kEncrypted) != 0);
					if(newItem->isEncrypted ())
						newItem->useExternalKey ((attr & IPackageItem::kUseExternalKey) != 0);
				}

				if(progress)
				{
					if(progress->isCanceled ())
						break;

					progress->updateAnimated (fileName);
				}

				count++;
			}
		}
	EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileTreeFileSystem::extractToFolder (UrlRef folderPath, bool deep, IUrlFilter* filter, IProgressNotify* progress, FileSystemItem* current)
{
	if(!current)
		current = &getRoot ();

	int count = 0;
	IterForEach (current->newIterator (), FileSystemItem, item)
		if(item->isFolder ())
		{
			if(deep)
			{
				if(filter)
				{
					Url itemPath;
					getItemUrl (itemPath, *item);
					if(!filter->matches (itemPath))
						continue;
				}

				Url subFolder (folderPath);
				subFolder.descend (item->getFileName (), Url::kFolder);
				count += extractToFolder (subFolder, true, filter, progress, item);
			}
		}
		else if(item->isFile ())
		{
			if(filter)
			{
				Url itemPath;
				getItemUrl (itemPath, *item);
				if(!filter->matches (itemPath))
					continue;
			}

			if(progress)
			{
				if(progress->isCanceled ())
					break;

				progress->updateAnimated (item->getFileName ());
			}

			Url dstPath (folderPath);
			dstPath.descend (item->getFileName ());

			AutoPtr<IStream> srcStream = openDataStream (*(FileStreamItem*)item, IStream::kOpenMode, nullptr/*no context!*/);
			AutoPtr<IStream> dstStream = System::GetFileSystem ().openStream (dstPath, IStream::kCreateMode, nullptr/*no context!*/);

			ASSERT (srcStream != nullptr && dstStream != nullptr)
			if(srcStream && dstStream)
			{
				AutoPtr<IProgressNotify> subProgress = progress ? progress->createSubProgress () : nullptr;
				int64 maxBytesToCopy = item->getSizeOnDisk ();
				bool copied = System::GetFileUtilities ().copyStream (*dstStream, *srcStream, subProgress, maxBytesToCopy) != 0;
				ASSERT (copied == true)
				if(copied)
					count++;
			}
		}
	EndFor
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSystemItem* FileTreeFileSystem::lookupItem (UrlRef url, bool create)
{
	FileSystemItem* item = unknown_cast<FileSystemItem> (getRoot ().lookupChild (url.getPath ()));
	if(item && item->isFolder () == (tbool)url.isFolder () && !item->isDeleted ())
		return item;

	if(create)
	{
		ASSERT (isReadOnly () == false)
		if(item && item->isDeleted ()) // hmm... what a special case ;-)
		{
			item->isDeleted (false);
			return item;
		}

		AutoPtr<IStringTokenizer> iter = url.getPath ().tokenize (Url::strPathChar);
		if(iter)
		{
			FileSystemItem* current = &getRoot ();
			while(!iter->done ())
			{
				uchar delimiter = 0;
				StringRef name = iter->nextToken (delimiter);
				bool isFolder = true;
				if(iter->done () && url.isFile ()) // last one is the file's name
					isFolder = false;

				// check if folder already exists...
				FileSystemItem* newItem = nullptr;
				if(isFolder)
					newItem = current->findChildNode<FolderItem> (name);

				if(!newItem)
				{
					newItem = isFolder ? (FileSystemItem*)NEW FolderItem : (FileSystemItem*)NEW FileStreamItem;
					newItem->setName (name);
					current->addChild (newItem);
				}

				current = newItem;
			}

			ASSERT (current != &getRoot ())
			return current;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTreeFileSystem::getItemPath (String& path, FileSystemItem& item)
{
	FileSystemItem* current = &item;
	while(current != &getRoot ())
	{
		if(!path.isEmpty ())
			path.prepend (Url::strPathChar);
		path.prepend (current->getFileName ());
		current = current->getParentNode<FileSystemItem> ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTreeFileSystem::getItemUrl (Url& url, FileSystemItem& item)
{
	String path;
	getItemPath (path, item);
	url.setPath (path, item.isFile () ? Url::kFile : Url::kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API FileTreeFileSystem::openStream (UrlRef url, int mode, IUnknown* context)
{
	bool create = (mode & IStream::kCreate) != 0;
	if(create && isReadOnly ())
		return nullptr;

	FileStreamItem* item = ccl_cast<FileStreamItem> (lookupItem (url, create));
	if(item)
	{
		if(create == true)
		{
			// set Compression/Encryption state
			item->isCompressed (isCompressed ());
			item->isEncrypted (isEncrypted ());
		}

		AutoPtr<IStream> dataStream = openDataStream (*item, mode, context);
		if(dataStream)
		{
			#if 1 // add wrapper to enable access to IPackageItem
			if(create == false)
				return NEW FileTreeStreamWrapper (dataStream, item);
			else
			#endif
				return dataStream.detach ();
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::fileExists (UrlRef url)
{
	if(url.getPath ().isEmpty ()) // empty path addresses root of this file tree
		return true;

	return lookupItem (url) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::getFileInfo (FileInfo& info, UrlRef url)
{
	FileStreamItem* item = ccl_cast<FileStreamItem> (lookupItem (url));
	if(!item)
		return false;

	info.flags = item->getAttributes () & FileSystemItem::kPublicAttrMask;

	info.createTime =
	info.modifiedTime =
	info.accessTime = item->getTime ();

	info.fileSize = item->getSizeOnDisk ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::removeFile (UrlRef url, int mode)
{
	ASSERT (mode == 0) // TODO: implement mode!	
	FileStreamItem* item = ccl_cast<FileStreamItem> (lookupItem (url));
	if(item)
	{
		item->isDeleted (true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::renameFile (UrlRef url, StringRef newName, int mode)
{
	CCL_NOT_IMPL ("Rename not implemented!\n")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* CCL_API FileTreeFileSystem::newIterator (UrlRef url, int mode)
{
	return NEW FileTreeIterator (*this, url, mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::createFolder (UrlRef url)
{
	if(isReadOnly ())
		return false;
	return lookupItem (url, true) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::removeFolder (UrlRef url, int mode)
{
	ASSERT (mode == 0)
	// TODO: implement mode!
	FolderItem* item = ccl_cast<FolderItem> (lookupItem (url));
	if(item)
	{
		item->isDeleted (true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTreeFileSystem::isCaseSensitive ()
{
	return true;
}

//************************************************************************************************
// FileTreeIterator
//************************************************************************************************

FileTreeIterator::FileTreeIterator (FileTreeFileSystem& fileSystem, UrlRef anchorPath, int mode)
: paths (*NEW ObjectArray),
  iter (nullptr)
{
	paths.objectCleanup (true);

	FileSystemItem* anchorItem = nullptr;
	if(anchorPath.getPath ().isEmpty ())
		anchorItem = &fileSystem.getRoot ();
	else
		anchorItem = fileSystem.lookupItem (anchorPath);

	bool wantFiles = (mode & IFileIterator::kFiles) != 0;
	bool wantFolders = (mode & IFileIterator::kFolders) != 0;
	bool wantHidden = (mode & kIgnoreHidden) == 0;

	if(anchorItem)
		ForEach (*anchorItem, FileSystemItem, item)
			if(item->isFile () && !wantFiles)
				continue;
			if(item->isFolder () && !wantFolders)
				continue;
			if(item->isHidden () && !wantHidden)
				continue;

			Url* path = NEW Url (anchorPath);
			path->descend (item->getName (), item->isFolder () ? Url::kFolder : Url::kFile);
			paths.add (path);
		EndFor

	iter = paths.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTreeIterator::~FileTreeIterator ()
{
	if(iter)
		iter->release ();
	paths.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IUrl* CCL_API FileTreeIterator::next ()
{
	return iter ? (Url*)iter->next () : nullptr;
}

//************************************************************************************************
// FileSystemItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileSystemItem, ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileSystemItem::FileSystemItem (StringRef fileName)
: ObjectNode (fileName),
  attributes (0),
  fileNameEncoding (Text::kUnknownEncoding)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileSystemItem::updateTime ()
{
	System::GetSystem ().getLocalTime (time);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileSystemItem::isPlain () const
{
	return !isCompressed () && !isEncrypted ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystemItem::isFile () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystemItem::isFolder () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API FileSystemItem::getFileName () const
{
	return getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FileSystemItem::getSizeOnDisk () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileSystemItem::getItemAttributes () const
{
	return getAttributes ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileSystemItem::getModifiedTime (FileTime& time) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileSystemItem::countSubItems () const
{
	return countChildren ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageItem* CCL_API FileSystemItem::getSubItem (int index) const
{
	return unknown_cast<FileSystemItem> (getChild (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileSystemItem::serialize (Streamer& s, int version) const
{
	// Attributes
	if(!s.write (attributes))
		return false;

	// File Name
	if(isUTF8FileNameVersion (version))
	{
		MutableCString utf8Name (getName (), Text::kUTF8);
		if(!s.writeCString (utf8Name))
			return false;
	}
	else
	{
		if(!s.writeString (getName ()))
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileSystemItem::deserialize (Streamer& s, int version)
{
	// Attributes
	if(!s.read (attributes))
		return false;

	// File Name
	String fileName;
	if(isUTF8FileNameVersion (version))
	{
		MutableCString utf8Name;
		if(!s.readCString (utf8Name))
			return false;
		fileName.appendCString (Text::kUTF8, utf8Name);
	}
	else
	{
		if(!s.readString (fileName))
			return false;
	}
	setName (fileName);
	return true;
}

//************************************************************************************************
// FileStreamItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileStreamItem, FileSystemItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStreamItem::FileStreamItem (StringRef fileName)
: FileSystemItem (fileName),
  fileDataOffset (0),
  fileDataSize   (0),
  fileSizeOnDisk (0),
  fileHeaderSize (0),
  crc32 (0),
  startsWithHeader (false),
  localPath (nullptr),
  temporaryFile (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileStreamItem::~FileStreamItem ()
{
	if(localPath)
		localPath->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileStreamItem::isFile () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FileStreamItem::getSizeOnDisk () const
{
	if(localPath)
	{
		FileInfo info;
		System::GetFileSystem ().getFileInfo (info, *localPath);
		return info.fileSize;
	}

	return fileSizeOnDisk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileStreamItem::getModifiedTime (FileTime& time) const
{
	time = getTime ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* FileStreamItem::getLocalPath () const
{
	return localPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileStreamItem::setLocalPath (Url* path)
{
	take_shared<Url> (localPath, path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamItem::unlinkLocalFile ()
{
	bool result = true;

	// delete if it is a temporary file...
	if(localPath && isTemporaryFile ())
	{
		result = System::GetFileSystem ().removeFile (*localPath) != 0;
		if(result)
			setTemporaryFile (false);
	}

	setLocalPath (nullptr);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamItem::serialize (Streamer& s, int version) const
{
	if(!FileSystemItem::serialize (s, version))
		return false;

	// Modified Time
	CompactDateTime packedTime;
	packedTime = getTime ();
	if(!packedTime.serialize (s))
		return false;

	s.write (fileDataOffset);
	s.write (fileDataSize);
	s.write (fileSizeOnDisk);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileStreamItem::deserialize (Streamer& s, int version)
{
	if(!FileSystemItem::deserialize (s, version))
		return false;

	// Modified Time
	CompactDateTime packedTime;
	if(!packedTime.deserialize (s))
		return false;
	setTime (packedTime);

	s.read (fileDataOffset);
	s.read (fileDataSize);
	s.read (fileSizeOnDisk);
	return true;
}

//************************************************************************************************
// FolderItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FolderItem, FileSystemItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

FolderItem::FolderItem ()
: folderHeaderOffset (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FolderItem::removeDeleted ()
{
	ForEach (getChildren (), FileSystemItem, item)
		if(item->isDeleted ())
		{
			removeChild (item);
			item->release ();
		}
		else
		{
			FolderItem* folderItem = ccl_cast<FolderItem> (item);
			if(folderItem)
				folderItem->removeDeleted ();
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FolderItem::isFolder () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API FolderItem::getSizeOnDisk () const
{
	int64 size = 0;
	ForEach (getChildren (), FileSystemItem, item)
		size += item->getSizeOnDisk ();
	EndFor
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderItem::serialize (Streamer& s, int version) const
{
	if(!FileSystemItem::serialize (s, version))
		return false;
	return serializeChildren (s, version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderItem::serializeChildren (Streamer& s, int version) const
{
	uint32 count = countChildren ();
	if(!s.write (count))
		return false;

	ForEach (getChildren (), FileSystemItem, item)
		FOURCC itemId = item->isFolder () ? kFolderID : kFileID;
		if(!s.write (itemId))
			return false;
		if(!item->serialize (s, version))
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderItem::deserialize (Streamer& s, int version)
{
	if(!FileSystemItem::deserialize (s, version))
		return false;
	return deserializeChildren (s, version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FolderItem::deserializeChildren (Streamer& s, int version)
{
	uint32 count = 0;
	if(!s.read (count))
		return false;

	for(uint32 i = 0; i < count; i++)
	{
		FOURCC itemId = {0};
		if(!s.read (itemId))
			return false;

		AutoPtr<FileSystemItem> newItem;
		if(itemId == kFolderID)
			newItem = NEW FolderItem;
		else if(itemId == kFileID)
			newItem = NEW FileStreamItem;

		if(!newItem)
			return false;

		if(!newItem->deserialize (s, version))
			return false;

		newItem->retain ();
		addChild (newItem);
	}
	return true;
}

//************************************************************************************************
// RootFolderItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (RootFolderItem, FolderItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootFolderItem::serialize (Streamer& s, int version) const
{
	if(!s.write (kRootID))
		return false;
	if(!s.write (version))
		return false;

	return serializeChildren (s, version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootFolderItem::deserialize (Streamer& s, int version)
{
	removeAll (); // just in case...

	FOURCC rootId = {0};
	int savedVersion = 0;
	if(!s.read (rootId))
		return false;
	if(!s.read (savedVersion))
		return false;

	if(rootId != kRootID || savedVersion != version)
		return false;

	return deserializeChildren (s, version);
}
