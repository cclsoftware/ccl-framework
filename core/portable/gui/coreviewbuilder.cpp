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
// Filename    : core/portable/gui/coreviewbuilder.cpp
// Description : View Builder
//
//************************************************************************************************

#define CORE_PROFILE 0
#define DEBUG_LOG 1

#include "coreviewbuilder.h"
#include "corecontrols.h"
#include "corelistview.h"
#include "corekeyboard.h"

#include "core/portable/corepersistence.h"
#include "core/portable/coreprofiling.h"
#include "core/public/coreprimitives.h"

namespace Core {
namespace Portable {

#define MODIFY_INPLACE 1

//************************************************************************************************
// ViewBuilder::AttributeModifier
//************************************************************************************************

struct ViewBuilder::AttributeModifier
{
	AttributeModifier (const Attributes& outer, AttributeModifier* parent);
	~AttributeModifier ();

	const Attributes& modifyAttributes (const Attributes& inner);

	// mark attribute values containing variables ('$') with a user flag
	static inline bool hasVariables (const AttributeValue& attribute)
	{
		ASSERT (attribute.getAttributes () || attribute.isUserFlag1 () == ConstString (attribute.getString ()).contains ('$'))
		return attribute.isUserFlag1 ();
	}
	static inline void hasVariables (AttributeValue& attribute, bool state)	{ attribute.isUserFlag1 (state); }

private:
	#if MODIFY_INPLACE
	struct SavedValue
	{
		int index;
		CString128 value;

		SavedValue (int index = 0, CStringPtr value = nullptr)
		: index (index), value (value)
		{}
	};
	FixedSizeVector<SavedValue, 8> oldValuesFixed;
	Vector<SavedValue> oldValuesDynamic;
	Attributes* originalAttribs;
	#else
	Attributes* modified;
	#endif

	AttributeModifier* parent;
	Attributes* defines;

