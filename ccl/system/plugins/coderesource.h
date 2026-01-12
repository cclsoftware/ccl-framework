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
// Filename    : ccl/system/plugins/coderesource.h
// Description : Code Resources
//
//************************************************************************************************

#ifndef _ccl_coderesource_h
#define _ccl_coderesource_h

#include "ccl/base/singleton.h"

#include "ccl/public/plugins/icoderesource.h"

namespace CCL {

interface IExecutableImage;

//************************************************************************************************
// CodeResource
//************************************************************************************************

class CodeResource: public Object,
					public ICodeResource
{
public:
	DECLARE_CLASS (CodeResource, Object)

	CodeResource (IClassFactory* classFactory = nullptr);
	~CodeResource ();

	// ICodeResource
	StringID CCL_API getType () const override;
	IClassFactory* CCL_API getClassFactory () override;
	IAttributeList* CCL_API getMetaInfo () override;
	tbool CCL_API getPath (IUrl& path) const override;

	CLASS_INTERFACE (ICodeResource, Object)

protected:
	IClassFactory* classFactory;
};

//************************************************************************************************
// CodeResourceLoader
//************************************************************************************************

class CodeResourceLoader: public Object,
						  public ICodeResourceLoader
{
public:
	DECLARE_CLASS (CodeResourceLoader, Object)

	// ICodeResourceLoader
	StringID CCL_API getType () const override;
	tbool CCL_API isCodeResource (UrlRef path) override;
	tresult CCL_API loadCodeResource (ICodeResource*& codeResource, UrlRef path) override;
	tbool CCL_API isKnownLocation (UrlRef path) override;

	CLASS_INTERFACE (ICodeResourceLoader, Object)
};

//************************************************************************************************
// NativeCodeResource
/** Native code resource (dynamic library). */
//************************************************************************************************

class NativeCodeResource: public CodeResource
{
public:
	NativeCodeResource (IExecutableImage& image);
	~NativeCodeResource ();

	// CodeResource
	IAttributeList* CCL_API getMetaInfo () override;
	tbool CCL_API getPath (IUrl& path) const override;

	CLASS_INTERFACES (CodeResource)

protected:
	IExecutableImage& image;
};

//************************************************************************************************
// NativeCodeLoader
/** Loader for native code resources. */
//************************************************************************************************

class NativeCodeLoader: public CodeResourceLoader,
						public Singleton<NativeCodeLoader>
{
public:
	tresult CCL_API loadCodeResource (ICodeResource*& codeResource, UrlRef path) override;
};

} // namespace CCL

#endif // _ccl_coderesource_h
