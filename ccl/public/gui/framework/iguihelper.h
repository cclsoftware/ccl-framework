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
// Filename    : ccl/public/gui/framework/iguihelper.h
// Description : GUI Helper Interface
//
//************************************************************************************************

#ifndef _ccl_iguihelper_h
#define _ccl_iguihelper_h

#include "ccl/public/cclexports.h"
#include "ccl/public/base/iunknown.h"

namespace CCL {

struct KeyState;
struct KeyEvent;

namespace Internal {

//************************************************************************************************
// Internal::IGUIHelper
/** Helper methods for GUI classes. Do not use this interface directly. */
//************************************************************************************************

interface IGUIHelper: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// KeyEvent
	//////////////////////////////////////////////////////////////////////////////////////////////

	virtual tbool CCL_API KeyState_fromString (KeyState& This, StringRef string) = 0;
	virtual void CCL_API KeyState_toString (const KeyState& This, String& string, tbool translated) = 0;

	virtual tbool CCL_API KeyEvent_fromString (KeyEvent& This, StringRef string) = 0;
	virtual void CCL_API KeyEvent_toString (const KeyEvent& This, String& string, tbool translated) = 0;

	DECLARE_IID (IGUIHelper)
};

DEFINE_IID (IGUIHelper, 0x4e23f, 0xef36, 0x40f3, 0x8c, 0xa8, 0xbf, 0x78, 0xb9, 0x8e, 0xb3, 0xf0)

} // namespace Internal

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace System {

/** Get GUI Helper singleton (internal) */
CCL_EXPORT Internal::IGUIHelper& CCL_API CCL_ISOLATED (GetGUIHelper) ();
inline Internal::IGUIHelper& GetGUIHelper () { return CCL_ISOLATED (GetGUIHelper) (); }

} // namespace System
} // namespace CCL

#endif // _ccl_iguihelper_h
