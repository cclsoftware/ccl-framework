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
// Filename    : core/portable/gui/coreviewbuilder.h
// Description : View Builder
//
//************************************************************************************************

#ifndef _coreviewbuilder_h
#define _coreviewbuilder_h

#include "core/portable/gui/coreview.h"
#include "core/portable/gui/coreviewcontroller.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// ViewBuilderObserver
/** \ingroup core_gui */
//************************************************************************************************

struct ViewBuilderObserver
{
	virtual void onViewLoaded (CStringPtr name) = 0;
};

//************************************************************************************************
// ViewBuilder
/** Build view tree from JSON files.
	\ingroup core_gui */
//************************************************************************************************

class ViewBuilder: public StaticSingleton<ViewBuilder>
{
public:
	ViewBuilder ();
	~ViewBuilder ();
	
	typedef View* (*CreateViewFunc) ();

	/** Register view class. */
	void addClass (CStringPtr name, CreateViewFunc createFunc);

	/** Register view class. */
	template <class Type>
	void addClass (CStringPtr name) { addClass (name, createView<Type>); }

	/** Load views from package defined in 'views.json/.ubj' file. */
	int loadViews (FilePackage& package);

	/** Remove all loaded view descriptors. */
	void removeAll ();

	/** Find view description by name. */
	const Attributes* findViewAttributes (CStringPtr name) const;

	/** Create view by name. */
	View* createView (CStringPtr name, ViewController* controller) const;

	/** Build view by name, use if outer view already exists. */
	bool buildView (View& view, CStringPtr name, ViewController* controller) const;

	/** Access to controller currently being used in buildView(). */
	ViewController* getCurrentController () const { return currentController; }

	DEFINE_OBSERVER (ViewBuilderObserver)

protected:
	template <class Type>
	static View* createView () { return NEW Type; }

	struct ViewClass
	{
		CStringPtr name;
		CreateViewFunc createFunc;

		ViewClass (CStringPtr name = nullptr, CreateViewFunc createFunc = nullptr)
		: name (name),
		  createFunc (createFunc)
		{}
	};

	struct ViewDescriptor
	{
		CString64 name;
		#if CORE_DEBUG_INTERNAL
		CString64 fileName;
		#endif
		Attributes* data;

		ViewDescriptor (CStringPtr name = nullptr, CStringPtr fileName = nullptr, Attributes* data = nullptr)
		: name (name),
		  #if CORE_DEBUG_INTERNAL
		  fileName (fileName),
		  #endif
		  data (data)
		{}

		bool operator == (const ViewDescriptor& other) const
		{
			return name.compare (other.name) == 0;
		}

		bool operator > (const ViewDescriptor& other) const
		{
			return name.compare (other.name) > 0;
		}
	};
	struct AttributeModifier;

	Vector<ViewClass> classes;
	Vector<ViewDescriptor> descriptors;
	int delegateClassIndex;
	mutable ViewController* currentController;

	void preprocessViewAttributes (Attributes& viewAttributes, AttributeValue* viewValue);

	int getViewClassIndex (CStringPtr name) const;
	View* createViewInstance (int classIndex) const;
	const ViewDescriptor* findDescriptor (CStringPtr name) const;
	void buildView (View& view, const Attributes& data, ViewController* controller, AttributeModifier* modifier) const;
	View* createSubView (CStringPtr name, const Attributes& outer, ViewController* controller, AttributeModifier* modifier) const;
};

} // namespace Portable
} // namespace Core

#endif // _coreviewbuilder_h
