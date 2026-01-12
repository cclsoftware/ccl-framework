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
// Filename    : ccl/extras/tools/toolhelp.cpp
// Description : Tool Helper
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/tools/toolhelp.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/system/packaging/packagefileformat.h" // include private package format header

using namespace CCL;

//************************************************************************************************
// PackFolderOptions
//************************************************************************************************

PackFolderOptions& PackFolderOptions::fromString (StringID string)
{
	useZip = string.contains ("-z") || string.contains ("-p");
	compressed = !string.contains ("-p") && !string.contains ("-e");
	encrypted = string.contains ("-e");
	formatVersion = string.contains ("-v2") ? 2 : 0;
	formatVersion = string.contains ("-v3") ? 3 : 0;
	reservedBlockSize = string.contains ("-r8k") ? 8192 : 0;
	xteaEncrypted = string.contains ("-xtea");
	aesEncrypted = string.contains ("-aes");
	keepHidden = string.contains ("-hidden");

	const CString kKeyAttr ("-key=");
	int keyIndex = string.index (kKeyAttr);
	if(keyIndex != -1)
		keyString = string.subString (keyIndex + kKeyAttr.length (), 32); // 16 Bytes ASCII-encoded

	return *this;
}

//************************************************************************************************
// ToolHelper
//************************************************************************************************

