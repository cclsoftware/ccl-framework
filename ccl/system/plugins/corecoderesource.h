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
// Filename    : ccl/system/plugins/corecoderesource.h
// Description : Core Plug-in Code Resource
//
//************************************************************************************************

#ifndef _ccl_corecoderesource_h
#define _ccl_corecoderesource_h

#include "ccl/system/plugins/coderesource.h"

#include "ccl/public/plugins/icoreplugin.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

//************************************************************************************************
// CoreCodeResource
//************************************************************************************************

class CoreCodeResource: public CodeResource
{
public:
	CoreCodeResource (IExecutableImage& image);
	~CoreCodeResource ();

	// CodeResource
	StringID CCL_API getType () const override;
	IAttributeList* CCL_API getMetaInfo () override;
	tbool CCL_API getPath (IUrl& path) const override;

	CLASS_INTERFACES (CodeResource)

protected:
	IExecutableImage& image;
};

//************************************************************************************************
// CoreCodeLoader
//************************************************************************************************

class CoreCodeLoader: public CodeResourceLoader,
					  public ICoreCodeLoader,
					  public Singleton<CoreCodeLoader>
{
public:
	~CoreCodeLoader ();

	// ICoreCodeLoader
	tbool CCL_API getDescription (ClassDesc& description, const Core::Plugins::ClassInfo& classInfo) override;
	IUnknown* CCL_API createInstance (const Core::Plugins::ClassInfo& classInfo, UIDRef iid) override;
	tresult CCL_API registerHandler (ICoreClassHandler* handler) override;
	tresult CCL_API unregisterHandler (ICoreClassHandler* handler) override;
	IClassFactory* CCL_API createClassFactory (const Core::Plugins::ClassInfoBundle& classBundle) override;

	// CodeResourceLoader
	StringID CCL_API getType () const override;
	tresult CCL_API loadCodeResource (ICodeResource*& codeResource, UrlRef path) override;

	CLASS_INTERFACE2 (ICoreClassHandler, ICoreCodeLoader, CodeResourceLoader)

protected:
	LinkedList<ICoreClassHandler*> handlerList;
};

} // namespace CCL

#endif // _ccl_corecoderesource_h
