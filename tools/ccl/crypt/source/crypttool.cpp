//************************************************************************************************
//
// CCL Crypt Tool
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
// Filename    : crypttool.cpp
// Description : CCL Crypt Tool
//
//************************************************************************************************

#include "crypttool.h"
#include "appversion.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/packagesignature.h"

#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Security;

//************************************************************************************************
// CryptTool
//************************************************************************************************

CryptTool::CryptTool (ArgsRef args)
: args (args)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::run ()
{
	if(args.count () < 2)
	{
		console.writeLine (APP_FULL_NAME ", " APP_COPYRIGHT);

		static CStringPtr usage =	"Usage:\n"
									"\t" APP_ID " -[action] [args...]\n"
									"\n"
									"\t* Encrypt file           : -encrypt outFile inFile cipher\n"
									"\t* Decrypt file           : -decrypt outFile inFile cipher\n"
									"\t* Generate RSA key pair  : -generate outFolder [keyName] [-overwrite]\n"
									"\t* Create XML signature   : -sign outFile inFile rootName privateKey\n"
									"\t* Sign ZIP/Package file  : -signzip outFile inFile privateKey\n"
									"\t* Package Vendor Signing : -vendorsign outFile inFile privateToken\n"
									"\t* Create Public Token    : -create-public-token outFile vendorPublicKey vendorName authorityPrivateKey authorityKeyId\n"
									"\t* Create Private Token   : -create-private-token outFile vendorPrivateKey vendorPublicToken\n";

		console.writeLine (usage);
		return -1;
	}

	MutableCString action (args[1]);

	if(action == "-encrypt" || action == "-decrypt")
	{
		CipherAction cipherAction = action == "-encrypt" ? kEncrypt : kDecrypt;

		Url outPath, inPath, cipherPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (inPath, args[3]);
		makeAbsolute (cipherPath, args[4]);

		return cipher (outPath, inPath, cipherPath, cipherAction);
	}
	else if(action == "-generate")
	{
		Url outPath;
		makeAbsolute (outPath, args[2], Url::kFolder);

		String keyName;
		if(args.count () > 2)
			keyName = LegalFileName (args[3]);
		else
			keyName = "generated";

		bool overwrite = args.count () > 3 ? MutableCString (args[4]) == "-overwrite" : false;

		Url privateKeyPath (outPath);
		privateKeyPath.descend (String () << keyName << ".privatekey");
		if(overwrite == false)
			privateKeyPath.makeUnique ();

		Url publicKeyPath (outPath);
		publicKeyPath.descend (String () << keyName << ".publickey");
		if(overwrite == false)
			publicKeyPath.makeUnique ();

		return generate (privateKeyPath, publicKeyPath);
	}
	else if(action == "-sign")
	{
		Url outPath, inPath, privateKeyPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (inPath, args[3]);
		MutableCString rootName (args[4]);
		makeAbsolute (privateKeyPath, args[5]);

		return sign (outPath, inPath, rootName, privateKeyPath);
	}
	else if(action == "-signzip")
	{
		Url outPath, inPath, privateKeyPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (inPath, args[3]);
		makeAbsolute (privateKeyPath, args[4]);

		return signPackage (outPath, inPath, privateKeyPath);
	}
	else if(action == "-vendorsign")
	{
		Url outPath, inPath, vendorTokenPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (inPath, args[3]);
		makeAbsolute (vendorTokenPath, args[4]);

		return vendorSignPackage (outPath, inPath, vendorTokenPath);
	}
	else if(action == "-create-public-token")
	{		
		Url outPath, vendorPublicKeyPath, authorityPrivateKeyPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (vendorPublicKeyPath, args[3]);
		String vendorName = args[4];
		makeAbsolute (authorityPrivateKeyPath, args[5]);
		MutableCString authorityKeyId (args[6]);

		return createPublicToken (outPath, vendorPublicKeyPath, vendorName, 
								  authorityPrivateKeyPath, authorityKeyId);
	}
	else if(action == "-create-private-token")
	{
		Url outPath, vendorPrivateKeyPath, vendorPublicTokenPath;
		makeAbsolute (outPath, args[2]);
		makeAbsolute (vendorPrivateKeyPath, args[3]);
		makeAbsolute (vendorPublicTokenPath, args[4]);

		return createPrivateToken (outPath, vendorPrivateKeyPath, vendorPublicTokenPath);
	}
	else
	{
		console.writeLine ("Unknown action!");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::cipher (UrlRef outDataPath, UrlRef inDataPath, UrlRef cipherPath, CipherAction action)
{
	Crypto::Cipher cipher;
	if(!cipher.loadFromFile (cipherPath))
	{
		console.writeLine ("Failed to load cipher!");
		return -1;
	}

	AutoPtr<IMemoryStream> inData = File::loadBinaryFile (inDataPath);
	if(inData == nullptr)
	{
		console.writeLine ("Failed to load input data!");
		return -1;
	}

	AutoPtr<IStream> outData = System::GetFileSystem ().openStream (outDataPath, IStream::kCreateMode);
	if(outData == nullptr)
	{
		console.writeLine ("Failed to create output file!");
		return -1;
	}

	bool result = action == kEncrypt ? cipher.encrypt (*outData, *inData) : cipher.decrypt (*outData, *inData);
	if(result == false)
	{
		console.writeLine ("Cipher failed!");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::generate (UrlRef privateKeyPath, UrlRef publicKeyPath)
{
	Crypto::RawMaterial privateKey;
	Crypto::RawMaterial publicKey;
	if(!Crypto::RSA::generateKeyPair (privateKey, publicKey))
	{
		console.writeLine ("Failed to generate RSA key pair!");
		return -1;
	}

	if(!privateKey.saveToFile (privateKeyPath))
	{
		console.writeLine ("Failed to save private key!");
		return -1;
	}

	if(!publicKey.saveToFile (publicKeyPath))
	{
		console.writeLine ("Failed to save public key!");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::sign (UrlRef outDataPath, UrlRef inDataPath, StringID rootName, UrlRef privateKeyPath)
{
	AutoPtr<IMemoryStream> inData = File::loadBinaryFile (inDataPath);
	if(inData == nullptr)
	{
		console.writeLine ("Failed to load input data!");
		return -1;
	}

	Crypto::RawMaterial privateKey;
	if(!privateKey.loadFromFile (privateKeyPath))
	{
		console.writeLine (UrlDisplayString (privateKeyPath));
		console.writeLine ("Failed to load private key!");
		return -1;
	}

	Crypto::SignedXmlMessage message (rootName);
	message.getData ().copyFrom (*inData);

	Crypto::Signer signer;
	signer.setPrivateKey (privateKey);
	if(!signer.sign (message))
	{
		console.writeLine ("Failed to sign message!");
		return -1;
	}

	if(!message.saveToFile (outDataPath))
	{
		console.writeLine ("Failed to save signed data!");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::signPackage (UrlRef outDataPath, UrlRef inDataPath, UrlRef privateKeyPath)
{
	Crypto::RawMaterial privateKey;
	if(!privateKey.loadFromFile (privateKeyPath))
	{
		console.writeLine (UrlDisplayString (privateKeyPath));
		console.writeLine ("Failed to load private key!");
		return -1;
	}

	Crypto::PackageSigner signer;
	signer.setPrivateKey (privateKey);
	return signer.sign (outDataPath, inDataPath) ? 0 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::vendorSignPackage (UrlRef outDataPath, UrlRef inDataPath, UrlRef privateTokenPath)
{
	Crypto::PackageVendorSignature vendorSignature (Crypto::PackageVendorSignature::kToolUsage);
	if(!vendorSignature.loadPrivateToken (privateTokenPath))
	{
		console.writeLine ("Failed to load private vendor token!");
		return -1;
	}

	return vendorSignature.sign (outDataPath, inDataPath) ? 0 : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::createPublicToken (UrlRef outPath, UrlRef vendorPublicKeyPath, StringRef vendorName, 
								  UrlRef authorityPrivateKeyPath, StringID authorityKeyId)
{
	Crypto::PackageVendorSigningAuthority authority;
	authority.setParentKeyID (authorityKeyId);
	if(!authority.loadPrivateParentKey (authorityPrivateKeyPath))
	{
		console.writeLine ("Failed to load authority private key!");
		return -1;
	}

	Crypto::RawMaterial vendorPublicKey;
	if(!vendorPublicKey.loadFromFile (vendorPublicKeyPath))
	{
		console.writeLine ("Failed to load vendor public key!");
		return -1;
	}

	String result = authority.createPublicVendorToken (vendorPublicKey, vendorName);
	if(result.isEmpty ())
	{
		console.writeLine ("Failed to create public vendor token. Please check your arguments.");
		return -1;
	}

	if(!Crypto::RawMaterial ().append (result, Text::kASCII).saveToFile (outPath))
	{
		console.writeLine ("Failed to save public vendor token to output file.");
		return -1;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CryptTool::createPrivateToken (UrlRef outPath, UrlRef vendorPrivateKeyPath, UrlRef vendorPublicTokenPath)
{
	Crypto::RawMaterial vendorPrivateKey;
	if(!vendorPrivateKey.loadFromFile (vendorPrivateKeyPath))
	{
		console.writeLine ("Failed to load vendor private key!");
		return -1;
	}
	
	Crypto::PackageVendorSignature vendorSignature (Crypto::PackageVendorSignature::kToolUsage);
	if(!vendorSignature.loadPublicToken (vendorPublicTokenPath))
	{
		console.writeLine ("Failed to load vendor public token!");
		return -1;
	}

	vendorSignature.setPrivateKey (vendorPrivateKey);
	String result = vendorSignature.serializePrivateToken ();

	if(!Crypto::RawMaterial ().append (result, Text::kASCII).saveToFile (outPath))
	{
		console.writeLine ("Failed to save private vendor token to output file.");
		return -1;
	}

	return 0;
}
