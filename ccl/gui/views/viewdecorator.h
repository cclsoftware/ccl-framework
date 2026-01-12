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
// Filename    : ccl/gui/views/viewdecorator.h
// Description : View Decorator
//
//************************************************************************************************

#ifndef _ccl_viewdecorator_h
#define _ccl_viewdecorator_h

#include "ccl/gui/views/view.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/paramlist.h"

namespace CCL {

//************************************************************************************************
// ViewDecorator
//************************************************************************************************

class ViewDecorator: public Object,
					 public AbstractController,
					 public IViewFactory
{
public:
	ViewDecorator (View* contentView, StringID decorFormName, IUnknown* decorController = nullptr);

	PROPERTY_MUTABLE_CSTRING (decorFormName, DecorFormName)
	PROPERTY_SHARED_AUTO (IUnknown, decorController, DecorController)

	Attributes& getDecorArguments ();
	ParamList& getParamList ();

	View* decorateView (ITheme& theme);

	// AbstractController
	DECLARE_PARAMETER_LOOKUP (paramList)

	// IViewFactory
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	CLASS_INTERFACE2 (IController, IViewFactory, Object)

private:
	SharedPtr<View> contentView;
	Attributes decorArguments;
	ParamList paramList;
};

} // namespace CCL

#endif // _ccl_viewdecorator_h
