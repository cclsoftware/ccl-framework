//************************************************************************************************
//
// CCL Builder
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
// Filename    : cclbuilder.cpp
// Description : Command line tool for setting up a new CCL-based project
//
//************************************************************************************************

#include "cclbuilder.h"
#include "appversion.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/development.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

#include "ccl/extras/tools/repositoryinfo.h"

#include "vendor.h"

using namespace CCL;

//************************************************************************************************
// Builder::Vendor
//************************************************************************************************

class Builder::Vendor: public Object
{
public:
	DECLARE_CLASS (Vendor, Object)

	PROPERTY_OBJECT (Url, url, Url)

	PROPERTY_STRING (id, ID)
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (website, Website)
	PROPERTY_STRING (copyright, Copyright)
	PROPERTY_STRING (vendorNamespace, Namespace)
	PROPERTY_STRING (packageDomain, PackageDomain)

	Vendor (StringRef id = nullptr, StringRef name = nullptr, StringRef website = nullptr, StringRef copyright = nullptr, StringRef vendorNamespace = nullptr, StringRef packageDomain = nullptr)
	: id (id),
	  name (name),
	  website (website),
	  copyright (copyright),
	  vendorNamespace (vendorNamespace),
	  packageDomain (packageDomain)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Builder::Vendor, Object)

//************************************************************************************************
// Builder::Variable
//************************************************************************************************

class Builder::Variable: public Object
{
public:
	DECLARE_CLASS (Variable, Object)

	DECLARE_STRINGID_MEMBER (kAttrKey)
	DECLARE_STRINGID_MEMBER (kAttrDefault)
	DECLARE_STRINGID_MEMBER (kAttrDescription)

	PROPERTY_STRING (key, Key)
	PROPERTY_STRING (value, Value)
	PROPERTY_STRING (defaultValue, DefaultValue)
	PROPERTY_STRING (description, Description)

	Variable (StringRef key = nullptr, StringRef value = nullptr, StringRef defaultValue = nullptr, StringRef description = nullptr)
	: key (key),
	  value (value),
	  defaultValue (defaultValue),
	  description (description)
	{}

	// Object
	bool equals (const Object& obj) const override;

	static Variable* create (const Attributes & a);
	bool load (const Attributes& a);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Builder::Variable, Object)

