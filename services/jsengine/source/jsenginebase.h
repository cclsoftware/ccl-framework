//************************************************************************************************
//
// JavaScript Engine
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : jsenginebase.h
// Description : JavaScript Engine - shared backend base class
//
//************************************************************************************************

#ifndef _jsenginebase_h
#define _jsenginebase_h

#include "ccl/public/plugins/serviceplugin.h"
#include "ccl/public/plugins/iscriptengine.h"

namespace JScript {

//************************************************************************************************
// JScript::EngineBase
/** Shared base for the per-backend script engines. */
//************************************************************************************************

class EngineBase: public CCL::ServicePlugin,
				  public CCL::Scripting::IEngine
{
public:
	CCL::Scripting::IEngineHost* getHost () const;

	// Scripting::IEngine
	const CCL::FileType& CCL_API getLanguage () const override;

	CLASS_INTERFACE (IEngine, ServicePlugin)

protected:
	static constexpr size_t kDefaultGCThreshold = 32 * 1024 * 1024; // 32 MB
};

} // namespace JScript

#endif // _jsenginebase_h
