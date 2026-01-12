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
// Filename    : ccl/base/security/packagesignature.cpp
// Description : Package Digital Signature
//
//************************************************************************************************

#define DEBUG_LOG 1

#include "ccl/base/security/packagesignature.h"
#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/jsonwebsecurity.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Security;
using namespace Crypto;

//************************************************************************************************
// Crypto::PackageSignature
//************************************************************************************************

const String PackageSignature::kFileName ("signature.xml");

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PackageSignature, SignedXmlMessage)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageSignature::PackageSignature ()
: SignedXmlMessage ("PackageSignature")
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageSignature::setParts (const PartList& partList)
{
	setDataWithObject (partList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::getParts (PartList& partList)
{
	return getObjectFromData (partList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::loadFromPackage (IPackageFile& package)
{
	IFileSystem* fileSystem = package.getFileSystem ();
	ASSERT (fileSystem != nullptr)
	if(!fileSystem)
		return false;

	ArchiveHandler handler (*fileSystem);
	return loadFromHandler (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::saveWithPackage (IPackageFile& package) const
{
	IFileSystem* fileSystem = package.getFileSystem ();
	ASSERT (fileSystem != nullptr)
	if(!fileSystem)
		return false;

	AutoPtr<ArchiveHandler> handler = NEW ArchiveHandler (*fileSystem);
	return saveWithHandler (*handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::loadFromHandler (ArchiveHandler& handler)
{
	 return handler.loadStream (kFileName, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::saveWithHandler (ArchiveHandler& handler) const
{
	return handler.addSaveTask (kFileName, const_cast<PackageSignature&> (*this));
}

//************************************************************************************************
// Crypto::PackageSigner
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageSigner, Signer)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSigner::sign (UrlRef outPath, UrlRef inPath, IProgressNotify* progress)
{
	AutoPtr<IPackageFile> inFile = System::GetPackageHandler ().openPackage (inPath);
	if(!inFile)
		return false;
	ASSERT (inFile->getFileSystem ())

	// build part list
	PackageSignature::Builder builder;
	if(!builder.build (*inFile, progress))
		return false;

	// sign part list
	AutoPtr<PackageSignature> signature = NEW PackageSignature;
	signature->setParts (builder.getParts ());
	if(!sign (*signature))
		return false;

	UIDRef format = UnknownPtr<IObject> (inFile)->getTypeInfo ().getClassID ();
	AutoPtr<IPackageFile> outFile = System::GetPackageHandler ().createPackage (outPath, format);
	if(!outFile)
		return false;

	// copy options
	Variant formatVersion, reservedBlockSize, compressed;
	inFile->getOption (formatVersion, PackageOption::kFormatVersion);
	inFile->getOption (reservedBlockSize, PackageOption::kReservedBlockSize);
	inFile->getOption (compressed, PackageOption::kCompressed);
	outFile->setOption (PackageOption::kFormatVersion, formatVersion);
	outFile->setOption (PackageOption::kReservedBlockSize, reservedBlockSize);
	outFile->setOption (PackageOption::kCompressed, compressed);

	if(!outFile->create ())
		return false;

	ASSERT (outFile->getFileSystem ())
	ArchiveHandler handler (*outFile->getFileSystem ());

	// add signature file
	signature->saveWithHandler (handler);

	// copy parts
	ForEach (builder.getParts (), PackageSignature::Part, part)
		Url path;
		path.setPath (part->getFileName (), Url::kDetect);
		if(path.isFolder ()) // ignore directories
			continue;

		AutoPtr<IStream> inStream = inFile->getFileSystem ()->openStream (path);
		ASSERT (inStream)
		if(!inStream)
			return false;

		FileInfo fileInfo;
		inFile->getFileSystem ()->getFileInfo (fileInfo, path);

		handler.addSaveTask (part->getFileName (), *inStream, &fileInfo.flags); // <-- keep original file attributes (compression)
	EndFor

	return outFile->flush (progress) != 0;
}

//************************************************************************************************
// Crypto::PackageVerifierOptions
//************************************************************************************************

PackageVerifierOptions::PackageVerifierOptions ()
: loggingEnabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVerifierOptions::setOptions (const PackageVerifierOptions& other)
{
	setLoggingEnabled (other.isLoggingEnabled ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVerifierOptions::logError (StringRef message, StringRef packageId)
{
	if(isLoggingEnabled ())
	{
		System::DebugReportWarning (System::GetCurrentModuleRef (), 
									String () << message << ": " << packageId);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVerifierOptions::logError (StringRef message, UrlRef path)
{
	logError (message, UrlDisplayString (path));
}

//************************************************************************************************
// Crypto::PackageVerifier
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageVerifier, Verifier)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVerifier::verify (UrlRef path, IUrlFilter* filter, IProgressNotify* progress)
{
	AutoPtr<IPackageFile> package = System::GetPackageHandler ().openPackage (path);
	if(!package)
	{
		logError ("Failed to open package", path);
		return false;
	}

	// load and verify signature
	PackageSignature signature;
	if(!signature.loadFromPackage (*package))
	{
		logError ("Failed to load signature from package", path);
		return false;
	}

	// check for embedded key info used with vendor signing
	if(!signature.getKeyInfo ().isEmpty ())
	{		
		PackageInfo metaInfo;
		if(!metaInfo.loadFromPackage (*package))
		{
			logError ("Failed to load meta info from package", path);
			return false;
		}

		PackageVendorSignature vendorSignature;
		vendorSignature.setOptions (*this);
		if(!vendorSignature.verify (*this, signature, metaInfo))
			return false;

		// automatically remember known vendors for later use
		PackageVendorStore::instance ().addOnce (vendorSignature);
	}
	else
	{
		if(!verify (signature))
		{
			logError ("Signature does not match for public key", path);
			return false;
		}
	}

	// compare loaded with calculated parts
	PackageSignature::PartList loadedParts;
	if(!signature.getParts (loadedParts))
	{
		logError ("Failed to load signature parts from package", path);
		return false;
	}

	#if DEBUG_LOG
	loadedParts.dump ();
	#endif

	PackageSignature::Builder builder;
	builder.setFilter (filter);
	if(!builder.build (*package, progress))
	{
		logError ("Failed to build signature parts for package", path);
		return false;
	}

	#if DEBUG_LOG
	builder.getParts ().dump ();
	#endif

	if(!builder.getParts ().equals (loadedParts))
	{
		logError ("Signature parts not equal for package", path);
		return false;
	}
	return true;
}

//************************************************************************************************
// Crypto::PackageSignature::Part
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageSignature::Part, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

int PackageSignature::Part::compare (const Object& obj) const
{
	return fileName.compare (((const Part&)obj).fileName);
}

//************************************************************************************************
// Crypto::PackageSignature::PartList
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageSignature::PartList, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageSignature::PartList::PartList ()
{
	parts.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageSignature::PartList::addPart (Part* part)
{
	parts.addSorted (part);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PackageSignature::PartList::newIterator () const
{
	return parts.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PackageSignature::Part* PackageSignature::PartList::findPart (StringRef fileName) const
{
	ArrayForEach (parts, Part, part)
		if(part->getFileName () == fileName)
			return part;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageSignature::PartList::dump () const
{
	Debugger::printf ("== PackageSignature parts (%d) ===\n", parts.count ());
	int i = 0;
	ArrayForEach (parts, Part, part)
		Debugger::println (String () << i << " filename: \"" << part->getFileName () << "\" SHA1: " << part->getDigestSHA1 ().toBase64 ());
		i++;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::PartList::equals (const Object& obj) const
{
	const PartList* other = ccl_cast<PartList> (&obj);
	ASSERT (other != nullptr)
	if(other == nullptr)
		return false;

	if(parts.count () != other->parts.count ())
		return false;

	for(int i = 0; i < parts.count (); i++)
	{
		Part* p1 = (Part*)parts.at (i);
		const Part* p2 = other->findPart (p1->getFileName ());
		if(p2 == nullptr)
			return false;
		if(!p2->getDigestSHA1 ().equals (p1->getDigestSHA1 ()))
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageSignature::PartList::save (IStream& stream) const
{
	AutoPtr<ITextStreamer> writer = System::CreateTextStreamer (stream, {Text::kUTF8, Text::kSystemLineFormat, ITextStreamer::kSuppressByteOrderMark});
	for(int i = 0; i < parts.count (); i++)
	{
		Part* p = (Part*)parts.at (i);

		if(i > 0)
			writer->writeNewline ();

		String line;
		line << p->getFileName ();
		if(!p->getDigestSHA1 ().isEmpty ())
			line << ": " << p->getDigestSHA1 ().toBase64 ();
		writer->writeString (line);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageSignature::PartList::load (IStream& stream)
{
	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream, {Text::kUTF8});
	while(!reader->isEndOfStream ())
	{
		String line;
		reader->readLine (line);
		line.trimWhitespace ();
		if(line.isEmpty ())
			continue;

		String fileName, digest;
		int index = line.lastIndex (":");
		if(index != -1)
		{
			fileName = line.subString (0, index);
			digest = line.subString (index + 1);
			digest.trimWhitespace ();
		}
		else
			fileName = line;

		Part* p = NEW Part;
		p->setFileName (fileName);
		if(!digest.isEmpty ())
			p->setDigestSHA1 (Crypto::Material ().fromBase64 (digest));
		addPart (p);
	}
	return true;
}

//************************************************************************************************
// Crypto::PackageSignature::Builder
//************************************************************************************************

PackageSignature::Builder::Builder ()
: filter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::Builder::build (UrlRef path, IProgressNotify* progress)
{
	AutoPtr<IPackageFile> p = System::GetPackageHandler ().openPackage (path);
	if(p == nullptr)
		return false;

	return build (*p, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::Builder::build (IPackageFile& package, IProgressNotify* progress)
{
	IFileSystem* fs = package.getFileSystem ();
	ASSERT (fs != nullptr)
	if(fs == nullptr)
		return false;

	return build (*fs, Url (), progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageSignature::Builder::build (IFileSystem& fs, UrlRef folder, IProgressNotify* progress)
{
	ForEachFile (fs.newIterator (folder), path)
		if(path->isFile ())
		{
			String fileName (path->getPath ());
			if(fileName == PackageSignature::kFileName) // ignore signature file itself
				continue;

			if(filter && !filter->matches (*path)) // check filter
				continue;

			if(progress)
			{
				progress->updateAnimated (fileName);
				if(progress->isCanceled ())
					return false;
			}

			// calculate SHA-1 digest
			AutoPtr<IStream> stream = fs.openStream (*path);
			if(!stream)
				return false;

			Material digest (SHA1::kDigestSize);
			AutoPtr<IStream> bufferedStream = System::GetFileUtilities ().createBufferedStream (*stream);
			if(!SHA1::calculate (digest, *bufferedStream, progress))
				return false;

			// add part to signature
			Part* part = NEW Part;
			part->setFileName (fileName);
			part->setDigestSHA1 (digest);
			partList.addPart (part);
		}
		else
		{
			Part* part = NEW Part;
			part->setFileName (String () << path->getPath () << Url::strPathChar);
			partList.addPart (part);

			if(!build (fs, *path, progress))
				return false;
		}
	EndFor
	return true;
}

//************************************************************************************************
// Crypto::PackageVendorSigningAuthority
//************************************************************************************************

bool PackageVendorSigningAuthority::loadPrivateParentKey (UrlRef path)
{
	AutoPtr<IMemoryStream> ms = File::loadBinaryFile (path);
	if(!ms)
		return false;

	privateParentKey.copyFrom (*ms);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageVendorSigningAuthority::createPublicVendorToken (const Material& publicKey, StringRef vendorName)
{
	ASSERT (!vendorName.isEmpty () && !parentKeyId.isEmpty () && !privateParentKey.isEmpty ())
	if(vendorName.isEmpty () || parentKeyId.isEmpty () || privateParentKey.isEmpty ())
		return String ();

	JWTObject jwt;
	jwt.getClaims ().set (PackageVendorSignature::kVendor, vendorName);
	jwt.getClaims ().set (PackageVendorSignature::kPublicKey, publicKey.toBase64 ());
	jwt.setIssuedAt (UnixTime::getTime ());

	JWSObject jws;
	jws.setType (JOSE::kJWT);
	jws.setAlgorithm (JOSE::kRS256);
	jws.getProtectedHeader ().set (PackageVendorSignature::kParentKeyID, parentKeyId);
	jwt.toSignature (jws);
	jws.sign (privateParentKey);

	return jws.toCompactSerialization ();
}

//************************************************************************************************
// Crypto::PackageVendorSignature
//************************************************************************************************

IPackageVendorSigningAuthority* PackageVendorSignature::globalAuthority = nullptr;
void PackageVendorSignature::setGlobalAuthority (IPackageVendorSigningAuthority* authority) { globalAuthority = authority; }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PackageVendorSignature, Object)
DEFINE_CLASS_UID (PackageVendorSignature, 0xe8717da2, 0x8d11, 0x4928, 0xb7, 0x7d, 0xf2, 0x50, 0x23, 0x82, 0x4f, 0x83)
DEFINE_CLASS_NAMESPACE (PackageVendorSignature, "Host") // class can be registered by host application

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (PackageVendorSignature, kVendor, "vendor")
DEFINE_STRINGID_MEMBER_ (PackageVendorSignature, kPrivateKey, "privatekey")
DEFINE_STRINGID_MEMBER_ (PackageVendorSignature, kPublicKey, "publickey")
DEFINE_STRINGID_MEMBER_ (PackageVendorSignature, kPublicToken, "publictoken")
DEFINE_STRINGID_MEMBER_ (PackageVendorSignature, kParentKeyID, "parentkeyid")

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageVendorSignature::PackageVendorSignature (UsageHint usage)
: usage (usage)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageVendorSignature::createPrivateTokenWithGlobalAuthority ()
{
	ASSERT (globalAuthority)
	if(!globalAuthority)
		return String ();

	return createPrivateToken (*globalAuthority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageVendorSignature::createPrivateTokenWithParentKey (UrlRef privateParentKeyPath, StringID parentKeyId)
{
	PackageVendorSigningAuthority authority;
	authority.setParentKeyID (parentKeyId);
	if(!authority.loadPrivateParentKey (privateParentKeyPath))
		return String ();

	return createPrivateToken (authority);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageVendorSignature::createPrivateToken (IPackageVendorSigningAuthority& authority)
{
	ASSERT (!vendorName.isEmpty ())
	if(vendorName.isEmpty ())
		return String ();
	
	// generate new key pair for vendor
	RSA::generateKeyPair (privateKey, publicKey);

	// create public vendor token signed by authority
	serializedPublicToken = authority.createPublicVendorToken (publicKey, vendorName);
	if(serializedPublicToken.isEmpty ())
		return String ();
	
	// create private vendor token incl. private key
	return serializePrivateToken ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String PackageVendorSignature::serializePrivateToken () const
{
	ASSERT (!vendorName.isEmpty ())
	ASSERT (!privateKey.isEmpty ())
	ASSERT (!serializedPublicToken.isEmpty ())

	JWTObject jwt;
	jwt.getClaims ().set (kVendor, vendorName);
	jwt.getClaims ().set (kPrivateKey, privateKey.toBase64 ());
	jwt.getClaims ().set (kPublicToken, serializedPublicToken);	
	jwt.setIssuedAt (UnixTime::getTime ());

	JWSObject jws;
	jws.setType (JOSE::kJWT);
	jwt.toSignature (jws); // private token isn't signed
		
	return jws.toCompactSerialization ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::loadPrivateToken (UrlRef tokenPath)
{
	String token = TextUtils::loadString (tokenPath, String::kEmpty);
	return loadPrivateToken (token);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::loadPrivateToken (StringRef token)
{
	{
		JWTObject jwt;
		if(!loadToken (jwt, token, false)) // don't verify: private token isn't signed
			return false;

		vendorName = jwt.getClaims ().getString (kVendor);
		privateKey.fromBase64 (jwt.getClaims ().getString (kPrivateKey));
		serializedPublicToken = jwt.getClaims ().getString (kPublicToken);
	}

	{
		JWTObject jwt;
		if(!loadToken (jwt, serializedPublicToken, usage != kToolUsage)) // verify for regular use, but signing tools might not know parent key
			return false;

		publicKey.fromBase64 (jwt.getClaims ().getString (kPublicKey));
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::loadPublicToken (UrlRef tokenPath)
{
	String token = TextUtils::loadString (tokenPath, String::kEmpty);
	return loadPublicToken (token);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::loadPublicToken (StringRef token)
{
	{
		JWTObject jwt;
		if(!loadToken (jwt, token, usage != kToolUsage)) // verify for regular use, but signing tools might not know parent key
			return false;

		vendorName = jwt.getClaims ().getString (kVendor);
		publicKey.fromBase64 (jwt.getClaims ().getString (kPublicKey));
	}

	serializedPublicToken = token;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::loadToken (JWTObject& jwt, StringRef token, bool verify)
{
	JWSObject jws;
	jws.fromCompactSerialization (token);
	if(!jws.isJWT ())
		return false;

	if(verify)
	{
		if(!jws.isKnownAlgorithm ())
			return false;

		// verify token signature with parent key
		MutableCString parentKeyId = jws.getProtectedHeader ().getCString (kParentKeyID);
		if(parentKeyId.isEmpty ())
		{
			parentKeyId = jws.getKeyID (); // fall back to standard key id header
			if(parentKeyId.isEmpty ())
				return false;
		}

		Verifier verifier;
		if(!verifier.setFromKeyStore (parentKeyId))
			return false;
		if(!jws.verify (verifier.getPublicKey ()))
			return false;
	}

	jwt.fromSignature (jws);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::sign (UrlRef outPath, UrlRef inPath, IProgressNotify* progress)
{
	ASSERT (!privateKey.isEmpty () && !serializedPublicToken.isEmpty ())
	if(privateKey.isEmpty () || serializedPublicToken.isEmpty ())
		return false;

	CCL_PRINTLN ("PackageVendorSignature::sign:")
	CCL_PRINTLN (UrlFullString (outPath))
	CCL_PRINTLN (UrlFullString (inPath))
	
	PackageSigner signer;
	signer.setKeyInfo (Material ().append (serializedPublicToken, Text::kASCII));
	signer.setPrivateKey (privateKey);
	return signer.sign (outPath, inPath, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::verifyVendor (PackageSignature& signature, const PackageInfo& info)
{
	String tokenString;
	signature.getKeyInfo ().copyTo (tokenString, Text::kASCII);
	if(!loadPublicToken (tokenString))
	{
		logError ("Failed to load public token from signature key info for package", info.getPackageID ());
		return false;
	}

	// check for matching vendor name
	String packageVendor = info.getString (Meta::kPackageVendor);
	if(packageVendor != vendorName)
	{
		logError (String () << "Package vendor name does not match with public token (expected: '" <<
				  vendorName << "' actual: '" << packageVendor << "')",
				  info.getPackageID ());
		return false;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::verify (PackageVerifier& verifier, PackageSignature& signature, const PackageInfo& info)
{
	if(!verifyVendor (signature, info))
		return false;

	verifier.setPublicKey (publicKey);
	if(!verifier.verify (signature))
	{
		logError ("Signature does not match for public key", info.getPackageID ());
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::load (const Storage& storage)
{
	const Attributes& a = storage.getAttributes ();
	String savedToken = a.getString ("publicToken");
	return loadPublicToken (savedToken); // this loads and verifies the public token
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorSignature::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	ASSERT (!serializedPublicToken.isEmpty ())
	a.set ("publicToken", serializedPublicToken);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (PackageVendorSignature)
	DEFINE_PROPERTY_TYPE (PackageVendorSignature::kVendor, ITypeInfo::kString)
END_PROPERTY_NAMES (PackageVendorSignature)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageVendorSignature::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kVendor)
	{
		var = vendorName;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageVendorSignature::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == kVendor)
	{
		vendorName = var.asString ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PackageVendorSignature)
	DEFINE_METHOD_ARGR ("createPrivateTokenWithGlobalAuthority", "", "string")
	DEFINE_METHOD_ARGR ("createPrivateTokenWithParentKey", "privateParentKey: Url, parentKeyId: string", "string")
	DEFINE_METHOD_ARGR ("loadPrivateToken", "tokenPath: Url", "string")
	DEFINE_METHOD_ARGR ("sign", "outPath: Url, inPath: Url, progress: Object = null", "bool")
END_METHOD_NAMES (PackageVendorSignature)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PackageVendorSignature::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "createPrivateTokenWithGlobalAuthority")
	{
		String privateToken = createPrivateTokenWithGlobalAuthority ();
		returnValue = privateToken;
		returnValue.share ();
		return true;
	}
	else if(msg == "createPrivateTokenWithParentKey")
	{
		UnknownPtr<IUrl> keyPath (msg[0].asUnknown ());	
		MutableCString parentKeyId (msg[1].asString ());
		String privateToken = keyPath ? createPrivateTokenWithParentKey (*keyPath, parentKeyId) : String::kEmpty;
		returnValue = privateToken;
		returnValue.share ();
		return true;
	}
	else if(msg == "loadPrivateToken")
	{
		UnknownPtr<IUrl> tokenPath (msg[0].asUnknown ());
		bool succeeded = tokenPath ? loadPrivateToken (*tokenPath) : false;
		returnValue = succeeded;
		return true;
	}
	else if(msg == "sign")
	{
		UnknownPtr<IUrl> outPath (msg[0].asUnknown ());
		UnknownPtr<IUrl> inPath (msg[1].asUnknown ());
		UnknownPtr<IProgressNotify> progress (msg.getArgCount () > 2 ? msg[2].asUnknown () : nullptr);
		bool succeeded = outPath && inPath && sign (*outPath, *inPath, progress);
		returnValue = succeeded;
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// Crypto::PackageVendorStore
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PackageVendorStore, StorableObject)
DEFINE_SINGLETON (PackageVendorStore)
const String PackageVendorStore::kFileName ("PackageVendorStore.xml");

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageVendorStore::PackageVendorStore ()
{
	vendors.objectCleanup (true);

	restore (); // restore on first use
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVendorStore::getLocation (IUrl& path) const
{
	System::GetSystem ().getLocation (path, System::kAppSettingsFolder);
	path.descend (kFileName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PackageVendorSignature* PackageVendorStore::find (StringRef vendorName) const
{
	return static_cast<PackageVendorSignature*> (vendors.findIf ([vendorName] (const Object* object) 
		{
			return static_cast<const PackageVendorSignature*> (object)->getVendorName () == vendorName;
		}));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PackageVendorStore::addOnce (const PackageVendorSignature& vendor)
{
	ASSERT (!vendor.getVendorName ().isEmpty ())
	ASSERT (!vendor.getSerializedPublicToken ().isEmpty ())
	ASSERT (!vendor.getPublicKey ().isEmpty ())

	if(find (vendor.getVendorName ()) == nullptr)
	{
		auto* v = NEW PackageVendorSignature;
		v->setVendorName (vendor.getVendorName ());
		v->setSerializedPublicToken (vendor.getSerializedPublicToken ());
		v->setPublicKey (vendor.getPublicKey ());
		vendors.add (v);

		store (); // store when changed
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorStore::findPublicKey (Material& publicKey, StringRef vendorName) const
{
	if(PackageVendorSignature* vendor = find (vendorName))
	{
		ASSERT (!vendor->getPublicKey ().isEmpty ())
		publicKey.copyFrom (vendor->getPublicKey ());
		return !publicKey.isEmpty ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorStore::store ()
{
	Url path;
	getLocation (path);
	return saveToFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorStore::restore ()
{
	Url path;
	getLocation (path);
	return loadFromFile (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorStore::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	a.unqueue (vendors, "vendors", ccl_typeid<PackageVendorSignature> ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PackageVendorStore::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.queue ("vendors", vendors);
	return true;
}
