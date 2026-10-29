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
// Filename    : ccl/gui/theme/visualstyleclass.h
// Description : VisualStyle MetaClass
//
//************************************************************************************************

#ifndef _ccl_visualstyleclass_h
#define _ccl_visualstyleclass_h

#include "ccl/gui/guitypelib.h"

#include "ccl/base/singleton.h"

namespace CCL {

namespace VisualStyleClass {

//************************************************************************************************
// VisualStyleClass::Library
//************************************************************************************************

class Library: public TypeLibrary,
			   public Singleton<Library>
{
public:
	Library ();

protected:
	CStringEnumTypeInfo metrics;
	CStringEnumTypeInfo colors;
	CStringEnumTypeInfo cursors;
	CStringEnumTypeInfo fonts;
	CStringEnumTypeInfo standardStyles;
	CStringEnumTypeInfo elementImages;
	CStringEnumTypeInfo elementStates;
	CStringEnumTypeInfo elementStatesOn;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Library& getTypeLibrary ()
{
	return Library::instance ();
}

} // namespace VisualStyleClass

#define VISUALSTYLES_TYPELIB CCL::VisualStyleClass::getTypeLibrary ()

DECLARE_TYPELIB_ENTRY (VisualStyle)

} // namespace CCL

#endif // _ccl_visualstyleclass_h
