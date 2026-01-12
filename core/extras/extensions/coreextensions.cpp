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
// Filename    : core/extras/extensions/coreextensions.cpp
// Description : Extension Management
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "core/extras/extensions/coreextensions.h"

#include "core/portable/corepersistence.h"
#include "core/portable/corestorage.h"
#include "core/portable/corebasecodec.h"

#include "core/portable/gui/coreviewbuilder.h"
#include "core/portable/gui/corefont.h"

#include "core/text/corejsonhandler.h"
#include "core/system/coredebug.h"
#include "core/public/corejsonsecurity.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// PackageInfo
//************************************************************************************************

PackageInfo::PackageInfo ()
: Attributes (AttributeAllocator::getDefault ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageInfo::loadFromPackage (FilePackage& package)
{
	IO::Stream* jsonStream = package.openStream (Meta::kPackageInfoFile);
	if(jsonStream == 0)
		return false;

	Deleter<IO::Stream> deleter (jsonStream);
	AttributePoolSuspender suspender; // don't allocate from memory pool
	return Archiver (jsonStream).load (*this);
}

//************************************************************************************************
// LicenseManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (LicenseManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr LicenseManager::kFolderName = "license_store";
CStringPtr LicenseManager::kUserFileName = "user.license";
CStringPtr LicenseManager::kExtensionFileName = "extensions.license";

//////////////////////////////////////////////////////////////////////////////////////////////////

LicenseManager::LicenseManager ()
: policy (AttributeAllocator::getDefault ()),
  systemKey ("systemid"),
  useBinaryFormat (false),
  signatureVerifier (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LicenseManager::~LicenseManager ()
{
	delete signatureVerifier;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LicenseManager::setPublicKey (const void* data, uint32 length, bool copy)
{
	IO::Buffer temp (const_cast<void*> (data), length, copy);
	publicKey.take (temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::decodeAndVerifyToken (Attributes& claims, CStringPtr token, int length) const
{
	struct Tokenizer
	{
		CStringPtr src;
		int remaining;

		Tokenizer (CStringPtr src, int length)
		: src (src),
		  remaining (length)
		{}

		bool next (IO::Buffer& buffer)
		{
			if(remaining > 0)
			{
				CStringPtr start = src;
				for(int i = 0; ; remaining--, src++, i++)
				{
					char c = remaining > 0 ? *src : 0;
					if(c == '.' || c == 0) 
					{
						IO::Buffer temp (const_cast<char*> (start), i, false);
						buffer.take (temp);
						if(c != 0)
						{
							remaining--;
							src++;
						}
						return true;
					}
				}
			}
			return false;
		}
	};

	/*
		JSON Web Signature Compact Serialization:

		BASE64URL(UTF8(JWS Protected Header)) || '.' ||
		BASE64URL(JWS Payload) || '.' ||
		BASE64URL(JWS Signature)
	*/

	Tokenizer tokenizer (token, length);
	IO::Buffer headerBase64, payloadBase64, signatureBase64;
	if(!(tokenizer.next (headerBase64) &&
		 tokenizer.next (payloadBase64) &&
		 tokenizer.next (signatureBase64)))
	   return false;

	// verify signature before loading payload
	if(signatureVerifier != 0)
	{
		IO::MemoryStream headerPlain;
		if(!Base64URLDecoder ().decodeBuffer (headerPlain, headerBase64))
		   return false;

		Attributes protectedHeader (AttributeAllocator::getDefault ());
		if(!Archiver (&headerPlain).load (protectedHeader))
			return false;

		Security::JOSE::Algorithm algorithmId = Security::JOSE::getAlgorithm (protectedHeader.getString (Security::JOSE::kAlgorithm));
		if(algorithmId != Security::JOSE::kRS256)
			return false;

		int signingInputLength = ConstString (token).lastIndex ('.');
		ASSERT (signingInputLength > 0)
		IO::Buffer signingInput (const_cast<char*> (token), signingInputLength, false);

		IO::MemoryStream signaturePlain;
		if(!Base64URLDecoder ().decodeBuffer (signaturePlain, signatureBase64))
		   return false;

		IO::Buffer signaturePlainBuffer (const_cast<void*> (signaturePlain.getBuffer ().getAddress ()),
										 signaturePlain.getBytesWritten (), false);

		if(!signatureVerifier->verifySignature (signingInput, publicKey, signaturePlainBuffer))
			return false;
	}
		
	IO::MemoryStream payloadPlain;
	if(!Base64URLDecoder ().decodeBuffer (payloadPlain, payloadBase64))
	   return false;

	return Archiver (&payloadPlain).load (claims);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LicenseManager::restoreAll ()
{
	ASSERT (!licenseFolder.isEmpty ())
	AttributePoolSuspender suspender; // don't allocate from memory pool

	// user token
	FileName tokenFileName (licenseFolder);
	tokenFileName.descend (kUserFileName);
	if(IO::MemoryStream* s = FileUtils::loadFile (tokenFileName))
	{
		setUserTokenInternal (s->getBuffer ().as<char> (), static_cast<int> (s->getBytesWritten ()));
		delete s;
	}

	// don't load any tokens if not registered to a user
	if(userIdentity.isEmpty ())
		return;

	// extension tokens
	tokenFileName = licenseFolder;
	tokenFileName.descend (kExtensionFileName);
	if(IO::MemoryStream* s = FileUtils::loadFile (tokenFileName))
	{
		Attributes tokenAttr (AttributeAllocator::getDefault ());
		if(isUseBinaryFormat ())
			Archiver::loadInplace (tokenAttr, const_cast<IO::Buffer&> (s->getBuffer ()), Archiver::kUBJSON);
		else
			Archiver (s, Archiver::kJSON).load (tokenAttr);

		if(const AttributeQueue* tokenArray = tokenAttr.getQueue (0))
			loadExtensionTokens (*tokenArray);
		delete s;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::saveToken (CStringPtr fileName, CStringPtr token, int length)
{
	ASSERT (!licenseFolder.isEmpty ())

	// make sure license folder exists
	if(!FileUtils::fileExists (licenseFolder))
		FileUtils::makeDirectory (licenseFolder);

	FileStream file;
	FileName tokenFileName (licenseFolder);
	tokenFileName.descend (fileName);
	return	file.create (tokenFileName) &&
			file.writeBytes (token, length) == length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::setUserToken (CStringPtr token, int length)
{
	return	setUserTokenInternal (token, length) &&
			saveToken (kUserFileName, token, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::setUserTokenInternal (CStringPtr token, int length)
{
	Attributes claims (AttributeAllocator::getDefault ());
	if(!decodeAndVerifyToken (claims, token, length))
		return false;

	// JWT subject is the system we are running on, audience is the user
	ConstString systemId (claims.getString (Security::JOSE::kSubject));
	ConstString userId (claims.getString (Security::JOSE::kAudience));
	ConstString userName (claims.getString ("name"));

	ASSERT (!systemIdentity.isEmpty ())
	if(systemId != systemIdentity)
		return false;

	setUserIdentity (userId);
	setUserDisplayName (userName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::getSystemFromUserToken (CString32& systemId, CStringPtr token, int length) const
{
	Attributes claims (AttributeAllocator::getDefault ());
	if(!decodeAndVerifyToken (claims, token, length))
		return false;

	// JWT subject is the system we are running on
	systemId = claims.getString (Security::JOSE::kSubject);
	return !systemId.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LicenseManager::removeUserToken ()
{
	setUserIdentity ("");
	setUserDisplayName ("");

	FileName userFileName (licenseFolder);
	userFileName.descend (kUserFileName);
	if(FileUtils::fileExists (userFileName))
		FileUtils::deleteFile (userFileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::setExtensionTokens (const AttributeQueue* tokenArray)
{
	ASSERT (!licenseFolder.isEmpty ())

	FileName tokenFileName (licenseFolder);
	tokenFileName.descend (kExtensionFileName);

	if(tokenArray == 0)
	{
		if(FileUtils::fileExists (tokenFileName))
			FileUtils::deleteFile (tokenFileName);
	}
	else
	{
		// make sure license folder exists
		if(!FileUtils::fileExists (licenseFolder))
			FileUtils::makeDirectory (licenseFolder);

		// rewrite extension token file, will be loaded on next start
		FileStream file;
		if(!file.create (tokenFileName))
			return false;

		if(isUseBinaryFormat ())
		{
			Text::Json::BinaryWriter writer (&file);
			OutputStorage storage (writer);
			saveExtensionTokens (storage, *tokenArray);
			return writer.getResult ();
		}
		else
		{
			Text::Json::Writer writer (&file);
			OutputStorage storage (writer);
			saveExtensionTokens (storage, *tokenArray);
			return writer.flush ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LicenseManager::saveExtensionTokens (OutputStorage& storage, const AttributeQueue& tokenArray) const
{
	AttributeHandler& writer = storage.getWriter ();
	writer.startArray (0);
	VectorForEachFast (tokenArray.getValues (), AttributeValue*, value)
		if(CStringPtr token = value->getString ())
			writer.setValue (0, token);
	EndFor
	writer.endArray (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LicenseManager::loadExtensionTokens (const AttributeQueue& tokenArray)
{
	ASSERT (!systemIdentity.isEmpty ())
	ASSERT (!userIdentity.isEmpty ())

	items.resize (tokenArray.getValues ().count ()); // avoid multiple reallocations
	VectorForEachFast (tokenArray.getValues (), AttributeValue*, value)
		if(CStringPtr token = value->getString ())
		{
			Attributes claims (AttributeAllocator::getDefault ());
			int tokenLength = ConstString (token).length ();
			if(!decodeAndVerifyToken (claims, token, tokenLength))
				continue;

			ConstString extensionId (claims.getString (Security::JOSE::kSubject));
			ASSERT (!extensionId.isEmpty ())
			if(extensionId.isEmpty ())
				continue;

			Item item (extensionId);
			ASSERT (item.id == extensionId) // check for truncation

			// token must have been issued for current user and system
			ConstString userId (claims.getString (Security::JOSE::kAudience));
			ConstString systemId (claims.getString (systemKey));
			item.state = userId == userIdentity && systemId == systemIdentity ? kValid : kInvalid;
			items.add (item);

			CORE_PRINTF ("Token for '%s' is %s\n", extensionId.str (), item.state == kValid ? "valid" : "invalid")

			if(item.state == kValid)
			{
				ConstString jsonPolicy (claims.getString ("policy"));
				if(!jsonPolicy.isEmpty ())
				{
					#if DEBUG
					if(jsonPolicy[0] != '<') // ignore XML
					#endif
						addToPolicy (jsonPolicy, jsonPolicy.length ());
				}
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LicenseManager::addToPolicy (CStringPtr jsonSnippet, int length)
{
	IO::MemoryStream jsonStream (const_cast<char*> (jsonSnippet), length);
	Attributes attr (AttributeAllocator::getDefault ());
	if(!Archiver (&jsonStream).load (attr))
		return false;

	AttributeQueue* dstArray = const_cast<AttributeQueue*> (policy.getQueue (AuthorizationPolicy::kChildren));
	if(dstArray == 0) // makeQueue() would clear the array otherwise!
		dstArray = policy.makeQueue (AuthorizationPolicy::kChildren);

	if(const AttributeQueue* srcArray = attr.getQueue (0))
		const_cast<AttributeQueue*> (srcArray)->moveTo (*dstArray);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LicenseManager::LicenseStatus LicenseManager::getLicenseStatus (CStringPtr _id) const
{
	ConstString id (_id);
	for(int i = 0; i < items.count (); i++)
		if(items[i].id == id)
			return items[i].state;
	return kNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes* LicenseManager::getPrivileges (CStringPtr resourceSid) const
{
	return AuthorizationPolicy::findItemOfType (policy, resourceSid, AuthorizationPolicy::kResource);
}

//************************************************************************************************
// ProductBundle
//************************************************************************************************

ProductBundle::~ProductBundle ()
{
	deleteAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProductBundle::deleteAll ()
{
	VectorForEachFast (*this, ProductItem*, p)
		delete p;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ProductBundle::loadFromPackage (FilePackage& package, CStringPtr baseFolder)
{
	IO::Stream* jsonStream = package.openStream (Meta::kProductBundleFile);
	if(jsonStream == 0)
		return false;

	Deleter<IO::Stream> deleter (jsonStream);
	Attributes a (AttributeAllocator::getDefault ());
	AttributePoolSuspender suspender; // don't allocate from memory pool
	if(!Archiver (jsonStream).load (a))
		return false;

	// check for indirection (used during development)
	ConstString includePath = a.getString (Meta::kInclude);
	if(!includePath.isEmpty ())
	{
		FileName fileName (includePath);
		fileName.makeAbsolute (baseFolder);

		jsonStream = FileUtils::loadFile (fileName);
		if(jsonStream == 0)
			return false;

		Deleter<IO::Stream> deleter2 (jsonStream);
		a.removeAll ();
		if(!Archiver (jsonStream).load (a))
			return false;	
	}

	if(const AttributeQueue* productArray = a.getQueue (0))
		VectorForEachFast (productArray->getValues (), AttributeValue*, value)
			if(const Attributes* productAttr = value->getAttributes ())
			{
				ProductItem* item = NEW ProductItem (productAttr->getString (Meta::kID), 
													 productAttr->getString (Meta::kName));

				if(const AttributeQueue* uidArray = productAttr->getQueue (Meta::kCID))
					VectorForEachFast (uidArray->getValues (), AttributeValue*, value)
						if(CStringPtr string = value->getString ())
						{
							UIDBytes uid = {0};
							uid.fromCString (string);
							if(!item->addClassID (uid))
								break;
						}
					EndFor

				add (item);
			}
		EndFor
	return true;
}

//************************************************************************************************
// ExtensionDescription
//************************************************************************************************

ExtensionDescription::ExtensionDescription (FilePackage& package, PackageInfo& info)
: useCount (0),
  package (package),
  info (info),
  products (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription::~ExtensionDescription ()
{
	VectorForEachFast (subPackages, FilePackage*, p)
		delete p;
	EndFor
	subPackages.removeAll ();
	delete &package;
	delete &info;
	delete products;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionDescription::addSubPackage (FilePackage* package)
{
	return subPackages.add (package);
}

//************************************************************************************************
// ExtensionManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (ExtensionManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ExtensionManager::kFolderName = "extensions";
CStringPtr ExtensionManager::kFileType = ".install";
CStringPtr ExtensionManager::kUpdateFileType = ".update";
CStringPtr ExtensionManager::kDeleteMarker = "delete_marker";

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionManager::ExtensionManager ()
: restartRequired (false),
  rescanning (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionManager::~ExtensionManager ()
{
	VectorForEachFast (extensions, ExtensionDescription*, e)
		delete e;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::scanAll ()
{
	ASSERT (!extensionFolder.isEmpty ())

	if(isDeletePending ())
	{
		FileIterator iter (extensionFolder);
		const FileIterator::Entry* entry = 0;
		while((entry = iter.next ()) != 0)
		{
			if(entry->directory) // ignore directories (used for development only)
				continue;

			if(entry->name.endsWith (kFileType) || entry->name.endsWith (kUpdateFileType))
				FileUtils::deleteFile (entry->name);
		}

		setDeleteOnStart (false);
	}
	else
	{
		scanFolder (extensionFolder);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::rescanAll ()
{
	ScopedVar<bool> scope (rescanning, true);
	scanAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::setDeleteOnStart (bool state)
{
	FileName markerFile (extensionFolder);
	markerFile.descend (kDeleteMarker);
	if(state)
	{
		FileStream file;
		file.create (markerFile);
	}
	else
	{
		if(FileUtils::fileExists (markerFile))
			FileUtils::deleteFile (markerFile);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::isDeletePending () const
{
	FileName markerFile (extensionFolder);
	markerFile.descend (kDeleteMarker);
	return FileUtils::fileExists (markerFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::hasFile (CStringPtr fileName) const
{
	VectorForEachFast (extensions, ExtensionDescription*, e)
		if(e->getFileName ().compare (fileName, false) == 0)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::makeUpdateFile (FileName& fileName) const
{
	fileName.setExtension (kUpdateFileType + 1); // excl. "."
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::scanFolder (CStringPtr folder)
{
	// build list of extension files/folders
	Vector<FileIterator::Entry> fileList;
	{
		FileIterator iter (folder);
		const FileIterator::Entry* entry = 0;
		while((entry = iter.next ()) != 0)
		{
			// ignore if scanned already
			if(rescanning && hasFile (entry->name))
				continue;
		
			if(entry->directory)
			{
				FileName fileName (entry->name);
				fileName.descend (Meta::kPackageInfoFile);
				if(FileUtils::fileExists (fileName))
					fileList.add (*entry);
			}
			else
			{
				if(entry->name.endsWith (kFileType))
					fileList.add (*entry);
			}
		}
	}

	// mount packages and handle updates
	for(int i = 0; i < fileList.count (); i++)
	{
		const FileIterator::Entry* entry = &fileList[i];
		
		FilePackage* package = 0;
		if(entry->directory)
		{
			package = NEW FolderPackage (entry->name, true); // buffered mode enabled
		}
		else
		{
			// check if update file exists
			FileName updateFile (entry->name);
			makeUpdateFile (updateFile);
			if(FileUtils::fileExists (updateFile))
			{
				FileUtils::deleteFile (entry->name);
				FileUtils::renameFile (updateFile, entry->name);
			}

			ZipPackage* zipPackage = NEW ZipPackage;
			if(zipPackage->openFromFile (entry->name))
				package = zipPackage;
			else
				delete zipPackage;
		}

		if(package != 0)
		{
			CStringPtr baseFolder = entry->directory ? entry->name.str () : folder;
			if(ExtensionDescription* e = scanPackage (*package, baseFolder))
			{
				e->setFileName (entry->name);
				extensions.add (e);
			}
			else
				delete package;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionManager::addMemoryFile (const void* data, uint32 size, CStringPtr fileName)
{
	ZipPackage* zipPackage = NEW ZipPackage;
	if(zipPackage->openFromMemory (data, size))
	{
		if(ExtensionDescription* e = scanPackage (*zipPackage, extensionFolder))
		{
			e->setFileName (fileName);
			extensions.add (e);
			return true;
		}
	}

	delete zipPackage;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionDescription* ExtensionManager::scanPackage (FilePackage& package, CStringPtr baseFolder)
{
	PackageInfo* packageInfo = NEW PackageInfo;
	Deleter<PackageInfo> packageInfoDeleter (packageInfo);
	if(!packageInfo->loadFromPackage (package))
		return 0;

	ConstString id (packageInfo->getID ());
	if(id.isEmpty ())
		return 0;

	// check for duplicate
	const ExtensionDescription* existing = findExtension (id);
	ASSERT (existing == 0 || rescanning == true)
	if(existing != 0)
		return 0;

	packageInfoDeleter._ptr = 0;
	ExtensionDescription* e = NEW ExtensionDescription (package, *packageInfo);

	// check for revision file (compatible to CCL implementation)
	FileName revFile ("revision.properties");
	if(package.fileExists (revFile))
	{
		if(IO::Stream* s = package.openStream (revFile))
		{
			CString256 buffer;
			int bytesRead = s->readBytes (buffer.getBuffer (), buffer.getSize ());
			if(bytesRead > 0)			
			{
				buffer.truncate (bytesRead);
				CString256 revisionString;
				buffer.subString (revisionString, buffer.index ("=") + 1);
				revisionString.trimWhitespace ();
				if(!revisionString.isEmpty ())
				{
					CString256 versionString (packageInfo->getVersion ());
					versionString += ".";
					versionString += revisionString;
					packageInfo->set (Meta::kVersion, versionString);
				}
			}

			delete s;
		}
	}

	// check for products
	ProductBundle* products = NEW ProductBundle;
	if(products->loadFromPackage (package, baseFolder))
		e->setProducts (products);
	else
		delete products;

	return e;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ExtensionDescription* ExtensionManager::findExtension (CStringPtr _id) const
{
	ConstString id (_id);
	VectorForEachFast (extensions, ExtensionDescription*, e)
		if(id == e->getInfo ().getID ())
			return e;
	EndFor
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::startupForHandler (IExtensionHandler& handler)
{
	VectorForEachFast (extensions, ExtensionDescription*, e)
		handler.startupExtension (*e);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::saveIndex (IO::Stream& stream, bool useBinaryFormat) const
{
	if(useBinaryFormat)
	{
		Text::Json::BinaryWriter writer (&stream);
		OutputStorage storage (writer);
		saveIndex (storage);
	}
	else
	{
		Text::Json::Writer writer (&stream);
		OutputStorage storage (writer);
		saveIndex (storage);
		writer.flush ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionManager::saveIndex (OutputStorage& storage) const
{
	bool deletePending = isDeletePending ();
	AttributeHandler& writer = storage.getWriter ();
	const int stringFlags = Attribute::kShareID|Attribute::kShareValue;
	writer.startArray (0);
	VectorForEachFast (extensions, ExtensionDescription*, e)
		writer.startObject (0);
		writer.setValue (Meta::kID, e->getInfo ().getID (), stringFlags);
		writer.setValue (Meta::kName, e->getInfo ().getName (), stringFlags);
		writer.setValue (Meta::kVersion, e->getInfo ().getVersion (), stringFlags);
		if(const ProductBundle* productBundle = e->getProducts ())
		{
			writer.startArray (Meta::kProductsArray, Attribute::kShareID);
			VectorForEachFast (*productBundle, ProductItem*, p)
				writer.startObject (0);
				writer.setValue (Meta::kID, p->getID (), stringFlags);
				writer.setValue (Meta::kName, p->getName (), stringFlags);

				ConstString statusString;
				if(deletePending)
					statusString = Meta::kStatusDeleted;
				else
				{
					LicenseManager::LicenseStatus status = LicenseManager::instance ().getLicenseStatus (p->getID ());
					if(status == LicenseManager::kValid)
						statusString = Meta::kStatusActivated;
				}
				
				if(!statusString.isEmpty ())
					writer.setValue (Meta::kStatus, statusString, stringFlags);

				writer.endObject (0);
			EndFor
			writer.endArray (Meta::kProductsArray, Attribute::kShareID);
		}
		writer.endObject (0);
	EndFor
	writer.endArray (0);
}

//************************************************************************************************
// ExtensionPluginHandler
//************************************************************************************************

CStringPtr ExtensionPluginHandler::kFolderName = "plugins";
CStringPtr ExtensionPluginHandler::kBuiltInFileType = ".pslib";

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionPluginHandler::~ExtensionPluginHandler ()
{
	VectorForEachFast (availableResources, BuiltInCodeResource*, c)
		delete c;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPluginHandler::addAvailableResource (BuiltInCodeResource* codeResource)
{
	availableResources.add (codeResource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPluginHandler::startupExtension (ExtensionDescription& e)
{
	ASSERT (!platformSubFolder.isEmpty ())
	for(int i = 0; i < availableResources.count (); i++)
	{
		BuiltInCodeResource* c = availableResources[i];
		FileName fileName (kFolderName);
		fileName.descend (platformSubFolder);
		fileName.descend (c->getResourceName ());
		fileName += kBuiltInFileType;
		if(e.getPackage ().fileExists (fileName) == true)
		{
			PluginManager::instance ().addCodeResource (c);
			availableResources.removeAt (i);
			i--;
		}
	}
}

//************************************************************************************************
// ExtensionSkinHandler
//************************************************************************************************

CStringPtr ExtensionSkinHandler::kFolderName = "skin";

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionSkinHandler::ExtensionSkinHandler ()
: delayBitmapDecoding (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionSkinHandler::startupExtension (ExtensionDescription& e)
{
	FileName fileName1 (kFolderName);
	fileName1.descend (Skin::FileNames::kViewFile1);
	FileName fileName2 (kFolderName);
	fileName2.descend (Skin::FileNames::kViewFile2);
	if(!(e.getPackage ().fileExists (fileName1) || e.getPackage ().fileExists (fileName2)))
		return;
	
	FilePackage* skinPackage = NEW SubPackage (e.getPackage (), kFolderName);
	e.addSubPackage (skinPackage);
	e.setUseCount (e.getUseCount () + 1);

	BitmapManager::instance ().loadBitmaps (*skinPackage, isDelayBitmapDecoding ());
	FontManager::instance ().loadFonts (*skinPackage);
	StyleManager::instance ().loadStyles (*skinPackage);
	ViewBuilder::instance ().loadViews (*skinPackage);
}
