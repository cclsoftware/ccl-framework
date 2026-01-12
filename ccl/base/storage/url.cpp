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
// Filename    : ccl/base/storage/url.cpp
// Description : Url class
//
//************************************************************************************************

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/urlencoder.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/text/language.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/ifilesystemsecuritystore.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/ifilemanager.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

#define POSIX_STRING_FORMAT Text::kUTF8

const String Url::strPathChar = CCLSTR ("/");
const String Url::strBackslash = CCLSTR ("\\");

static const String strThisFolder = CCLSTR (".");
static const String strParentFolder = CCLSTR ("..");
static const String strThisFolderPrefix = CCLSTR ("./");
static const String strParentFolderPrefix = CCLSTR ("../");
static const String strProtocolSeparator = CCLSTR ("://");
static const String strExtensionSeparator = CCLSTR (".");
static const String strFileProtocol = CCLSTR ("file");
static const String strLocalHost = CCLSTR ("localhost");
static const String strQuestionMark = CCLSTR ("?");
static const String strDoubleBackslash = CCLSTR ("\\\\");
static const String strUncPrefix1 = CCLSTR ("\\\\?\\");
static const String strUncPrefix2 = CCLSTR ("\\\\.\\");

//************************************************************************************************
// Url::Comparer
/** URL comparison is somewhat tricky, because different strings can identify the same resource. */
//************************************************************************************************

class Url::Comparer
{
public:
	const Url& a;
	bool caseSensitive;

	Comparer (const Url& a)
	: a (a),
	  caseSensitive (a.isCaseSensitive ())
	{}

	INLINE bool equalsString (StringRef s1, StringRef s2)
	{
		return s1.compare (s2, caseSensitive) == Text::kEqual;
	}

	INLINE StringRef sanitizeHost (StringRef protocol, StringRef hostname)
	{
		// ignore localhost for file protocol
		if(protocol == strFileProtocol)
			if(equalsString (hostname, strLocalHost))
				return String::kEmpty;
		return hostname;
	}

	INLINE String sanitizePath (StringRef _path)
	{
		// ignore if there's a slash too much
		String path (_path);
		if(path.startsWith (Url::strPathChar))
			path.remove (0, 1);
		return path;
	}

	INLINE bool equals (UrlRef b, bool withParameters)
	{
		if(a.type != b.getType ())
			return false;

		// protocol (always case-sensitive)
		if(a.protocol != b.getProtocol ())
			return false;

		// hostname
		if(!equalsString (sanitizeHost (a.protocol, a.hostname),
						  sanitizeHost (a.protocol, b.getHostName ())))
		  return false;

		// path
		if(!equalsString (sanitizePath (a.path),
						  sanitizePath (b.getPath ())))
		  return false;

		// parameters (always case-sensitive)
		if(withParameters)
		{
			IStringDictionary& params1 = a.getParameters ();
			IStringDictionary& params2 = b.getParameters ();
			if(params1.countEntries () != params2.countEntries ())
				return false;

			int count = params1.countEntries ();
			for(int i = 0; i < count; i++)
			{
				if(params1.getKeyAt (i) != params2.getKeyAt (i))
					return false;
				if(params1.getValueAt (i) != params2.getValueAt (i))
					return false;
			}
		}

		return true;
	}
};

//************************************************************************************************
// LegalFileName
//************************************************************************************************

LegalFileName::LegalFileName (StringRef fileName)
: String (fileName)
{
	System::GetFileUtilities ().makeValidFileName (*this);
}

//************************************************************************************************
// LegalFolderName
//************************************************************************************************

LegalFolderName::LegalFolderName (StringRef fileName)
: LegalFileName (fileName)
{
	while(lastChar () == '.')
		truncate (length () - 1);
}

//************************************************************************************************
// MemoryUrl
//************************************************************************************************