DEFINE_CLASS (ToolHelper, Object)
DEFINE_CLASS_UID (ToolHelper, 0xa440d062, 0x2ef2, 0x4764, 0xa5, 0xfd, 0x38, 0x12, 0x5a, 0x19, 0x97, 0x15)
DEFINE_CLASS_NAMESPACE (ToolHelper, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolHelper::copyFile (UrlRef dstPath, UrlRef srcPath)
{
	return System::GetFileSystem ().copyFile (dstPath, srcPath) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolHelper::copyFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter& filter, bool recursive)
{
	return File::copyFolder (dstPath, srcPath, &filter, recursive);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString ToolHelper::generateKeyString ()
{
	UID uid;
	uid.generate ();
	MutableCString keyString;
	uid.toCString (keyString, UID::kCompact);
	return keyString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolHelper::packageFolder (UrlRef dstPath, UrlRef srcPath, const IUrlFilter& filter, bool recursive, 
								const PackFolderOptions& options, IProgressNotify* progress, UrlRef dependencyFilePath)
{
	AutoPtr<IPackageFile> pf = System::GetPackageHandler ().createPackage (dstPath, options.useZip ? ClassID::ZipFile : ClassID::PackageFile);

	// apply options
	pf->setOption (PackageOption::kCompressed, options.compressed);
	if(options.encrypted)
	{
		if(options.aesEncrypted)
			pf->setOption (PackageOption::kAESEncrypted, true);
		else if(options.xteaEncrypted)
			pf->setOption (PackageOption::kXTEAEncrypted, true);
		else
			pf->setOption (PackageOption::kBasicEncrypted, true);
	}
	if(options.formatVersion != 0)
		pf->setOption (PackageOption::kFormatVersion, options.formatVersion);
	if(options.reservedBlockSize != 0)
		pf->setOption (PackageOption::kReservedBlockSize, options.reservedBlockSize);
	if(!options.keyString.isEmpty ())
		pf->setOption (PackageOption::kExternalEncryptionKey, options.keyString.str ());

	pf->deletePhysical ();
	if(!pf->create ())
		return false;

	int fileIteratorMode = recursive ? IFileIterator::kAll : IFileIterator::kFiles;
	pf->embedd (srcPath, fileIteratorMode, const_cast<IUrlFilter*> (&filter), progress);

	if(!pf->flush (progress))
		return false;

	if(!dependencyFilePath.isEmpty ())
	{
		DependencyFile dependencyFile (dependencyFilePath);
		dependencyFile.setOutputFile (dstPath);
		dependencyFile.addDependency (srcPath);
	}

	pf->close ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ToolHelper::embeddDataInPackageFile (UrlRef dstPath, UrlRef srcPath, StringRef comment)
{
	AutoPtr<IStream> dstStream = System::GetFileSystem ().openStream (dstPath, IStream::kWriteMode|IStream::kReadMode);
	AutoPtr<IMemoryStream> srcStream = File::loadBinaryFile (srcPath);
	if(!dstStream || !srcStream)
		return false;

	// check package signature
	FOURCC fcc = {0};
	Streamer streamer (*dstStream, kLittleEndian);
	streamer.read (fcc);
	if(fcc != kPackageSignature1)
		return false;
	streamer.read (fcc);
	if(fcc != kPackageSignature2)
		return false;

	// check reserved block header
	ReservedBlockHeader header;
	header.deserialize (streamer);
	if(header.signature != kReservedBlockSignature)
		return false;

	// update header
	String fileName;
	srcPath.getName (fileName);
	uint32 srcSize = srcStream->getBytesWritten ();

	header.usedSize = srcSize;
	header.comment = MutableCString (comment, Text::kUTF8);
	header.fileName = MutableCString (fileName, Text::kUTF8);

	int headerSize = header.getHeaderSize ();
	if(srcSize + headerSize > header.totalSize)
		return false;

	dstStream->seek (kReservedBlockOffset, IStream::kSeekSet);
	header.serialize (streamer);

	// write data
	uint32 numWritten = dstStream->write (srcStream->getMemoryAddress (), srcSize);
	return numWritten == srcSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ToolHelper)
	DEFINE_METHOD_ARGR ("generateKeyString", "", "string")
	DEFINE_METHOD_ARGR ("packageFolder", "dstPath: Url, srcPath: Url, options: string = null, progress: Object = null", "bool")
END_METHOD_NAMES (ToolHelper)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ToolHelper::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "generateKeyString")
	{
		String keyString (generateKeyString ());
		returnValue = keyString;
		returnValue.share ();
		return true;
	}
	else if(msg == "packageFolder")
	{
		UnknownPtr<IUrl> dstPath (msg[0].asUnknown ());
		UnknownPtr<IUrl> srcPath (msg[1].asUnknown ());
		if(dstPath && srcPath)
		{
			CCL_PRINTF ("Package folder: src = '%s' dst = '%s'\n",
						MutableCString (UrlDisplayString (*srcPath)).str (),
						MutableCString (UrlDisplayString (*dstPath)).str ())
			
			PackFolderOptions options;
			if(msg.getArgCount () > 2)
				options.fromString (MutableCString (msg[2].asString ()));
			UnknownPtr<IProgressNotify> progress;
			if(msg.getArgCount () > 3)
				progress = msg[3].asUnknown ();

			PackageFilter filter;
			filter.applyOptions (options);

			returnValue = packageFolder (*dstPath, *srcPath, filter, true, options, progress);
		}		
		return true;
	}
	return false;
}

//************************************************************************************************
// CommandLineTool
//************************************************************************************************

CommandLineTool::CommandLineTool ()
: console (System::GetConsole ())
{
	System::GetFileSystem ().getWorkingDirectory (workDir);
	System::GetLogger ().addOutput (&console);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CommandLineTool::~CommandLineTool ()
{
	System::GetLogger ().removeOutput (&console);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandLineTool::configureLogging (int severity, int format)
{
	console.setReportOptions (severity, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandLineTool::makeAbsolute (Url& path)
{
	if(path.isRelative ())
		path.makeAbsolute (workDir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CommandLineTool::makeAbsolute (Url& path, StringRef _fileName, int type, UrlRef baseDir)
{
	String fileName (_fileName);

	if(fileName.contains (CCLSTR ("://")))
	{
		Url url (fileName, type);
		#if 1 // resolve for easier debugging
		if(url.getProtocol () == CCLSTR ("local"))
			System::GetSystem ().resolveLocation (path, url);
		else
		#endif
			path = url;
	}
	else
	{
		if(!fileName.startsWith (Url::strPathChar) && !fileName.contains (CCLSTR (":\\")))
			fileName.prepend ("./");
		path.fromDisplayString (fileName, type);
		if(path.isRelative ())
			path.makeAbsolute (baseDir.isEmpty () ? workDir : baseDir);
	}
}

//************************************************************************************************
// BatchProcessor
//************************************************************************************************

bool BatchProcessor::isCommentLine (StringRef line) const
{
	static const String semicolon = CCLSTR (";");
	return line.isEmpty () || line.startsWith (semicolon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchProcessor::run (StringRef batchFileName, bool changeWorkDir)
{
	Url batchFilePath;
	makeAbsolute (batchFilePath, batchFileName);
	return run (batchFilePath, changeWorkDir);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BatchProcessor::run (UrlRef batchFilePath, bool changeWorkDir)
{
	TextFile batchFile (batchFilePath, TextFile::kOpen);
	if(!batchFile.isValid ())
	{
		console.writeLine ("Batch file not found at:");
		console.writeLine (UrlDisplayString (batchFilePath));
		return false;
	}

	// paths in batch file are relative to the file itself!
	if(changeWorkDir)
	{
		workDir = batchFilePath;
		workDir.ascend ();
	}

	String line;
	while(batchFile->readLine (line))
	{
		line.trimWhitespace ();
		if(isCommentLine (line))
			continue;

		if(!runLine (line))
		{
			console.writeLine ("BatchProcessor failed with line:");
			console.writeLine (line);
			return false;
		}
	}

	return true;
}

//************************************************************************************************
// PackageFilter
//************************************************************************************************

PackageFilter::PackageFilter ()
: compressed (false),
  encrypted (false),
  externalKeyEnabled (false),
  keepHidden (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilter::applyOptions (const PackFolderOptions& options)
{
	setCompressed (options.compressed);
	setEncrypted (options.encrypted);
	setExternalKeyEnabled (!options.keyString.isEmpty ());
	setKeepHidden (options.keepHidden);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageFilter::resetOptions ()
{
	setCompressed (false);
	setEncrypted (false);
	setExternalKeyEnabled (false);
	setKeepHidden (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageFilter::isMetaInfoFile (UrlRef path) const
{
	static const String names[] = // hardcoded for now
	{
		String ("metainfo.xml"),
		String ("installdata.xml"),
		String ("signature.xml"),
		String ("package.iconset")
	};

	String fileName;
	path.getName (fileName);
	for(int i = 0; i < ARRAY_COUNT (names); i++)
		if(fileName == names[i])
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PackageFilter::getPackageItemAttributes (UrlRef path) const
{
	int attributes = IPackageItem::kPlain;

	if(isCompressed ())
		attributes |= IPackageItem::kCompressed;

	if(isEncrypted ())
	{
		if(isMetaInfoFile (path) == false) // don't encrypt meta information
		{
			attributes |= IPackageItem::kEncrypted;
			if(isExternalKeyEnabled ())
				attributes |= IPackageItem::kUseExternalKey;
		}
	}

	if(isKeepHidden ())
	{
		if(System::GetFileSystem ().isHiddenFile (path))
			attributes |= IPackageItem::kHidden;
	}

	return attributes;
}

//************************************************************************************************
// PatternFilter
//************************************************************************************************

PatternFilter::PatternFilter ()
: caseSensitive (false),
  positive (false)
{
	strings.objectCleanup (true);

	setCaseSensitive (System::GetFileSystem ().isCaseSensitive () != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PatternFilter::loadFromFile (UrlRef path)
{
	StringList stringList;
	if(!TextUtils::loadStringList (stringList, path))
		return false;

	ForEach (stringList, Boxed::String, str)
		strings.add (return_shared (str));
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PatternFilter::add (StringRef string)
{
	ForEach (strings, Boxed::String, str)
		if(*str == string)
			return;
	EndFor
	strings.add (NEW Boxed::String (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PatternFilter::remove (StringRef string)
{
	ForEach (strings, Boxed::String, str)
		if(*str == string)
		{
			strings.remove (str);
			str->release ();
			return;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PatternFilter::removeAll ()
{
	strings.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PatternFilter::matches (UrlRef path) const
{
	String name;
	path.getName (name);

	String extension;
	if(path.isFile ())
		path.getExtension (extension);

	static const String wildcard = CCLSTR ("*.");
	static const String bracketLeft = CCLSTR ("<");
	static const String bracketRight = CCLSTR (">");

	ForEach (strings, Boxed::String, str)
		// *.ext
		if(str->startsWith (wildcard))
		{
			if(!extension.isEmpty ())
			{
				String ext = str->subString (2);
				if(extension.compare (ext, caseSensitive) == 0)
					return positive;
			}
		}
		// <folder>
		else if(str->startsWith (bracketLeft) && str->endsWith (bracketRight))
		{
			String folderName = str->subString (1, str->length ()-2);
			if(path.isFolder ())
			{
				if(name.compare (folderName, caseSensitive) == 0)
					return positive;
			}
		}
		else if(path.isFile ())
		{
			StringRef string = *str;

			CCL_PRINT ("Compare ")
			CCL_PRINT (name)
			CCL_PRINT (" == ")
			CCL_PRINTLN (string)

			if(name.compare (string, caseSensitive) == 0)
				return positive;
		}
	EndFor
	return !positive;
}

//************************************************************************************************
// ExtensionFilter
//************************************************************************************************

ExtensionFilter::ExtensionFilter (StringRef extensions)
: extensions (extensions)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ExtensionFilter::matches (UrlRef url) const
{
	String name;
	url.getName (name);

	if(name == CCLSTR (".svn") || 
	   name == CCLSTR (".git") || 
	   name == CCLSTR (".DS_Store") ||
	   name.compare (CCLSTR ("Thumbs.db"), false) == 0)
		return false;

	if(extensions.isEmpty () == false)
	{
		url.getExtension (name);
		if(extensions.index (name) >= 0)
			return false;
	}

	return true;
}

//************************************************************************************************
// DependencyFile
//************************************************************************************************

DependencyFile::DependencyFile (UrlRef path)
: path (path)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DependencyFile::~DependencyFile ()
{
	if(path.isEmpty ())
		return;

	TextFile file (path, Text::kUTF8, Text::kLFLineFormat, ITextStreamer::kSuppressByteOrderMark);
	if(!file.isValid ())
		return;

	file->writeString (getPathString (targets.last ().path).append (": "), false);
	for(int i = targets.count () - 1; i >= 0; i--)
	{
		for(UrlRef inputFile : targets[i].dependencies)
		{
			if(!targets.contains (inputFile))
				file->writeString (getPathString (inputFile).append (" "), false);
		}
	}
	file->writeNewline ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DependencyFile::setOutputFile (UrlRef path)
{
	targets.add ({ path });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DependencyFile::addDependency (UrlRef inputFile)
{
	if(inputFile.isFolder ())
	{
		ForEachFile (File (inputFile).newIterator (IFileIterator::kAll), filePath)
			addDependency (*filePath);
		EndFor
		return;
	}
	targets.last ().dependencies.add (inputFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String DependencyFile::getPathString (UrlRef path)
{
	UrlDisplayString pathString (path);
	pathString.replace (" ", "\\ ");
	return pathString;
}
