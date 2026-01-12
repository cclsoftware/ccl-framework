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
// Filename    : ccl/public/plugins/serviceplugin.h
// Description : Service Plugin
//
//************************************************************************************************

#ifndef _ccl_serviceplugin_h
#define _ccl_serviceplugin_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/plugins/icomponent.h"

namespace Core {
namespace Plugins {
struct ClassInfoBundle; }}

namespace CCL {

class ClassFactory;

//************************************************************************************************
// ServicePlugin
/** \ingroup base_plug */
//************************************************************************************************

class ServicePlugin: public Unknown,
					 public PluginInstance,
					 public IComponent
{
public:
	ServicePlugin ();
	~ServicePlugin ();

	// IComponent
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;

	CLASS_INTERFACES (Unknown)

protected:
	IUnknown* context;
	ClassFactory* classFactory;

	ClassFactory& getClassFactory ();
};

//************************************************************************************************
// CoreServicePlugin
/** Wrapper service for core classes.
	\ingroup base_plug */
//************************************************************************************************

class CoreServicePlugin: public ServicePlugin
{
public:
	CoreServicePlugin ();

	// ServicePlugin
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	tresult CCL_API terminate () override;

protected:
	IClassFactory* coreClassFactory;

	bool initFactory (const Core::Plugins::ClassInfoBundle* classBundle);
};

} // namespace CCL

#endif // _ccl_serviceplugin_h
