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
// Filename    : ccl/public/plugins/pluginst.h
// Description : Instance base class
//
//************************************************************************************************

#ifndef _ccl_pluginst_h
#define _ccl_pluginst_h

#include "ccl/public/base/cclmacros.h"
#include "ccl/public/plugins/iclassfactory.h"

namespace CCL {

//************************************************************************************************
// PluginInstance
/** \ingroup base_plug */
//************************************************************************************************

class PluginInstance: public IPluginInstance
{
public:
	PluginInstance ()
	: factoryToken (nullptr)
	{}

	// IPluginInstance
	void CCL_API setFactoryToken (Token token) override { factoryToken = token; }
	Token CCL_API getFactoryToken () const override		{ return factoryToken; }

	// IUnknown - will be overwritten by subclass!
	IMPLEMENT_DUMMY_UNKNOWN (IPluginInstance)

private:
	SharedPtr<IUnknown> factoryToken;
};

//************************************************************************************************
// PluginConstructor
/** \ingroup base_plug */
//************************************************************************************************

template <class Class, class Interface>
class PluginConstructor
{
public:
	static IUnknown* createInstance (UIDRef cid, void* userData)
	{
		return static_cast<Interface*> (NEW Class);
	}
};

} // namespace CCL

#endif // _ccl_pluginst_h
