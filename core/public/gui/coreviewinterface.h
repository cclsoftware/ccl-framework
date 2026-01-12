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
// Filename    : core/public/gui/coreviewinterface.h
// Description : View public interface
//
//************************************************************************************************

#ifndef _coreviewinterface_h
#define _coreviewinterface_h

#include "core/public/coreproperty.h"

namespace Core {

//************************************************************************************************
// ICoreView
/**	Interface for views, implemented by Core::Portable::View.
	For properties, see coreuiproperties.h.
	\ingroup core_gui */
//************************************************************************************************

struct ICoreView: IPropertyHandler
{
	/** Count sub views. */
	virtual int countSubViews () const = 0;

	/** Access sub views. */
	virtual ICoreView* getSubViewAt (int index) const = 0;

	static const InterfaceID kIID = FOUR_CHAR_ID ('V','i','e','w');
};

} // namespace Core

#endif // coreviewinterface
