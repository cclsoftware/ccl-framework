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
// Filename    : ccl/public/plugservices.h
// Description : Plug-in Service APIs
//
//************************************************************************************************

#ifndef _ccl_plugservices_h
#define _ccl_plugservices_h

#include "ccl/public/cclexports.h"
#include "ccl/public/plugins/ipluginmanager.h"
#include "ccl/public/plugins/iscriptingmanager.h"

namespace CCL {

interface IServiceManager;
interface IObjectTable;
interface ITypeLibRegistry;

namespace System {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

/** Get plug-in manager singleton. */
CCL_EXPORT IPlugInManager& CCL_API CCL_ISOLATED (GetPlugInManager) ();
inline IPlugInManager& GetPlugInManager () { return CCL_ISOLATED (GetPlugInManager) (); }

/** Get service manager singleton. */
CCL_EXPORT IServiceManager& CCL_API CCL_ISOLATED (GetServiceManager) ();
inline IServiceManager& GetServiceManager () { return CCL_ISOLATED (GetServiceManager) (); }

/** Get global object table. */
CCL_EXPORT IObjectTable& CCL_API CCL_ISOLATED (GetObjectTable) ();
inline IObjectTable& GetObjectTable () { return CCL_ISOLATED (GetObjectTable) (); }

/** Get scripting manager singleton. */
CCL_EXPORT IScriptingManager& CCL_API CCL_ISOLATED (GetScriptingManager) ();
inline IScriptingManager& GetScriptingManager () { return CCL_ISOLATED (GetScriptingManager) (); }

/** Get type library registry singleton. */
CCL_EXPORT ITypeLibRegistry& CCL_API CCL_ISOLATED (GetTypeLibRegistry) ();
inline ITypeLibRegistry& GetTypeLibRegistry () { return CCL_ISOLATED (GetTypeLibRegistry) (); }

} // namespace System

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////

/** Shortcut to create instance via Plug-in Manager. */
template <class Interface> 
Interface* ccl_new (UIDRef cid = kNullUID)
{
	Interface* obj = nullptr;
	System::GetPlugInManager ().createInstance (cid.isValid () ? cid : ccl_iid<Interface> (), 
												ccl_iid<Interface> (), (void**)&obj);
	return obj;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Shorcut to create instance via Plug-in Manager by class name. */
template <class Interface> 
Interface* ccl_new (StringRef className)
{
	Interface* obj = nullptr;
	System::GetPlugInManager ().createInstance (className, ccl_iid<Interface> (), (void**)&obj);
	return obj;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Shortcut to release instance via Plug-in Manager. */
inline void ccl_release (IUnknown* obj)
{
	System::GetPlugInManager ().releaseInstance (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Shortcut to get class of existing instance via Plug-in Manager. */
inline const IClassDescription* ccl_classof (IUnknown* obj)
{
	return System::GetPlugInManager ().getInstanceClass (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Shortcut to force garbage collection via Scripting Manager. */
inline void ccl_forceGC ()
{
	System::GetScriptingManager ().garbageCollect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Shortcut to remove object reference via Scripting Manager. */
inline bool ccl_markGC (IUnknown* obj)
{
	return System::GetScriptingManager ().removeReference (obj) != 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper to pass automatic variables to scripts. */
template <typename T>
struct AutoGCObject
{
	T& object;
	AutoGCObject (T& object): object (object) {}
	~AutoGCObject () { ccl_markGC (&object); }
	operator T* () { return &object; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_plugservices_h
