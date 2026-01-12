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

#include "ccl/base/typelib.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Visual Style Meta Class Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
	Example:

	BEGIN_VISUALSTYLE_CLASS (Control, VisualStyle, "ControlStyle")
		ADD_VISUALSTYLE_COLOR  ("background")
		ADD_VISUALSTYLE_METRIC ("margin")
		...
	END_VISUALSTYLE_CLASS (Control)
*/

#define BEGIN_VISUALSTYLE_CLASS(Class, Parent, name) \
	VisualStyleClass CCL::__vsc##Class (name, &__vsc##Parent); \
	Model::MemberDescription __vscMembers##Class []= {

#define BEGIN_VISUALSTYLE_BASE(Class, name) \
	VisualStyleClass CCL::__vsc##Class (name, 0); \
	Model::MemberDescription __vscMembers##Class[]= {

#define END_VISUALSTYLE_CLASS(Class) Model::MemberDescription () }; \
	static VisualStyleClass::MemberDescriptionModifier UNIQUE_IDENT (__vscmod) (__vsc##Class, __vscMembers##Class); \
	static VisualStyleClass::Registrar __reg##Class (__vsc##Class);

#define DECLARE_VISUALSTYLE_CLASS(Class) \
	extern VisualStyleClass __vsc##Class;

#define ADD_VISUALSTYLE_COLOR(name) \
	Model::MemberDescription (name, ITypeInfo::kString, "color"),

#define ADD_VISUALSTYLE_METRIC(name) \
	Model::MemberDescription (name, ITypeInfo::kFloat, "metric"),

#define ADD_VISUALSTYLE_FONT(name) \
	Model::MemberDescription (name, ITypeInfo::kString, "string"),

#define ADD_VISUALSTYLE_ALIGN(name) \
	Model::MemberDescription (name, ITypeInfo::kInt, "enum"),

#define ADD_VISUALSTYLE_OPTIONS(name) \
	Model::MemberDescription (name, ITypeInfo::kInt, "enum"),

#define ADD_VISUALSTYLE_IMAGE(name) \
	Model::MemberDescription (name, ITypeInfo::kString, "image"),

#define ADD_VISUALSTYLE_STRING(name) \
	Model::MemberDescription (name, ITypeInfo::kString, "string"),

//************************************************************************************************
// VisualStyleClass
//************************************************************************************************

class VisualStyleClass: public TypeInfo
{
public:
	VisualStyleClass (const char* name, const VisualStyleClass* parentClass = nullptr);

	struct Registrar;
	class Library;
	static TypeLibrary& getTypeLibrary ();

	// TypeInfo
	bool getDetails (ITypeInfoDetails& details) const override;

	struct MemberDescriptionModifier
	{
		MemberDescriptionModifier (VisualStyleClass& This, Model::MemberDescription members[]);
	};

private:
	Model::MemberDescription* members;
};

DECLARE_VISUALSTYLE_CLASS (VisualStyle)

//************************************************************************************************
// VisualStyleClass::Registrar
//************************************************************************************************

struct VisualStyleClass::Registrar
{
	Registrar (VisualStyleClass& vsc)
	{
		VisualStyleClass::getTypeLibrary ().addType (&vsc);
	}
};

} // namespace CCL

#endif // _ccl_visualstyleclass_h