	bool anyDefines () const;
	CStringPtr resolveString (CString128& tmp, CStringPtr name) const;
};

//************************************************************************************************
// DelegateView
//************************************************************************************************

class DelegateView: public ContainerView,
					public IVariantChildView
{
public:
	BEGIN_CORE_CLASS ('DelV', DelegateView)
		ADD_CORE_CLASS_ (IVariantChildView)
	END_CORE_CLASS (ContainerView)

	DECLARE_CORE_VIEWCLASS (ViewClasses::kDelegate)

	DelegateView (RectRef size = Rect ());

	PROPERTY_POINTER (ViewController, controller, Controller)
	PROPERTY_CSTRING_BUFFER (64, viewName, ViewName)

	// ContainerView
	void setAttributes (const Attributes& a) override;

protected:
	// IVariantChildView
	void onVariantAttached (bool state) override;
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// ViewBuilder
//************************************************************************************************

DEFINE_STATIC_SINGLETON (ViewBuilder)

//************************************************************************************************
// DelegateView
//************************************************************************************************

DelegateView::DelegateView (RectRef size)
: ContainerView (size),
  controller (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DelegateView::setAttributes (const Attributes& a)
{
	setViewName (a.getString (ViewAttributes::kViewName));
	ContainerView::setAttributes (a);

	if(size.isEmpty ()) // take size from view descriptor
		if(const Attributes* attr = ViewBuilder::instance ().findViewAttributes (viewName))
			setSize (ViewAttributes::getSize (*attr));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DelegateView::onVariantAttached (bool state)
{
	if(state)
	{
		ASSERT (children.isEmpty ())
		View* child = ViewBuilder::instance ().createView (viewName, controller);
		if(child)
			addView (child);
	}
	else
	{
		removeAll ();
	}
}

//************************************************************************************************
// ViewBuilder
//************************************************************************************************

ViewBuilder::ViewBuilder ()
: delegateClassIndex (-1),
  currentController (nullptr)
{
	addClass<View> (ViewClasses::kView);
	addClass<Label> (ViewClasses::kLabel);
	addClass<MultiLineLabel> (ViewClasses::kMultiLineLabel);
	addClass<ImageView> (ViewClasses::kImageView);
	addClass<VariantView> (ViewClasses::kVariantView);
	addClass<AlignView> (ViewClasses::kAlignView);
	addClass<Button> (ViewClasses::kButton);
	addClass<Toggle> (ViewClasses::kToggle);
	addClass<RadioButton> (ViewClasses::kRadioButton);
	addClass<ValueBar> (ViewClasses::kValueBar);
	addClass<Slider> (ViewClasses::kSlider);
	addClass<TextBox> (ViewClasses::kTextBox);
	addClass<EditBox> (ViewClasses::kEditBox);
	addClass<SelectBox> (ViewClasses::kSelectBox);
	addClass<ListView> (ViewClasses::kListView);
	addClass<TouchKeyboard> (ViewClasses::kTouchKeyboard);
	addClass<TextInputBox> (ViewClasses::kTextInputBox);

	// special view classes used with builder
	delegateClassIndex = classes.count ();
	addClass<DelegateView> (ViewClasses::kDelegate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewBuilder::~ViewBuilder ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBuilder::removeAll ()
{
	VectorForEach (descriptors, ViewDescriptor, descriptor)
		delete descriptor.data;
	EndFor
	descriptors.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBuilder::addClass (CStringPtr name, CreateViewFunc createFunc)
{
	ASSERT (descriptors.isEmpty ()) // must register before loading views!
	classes.add (ViewClass (name, createFunc));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBuilder::getViewClassIndex (CStringPtr _name) const
{
	ConstString name (_name);
	for(int i = 0; i < classes.count (); i++)
		if(name == classes[i].name)
			return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewBuilder::createViewInstance (int classIndex) const
{
	ViewClass viewClass = classes.at (classIndex);
	return viewClass.createFunc ? viewClass.createFunc () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewBuilder::loadViews (FilePackage& package)
{
	int count = 0;
	Archiver::Format primaryFormat = Archiver::kJSON;
	IO::Stream* jsonStream = package.openStream (Skin::FileNames::kViewFile1);
	if(jsonStream == nullptr)
	{
		jsonStream = package.openStream (Skin::FileNames::kViewFile2);
		primaryFormat = Archiver::kUBJSON;
	}
	if(jsonStream != nullptr)
	{
		Deleter<IO::Stream> deleter (jsonStream);
		Attributes a (AttributeAllocator::getDefault ());
		AttributePoolSuspender suspender; // don't allocate from memory pool
		if(Archiver (jsonStream, primaryFormat).load (a))
			if(const AttributeQueue* viewArray = a.getQueue (nullptr))
				VectorForEach (viewArray->getValues (), AttributeValue*, value)
					if(const Attributes* viewAttr = value->getAttributes ())
					{
						CStringPtr name = viewAttr->getString (ResourceAttributes::kName);
						CStringPtr fileName = viewAttr->getString (ResourceAttributes::kFile);
						Archiver::Format secondaryFormat = Archiver::detectFormat (fileName);

						IO::Stream* subStream = package.openStream (fileName);
						#if 0 // second try with primary format
						if(subStream == 0 && primaryFormat != secondaryFormat)
						{
							FileName altFileName (fileName);
							altFileName.setExtension (Archiver::getFileType (primaryFormat));
							subStream = package.openStream (altFileName);
							secondaryFormat = primaryFormat;
						}
						#endif
						if(subStream != nullptr)
						{
							Attributes* subAttr = NEW Attributes (a.getAllocator ());
							if(Archiver (subStream, secondaryFormat).load (*subAttr))
							{
								preprocessViewAttributes (*subAttr, value);

								ViewDescriptor descriptor (name, fileName, subAttr);
								ASSERT (descriptor.name == name) // check for truncated name
								descriptors.addSorted (descriptor);
								observers.notify (&ViewBuilderObserver::onViewLoaded, name);
								count++;
							}
							else
							{
								CORE_PRINTF ("Failed to parse view file: %s\n", fileName)
								delete subAttr;
							}

							delete subStream;
						}
						#if DEBUG_LOG
						else
							CORE_PRINTF ("Failed to open view file: %s\n", fileName);
						#endif
					}
				EndFor
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBuilder::preprocessViewAttributes (Attributes& viewAttributes, AttributeValue* viewValue)
{
	// get "inline" style attributes of view and apply attributes from parent style, e.g. "style": {"inherit":"ParentStyle", ... }
	Attributes* styleAttribs = viewAttributes.getAttributes (ViewAttributes::kStyle);
	if(styleAttribs)
	{
		StyleManager::preprocessStyleAttributes (*styleAttribs);
		StyleManager::addInheritedStyleAttributes (styleAttribs);
	}

	// resolve index to built-in view classes
	if(Attribute* typeAttr = const_cast<Attribute*> (viewAttributes.lookup (ViewAttributes::kType)))
	{
		int64 index = getViewClassIndex (typeAttr->getString ());
		if(index != -1)
			typeAttr->set (index);
	}

	// pack rect into int64 to avoid string operations during view creation
	if(Attribute* sizeAttr = const_cast<Attribute*> (viewAttributes.lookup (ViewAttributes::kSize)))
	{
		Rect size;
		ResourceAttributes::parseSize (size, sizeAttr->getString ());
		sizeAttr->set (ResourceAttributes::packRect (size));
	}

	// flag attributes containing variables
	bool hasVariables = false;
	for(int i = 0, num = viewAttributes.countAttributes (); i < num; i++)
	{
		Attribute* attr = const_cast<Attribute*> (viewAttributes.getAttribute (i));
		if(CStringPtr string = attr->getString ())
			if(ConstString (string).contains ('$'))
			{
				AttributeModifier::hasVariables (*attr, true); // flag the attribute with the '$'
				hasVariables = true;
			}
	}
	if(hasVariables && viewValue)
		AttributeModifier::hasVariables (*viewValue, true); // also flag the containing view

	StyleManager::preprocessStyleAttributes (viewAttributes); // for colors as direct view attribute

	// recursion for all child views
	if(const AttributeQueue* childArray = viewAttributes.getQueue (ViewAttributes::kChildren))
		VectorForEach (childArray->getValues (), AttributeValue*, value)
			if(Attributes* childAttr = value->getAttributes ())
				preprocessViewAttributes (*childAttr, value);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ViewBuilder::ViewDescriptor* ViewBuilder::findDescriptor (CStringPtr name) const
{
	return descriptors.search (ViewDescriptor (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes* ViewBuilder::findViewAttributes (CStringPtr name) const
{
	const ViewDescriptor* descriptor = findDescriptor (name);
	return descriptor ? descriptor->data : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewBuilder::createView (CStringPtr name, ViewController* controller) const
{
	ContainerView* view = NEW ContainerView;
	if(!buildView (*view, name, controller))
	{
		delete view;
		return nullptr;
	}
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBuilder::buildView (View& view, CStringPtr name, ViewController* controller) const
{
	CORE_PROFILE_START (findDescriptor)
	
	const ViewDescriptor* descriptor = findDescriptor (name);
	if(descriptor == nullptr)
		return false;

	CORE_PROFILE_STOP (findDescriptor, "ViewBuilder::buildView found descriptor")
	
	CORE_PROFILE_START (buildView)
	buildView (view, *descriptor->data, controller, nullptr);
	
	// keep reference to source file for spy tool
	#if CORE_DEBUG_INTERNAL
	if(ContainerView* container = view.asContainer ())
		container->setSourceFile (descriptor->fileName);
	#endif
	
	CORE_PROFILE_STOP (buildView, "ViewBuilder::buildView finished building view")
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewBuilder::createSubView (CStringPtr name, const Attributes& outer, ViewController* controller, AttributeModifier* modifier) const
{
	const ViewDescriptor* descriptor = findDescriptor (name);
	if(descriptor == nullptr)
		return nullptr;
		
	ContainerView* view = NEW ContainerView;
	#if CORE_DEBUG_INTERNAL
	view->setSourceFile (descriptor->fileName); // keep reference to source file for spy tool
	#endif
	AttributeModifier mod (outer, modifier);	

	buildView (*view, mod.modifyAttributes (*descriptor->data), controller, &mod);

	Rect size = ViewAttributes::getSize (outer);
	if(size != Rect ())
	{
		Rect viewSize = view->getSize ();
		viewSize.moveTo (size.getLeftTop ());
		if(size.getWidth () > 0)
			viewSize.setWidth (size.getWidth ());
		if(size.getHeight () > 0)
			viewSize.setHeight (size.getHeight ());
		view->setSize (viewSize);
	}

	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewBuilder::buildView (View& view, const Attributes& data, ViewController* controller, AttributeModifier* modifier) const
{
	ScopedVar<ViewController*> controllerScope (currentController, controller);
	view.setAttributes (data);

	ConstString name = data.getString (ViewAttributes::kName);		
	ASSERT (name.length () < view.getName ().getSize ()) // check for truncation
	view.setName (name);

	if(controller != nullptr)
	{
		ConstString subControllerName = data.getString (ViewAttributes::kController);
		if(subControllerName.isEmpty () == false)
		{
			ViewController* subController = reinterpret_cast<ViewController*> (controller->getObjectForView (subControllerName, kControllerType));
			ASSERT (subController)
			if(subController)
				controller = subController;
		}
	}

	if(!name.isEmpty () && controller != nullptr)
	{
		// connect view with object provided by controller
		CStringPtr type = view.getConnectionType ();
		if(type != nullptr)
		{
			void* object = controller->getObjectForView (name, type);
			ASSERT (object)
			if(object != nullptr)
				view.connect (object);
		}
	}
	
	// create child views
	if(ContainerView* container = view.asContainer ())
	{
		if(const AttributeQueue* childArray = data.getQueue (ViewAttributes::kChildren))
			VectorForEach (childArray->getValues (), AttributeValue*, value)
				if(const Attributes* childAttr = value->getAttributes ())
				{
					View* subView = nullptr;
					bool viewBuilt = false;

					if(const Attribute* typeAttr = childAttr->lookup (ViewAttributes::kType))
					{
						// 1) try built-in control
						if(typeAttr->getType () == Attribute::kInt) // index of built-in view class
						{
							int classIndex = (int)typeAttr->getInt ();
							if(classIndex == delegateClassIndex)
							{
								DelegateView* delegateView = NEW DelegateView;
								delegateView->setController (controller);
								subView = delegateView;
							}
							else
								subView = createViewInstance (classIndex);
						}
						else
						{
							ConstString type (typeAttr->getString ());
							if(!type.isEmpty ())
							{
								// 2) ask controller to create user control
								if(controller != nullptr)
									subView = controller->createView (type);

								// 3) try reference to other view descriptor
								if(subView == nullptr)
								{
									subView = createSubView (type, *childAttr, controller, modifier);
									viewBuilt = subView != nullptr;
								}
							}
						}
					}

					// 4) fallback to simple container
					if(subView == nullptr)
						subView = NEW ContainerView;

					if(viewBuilt == false)
					{
						if(AttributeModifier::hasVariables (*value))
						{
							AttributeModifier mod (data, modifier);	
							buildView (*subView, mod.modifyAttributes (*childAttr), controller, &mod);
						}
						else
							buildView (*subView, *childAttr, controller, modifier);
					}
					
					container->addView (subView);
				}
			EndFor

		if(container->getSize ().isEmpty () && container->getChildren ().isEmpty () == false)
			container->resizeToChildren ();							
	}
}

//************************************************************************************************
// ViewBuilder::AttributeModifier
//************************************************************************************************

ViewBuilder::AttributeModifier::AttributeModifier (const Attributes& outer,  AttributeModifier* _parent)
: parent (_parent),
  defines (nullptr),
  #if MODIFY_INPLACE
  originalAttribs (nullptr)
  #else
  modified (0)
  #endif
{
	defines = outer.getAttributes (ViewAttributes::kDefines);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewBuilder::AttributeModifier::~AttributeModifier ()
{
	#if MODIFY_INPLACE
	// restore modified attributes
	if(originalAttribs)
	{
		VectorForEach (oldValuesFixed, const SavedValue&, saved)
			Attribute* attr = const_cast<Attribute*> (originalAttribs->getAttribute (saved.index));
			//CORE_PRINTF ("  restore %d %s -> %s\n", saved.index, attr->getString (), saved.value.str ());
			attr->set (saved.value);
		EndFor
		VectorForEach (oldValuesDynamic, const SavedValue&, saved)
			Attribute* attr = const_cast<Attribute*> (originalAttribs->getAttribute (saved.index));
			//CORE_PRINTF ("  restore %d %s -> %s\n", saved.index, attr->getString (), saved.value.str ());
			attr->set (saved.value);
		EndFor
	}
	#else
	if(modified)
		delete modified;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewBuilder::AttributeModifier::anyDefines () const
{
	if(defines && defines->countAttributes () > 0)
		return true;
	if(parent)
		return parent->anyDefines ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes& ViewBuilder::AttributeModifier::modifyAttributes (const Attributes& inner)
{
	if(anyDefines ())
	{
		#if MODIFY_INPLACE
		ASSERT (originalAttribs == nullptr)

		#else
		ASSERT (modified == 0)
		if(modified)
		{
			delete modified;
			modified = 0;
		}
		#endif

		CString128 str; 

		for(int i = 0; i < inner.countAttributes (); i++)
		{
			const Attribute* attr = inner.getAttribute (i);
			#if !MODIFY_INPLACE
			bool replaced = false;
			#endif
			if(hasVariables (*attr) && attr->getType () == Attribute::kString)
			{
				str.empty ();
				CStringPtr toResolve = attr->getString ();
				CStringPtr resolved = resolveString (str, toResolve);
				if(toResolve != resolved)
				{
					#if MODIFY_INPLACE
					originalAttribs = const_cast<Attributes*> (&inner);

					// save old value (for restoring later) and replace directly in attribute
					//CORE_PRINTF ("replace %d %s -> %s\n", i, toResolve, resolved);
					if(oldValuesFixed.count () < oldValuesFixed.getCapacity ())
						oldValuesFixed.add (SavedValue (i, toResolve));
					else
						oldValuesDynamic.add (SavedValue (i, toResolve));
					
					const_cast<Attribute*> (attr)->set (resolved);
					#else
					if(modified == 0) 
					{
						// create replacement attributes & copy all so far source attributes
						modified = NEW Attributes;
						for(int j = 0; j < i; j++)
							modified->addAttribute (*inner.getAttribute (j));
					}

					modified->set (attr->getID (), resolved);
					replaced = true;
					#endif
				}			
			}
			
			#if !MODIFY_INPLACE
			if(modified && replaced == false)
				modified->addAttribute (*attr);
			#endif
		}
		#if !MODIFY_INPLACE
		if(modified)
			return *modified;		
		#endif
	}

	return inner;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ViewBuilder::AttributeModifier::resolveString (CString128& tmp, CStringPtr toResolve) const
{
	if(defines && toResolve)
	{
		if(tmp.getBuffer () != toResolve) // when called recursively tmp contains toResolve already
			tmp = toResolve;
		
		bool replaced = false;
		for(int i = 0; i < defines->countAttributes (); i++)
		{
			const Attribute* attr = defines->getAttribute (i);
			ConstString toReplace = attr->getID ().str ();
			ConstString toReplaceWith = attr->getString ();
			ASSERT (!toReplace.isEmpty ()) // empty variable name
			if(toReplaceWith.isEmpty () == false && !toReplace.equalsUnsafe (toReplaceWith))
			{
				while(true)
				{
					int idx = tmp.index (toReplace);
					if(idx < 0)
						break;

					tmp.replace (idx, toReplace.length (), toReplaceWith);
					replaced = true;
				}			
			}
		}

		// allow further replacement on parent level
		if(replaced)
			toResolve = tmp;
	}
	if(parent)
		return parent->resolveString (tmp, toResolve);
	return toResolve;
}
