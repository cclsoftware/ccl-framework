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
// Filename    : ccl/system/plugins/scriptcoderesource.h
// Description : Script Code Resource
//
//************************************************************************************************

#ifndef _ccl_scriptcoderesource_h
#define _ccl_scriptcoderesource_h

#include "ccl/system/plugins/coderesource.h"

#include "ccl/public/system/ikeyprovider.h"
#include "ccl/public/plugins/iscriptcodeloader.h"

namespace CCL {

class ScriptUplink;
interface IPackageFile;
interface ITranslationTable;

//************************************************************************************************
// ScriptCodeResource
/** Code resource representing a script. */
//************************************************************************************************

class ScriptCodeResource: public CodeResource
{
public:
	ScriptCodeResource (IPackageFile* package, 
						IClassFactory* classFactory, 
						ScriptUplink* uplink,
						IObject* executable = nullptr);
	~ScriptCodeResource ();

	ITranslationTable* getStrings ();

	// CodeResource
	StringID CCL_API getType () const override;
	IAttributeList* CCL_API getMetaInfo () override;
	tbool CCL_API getPath (IUrl& path) const override;

protected:
	IPackageFile* package;
	ScriptUplink* uplink;
	IObject* executable;
	ITranslationTable* stringTable;
};

//************************************************************************************************
// ScriptCodeLoader
/** Loader for script code resources. */
//************************************************************************************************

class ScriptCodeLoader: public CodeResourceLoader,
						public IScriptCodeLoader,
						public Singleton<ScriptCodeLoader>
{
public:
	// IScriptCodeLoader
	tresult CCL_API setKeyProvider (IEncryptionKeyProvider* keyProvider) override;

	// CodeResourceLoader
	StringID CCL_API getType () const override;
	tbool CCL_API isCodeResource (UrlRef path) override;
	tresult CCL_API loadCodeResource (ICodeResource*& codeResource, UrlRef path) override;

	CLASS_INTERFACE (IScriptCodeLoader, CodeResourceLoader)

protected:
	SharedPtr<IEncryptionKeyProvider> keyProvider;

	~ScriptCodeLoader ();
};

} // namespace CCL

#endif // _ccl_scriptcoderesource_h
