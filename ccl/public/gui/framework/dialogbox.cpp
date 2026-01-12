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
// Filename    : ccl/public/gui/framework/dialogbox.cpp
// Description : Dialog Box
//
//************************************************************************************************

#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/viewbox.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// DialogBox
//************************************************************************************************

IDialogBuilder* DialogBox::createBuilder ()
{
	IDialogBuilder* builder = ccl_new<IDialogBuilder> (ClassID::DialogBuilder);
	ASSERT (builder != nullptr)
	
	builder->setTheme (ViewBox::getModuleTheme ());
	builder->setStrings (LocalString::getTable ());
	return builder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogBox::DialogBox ()
: builder (createBuilder ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogBox::~DialogBox ()
{
	builder->release ();
}
