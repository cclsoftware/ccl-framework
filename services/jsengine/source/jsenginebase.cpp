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
// Filename    : jsenginebase.cpp
// Description : JavaScript Engine - shared backend base class
//
//************************************************************************************************

#include "jsenginebase.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;
using namespace Scripting;
using namespace JScript;

//************************************************************************************************
// JScript::EngineBase
//************************************************************************************************

IEngineHost* EngineBase::getHost () const
{
	return UnknownPtr<IEngineHost> (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FileType& CCL_API EngineBase::getLanguage () const
{
	static FileType jsFileType ("JavaScript", "js", kJavaScript);
	return jsFileType;
}
