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
// Filename    : ccl/base/storage/file.cpp
// Description : File class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/file.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/jsonarchive.h"

#include "ccl/public/text/iregexp.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/storage/istorage.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// RegExpSearchDescription
//************************************************************************************************

class RegExpSearchDescription: public SearchDescription
{
public:
	RegExpSearchDescription (UrlRef startPoint, StringRef searchTerms, int options);
	~RegExpSearchDescription ();

	bool isValid () const { return regExp != nullptr; }

	// SearchDescription
	tbool CCL_API matchesName (StringRef name) const override;

private:
	IRegularExpression* regExp;
};

//************************************************************************************************
// TokenizedSearchDescription
//************************************************************************************************

class TokenizedSearchDescription: public SearchDescription
{
public:
	TokenizedSearchDescription (UrlRef startPoint, StringRef searchTerms, int options, StringRef delimiter);
	
	// SearchDescription
	tbool CCL_API matchesName (StringRef name) const override;
	int CCL_API getSearchTokenCount () const override;
	StringRef CCL_API getSearchToken (int index) const override;
	StringRef CCL_API getTokenDelimiter () const override;

private:
	ObjectArray searchDescriptions;
	String delimiter;
};

//************************************************************************************************
// FindFilesIterator
//************************************************************************************************

class IFileIteratorMixin: public IFileIterator
{
public:
	virtual const IUrl* nextUrl () = 0; // workaround for method name conflict with Iterator class
	const IUrl* CCL_API next () override { return nextUrl (); }
};

class FindFilesIterator: public HoldingIterator,
						 public IFileIteratorMixin
{
public:
	FindFilesIterator (Container* container, Iterator* iter): HoldingIterator (container, iter) {}
	const IUrl* nextUrl () override { return (Url*)HoldingIterator::next (); }
	CLASS_INTERFACE (IFileIterator, HoldingIterator)
};

//************************************************************************************************
// LockFile
//************************************************************************************************

namespace LockFile 
{
	static const CString kAppName = "application";
	static const CString kExecutablePath = "executablepath";

