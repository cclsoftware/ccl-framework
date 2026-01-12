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
// Filename    : ccl/gui/guihelper.h
// Description : GUI Helper
//
//************************************************************************************************

#ifndef _ccl_guihelper_h
#define _ccl_guihelper_h

#include "ccl/base/object.h"

#include "ccl/public/gui/framework/iguihelper.h"

namespace CCL {

//************************************************************************************************
// GUIHelper
//************************************************************************************************

class GUIHelper: public Object,
				 public Internal::IGUIHelper
{
public:
	DECLARE_CLASS_ABSTRACT (GUIHelper, Object)

	// KeyEvent
	tbool CCL_API KeyState_fromString (KeyState& This, StringRef string) override;
	void CCL_API KeyState_toString (const KeyState& This, String& string, tbool translated) override;
	tbool CCL_API KeyEvent_fromString (KeyEvent& This, StringRef string) override;
	void CCL_API KeyEvent_toString (const KeyEvent& This, String& string, tbool translated) override;

	CLASS_INTERFACE (IGUIHelper, Object)
};

} // namespace CCL

#endif // _ccl_guihelper_h
