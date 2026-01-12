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
// Filename    : ccl/system/fileutilities.cpp
// Description : File Utilities
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/fileutilities.h"
#include "ccl/system/packaging/bufferedstream.h"
#include "ccl/system/packaging/sectionstream.h"
#include "ccl/system/packaging/packagehandler.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ifileitem.h"
#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/stringbuilder.h"

#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// SeekableWriteStream
//************************************************************************************************

class SeekableWriteStream: public Object,
						   public IStream
{
public:
	SeekableWriteStream (IStream& dstStream);
	~SeekableWriteStream ();

	// IStream
	int CCL_API read (void* buffer, int size) override;
	int CCL_API write (const void* buffer, int size) override;
	int64 CCL_API tell () override;
	tbool CCL_API isSeekable () const override;
	int64 CCL_API seek (int64 pos, int mode) override;

	CLASS_INTERFACE (IStream, Object)

protected:
	IStream& dstStream;
	MemoryStream tempStream;
};

//************************************************************************************************
// FileTypeIterator
//************************************************************************************************

class FileTypeIterator: public Unknown,
						public IFileTypeIterator
{
public:
	FileTypeIterator (Iterator& iter);
	~FileTypeIterator ();

	// IFileTypeIterator
	const FileType* CCL_API nextFileType () override;

	CLASS_INTERFACE (IFileTypeIterator, Unknown)

private:
	Iterator& iter;
};

//************************************************************************************************
// SimpleFileHandler
//************************************************************************************************

class SimpleFileHandler: public Object,
						 public IFileHandler
{
public:
	DECLARE_CLASS (SimpleFileHandler, Object)

	PROPERTY_OBJECT (FileType, fileType, FileType)
	PROPERTY_SHARED_AUTO (Url, location, Location)
	PROPERTY_SHARED_AUTO (IObserver, observer, Observer)

	// IFileHandler
	tbool CCL_API openFile (UrlRef path) override;
	State CCL_API getState (IFileDescriptor& descriptor) override;
	tbool CCL_API getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor) override;

	CLASS_INTERFACE (IFileHandler, Object)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Predefined File Types
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (TextFile, "Text File")
	XSTRING (XmlFile, "XML File")
	XSTRING (HtmlFile, "HTML File")
	XSTRING (RtfFile, "Rich Text Format")
	XSTRING (PdfFile, "PDF File")
	XSTRING (PropertiesFile, "Java Properties File")
	XSTRING (BinaryFile, "Binary File")
	XSTRING (AppFile, "Application")
	XSTRING (ModuleFile, "Application Extension")
	XSTRING (ZipFile, "ZIP File")
	XSTRING (PackageFile, "Package File")
	XSTRING (JsonFile, "JSON File")
	XSTRING (UBJsonFile, "UBJSON File")
	XSTRING (CsvFile, "Spreadsheet")
END_XSTRINGS

namespace CCL {
namespace FileTypes
{
	static FileType empty;

	static FileType text	(nullptr, "txt", "text/plain");
	static FileType xml		(nullptr, "xml", "text/xml");
	static FileType html	(nullptr, "html", "text/html");
	static FileType rtf		(nullptr, "rtf", "text/rtf");
	static FileType pdf		(nullptr, "pdf", "application/pdf");
	static FileType props	(nullptr, "properties", "text/plain");

	static FileType binary	(nullptr, "bin", "application/octet-stream");

	#if CCL_PLATFORM_WINDOWS
	static FileType app		(nullptr, "exe", "application/octet-stream");
	static FileType module	(nullptr, "dll", "application/octet-stream");
	#elif CCL_PLATFORM_MAC
	static FileType app		(nullptr, "app", "application/octet-stream");
	static FileType module	(nullptr, "bundle", "application/octet-stream");
	#elif CCL_PLATFORM_IOS
	static FileType app		(nullptr, "app", "application/octet-stream");
	static FileType module	(nullptr, "plugin", "application/octet-stream");
	#elif CCL_PLATFORM_ANDROID
	static FileType app		(nullptr, "apk", "application/octet-stream");
	static FileType module	(nullptr, "plugin", "application/octet-stream");
	#else
	static FileType app		(nullptr, "", "application/octet-stream");
	static FileType module	(nullptr, "so", "application/octet-stream");
	#endif

	static FileType zip 	(nullptr, "zip", "application/zip");
	static FileType package (nullptr, "package", CCL_MIME_TYPE "-package");