DEFINE_STRINGID_MEMBER_ (Builder::Variable, kAttrKey, "key")
DEFINE_STRINGID_MEMBER_ (Builder::Variable, kAttrDefault, "defaultValue")
DEFINE_STRINGID_MEMBER_ (Builder::Variable, kAttrDescription, "description")

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Variable* Builder::Variable::create (const Attributes& a)
{
	Variable* variable = NEW Variable;
	if(variable->load (a))
		return variable;

	safe_release (variable);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::Variable::load (const Attributes& a)
{
	setKey (a.getString (kAttrKey));
	setDefaultValue (a.getString (kAttrDefault));
	setDescription (a.getString (kAttrDescription));

	return !key.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::Variable::equals (const Object& obj) const
{
	const Variable* other = ccl_cast<Variable> (&obj);
	return other && key == other->key;
}

//************************************************************************************************
// Builder::Replacement
//************************************************************************************************

class Builder::Replacement: public Object
{
public:
	DECLARE_CLASS (Replacement, Object)

	DECLARE_STRINGID_MEMBER (kAttrSearch)
	DECLARE_STRINGID_MEMBER (kAttrReplace)
	DECLARE_STRINGID_MEMBER (kAttrFilter)

	PROPERTY_STRING (searchString, SearchString)
	PROPERTY_STRING (replaceString, ReplaceString)
	PROPERTY_SHARED_AUTO (UrlFilter, urlFilter, UrlFilter)

	// Object
	bool equals (const Object& obj) const override;

	static Replacement* create (const Attributes& a);
	bool load (const Attributes& a);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Builder::Replacement, Object)

DEFINE_STRINGID_MEMBER_ (Builder::Replacement, kAttrSearch, "searchString")
DEFINE_STRINGID_MEMBER_ (Builder::Replacement, kAttrReplace, "replaceString")
DEFINE_STRINGID_MEMBER_ (Builder::Replacement, kAttrFilter, "filter")

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Replacement* Builder::Replacement::create (const Attributes& a)
{
	Replacement* replacement = NEW Replacement;
	if(replacement->load (a))
		return replacement;

	safe_release (replacement);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::Replacement::load (const Attributes& a)
{
	setSearchString (a.getString (kAttrSearch));
	setReplaceString (a.getString (kAttrReplace));

	urlFilter.release ();
	String filter = a.getString (kAttrFilter);
	if(!filter.isEmpty ())
	{
		AutoPtr<PatternFilter> patternFilter;

		ForEachStringToken (filter, ";", token)
			token.trimWhitespace ();
			if(!urlFilter.isValid ())
			{
				patternFilter = NEW PatternFilter;
				setUrlFilter (patternFilter);
				patternFilter->setPositive (true);
			}
			patternFilter->add (token);
		EndFor
	}

	return !searchString.isEmpty () && !replaceString.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::Replacement::equals (const Object& obj) const
{
	const Replacement* other = ccl_cast<Replacement> (&obj);
	return other && searchString == other->searchString;
}

//************************************************************************************************
// Builder::FileEntry
//************************************************************************************************

class Builder::FileEntry: public Object
{
public:
	DECLARE_CLASS (FileEntry, Object)

	DECLARE_STRINGID_MEMBER (kAttrSource)
	DECLARE_STRINGID_MEMBER (kAttrDestination)
	DECLARE_STRINGID_MEMBER (kAttrPlatform)

	PROPERTY_STRING (source, Source)
	PROPERTY_STRING (destination, Destination)
	PROPERTY_STRING (platform, Platform)
	PROPERTY_OBJECT (Url, baseUrl, BaseUrl)

	// Object
	bool equals (const Object& obj) const override;

	static FileEntry* create (const Attributes& a);
	bool load (const Attributes& a);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Builder::FileEntry, Object)

DEFINE_STRINGID_MEMBER_ (Builder::FileEntry, kAttrSource, "source")
DEFINE_STRINGID_MEMBER_ (Builder::FileEntry, kAttrDestination, "destination")
DEFINE_STRINGID_MEMBER_ (Builder::FileEntry, kAttrPlatform, "platform")

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::FileEntry* Builder::FileEntry::create (const Attributes& a)
{
	Builder::FileEntry* file = NEW Builder::FileEntry;
	if(file->load (a))
		return file;

	safe_release (file);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::FileEntry::load (const Attributes& a)
{
	setSource (a.getString (kAttrSource));
	setDestination (a.getString (kAttrDestination));
	setPlatform (a.getString (kAttrPlatform));

	return !source.isEmpty () && !destination.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::FileEntry::equals (const Object& obj) const
{
	const FileEntry* other = ccl_cast<FileEntry> (&obj);
	return other && source == other->source;
}

//************************************************************************************************
// Builder::Template
//************************************************************************************************

class Builder::Template: public JsonStorableObject
{
public:
	DECLARE_CLASS (Template, JsonStorableObject)

	Template ();

	enum Flags
	{
		kHidden = 1 << 0
	};

	DECLARE_STRINGID_MEMBER (kAttrVendor)
	DECLARE_STRINGID_MEMBER (kAttrName)
	DECLARE_STRINGID_MEMBER (kAttrDescription)
	DECLARE_STRINGID_MEMBER (kAttrDestination)
	DECLARE_STRINGID_MEMBER (kAttrInherits)
	DECLARE_STRINGID_MEMBER (kAttrHidden)
	DECLARE_STRINGID_MEMBER (kAttrPlatforms)
	DECLARE_STRINGID_MEMBER (kAttrVariables)
	DECLARE_STRINGID_MEMBER (kAttrReplacements)
	DECLARE_STRINGID_MEMBER (kAttrFiles)

	PROPERTY_STRING (vendorId, VendorID)
	PROPERTY_STRING (name, Name)
	PROPERTY_STRING (description, Description)
	PROPERTY_STRING (destination, Destination)
	PROPERTY_STRING (parent, Parent)
	PROPERTY_OBJECT (Url, sourceFolder, SourceFolder)
	PROPERTY_OBJECT (ObjectList, replacements, Replacements)
	PROPERTY_OBJECT (ObjectList, variables, Variables)
	PROPERTY_OBJECT (ObjectList, files, Files)
	PROPERTY_OBJECT (StringList, platforms, Platforms)
	PROPERTY_FLAG (flags, kHidden, hidden)

	void resolveParent (Template* parent);

	// JsonStorableObject
	bool load (const Storage& storage) override;

private:
	int flags;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Builder::Template, Object)

DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrVendor, "vendor")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrName, "name")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrDescription, "description")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrDestination, "destination")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrInherits, "inherits")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrHidden, "hidden")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrPlatforms, "platforms")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrVariables, "variables")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrReplacements, "replacements")
DEFINE_STRINGID_MEMBER_ (Builder::Template, kAttrFiles, "files")

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Template::Template ()
: flags (0)
{
	replacements.objectCleanup ();
	variables.objectCleanup ();
	files.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::Template::resolveParent (Template* parent)
{
	if(parent == nullptr)
		return;

	ASSERT (parent->getName () == getParent ())

	if(destination.isEmpty ())
		destination = parent->getDestination ();

	if(platforms.isEmpty ())
		platforms.addAllFrom (parent->getPlatforms ());

	if(vendorId.isEmpty ())
		vendorId = parent->getVendorID ();

	ForEach (parent->getVariables (), Variable, variable)
		if(!variables.contains (*variable))
			variables.add (return_shared (variable));
	EndFor

	ForEach (parent->getReplacements (), Replacement, replacement)
		if(!replacements.contains (*replacement))
			replacements.add (return_shared (replacement));
	EndFor

	ForEach (parent->getFiles (), FileEntry, file)
		if(!files.contains (*file))
		{
			// skip file defined in parent if another file with the same destination has been defined in the deriving template
			bool skip = false;
			ForEach (files, FileEntry, existingFile)
				if(existingFile->getDestination () == file->getDestination ())
				{
					Logging::trace ("Skipping %(1). Override exists in deriving template.", file->getSource ());
					skip = true;
					break;
				}
			EndFor
			if(skip)
				continue;

			files.add (return_shared (file));
			file->setBaseUrl (parent->getSourceFolder ());
		}
	EndFor

	setParent (String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::Template::load (const Storage& storage)
{
	const Attributes& a = storage.getAttributes ();

	setVendorID (a.getString (kAttrVendor));
	setName (a.getString (kAttrName));
	setDescription (a.getString (kAttrDescription));
	setDestination (a.getString (kAttrDestination));
	setParent (a.getString (kAttrInherits));

	hidden (a.getBool (kAttrHidden));

	IterForEach (a.newQueueIterator (kAttrPlatforms, ccl_typeid<Attribute> ()), Attribute, attr)
		platforms.add (attr->getValue ());
	EndFor

	IterForEach (a.newQueueIterator (kAttrVariables, ccl_typeid<Attributes> ()), Attributes, attr)
		if(Variable* variable = Variable::create (*attr))
			variables.add (variable);
	EndFor

	IterForEach (a.newQueueIterator (kAttrReplacements, ccl_typeid<Attributes> ()), Attributes, attr)
		if(Replacement* replacement = Replacement::create (*attr))
			replacements.add (replacement);
	EndFor

	IterForEach (a.newQueueIterator (kAttrFiles, ccl_typeid<Attributes> ()), Attributes, attr)
		if(FileEntry* file = FileEntry::create (*attr))
			files.add (file);
	EndFor

	return !name.isEmpty ();
}

//************************************************************************************************
// Builder
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (Builder, kAttrTemplates, "templates")

const String Builder::kVendor = "VendorID";
const String Builder::kVendorName = "VendorName";
const String Builder::kVendorWebsite = "VendorWebsite";
const String Builder::kDestination = "Destination";
const String Builder::kRelativeDestination = "RelativeDestination";
const String Builder::kDefaultCopyright = "DefaultCopyright";
const String Builder::kDefaultNamespace = "DefaultNamespace";
const String Builder::kRepositoryRoot = "RepositoryRoot";
const String Builder::kFrameworkRoot = "FrameworkRoot";
const String Builder::kRelativePathToRoot = "RelativePathToRoot";
const String Builder::kRelativePathToFramework = "RelativePathToFramework";
const String Builder::kNativeRelativePathToRoot = "NativeRelativePathToRoot";
const String Builder::kFolderLevelsToRoot = "FolderLevelsToRoot";
const String Builder::kVendorPackageDomain = "VendorPackageDomain";
const String Builder::kProjectGUID = "ProjectGUID";
const String Builder::kPreferredFrameworkVersion = "PreferredFrameworkVersion";

const String Builder::kIdentityFileName = "identity.cmake";

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Builder ()
: interactive (false),
  selectedTemplate (nullptr),
  selectedVendor (nullptr)
{
	templateDirectories.objectCleanup ();
	identityDirectories.objectCleanup ();
	templates.objectCleanup ();
	vendors.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::initialize ()
{
	Url executablePath;
	System::GetExecutableLoader ().getMainImage ().getPath (executablePath);
	executablePath.ascend ();

	// determine repository root

	// 1. try from working directory
	bool foundRoot = Development::getRootFolder (repositoryRoot, workDir);
	if(!foundRoot)
	{
		// 2. try from folder of executable
		foundRoot = Development::getRootFolder (repositoryRoot, executablePath);
	}

	// load repository info
	
	RepositoryInfo info;
	ObjectList frameworkSearchPaths;
	frameworkSearchPaths.objectCleanup ();

	Url searchRoot = foundRoot ? repositoryRoot : workDir;
	if(info.load (searchRoot))
	{
		int i = templateDirectories.count ();
		info.getPaths (templateDirectories, RepositoryInfo::kTemplateDirectories);
		if(templateDirectories.count () > i)
			Logging::debug ("Using template directories from repo.json:");
		for(; i < templateDirectories.count (); i++)
			Logging::debug ("\t%(1)", UrlDisplayString (*ccl_cast<Url> (templateDirectories.at (i))));

		i = identityDirectories.count ();
		info.getPaths (identityDirectories, RepositoryInfo::kIdentityDirectories);
		if(identityDirectories.count () > i)
			Logging::debug ("Using identity directories from repo.json:");
		for(; i < identityDirectories.count (); i++)
			Logging::debug ("\t%(1)", UrlDisplayString (*ccl_cast<Url> (identityDirectories.at (i))));

		info.getPaths (frameworkSearchPaths, RepositoryInfo::kSubmoduleDirectories);

		if(!foundRoot)
		{
			repositoryRoot = info.getRootDirectory ();
			foundRoot = true;
		}
	}
	
	// try to find a framework submodule
	
	#ifdef CCL_SEARCH_FILE_PATH
	if(searchRoot.contains (executablePath))
		frameworkSearchPaths.add (NEW Url (executablePath));
	
	bool foundFramework = false;
	ForEach (frameworkSearchPaths, Url, searchPath)
		Logging::debug ("Searching for framework in %(1)", UrlDisplayString (*searchPath));
		while(!searchPath->isRootPath ())
		{
			Url versionFilePath (*searchPath);
			versionFilePath.descend (CCL_SEARCH_FILE_PATH, IUrl::kFile);
			if(System::GetFileSystem ().fileExists (versionFilePath))
			{
				frameworkRoot = *searchPath;
				foundFramework = true;
				
				#ifdef RELATIVE_TEMPLATES_DIRECTORY
				AutoPtr<Url> frameworkTemplatesDirectory = NEW Url;
				frameworkTemplatesDirectory->assign (frameworkRoot);
				frameworkTemplatesDirectory->descend (RELATIVE_TEMPLATES_DIRECTORY, IUrl::kFolder);
				if(System::GetFileSystem ().fileExists (*frameworkTemplatesDirectory) && !templateDirectories.contains (*frameworkTemplatesDirectory))
				{
					Logging::debug ("Using framework template directory:");
					Logging::debug ("\t%(1)", UrlDisplayString (*frameworkTemplatesDirectory));
					templateDirectories.add (frameworkTemplatesDirectory.detach ());
				}
				#endif
				
				#ifdef RELATIVE_IDENTITIES_DIRECTORY
				AutoPtr<Url> frameworkIdentitiesDirectory = NEW Url;
				frameworkIdentitiesDirectory->assign (frameworkRoot);
				frameworkIdentitiesDirectory->descend (RELATIVE_IDENTITIES_DIRECTORY, IUrl::kFolder);
				if(System::GetFileSystem ().fileExists (*frameworkIdentitiesDirectory) && !identityDirectories.contains (*frameworkIdentitiesDirectory))
				{
					Logging::debug ("Using framework identities directory:");
					Logging::debug ("\t%(1)", UrlDisplayString (*frameworkIdentitiesDirectory));
					identityDirectories.add (frameworkIdentitiesDirectory.detach ());
				}
				#endif

				Logging::debug ("Found framework in %(1)", UrlDisplayString (frameworkRoot));
				break;
			}
			searchPath->ascend ();
		}
	EndFor
	#endif

	// TEMPLATES_DIRECTORY set by CMake
	#ifdef TEMPLATES_DIRECTORY
	AutoPtr<Url> templatesDirectory = NEW Url;
	templatesDirectory->fromDisplayString (TEMPLATES_DIRECTORY, IUrl::kFolder);
	if(!templatesDirectory->isEmpty ())
	{
		Logging::debug ("Using predefined templates directory: \"%(1)\".", UrlDisplayString (*templatesDirectory));
		templateDirectories.add (templatesDirectory.detach ());
	}
	#endif
	
	// add user identities directory
	System::GetSystem ().getLocation (userIdentitiesPath, System::kUserDocumentFolder);
	userIdentitiesPath.descend (CCL_SHORT_NAME);
	userIdentitiesPath.descend (String (Text::kUTF8, RepositoryInfo::kIdentityDirectories));
	Logging::debug ("Using user identities directory: \"%(1)\".", UrlDisplayString (userIdentitiesPath));
	identityDirectories.add (NEW Url (userIdentitiesPath));
	
	// IDENTITY_DIRECTORY set by CMake
	#ifdef IDENTITIES_DIRECTORY
	AutoPtr<Url> identitiesDirectory = NEW Url;
	identitiesDirectory->fromDisplayString (IDENTITIES_DIRECTORY, IUrl::kFolder);
	if(!identitiesDirectory->isEmpty ())
	{
		Logging::debug ("Using predefined identities directory: \"%(1)\".", UrlDisplayString (*identitiesDirectory));
		identityDirectories.add (identitiesDirectory.detach ());
	}
	#endif
	
	// Search for template and identity directories in the executable path hierarchy
	Url folder (executablePath);
	while(!folder.isRootPath ())
	{
		Url templatesDirectory (folder);
		templatesDirectory.descend (String (Text::kUTF8, RepositoryInfo::kTemplateDirectories), IUrl::kFolder);
		#ifdef TEMPLATES_SUBDIRECTORY
		if(!System::GetFileSystem ().fileExists (templatesDirectory))
		{
			templatesDirectory.ascend ();
			templatesDirectory.descend (TEMPLATES_SUBDIRECTORY, IUrl::kFolder);
			templatesDirectory.descend (String (Text::kUTF8, RepositoryInfo::kTemplateDirectories), IUrl::kFolder);
		}
		#endif
		if(System::GetFileSystem ().fileExists (templatesDirectory))
		{
			Logging::debug ("Using templates directory in executable path: \"%(1)\".", UrlDisplayString (templatesDirectory));
			templateDirectories.add (NEW Url (templatesDirectory));
		}

		Url identitiesDirectory (folder);
		identitiesDirectory.descend (String (Text::kUTF8, RepositoryInfo::kIdentityDirectories), IUrl::kFolder);
		#if CCL_PLATFORM_MAC
		if(!System::GetFileSystem ().fileExists (identitiesDirectory))
		{
			identitiesDirectory.ascend ();
			identitiesDirectory.descend ("Frameworks", IUrl::kFolder);
			identitiesDirectory.descend ("cmake", IUrl::kFolder);
			identitiesDirectory.descend ("ccl", IUrl::kFolder);
			identitiesDirectory.descend (String (Text::kUTF8, RepositoryInfo::kIdentityDirectories), IUrl::kFolder);
		}
		#endif
		if(System::GetFileSystem ().fileExists (identitiesDirectory))
		{
			Logging::debug ("Using identities directory in executable path: \"%(1)\".", UrlDisplayString (identitiesDirectory));
			identityDirectories.add (NEW Url (identitiesDirectory));
		}

		identitiesDirectory.ascend ();
		identitiesDirectory.descend ("cmake");
		identitiesDirectory.descend (String (Text::kUTF8, RepositoryInfo::kIdentityDirectories), IUrl::kFolder);
		if(System::GetFileSystem ().fileExists (identitiesDirectory))
		{
			Logging::debug ("Using identities directory in executable path: \"%(1)\".", UrlDisplayString (identitiesDirectory));
			identityDirectories.add (NEW Url (identitiesDirectory));
		}

		folder.ascend ();
	}

	// set predefined variables
	if(foundRoot)
		setVariable (kRepositoryRoot, UrlDisplayString (repositoryRoot));
	if(foundFramework)
		setVariable (kFrameworkRoot, UrlDisplayString (frameworkRoot));
	
	#ifdef PREFERRED_CCL_VERSION
	setVariable (kPreferredFrameworkVersion, " " PREFERRED_CCL_VERSION);
	#else
	setVariable (kPreferredFrameworkVersion, "");
	#endif

	setVariable (kDefaultCopyright, VENDOR_COPYRIGHT);
	setVariable (kDefaultNamespace, NAMESPACE_CCL);
	setVariable (kVendorPackageDomain, VENDOR_PACKAGE_DOMAIN);
	setVariable (kProjectGUID, UIDString::generate ());

	scanVendors ();
	scanTemplates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::scanTemplates ()
{
	ForEach (templateDirectories, Url, folder)
		ForEachFile (System::GetFileSystem ().newIterator (*folder, IFileIterator::kFiles), p)
			if(p->getFileType () != FileTypes::Json ())
				continue;

			AutoPtr<Template> templ = NEW Template;
			if(templ->loadFromFile (*p))
			{
				bool duplicate = false;
				ForEach (templates, Template, existingTemplate)
					if(existingTemplate->getName () == templ->getName ())
					{
						duplicate = true;
						Logging::debug ("Duplicate template definition: \"%(1)\".", templ->getName ());
					}
				EndFor
				if(duplicate)
					continue;

				templ->setSourceFolder (*folder);
				templates.add (templ.detach ());
			}
		EndFor
	EndFor

	bool anyUnresolved = true;
	while(anyUnresolved)
	{
		anyUnresolved = false;
		ForEach (templates, Template, templ)
			if(!templ->getParent ().isEmpty ())
			{
				Template* parent = lookupTemplate (templ->getParent ());
				if(parent == nullptr)
				{
					Logging::warning ("Template %(1) inherits %(2), but %(2) could not be found.", templ->getName (), templ->getParent ());
					continue;
				}

				if(!parent->getParent ().isEmpty ())
				{
					anyUnresolved = true;
					continue;
				}

				templ->resolveParent (parent);
			}
		EndFor
	}
	
	if(templates.isEmpty ())
		Logging::error ("No templates found.");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::scanVendors ()
{
	vendors.removeAll ();

	for(Url* identityDirectory : iterate_as<Url> (identityDirectories))
	{
		ForEachFile (System::GetFileSystem ().newIterator (*identityDirectory, IFileIterator::kFolders), folder)
			Url identityUrl (*folder);
			identityUrl.descend (kIdentityFileName, IUrl::kFile);

			if(System::GetFileSystem ().fileExists (identityUrl))
			{
				String vendorId;
				folder->getName (vendorId);
				AutoPtr<Vendor> vendor (NEW Vendor (vendorId));
				
				bool duplicate = false;
				ForEach (vendors, Vendor, vendor)
					if(vendor->getID () == vendorId)
					{
						duplicate = true;
						Logging::debug ("Duplicate vendor definition: \"%(1)\".", vendor->getID ());
					}
				EndFor
				if(duplicate)
					continue;

				AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (identityUrl, IStream::kOpenMode);
				AutoPtr<ITextStreamer> reader (System::CreateTextStreamer (*fileStream));
				String line;
				while(reader->readLine (line))
				{
					auto parseValue = [&] (String& value, StringRef key)
					{
						int index = 0;
						if((index = line.index (key)) >= 0)
						{
							int startIndex = line.index ("\"") + 1;
							int length = line.lastIndex ("\"") - startIndex;
							if(length > 0)
							{
								value = line.subString (startIndex, length);
								return true;
							}
						}
						return false;
					};

					String value;
					if(parseValue (value, "(VENDOR_NAME"))
						vendor->setName (value);
					if(parseValue (value, "(VENDOR_COPYRIGHT"))
						vendor->setCopyright (value);
					if(parseValue (value, "(VENDOR_NAMESPACE"))
						vendor->setNamespace (value);
					if(parseValue (value, "(VENDOR_PACKAGE_DOMAIN"))
						vendor->setPackageDomain (value);
					if(parseValue (value, "(VENDOR_WEBSITE"))
						vendor->setWebsite (value);
				}

				String copyright = vendor->getCopyright ();
				copyright.replace ("${VENDOR_COPYRIGHT_YEAR}", VENDOR_COPYRIGHT_YEAR);
				copyright.replace ("${VENDOR_NAME}", vendor->getName ());
				vendor->setCopyright (copyright);

				vendor->setUrl (identityUrl);

				vendors.add (vendor.detach ());
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::listTemplates ()
{
	if(templates.isEmpty ())
		return;

	Logging::info ("Available templates: ");
	ForEach (templates, Template, templ)
		if(templ->hidden ())
			continue;
		Logging::info ("   %(1) (%(2))", templ->getName (), templ->getDescription ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::listVendors ()
{
	Logging::info ("Available vendors: ");
	ForEach (vendors, Vendor, vendor)
		Logging::info ("   %(1)", vendor->getID ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::resolvePath (Url& url, UrlRef basePath, StringRef path)
{
	String evaluatedPath = path;
	evaluate (evaluatedPath);

	if(evaluatedPath.startsWith (CCLSTR ("."))
		|| evaluatedPath.startsWith (Url::strPathChar)
		|| evaluatedPath.contains (CCLSTR ("://"))
		|| evaluatedPath.contains (CCLSTR (":\\")))
	{
		// handle absolute paths or "explicit" relative paths (relative to working dir)
		makeAbsolute (url, evaluatedPath, Url::kFolder);
	}
	else
	{
		// interpret as relative path to given basePath
		url.fromDisplayString (evaluatedPath, Url::kFolder);
		url.makeAbsolute (basePath);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::addTemplateFolder (UrlRef folder)
{
	if(File (folder).exists ())
		templateDirectories.add (NEW Url (folder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::setVariable (StringRef key, StringRef value)
{
	variables.setEntry (key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::prepare ()
{
	if(templates.isEmpty ())
		return false;

	while(templateName.isEmpty ())
	{
		Template* defaultTemplate = ccl_cast<Template> (templates.at (0));
		if(defaultTemplate == nullptr)
			return false;
		listTemplates ();
		Logging::info ("Type the name of a template or press enter to select the default template (%(1))", defaultTemplate->getName ());
		console.readLine (templateName);

		if(templateName.isEmpty ())
			templateName = defaultTemplate->getName ();

		selectedTemplate = lookupTemplate (templateName);
		if(selectedTemplate == nullptr)
			templateName.empty ();
	}

	selectedTemplate = lookupTemplate (templateName);
	if(selectedTemplate == nullptr)
	{
		Logging::error ("Unknown template \"%(1)\".", templateName);
		return false;
	}

	if(vendorId.isEmpty ())
	{
		vendorId = selectedTemplate->getVendorID ();
		selectedVendor = lookupVendor (vendorId);
		if(selectedVendor == nullptr)
			vendorId.empty ();
	}
	while(vendorId.isEmpty ())
	{
		listVendors ();
		Logging::info ("Type a vendor ID or press enter to create a new vendor identity");
		console.readLine (vendorId);

		if(vendorId.isEmpty ())
			prepareVendorID (vendorId);

		selectedVendor = lookupVendor (vendorId);
		if(selectedVendor == nullptr)
			vendorId.empty ();
	}

	selectedVendor = lookupVendor (vendorId);
	if(selectedVendor == nullptr)
	{
		Logging::error ("Unknown vendor id \"%(1)\".", vendorId);
		return false;
	}
	
	if(!selectedVendor->getCopyright ().isEmpty ())
		setVariable (kDefaultCopyright, selectedVendor->getCopyright ());
	if(!selectedVendor->getNamespace ().isEmpty ())
		setVariable (kDefaultNamespace, selectedVendor->getNamespace ());
	if(!selectedVendor->getPackageDomain ().isEmpty ())
		setVariable (kVendorPackageDomain, selectedVendor->getPackageDomain ());
	setVariable (kVendor, selectedVendor->getID ());
	setVariable (kVendorName, selectedVendor->getName ());
	setVariable (kVendorWebsite, selectedVendor->getWebsite ());

	if(selectedPlatforms.isEmpty ())
	{
		Logging::info ("Platforms available for this template:");
		String allPlatforms = selectedTemplate->getPlatforms ().concat (",");
		Logging::info ("   %(1)", allPlatforms);
		Logging::info ("Type a comma separated list of platforms or press enter to use all available platforms");
		console.readLine (selectedPlatforms);
		if(selectedPlatforms.isEmpty ())
			selectedPlatforms = allPlatforms;
	}

	ForEach (selectedTemplate->getVariables (), Variable, var)
		String value = variables.lookupValue (var->getKey ());
		while(value.isEmpty ())
		{
			String message ("Type a value for ");
			message.appendFormat ("%(1) (%(2))", var->getKey (), var->getDescription ());
			String defaultValue = var->getDefaultValue ();
			evaluate (defaultValue);
			if(!defaultValue.isEmpty ())
				message.appendFormat (" or press enter to use the default value (%(1))", defaultValue);
			Logging::info (message);
			console.readLine (value);

			if(value.isEmpty ())
				value = defaultValue;

			setVariable (var->getKey (), value);
		}
	EndFor

	Url baseDirectory = repositoryRoot.isEmpty () ? workDir : repositoryRoot;

	destFolder = Url::kEmpty;
	if(!destPath.isEmpty ())
		resolvePath (destFolder, baseDirectory, destPath);

	while(destFolder.isEmpty ())
	{
		String pathString = selectedTemplate->getDestination ();
		evaluate (pathString);
		Logging::info ("Type in the destination path or press enter to select the default path (%(1))", pathString);
		console.readLine (pathString);

		if(pathString.isEmpty ())
			pathString = selectedTemplate->getDestination ();

		if(!pathString.isEmpty ())
			resolvePath (destFolder, baseDirectory, pathString);
	}

	if(File (destFolder).exists ())
	{
		Logging::warning ("Destination already exists!");
		Logging::info ("Do you want to use this destination path anyway? (y/N)");
		String answer;
		console.readLine (answer);
		if(answer != "y" && answer != "Y")
			return false;
	}

	setVariable (kDestination, UrlDisplayString (destFolder));
	Logging::debug ("Destination set to %(1)", UrlDisplayString (destFolder));

	while(repositoryRoot.isEmpty ())
	{
		String pathString = UrlDisplayString (destFolder);
		Logging::info ("Could not find repository root. Type the path to the root directory of the repository or press enter to use the default path (%(1))", pathString);
		console.readLine (pathString);

		if(pathString.isEmpty ())
			pathString = UrlDisplayString (destFolder);

		if(!pathString.isEmpty ())
		{
			resolvePath (repositoryRoot, baseDirectory, pathString);
			setVariable (kRepositoryRoot, UrlDisplayString (repositoryRoot));
			Logging::debug ("Repository Root set to %(1).", UrlDisplayString (destFolder));

			Url repositoryInfoUrl (repositoryRoot);
			repositoryInfoUrl.descend (RepositoryInfo::kFileName, IUrl::kFile);
			File repositoryInfoFile (repositoryInfoUrl);
			if(!repositoryInfoFile.exists ())
			{
				if(!repositoryInfoFile.create ())
					Logging::warning ("Failed to create a repository info file at %(1).", UrlDisplayString (repositoryInfoUrl));
			}
		}
	}
	
	Url relativeDestPath;
	resolvePath (relativeDestPath, repositoryRoot, destPath);
	relativeDestPath.makeRelative (repositoryRoot);
	setVariable (kRelativeDestination, relativeDestPath.getPath ());
	Logging::debug ("Relative path to destination: %(1).", relativeDestPath.getPath ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::prepareVendorID (CCL::String& vendorId)
{
	Logging::info ("Type a new vendor ID");
	console.readLine (vendorId);
	if(vendorId.isEmpty ())
		return;

	Url identityFilePath (userIdentitiesPath);
	identityFilePath.descend (vendorId);
	identityFilePath.descend ("identity.cmake");
	
	String vendorName;
	String vendorWebsite;
	String vendorMail;
	String vendorNamespace;
	String vendorPackagingDomain;

	Logging::info ("Type the vendor's full name, e.g. My Company");
	console.readLine (vendorName);
	Logging::info ("Type the vendor's website URL");
	console.readLine (vendorWebsite);
	Logging::info ("Type the vendor's default email address");
	console.readLine (vendorMail);
	Logging::info ("Type the vendor's default C++ namespace");
	console.readLine (vendorNamespace);
	Logging::info ("Type the vendor's packaging domain, e.g. com.%(1)", vendorId);
	console.readLine (vendorPackagingDomain);
	
	{
		AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (identityFilePath, IStream::kCreateMode);
		if(!fileStream.isValid ())
		{
			vendorId.empty ();
			return;
		}

		AutoPtr<ITextStreamer> writer (System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kLFLineFormat}));
		if(!writer.isValid ())
		{
			vendorId.empty ();
			return;
		}

		writer->writeLine (String ().appendFormat ("set (VENDOR_NAME \"%(1)\")", vendorName));
		writer->writeLine ("string (TIMESTAMP VENDOR_COPYRIGHT_YEAR \"%Y\")");
		writer->writeLine ("set (VENDOR_COPYRIGHT \"Copyright (c) ${VENDOR_COPYRIGHT_YEAR} ${VENDOR_NAME}\")");
		writer->writeLine (String ().appendFormat ("set (VENDOR_WEBSITE \"%(1)\")", vendorWebsite));
		writer->writeLine (String ().appendFormat ("set (VENDOR_MAIL \"%(1)\")", vendorMail));
		writer->writeNewline ();
		writer->writeLine ("set (VENDOR_PUBLISHER \"${VENDOR_NAME}\")");
		writer->writeLine ("set (VENDOR_PUBLISHER_WEBSITE \"${VENDOR_WEBSITE}\")");
		writer->writeNewline ();
		writer->writeLine (String ().appendFormat ("set (VENDOR_PACKAGE_DOMAIN \"%(1)\")", vendorPackagingDomain));
		writer->writeLine (String ().appendFormat ("set (VENDOR_MIME_TYPE \"application/x.%(1)\")", vendorId));
		writer->writeNewline ();
		writer->writeLine ("set (VENDOR_INSTALL_SUBDIR \"\")");
		writer->writeNewline ();
		writer->writeLine (String ().appendFormat ("set (VENDOR_NAMESPACE \"%(1)\")", vendorNamespace));
	
		Logging::info ("A new identity file has been created at %(1).", UrlDisplayString (identityFilePath));
	}

	scanVendors ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::copyFiles ()
{
	if(selectedTemplate == nullptr)
		return false;

	bool succeeded = true;
	ForEach (selectedTemplate->getFiles (), FileEntry, file)
		if(!file->getPlatform ().isEmpty () && !selectedPlatforms.contains (file->getPlatform ()))
			continue;

		Url sourceUrl;
		resolvePath (sourceUrl, file->getBaseUrl ().isEmpty () ? selectedTemplate->getSourceFolder () : file->getBaseUrl (), file->getSource ());

		Url destinationUrl;
		resolvePath (destinationUrl, repositoryRoot, file->getDestination ());

		if(!System::GetFileSystem ().fileExists (sourceUrl))
		{
			sourceUrl.descend ("", IUrl::kFile);
			if(file->getDestination ().endsWith ("/"))
			{
				String fileName;
				sourceUrl.getName (fileName);
				destinationUrl.descend (fileName, IUrl::kFile);
			}
			else
				destinationUrl.descend ("", IUrl::kFile);
		}

		if(!System::GetFileSystem ().fileExists (sourceUrl))
		{
			Logging::error ("Failed to copy %(1)!", file->getSource ());
			return false;
		}

		// If files are copied to a location outside of destFolder, ask before overwriting existing files
		if(System::GetFileSystem ().fileExists (destinationUrl) && !destinationUrl.getPath ().startsWith (destFolder.getPath ()))
		{
			bool overwrite = false;

			while(interactive)
			{
				Logging::info ("%(1) already exists. Overwrite existing files? (y/n)", UrlDisplayString (destinationUrl));
				String answer;
				console.readLine (answer);
				if(answer == "y" || answer == "Y")
				{
					overwrite = true;
					break;
				}
				else if(answer == "n" || answer == "N")
					break;
			}

			if(!overwrite)
			{
				Logging::warning ("Skipping %(1). File already exists.", UrlDisplayString (destinationUrl));
				continue;
			}
		}

		if(sourceUrl.isFile ())
		{
			succeeded |= ToolHelper::copyFile (destinationUrl, sourceUrl);
			if(succeeded)
				Logging::debug ("Copied file %(1) to %(2).", file->getSource (), UrlDisplayString (destinationUrl));
			else
			{
				Logging::error ("Failed to copy file : %(1)!", file->getSource ());
				break;
			}
		}
		else
		{
			succeeded |= ToolHelper::copyFolder (destinationUrl, sourceUrl, UrlFilter (), true);
			if(succeeded)
				Logging::debug ("Copied folder %(1) to %(2).", file->getSource (), UrlDisplayString (destinationUrl));
			else
			{
				Logging::error ("Failed to copy folder: %(1)!", file->getSource ());
				break;
			}
		}
	EndFor

	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::replaceStrings (UrlRef path)
{
	if(selectedTemplate == nullptr)
		return false;

	if(path.isFolder ())
	{
		ForEachFile (System::GetFileSystem ().newIterator (path), p)
			replaceStrings (*p);
		EndFor
	}
	else
	{
		TextResource textFile;
		textFile.setSuppressByteOrderMark (true);
		textFile.setSuppressFinalLineEnd (false); // don't strip a final lineEnd (Windows resource compiler does not like it; todo: only keep if it existed in the original file)
		String content;
		int numReplaced = 0;

		auto getRelativePath = [&] (String& result, int& levels, UrlRef root)
		{
			Url filePath (path);
			filePath.ascend ();
			int folderLevels = 0;
			if(filePath.getPath ().startsWith (root.getPath ()))
			{
				while(filePath != root)
				{
					folderLevels++;
					result.append ("../");
					filePath.ascend ();
				}
			}
			result.truncate (result.length () - 1);
		};

		String relativePathToRoot;
		int folderLevelsToRoot = 0;
		getRelativePath (relativePathToRoot, folderLevelsToRoot, repositoryRoot);
		
		Url relativeFrameworkRoot (frameworkRoot);
		relativeFrameworkRoot.makeRelative (repositoryRoot);
		String relativePathToFramework = relativeFrameworkRoot.getPath ();
		Logging::debug ("relative path to framework: %(1)", relativePathToFramework);

		Url relativeUrl ("", "", relativePathToRoot, IUrl::kFolder);
		NativePath nativePath (relativeUrl);
		String nativePathToRoot (nativePath.path);

		ForEach (selectedTemplate->getReplacements (), Replacement, replacement)
			if(!replacement->getUrlFilter () || replacement->getUrlFilter ()->matches (path))
			{
				if(content.isEmpty ())
					if(textFile.loadFromFile (path))
						content = textFile.getContent ();

				String replaceString = replacement->getReplaceString ();
				evaluate (replaceString);

				replaceString.replace (String ("@").append (kNativeRelativePathToRoot), nativePath.path);
				replaceString.replace (String ("@").append (kRelativePathToRoot), relativePathToRoot);
				replaceString.replace (String ("@").append (kFolderLevelsToRoot), String ().appendIntValue (folderLevelsToRoot));
				replaceString.replace (String ("@").append (kRelativePathToFramework), relativePathToFramework);

				numReplaced += content.replace (replacement->getSearchString (), replaceString);
			}
		EndFor

		if(!content.isEmpty ())
		{
			textFile.setContent (content);
			bool result = textFile.saveToFile (path);
			if(result)
				Logging::debug ("Replaced %(1) string(s) in %(2).", numReplaced, UrlDisplayString (path));
			else
			{
				Logging::error ("Failed to save file: %(1).", UrlDisplayString (path));
				return false;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::replaceStrings ()
{
	if(selectedTemplate == nullptr)
		return false;

	ForEach (selectedTemplate->getFiles (), FileEntry, file)
		if(!file->getPlatform ().isEmpty () && !selectedPlatforms.contains (file->getPlatform ()))
			continue;

		Url destinationUrl;
		resolvePath (destinationUrl, repositoryRoot, file->getDestination ());

		if(!System::GetFileSystem ().fileExists (destinationUrl))
			destinationUrl.descend ("", IUrl::kFile);

		if(!System::GetFileSystem ().fileExists (destinationUrl))
		{
			Logging::error ("File not found: %(1)!", file->getDestination ());
			return false;
		}

		if(!replaceStrings (destinationUrl))
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::renameFiles (UrlRef path)
{
	if(selectedTemplate == nullptr)
		return false;

	if(path.isFolder ())
	{
		ForEachFile (System::GetFileSystem ().newIterator (path), p)
			renameFiles (*p);
		EndFor
	}

	String originalFileName;
	path.getName (originalFileName);
	String fileName = originalFileName;

	ForEach (selectedTemplate->getReplacements (), Replacement, replacement)
		if(path.isFolder () || !replacement->getUrlFilter () || replacement->getUrlFilter ()->matches (path))
		{
			String replaceString = replacement->getReplaceString ();
			evaluate (replaceString);
			fileName.replace (replacement->getSearchString (), replaceString);
		}
	EndFor

	if(fileName != originalFileName)
	{
		if(File (path).rename (fileName))
			Logging::debug ("Renamed: %(1) to %(2).", UrlDisplayString (path), fileName);
		else
		{
			Logging::error ("Failed to rename %(1) to %(2).", UrlDisplayString (path), fileName);
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::renameFiles ()
{
	if(selectedTemplate == nullptr)
		return false;

	ForEach (selectedTemplate->getFiles (), FileEntry, file)
		if(!file->getPlatform ().isEmpty () && !selectedPlatforms.contains (file->getPlatform ()))
			continue;

		Url destinationUrl;
		resolvePath (destinationUrl, repositoryRoot, file->getDestination ());

		if(!System::GetFileSystem ().fileExists (destinationUrl))
			destinationUrl.descend ("", IUrl::kFile);

		if(!System::GetFileSystem ().fileExists (destinationUrl))
		{
			Logging::error ("File not found: %(1)!", file->getDestination ());
			return false;
		}

		if(!renameFiles (destinationUrl))
			return false;
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::checkIdentityFile ()
{
	#ifdef RELATIVE_IDENTITIES_DIRECTORY
	if(selectedVendor == nullptr)
		return false;

	if(repositoryRoot.isEmpty ())
		return false;

	if(repositoryRoot.contains (selectedVendor->getUrl ()))
		return true;

	Url identityDirectory (repositoryRoot);
	identityDirectory.descend (RELATIVE_IDENTITIES_DIRECTORY, IUrl::kFolder);
	identityDirectory.descend (selectedVendor->getID (), IUrl::kFolder);

	if(System::GetFileSystem ().fileExists (identityDirectory))
		return true;

	Logging::warning ("Vendor identity file is located outside of the repository root.");
	
	Logging::info ("Copy the identity file to %(1)? (y/N)", UrlDisplayString (identityDirectory));
	String answer;
	console.readLine (answer);
	if(answer == "y" || answer == "Y")
	{
		String fileName;
		selectedVendor->getUrl ().getName (fileName);
		identityDirectory.descend (fileName, IUrl::kFile);
		if(!ToolHelper::copyFile (identityDirectory, selectedVendor->getUrl ()))
			Logging::error ("Failed to copy identity file!");
	}
	#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Builder::evaluate (String& string)
{
	for(int i = 0, numVariables = variables.countEntries (); i < numVariables; i++)
		string.replace (String ("@").append (variables.getKeyAt (i)), variables.getValueAt (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Template* Builder::lookupTemplate (StringRef name) const
{
	ForEach (templates, Template, templ)
		if(templ->getName () == name)
			return templ;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Builder::Vendor* Builder::lookupVendor (StringRef vendorId) const
{
	ForEach (vendors, Vendor, vendor)
		if(vendor->getID () == vendorId)
			return vendor;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Builder::run ()
{
	if(!prepare ())
		return false;

	Logging::info ("Copying files...");
	if(!copyFiles ())
	{
		Logging::error ("Failed to copy files!");
		return false;
	}

	// replace before renaming (url filters refer to original filenames)
	Logging::info ("Replacing strings...");
	if(!replaceStrings ())
	{
		Logging::error ("Failed to replace strings!");
		return false;
	}

	Logging::info ("Renaming files...");
	if(!renameFiles ())
	{
		Logging::error ("Failed to rename files!");
		return false;
	}

	if(!checkIdentityFile ())
		return false;

	return true;
}
