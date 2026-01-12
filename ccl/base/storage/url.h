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
// Filename    : ccl/base/storage/url.h
// Description : Url class
//
//************************************************************************************************

#ifndef _ccl_url_h
#define _ccl_url_h

#include "ccl/base/object.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

class Attributes;
class Container;
interface IFileSystem;

//************************************************************************************************
// Url
//************************************************************************************************

class Url: public Object,
		   public IUrl
{
public:
	DECLARE_CLASS (Url, Object)
	DECLARE_PROPERTY_NAMES (Url)
	DECLARE_METHOD_NAMES (Url)

	/** Construct an empty URL. */
	Url ();

	/** Construct from a URL string.
	* \param url A URL string in the form <protocol>://<hostname>/<path>, see also https://datatracker.ietf.org/doc/html/rfc1738
	* This constructor URL-decodes URL parameters, but does not decode protocol, hostname, or path. See also \a UrlUtils::fromEncodedString.
	* If you need to convert a display string to a URL, consider using \a fromDisplayString or \a fromNativePath instead. */
	Url (StringRef url, int type = kFile);

	/** Construct from protocol, hostname and path. */
	Url (StringRef protocol, StringRef hostname, StringRef path, int type = kFile);

	/** Copy constructor. */
	Url (UrlRef url);

	/** Construct from a base URL and a relative path string.
	 * Equivalent to calling fromRelativePath. */
	Url (StringRef relative, UrlRef baseUrl, int type = kFile);

	Url (const Url& url); // gcc needs this!
	~Url ();
		
	static const Url kEmpty;
	static const String strPathChar;
	static const String strBackslash;

	// IUrl
	void CCL_API clone (IUrl*& url) const override;
	void CCL_API assign (UrlRef url) override;
	tbool CCL_API isEqualUrl (UrlRef url, tbool withParameters = true) const override;
	tbool CCL_API isEmpty () const override;
	int CCL_API getType () const override;
	void CCL_API getUrl (String& url, tbool withParameters = false) const override;
	void CCL_API setUrl (StringRef url, int type = kFile) override;
	StringRef CCL_API getProtocol () const override;
	void CCL_API setProtocol (StringRef protocol) override;
	StringRef CCL_API getHostName () const override;
	void CCL_API setHostName (StringRef name) override;
	StringRef CCL_API getPath () const override;
	void CCL_API setPath (StringRef path, int type = kIgnore) override;
	void CCL_API getPathName (String& pathName) const override;
	void CCL_API getName (String& name, tbool withExtension = true) const override;
	void CCL_API setName (StringRef name, int type = kIgnore) override;
	tbool CCL_API getExtension (String& ext) const override;
	void  CCL_API setExtension (StringRef ext, tbool replace = true) override;
	const FileType& CCL_API getFileType () const override;
	void CCL_API setFileType (const FileType& type, tbool replaceExtension = true) override;
	tbool CCL_API isRootPath () const override;
	tbool CCL_API isNativePath () const override;
	tbool CCL_API toNativePath (uchar* pathBuffer, int bufferSize) const override;
	tbool CCL_API fromNativePath (const uchar* pathBuffer, int type = kFile) override;
	tbool CCL_API toPOSIXPath (char* pathBuffer, int bufferSize) const override;
	tbool CCL_API fromPOSIXPath (const char* pathBuffer, int type = kFile) override;
	tbool CCL_API toDisplayString (String& displayString, int which = kStringNativePath) const override;
	tbool CCL_API fromDisplayString (StringRef displayString, int type = kFile) override;
	tbool CCL_API isAbsolute () const override;
	tbool CCL_API isRelative () const override;
	tbool CCL_API makeAbsolute (UrlRef baseUrl) override;
	tbool CCL_API makeRelative (UrlRef baseUrl) override;
	tbool CCL_API ascend () override;
	tbool CCL_API descend (StringRef name, int type = kFile) override;
	void CCL_API normalize (int flags) override;
	IStringDictionary& CCL_API getParameters () const override;
	void CCL_API getParameters (String& params) const override;
	void CCL_API setParameters (StringRef params) override;
	tbool CCL_API hasParameters () const override;

	bool isCaseSensitive () const;
	bool contains (UrlRef childUrl) const;

	Url& makeUnique (bool forceSuffix = false);
	Url& makeUnique (IFileSystem& fileSystem, bool forceSuffix = false);
	void fromRelativePath (StringRef relative, UrlRef baseUrl, int type = kFile);
	
	static bool isUrlString (StringRef string);
	static bool isRelativePathString (StringRef relative);

	// Object
	bool equals (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	// Operators
	Url& operator = (const Url& url);
	Url& operator = (const IUrl& url);

	bool operator == (UrlRef url) const;
	bool operator != (UrlRef url) const;

	CLASS_INTERFACE (IUrl, Object)

protected:
	int type;
	String protocol;
	String hostname;
	String path;
	mutable FileType fileType;
	mutable IStringDictionary* parameters;

	class Comparer;

	void updateType (int type);
	bool fromNativePath (StringRef, int type);
	void removeDotSegments ();	

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// UrlWithTitle
//************************************************************************************************

class UrlWithTitle: public Url
{
public:
	DECLARE_CLASS (UrlWithTitle, Url)

	UrlWithTitle (UrlRef url = Url (), StringRef title = nullptr);

	PROPERTY_STRING (title, Title)

	// Url
	int compare (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// LocalizedUrl
//************************************************************************************************

class LocalizedUrl: public Url
{
public:
	LocalizedUrl (UrlRef url, StringRef resourceName);

	static bool localize (Url& url, StringRef resourceName);
};

//************************************************************************************************
// ResourceUrl
//************************************************************************************************

class ResourceUrl: public Url
{
public:
	ResourceUrl (StringRef path, int type = kFile);	///< resource resides in current module
	ResourceUrl (ModuleRef module, StringRef path, int type = kFile);

	static const String Protocol;
};

//************************************************************************************************
// PackageUrl
//************************************************************************************************

class PackageUrl: public Url
{
public:
	PackageUrl (StringRef packageId, StringRef path = nullptr, int type = kFile);

	static const String Protocol;
};

//************************************************************************************************
// MemoryUrl
//************************************************************************************************

class MemoryUrl: public Url
{
public:
	MemoryUrl (StringRef binName, StringRef path = nullptr, int type = kFile);

	static const String Protocol;
	static Url* newBin ();
};

//************************************************************************************************
// UrlUtils
//************************************************************************************************

namespace UrlUtils
{
	String extractPackageID (UrlRef url); ///< extract package id from package URL or URL parameter
	String getNameFromParameters (UrlRef url, bool withExtension = true); ///< get name from URL parameter (if available)
	String getExtensionFromParameters (UrlRef url); ///< get extension from URL parameter (if available)

	IUrl& fromEncodedString (IUrl& url, StringRef string); ///< handle URL-decoding
	String toEncodedString (UrlRef url); ///< handle URL-encoding
	
	String toResourcePath (UrlRef url); ///< extract resource path and parameters
	String toEncodedPath (StringRef resourcePath); ///< make sure path is URL-encoded
	
	String stripLeadingSlashes (StringRef path); ///< strip leading slashes
	String stripTrailingSlashes (StringRef path); ///< strip trailing slashes
	String stripSlashes (StringRef path); ///< strip leading and trailing slashes
	String toParentPath (StringRef path); ///< get path to parent folder
	String extractName (StringRef path); ///< extract file/folder name from path
}

//************************************************************************************************
// LegalFileName
//************************************************************************************************

class LegalFileName: public String
{
public:
	LegalFileName (StringRef fileName);
};

//************************************************************************************************
// LegalFolderName
//************************************************************************************************

class LegalFolderName: public LegalFileName
{
public:
	LegalFolderName (StringRef fileName);
};

//************************************************************************************************
// Boxed::FileType
//************************************************************************************************

namespace Boxed 
{
	class FileType: public Object,
					public CCL::FileType
	{
	public:
		DECLARE_CLASS (FileType, Object)
		DECLARE_PROPERTY_NAMES (FileType)

		FileType ();
		FileType (const CCL::FileType& type);

		void fromVariant (VariantRef var);
		void fromProperties (const IObject& object);

		// Object
		bool equals (const Object& obj) const override;
		bool toString (CCL::String& string, int flags = 0) const override;
		int getHashCode (int size) const override;
		bool load (const Storage& storage) override;
		bool save (const Storage& storage) const override;

	protected:
		// IObject
		tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	};
}

//************************************************************************************************
// FileTypeFilter
/** URL filter that matches a collection of fileTypes . */
//************************************************************************************************

class FileTypeFilter: public UrlFilter,
					  public IFileTypeFilter
{
public:
	FileTypeFilter ();

	PROPERTY_BOOL (allowFolders, AllowFolders)

	Vector<FileType>& getContent ();
	const Vector<FileType>& getContent () const;

	void setContent (const Vector<FileType>& fileTypes);
	void setContent (const Container& fileTypes);

	void saveContent (Attributes& a) const;
	void loadContent (Attributes& a);

	// IFileTypeFilter
	void CCL_API addFileType (const FileType& type) override;
	int CCL_API countFileTypes () const override;
	const FileType& CCL_API getFileType (int index) const override;
	tbool CCL_API matches (const FileType& fileType) const override;

	// UrlFilter
	tbool CCL_API matches (UrlRef url) const override;

	CLASS_INTERFACE (IFileTypeFilter, UrlFilter)

protected:
	Vector<FileType> fileTypes;
};

//************************************************************************************************
// FileTypeExcludeFilter
/** URL filter to exclude given file types. */
//************************************************************************************************

class FileTypeExcludeFilter: public FileTypeFilter
{
public:
	// FileTypeFilter
	tbool CCL_API matches (const FileType& fileType) const override;
};

//************************************************************************************************
// HostNameFilter
//************************************************************************************************

class HostNameFilter: public UrlFilter
{
public:
	HostNameFilter (StringRef hostName, bool include = true);

	PROPERTY_STRING (hostName, HostName)
	PROPERTY_BOOL (include, Include)

	// UrlFilter
	tbool CCL_API matches (UrlRef url) const override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Url inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Url::Url ()							: type (kFile), parameters (nullptr) {}
inline Url::Url (StringRef url, int type)	: type (kFile), parameters (nullptr) { setUrl (url, type); }
inline Url::Url (UrlRef url)				: type (kFile), parameters (nullptr) { assign (url); }
inline Url::Url (const Url& url)			: type (kFile), parameters (nullptr) { assign (url); }
inline Url::Url (StringRef relative, UrlRef baseUrl, int type) 
                                            : type (kFile), parameters (nullptr) { fromRelativePath (relative, baseUrl, type); }

inline Url& Url::operator = (const Url& url)	{ assign (url); return *this; }
inline Url& Url::operator = (const IUrl& url)	{ assign (url); return *this; }
inline bool Url::operator == (UrlRef url) const { return isEqualUrl (url) != 0; }
inline bool Url::operator != (UrlRef url) const { return isEqualUrl (url) == 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_url_h