	static FileType json	(nullptr, "json", "application/json");
	static FileType ubjson	(nullptr, "ubj", "application/ubjson");

	static FileType csv		(nullptr, "csv", "text/csv");
}}

#if 1
#define REGISTER_DEFAULT_TYPE(name, description) \
	FileTypes::name.setDescription (XSTR (description)); \
	System::GetFileTypeRegistry ().registerFileType (FileTypes::name);
#else
#define REGISTER_DEFAULT_TYPE(name, description) \
	System::GetFileTypeRegistry ().registerFileType (FileTypes::name);
#endif

CCL_KERNEL_INIT_LEVEL (FileTypeRegistry, kFrameworkLevelFirst + 1) // after translations are loaded!
{
	REGISTER_DEFAULT_TYPE (text, TextFile)
	REGISTER_DEFAULT_TYPE (xml, XmlFile)
	REGISTER_DEFAULT_TYPE (html, HtmlFile)
	REGISTER_DEFAULT_TYPE (rtf, RtfFile)
	REGISTER_DEFAULT_TYPE (pdf, PdfFile)
	REGISTER_DEFAULT_TYPE (props, PropertiesFile)
	REGISTER_DEFAULT_TYPE (binary, BinaryFile)
	REGISTER_DEFAULT_TYPE (app, AppFile)
	REGISTER_DEFAULT_TYPE (module, ModuleFile)
	REGISTER_DEFAULT_TYPE (zip, ZipFile)
	REGISTER_DEFAULT_TYPE (package, PackageFile)
	REGISTER_DEFAULT_TYPE (json, JsonFile)
	REGISTER_DEFAULT_TYPE (ubjson, UBJsonFile)
	REGISTER_DEFAULT_TYPE (csv, CsvFile)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IFileUtilities& CCL_API System::CCL_ISOLATED (GetFileUtilities) ()
{
	static FileUtilities theFileUtilities;
	return theFileUtilities;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IFileTypeRegistry& CCL_API System::CCL_ISOLATED (GetFileTypeRegistry) ()
{
	return FileTypeRegistry::instance ();
}

//************************************************************************************************
// FileUtilities
//************************************************************************************************

void CCL_API FileUtilities::makeUniqueFileName (IFileSystem& fileSystem, IUrl& path, tbool forceSuffix)
{
	if(!forceSuffix && !fileSystem.fileExists (path))
		return; // already unique

	String fileName, fileExt;
	if(path.isFile ())
	{
		path.getName (fileName, false);
		path.getExtension (fileExt);
	}
	else // preserve dots in folder name
		path.getName (fileName);

	// remove suffix if it already exists to avoid "filename(1)(1)(1)..."
	if(fileName.lastChar () == ')')
	{
		int index = fileName.lastIndex ("(");
		if(index > 0)
		{
			int64 test = 0;
			if(fileName.subString (index + 1).getIntValue (test))
				fileName.truncate (index);
		}
	}

	int index = forceSuffix ? 1 : 2;

	do
	{
		String newName;
		newName << fileName << "(" << index++ << ")";
		if(!fileExt.isEmpty ())
			newName << "." << fileExt;
		path.setName (newName);
	}
	while(fileSystem.fileExists (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileUtilities::makeValidFileName (String& fileName)
{
	static const char* kInvalidFileNameChars = "?*/\\<>|:\"\t\r\n";

	String newFileName;
	// hmm... we are creating a copy even if the filename is valid :-(
	{
		StringChars chars (fileName);
		StringWriter<512> writer (newFileName);
		int length = fileName.length ();
		for(int i = 0; i < length; i++)
			if((chars[i] < 255 && ::strchr (kInvalidFileNameChars, (char)chars[i])) || chars[i] < 32) // replace all ASCII control characters, we couldn't reference such files in xml (see XmlEntities::encode)
				writer.append ('_');
			else
				writer.append (chars[i]);
		writer.flush ();
	}

	#if CCL_PLATFORM_WINDOWS
	// these string are not allowed as file names.
	// https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file
	static const char* kInvalidFileNames [] = { "CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };

	char tmp [8] = {};
	newFileName.toCString (Text::kUTF8, tmp, ARRAY_COUNT (tmp));
	CString tmp2 (tmp);
	if(tmp2.length () < 5)
	{
		for(int i = 0; i < ARRAY_COUNT (kInvalidFileNames); i++)
		{
			if(tmp2.compare (kInvalidFileNames[i], false) == 0)
			{
				StringWriter<2> writer (newFileName, false);
				writer.append ('_');
				writer.flush ();
				break;
			}
		}
	}
	
	// Windows removes leading/trailing white space when files/directories are created.
	newFileName.trimWhitespace ();
	#endif

	fileName = newFileName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileUtilities::appendDateTime (String& fileName)
{
	DateTime time;
	System::GetSystem ().getLocalTime (time);

	MutableCString temp;
	temp.appendFormat (" %.2d%.2d%.2d-%.2d%.2d%.2d",
		time.getDate ().getYear (), time.getDate ().getMonth (), time.getDate ().getDay (),
		time.getTime ().getHour (), time.getTime ().getMinute (), time.getTime ().getSecond ());
	fileName.appendCString (Text::kASCII, temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileUtilities::scanDateTime (DateTime& time, StringRef fileName, String* prefix, String* suffix)
{
	MutableCString string (fileName);

	if(const char* str = string)
	{
		int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
		int dateTimeStart = 0;

		while(*str)
		{
			if(::sscanf (str, " %4d%2d%2d-%2d%2d%2d", &year, &month, &day, &hour, &minute, &second) == 6)
			{
				time.setDate (Date (year, month, day));
				time.setTime (Time (hour, minute, second));

				if(prefix)
				{
					*prefix = fileName.subString (0, dateTimeStart);
					prefix->trimWhitespace ();
				}

				if(suffix)
				{
					MutableCString temp;
					temp.appendFormat (" %.2d%.2d%.2d-%.2d%.2d%.2d", year, month, day, hour, minute, second);

					*suffix = fileName.subString (dateTimeStart + temp.length ());
					suffix->trimWhitespace ();
				}
				return true;
			}
			str++;
			dateTimeStart++;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API FileUtilities::makeUniqueTempFolder (IUrl& tempFolder)
{
	System::GetSystem ().getLocation (tempFolder, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFolder.descend (folderName, IUrl::kFolder);
	return tempFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API FileUtilities::makeUniqueTempFile (IUrl& tempFile, StringRef name)
{
	String fileName (name);
	String subFolder;

	// allow relative subfolder in name
	int separatorIndex = fileName.lastIndex ("/");
	if(separatorIndex != -1)
	{
		subFolder = fileName.subString (0, separatorIndex);
		fileName = fileName.subString (separatorIndex+1);
	}

	System::GetSystem ().getLocation (tempFile, System::kTempFolder);

	if(fileName.isEmpty ())
		fileName = CCLSTR ("~temp");
	else
		makeValidFileName (fileName);

	if(!subFolder.isEmpty ())
	{
		makeValidFileName (subFolder);
		tempFile.descend (subFolder, Url::kFolder);
	}

	#if 1 // append timestamp (speedup for makeUniqueFileName())
	String ext;
	int extIndex = fileName.lastIndex (".");
	if(extIndex != -1)
	{
		ext = fileName.subString (extIndex+1);
		fileName.truncate (extIndex);
	}

	fileName << "_" << System::GetSystemTicks ();

	if(!ext.isEmpty ())
		fileName = String () << fileName << "." << ext;
	#endif

	tempFile.descend (fileName);
	makeUniqueFileName (System::GetFileSystem (), tempFile, false);
	return tempFile;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileUtilities::copyStream (IStream& destStream, IStream& srcStream, IProgressNotify* progress, int64 maxBytesToCopy)
{
	static const unsigned int kCopyBufferSize = 8192;
	char buffer[kCopyBufferSize];

	if(progress)
		progress->beginProgress ();

	// determine copy limit if stream is seekable...
	if(maxBytesToCopy < 0 && srcStream.isSeekable ())
	{
		int64 oldPos = srcStream.tell ();
		maxBytesToCopy = srcStream.seek (0, IStream::kSeekEnd) - oldPos;
		srcStream.seek (oldPos, IStream::kSeekSet);
	}

	bool result = true;
	int64 numBytesCopied = 0;

	CCL_PRINTF ("Stream %x copy begin \n", (int)(int64)&destStream, 0)

	while(1)
	{
		int numBytesToRead = kCopyBufferSize;

		// check copy limit...
		if(maxBytesToCopy >= 0 && numBytesCopied + numBytesToRead > maxBytesToCopy)
		{
			numBytesToRead = (int)(maxBytesToCopy - numBytesCopied);
			ASSERT (numBytesToRead >= 0)
			if(numBytesToRead <= 0)
				break;
		}

		int numBytesRead = srcStream.read (buffer, numBytesToRead);
		if(numBytesRead <= 0)
		{
			if(numBytesRead < 0) // a reading error occurred!
				result = false;
			break;
		}

		int numBytesWritten = destStream.write (buffer, numBytesRead);
		if(numBytesWritten != numBytesRead)
		{
			result = false; // a writing error occurred!
			break;
		}

		CCL_PRINTF ("Stream %x copy rd %d wr %d\n", (int)(int64)&destStream, numBytesRead, numBytesWritten)

		numBytesCopied += numBytesWritten;

		if(progress)
		{
			if(progress->isCanceled ())
			{
				result = false;
				break;
			}

			double normProgress = 0.;
			int flags = 0;
			if(maxBytesToCopy > 0)
				normProgress = (double)numBytesCopied / (double)maxBytesToCopy;
			else
				flags = IProgressNotify::kIndeterminate;

			progress->updateProgress (IProgressNotify::State (normProgress, flags));
		}
	}

	CCL_PRINTF ("Stream %x copy finished %d\n", (int)(int64)&destStream, numBytesCopied)

	if(progress)
		progress->endProgress ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API FileUtilities::createSectionStream (IStream& inStream, int64 offset, int64 size, tbool writeMode)
{
	return NEW SectionStream (&inStream, offset, size, writeMode ? IStream::kWriteMode : IStream::kReadMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API FileUtilities::createSeekableStream (IStream& inStream, tbool writeMode)
{
	if(!inStream.isSeekable ())
	{
		if(writeMode)
			return NEW SeekableWriteStream (inStream);
		else
			return createStreamCopyInMemory (inStream);
	}

	inStream.retain ();
	return &inStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API FileUtilities::createBufferedStream (IStream& inStream, int bufferSize)
{
	if(bufferSize == -1)
		bufferSize = 65536;
	return NEW BufferedStream (&inStream, bufferSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMemoryStream* CCL_API FileUtilities::createStreamCopyInMemory (IStream& inStream, IMemoryStream* destStream)
{
	// try to determine size to avoid reallocations
	int64 size = -1;
	if(inStream.isSeekable ())
	{
		size = inStream.seek (0, IStream::kSeekEnd);
		inStream.seek (0, IStream::kSeekSet);
	}
	else
	{
		// second try via package item
		if(UnknownPtr<IPackageItem> item = &inStream)
			size = item->getSizeOnDisk ();
	}

	AutoPtr<IMemoryStream> outStream;
	if(destStream)
		outStream.share (destStream);
	else
		outStream = NEW MemoryStream;
	if(size != -1)
	{
		if(outStream->allocateMemoryForStream ((uint32)size))
		{
			int numRead = 0;
			if(void* dst = outStream->getMemoryAddress ())
				numRead = inStream.read (dst, (int)size);
			if(numRead >= 0)
			{
				outStream->setBytesWritten (numRead);
				return outStream.detach ();
			}
		}
	}
	else
	{
		if(copyStream (*outStream, inStream))
		{
			outStream->rewind ();
			return outStream.detach ();
		}
	}

	CCL_DEBUGGER ("Failed to create copy of stream in memory!!!\n")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API FileUtilities::createStringStream (StringRef string, TextEncoding encoding, int flags)
{
	// TODO:
	// - byte-order swapping for kUTF16LE/kUTF16BE?
	// - move code to IString::toStream?

	int stringLength = string.length ();
	int encodingFactor = Text::getMaxEncodingBytesPerCharacter (encoding);
	int bomLength = 0;
	if(!(flags & kSuppressByteOrderMark))
		bomLength = encoding == Text::kUTF8 ? 3 : Text::isUTF16Encoding (encoding) ? 2 : 0;

	int bytesAllocated = bomLength + (stringLength + 1) * encodingFactor; // including null

	MemoryStream* outStream = NEW MemoryStream;
	if(!outStream->allocateMemory (bytesAllocated))
	{
		outStream->release ();
		return nullptr;
	}

	int bytesWritten = 0;
	if(Text::isUTF16Encoding (encoding))
	{
		uchar* dst = (uchar*)outStream->getMemoryAddress ();
		if(!(flags & kSuppressByteOrderMark))
		{
			static const uchar kBomUTF16 = 0xFEFF;
			*dst++ = kBomUTF16;
			bytesWritten += 2;
		}

		string.copyTo (dst, (bytesAllocated - bytesWritten)/sizeof(uchar));
		bytesWritten += stringLength * sizeof(uchar);
	}
	else
	{
		char* dst = (char*)outStream->getMemoryAddress ();
		if(encoding == Text::kUTF8)
		{
			if(!(flags & kSuppressByteOrderMark))
			{
				static const uint8 kBomUTF8[] = {0xEF, 0xBB, 0xBF};
				::memcpy (dst, kBomUTF8, 3);
				dst += 3;
				bytesWritten += 3;
			}
		}

		int dataBytes = 0;
		string.toCString (encoding, dst, bytesAllocated - bytesWritten, &dataBytes);
		bytesWritten += dataBytes;
	}

	outStream->setBytesWritten (bytesWritten); // without null
	return outStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl* CCL_API FileUtilities::translatePathInMountedFolder (UrlRef path)
{
	if(path.isNativePath ())
	{
		Url folderPath (path);
		folderPath.ascend ();

		// check if path is inside a mounted folder
		while(!folderPath.isRootPath ())
		{
			if(PackageHandler::instance ().isMounted (folderPath))
			{
				// translate to path inside package
				PackageInfo info;
				if(info.loadFromPackage (folderPath))
				{
					PackageUrl contentRoot (info.getPackageID (), String::kEmpty, Url::kFolder);

					Url* contentPath = NEW Url (path);
					contentPath->makeRelative (folderPath);
					contentPath->makeAbsolute (contentRoot);
					return contentPath;
				}
			}

			// try parent folders
			folderPath.ascend ();
		}
	}
	return nullptr;
}

//************************************************************************************************
// FileTypeRegistry::FileTypeItem
//************************************************************************************************

class FileTypeRegistry::FileTypeItem: public CCL::Object
{
public:
	FileTypeItem (const FileType& fileType)
	: fileType (fileType),
	  extension (fileType.getExtension ())
	{}

	PROPERTY_OBJECT (FileType, fileType, FileType)
	PROPERTY_MUTABLE_CSTRING (extension, Extension) // CString for faster lookup
};

//************************************************************************************************
// FileTypeIterator
//************************************************************************************************

FileTypeIterator::FileTypeIterator (Iterator& iter)
: iter (iter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeIterator::~FileTypeIterator ()
{
	iter.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API FileTypeIterator::nextFileType ()
{
	auto item = static_cast<FileTypeRegistry::FileTypeItem*> (iter.next ());
	return item ? &item->getFileType () : nullptr;
}

//************************************************************************************************
// SimpleFileHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimpleFileHandler, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimpleFileHandler::openFile (UrlRef path)
{
	if(observer)
		if(path.getFileType () == getFileType ())
		{
			AutoPtr<Url> path2 (NEW Url (path)); // might be kept by script world!
			observer->notify (nullptr, Message (kOpenFile, path2->asUnknown ()));
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileHandler::State CCL_API SimpleFileHandler::getState (IFileDescriptor& descriptor)
{
	FileType fileType;
	descriptor.getFileType (fileType);
	if(fileType == getFileType ())
	{
		// check if file already exists
		if(location)
		{
			String fileName;
			descriptor.getFileName (fileName);
			Url path (*location);
			path.descend (fileName);
			if(System::GetFileSystem ().fileExists (path))
				return kCanUpdate;
		}

		return kCanInstall;
	}
	return kNotHandled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SimpleFileHandler::getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor)
{
	if(location)
	{
		FileType fileType;
		descriptor.getFileType (fileType);
		if(fileType == getFileType ())
		{
			dst.assign (*location);
			return true;
		}
	}
	return false;
}

//************************************************************************************************
// FileTypeRegistry
//************************************************************************************************

FileTypeRegistry& FileTypeRegistry::instance ()
{
	static FileTypeRegistry theFileTypeRegistry;
	return theFileTypeRegistry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (FileTypeRegistry, Object)
DEFINE_CLASS_NAMESPACE (FileTypeRegistry, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeRegistry::FileTypeRegistry ()
{
	fileTypes.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeRegistry::~FileTypeRegistry ()
{
	if(handlers)
	{
		SOFT_ASSERT (handlers.isEmpty (), "File handlers still exist!\n")

		// We assume any handler still existing at this stage to be our own
		VectorForEach (handlers, IFileHandler*, handler)
			ASSERT (unknown_cast<SimpleFileHandler> (handler) != nullptr)
			if(handler)
				handler->release ();
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeRegistry::getFileTypeCategory (String& title, const FileType& fileType) const
{
	return fileTypeClassifier ? fileTypeClassifier->getFileTypeCategory (title, fileType) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileTypeRegistry::setFileTypeClassifier (IFileTypeClassifier* classifier)
{
	fileTypeClassifier = classifier;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API FileTypeRegistry::getDefaultFileType (int which) const
{
	switch(which)
	{
	case FileTypes::kEmpty  : return FileTypes::empty;
	case FileTypes::kText   : return FileTypes::text;
	case FileTypes::kXml    : return FileTypes::xml;
	case FileTypes::kHtml   : return FileTypes::html;
	case FileTypes::kRtf    : return FileTypes::rtf;
	case FileTypes::kPdf    : return FileTypes::pdf;
	case FileTypes::kProperties : return FileTypes::props;
	case FileTypes::kBinary : return FileTypes::binary;
	case FileTypes::kApp    : return FileTypes::app;
	case FileTypes::kModule : return FileTypes::module;
	case FileTypes::kZip    : return FileTypes::zip;
	case FileTypes::kPackage : return FileTypes::package;
	case FileTypes::kJson	: return FileTypes::json;
	case FileTypes::kUBJson	: return FileTypes::ubjson;
	case FileTypes::kCsv    : return FileTypes::csv;
	}

	CCL_DEBUGGER ("Unknown default file type!")
	return FileTypes::empty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeRegistry::FileTypeItem* FileTypeRegistry::findRegisteredType (const FileType& fileType) const
{
	return static_cast<FileTypeItem*> (fileTypes.findIf ([&] (const Object* obj)
	{
		return static_cast<const FileTypeItem*> (obj)->getFileType () == fileType;
	}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileTypeRegistry::registerFileType (const FileType& fileType)
{
	FileTypeItem* registeredType = findRegisteredType (fileType);
	if(registeredType)
	{
		CCL_PRINTF ("fileType already registered: %s\n", MutableCString (registeredType->getFileType ().getDescription ()).str ());
		return kResultAlreadyExists;
	}

	registeredType = NEW FileTypeItem (fileType);
	fileTypes.add (registeredType);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileTypeRegistry::unregisterFileType (const FileType& fileType)
{
	FileTypeItem* registeredType = findRegisteredType (fileType);
	if(!registeredType)
		return kResultFailed;

	fileTypes.remove (registeredType);
	registeredType->release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileTypeRegistry::updateFileType (const FileType& fileType)
{
	FileTypeItem* registeredType = findRegisteredType (fileType);
	ASSERT (registeredType != nullptr)
	if(registeredType == nullptr)
		return kResultFailed;

	ccl_const_cast (registeredType->getFileType ()).setDescription (fileType.getDescription ());
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API FileTypeRegistry::getFileTypeByUrl (UrlRef path) const
{
	String ext;
	if(path.getExtension (ext))
		return getFileTypeByExtension (ext);

	// in cases where it is not part of the URL, the display name contains the actual file name and extension
	ext = UrlUtils::getExtensionFromParameters (path);
	if(!ext.isEmpty ())
		return getFileTypeByExtension (ext);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API FileTypeRegistry::getFileTypeByExtension (StringRef extension) const
{
	if(extension.isEmpty ())
		return nullptr;

	MutableCString ext (extension); // compare CString

	ArrayForEachFast (fileTypes, FileTypeItem, item)
		if(item->getExtension () == ext)
			return &item->getFileType ();
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType* CCL_API FileTypeRegistry::getFileTypeByMimeType (StringRef mimeType) const
{
	if(mimeType.isEmpty ())
		return nullptr;

	ArrayForEachFast (fileTypes, FileTypeItem, item)
		if(item->getFileType ().getMimeType () == mimeType)
			return &item->getFileType ();
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileTypeIterator* CCL_API FileTypeRegistry::newIterator () const
{
	Iterator* iter = fileTypes.newIterator ();
	return iter ? NEW FileTypeIterator (*iter) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileTypeRegistry::registerHandler (IFileHandler* handler)
{
	ASSERT (handler && !handlers.contains (handler))
	handlers.add (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API FileTypeRegistry::unregisterHandler (IFileHandler* handler)
{
	ASSERT (handler && handlers.contains (handler))
	handlers.remove (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileHandler& CCL_API FileTypeRegistry::getHandlers ()
{
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API FileTypeRegistry::newHandlerIterator () const
{
	return NEW HandlerIterator (handlers);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeRegistry::openFile (UrlRef path)
{
	VectorForEach (handlers, IFileHandler*, handler)
		if(handler->openFile (path))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileHandler::State CCL_API FileTypeRegistry::getState (IFileDescriptor& descriptor)
{
	VectorForEach (handlers, IFileHandler*, handler)
		State state = handler->getState (descriptor);
		if(state != kNotHandled)
			return state;
	EndFor
	return kNotHandled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeRegistry::getDefaultLocation (IUrl& dst, IFileDescriptor& descriptor)
{
	VectorForEach (handlers, IFileHandler*, handler)
		if(handler->getDefaultLocation (dst, descriptor))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (FileTypeRegistry)
	DEFINE_METHOD_ARGR ("registerFileType", "fileType: Object | string", "tresult")
	DEFINE_METHOD_ARGR ("getFileTypeByExtension", "extension: string", "Object")
	DEFINE_METHOD_ARGR ("getFileTypeByMimeType", "mimetype: string", "Object")
	DEFINE_METHOD_ARGR ("registerHandler", "fileType: Object | string, location: Url, observer: Object", "tresult")
	DEFINE_METHOD_ARGR ("unregisterHandler", "fileType: Object | string", "tresult")
END_METHOD_NAMES (FileTypeRegistry)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeRegistry::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "registerFileType")
	{
		Boxed::FileType fileType;
		fileType.fromVariant (msg[0]);
		returnValue = registerFileType (fileType);
		return true;
	}
	else if(msg == "getFileTypeByExtension" || msg == "getFileTypeByMimeType")
	{
		const auto* fileType = msg == "getFileTypeByExtension" ? 
									   getFileTypeByExtension (msg[0]) : 
									   getFileTypeByMimeType (msg[0]);
		if(fileType)
		{
			AutoPtr<Boxed::FileType> boxedType = NEW Boxed::FileType (*fileType);
			returnValue.takeShared (boxedType->asUnknown ());
		}
		return true;
	}
	else if(msg == "registerHandler")
	{
		Boxed::FileType fileType;
		fileType.fromVariant (msg[0]);

		AutoPtr<Url> location; // copy to ensure it's in our module address space
		if(UnknownPtr<IUrl> url = msg[1].asUnknown ())
			location = NEW Url (*url);

		UnknownPtr<IObserver> observer (msg[2].asUnknown ());

		SimpleFileHandler* handler = NEW SimpleFileHandler;
		handler->setFileType (fileType);
		handler->setLocation (location);
		handler->setObserver (observer);
		returnValue = registerHandler (handler);
		return true;
	}
	else if(msg == "unregisterHandler")
	{
		Boxed::FileType fileType;
		fileType.fromVariant (msg[0]);

		tresult tr = kResultFailed;
		VectorForEach (handlers, IFileHandler*, h)
			if(SimpleFileHandler* handler = unknown_cast<SimpleFileHandler> (h))
				if(handler->getFileType () == fileType)
				{
					tr = unregisterHandler (handler);
					handler->release ();
					break;
				}
		EndFor

		ASSERT (tr == kResultOk)
		returnValue = tr;
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// SeekableWriteStream
//************************************************************************************************

SeekableWriteStream::SeekableWriteStream (IStream& dstStream)
: dstStream (dstStream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SeekableWriteStream::~SeekableWriteStream ()
{
	tempStream.writeTo (dstStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SeekableWriteStream::read (void* buffer, int size)
{
	CCL_NOT_IMPL ("SeekableWriteStream::read not possible!")
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SeekableWriteStream::write (const void* buffer, int size)
{
	return tempStream.write (buffer, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SeekableWriteStream::tell ()
{
	return tempStream.tell ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SeekableWriteStream::isSeekable () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API SeekableWriteStream::seek (int64 pos, int mode)
{
	return tempStream.seek (pos, mode);
}