	bool readLockFile (String& executablePath, String& applicationName, UrlRef filePath);
	bool writeLockFile (UrlRef filePath, StringRef executablePath, StringRef applicationName);
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TempFile
//************************************************************************************************

TempFile::TempFile (StringRef fileName)
{
	System::GetFileUtilities ().makeUniqueTempFile (path, fileName);

	// force creation
	bool created = create ();
	ASSERT (created == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TempFile::TempFile (IStream& data, StringRef fileName)
{
	System::GetFileUtilities ().makeUniqueTempFile (path, fileName);

	// copy data
	AutoPtr<IStream> stream = open (IStream::kCreateMode);
	ASSERT (stream.isValid () == true)
	if(stream)
	{
		if(UnknownPtr<IMemoryStream> memStream = &data)
			memStream->writeTo (*stream);
		else
			System::GetFileUtilities ().copyStream (*stream, data);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TempFile::~TempFile ()
{
	if(!exists ())
		return;

	bool removed = remove ();
	ASSERT (removed == true)
}

//************************************************************************************************
// LockFile
//************************************************************************************************

bool LockFile::lockDirectory (UrlRef _path, StringRef _applicationName)
{
	Url path (_path);

	Threading::ProcessID pid = System::GetProcessSelfID ();
	path.descend (String () << pid << "." << getLockFileType ().getExtension (), Url::kFile);
	
	Url modulePath;
	if(System::GetExecutableLoader ().getExecutablePath (modulePath, pid) != kResultOk)
		return false;

	String pathString;
	modulePath.toDisplayString (pathString);
	
	String applicationName (_applicationName);
	if(applicationName.isEmpty ())
		modulePath.getName (applicationName, false);

	return writeLockFile (path, pathString, applicationName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LockFile::unlockDirectory (UrlRef _path)
{
	Url path (_path);

	Threading::ProcessID pid = System::GetProcessSelfID ();
	path.descend (String () << pid << "." << getLockFileType ().getExtension (), Url::kFile);
	
	{
		String lockingModulePath;
		String lockingAppName;
		
		if(readLockFile (lockingModulePath, lockingAppName, path) == false)
			return false;
		
		Url modulePath;
		if(System::GetExecutableLoader ().getExecutablePath (modulePath, pid) != kResultOk)
			return false;

		String pathString;
		modulePath.toDisplayString (pathString);

		if(lockingModulePath != pathString)
			return false;
	}

	return System::GetFileSystem ().removeFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LockFile::isDirectoryLocked (UrlRef path)
{
	StringList nameList;
	getLockingApplicationNames (nameList, path);
	return nameList.isEmpty () == false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LockFile::getLockingApplicationNames (StringList& nameList, UrlRef path)
{
	ForEachFile (System::GetFileSystem ().newIterator (path, IFileIterator::kFiles), filePath)
		if(filePath->getFileType () == getLockFileType ())
		{
			String name;
			Threading::ProcessID pid = 0;
			filePath->getName (name, false);
			name.getIntValue (pid);
			
			if(pid == System::GetProcessSelfID ())
				continue;
			
			String lockingModulePath;
			String lockingAppName;
		
			if(readLockFile (lockingModulePath, lockingAppName, *filePath) == false)
				continue;

			Url modulePath;
			if(System::GetExecutableLoader ().getExecutablePath (modulePath, pid) != kResultOk)
			{
				System::GetFileSystem ().removeFile (*filePath);
				continue;
			}

			String pathString;
			modulePath.toDisplayString (pathString);

			if(pathString != lockingModulePath)
				System::GetFileSystem ().removeFile (*filePath);
			else if(nameList.contains (lockingAppName) == false)
				nameList.add (lockingAppName);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& LockFile::getLockFileType ()
{
	static FileType fileType (nullptr, "lock");
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LockFile::readLockFile (String& executablePath, String& applicationName, UrlRef filePath)
{
	Attributes attributes;
	if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (filePath))
	{
		if(JsonArchive (*stream).loadAttributes (nullptr, attributes))
		{
			Variant value;
			if(attributes.getAttribute (value, kAppName) == false || value.isString () == false)
				return false;
			applicationName = value.asString ();

			if(attributes.getAttribute (value, kExecutablePath) == false || value.isString () == false)
				return false;
			executablePath = value.asString ();

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LockFile::writeLockFile (UrlRef filePath, StringRef executablePath, StringRef applicationName)
{
	Attributes attributes;
	if(attributes.setAttribute (kAppName, applicationName) == false)
		return false;
	if(attributes.setAttribute (kExecutablePath, executablePath) == false)
		return false;

	if(AutoPtr<IStream> stream = System::GetFileSystem ().openStream (filePath, IStream::kCreateMode))
	{
		if(JsonArchive (*stream).saveAttributes (nullptr, attributes))
			return true;
	}
	return false;
}

//************************************************************************************************
// ScopedLockFile
//************************************************************************************************

ScopedLockFile::ScopedLockFile (UrlRef _path, StringRef applicationName, bool enable)
: path (_path),
  enabled (enable)
{
	if(enabled == false)
		return;

	if(path.isFile ())
		path.ascend ();
	bool succeeded = LockFile::lockDirectory (path, applicationName);
	if(succeeded == false)
	{
		ASSERT (succeeded)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScopedLockFile::~ScopedLockFile ()
{
	if(enabled == false)
		return;

	bool succeeded = LockFile::unlockDirectory (path);
	if(succeeded == false)
	{
		ASSERT (succeeded)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScopedLockFile::isLocked () const
{
	if(enabled == false)
		return false;
	return LockFile::isDirectoryLocked (path);
}

//************************************************************************************************
// File
//************************************************************************************************

INativeFileSystem& File::getFS ()
{
	return System::GetFileSystem ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMemoryStream* File::loadBinaryFile (UrlRef path, IFileSystem* fileSystem)
{
	if(fileSystem == nullptr)
		fileSystem = &System::GetFileSystem ();

	AutoPtr<IStream> stream = fileSystem->openStream (path);
	if(stream == nullptr)
		return nullptr;

	return System::GetFileUtilities ().createStreamCopyInMemory (*stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::save (UrlRef path, const IMemoryStream& srcStream)
{
	AutoPtr<IStream> dstStream = getFS ().openStream (path, IStream::kCreateMode);
	if(dstStream == nullptr)
		return false;

	int size = static_cast<int> (srcStream.getBytesWritten ());
	return dstStream->write (srcStream.getMemoryAddress (), size) == size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::save (UrlRef path, const IStorable& storable)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	if(stream == nullptr)
		return false;

	return storable.save (*stream) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::load (UrlRef path, IStorable& storable)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kOpenMode);
	if(stream == nullptr)
		return false;

	return storable.load (*stream) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void File::signalFile (StringID signalID, UrlRef path)
{
	SignalSource (Signals::kFileSystem).signal (Message (signalID, const_cast<IUrl*> (&path)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* File::findFiles (UrlRef startPoint, StringRef searchPattern, int mode)
{
	AutoPtr<Container> result = NEW ObjectList;
	result->objectCleanup (true);

	bool wantFiles = (mode & IFileIterator::kFiles) != 0;
	bool wantFolders = (mode & IFileIterator::kFolders) != 0;
	AutoPtr<SearchDescription> description = SearchDescription::create (startPoint, searchPattern);

	ForEachFile (getFS ().newIterator (startPoint), p)
		if(p->isFile () && !wantFiles)
			continue;
		if(p->isFolder () && !wantFolders)
			continue;

		String fileName;
		p->getName (fileName);
		if(!description->matchesName (fileName))
			continue;

		result->add (NEW Url (*p));
	EndFor

	return NEW FindFilesIterator (result, result->newIterator ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* File::filterIterator (IFileIterator& iter, const IUrlFilter& filter)
{
	AutoPtr<Container> result = NEW ObjectList;
	result->objectCleanup (true);

	const IUrl* p;
	while((p = iter.next ()) != nullptr)
	{
		if(filter.matches (*p))
			result->add (NEW Url (*p));
	}

	return NEW FindFilesIterator (result, result->newIterator ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::isFolderEmpty (UrlRef path)
{
	// check if folder contains files
	AutoPtr<IFileIterator> iter = getFS ().newIterator (path, IFileIterator::kFiles);
	if(iter && iter->next () != nullptr)
		return false;

	// check subfolders
	ForEachFile (getFS ().newIterator (path, IFileIterator::kFolders), childPath)
		if(!isFolderEmpty (*childPath))
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::copyFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter* filter, bool recursive)
{
	ForEachFile (getFS ().newIterator (srcPath), path)
		if(filter && !filter->matches (*path))
			continue;
		
		String name;
		path->getName (name);

		if(path->isFolder () && recursive)
		{
			Url dstSubPath (dstPath);
			dstSubPath.descend (name, Url::kFolder);
			if(!copyFolder (dstSubPath, *path, filter, true))
				return false;
		}
		else if(path->isFile ())
		{
			Url dstFilePath (dstPath);
			dstFilePath.descend (name, Url::kFile);
			if(!getFS ().copyFile (dstFilePath, *path))
				return false;
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (File, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

File::File ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

File::File (UrlRef path)
: path (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::exists () const
{
	return getFS ().fileExists (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::create () const
{
	if(path.isFile ())
	{
		AutoPtr<IStream> stream = open (IStream::kCreateMode);
		return stream.isValid ();
	}
	else
		return getFS ().createFolder (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::remove (int mode) const
{
	if(path.isFile ())
		return getFS ().removeFile (path, mode) != 0;
	else
		return getFS ().removeFolder (path, mode) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::rename (StringRef newName, int mode)
{
	if(getFS ().renameFile (path, newName, mode))
	{
		path.setName (newName);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::getInfo (FileInfo& info) const
{
	return getFS ().getFileInfo (info, path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileIterator* File::newIterator (int mode) const
{
	return getFS ().newIterator (path, mode);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* File::open (int mode, IUnknown* context) const
{
	return getFS ().openStream (path, mode, context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::getVolumeInfo (VolumeInfo& info) const
{
	return getFS ().getVolumeInfo (info, path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::isLocal () const
{
	return getFS ().isLocalFile (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::isHidden () const
{
	return getFS ().isHiddenFile (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::isWriteProtected () const
{
	return getFS ().isWriteProtected (path) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::moveTo (UrlRef dstPath, int mode, IProgressNotify* progress)
{
	if(getFS ().moveFile (dstPath, this->path, mode, progress))
	{
		setPath (dstPath);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::copyTo (UrlRef dstPath, int mode, IProgressNotify* progress) const
{
	return getFS ().copyFile (dstPath, this->path, mode, progress) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool File::setTime (const FileTime& modifiedTime) const
{
	return getFS ().setFileTime (path, modifiedTime) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (File)
	DEFINE_PROPERTY_NAME ("path")
END_PROPERTY_NAMES (File)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API File::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "path")
	{
		// TODO: optimize this!!!
		var.takeShared (AutoPtr<IUrl> (NEW Url (path)));
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API File::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "path")
	{
		UnknownPtr<IUrl> newPath (var.asUnknown ());
		if(newPath)
			setPath (*newPath);
		else
			setPath (Url::kEmpty);
		return true;
	}
	else
		return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (File)
	DEFINE_METHOD_NAME ("exists")
	DEFINE_METHOD_ARGS ("remove", "bool, bool")
	DEFINE_METHOD_ARGS ("rename", "newName")
	DEFINE_METHOD_ARGS ("moveTo", "dstPath")
	DEFINE_METHOD_ARGS ("copyTo", "dstPath")
END_METHOD_NAMES (File)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API File::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "exists")
	{
		returnValue = exists ();
		return true;
	}
	else if(msg == "remove")
	{
		int mode = 0;

		if(msg.getArgCount () == 0)
			mode |= IFileSystem::kDeleteToTrashBin;
		if(msg.getArgCount () >= 1)
		{
			if(msg[0].asBool ())
				mode |= IFileSystem::kDeleteToTrashBin;
		}
		if(msg.getArgCount () >= 2)
		{
			if(msg[1].asBool () && path.isFolder ())
				mode |= IFileSystem::kDeleteRecursively;
		}
		else if(path.isFolder ())
			mode |= IFileSystem::kDeleteRecursively;

		returnValue = remove (mode);
		return true;
	}
	else if(msg == "rename")
	{
		// TODO: mode???
		String newName (msg[0].asString ());
		returnValue = rename (newName);
		return true;
	}
	else if(msg == "moveTo")
	{
		// TODO: mode, progress???
		UnknownPtr<IUrl> dstPath (msg[0].asUnknown ());
		ASSERT (dstPath)
		returnValue = dstPath && moveTo (*dstPath);
		return true;
	}
	else if(msg == "copyTo")
	{
		// TODO: mode, progress???
		UnknownPtr<IUrl> dstPath (msg[0].asUnknown ());
		ASSERT (dstPath)
		returnValue = dstPath && copyTo (*dstPath);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// FileDescriptor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileDescriptor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileDescriptor::FileDescriptor (StringRef fileName, int64 fileSize)
: fileName (fileName),
  fileSize (fileSize),
  metaInfo (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileDescriptor::~FileDescriptor ()
{
	safe_release (metaInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& FileDescriptor::getMetaInfo ()
{
	if(!metaInfo)
		metaInfo = NEW Attributes;
	return *metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getTitle (String& title) const
{
	if(!explicitTitle.isEmpty ())
	{
		title = explicitTitle;
		return true;
	}
	else
	{
		title = fileName;
		int index = title.lastIndex (".");
		if(index > 0)
			title.truncate (index);
		return !title.isEmpty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getFileName (String& fileName) const
{
	fileName = this->fileName;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getFileType (FileType& fileType) const
{
	String ext;
	int index = fileName.lastIndex (".");
	if(index >= 0)
	  ext = fileName.subString (index + 1);
	if(ext.isEmpty ())
		return false;

	const FileType* ft = System::GetFileTypeRegistry ().getFileTypeByExtension (ext);
	if(ft)
		fileType = *ft;
	else
	{
		fileType = FileType ();
		fileType.setExtension (ext);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getFileSize (int64& _fileSize) const
{
	if(fileSize < 0) // -1 means unknown
		return false;
	_fileSize = fileSize;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getFileTime (DateTime& _fileTime) const
{
	_fileTime = fileTime;
	return fileTime != DateTime ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileDescriptor::getMetaInfo (IAttributeList& a) const
{
	if(metaInfo)
	{
		a.copyFrom (*metaInfo);
		return true;
	}
	return false;
}

//************************************************************************************************
// SearchDescription
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SearchDescription, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchDescription* SearchDescription::create (UrlRef startPoint, StringRef searchTerms, int options, StringRef delimiter)
{
	String normalizedTerms (searchTerms);
	normalizedTerms.normalize (Text::kNormalizationC);

	if(!delimiter.isEmpty ())
	{
		return NEW TokenizedSearchDescription (startPoint, normalizedTerms, options, delimiter);
	}
	else if(searchTerms.contains (CCLSTR ("*")))
	{
		RegExpSearchDescription* description = NEW RegExpSearchDescription (startPoint, normalizedTerms, options);
		if(description->isValid ())
			return description;
		else
			description->release ();
	}
	
	return NEW SearchDescription (startPoint, normalizedTerms, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchDescription::matchesName (StringRef name) const
{
	String normalizedName (name);
	normalizedName.normalize (Text::kNormalizationC);

	auto matches = [&] (StringRef searchTerms)
	{
		bool caseSensitive = (options & kMatchCase) != 0;

		if(options & kMatchWholeWord)
			return normalizedName.compare (searchTerms, caseSensitive) == Text::kEqual;
		else
			return normalizedName.contains (searchTerms, caseSensitive);
	};

	if(options & kIgnoreDelimiters)
	{
		removeDelimiters (normalizedName);

		String modifiedTerms (searchTerms);
		removeDelimiters (modifiedTerms);

		return matches (modifiedTerms);
	}
	else
		return matches (searchTerms);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchDescription::removeDelimiters (String& string)
{
	string.replace (CCLSTR ("-"), String::kEmpty, false);
}

//************************************************************************************************
// RegExpSearchDescription
//************************************************************************************************

RegExpSearchDescription::RegExpSearchDescription (UrlRef startPoint, StringRef searchTerms, int options)
: SearchDescription (startPoint, searchTerms, options),
  regExp (System::CreateRegularExpression ())
{
	String expression(searchTerms);

	for(int i = 0; i < expression.length (); i++)
	{
		uchar c = expression[i];
		if(c == '*')
		{
			expression.insert (i, ".");
			i++; // skip the inserted .
		}
		else if(c == '[' ||
				c == '\\' ||
				c == '^' ||
				c == '$' ||
				c == '.' ||
				c == '|' ||
				c == '?' ||
				c == '+' ||
				c == '(' ||
				c == ')')
		{
			expression.insert (i, "\\"); // escape the other regex special characters [\^$.|?+()
			i++; // skip the inserted backslash
		}
	}

	CCL_PRINTF ("regular expression search: %s\n", MutableCString (expression).str ())

	int regExpOptions = 0;
	if((options & kMatchCase) == 0)
		regExpOptions |= IRegularExpression::kCaseInsensitive;

	if(regExp->construct (expression, regExpOptions) != kResultOk)
		safe_release (regExp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RegExpSearchDescription::~RegExpSearchDescription ()
{
	safe_release (regExp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RegExpSearchDescription::matchesName (StringRef name) const
{
	return regExp && regExp->isFullMatch (name);
}

//************************************************************************************************
// TokenizedSearchDescription
//************************************************************************************************

TokenizedSearchDescription::TokenizedSearchDescription (UrlRef startPoint, StringRef _searchTerms, int options, StringRef delimiter)
: SearchDescription (startPoint, _searchTerms, options),
  delimiter (delimiter)
{
	static StringRef kGroupingDelimiter = CCLSTR ("\"");
	searchDescriptions.objectCleanup (true);
		
	String tokenSearchString;
	if(options & kAllowTokenGrouping)
	{
		bool delimitedEnd = searchTerms.endsWith (kGroupingDelimiter);
		int index = 0;
		AutoPtr<IStringTokenizer> tokenizer = searchTerms.tokenize (kGroupingDelimiter, Text::kPreserveEmptyToken);
		if(tokenizer)
		{
			uchar delimiter = 0;
			while(!tokenizer->done ())
			{
				StringRef group = tokenizer->nextToken (delimiter);
				CCL_PRINTLN (group)
				bool odd = (index % 2) != 0;
				if(odd && (!tokenizer->done () || (tokenizer->done () && delimitedEnd)))
					searchDescriptions.add (SearchDescription::create (startPoint, group, 0));
				else
					tokenSearchString.append (group);
				index++;
			}
		}
	}
	else
		tokenSearchString = searchTerms;
	
	ForEachStringToken (tokenSearchString, delimiter, token)
		searchDescriptions.add (SearchDescription::create (startPoint, token, 0));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TokenizedSearchDescription::matchesName (StringRef name) const
{
	int matches = 0;
	for(auto searchDescription : iterate_as<SearchDescription> (searchDescriptions))
		if(searchDescription->matchesName (name))
			matches++;
	
	if(options & kMatchAllTokens)
		return searchDescriptions.count () == matches;
	
	return matches > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TokenizedSearchDescription::getSearchTokenCount () const
{
	return searchDescriptions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TokenizedSearchDescription::getSearchToken (int index) const
{
	SearchDescription* searchDescription = ccl_cast<SearchDescription> (searchDescriptions.at (index));
	if(searchDescription)
		return searchDescription->getSearchTerms ();
	
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API TokenizedSearchDescription::getTokenDelimiter () const
{
	return delimiter;
}
