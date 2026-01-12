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
// Filename    : ccl/base/security/packagesignature.h
// Description : Package Digital Signature
//
//************************************************************************************************

#ifndef _ccl_packagesignature_h
#define _ccl_packagesignature_h

#include "ccl/base/security/signature.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

namespace CCL {

interface IUrlFilter;
interface IPackageFile;
class ArchiveHandler;
class PackageInfo;

namespace Security {
class JWTObject;

namespace Crypto {

//************************************************************************************************
// Crypto::PackageSignature
//************************************************************************************************

class PackageSignature: public SignedXmlMessage
{
public:
	DECLARE_CLASS (PackageSignature, SignedXmlMessage)

	PackageSignature ();

	class Part;
	class PartList;
	class Builder;

	static const String kFileName;	///< name of signature file inside package

	void setParts (const PartList& partList);
	bool getParts (PartList& partList);

	bool loadFromPackage (IPackageFile& package);
	bool saveWithPackage (IPackageFile& package) const;

	bool loadFromHandler (ArchiveHandler& handler);
	bool saveWithHandler (ArchiveHandler& handler) const;
};

//************************************************************************************************
// Crypto::PackageSigner
//************************************************************************************************

class PackageSigner: public Signer
{
public:
	DECLARE_CLASS (PackageSigner, Signer)

	using Signer::sign;

	/** Create signed copy of package file. */
	bool sign (UrlRef outPath, UrlRef inPath, IProgressNotify* progress = nullptr);
};

//************************************************************************************************
// Crypto::PackageVerifierOptions
//************************************************************************************************

class PackageVerifierOptions
{
public:
	PackageVerifierOptions ();

	PROPERTY_BOOL (loggingEnabled, LoggingEnabled)

	void setOptions (const PackageVerifierOptions& other);

protected:
	void logError (StringRef message, UrlRef path);
	void logError (StringRef message, StringRef packageId);
};

//************************************************************************************************
// Crypto::PackageVerifier
//************************************************************************************************

class PackageVerifier: public Verifier,
					   public PackageVerifierOptions
{
public:
	DECLARE_CLASS (PackageVerifier, Verifier)

	using Verifier::verify;

	/** Verify package file signature. */
	bool verify (UrlRef path, IUrlFilter* filter = nullptr, IProgressNotify* progress = nullptr);
};

//************************************************************************************************
// Crypto::PackageSignature::Part
//************************************************************************************************

class PackageSignature::Part: public Object
{
public:
	DECLARE_CLASS (Part, Object)

	PROPERTY_STRING (fileName, FileName)
	PROPERTY_OBJECT (Material, digestSHA1, DigestSHA1)

	// Object
	int compare (const Object& obj) const override;
};

//************************************************************************************************
// Crypto::PackageSignature::PartList
//************************************************************************************************

class PackageSignature::PartList: public StorableObject
{
public:
	DECLARE_CLASS (PartList, StorableObject)

	PartList ();

	void addPart (Part* part);
	Iterator* newIterator () const;
	const Part* findPart (StringRef fileName) const;

	void dump () const;

	// StorableObject
	bool equals (const Object& obj) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;

protected:
	ObjectArray parts;
};

//************************************************************************************************
// Crypto::PackageSignature::Builder
//************************************************************************************************

class PackageSignature::Builder
{
public:
	Builder ();

	PROPERTY_OBJECT (PartList, partList, Parts)
	PROPERTY_POINTER (IUrlFilter, filter, Filter)

	bool build (UrlRef path, IProgressNotify* progress = nullptr);
	bool build (IPackageFile& package, IProgressNotify* progress = nullptr);

protected:
	bool build (IFileSystem& fs, UrlRef folder, IProgressNotify* progress);
};

//************************************************************************************************
// Crypto::IPackageVendorSigningAuthority
//************************************************************************************************

interface IPackageVendorSigningAuthority
{
	/** Create JWT with signed public key of vendor. */
	virtual String createPublicVendorToken (const Material& publicKey, StringRef vendorName) = 0;

	DECLARE_IID (IPackageVendorSigningAuthority)
};

//************************************************************************************************
// Crypto::PackageVendorSigningAuthority
//************************************************************************************************

class PackageVendorSigningAuthority: public IPackageVendorSigningAuthority
{
public:
	PROPERTY_MUTABLE_CSTRING (parentKeyId, ParentKeyID)
	PROPERTY_OBJECT (Material, privateParentKey, PrivateParentKey)
	
	bool loadPrivateParentKey (UrlRef path);

	// IPackageVendorSigningAuthority
	String createPublicVendorToken (const Material& publicKey, StringRef vendorName) override;
};

//************************************************************************************************
// Crypto::PackageVendorSignature
//************************************************************************************************

class PackageVendorSignature: public Object,
							  public PackageVerifierOptions
{
public:
	DECLARE_CLASS (PackageVendorSignature, Object)

	enum UsageHint { kRegularUsage, kToolUsage };
	PackageVendorSignature (UsageHint usage = kRegularUsage);

	DECLARE_METHOD_NAMES (PackageVendorSignature)
	DECLARE_PROPERTY_NAMES (PackageVendorSignature)

	DECLARE_STRINGID_MEMBER (kVendor)
	DECLARE_STRINGID_MEMBER (kPrivateKey)
	DECLARE_STRINGID_MEMBER (kPublicKey)
	DECLARE_STRINGID_MEMBER (kPublicToken)
	DECLARE_STRINGID_MEMBER (kParentKeyID)

	PROPERTY_STRING (vendorName, VendorName)
	PROPERTY_OBJECT (Material, privateKey, PrivateKey) // optional, for vendor only
	PROPERTY_OBJECT (Material, publicKey, PublicKey)
	PROPERTY_STRING (serializedPublicToken, SerializedPublicToken)
		
	static void setGlobalAuthority (IPackageVendorSigningAuthority* authority);
	String createPrivateTokenWithGlobalAuthority ();
	String createPrivateTokenWithParentKey (UrlRef privateParentKeyPath, StringID parentKeyId);
	String createPrivateToken (IPackageVendorSigningAuthority& authority);
	String serializePrivateToken () const;
	
	bool loadPrivateToken (UrlRef tokenPath);
	bool loadPrivateToken (StringRef token);

	bool loadPublicToken (UrlRef tokenPath);
	bool loadPublicToken (StringRef token);

	bool sign (UrlRef outPath, UrlRef inPath, IProgressNotify* progress = nullptr);
	bool verifyVendor (PackageSignature& signature, const PackageInfo& info);
	bool verify (PackageVerifier& verifier, PackageSignature& signature, const PackageInfo& info);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	static IPackageVendorSigningAuthority* globalAuthority;

	bool loadToken (JWTObject& jwt, StringRef token, bool verify);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	UsageHint usage;
};

//************************************************************************************************
// Crypto::PackageVendorStore
//************************************************************************************************

class PackageVendorStore: public StorableObject,
						  public Singleton<PackageVendorStore>
{
public:
	DECLARE_CLASS (PackageVendorStore, StorableObject)

	PackageVendorStore ();

	void addOnce (const PackageVendorSignature& vendor);
	bool findPublicKey (Material& publicKey, StringRef vendorName) const;

	bool store ();
	bool restore ();

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	static const String kFileName;

	ObjectArray vendors;

	void getLocation (IUrl& path) const;
	PackageVendorSignature* find (StringRef vendorName) const;
};

} // namespace Crypto
} // namespace Security
} // namespace CCL

#endif // _ccl_packagesignature_h
