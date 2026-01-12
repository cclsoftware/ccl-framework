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
// Filename    : ccl/public/gui/framework/dialogbox.h
// Description : Dialog Box
//
//************************************************************************************************

#ifndef _ccl_dialogbox_h
#define _ccl_dialogbox_h

#include "ccl/public/gui/framework/idialogbuilder.h"

namespace CCL {

//************************************************************************************************
// DialogBox
//************************************************************************************************

class DialogBox
{
public:
	DialogBox ();
	~DialogBox ();

	static IDialogBuilder* createBuilder ();

	/** Access dialog builder. */
	IDialogBuilder* operator -> () { return builder; }
	operator IDialogBuilder* () { return builder; }

protected:
	IDialogBuilder* builder;
};

} // namespace CCL

#endif // _ccl_dialogbox_h