const String MemoryUrl::Protocol ("memory");

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* MemoryUrl::newBin ()
{
	return NEW MemoryUrl (UIDString::generate (), nullptr, kFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MemoryUrl::MemoryUrl (StringRef binName, StringRef path, int type)
{
	setProtocol (Protocol);
	setHostName (binName);
	setPath (path, type);
}

//************************************************************************************************
// PackageUrl
//************************************************************************************************

const String PackageUrl::Protocol ("package");

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageUrl::PackageUrl (StringRef packageId, StringRef path, int type)
{
	setProtocol (Protocol);
	setHostName (packageId);
	setPath (path, type);
}

//************************************************************************************************
// ResourceUrl
//************************************************************************************************

const String ResourceUrl::Protocol ("resource");

//////////////////////////////////////////////////////////////////////////////////////////////////

ResourceUrl::ResourceUrl (StringRef path, int type)
{
	ASSERT (!path.contains (strProtocolSeparator))
	setProtocol (Protocol);
	String moduleId;
	System::GetModuleIdentifier (moduleId, System::GetCurrentModuleRef ());
	setHostName (moduleId);
	setPath (path, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ResourceUrl::ResourceUrl (ModuleRef module, StringRef path, int type)
{
	ASSERT (!path.contains (strProtocolSeparator))
	setProtocol (Protocol);
	String moduleId;
	System::GetModuleIdentifier (moduleId, module);
	setHostName (moduleId);
	setPath (path, type);
}

//************************************************************************************************
// LocalizedUrl
//************************************************************************************************

bool LocalizedUrl::localize (Url& url, StringRef resourceName)
{
	StringID language = System::GetLocaleManager ().getLanguage ();
	if(language != LanguageCode::English)
	{
		// 1) check if "filename-xx" exists next to original file
		Url langUrl (url);
		String fileName;
		langUrl.getName (fileName, false);
		fileName << "-" << language;
		FileType fileType (langUrl.getFileType ());
		langUrl.setName (fileName);
		langUrl.setFileType (fileType, true);
		if(System::GetFileSystem ().fileExists (langUrl))
		{
			url.assign (langUrl);
			return true;
		}

		// 2) try to redirect to active language pack
		if(!resourceName.isEmpty ())
			if(const ILanguagePack* languagePack = System::GetLocaleManager ().getActiveLanguagePack ())
				if(languagePack->getResourceLocation (langUrl, resourceName))
				{
					url.assign (langUrl);
					return true;
				}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalizedUrl::LocalizedUrl (UrlRef url, StringRef resourceName)
: Url (url)
{
	localize (*this, resourceName);
}

//************************************************************************************************
// UrlWithTitle
//************************************************************************************************

DEFINE_CLASS (UrlWithTitle, Url)
DEFINE_CLASS_NAMESPACE (UrlWithTitle, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlWithTitle::UrlWithTitle (UrlRef url, StringRef title)
: Url (url),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int UrlWithTitle::compare (const Object& obj) const
{
	String str1, str2;
	toString (str1);
	obj.toString (str2);
	return str1.compareWithOptions (str2, Text::kIgnoreCase|Text::kCompareNumerically);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UrlWithTitle::toString (String& string, int flags) const
{
	if(!title.isEmpty ())
		string = title;
	else
		toDisplayString (string, kStringDisplayPath);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UrlWithTitle::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	title = a.getString ("title");
	return SuperClass::load (storage);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UrlWithTitle::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("title", title);
	return SuperClass::save (storage);
}

//************************************************************************************************
// Url
//************************************************************************************************

const Url Url::kEmpty;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::isUrlString (StringRef string)
{
	return string.contains (strProtocolSeparator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::isRelativePathString (StringRef urlString)
{
	return urlString.startsWith (strThisFolderPrefix) || urlString.startsWith (strParentFolderPrefix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Url, Object)
DEFINE_CLASS_NAMESPACE (Url, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Url::Url (StringRef protocol, StringRef hostname, StringRef path, int type)
: protocol (protocol),
  hostname (hostname),
  path (path),
  type (type),
  parameters (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url::~Url ()
{
	if(parameters)
		parameters->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::isCaseSensitive () const
{
	bool nativePathsAreCaseSensitive = System::GetFileSystem ().isCaseSensitive () != 0;
	bool caseSensitive = nativePathsAreCaseSensitive || protocol != strFileProtocol;
	return caseSensitive;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::equals (const Object& obj) const
{
	const Url* p = ccl_cast<Url> (&obj);
	if(p)
		return isEqualUrl (*p) ? true : false;
	return Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	int type = a.getInt ("type"); // 0=kDetect, thus if not set, the type will be detected

	String url;
	a.get (url, "url");
	setUrl (url, type);

	if(hasParameters ())
	{
		// Migrate legacy security attribute (CCL 4.2 and earlier)
		static StringRef kSecurityAccessDataKey = CCLSTR ("SecurityAccessData");
		StringRef base64 = getParameters ().lookupValue (kSecurityAccessDataKey);
		if(!base64.isEmpty ())
		{
			System::GetFileSystemSecurityStore ().setSecurityData (*this, base64); 
			getParameters ().removeEntry (kSecurityAccessDataKey);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("type", type);

	String url;
	getUrl (url, true); // save with parameters
	a.set ("url", url);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::clone (IUrl*& url) const
{
	url = NEW Url (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::assign (UrlRef url)
{
	type = url.getType ();
	protocol = url.getProtocol ();
	hostname = url.getHostName ();
	path = url.getPath ();
	fileType = url.getFileType ();
	getParameters ().copyFrom (url.getParameters ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isEqualUrl (UrlRef url, tbool withParameters) const
{
	return Comparer (*this).equals (url, withParameters != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isEmpty () const
{
	return protocol.isEmpty () && hostname.isEmpty () && path.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Url::getType () const
{
	return type;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Url::updateType (int _type)
{
	switch(_type)
	{
	case kDetect:
		if(path.endsWith (strPathChar))
		{
			// remove trailing delimiter
			int length = path.length ();
			if(length > 1)
				path.truncate (length - 1);
			type = kFolder;
		}
		else if(path.isEmpty ())
			type = kFolder;
		else
			type = kFile;
		break;

	case kFile:
	case kFolder:
		type = _type;
		// through:
	case kIgnore:
		{
			// remove trailing delimiter
			int length = path.length ();
			if(length > 1 && path.endsWith (strPathChar))
				path.truncate (length - 1);
		}
		break;

	#if DEBUG
	default:
		ASSERT (0) // invalid url type
	#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::getUrl (String& url, tbool withParameters) const
{
	url.empty ();
	url << protocol << strProtocolSeparator << hostname << strPathChar << path;

	// mark folders with "/" at the end
	if(isFolder () && !url.endsWith (strPathChar))
		url.append (strPathChar);

	if(withParameters && hasParameters ())
	{
		String params = UrlEncoder ().encode (getParameters ());
		if(!params.isEmpty ())
			url << strQuestionMark << params;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setUrl (StringRef _url, int _type)
{
	String url (_url);
	if(url.isEmpty ())
	{
		protocol.empty ();
		hostname.empty ();
		path.empty ();
		updateType (_type);
	}
	else
	{
		int index = url.index (strQuestionMark);
		if(index != -1)
		{
			String params = url.subString (index + 1);
			UrlEncoder ().decode (getParameters (), params);
			url.truncate (index);
		}

		index = url.index (strProtocolSeparator);
		if(index != -1)
		{
			protocol = url.subString (0, index);
			url.remove (0, index + 3);
		}
		else
			protocol.empty ();

		index = url.index (strPathChar);
		if(index != -1)
		{
			if(index > 0)
				hostname = url.subString (0, index);
			else
				hostname.empty (); // was a ":///"

			path = url.subString (index + 1);
		}
		else
		{
 			hostname = url;
			path.empty ();
		}
	}

	fileType.clear ();
	updateType (_type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setProtocol (StringRef _protocol)
{
	protocol = _protocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Url::getProtocol () const
{
	return protocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Url::getHostName () const
{
	return hostname;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setHostName (StringRef name)
{
	hostname = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Url::getPath () const
{
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setPath (StringRef _path, int _type)
{
	path.empty ();
	descend (_path, _type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::getPathName (String& pathName) const
{
	int index = path.lastIndex (strPathChar);
	if(index == -1)
		pathName.empty ();
	else
		pathName = path.subString (0, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::getName (String& name, tbool withExtension) const
{
	int index = path.lastIndex (strPathChar);
	if(index == -1)
		name = path;
		// strict would be: name.empty ();
	else
		name = path.subString (index + 1);

	if(!withExtension)
	{
		index = name.lastIndex (strExtensionSeparator);
		if(index != -1)
			name.truncate (index);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setName (StringRef _name, int _type)
{
	int index = path.lastIndex (strPathChar);
	if(index != -1)
		path.truncate (index);
	else
		path.empty (); // path contains the file name only

	descend (_name, _type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::getExtension (String& ext) const
{
	// there might be a dot in the path name (e.g. "folder.1")!!!
	String name;
	getName (name);

	int index = name.lastIndex (strExtensionSeparator);
	if(index == -1)
		ext.empty ();
	else
		ext = name.subString (index + 1);

	ext.toLowercase (); // what if file system is case-sensitive??
	return !ext.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setExtension (StringRef ext, tbool replace)
{
	if(replace)
	{
		int index = path.lastIndex (strExtensionSeparator);
		if(index != -1)
		{
			// there may be a dot in the path name (e.g. "folder.1")!!!
			int slashIndex = path.lastIndex (strPathChar);
			if(slashIndex == -1 || index > slashIndex)
				path.truncate (index);
		}
	}
	else
	{
		// don't append the existing extension again
		String testExt (strExtensionSeparator);
		testExt.append (ext);
		if(path.endsWith (testExt, false))
			return;
	}

	if(!ext.isEmpty ())
	{
		path.append (strExtensionSeparator);
		path.append (ext);
	}

	fileType.clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API Url::getFileType () const
{
	if(isFolder () || fileType.isValid ())
		return fileType;

	const FileType* knownType = System::GetFileTypeRegistry ().getFileTypeByUrl (*this);
	if(knownType)
		fileType = *knownType;
	else
	{
		String ext;
		if(!getExtension (ext))
			ext = UrlUtils::getExtensionFromParameters (*this); // second attempt from URL parameters

		fileType.setExtension (ext);
	}

	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setFileType (const FileType& type, tbool replaceExtension)
{
	setExtension (type.getExtension (), replaceExtension);
	fileType = type; // must set *after* setExtension!!

	this->type = kFile;  // folders don't have filetypes
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isRootPath () const
{
	if(path.isEmpty ())
		return true;

	int length = path.length ();

	// "/" definitely is the root path
	if(length == 1)
		return path.at (0) == '/';

	// Windows: "C:"
	if(length == 2)
		return path.at (1) == ':';

	// Windows: "C:/"
	if(length == 3)
		return path.at (1) == ':' && path.at (2) == '/';

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isNativePath () const
{
	return protocol == strFileProtocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::toNativePath (uchar* pathBuffer, int bufferSize) const
{
	#if CCL_PLATFORM_WINDOWS
	String temp (path);
	temp.replace (strPathChar, strBackslash);

	// check for network path ("//hostname/...")
	if(!hostname.isEmpty () && isNativePath ())
	{
		temp.prepend (strBackslash);
		temp.prepend (hostname);
		temp.prepend (strDoubleBackslash);
	}

	temp.copyTo (pathBuffer, bufferSize);
	#else
	if(path.startsWith (strPathChar) == false && path.startsWith (strThisFolderPrefix) == false)
	{
		pathBuffer[0] = '/';
		path.copyTo (&pathBuffer[1], bufferSize - 1);
	}
	else
		path.copyTo (pathBuffer, bufferSize);
	#endif
	return !path.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::fromNativePath (const uchar* pathBuffer, int type)
{
	String pathString (pathBuffer);
	return fromNativePath (pathString, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::fromNativePath (StringRef pathString, int _type)
{
	// If path is empty, set empty and return
	if(pathString.isEmpty ())
	{
		protocol.empty ();
		hostname.empty ();
		path.empty ();
		updateType (_type);
		return true;
	}

	#if CCL_PLATFORM_WINDOWS
	// check for UNC prefix (i.e. "\\?\C:\Windows\notepad.exe")
	if(pathString.startsWith (strUncPrefix1) || pathString.startsWith (strUncPrefix2))
	{
		String urlString;
		urlString << strFileProtocol << strProtocolSeparator << strPathChar;
		urlString.append (pathString.subString (strUncPrefix1.length ()));
		urlString.replace (strBackslash, strPathChar);

		setUrl (urlString, _type);
		return true;
	}

	// check for network path ("//hostname/...")
	if(pathString.startsWith (strDoubleBackslash))
	{
		String mutablePathString (pathString);
		mutablePathString.replace (strBackslash, strPathChar);

		String urlString;
		urlString << strFileProtocol << ":";
		urlString.append (mutablePathString);

		setUrl (urlString, _type);
		return true;
	}
	#endif

	protocol = strFileProtocol;
	hostname.empty ();

	if(pathString.startsWith (strPathChar, false))
		path = pathString.subString (1);
	else
		path = pathString;

	#if 1//CCL_PLATFORM_WINDOWS	// backslashes can also appear on other platforms, e.g. when importing foreign file formats written on Windows
	path.replace (strBackslash, strPathChar);
	#endif

	fileType.clear ();
	updateType (_type);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::toPOSIXPath (char* pathBuffer, int bufferSize) const
{
	if(isRelative () || path.at(0) == '/')
		path.toCString (POSIX_STRING_FORMAT, pathBuffer, bufferSize);
	else
	{
		*pathBuffer = '/';
		path.toCString (POSIX_STRING_FORMAT, pathBuffer + 1, bufferSize - 1);
	}
	return !path.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::fromPOSIXPath (const char* pathBuffer, int _type)
{
	protocol = strFileProtocol;
	hostname.empty ();
	path.empty ();

	if(pathBuffer[0] == '/')
		path.appendCString (POSIX_STRING_FORMAT, pathBuffer + 1);
	else
	path.appendCString (POSIX_STRING_FORMAT, pathBuffer);

	fileType.clear ();
	updateType (_type);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::toDisplayString (String& displayString, int which) const
{
	return System::GetFileManager ().getFileDisplayString (displayString, *this, which);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::fromDisplayString (StringRef displayString, int type)
{	
	return fromNativePath (displayString, type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isAbsolute () const
{
	return !isRelative ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::isRelative () const
{
	return path.isEmpty ()
		|| path == strThisFolder
		|| path == strParentFolder
		|| path.startsWith (strThisFolderPrefix)
		|| path.startsWith (strParentFolderPrefix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::makeAbsolute (UrlRef baseUrl)
{
	ASSERT (baseUrl.getType () != kFile || baseUrl.isEmpty ())

	protocol = baseUrl.getProtocol ();
	hostname = baseUrl.getHostName ();

	if(path.isEmpty ())
	{
		path = baseUrl.getPath ();
		updateType (kFolder);
	}
	else
	{
		String relativePath (path);
		path = baseUrl.getPath ();

		int oldType = type; // might be changed by ascend

		uchar delimiter = 0;
		AutoPtr<IStringTokenizer> tokenizer (relativePath.tokenize (CCLSTR ("/")));
		if(tokenizer)
			while(!tokenizer->done ())
			{
				StringRef name (tokenizer->nextToken (delimiter));
				if(name == strParentFolder)
					ascend ();
				else if(name != strThisFolder)
					descend (name, oldType);
			}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::makeRelative (UrlRef baseUrl)
{
	if(baseUrl.getProtocol () != protocol || baseUrl.getHostName () != hostname)
		return false;

	String base (baseUrl.getPath ());
	if(base != strPathChar && !base.isEmpty ())
		base.append (strPathChar);
	if(path.startsWith (base, isCaseSensitive ()) || base.isEmpty ())
	{
		path.remove (0, base.length ());
		if(path.firstChar () == '/')
			path.remove (0, 1);
		if(!path.isEmpty ())
			path.insert (0, strThisFolderPrefix);
		hostname.empty (); // we don't allow a hostname in relative urls, the string representation would be unclear
		return true;
	}
	else if(path == baseUrl.getPath ())
	{
		path = strThisFolder;
		hostname.empty ();
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Url::fromRelativePath (StringRef urlString, UrlRef baseUrl, int _type)
{
	if(isRelativePathString (urlString))
	{
		setPath (urlString, _type);
		makeAbsolute (baseUrl);
	}
	else
		setUrl (urlString, _type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::ascend ()
{
	int index = path.lastIndex (strPathChar);
	if(index != -1)
	{
		path.truncate (index);
		type = kFolder; // <--- type changes to folder!
		fileType.clear ();
		return true;
	}
	else if(!isRootPath () && !path.isEmpty ())
	{
		path.empty ();
		type = kFolder; // <--- type changes to folder!
		fileType.clear ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::descend (StringRef name, int _type)
{
	if(!path.isEmpty () && path.lastChar () != '/' && name.firstChar () != '/')
		path.append (strPathChar);

	path.append (name);

	fileType.clear ();
	updateType (_type);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Url::contains (UrlRef childUrl) const
{
	if(protocol != childUrl.getProtocol ())
		return false;
	if(hostname != childUrl.getHostName ())
		return hostname.isEmpty () && path.isEmpty (); // url with empty hostname & path contains all urls with the same protocol

	if(path.isEmpty ())
		return true; // empty path contains any path

	String p (path);
	if(!p.endsWith (strPathChar))
		p.append (strPathChar);
	return childUrl.getPath ().startsWith (p, isCaseSensitive ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStringDictionary& CCL_API Url::getParameters () const
{
	if(!parameters)
		parameters = System::CreateStringDictionary ();
	return *parameters;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::getParameters (String& params) const
{
	params = UrlEncoder ().encode (getParameters ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::setParameters (StringRef params)
{
	UrlEncoder ().decode (getParameters (), params);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::hasParameters () const
{
	return parameters && parameters->countEntries () > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Url::normalize (int flags)
{
	if(flags & kRemoveDotSegments)
		removeDotSegments ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Url::removeDotSegments ()
{
	if(path.contains (strParentFolderPrefix) || path.contains (strThisFolderPrefix))
	{
		Url result;
		result.setProtocol (protocol);
		result.setHostName (hostname);

		ForEachStringToken (path, strPathChar, name)
			if(name == strThisFolder)
				continue;

			if(name == strParentFolder)
			{
				result.ascend ();
			}
			else
			{
				int type = __tokenizer->done () ? this->type : Url::kFolder;
				result.descend (name, type);
			}
		EndFor

		*this = result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& Url::makeUnique (bool forceSuffix)
{
	return makeUnique (System::GetFileSystem (), forceSuffix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url& Url::makeUnique (IFileSystem& fileSystem, bool forceSuffix)
{
	System::GetFileUtilities ().makeUniqueFileName (fileSystem, *this, forceSuffix);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Url)
	DEFINE_PROPERTY_NAME ("name")
	DEFINE_PROPERTY_NAME ("url")
	DEFINE_PROPERTY_NAME ("extension")
	DEFINE_PROPERTY_NAME ("protocol")
	DEFINE_PROPERTY_NAME ("hostname")
END_PROPERTY_NAMES (Url)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "name")
	{
		String name;
		getName (name);
		var = name;
		var.share ();
		return true;
	}
	else if(propertyId == "url")
	{
		String url;
		getUrl (url);
		var = url;
		var.share ();
		return true;
	}
	else if(propertyId == "extension")
	{
		String ext;
		getExtension (ext);
		var = ext;
		var.share ();
		return true;
	}
	else if(propertyId == "protocol")
	{
		var = getProtocol ();
		var.share ();
		return true;
	}
	else if(propertyId == "hostname")
	{
		var = getHostName ();
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::setProperty (MemberID propertyId, const Variant& var)
{
	CCL_NOT_IMPL ("Url::setProperty")
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Url)
	DEFINE_METHOD_NAME ("ascend")
	DEFINE_METHOD_ARGS ("descend", "name, folder=false")
	DEFINE_METHOD_ARGS ("makeUnique", "forceSuffix=false")
	DEFINE_METHOD_ARGR ("toDisplayString", "", "string")
	DEFINE_METHOD_ARGS ("fromDisplayString", "string, folder=false")
	DEFINE_METHOD_ARGS ("contains", "childUrl: Url")
	DEFINE_METHOD_ARGR ("getName", "withExtension: bool=true", "string")
END_METHOD_NAMES (Url)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Url::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "ascend")
	{
		returnValue = (ascend () != 0);
		return true;
	}
	else if(msg == "descend")
	{
		String name (msg[0].asString ());
		int type = msg.getArgCount () > 1 && msg[1].asBool () ? kFolder : kFile;
		returnValue = (descend (name, type) != 0);
		return true;
	}
	else if(msg == "makeUnique")
	{
		bool forceSuffix = msg.getArgCount () > 0 ? msg[0].asBool () : false;
		makeUnique (forceSuffix);
		return true;
	}
	else if(msg == "toDisplayString")
	{
		String string;
		toDisplayString (string);
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else if(msg == "fromDisplayString")
	{
		int type = msg.getArgCount () > 1 && msg[1].asBool () ? kFolder : kFile;
		fromDisplayString (msg[0].asString (), type);
		return true;
	}
	else if(msg == "contains")
	{
		UnknownPtr<IUrl> childUrl (msg[0]);
		ASSERT (childUrl)
		if(childUrl)
			returnValue = contains (*childUrl);
		return true;
	}
	else if(msg == "getName")
	{
		bool withExtension = msg.getArgCount () > 0 ? msg[0].asBool () : true;

		String string;
		getName (string, withExtension);
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// UrlUtils
//************************************************************************************************

String UrlUtils::extractPackageID (UrlRef url)
{
	String packageId;
	if(url.getProtocol () == PackageUrl::Protocol)
		packageId = url.getHostName ();
	else
	{
		if(url.hasParameters ())
			packageId = url.getParameters ().lookupValue (CCLSTR (UrlParameter::kPackageID));
	}
	return packageId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::getNameFromParameters (UrlRef url, bool withExtension)
{
	String displayName;
	if(url.hasParameters ())
	{
		displayName = url.getParameters ().lookupValue (CCLSTR (UrlParameter::kDisplayName));

		if(!withExtension)
		{
			int extIndex = displayName.lastIndex (strExtensionSeparator);
			if(extIndex != -1)
				displayName.truncate (extIndex);
		}
	}
	return displayName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::getExtensionFromParameters (UrlRef url)
{
	String extension;
	if(url.hasParameters ())
	{
		String displayName = url.getParameters ().lookupValue (CCLSTR (UrlParameter::kDisplayName));	
		
		int extIndex = displayName.lastIndex (strExtensionSeparator);
		if(extIndex != -1)
			extension = displayName.subString (extIndex + 1);
	}
	return extension;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUrl& UrlUtils::fromEncodedString (IUrl& url, StringRef string)
{
	url.setUrl (string);
	url.setPath (UrlEncoder ().decodePathComponents (url.getPath ()));
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::toEncodedString (UrlRef url)
{
	Url url2 (url);
	url2.setPath (UrlEncoder ().encodePathComponents (url2.getPath ()));
	return UrlFullString (url2, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::toResourcePath (UrlRef url)
{
	String path;
	path << Url::strPathChar << url.getPath ();

	// append trailing slash for folder URLs
	if(url.isFolder () && !path.endsWith (Url::strPathChar))
		path << Url::strPathChar;

	String params;
	url.getParameters (params);
	if(!params.isEmpty ())
		path << strQuestionMark << params;

	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::toEncodedPath (StringRef inPath)
{
	String outPath;
	int queryIndex = inPath.index (strQuestionMark);
	if(queryIndex != -1) // we assume the query part to be URL-encoded already (done by Url::getParameters())
	{
		String pathPart = inPath.subString (0, queryIndex);
		String encodedPath = UrlEncoder ().encodePathComponents (pathPart);
		outPath.append (encodedPath);
		outPath.append (inPath.subString (queryIndex)); // including '?'
	}
	else
	{
		String encodedPath = UrlEncoder ().encodePathComponents (inPath);
		outPath.append (encodedPath);
	}
	return outPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::stripLeadingSlashes (StringRef _path)
{
	String path (_path);

	while(path.startsWith (Url::strPathChar))
		path.remove (0, 1);

	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::stripTrailingSlashes (StringRef _path)
{
	String path (_path);

	while(path.endsWith (Url::strPathChar))
		path.truncate (path.length () - 1);

	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::stripSlashes (StringRef path)
{
	return stripTrailingSlashes (stripLeadingSlashes (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::toParentPath (StringRef _path)
{
	String path = stripTrailingSlashes (_path);
	int slashIndex = path.lastIndex (Url::strPathChar);
	return String () << path.subString (0, slashIndex) << Url::strPathChar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String UrlUtils::extractName (StringRef _path)
{
	String path = stripTrailingSlashes (_path);
	int slashIndex = path.lastIndex (Url::strPathChar);
	return path.subString (slashIndex+1);
}

//************************************************************************************************
// Boxed::FileType
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Boxed::FileType, Object, "FileType")
DEFINE_CLASS_NAMESPACE (Boxed::FileType, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::FileType::FileType ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Boxed::FileType::FileType (const CCL::FileType& type)
: CCL::FileType (type)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Boxed::FileType::fromVariant (VariantRef var)
{
	if(UnknownPtr<IObject> object = var.asUnknown ())
		fromProperties (*object);
	else
		setExtension (var.asString ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Boxed::FileType::fromProperties (const IObject& object)
{
	Variant v1;
	object.getProperty (v1, "description");
	setDescription (VariantString (v1));

	Variant v2;
	object.getProperty (v2, "extension");
	setExtension (VariantString (v2));

	Variant v3;
	object.getProperty (v3, "mimetype");
	setMimeType (VariantString (v3));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::FileType::equals (const Object& obj) const
{
	const Boxed::FileType* other = ccl_cast<Boxed::FileType> (&obj);
	if(other)
		return CCL::FileType::equals (*other);
	return SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::FileType::toString (String& string, int flags) const
{
	string = getDescription ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Boxed::FileType::getHashCode (int size) const
{
	return (extension.getHashCode () & 0x7FFFFFFF) % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::FileType::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	description = a.getString ("description");
	extension = a.getString ("extension");
	mimeType = a.getString ("mimeType");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Boxed::FileType::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!description.isEmpty ())
		a.set ("description", description);
	if(!extension.isEmpty ())
		a.set ("extension", extension);
	if(!mimeType.isEmpty ())
		a.set ("mimeType", mimeType);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (Boxed::FileType)
	DEFINE_PROPERTY_TYPE ("description", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("extension", ITypeInfo::kString)
	DEFINE_PROPERTY_TYPE ("mimetype", ITypeInfo::kString)
END_PROPERTY_NAMES (Boxed::FileType)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Boxed::FileType::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "description")
	{
		var = description;
		var.share ();
		return true;
	}
	if(propertyId == "extension")
	{
		var = extension;
		var.share ();
		return true;
	}
	if(propertyId == "mimetype")
	{
		var = mimeType;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// FileTypeFilter
//************************************************************************************************

FileTypeFilter::FileTypeFilter ()
: allowFolders (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Vector<FileType>& FileTypeFilter::getContent ()
{
	return fileTypes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<FileType>& FileTypeFilter::getContent () const
{
	return fileTypes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileTypeFilter::addFileType (const FileType& type)
{
	fileTypes.add (type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API FileTypeFilter::countFileTypes () const
{
	return fileTypes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API FileTypeFilter::getFileType (int index) const
{
	return fileTypes.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeFilter::matches (const FileType& fileType) const
{
	VectorForEach (fileTypes, const FileType&, ft)
		if(ft == fileType)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeFilter::matches (UrlRef url) const
{
	if(url.isFolder ())
		return allowFolders;

	return matches (url.getFileType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeFilter::setContent (const Vector<FileType>& _fileTypes)
{
	fileTypes.removeAll ();
	fileTypes.addAll (_fileTypes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeFilter::setContent (const Container& _fileTypes)
{
	fileTypes.removeAll ();
	ForEach (_fileTypes, Object, obj)
		if(auto ft = ccl_cast<Boxed::FileType> (obj))
			fileTypes.add (*ft);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeFilter::saveContent (Attributes& a) const
{
	a.removeAll ();
	VectorForEach (fileTypes, const FileType&, ft)
		a.queue (nullptr, NEW Boxed::FileType (ft), Attributes::kOwns);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeFilter::loadContent (Attributes& a)
{
	Boxed::FileType* ft;
	while((ft = a.unqueueObject<Boxed::FileType> (nullptr)) != nullptr)
	{
		addFileType (*ft);
		ft->release ();
	}
}

//************************************************************************************************
// FileTypeExcludeFilter
//************************************************************************************************

tbool CCL_API FileTypeExcludeFilter::matches (const FileType& fileType) const
{
	return !FileTypeFilter::matches (fileType);
}

//************************************************************************************************
// HostNameFilter
//************************************************************************************************

HostNameFilter::HostNameFilter (StringRef hostName, bool include)
: hostName (hostName),
  include (include)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HostNameFilter::matches (UrlRef url) const
{
	return (url.getHostName () == hostName) == isInclude ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL
