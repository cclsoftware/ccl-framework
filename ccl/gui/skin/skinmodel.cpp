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
// Filename    : ccl/gui/skin/skinmodel.cpp
// Description : Skin Data Model
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/skin/skinmodel.h"
#include "ccl/gui/skin/skincontrols.h"
#include "ccl/gui/skin/skininteractive.h"
#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skinregistry.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/skin/zoomableview.h"
#include "ccl/gui/gui.h"
#include "ccl/gui/commands.h"

#include "ccl/gui/views/viewanimation.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/layout/flexboxlayout.h"
#include "ccl/gui/layout/workspace.h"
#include "ccl/gui/layout/workspaceframes.h"
#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/imaging/filmstrip.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/tiledimage.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/imagepart.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/coloredbitmap.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/fontresource.h"
#include "ccl/gui/system/accessibility.h"
#include "ccl/gui/theme/colorscheme.h"
#include "ccl/gui/theme/visualstyleselector.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/plugins/iobjecttable.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// SpaceView
//************************************************************************************************

class SpaceView: public View
{
public:
	DECLARE_CLASS (SpaceView, View)

	SpaceView (RectRef size = Rect (), StyleRef style = 0)
	: View (size, style)
	{
		isExplicitlyTransparent (style.isTransparent ());
		this->style.setCommonStyle (Styles::kTransparent, true); // transparent as long as there are no sub-views
	}

private:
	enum PrivateFlags
	{
		kExplicitlyTransparent = 1 << (kLastPrivateFlag + 1)
	};
	PROPERTY_FLAG (privateFlags, kExplicitlyTransparent, isExplicitlyTransparent)

	// View
	void setStyle (StyleRef newStyle) override
	{
		SuperClass::setStyle (newStyle);

		isExplicitlyTransparent (newStyle.isTransparent ());
		style.setCommonStyle (Styles::kTransparent, isExplicitlyTransparent () || views.isEmpty ());
	}

	void onViewsChanged () override
	{
		SuperClass::onViewsChanged ();

		style.setCommonStyle (Styles::kTransparent, isExplicitlyTransparent () || views.isEmpty ());
	}
};

//************************************************************************************************
// NullView
//************************************************************************************************

class NullView: public View
{
public:
	DECLARE_CLASS (NullView, View)

	// View
	void CCL_API setSize (RectRef size, tbool invalidate = true) override
	{
		SuperClass::setSize (Rect (), invalidate);
	}
};

//************************************************************************************************
// CursorView
//************************************************************************************************

class CursorView: public View
{
public:
	DECLARE_CLASS (CursorView, View)

	PROPERTY_SHARED_AUTO (MouseCursor, cursor, Cursor)

	CursorView (RectRef size = Rect (), MouseCursor* _cursor = nullptr)
	: View (size)
	{
		setCursor (_cursor);
	}

	// View
	bool onMouseEnter (const MouseEvent& event) override
	{
		if(cursor)
		{
			SuperClass::setCursor (cursor);
			return true;
		}
		else
			return SuperClass::onMouseEnter (event);
	}
};

//************************************************************************************************
// HelpAnchorView
//************************************************************************************************

class HelpAnchorView: public View
{
public:
	DECLARE_CLASS_ABSTRACT (HelpAnchorView, View)

	HelpAnchorView (const Rect& size, StyleRef style, StringRef helpIdentifier)
	: View (size, style),
	  helpIdentifier (helpIdentifier)
	{}

	// View
	bool setHelpIdentifier (StringRef id) override
	{
		helpIdentifier = id;
		return true;
	}

	StringRef getHelpIdentifier () const override
	{
		return helpIdentifier;
	}

private:
	String helpIdentifier;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SkinElements 
{
	extern void linkSkinViews ();
	extern void linkSkinControls ();
	extern void linkSkinLayouts ();
	extern void linkSkinShapes ();
	extern void linkSkinInteractive ();
	extern void linkSkinElements3D ();
}

namespace SVG 
{
	extern void linkSvgHandler ();
}

} // namespace CCL

using namespace CCL;
using namespace SkinElements;

//************************************************************************************************
// SpaceView / NullView / CursorView / HelpAnchorView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SpaceView, View)
DEFINE_CLASS (NullView, View)
DEFINE_CLASS_UID (NullView, 0x4b1943a5, 0x7591, 0x4c87, 0xb6, 0x21, 0xf5, 0xea, 0x4, 0x53, 0x9f, 0xc)
DEFINE_CLASS_HIDDEN (CursorView, View)
DEFINE_CLASS_ABSTRACT_HIDDEN (HelpAnchorView, View)

//************************************************************************************************
// Element classes
//************************************************************************************************

DEFINE_SKIN_ELEMENT (Resources, Element, TAG_RESOURCES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Resources)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
END_SKIN_ELEMENT_ATTRIBUTES (Resources)

DEFINE_SKIN_ELEMENT (Forms, Element, TAG_FORMS, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Forms)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_FORM)
END_SKIN_ELEMENT_ATTRIBUTES (Forms)

DEFINE_SKIN_ELEMENT (Includes, Element, TAG_INCLUDES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Includes)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_INCLUDE)
END_SKIN_ELEMENT_ATTRIBUTES (Includes)

DEFINE_SKIN_ELEMENT (Imports, Element, TAG_IMPORTS, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Imports)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_IMPORT)
END_SKIN_ELEMENT_ATTRIBUTES (Imports)

DEFINE_SKIN_ELEMENT (Externals, Element, TAG_EXTERNALS, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Externals)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_EXTERNAL)
END_SKIN_ELEMENT_ATTRIBUTES (Externals)

DEFINE_SKIN_ELEMENT (Overlays, Element, TAG_OVERLAYS, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Overlays)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_OVERLAY)
END_SKIN_ELEMENT_ATTRIBUTES (Overlays)

DEFINE_SKIN_ELEMENT (Shapes, Element, TAG_SHAPES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (Shapes)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_SHAPES)
END_SKIN_ELEMENT_ATTRIBUTES (Shapes)

DEFINE_SKIN_ELEMENT (StylesElement, Element, TAG_STYLES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (StylesElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_STYLE)
END_SKIN_ELEMENT_ATTRIBUTES (StylesElement)

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (IncludeElement, Element, TAG_INCLUDE, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_URL, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (IncludeElement)

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ImportElement, Element, TAG_IMPORT, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_URL, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (ImportElement)

DEFINE_SKIN_ELEMENT (ExternalElement, Element, TAG_EXTERNAL, DOC_GROUP_GENERAL, 0)

DEFINE_SKIN_ELEMENT (WindowClassesElement, Element, TAG_WINDOWCLASSES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (WindowClassesElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_WINDOWCLASS)
END_SKIN_ELEMENT_ATTRIBUTES (WindowClassesElement)

DEFINE_SKIN_ELEMENT (WorkspacesElement, Element, TAG_WORKSPACES, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (WorkspacesElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_WORKSPACE)
END_SKIN_ELEMENT_ATTRIBUTES (WorkspacesElement)

//************************************************************************************************
// SkinModel
//************************************************************************************************

SkinModel* SkinModel::getModel (const Element* _e)
{
	for(Element* e = ccl_const_cast (_e); e != nullptr; e = e->getParent ())
		if(SkinModel* model = ccl_cast<SkinModel> (e))
			return model;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SKIN_ELEMENT (SkinModel, Element, TAG_SKIN, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (SkinModel)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
END_SKIN_ELEMENT_ATTRIBUTES (SkinModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel::SkinModel (ISkinContext* context)
: context (context),
  includes (nullptr),
  imports (nullptr),
  overlays (nullptr),
  resources (nullptr),
  stylesElement (nullptr),
  shapes (nullptr),
  forms (nullptr),
  windowClasses (nullptr),
  workspacesElement (nullptr),
  loadingResources (false)
{
	models.setParent (this);
	importedPaths.objectCleanup (true);

	// force linkage
	linkSkinViews ();
	linkSkinControls ();
	linkSkinLayouts ();
	linkSkinShapes ();
	linkSkinInteractive ();
	linkSkinElements3D ();
	SVG::linkSvgHandler ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::removeAll ()
{
	Element::removeAll ();
	models.removeAll (); // additionally remove all sub-models!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinContext* SkinModel::getSkinContext () const
{
	return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* SkinModel::getResource (CStringRef name)
{
	SkinModel* model = nullptr;
	auto resourceElement = getResourceElement<ResourceObjectElement> (name, model);
	if(resourceElement)
	{
		if(!resourceElement->getObject ())
			resourceElement->loadObject (*model);
		return resourceElement->getObject ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* SkinModel::getGradient (CStringRef name, Element* caller)
{
	SkinModel* model = nullptr;
	if(auto gradientElement = getResourceElement<GradientElement> (name, model))
		return gradientElement->getGradient ();
	else
	{
		if(caller != nullptr)
			SKIN_WARNING (caller, "Gradient Element not found: '%s'", name.str ())
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* SkinModel::getImage (CStringRef name, Element* caller)
{
	// check for sub frames
	int subFrameIndex = name.index ('[');
	if(subFrameIndex != -1)
	{
		MutableCString imageName = name.subString (0, subFrameIndex);
		int endIndex = name.lastIndex (']');
		MutableCString frameName = name.subString (subFrameIndex + 1, endIndex - subFrameIndex - 1);
		if(Image* image = getImageInternal (imageName, caller))
		{
			if(Filmstrip* filmstrip = ccl_cast<Filmstrip> (image))
			{
				if(Image* subFrame = filmstrip->getSubFrame (frameName))
					return subFrame;
			}
			else if(MultiImage* multiImage = ccl_cast<MultiImage> (image))
			{
				if(Image* subFrame = multiImage->getFrame (multiImage->getFrameIndex (frameName)))
					return subFrame;
			}
		}

		if(caller != nullptr) // report only when called inside skin, not from application
			SKIN_WARNING (caller, "Image frame not found: '%s[%s]'", imageName.str (), frameName.str ())
		return nullptr;
	}
	else
		return getImageInternal (name, caller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* SkinModel::getImageInternal (CStringRef name, Element* caller)
{
	SkinModel* model = nullptr;
	ImageElement* imageElement = getResourceElement<ImageElement> (name, model);
	if(imageElement)
	{
		if(!imageElement->getImageInternal ())
			imageElement->loadImage (*model);
		return imageElement->getImageInternal ();
	}

	if(caller != nullptr // report only when called inside skin, not from application
		&& !name.startsWith ("@")) // no warning if an image object variable is 0
		SKIN_WARNING (caller, "Image Element not found: '%s'", name.str ())
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinModel::getColorReference (ColorValueReference& reference, StringID name, const Element* caller)
{
	auto resolveColor = [] (Color& color, CStringRef string)
	{
		if(!Colors::fromCString (color, string)) // try theme color by name
			color = Theme::getGlobalStyle ().getColor (string); 
	};

	if(name[0] == '@') 
	{
		int dotIndex = name.index ('.');
		MutableCString schemeName = name.subString (1, dotIndex-1);
		reference.scheme = &ColorSchemes::instance ().get (schemeName);
		reference.nameInScheme = name.subString (dotIndex+1);
		reference.colorValue = reference.scheme->getColor (reference.nameInScheme);
		// TODO: check if item exists in scheme???
		return true;
	}
	else if(name[0] == '$')
	{
		SkinModel* model = nullptr;
		if(ColorElement* colorElement = getResourceElement<ColorElement> (name.subString (1), model))
		{
			resolveColor (reference.colorValue, colorElement->getColor ());
			return true;
		}

		SKIN_WARNING (caller, "Color Element not found: '%s'", name.str ())
		return false;
	}
	else
	{
		resolveColor (reference.colorValue, name);
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/bool SkinModel::getColorFromAttributes (ColorValueReference& reference, const SkinAttributes& a, StringID attrName, const Element* caller)
{
	if(!a.exists (attrName))
		return false;

	return getColorFromString (reference, a.getCString (attrName), caller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/bool SkinModel::getColorFromString (ColorValueReference& reference, StringID string, const Element* caller)
{
	SkinModel* model = getModel (caller);
	ASSERT (model != nullptr)
	if(model)
		return model->getColorReference (reference, string, caller);
	else
		Colors::fromCString (reference.colorValue, string);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* SkinModel::getStyle (CStringRef name, Element* caller)
{
	auto getSingleStyle = [this, caller] (CStringRef name) -> VisualStyle*
	{
		// allow style lookup from other scopes...
		StyleElement* styleElement = nullptr;
		const char* p = ::strrchr (name.str (), '/');
		if(p)
		{
			MutableCString scopeName;
			scopeName.append (name, (int)(p - name.str ()));
			SkinModel* model = getRootModel ().getScopeModel (scopeName);
			if(model)
			{
				CString resolved (++p);
				styleElement = static_cast<StyleElement*> (model->getStylesElement ().findElement (resolved));
			}
		}
		else
			styleElement = static_cast<StyleElement*> (getStylesElement ().findElement (name));

		if(styleElement)
			return &styleElement->getStyle ();
			
		// try public theme styles...
		VisualStyle* themeStyle = getTheme ()->lookupStyle (name);
		if(themeStyle)
			return themeStyle;
		
		if(!loadingResources)
		{
			// report only when called inside skin, not from application (caller == nullptr)
			SKIN_WARNING (caller, "Style Element not found: '%s'", name.str ())
		}

		return nullptr;
	};

	// If a composite style is encountered, the resulting style is computed and added to the model
	// so that future requests for the same style can be resolved just like any regular style.
	bool compositeStyle = name.contains (" ");
	if(!compositeStyle)
		return getSingleStyle (name);

	StyleElement* styleElement = static_cast<StyleElement*> (getStylesElement ().findElement (name));
	if(styleElement == nullptr)
	{
		styleElement = NEW StyleElement ();
		styleElement->setName (name);
		VisualStyle& result = styleElement->getStyle ();
		result.setName (name);

		bool found = false;
		String nameString (name);
		MutableCString inheritedName;
		ForEachStringToken (nameString, " ", token)
			MutableCString cstring (token);
			if(VisualStyle* style = getSingleStyle (cstring))
			{
				found = true;
				result.merge (*style);

				if(style->getTrigger (false))
					result.setTrigger (style->getTrigger (false));

				if(const IVisualStyle* inherited = style->getInherited ())
				{
					if(!inheritedName.isEmpty ())
						inheritedName.append (" ");

					if(style->getName ().startsWith (ThemePainter::kStandardPrefix))
						inheritedName.append ("/"); // set to global scope

					inheritedName.append (inherited->getName ());
				}
			}
		EndFor

		if(!inheritedName.isEmpty ())
			result.setInherited (getStyle (inheritedName, caller));

		if(found)
			getStylesElement ().addChild (styleElement);
		else
			safe_release (styleElement);
	}

	if(styleElement)
		return &styleElement->getStyle ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
T* SkinModel::getResourceElement (CStringRef _name, SkinModel*& model)
{
	CString name (_name);
	model = this;
	T* element = nullptr;

	// allow resource lookup from other scopes...
	const char* p = ::strrchr (name.str (), '/');
	if(p)
	{
		MutableCString scopeName;
		
		int rootIndex = name.index ("://");
		if(rootIndex >= 0) // enable cross-skin references
		{
			Url path (String (name), Url::kFile);
			MutableCString skinID (path.getHostName ());
			SkinWizard* skin = SkinRegistry::instance ().getSkin (skinID);
			if(skin)
			{
				MutableCString pathName (path.getPath ());
				int scopeIndex = pathName.index ("/");
				if(scopeIndex > 0)
					scopeName = pathName.subString (0, scopeIndex);
				model = skin->getScopeModel (scopeName);
			}
		}
		else
		{
			scopeName.append (name, (int)(p - name.str ()));
			model = getRootModel ().getScopeModel (scopeName);
		}
		
		if(model)
		{
			CString resolved (++p);
			element = model->getResources ().findElement<T> (resolved);
		}
	}
	else
		element = model->getResources ().findElement<T> (name);
	return element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::loadResources (bool force)
{
	ScopedVar<bool> scope (loadingResources, true);
	
	{
		FontResource::InstallationScope fontInstallationScope;

		ArrayForEach (getResources (), Element, e)
			if(CursorElement* cursorElement = ccl_cast<CursorElement> (e))
				cursorElement->loadCursor (*this);
			else if(FontResourceElement* fontElement = ccl_cast<FontResourceElement> (e))
				fontElement->loadFont (*this);
			else if(force == true) 
			{
				// load images and other objects only if forced (used for skin imports)
				if(ImageElement* imageElement = ccl_cast<ImageElement> (e))
					imageElement->loadImage (*this);
				else if(ResourceObjectElement* resourceElement = ccl_cast<ResourceObjectElement> (e))
					resourceElement->loadObject (*this);
			}
		EndFor
	}

	{
		// Style elements can be added during loadResources, which affects iteration.
		// The SortingSuspender always appends new elements and resorts afterwards
		Element::SortingSuspender styleSortingSuspender (getStylesElement ());
		ArrayForEach (getStylesElement (), Element, e)
			if(StyleElement* styleElement = ccl_cast<StyleElement> (e))
				styleElement->loadResources (*this);
		EndFor
	}

	ArrayForEach (getWorkspacesElement (), Element, e)
		if(WorkspaceElement* wsElement = ccl_cast<WorkspaceElement> (e))
			wsElement->loadResources (*this);
	EndFor

	ArrayForEach (models, SkinModel, m)
		m->loadResources (force);
	EndFor

	#if 0 && DEBUG
	if(&getRootModel () ==  this)
	{
		struct BitmapMemoryEstimate
		{
			static int getTotalMemory (SkinModel& model)
			{
				int totalMemory = 0;

				ArrayForEach (model.getResources (), Element, e)
					if(ImageElement* imageElement = ccl_cast<ImageElement> (e))
						if(Bitmap* bitmap = ccl_cast<Bitmap> (imageElement->getImage ()))
						{
							Point size (bitmap->getSize ());
							size *= bitmap->getContentScaleFactor ();
							int memory = size.x * size.y * 32;
							totalMemory += memory;
						}
				EndFor

				ArrayForEach (model.getModels (), SkinModel, m)
					totalMemory += getTotalMemory (*m);
				EndFor

				return totalMemory;
			}
		};

		Debugger::printf ("Skin %s Bitmap Memory: %.2f MB\n", getSkinContext ()->getSkinID ().str (), BitmapMemoryEstimate::getTotalMemory (*this)  / 1024. / 1024.);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::reuseResources (SkinModel& sourceModel)
{
	// lookup ImageElements in the sourceModel
	ArrayForEach (getResources (), Element, e)
		if(ImageElement* imageElement = ccl_cast<ImageElement> (e))
			if(ImageElement* sourceImage = sourceModel.getResources ().findElement<ImageElement> (imageElement->getName ()))
				imageElement->reuseImage (*sourceImage);

		// todo: findElement might fail if another resource type has the same name
	EndFor

	ArrayForEach (models, SkinModel, child)
		if(SkinModel* sourceChild = sourceModel.getModels ().findElement<SkinModel> (child->getName ()))
			child->reuseResources (*sourceChild);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::addImportedPath (UrlRef path)
{
	importedPaths.add (NEW Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IContainer* CCL_API SkinModel::getContainerForType (ElementType which)
{
	switch(which)
	{
	// kFontsElement doesn't exist in this model
	case kStylesElement : return stylesElement;
	case kImagesElement : return resources;
	case kFormsElement : return forms;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SkinModel::getImportedPaths (IUnknownList& paths) const
{
	ArrayForEach (importedPaths, Url, path)
		paths.add (path->asUnknown (), true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG
String SkinModel::dumpHelpIdentifiers ()
{
	String result;
	ArrayForEach (getForms (), FormElement, e)
		if(!e->getHelpIdentifier ().isEmpty ())
			result << "ID: \"" << e->getHelpIdentifier () << "\" (Form: " << e->getName () << " File: " << e->getFileName () << ":" << e->getLineNumber () << ")\n";
	EndFor

	ArrayForEach (models, SkinModel, model)
		result << model->dumpHelpIdentifiers ();
	EndFor
	return result;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

#define GET_SKIN_SECTION_METHOD(Method, var, Class)	\
Element& SkinModel::get##Method ()					\
{ if(!var) { var = findElement (ccl_typeid<Class> ());	\
	if(!var) addChild (var = NEW Class); }			\
  return *var; }

//////////////////////////////////////////////////////////////////////////////////////////////////

GET_SKIN_SECTION_METHOD (Includes,  includes,  Includes)
GET_SKIN_SECTION_METHOD (Imports,   imports,   Imports)
GET_SKIN_SECTION_METHOD (Overlays,  overlays,  Overlays)
GET_SKIN_SECTION_METHOD (Resources, resources, Resources)
GET_SKIN_SECTION_METHOD (Forms,     forms,     Forms)
GET_SKIN_SECTION_METHOD (Shapes,    shapes,    Shapes)
GET_SKIN_SECTION_METHOD (StylesElement, stylesElement, StylesElement)
GET_SKIN_SECTION_METHOD (WindowClasses, windowClasses, WindowClassesElement)
GET_SKIN_SECTION_METHOD (WorkspacesElement, workspacesElement, WorkspacesElement)

#undef GET_SKIN_SECTION_METHOD

//////////////////////////////////////////////////////////////////////////////////////////////////

Element& SkinModel::getModels ()
{
	return models;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel& SkinModel::getRootModel ()
{
	SkinModel* parentModel = (SkinModel*)getParent (ccl_typeid<SkinModel> ());
	if(parentModel)
		return parentModel->getRootModel ();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel* SkinModel::getScopeModel (CStringRef scopeName)
{
	SkinModel* current = this;
	if(!scopeName.isEmpty ())
	{
		String _scopeName;
		_scopeName.appendASCII (scopeName.str ());
		ForEachStringToken (_scopeName, Url::strPathChar, name)
			current = current->getModels ().findElement<SkinModel> (MutableCString (name));
			if(!current) // requested scope does not exist!
				break;
		EndFor
	}
	return current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinModel::mergeElements (Element& other)
{
	auto model = ccl_cast<SkinModel> (&other);
	ASSERT (model)
	if(model)
	{
		merge (*model);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::merge (SkinModel& model)
{
	// Note: Includes should already be resolved!

	getResources ().takeElements (model.getResources ());
	getForms ().takeElements (model.getForms ());
	getShapes ().takeElements (model.getShapes ());
	getStylesElement ().takeElements (model.getStylesElement ());
	getWindowClasses ().takeElements (model.getWindowClasses ());
	getWorkspacesElement ().takeElements (model.getWorkspacesElement ());
	getOverlays ().takeElements (model.getOverlays ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinModel::takeSubModels (SkinModel& model)
{
	ForEach (model.getModels (), SkinModel, m)
		m->context = context; // sub-model needs a new context!
	EndFor
	getModels ().takeElements (model.getModels ());
}

//************************************************************************************************
// OverlayElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (OverlayElement, Element, TAG_OVERLAY, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TARGET, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SOURCE, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (OverlayElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

OverlayElement::OverlayElement ()
: overlay (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OverlayElement::~OverlayElement ()
{
	if(overlay)
		SkinRegistry::instance ().removeOverlay (overlay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverlayElement::setAttributes (const SkinAttributes& a)
{
	target = a.getString (ATTR_TARGET);
	source = a.getString (ATTR_SOURCE);
	return Element::setAttributes (a);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OverlayElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_TARGET, target);
	a.setString (ATTR_SOURCE, source);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static String makeFullFormName (StringRef _name, StringID skinID)
{
	String name (_name);
	if(!name.contains ("://"))
	{
		if(!name.startsWith ("/"))
			name.prepend ("/");
		name.prepend (String (skinID));
		name.prepend ("://");
	}
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OverlayElement::loadFinished ()
{
	if(overlay == nullptr)
	{
		StringID skinID = getSkinContext ()->getSkinID ();
		String target (makeFullFormName (getTarget (), skinID));
		String source (makeFullFormName (getSource (), skinID));

		#if (0 && DEBUG)
		Debugger::printf ("Skin overlay: target = '%s' source = '%s'\n", MutableCString (target).str (), MutableCString (source).str ());
		#endif

		overlay = SkinRegistry::instance ().addOverlay (target, source);
	}
}

//************************************************************************************************
// ResourceElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ResourceElement, Element, TAG_RESOURCE, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_URL, TYPE_STRING)	///< url of the file
END_SKIN_ELEMENT_WITH_MEMBERS (ResourceElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ResourceElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
END_SKIN_ELEMENT_ATTRIBUTES (ResourceElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ResourceElement::setAttributes (const SkinAttributes& a)
{
	url = a.getString (ATTR_URL);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ResourceElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_URL, url);
	return Element::getAttributes (a);
}

//************************************************************************************************
// ResourceObjectElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT_ABSTRACT (ResourceObjectElement, ResourceElement, TAG_OBJECT, DOC_GROUP_RESOURCES, 0)

//************************************************************************************************
// ImageElement
//************************************************************************************************

BEGIN_STYLEDEF (ImageElement::tileMethods)
	{"tile-x",		IImage::kTileX},
	{"tile-y",		IImage::kTileY},
	{"repeat-x",	IImage::kRepeatX},
	{"repeat-y",	IImage::kRepeatY},
	{"tile-xy",		IImage::kTileXY},
	{"repeat-xy",	IImage::kRepeatXY},
	{"stretch-xy",	IImage::kStretchXY},
	{"stretch-x",	IImage::kStretchX},
	{"stretch-y",	IImage::kStretchY},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage::TileMethod ImageElement::parseTileMethod (StringRef tile)
{
	IImage::TileMethod method = IImage::kNone;
	if(tile.startsWith ("ti"))
	{
		if(tile.startsWith ("tile-xy"))
			method = IImage::kTileXY;
		else if(tile.startsWith ("tile-y"))
			method = IImage::kTileY;
		else if(tile.startsWith ("tile-x"))
			method = IImage::kTileX;
	}
	else if(tile.startsWith ("re"))
	{
		if(tile.startsWith ("repeat-xy"))
			method = IImage::kRepeatXY;
		else if(tile.startsWith ("repeat-x"))
			method = IImage::kRepeatX;
		else if(tile.startsWith ("repeat-y"))
			method = IImage::kRepeatY;
	}
	else if(tile.startsWith ("st"))
	{
		if(tile.startsWith ("stretch-xy"))
			method = IImage::kStretchXY;
		else if(tile.startsWith ("stretch-x"))
			method = IImage::kStretchX;
		else if(tile.startsWith ("stretch-y"))
			method = IImage::kStretchY;
	}
	return method;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double ImageElement::parseDuration (StringRef string)
{
	double divider = 1.;
	if(string.contains (CCLSTR ("ms")))
		divider = 1000.;
	double duration = 0.;
	string.getFloatValue (duration);
	duration /= divider;
	return duration;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ImageElement, ResourceElement, TAG_IMAGE, DOC_GROUP_RESOURCES, Image)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_IMAGE, TYPE_STRING)	///< name of the image
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FRAMES, TYPE_STRING)	///< number of frames in the image, or space-separated list of frame names
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DURATION, TYPE_FLOAT)	///< duration of a filmstrip animation with this image
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEMPLATE, TYPE_BOOL)	///< template images can be colorized by the framework
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ADAPTIVE, TYPE_BOOL)	///< adaptive images can adapt to the brightness of a given color by the framework
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TILE, TYPE_ENUM)		///< specifies how the image is tiled when used to fill a larger area
	ADD_SKIN_ELEMENT_MEMBER (ATTR_MARGIN, TYPE_RECT)	///< margins used for some tile modes: "left, top, right, bottom"
END_SKIN_ELEMENT_WITH_MEMBERS (ImageElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ImageElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (ImageElement)
DEFINE_SKIN_ENUMERATION (TAG_IMAGE, ATTR_TILE, ImageElement::tileMethods)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageElement::ImageElement ()
: image (nullptr),
  duration (0.),
  isTemplate (false),
  isAdaptive (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageElement::~ImageElement ()
{
	if(image)
		image->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ImageElement::getImageInternal () const
{ 
	return image; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::reuseImage (ImageElement& element)
{
	take_shared (image, element.getImageInternal ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef ImageElement::getAlias () const
{ 
	return alias; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageElement::setAttributes (const SkinAttributes& a)
{
	frames = a.getString (ATTR_FRAMES);
	if(!frames.isEmpty ())
		duration = parseDuration (a.getString (ATTR_DURATION));
	alias = a.getString (ATTR_IMAGE);
	tile = a.getString (ATTR_TILE);
	if(!tile.isEmpty ())
		a.getRect (margins, ATTR_MARGIN); ///< "left, top, right, bottom"
	isTemplate = a.getBool (ATTR_TEMPLATE);
	isAdaptive = a.getBool (ATTR_ADAPTIVE);

	return ResourceElement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageElement::getAttributes (SkinAttributes& a) const
{
	if(!frames.isEmpty ())
		a.setString (ATTR_FRAMES, frames);
	if(!alias.isEmpty ())
		a.setString (ATTR_IMAGE, alias);
	if(!tile.isEmpty ())
		a.setString (ATTR_TILE, tile);
	if(!margins.isEmpty ())
		a.setRect (ATTR_MARGIN, margins);
	if(duration != 0.)
		a.setFloat (ATTR_DURATION, (float)duration);
	if(isTemplate)
		a.setBool (ATTR_TEMPLATE, isTemplate);
	if(isAdaptive)
		a.setBool (ATTR_ADAPTIVE, isAdaptive);

	return ResourceElement::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageElement::loadImage (SkinModel& model)
{
	if(image == nullptr)
	{
		if(alias.isEmpty () == false)
		{
			image = return_shared (model.getImage (alias, this));
			if(image)
			{
				if(image->getIsTemplate ())
					isTemplate = true;
				if(image->getIsAdaptive ())
					isAdaptive = true;
			}
		}
		else if(url.isEmpty ())
		{
			MultiImage* multiImage = NEW MultiImage;
			ForEach (*this, Element, e)
				if(ImageElement* imageElement = ccl_cast<ImageElement> (e))
				{
					if(imageElement->loadImage (model))
						multiImage->addFrame (imageElement->getImageInternal (), imageElement->getName ());
				}
			EndFor
			image = multiImage;
		}
		else
		{
			Url imageUrl;
			makeSkinUrl (imageUrl, url);

			image = Image::loadImage (imageUrl);
			checkImageLoaded (image, imageUrl);
		}

		// apply modification (frames, tile, etc.)
		if(image)
		{
			applyImageModification ();
			image->setIsTemplate (isTemplate);
			image->setIsAdaptive (isAdaptive);
		}
	}
	return image != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::checkImageLoaded (Image* image, UrlRef imageUrl) const
{
	if(image == nullptr)
		SKIN_WARNING (this, "Image not loaded: '%s'", MutableCString (UrlFullString (imageUrl)).str ())
	#if DEBUG
	else
	{
		// check if bitmap size is reasonable
		static const int kMaxSize = 2048;
		auto testSize = [&] (Bitmap* bitmap)
		{
			Point pixelSize (bitmap->getPixelSize ());
			if(pixelSize.x > kMaxSize || pixelSize.y > kMaxSize)
				SKIN_WARNING (this, "Unreasonable bitmap size: %d x %d '%s'", pixelSize.x, pixelSize.y,
							  MutableCString (UrlFullString (imageUrl)).str ())
		};

		if(Bitmap* bitmap = ccl_cast<Bitmap> (image))
		{
			if(MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (bitmap))
				for(int i = 0; i < multiBitmap->getRepresentationCount (); i++)
				{
					MultiResolutionBitmap::RepSelector selector (multiBitmap, i);
					testSize (multiBitmap);
				}
			else
				testSize (bitmap);

			Point size (bitmap->getSize ());
			size *= bitmap->getContentScaleFactor ();
			int memory = size.x * size.y * 32;

			static int totalBitmapMemory = 0;
			totalBitmapMemory += memory;
		}
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImageElement::applyImageModification ()
{
	ASSERT (image != nullptr)

	// check for image filters
	bool schemeDependent = false;
	Vector<ImageFilterElement*> filterElements;
	ArrayForEachFast (*this, Element, e)
		if(ImageFilterElement* filterElement = ccl_cast<ImageFilterElement> (e))
		{
			filterElements.add (filterElement);
			if(!filterElement->getSchemeName ().isEmpty ())
				schemeDependent = true;
		}
	EndFor

	// apply image filters
	if(!filterElements.isEmpty ())
	{
		if(schemeDependent == true)
		{
			ColoredSchemeBitmap* coloredSchemeBitmap = NEW ColoredSchemeBitmap (image);
			VectorForEachFast (filterElements, ImageFilterElement*, filterElement)
				if(IBitmapFilter* filter = filterElement->createFilter ())
				{
					if(filterElement->getSchemeName ().isEmpty ())
						coloredSchemeBitmap->addFilter (filter);
					else
					{
						ColorScheme* scheme = &ColorSchemes::instance ().get (filterElement->getSchemeName ());
						coloredSchemeBitmap->addFilter (filter, scheme, filterElement->getNameInScheme ());
					}
				}
			EndFor
			image->release ();
			image = coloredSchemeBitmap;
		}
		else
		{
			BitmapFilterList filterList;
			VectorForEachFast (filterElements, ImageFilterElement*, filterElement)
				if(IBitmapFilter* filter = filterElement->createFilter ())
					filterList.addFilter (filter);
			EndFor

			BitmapProcessor processor;
			processor.setup (image, Colors::kWhite);
			processor.process (static_cast<IBitmapFilterList&> (filterList));
			take_shared (image, unknown_cast<Image> (processor.getOutput ()));
			ASSERT (image != nullptr)
		}
	}

	if(!frames.isEmpty ())
	{
		if(ShapeImage* shapeImage = ccl_cast<ShapeImage> (image))
			shapeImage->setFilmstrip (true); // frame names are taken from subshapes
		else
		{		
			Filmstrip* filmstrip = NEW Filmstrip (image);
			if(!filmstrip->parseFrameNames (frames))
				SKIN_WARNING (this, "Failed to parse image '%s' frames: %s!", getName ().str (), MutableCString (frames).str ())
			filmstrip->setDuration (duration);
			image->release ();
			image = filmstrip;
		}
	}

	if(!tile.isEmpty ())
	{
		// check margins (see TiledImage::checkMargins())
		if(!(margins.left == 0 && margins.right == 0 && margins.top == 0 && margins.bottom == 0))
		{
			Point size = image->getSize ();
			if(!(margins.left + margins.right < size.x && margins.top + margins.bottom < size.y))
				SKIN_WARNING (this, "Image margins for '%s' larger than source image!", getName ().str ())
		}

		IImage::TileMethod method = parseTileMethod (tile);				
		TiledImage* tiledImage = NEW TiledImage (image, method, margins);
		image->release ();
		image = tiledImage;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ImageElement::getImage () const 
{
	if(!image) // public interface expects image to be loaded
	{
		if(auto model = SkinModel::getModel (this))
			const_cast<ImageElement*> (this)->loadImage (*model);
	}
	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ImageElement::setImage (IImage* _image)
{
	take_shared (image, unknown_cast<Image> (_image)); 
}

//************************************************************************************************
// ImagePartElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ImagePartElement, ImageElement, TAG_IMAGEPART, DOC_GROUP_RESOURCES, ImagePart)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SIZE, TYPE_SIZE)	///< recangle the describes the excerpt area in the original image
END_SKIN_ELEMENT_WITH_MEMBERS (ImagePartElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ImagePartElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_IMAGEFILTER)
END_SKIN_ELEMENT_ATTRIBUTES (ImagePartElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ImagePartElement::applyImageModification ()
{
	ASSERT (image != nullptr)

	// resolve to original
	Rect originalRect;
	SharedPtr<Image> originalImage = image->getOriginalImage (originalRect, true);
	ASSERT (originalImage != nullptr)
	if(originalImage)
		take_shared<Image> (image, originalImage);

	// check part rect
	Rect partRect (this->partRect);
	Rect limits;
	image->getSize (limits);
	partRect.bound (limits);
	if(partRect != this->partRect)
	{
		SKIN_WARNING (this, "ImagePart '%s' is larger than source image!", getName ().str ())
	}
			
	ImagePart* imagePart = NEW ImagePart (image, partRect);
	image->release ();
	image = imagePart;

	// apply frames + tile
	SuperClass::applyImageModification ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImagePartElement::setAttributes (const SkinAttributes& a)
{
	partRect = ElementSizeParser ().trySizeAttributes (a);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImagePartElement::getAttributes (SkinAttributes& a) const
{
	a.setSize (ATTR_SIZE, partRect);

	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// IconSetElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (IconSetElement, ImageElement, TAG_ICONSET, DOC_GROUP_RESOURCES, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (IconSetElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE ("")
END_SKIN_ELEMENT_ATTRIBUTES (IconSetElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IconSetElement::loadImage (SkinModel& model)
{
	if(image == nullptr)
	{
		if(!alias.isEmpty ())
		{
			image = return_shared (model.getImage (alias, this));
			if(image)
			{
				ASSERT (ccl_cast<MultiImage> (image))
				if(image->getIsTemplate ())
					isTemplate = true;
				if(image->getIsAdaptive ())
					isAdaptive = true;
			}
			return true;
		}

		Url iconFolder (nullptr, Url::kFolder);
		makeSkinUrl (iconFolder, url);
		if(System::GetFileSystem ().fileExists (iconFolder))
		{
			image = NEW MultiImage;
			bool allFrames = frames == "all";
			
			int sizeCount = allFrames ? IconSetFormat::kIconSizesAll : IconSetFormat::kIconSizesMin;
			for(int i = 0; i < sizeCount; i++)
			{
				String fileName;
				const IconSetFormat::IconSize& iconSize = IconSetFormat::getIconSizeAt (i);
				IconSetFormat2::makeIconName (fileName, iconSize);

				Url path (iconFolder);
				path.descend (fileName);
				AutoPtr<Image> frame = Image::loadImage (path);
				
				checkImageLoaded (image, path);
				
				if(frame)
					((MultiImage*)image)->addFrame (frame, iconSize.name);
			}
			
			image->setIsTemplate (isTemplate);
			image->setIsAdaptive (isAdaptive);
		}
	}
	return image != nullptr;
}

//************************************************************************************************
// ImageFilterElement
//************************************************************************************************

// copied from ibitmapfilter.h:
BEGIN_STYLEDEF (ImageFilterElement::filterNames)
	{"invert",		0},
	{"grayscale",	0},
	{"alpha",		0},
	{"blend",		0},
	{"lighten",		0},
	{"noise",		0},
	{"tint",		0},
	{"colorize",	0},
	{"adaptlight",	0},
	{"saturate",	0},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ImageFilterElement, Element, TAG_IMAGEFILTER, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR)	///< filter color
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_FLOAT)	///< filter value
END_SKIN_ELEMENT_WITH_MEMBERS (ImageFilterElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ImageFilterElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_IMAGECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (ImageFilterElement)
DEFINE_SKIN_ENUMERATION (TAG_IMAGEFILTER, ATTR_NAME, ImageFilterElement::filterNames)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageFilterElement::ImageFilterElement ()
: value (0.f),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IBitmapFilter* ImageFilterElement::createFilter () const
{
	BitmapFilter* filter = BitmapFilterFactory::createFilter (name);
	if(filter != nullptr)
	{
		if(hasColor ())
			static_cast<IObject*> (filter)->setProperty (IBitmapFilter::kColorID, (int)(uint32)color);
		if(hasValue ())
			static_cast<IObject*> (filter)->setProperty (IBitmapFilter::kValueID, value);
	}
	else
		SKIN_WARNING (this, "Bitmap filter %s not found!\n", name.str ())
	return filter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageFilterElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	
	ColorValueReference ref;
	hasColor (SkinModel::getColorFromAttributes (ref, a, ATTR_COLOR, this));
	setColor (ref.colorValue);
	if(ref.scheme)
	{
		setSchemeName (ref.scheme->getName ());
		setNameInScheme (ref.nameInScheme);
	}

	hasValue (a.exists (ATTR_VALUE));
	if(hasValue ())
		value = a.getFloat (ATTR_VALUE);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageFilterElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	if(hasColor () || a.isVerbose ())
		a.setColor (ATTR_COLOR, color);
	if(hasValue () || a.isVerbose ())
		a.setFloat (ATTR_VALUE, value);
	return true;
}

//************************************************************************************************
// CursorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (CursorElement, Element, TAG_CURSOR, DOC_GROUP_RESOURCES, MouseCursor)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_IMAGE, TYPE_STRING)	///< cursor image
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HOTSPOT, TYPE_POINT)	///< the active point in the image
END_SKIN_ELEMENT_WITH_MEMBERS (CursorElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (CursorElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
END_SKIN_ELEMENT_ATTRIBUTES (CursorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

CursorElement::CursorElement ()
: cursor (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CursorElement::~CursorElement ()
{
	if(cursor)
		cursor->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CursorElement::setAttributes (const SkinAttributes& a)
{
	sourceImage = a.getString (ATTR_IMAGE);
	a.getPoint (hotspot, ATTR_HOTSPOT);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CursorElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_IMAGE, sourceImage);
	a.setPoint (ATTR_HOTSPOT, hotspot);
	return Element::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CursorElement::loadCursor (SkinModel& model)
{
	if(!cursor && !sourceImage.isEmpty ())
	{
		Image* image = model.getImage (sourceImage, this);
		if(image)
		{
			cursor = MouseCursor::createCursor (*image, hotspot);

			// register as theme cursor...
			if(cursor && !getName ().isEmpty ())
				getTheme ()->setCursor (getName (), cursor);
		}
	}
	return cursor != nullptr;
}

//************************************************************************************************
// GradientStopElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (GradientStopElement, Element, TAG_GRADIENTSTOP, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_POSITION, TYPE_FLOAT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR)
END_SKIN_ELEMENT_WITH_MEMBERS (GradientStopElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

GradientStopElement::GradientStopElement ()
: position (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GradientStopElement::setAttributes (const SkinAttributes& a)
{
	String positionString = a.getString (ATTR_POSITION);
	if(positionString.contains (DesignCoord::kStrPercent))
	{
		double percent = 0.;
		positionString.getFloatValue (percent);
		position = float(percent / 100.);
	}
	else
	{
		double value = 0.;
		positionString.getFloatValue (value);
		position = float(value);
	}

	colorString = a.getCString (ATTR_COLOR);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GradientStopElement::getAttributes (SkinAttributes& a) const
{
	a.setFloat (ATTR_POSITION, position);
	a.setString (ATTR_COLOR, colorString);
	return true;
}

//************************************************************************************************
// GradientElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (GradientElement, Element, TAG_GRADIENT, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STARTCOLOR, TYPE_COLOR)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ENDCOLOR, TYPE_COLOR)
END_SKIN_ELEMENT_WITH_MEMBERS (GradientElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (GradientElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_GRADIENTSTOP)
END_SKIN_ELEMENT_ATTRIBUTES (GradientElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GradientElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	if(a.exists (ATTR_STARTCOLOR))
	{
		auto startElement = NEW GradientStopElement;
		startElement->setColorString (a.getCString (ATTR_STARTCOLOR));
		addChild (startElement);
	}
	if(a.exists (ATTR_ENDCOLOR))
	{
		auto endElement = NEW GradientStopElement;
		endElement->setPosition (1.f);
		endElement->setColorString (a.getCString (ATTR_ENDCOLOR));
		addChild (endElement);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool GradientElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	if(a.isVerbose ())
		a.setString (ATTR_STARTCOLOR, CString::kEmpty);
	if(a.isVerbose ())
		a.setString (ATTR_ENDCOLOR, CString::kEmpty);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorGradientStopCollection* GradientElement::createStops () const
{
	auto stops = NEW ColorGradientStopCollection;
	ArrayForEach (*this, Element, e)
		if(auto stopElement = ccl_cast<GradientStopElement> (e))
		{
			ColorGradientStop stop;
			SkinModel::getColorFromString (stop, stopElement->getColorString (), this);
			stop.position = stopElement->getPosition ();
			stops->addStop (stop);
		}
	EndFor
	return stops;
}

//************************************************************************************************
// LinearGradientElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (LinearGradientElement, GradientElement, TAG_LINEARGRADIENT, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_START, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_END, TYPE_POINT)
END_SKIN_ELEMENT_WITH_MEMBERS (LinearGradientElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* LinearGradientElement::getGradient ()
{
	if(!gradient)
	{
		AutoPtr<ColorGradientStopCollection> stops = createStops ();
		gradient = NEW LinearColorGradient (stops, startPoint, endPoint);
	};
	return gradient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinearGradientElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	a.getPointF (startPoint, ATTR_START);
	a.getPointF (endPoint, ATTR_END);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinearGradientElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setPointF (ATTR_START, startPoint);
	a.setPointF (ATTR_END, endPoint);
	return true;
}

//************************************************************************************************
// RadialGradientElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (RadialGradientElement, GradientElement, TAG_RADIALGRADIENT, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CENTER, TYPE_POINT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RADIUS, TYPE_FLOAT)
END_SKIN_ELEMENT_WITH_MEMBERS (RadialGradientElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

RadialGradientElement::RadialGradientElement ()
: radius (0.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* RadialGradientElement::getGradient ()
{
	if(!gradient)
	{
		AutoPtr<ColorGradientStopCollection> stops = createStops ();
		gradient = NEW RadialColorGradient (stops, center, radius);
	};
	return gradient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadialGradientElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	a.getPointF (center, ATTR_CENTER);
	radius = a.getFloat (ATTR_RADIUS);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RadialGradientElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setPointF (ATTR_CENTER, center);
	a.setFloat (ATTR_RADIUS, radius);
	return true;
}

//************************************************************************************************
// FontResourceElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (FontResourceElement, ResourceElement, TAG_FONTRESOURCE, DOC_GROUP_RESOURCES, FontResource)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLE, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (FontResourceElement)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_FONTRESOURCE, ATTR_STYLE, nullptr, TAG_FONT, ATTR_STYLE)

//////////////////////////////////////////////////////////////////////////////////////////////////

FontResourceElement::FontResourceElement ()
: font (nullptr),
  fontStyle (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontResourceElement::~FontResourceElement ()
{
	if(font)
		font->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontResourceElement::loadFont (SkinModel& model)
{
	if(font == nullptr)
	{
		Url path;
		makeSkinUrl (path, url);
		font = FontResource::install (path, fontStyle);
		#if DEBUG
		if(font == nullptr)
		{
			CCL_PRINTLN (String() << "Failed to install Font: " << url)
		}
		#endif
	}
	return font != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontResourceElement::setAttributes (const SkinAttributes& a)
{
	fontStyle = a.getOptions (ATTR_STYLE, FontElement::fontStyles);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontResourceElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_STYLE, fontStyle, FontElement::fontStyles);
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// ControlStatement
//************************************************************************************************

DEFINE_SKIN_ELEMENT_ABSTRACT (ControlStatement, Element, TAG_STATEMENT, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ControlStatement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_SKIN_ELEMENT_ATTRIBUTES (ControlStatement)

//************************************************************************************************
// DefineStatement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (DefineStatement, ControlStatement, TAG_DEFINE, DOC_GROUP_GENERAL, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

DefineStatement::DefineStatement ()
{
	variables.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DefineStatement::setAttributes (const SkinAttributes& a)
{
	bool localize = a.getBool (ATTR_LOCALIZE, true);
	for(int i = 0, count = a.count (); i < count; i++)
	{
		MutableCString name (a.getNameAt (i));
		String value (a.getStringAt (i));

		// implicitly translate known attributes in <define> statement
		if(localize && (SkinAttributes::isEqual (name, ATTR_TITLE) || SkinAttributes::isEqual (name, ATTR_TOOLTIP)))
		{
			#if 0 // disabled, too hard to maintain in translation files!
			if(value.startsWith ("@select:"))
			{
				// translate each element in the list
				int start = value.subString (8).index (":") + 9;
				if(start >= 0)
				{
					String values (value.subString (start));
					value.truncate (start);
					bool first = true;
					ForEachStringToken (values, ",", token)
						value << translate (token);
						if(!__tokenizer->done ())
							value << ",";
					EndFor
				}
			}
			else
			#endif
				value = translate (value);
		}

		MutableCString varName (SkinVariable::kPrefix);
		varName += name;
		variables.add (NEW SkinVariable (varName, Variant (value, true)));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DefineStatement::getAttributes (SkinAttributes& a) const
{
	ForEach (variables, SkinVariable, variable)
		MutableCString name (variable->getName ().str () + 1); // remove variable prefix
		a.setString (name, variable->getValue ().asString ());
	EndFor
	return true;
}

//************************************************************************************************
// UsingStatement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (UsingStatement, ControlStatement, TAG_USING, DOC_GROUP_GENERAL, 0)
/** Relative or absolute path to the new controller. 
Examples:

relative path:
  "Child/GrandChild"
  "../Sibling/Child"

absolute path:
  "object://WindowManager"	*/
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING)	///< relative or absolute path to the new controller
	ADD_SKIN_ELEMENT_MEMBER (ATTR_NAMESPACE, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONAL, TYPE_BOOL)      ///< Flag that controller might not exist
END_SKIN_ELEMENT_WITH_MEMBERS (UsingStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

UsingStatement::UsingStatement (Type type)
: type (type),
  optional (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UsingStatement::setAttributes (const SkinAttributes& a)
{
	setName (a.getString (ATTR_CONTROLLER));
	if(!getName ().isEmpty ())
		type = kController;
	else
	{
		setName (a.getString (ATTR_NAMESPACE));
		type = kNamespace;
	}

	optional = a.getBool (ATTR_OPTIONAL);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UsingStatement::getAttributes (SkinAttributes& a) const
{
	switch(type)
	{
	case kController :
		a.setString (ATTR_CONTROLLER, getName ());
		break;

	case kNamespace :
		a.setString (ATTR_NAMESPACE, getName ());
		break;
	}

	if(optional)
		a.setBool (ATTR_OPTIONAL, optional);
	return true;
}

//************************************************************************************************
// SwitchStatement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (SwitchStatement, ControlStatement, TAG_SWITCH, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)	///< Name of the property or variable ($)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING)	///< Optional controller for evaluating the property or variable
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DEFINED, TYPE_STRING)		///< Interpreted as variable name, evaluates to 1 if the variable exists. Used as an alternative to the "property" attribute
	ADD_SKIN_ELEMENT_MEMBER (ATTR_NOTDEFINED, TYPE_STRING)	///< Same as 'defined', but negates the condition
END_SKIN_ELEMENT_WITH_MEMBERS (SwitchStatement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (SwitchStatement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_SWITCHCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (SwitchStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

SwitchStatement::SwitchStatement ()
: defineNegated (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* SwitchStatement::getCaseElement (VariantRef value)
{
	ArrayForEach (*this, Element, e)
		if(e->canCast (ccl_typeid<CaseStatement> ()))
		{
			CaseStatement& cs = (CaseStatement&)*e;
			
			for(int i = 0; i < cs.cases.count (); i++)
			{
				if(value.isString ())
				{
					if(value == cs.cases[i])
						return e;
				}
				else
				{
					Variant v2;
					v2.fromString (cs.cases[i]);
					if(value == v2)
						return e;
				}
			}
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* SwitchStatement::getDefaultElement () const
{
	return findElement<DefaultStatement> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwitchStatement::setAttributes (const SkinAttributes& a)
{
	setName (a.getString (ATTR_PROPERTY));
	controller = a.getString (ATTR_CONTROLLER);
	defined = MutableCString (a.getString (ATTR_DEFINED));
	defineNegated = false;
	if(defined.isEmpty ())
	{
		defined = MutableCString (a.getString (ATTR_NOTDEFINED));
		if(!defined.isEmpty ())
			defineNegated = true;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SwitchStatement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_PROPERTY, getName ());
	a.setString (ATTR_CONTROLLER, getController ());
	a.setString (ATTR_DEFINED, defined);
	if(a.isVerbose ())
		a.setString (ATTR_NOTDEFINED, String::kEmpty);
	return true;
}

//************************************************************************************************
// CaseStatement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (CaseStatement, ControlStatement, TAG_CASE, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING)	///< The value for which the contained elements should appear
END_SKIN_ELEMENT_WITH_MEMBERS (CaseStatement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (CaseStatement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SWITCHCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (CaseStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CaseStatement::setAttributes (const SkinAttributes& a)
{
	setName (a.getString (ATTR_VALUE));
	
	ForEachStringToken (a.getString (ATTR_VALUE), " ", c)
		cases.add (c.trimWhitespace ());
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CaseStatement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_VALUE, getName ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CaseStatement::loadFinished ()
{
	if(!ccl_cast<SwitchStatement> (getParent ()))
	{
		SKIN_WARNING (this, "Case statement in wrong context.", 0); 
	}
}

//************************************************************************************************
// DefaultStatement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (DefaultStatement, ControlStatement, TAG_DEFAULT, DOC_GROUP_GENERAL, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (DefaultStatement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_SWITCHCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (DefaultStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

void DefaultStatement::loadFinished ()
{
	if(!ccl_cast<SwitchStatement> (getParent ()))
	{
		SKIN_WARNING (this, "Default statement in wrong context.", 0); 
	}
}

//************************************************************************************************
// IfStatement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (IfStatement, SwitchStatement, TAG_IF, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING) ///< The value for which the contained elements should appear
END_SKIN_ELEMENT_WITH_MEMBERS (IfStatement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (IfStatement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_SKIN_ELEMENT_ATTRIBUTES (IfStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IfStatement::setAttributes (const SkinAttributes& a)
{
	value = a.getString (ATTR_VALUE);
	if(value.isEmpty ())
		cases.add ("1");
	else
	{
		ForEachStringToken (value, " ", c)
			cases.add (c.trimWhitespace ());
		EndFor
	}
	return SwitchStatement::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IfStatement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_VALUE, value);
	return SwitchStatement::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element* IfStatement::getCaseElement (VariantRef _value)
{
	for(int i = 0; i < cases.count (); i++)
	{
		if(_value.isString ())
		{
			if(_value == cases[i])
				return this;
		}
		else
		{
			Variant v2;
			v2.fromString (cases[i]);
			if(_value == v2)
				return this;
		}
	}
	return nullptr;
}

//************************************************************************************************
// ForEachStatement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ForEachStatement, ControlStatement, TAG_FOREACH, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VARIABLE, TYPE_STRING) ///< The variable that controls the loop
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COUNT, TYPE_INT)       ///< Number of repetitions. Can be a variable (starting with $).
	ADD_SKIN_ELEMENT_MEMBER (ATTR_START, TYPE_INT)   ///< Start value of the loop variable. Can be a variable (starting with $). Default value is 0.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_IN, TYPE_STRING)       ///< String tokens to be iterated, separted by spaces. Can be a variable (starting with $).
END_SKIN_ELEMENT_WITH_MEMBERS (ForEachStatement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ForEachStatement::setAttributes (const SkinAttributes& a)
{
	setName (a.getString (ATTR_VARIABLE));
	countString = a.getString (ATTR_COUNT);
	startString = a.getString (ATTR_START);
	inString = a.getString (ATTR_IN);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ForEachStatement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_VARIABLE, getName ());
	a.setString (ATTR_COUNT, countString);
	a.setString (ATTR_START, startString);
	a.setString (ATTR_IN, inString);
	return true;
}

//************************************************************************************************
// ZoomStatement
//************************************************************************************************

BEGIN_STYLEDEF (ZoomStatement::modes)
	{"relative",	ZoomStatement::kRelative},
	{"absolute",	ZoomStatement::kAbsolute},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ZoomStatement, ControlStatement, TAG_ZOOM, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FACTOR, TYPE_FLOAT)	///< zoom factor for views
END_SKIN_ELEMENT_WITH_MEMBERS (ZoomStatement)
DEFINE_SKIN_ENUMERATION (TAG_ZOOM, ATTR_MODE, ZoomStatement::modes)

//////////////////////////////////////////////////////////////////////////////////////////////////

ZoomStatement::ZoomStatement ()
: zoomFactor (1.f),
  mode (kRelative)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomStatement::setAttributes (const SkinAttributes& a)
{
	zoomFactor = a.getFloat (ATTR_FACTOR);
	int value = (a.getOptions (ATTR_MODE, modes, true, kRelative));
	setMode (value);
	
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomStatement::getAttributes (SkinAttributes& a) const
{
	a.setFloat (ATTR_FACTOR, zoomFactor);
	a.setOptions (ATTR_MODE, getMode (), modes, true);
	
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// VisualStyleSelectorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (VisualStyleSelectorElement, ControlStatement, TAG_STYLESELECTOR, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VARIABLE, TYPE_STRING)	///< A variable that references the dynamic style (starting with $).
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROPERTY, TYPE_STRING)	///< Name of the property or variable ($)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING)	///< Optional controller for evaluating the property or variable
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLES, TYPE_STRING)		///< The available style names, separted by spaces.
END_SKIN_ELEMENT_WITH_MEMBERS (VisualStyleSelectorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VisualStyleSelectorElement::setAttributes (const SkinAttributes& a)
{
	variableName = a.getString (ATTR_VARIABLE);
	propertyId = a.getString (ATTR_PROPERTY);
	controller = a.getString (ATTR_CONTROLLER);

	ForEachStringToken (a.getString (ATTR_STYLES), " ", style)
		styleNames.add (style);
	EndFor
	
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VisualStyleSelectorElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_VARIABLE, variableName);
	a.setString (ATTR_PROPERTY, propertyId);
	a.setString (ATTR_CONTROLLER, getController ());

	String stylesString;
	for(int i = 0; i < styleNames.count (); i++)
	{
		if(i)
			stylesString << " ";
		stylesString << styleNames[i];
	}
	a.setString (ATTR_STYLES, stylesString);

	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// ViewElement::CreateArgsEx
//************************************************************************************************

ViewElement::CreateArgsEx::CreateArgsEx (ViewElement* element, const CreateArgs& args)
: element (element),
  args (args)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISkinViewElement* CCL_API ViewElement::CreateArgsEx::getElement () const
{
	return element;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewElement::CreateArgsEx::getVariable (Variant& value, StringID _name) const
{
	MutableCString name;
	if(!_name.startsWith (SkinVariable::kPrefix))
	{
		name = SkinVariable::kPrefix;
		name += _name;
	}
	else
		name = _name;

	if(const SkinVariable* v = args.wizard.getVariable (name))
	{
		value = v->getValue ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IVisualStyle* CCL_API ViewElement::CreateArgsEx::getVisualStyleForElement () const
{
	return element->determineVisualStyle (args);
}

//************************************************************************************************
// ElementSizeParser
//************************************************************************************************

static const String kNoneString (VALUE_NONE);

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ElementSizeParser::trySizeAttributes (const SkinAttributes& a)
{
	size.setEmpty ();
	if(!(trySize (a) || tryRect (a)))
	{
		tryWidth (a);
		tryHeight (a);
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::resolveSize (SkinWizard& wizard)
{
	if(mustResolveSize ())
		parseSize (wizard.resolveTitle (sizeString));
	else if(mustResolveRect ())
		parseRect (wizard.resolveTitle (sizeString));
	else
	{
		if(mustResolveWidth ())
		{
			String resolvedWidth = wizard.resolveTitle (widthString);
			resolvedWidth.trimWhitespace ();
			parseWidth (resolvedWidth);
		}
		
		if(mustResolveHeight ())
		{
			String resolvedHeight = wizard.resolveTitle (heightString);
			resolvedHeight.trimWhitespace ();
			parseHeight (resolvedHeight);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementSizeParser::trySize (const SkinAttributes& a)
{
	sizeString = a.getString (ATTR_SIZE);
	if(sizeString.isEmpty ())
		return false;
		
	mustResolveSize (sizeString.contains (SkinVariable::prefix));
	if(!mustResolveSize())
		parseSize (sizeString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ElementSizeParser::tryRect (const SkinAttributes& a)
{
	sizeString = a.getString (ATTR_RECT);
	if(sizeString.isEmpty ())
		return false;
	
	mustResolveRect (sizeString.contains (SkinVariable::prefix));
	if(!mustResolveRect())
		parseRect (sizeString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::tryWidth (const SkinAttributes& a)
{
	widthString = a.getString (ATTR_WIDTH);
	if(widthString.isEmpty ())
		return;
		
	mustResolveWidth (widthString.contains (SkinVariable::prefix));
	if(!mustResolveWidth ())
	{
		widthString.trimWhitespace ();
		parseWidth (widthString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::tryHeight (const SkinAttributes& a)
{
	heightString = a.getString (ATTR_HEIGHT);
	if(heightString.isEmpty ())
		return;
	
	mustResolveHeight (heightString.contains (SkinVariable::prefix));
	if(!mustResolveHeight ())
	{
		heightString.trimWhitespace ();
		parseHeight (heightString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::parseWidth (StringRef resolvedWidth)
{
	SkinAttributes::scanDesignCoord (designSize.width, resolvedWidth);
	if(designSize.width.isCoord ())
		size.setWidth (designSize.width.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::parseHeight (StringRef resolvedHeight)
{
	SkinAttributes::scanDesignCoord (designSize.height, resolvedHeight);
	if(designSize.height.isCoord ())
		size.setHeight (designSize.height.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::parseSize (StringRef resolvedString)
{
	SkinAttributes::scanDesignSize (designSize, resolvedString);
	designSize.toRect (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ElementSizeParser::parseRect (StringRef resolvedString)
{
	SkinAttributes::scanDesignRect (designSize, resolvedString);
	designSize.toRect (size);
}

//************************************************************************************************
// ViewElement
//************************************************************************************************

BEGIN_STYLEDEF (ViewElement::layerBackingTypes)
	{"false",	 ViewElement::kLayerBackingFalse},
	{"true",	 ViewElement::kLayerBackingTrue},
	{"optional", ViewElement::kLayerBackingOptional},
END_STYLEDEF

BEGIN_STYLEDEF (ViewElement::accessibilityTypes)
	{"disabled", ViewElement::kAccessibilityDisabled},
	{"enabled",	 ViewElement::kAccessibilityEnabled},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ViewElement, Element, TAG_VIEW, DOC_GROUP_GENERAL, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SIZE, TYPE_SIZE)  ///< Size of the view in format "left, top, width, height". Alternative forms are "rect" and "width" + "height". 
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RECT, TYPE_RECT)  ///< Size of the view in format "left, top, right, bottom". Alternative forms are "size" and "width" + "height".
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WIDTH, TYPE_METRIC) ///< Width of the view. Alternative forms: "size" and "rect".
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HEIGHT, TYPE_METRIC) ///< Height of the view. Alternative forms: "size" and "rect".
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TITLE, TYPE_STRING)  ///< Title of the view. Some views display their title.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TOOLTIP, TYPE_STRING) ///< Tooltip that appears when mouse rest on the view for a certain time
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ATTACH, TYPE_ENUM) ///< Attachment of the view relative to it's parent
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLE, TYPE_STRING) ///< Name of the style class used to display the view. \see Style
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM) ///< Options that change behavior and appearance of the view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LAYERBACKING, TYPE_ENUM) ///< Enables layer backing
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SIZELIMITS, TYPE_RECT) ///< Minimum and maximum width and height of the view. Specified in order "minWidth, minHeight, maxWidth, maxHeight". Specifying "0" for a minimum or  "-1" for a maximum means unlimited. "none" means completely unlimited.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LAYOUTPRIORITY, TYPE_INT) ///< Priority of this view when placed inside a <Horizontal> or <Vertical> layout with option "hidepriority". Views with lower priority are hidden first, views with priority "-1" are never hidden. "groupdecor" displays its view when there are views between two groupdecor items.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_MINSIZE, TYPE_INT) ///< For views inside a <SizeVariant>: minimum container width or height that selects this view (if there's no view with a higher "data.minsize")
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LOCALIZE, TYPE_BOOL)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TRANSITION, TYPE_ENUM)

	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXGROW, TYPE_FLOAT)		  		///< Defines how remaining space in the parent is distributed relative to the other children's flex grow values
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXSHRINK, TYPE_FLOAT)	  		///< Defines how much an element should shrink relative to other children's flex shrink values, if there is not enough space.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXBASIS, TYPE_METRIC)	   		///< The initial item size along the main axis inside a flexbox. If set to "auto", the size of the item in the main axis direction is used
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXALIGNSELF, TYPE_ENUM)	   		///< Overrides the flexbox item alignment setting when layed out inside a flexbox  [auto, flexstart, flexend, center, stretch]
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXMARGIN, TYPE_STRING)	   		///< Shorthand for individual margins, enter between one and four values which are interpreted as follows: "left=top=right=bottom", "left=right, top=bottom", "left, top, right, bottom=0", "left, top, right, bottom"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXMARGINTOP, TYPE_METRIC)		///< Space added to the top of this element if the parent is a flexbox
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXMARGINRIGHT, TYPE_METRIC)		///< Space added to the right of this element if the parent is a flexbox
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXMARGINBOTTOM, TYPE_METRIC)	///< Space added to the bottom of this element if the parent is a flexbox
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXMARGINLEFT, TYPE_METRIC)		///< Space added to the left of this element if the parent is a flexbox
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXINSET, TYPE_STRING)			///< Shorthand for individual insets, enter between one and four values which are interpreted as follows: "left=top=right=bottom", "left=right, top=bottom", "left, top, right, bottom=0", "left, top, right, bottom"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXINSETTOP, TYPE_METRIC)		///< Distance of the top edge to the corresponding parent edge if positionType is "absolute" or to the calculated top edge of this element if the positionType is "relative"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXINSETRIGHT, TYPE_METRIC)		///< Distance of the right edge to the corresponding parent edge if positionType is "absolute" or to the calculated right edge of this element if the positionType is "relative"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXINSETBOTTOM, TYPE_METRIC)		///< Distance of the bottom edge to the corresponding parent edge if positionType is "absolute" or to the calculated bottom edge of this element if the positionType is "relative"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXINSETLEFT, TYPE_METRIC)		///< Distance of the left edge to the corresponding parent edge if positionType is "absolute" or to the calculated left edge of this element if the positionType is "relative"
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXPOSITIONTYPE, TYPE_ENUM)		///< Elements with absolute position type are excluded from the flow inside a flexbox and positioned absolutely
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FLEXSIZEMODE, TYPE_ENUM)			///< Items can either fill the available space or hug their content [hug, hughorizontal, hugvertical, fill]

	ADD_SKIN_ELEMENT_MEMBER (ATTR_ACCESSIBILITYID, TYPE_STRING)		///< Accessibility id of this view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ACCESSIBILITYPROXY, TYPE_STRING)	///< Name or accessibility id of a child view which acts as the primary accessibility provider of this view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ACCESSIBILITYLABEL, TYPE_STRING)	///< Name or accessibility id of a child view which provides a label for this view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ACCESSIBILITYVALUE, TYPE_STRING)	///< Name or accessibility id of a child view which provides the value for this view
END_SKIN_ELEMENT_WITH_MEMBERS (ViewElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ViewElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
END_SKIN_ELEMENT_ATTRIBUTES (ViewElement)

DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_OPTIONS, View::commonStyles)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_ATTACH, View::resizeStyles)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_TRANSITION, ViewAnimator::transitionTypes)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_LAYERBACKING, ViewElement::layerBackingTypes)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_ACCESSIBILITY, ViewElement::accessibilityTypes)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_FLEXALIGNSELF, FlexItem::flexAlignSelf)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_FLEXPOSITIONTYPE, FlexItem::flexPositionType)
DEFINE_SKIN_ENUMERATION (TAG_VIEW, ATTR_FLEXSIZEMODE, FlexItem::flexSizeMode)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewElement::ViewElement ()
: sizeMode (0),
  dataAttributes (nullptr),
  flexAttributes (nullptr),
  layerBackingType (kLayerBackingFalse),
  accessibilityType (kAccessibilityEnabled),
  accessibilityInfo (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewElement::~ViewElement ()
{
	if(dataAttributes)
		dataAttributes->release ();
	
	if(flexAttributes)
		flexAttributes->release ();

	if(accessibilityInfo)
		delete accessibilityInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableSkinAttributes& ViewElement::getMutableDataAttributes ()
{
	if(dataAttributes == nullptr)
		dataAttributes = NEW MutableSkinAttributes;
	return *dataAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableSkinAttributes& ViewElement::getMutableFlexAttributes ()
{
	if(flexAttributes == nullptr)
		flexAttributes = NEW MutableSkinAttributes;
	return *flexAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewElement::AccessibilityInfo& ViewElement::getAccessibilityInfo ()
{
	if(accessibilityInfo == nullptr)
		accessibilityInfo = NEW AccessibilityInfo;
	return *accessibilityInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const SkinAttributes* ViewElement::getDataAttributes () const
{
	return dataAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const SkinAttributes* ViewElement::getFlexAttributes () const
{
	return flexAttributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewElement::getDataDefinition (String& string, StringID _id) const
{
	if(dataAttributes)
	{
		MutableCString id (ATTR_DATAPREFIX);
		id += _id;
		string = dataAttributes->getString (id);
		return !string.isEmpty ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleFlags CCL_API ViewElement::getStandardOptions () const
{
	return getOptions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewElement::setAttributes (const SkinAttributes& a)
{
	Element::setAttributes (a);
	mustResolveName (getName ().contains (SkinVariable::kPrefix));

	trySizeAttributes (a);

	title = a.getString (ATTR_TITLE);
	tooltip = a.getString (ATTR_TOOLTIP);
	if(a.getBool (ATTR_LOCALIZE, true))
	{
		title = translate (title);
		tooltip = translate (tooltip);
	}
	mustResolveTitle (title.contains (SkinVariable::prefix));
	mustResolveTip (tooltip.contains (SkinVariable::prefix));

	sizeMode = a.getOptions (ATTR_ATTACH, View::resizeStyles);
	styleClass = a.getString (ATTR_STYLE);
	layerBackingType = static_cast<LayerBackingType> (a.getOptions (ATTR_LAYERBACKING, layerBackingTypes, true, kLayerBackingFalse));
	accessibilityType = static_cast<AccessibilityType> (a.getOptions (ATTR_ACCESSIBILITY, accessibilityTypes, true, kAccessibilityEnabled));
	
	sizeLimitsString = a.getString (ATTR_SIZELIMITS);
	if(!sizeLimitsString.isEmpty ())
	{
		if(sizeLimitsString == kNoneString)
			sizeLimits.setUnlimited ();
		else
		{
			mustResolveSizeLimits (sizeLimitsString.contains (SkinVariable::prefix));
			if(!mustResolveSizeLimits ())
			{
				Rect r;
				if(SkinAttributes::scanRect (r, sizeLimitsString))
				{
					sizeLimits = SizeLimit (r);

					// a negative maximum means unlimited
					if(sizeLimits.maxWidth < 0)
						sizeLimits.maxWidth = kMaxCoord;
					if(sizeLimits.maxHeight < 0)
						sizeLimits.maxHeight = kMaxCoord;
				}
			}
		}
	}

	// keep common style flags for base class
	if(isClass (ccl_typeid<ViewElement> ()))
		a.getOptions (options, ATTR_OPTIONS);

	// copy data & flexbox attributes
	ForEachSkinAttribute (a, name, value)
		if(name.startsWith (ATTR_DATAPREFIX))
			getMutableDataAttributes ().setString (name, value);
		else if(name.startsWith (ATTR_FLEXPREFIX))
			getMutableFlexAttributes ().setString (name, value);
	EndFor

	// accessibility
	if(a.exists (ATTR_ACCESSIBILITYID))
		getAccessibilityInfo ().id = a.getCString (ATTR_ACCESSIBILITYID);
	if(a.exists (ATTR_ACCESSIBILITYPROXY))
		getAccessibilityInfo ().proxyId = a.getCString (ATTR_ACCESSIBILITYPROXY);
	if(a.exists (ATTR_ACCESSIBILITYLABEL))
		getAccessibilityInfo ().labelProviderId = a.getCString (ATTR_ACCESSIBILITYLABEL);
	if(a.exists (ATTR_ACCESSIBILITYVALUE))
		getAccessibilityInfo ().valueProviderId = a.getCString (ATTR_ACCESSIBILITYVALUE);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewElement::getAttributes (SkinAttributes& a) const
{
	if(!size.isEmpty () || a.isVerbose ())
	{
		bool sizeSaved = false;
		if(size.left == 0 && size.top == 0)
		{
			a.setInt (ATTR_WIDTH, size.getWidth ());
			a.setInt (ATTR_HEIGHT, size.getHeight ());
			sizeSaved = true;
		}

		if(!sizeSaved || a.isVerbose ())
			a.setSize (ATTR_SIZE, size);
	}

	a.setString (ATTR_TITLE, title);
	a.setString (ATTR_TOOLTIP, tooltip);
	a.setOptions (ATTR_ATTACH, sizeMode, View::resizeStyles);
	a.setString (ATTR_STYLE, styleClass);
	a.setRect (ATTR_SIZELIMITS, sizeLimits);

	if(layerBackingType != kLayerBackingFalse || a.isVerbose ())
		a.setOptions (ATTR_LAYERBACKING, layerBackingType, layerBackingTypes, true);

	if(accessibilityType != kAccessibilityDisabled || a.isVerbose ())
		a.setOptions (ATTR_ACCESSIBILITY, accessibilityType, accessibilityTypes, true);
	
	if(a.isVerbose ())
		a.setBool (ATTR_LOCALIZE, true);

	// append custom styles
	String string;
	appendOptions (string);
	if(!string.isEmpty () || a.isVerbose ())
		a.setString (ATTR_OPTIONS, string);

	// copy data attributes
	if(dataAttributes)
	{
		ForEachSkinAttribute (*dataAttributes, name, value)
			a.setString (name, value);
		EndFor
	}

	// add prototypes for data attributes
	if(a.isVerbose ())
	{
		a.setInt (ATTR_LAYOUTPRIORITY, 0);
		a.setInt (ATTR_MINSIZE, 0);
	}

	// copy flexbox attributes
	if(flexAttributes)
	{
		a.setFloat (ATTR_FLEXGROW, flexAttributes->getFloat (ATTR_FLEXGROW, 0.f));
		a.setFloat (ATTR_FLEXSHRINK, flexAttributes->getFloat (ATTR_FLEXSHRINK, 1.f));
		a.setString (ATTR_FLEXBASIS, flexAttributes->getString (ATTR_FLEXBASIS));
		a.setString (ATTR_FLEXALIGNSELF, flexAttributes->getString (ATTR_FLEXALIGNSELF));
	}

	// accessibility
	if(accessibilityInfo)
	{
		a.setString (ATTR_ACCESSIBILITYID, accessibilityInfo->id);
		a.setString (ATTR_ACCESSIBILITYPROXY, accessibilityInfo->proxyId);
		a.setString (ATTR_ACCESSIBILITYLABEL, accessibilityInfo->labelProviderId);
		a.setString (ATTR_ACCESSIBILITYVALUE, accessibilityInfo->valueProviderId);
	}
	
	return Element::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewElement::viewCreated (View* view)
{
	// autoSize if size is empty in one direction
	tbool autoH = view->getSize ().getWidth () <= 0;
	tbool autoV = view->getSize ().getHeight () <= 0;
	if(autoH || autoV)
		view->autoSize (autoH, autoV);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewElement::viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.common, View::commonStyles);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& ViewElement::getDefaultSize (Rect& r) const
{
	int w = getTheme ()->getThemeMetric (ThemeElements::kButtonWidth);
	int h = getTheme ()->getThemeMetric (ThemeElements::kButtonHeight);
	r (0, 0, w, h);
	r.offset (size.left, size.top);
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* ViewElement::determineVisualStyle (const CreateArgs& args)
{
	if(visualStyle && (styleClass.contains (SkinVariable::kPrefix) || styleClass[0] == '@'))
		visualStyle = nullptr; // reset cached style if style name is a variable

	if(visualStyle == nullptr)
		if(!styleClass.isEmpty ())
			visualStyle = args.wizard.lookupStyle (styleClass, this);

	if(styleClass.isEmpty () == false && visualStyle == nullptr)
	{
		SKIN_WARNING (this, "Style not found: '%s'", styleClass.str ());
	}

	return visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewElement::createView (const CreateArgs& args, View* view)
{
	bool isForm = false;
	if(!view)
	{
		// ask controller if it wants to create the view:
		if(!getName ().isEmpty ())
		{
			SkinWizard::ResolvedName resolvedName (args.wizard, getName (), mustResolveName ());
			CString name = resolvedName.string ();

			if(args.controller)
			{
				UnknownPtr<IViewFactory> viewFactory (args.controller);
				if(viewFactory)
				{
					CreateArgsEx args2 (this, args);
					view = unknown_cast<View> (viewFactory->createView (name, Variant (args2.asUnknown ()), getSize ()));
				}
			}

			// maybe it is a reference to another <Form>
			if(!view)
			{
				if(name.contains ("://")) // enable cross-skin references
				{
					Attributes arguments;
					args.wizard.getVariables (arguments);
					view = SkinRegistry::instance ().createView (name, args.controller, &arguments);
				}
				else
					view = args.wizard.createView (name, args.controller);

				if(view)
					isForm = true;
			}
		}

		if(!view)
		{
			if(!getName ().isEmpty ())
			{
				SKIN_WARNING (this, "View not found: '%s'", getName ().str ());
			}
			view = NEW View (size);
		}
	}

	// assign theme
	view->setTheme (getTheme ());

	// apply view attributes
	// TO BE TESTED: do not overwrite attributes if it is another <Form>!
	if(!isForm)
	{
		SkinWizard::ResolvedName resolvedName (args.wizard, getName (), mustResolveName ());
		view->setName (String (resolvedName.string ()));

		if(!title.isEmpty ())
			view->setTitle (mustResolveTitle () ? args.wizard.resolveTitle (title) : title);
		if(!tooltip.isEmpty ())
			view->setTooltip (mustResolveTip () ? args.wizard.resolveTitle (tooltip) : tooltip);
	}
	
	resolveSize (args.wizard);
	
	Rect r;
	calculateViewSize (r, view);
	applyZoomFactor (r, view, args);
	view->setSize (r);

	// Don't overide sizemode flags from views that are created by controller
	if(sizeMode || isForm)
		view->setSizeMode (sizeMode);

	if(mustResolveSizeLimits ())
	{
		Rect resolvedLimits;
		if(SkinAttributes::scanRect (resolvedLimits, args.wizard.resolveTitle (sizeLimitsString)))
			sizeLimits = SizeLimit (resolvedLimits);
	}
			
	if(sizeLimits.isValid ())
		view->setSizeLimits (sizeLimits);

	// visual style
	if(!view->hasVisualStyle ())
	{
		if(auto vs = determineVisualStyle (args))
			view->setVisualStyle (vs);
	}

	// layer backing
	if(layerBackingType != kLayerBackingFalse)
	{
		if(layerBackingType == kLayerBackingOptional)
		{
			// enable optional layer-backing only if platform doesn't support partial updates
			#if CCL_PLATFORM_IOS
			view->setLayerBackingEnabled (true);
			#endif
		}
		else
			view->setLayerBackingEnabled (true);
	}
	
	// accessibility
	if(accessibilityType == kAccessibilityDisabled)
		view->setAccessibilityEnabled (false);
	
	if(accessibilityInfo && !accessibilityInfo->isEmpty ())
	{
		SkinWizard::ResolvedName resolvedAccessibilityName (args.wizard, accessibilityInfo->id.isEmpty () ? name : accessibilityInfo->id);
		SkinWizard::ResolvedName resolvedAccessibilityProxy (args.wizard, accessibilityInfo->proxyId);
		SkinWizard::ResolvedName resolvedAccessibilityLabel (args.wizard, accessibilityInfo->labelProviderId.isEmpty () ? accessibilityInfo->proxyId: accessibilityInfo->labelProviderId);
		SkinWizard::ResolvedName resolvedAccessibilityValue (args.wizard, accessibilityInfo->valueProviderId.isEmpty () ? accessibilityInfo->proxyId : accessibilityInfo->valueProviderId);

		AccessibilityManager& manager = AccessibilityManager::instance ();
		manager.registerAccessibleView (view, resolvedAccessibilityName.string ());
		manager.setViewRelation (view, AccessibilityRelation::kProxy, resolvedAccessibilityProxy.string ());
		manager.setViewRelation (view, AccessibilityRelation::kLabel, resolvedAccessibilityLabel.string ());
		manager.setViewRelation (view, AccessibilityRelation::kValue, resolvedAccessibilityValue.string ());
	}

	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewElement::calculateViewSize (Rect& r, View* view) const
{
	r = size;
	if(size.getWidth () <= 0)
		r.setWidth (view->getWidth ());
	
	if(size.getHeight () <= 0)
		r.setHeight (view->getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewElement::applyZoomFactor (Rect& r, View* view, const CreateArgs& args) const
{
	float zoomFactor = args.wizard.getZoomFactor ();
	if(view->getZoomFactor () != zoomFactor)
	{
		view->setZoomFactor (zoomFactor);
		r.zoom (zoomFactor);
	}
	else if(zoomFactor != 1.f)
	{
		// the view might have been zoomed already if it was created via IViewFactory, we must avoid zooming it again if no target size is specified
		// but an explicitely specified position / size in this element must be zoomed & applied
		Point position (r.getLeftTop ());
		position *= zoomFactor;
		r.moveTo (position);

		if(size.getWidth () > 0)
			r.setWidth (zoomFactor * size.getWidth ());

		if(size.getHeight () > 0)
			r.setHeight (zoomFactor * size.getHeight ());
	}
}

//************************************************************************************************
// ImageViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ImageViewElement, ViewElement, TAG_IMAGEVIEW, DOC_GROUP_GENERAL, ImageView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_IMAGENAME, TYPE_STRING)	///< name of an image resource
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SELECTNAME, TYPE_STRING)	///< name of a parameter that selects the image frame ("normal" or "pressed")
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PROVIDER, TYPE_STRING)	///< name of an application object that can provide an image
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DATATARGET, TYPE_STRING)	///< name of an application object that manages dragging data onto the image
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TRANSITION, TYPE_ENUM)	///< an animation that is performed when the image changes
END_SKIN_ELEMENT_WITH_MEMBERS (ImageViewElement)
DEFINE_SKIN_ENUMERATION (TAG_IMAGEVIEW, ATTR_OPTIONS, ImageView::customStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

ImageViewElement::ImageViewElement ()
: transitionType (Styles::kTransitionNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageViewElement::setAttributes (const SkinAttributes& a)
{
	imageName = a.getString (ATTR_IMAGENAME);
	selectName = a.getString (ATTR_SELECTNAME);
	providerName = a.getString (ATTR_PROVIDER);
	dataTargetName = a.getString (ATTR_DATATARGET);
	a.getOptions (imageStyle, ATTR_OPTIONS, ImageView::customStyles);
	transitionType = a.getOptions (ATTR_TRANSITION, ViewAnimator::transitionTypes, true, Styles::kTransitionNone);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ImageViewElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_IMAGENAME, imageName);
	a.setString (ATTR_SELECTNAME, selectName);
	a.setString (ATTR_PROVIDER, providerName);
	a.setString (ATTR_DATATARGET, dataTargetName);
	a.setOptions (ATTR_OPTIONS, imageStyle, ImageView::customStyles);
	a.setOptions (ATTR_TRANSITION, transitionType, ViewAnimator::transitionTypes, true);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ImageViewElement::createView (const CreateArgs& args, View* view)
{
	if(imageName && (imageName.contains (SkinVariable::kPrefix)))
	{
		SkinWizard::ResolvedName resolvedName (args.wizard, imageName);
		image = args.wizard.getModel ().getImage (resolvedName.string (), this);
	}
	
	if(image == nullptr)
	{
		if(!imageName.isEmpty ())
			image = args.wizard.getModel ().getImage (imageName, this);
	}

	ImageView* imageView = (ImageView*)view;
	if(!imageView)
		imageView = NEW ImageView (image, size, imageStyle);
	else
		imageView->setBackground (image);

	if(!providerName.isEmpty ())
	{
		UnknownPtr<IImageProvider> imageProvider (ControlElement::getParameter (args, providerName, this));
		imageView->setImageProvider (imageProvider);
	}

	if(!selectName.isEmpty ())
		imageView->setSelectParam (ControlElement::getParameter (args, selectName, this));

	if(!dataTargetName.isEmpty ())
	{
		UnknownPtr<IDataTarget> dataTarget = ControlElement::getObject (args, dataTargetName, ccl_iid<IDataTarget> ());
		imageView->setDataTarget (dataTarget);
	}

	imageView->setTransitionType (transitionType);

	return SuperClass::createView (args, imageView);
}

//************************************************************************************************
// FormElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (FormElement, ImageViewElement, TAG_FORM, DOC_GROUP_GENERAL, Form)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WINDOWSTYLE, TYPE_ENUM) ///< The style applied to the window when the form is opened as window.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DIALOGBUTTONS, TYPE_ENUM) 
/** The name of a view in the form that will become the focus view when the form opens. 
If this optional name is specified, the child views of the form are searched (depth-first) to find the first view with that name. 
	
This view becomes focus view e.g. when the form is used to open a dialog or window.  */
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FIRSTFOCUS, TYPE_STRING) 
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HELPIDENTIFIER, TYPE_STRING) ///< A string id that refers to a page in the user documentation
END_SKIN_ELEMENT_WITH_MEMBERS (FormElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (FormElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE ("") // remove inherited schema groups
END_SKIN_ELEMENT_ATTRIBUTES (FormElement)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_FORM, ATTR_WINDOWSTYLE, Window::windowStyles, TAG_VIEW, ATTR_OPTIONS)
DEFINE_SKIN_ENUMERATION (TAG_FORM, ATTR_OPTIONS, Form::customStyles)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_FORM, ATTR_DIALOGBUTTONS, nullptr, TAG_DIALOGBUTTON, ATTR_RESULT)

//////////////////////////////////////////////////////////////////////////////////////////////////

FormElement::FormElement ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (windowStyle, ATTR_WINDOWSTYLE, Window::windowStyles);
	formStyle.custom = a.getOptions (ATTR_DIALOGBUTTONS, Dialog::dialogButtons);
	firstFocus = a.getString (ATTR_FIRSTFOCUS);
	helpIdentifier = a.getString (ATTR_HELPIDENTIFIER);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, formStyle, Form::customStyles);
	a.setOptions (ATTR_WINDOWSTYLE, windowStyle, Window::windowStyles);
	a.setOptions (ATTR_DIALOGBUTTONS, formStyle.custom, Dialog::dialogButtons);
	a.setString  (ATTR_FIRSTFOCUS, firstFocus);
	a.setString  (ATTR_HELPIDENTIFIER, helpIdentifier);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FormElement::createView (const CreateArgs& args, View* view)
{
	Form* form = nullptr;
	if(!view)
	{
		StyleFlags style (imageStyle);
		style.custom |= formStyle.custom;
		view = form = NEW Form (&args.wizard, size, style);
	}
	else
		form = ccl_cast<Form> (view);

	if(form)
	{
		form->setSkinElement (this);
		form->setController (args.controller);
		if(!firstFocus.isEmpty ())
			form->setFirstFocus (firstFocus);
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// FormDelegateElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (FormDelegateElement, ViewElement, TAG_FORMDELEGATE, DOC_GROUP_GENERAL, FormDelegateView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORMNAME, TYPE_STRING)  ///< The name of the form for creating the content view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING) ///< The name of the controller for creating the content view. Can be a sub controller of the current controller, or an absolute controler path.
END_SKIN_ELEMENT_WITH_MEMBERS (FormDelegateElement)
DEFINE_SKIN_ENUMERATION (TAG_FORMDELEGATE, ATTR_OPTIONS, FormDelegateView::customStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FormDelegateElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		FormDelegateView* delegateView = NEW FormDelegateView (&args.wizard, Rect (), options);
		delegateView->setFormController (args.controller);

		SkinWizard::ResolvedName resolvedControllerName (args.wizard, controllerName); // allow <Delegate controller="$...">
		SkinWizard::ResolvedName resolvedFormName (args.wizard, formName); // allow <Delegate form.name="$...">

		delegateView->setSubControllerName (resolvedControllerName.string ());
		delegateView->setFormName (resolvedFormName.string ());
		args.wizard.getVariables (delegateView->getFormArguments ());
		view = delegateView;
	}
	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormDelegateElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	formName = a.getString (ATTR_FORMNAME);
	controllerName = a.getString (ATTR_CONTROLLER);
	a.getOptions (options, ATTR_OPTIONS, FormDelegateView::customStyles);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormDelegateElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_FORMNAME, formName);
	a.setString (ATTR_CONTROLLER, controllerName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FormDelegateElement::appendOptions (String& string) const
{
	SkinAttributes::makeOptionsString (string, options.custom, FormDelegateView::customStyles);
	return SuperClass::appendOptions (string);
}

//************************************************************************************************
// ZoomableViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ZoomableViewElement, ViewElement, TAG_ZOOMABLE, DOC_GROUP_GENERAL, ZoomableView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORMNAME, TYPE_STRING)  ///< The name of the form for creating the content view
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FACTORS, TYPE_STRING)  ///< List of supported zoom factors (no restriction if empty)
END_SKIN_ELEMENT_WITH_MEMBERS (ZoomableViewElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ZoomableViewElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		SkinWizard::ResolvedName resolvedFormName (args.wizard, formName);

		auto* zoomableView = NEW ZoomableView (Rect (), options);
		zoomableView->setSupportedZoomfactors (supportedZoomfactors);
		zoomableView->setFormController (args.controller);
		zoomableView->setFormName (resolvedFormName.string ());
		args.wizard.getVariables (zoomableView->getFormArguments ());
		view = zoomableView;
	}
	return SuperClass::createView (args, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomableViewElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	formName = a.getString (ATTR_FORMNAME);

	ForEachStringToken (a.getString (ATTR_FACTORS), " ", token)
		float factor = token.scanFloat ();
		if(factor > 0.f)
			supportedZoomfactors.addSorted (factor);
	EndFor
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ZoomableViewElement::getAttributes (SkinAttributes& a) const
{
	SuperClass::getAttributes (a);
	a.setString (ATTR_FORMNAME, formName);

	if(!supportedZoomfactors.isEmpty ())
	{
		String factorsString;
		for(float f : supportedZoomfactors)
		{
			if(!factorsString.isEmpty ())
				factorsString << " ";
			factorsString << f;
		}
		a.setString (ATTR_FACTORS, factorsString);
	}
	return true;
}

//************************************************************************************************
// CursorViewElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (CursorViewElement, ImageViewElement, TAG_CURSORVIEW, DOC_GROUP_GENERAL, CursorView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CURSOR, TYPE_STRING)  ///< Name of a cursor resource.
END_SKIN_ELEMENT_WITH_MEMBERS (CursorViewElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CursorViewElement::setAttributes (const SkinAttributes& a)
{
	cursorName = a.getString (ATTR_CURSOR);

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CursorViewElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_CURSOR, cursorName);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* CursorViewElement::createView (const CreateArgs& args, View* view)
{
	if(view == nullptr)
	{
		MouseCursor* cursor = unknown_cast<MouseCursor> (getTheme ()->getCursor (cursorName));
		ASSERT (cursor != nullptr)
		if(cursor == nullptr)
			SKIN_WARNING (this, "Cursor not found: '%s'", cursorName.str ())

		view = NEW CursorView (size, cursor);
	}

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// HelpAnchorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (HelpAnchorElement, ViewElement, TAG_HELPANCHOR, DOC_GROUP_GENERAL, HelpInfoView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HELPIDENTIFIER, TYPE_STRING) ///< A string id that refers to a page in the user documentation
END_SKIN_ELEMENT_WITH_MEMBERS (HelpAnchorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpAnchorElement::setAttributes (const SkinAttributes& a)
{
	helpIdentifier = a.getString (ATTR_HELPIDENTIFIER);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HelpAnchorElement::getAttributes (SkinAttributes& a) const
{
	a.setString  (ATTR_HELPIDENTIFIER, helpIdentifier);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* HelpAnchorElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW HelpAnchorView (size, options, getHelpIdentifier ());

	return SuperClass::createView (args, view);
}

//************************************************************************************************
// WindowClassElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (WindowClassElement, Element, TAG_WINDOWCLASS, DOC_GROUP_WORKSPACE, WindowClass)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_NAME, TYPE_STRING)		///< general identifier
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TITLE, TYPE_STRING)		///< window class title visible to user
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORMNAME, TYPE_STRING)	///< form name
	ADD_SKIN_ELEMENT_MEMBER (ATTR_GROUP, TYPE_STRING)		///< associates window class with a frame in a perspective
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CMDCATEGORY, TYPE_STRING)	///< command category
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CMDNAME, TYPE_STRING)		///< command name
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VISIBLE, TYPE_BOOL)		///< true if window should be visible by default
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTROLLER, TYPE_STRING)	///< object table url of associated controller
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WORKSPACE, TYPE_STRING)	///< in which workspace this should appear
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PERSISTENCE_ID, TYPE_STRING)	///< storage id used to store and restore the layout state
END_SKIN_ELEMENT_WITH_MEMBERS (WindowClassElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClassElement::WindowClassElement ()
: windowClass (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClassElement::~WindowClassElement ()
{
	if(windowClass)
	{
		if(WindowManager::peekInstance ())
		{
			if(WindowManager::instance ().isClassRegistered (windowClass))
				WindowManager::instance ().unregisterClass (windowClass);
		}
		windowClass->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowClass& WindowClassElement::getWindowClass () const
{
	if(!windowClass)
		windowClass = NEW WindowClass;
	return *windowClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowClassElement::setAttributes (const SkinAttributes& a)
{
	Element::setAttributes (a);

	WindowClass& wc = getWindowClass ();

	wc.setID (MutableCString (a.getString (ATTR_NAME)));
	wc.setTitle (translate (a.getString (ATTR_TITLE)));
	wc.setFormName (a.getString (ATTR_FORMNAME));
	wc.setGroupID (a.getString (ATTR_GROUP));
	wc.setCommandCategory (a.getString (ATTR_CMDCATEGORY));
	wc.setCommandName (a.getString (ATTR_CMDNAME));
	wc.setDefaultVisible (a.getBool (ATTR_VISIBLE));
	wc.setControllerUrl (a.getString (ATTR_CONTROLLER));
	wc.setWorkspaceID (MutableCString (a.getString (ATTR_WORKSPACE)));
	wc.setStorageID (MutableCString (a.getString (ATTR_PERSISTENCE_ID)));
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowClassElement::getAttributes (SkinAttributes& a) const
{
	WindowClass& wc = getWindowClass ();

	a.setString (ATTR_NAME, wc.getID ());
	a.setString (ATTR_TITLE, wc.getTitle ());
	a.setString (ATTR_FORMNAME, wc.getFormName ());
	a.setString (ATTR_GROUP, wc.getGroupID ());
	a.setString (ATTR_CMDCATEGORY, wc.getCommandCategory ());
	a.setString (ATTR_CMDNAME, wc.getCommandName ());
	a.setBool (ATTR_VISIBLE, wc.isDefaultVisible ());
	a.setString (ATTR_CONTROLLER, wc.getControllerUrl ());
	a.setString (ATTR_WORKSPACE, wc.getWorkspaceID ());
	a.setString (ATTR_PERSISTENCE_ID, wc.getStorageID ());
	return Element::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowClassElement::loadFinished ()
{
	if(windowClass)
	{
		windowClass->retain ();
		windowClass->setTheme (getTheme ());
		if(windowClass->getWorkspaceID ().isEmpty ())
			windowClass->setWorkspaceID (getSkinContext ()->getSkinID ());
		WindowManager::instance ().registerClass (windowClass);

		// register command
		if(!windowClass->getCommandCategory ().isEmpty () && !windowClass->getCommandName ().isEmpty ())
		{
			CommandDescription description;
			description.category = MutableCString (windowClass->getCommandCategory ());
			description.name = MutableCString (windowClass->getCommandName ());
			description.displayCategory = translateWithScope ("Command", windowClass->getCommandCategory ());
			description.displayName = translateWithScope ("Command", windowClass->getCommandName ());
			description.englishName = description.name;
			description.arguments = "State";
			
			CommandTable::instance ().registerCommand (description);
		}
	}
}

//************************************************************************************************
// WorkspaceElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (WorkspaceElement, Element, TAG_WORKSPACE, DOC_GROUP_WORKSPACE, Workspace)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WINDOWSTYLE, TYPE_ENUM)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STORABLE, TYPE_BOOL)
END_SKIN_ELEMENT_WITH_MEMBERS (WorkspaceElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (WorkspaceElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_PERSPECTIVE)
END_SKIN_ELEMENT_ATTRIBUTES (WorkspaceElement)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_WORKSPACE, ATTR_WINDOWSTYLE, nullptr, TAG_FORM, ATTR_WINDOWSTYLE)

ObjectList WorkspaceElement::workspaceCleanupList;

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkspaceElement::WorkspaceElement ()
: windowStyle (Styles::panelWindowStyle),
  workspace (nullptr),
  storable (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WorkspaceElement::~WorkspaceElement ()
{
	discardWorkspace ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceElement::setAttributes (const SkinAttributes& a)
{
	windowStyle.common = a.getOptions (ATTR_WINDOWSTYLE, View::commonStyles);
	windowStyle.custom = a.getOptions (ATTR_WINDOWSTYLE, Window::windowStyles, false, Styles::kWindowCombinedStylePanel);
	storable = a.getBool (ATTR_STORABLE, false);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WorkspaceElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_WINDOWSTYLE, windowStyle, Window::windowStyles);
	a.setBool (ATTR_STORABLE, storable);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WorkspaceElement::getWorkspaceID () const
{
	return getName ().isEmpty () ? getSkinContext ()->getSkinID () : getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace* WorkspaceElement::createWorkspace (SkinModel& model)
{
	Workspace* workspace = NEW Workspace;
	workspace->setID (getWorkspaceID ());
	workspace->setTheme (getTheme ());
	workspace->setWindowStyle (getWindowStyle ());
	workspace->setStorable (storable);

	ForEach (*this, Element, e)
		PerspectiveElement* perspectiveElement = ccl_cast<PerspectiveElement> (e);
		if(perspectiveElement)
		{
			Perspective* perspective = perspectiveElement->createPerspective (model);
			workspace->addPerspective (perspective);
		}
	EndFor

	workspace->restore (Window::getWindowSettings ());
	return workspace;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceElement::loadResources (SkinModel& model)
{
	if(SkinWizard::isReloadingSkin ()) // don't add again during reload, previous workspace stays registered (see discardWorkspace)
		return;

	discardWorkspace ();

	workspace = createWorkspace (model);
	WorkspaceSystem::instance ().addWorkspace (workspace);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WorkspaceElement::discardWorkspace ()
{
	if(workspace)
	{
		if(SkinWizard::isReloadingSkin ())
		{
			// during reload, keep workspaces registered in WorkspaceSystem, ensure cleanup when terminating app
			// (application code might keep pointers to workspaces, spy injects a "SkinRefresh" perspective into app workspace)
			workspaceCleanupList.objectCleanup (true);
			workspaceCleanupList.add (workspace);
		}
		else
		{
			if(WorkspaceSystem::peekInstance ())
				WorkspaceSystem::instance ().removeWorkspace (workspace);

			workspace->release ();
		}
		workspace = nullptr;
	}
}

//************************************************************************************************
// PerspectiveElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (PerspectiveElement, Element, TAG_PERSPECTIVE, DOC_GROUP_WORKSPACE, Perspective)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TITLE, TYPE_STRING)		///< title visible to user
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ICON, TYPE_STRING)		///< icon
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM)		///< \see Perspective.customStyles
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ORIENTATION, TYPE_ENUM)	///< \see Perspective.orientation
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TRANSITION, TYPE_ENUM)	///< transition used when perspective is selected
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLE, TYPE_STRING)		///< name of an optional style class used to draw the background of the full perspective. \see Style
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BACKGROUNDOPTIONS, TYPE_STRING) ///< options for drawing background. \see ImageView
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BACKCMDCATEGORY, TYPE_STRING)	///< command category for "back" navigation from this perspective
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BACKCMDNAME, TYPE_STRING)	///< command name for "back" navigation from this perspective
END_SKIN_ELEMENT_WITH_MEMBERS (PerspectiveElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (PerspectiveElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_PERSPECTIVECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (PerspectiveElement)
DEFINE_SKIN_ENUMERATION (TAG_PERSPECTIVE, ATTR_OPTIONS, Perspective::customStyles)
DEFINE_SKIN_ENUMERATION (TAG_PERSPECTIVE, ATTR_ORIENTATION, Perspective::orientations)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_PERSPECTIVE, ATTR_TRANSITION, nullptr, TAG_VIEW, ATTR_TRANSITION)

//////////////////////////////////////////////////////////////////////////////////////////////////

PerspectiveElement::PerspectiveElement ()
: orientation (Styles::kAnyOrientation),
  transitionType (Styles::kTransitionNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveElement::setAttributes (const SkinAttributes& a)
{
	title = translate (a.getString (ATTR_TITLE));
	iconName = a.getString (ATTR_ICON);
	a.getOptions (style, ATTR_OPTIONS, Perspective::customStyles);
	orientation = a.getOptions (ATTR_ORIENTATION, Perspective::orientations);
	transitionType = a.getOptions (ATTR_TRANSITION, ViewAnimator::transitionTypes, true, Styles::kTransitionNone);
	styleClass = a.getString (ATTR_STYLE);
	a.getOptions (backgroundOptions, ATTR_BACKGROUNDOPTIONS, ImageView::customStyles);
	backCommandCategory = a.getString (ATTR_BACKCMDCATEGORY);
	backCommandName = a.getString (ATTR_BACKCMDNAME);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PerspectiveElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_TITLE, title);
	a.setString (ATTR_ICON, iconName);
	a.setOptions (ATTR_OPTIONS, style, FrameItem::customStyles);
	a.setOptions (ATTR_OPTIONS, orientation, Perspective::orientations);
	a.setOptions (ATTR_TRANSITION, transitionType, ViewAnimator::transitionTypes, true);
	a.setString (ATTR_STYLE, styleClass);
	a.setOptions (ATTR_BACKGROUNDOPTIONS, backgroundOptions, ImageView::customStyles);
	a.setString (ATTR_BACKCMDCATEGORY, backCommandCategory);
	a.setString (ATTR_BACKCMDNAME, backCommandName);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective* PerspectiveElement::createPerspective (SkinModel& model)
{
	RootFrameItem* rootFrame = NEW RootFrameItem;

	// check if there is a single group frame that can be the root frame
	FrameElement* singleRootElement = nullptr;
	ObjectList paramElements;

	int numRootFrames = 0;
	ForEach (*this, Element, e)
		if(FrameElement* frameElement = ccl_cast<FrameElement> (e))
		{
			numRootFrames++;
			if(numRootFrames > 1)
			{
				singleRootElement = nullptr;
				break;
			}
			else if(frameElement->isGroup ())
				singleRootElement = frameElement;
		}
	EndFor

	ForEach (*this, Element, e)
		if(FrameElement* frameElement = ccl_cast<FrameElement> (e))
		{
			if(singleRootElement)
				singleRootElement->createItem (rootFrame); // use rootFrame directly as group
			else
			{
				if(DockPanelItem* item = frameElement->createItem (nullptr))
					rootFrame->addItem (item);
			}
		}
		else if(ParameterElement* paramElement = ccl_cast<ParameterElement> (e))
			paramElements.add (paramElement);
	EndFor

	Perspective* perspective = NEW Perspective (getName (), rootFrame);
	perspective->setStyle (getStyle ());
	perspective->setOrientation (getOrientation ());
	perspective->setTransitionType (transitionType);
	perspective->setBackCommandCategory (getBackCommandCategory ());
	perspective->setBackCommandName (getBackCommandName ());

	if(!styleClass.isEmpty ())
	{
		perspective->setVisualStyle (model.getStyle (styleClass, this));
		perspective->setBackgroundOptions (backgroundOptions);
	}

	if(!title.isEmpty ())
	{
		AutoPtr<PerspectiveActivator> activator = NEW PerspectiveActivator (perspective, title);
		activator->setIcon (model.getImage (iconName));

		perspective->setActivator (activator);
	}

	ListForEachObject (paramElements, ParameterElement, paramElement)
		if(IParameter* param = paramElement->createParameter ())
			perspective->addCustomParam (param);
	EndFor

	return perspective;
}

//************************************************************************************************
// FrameElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (FrameElement, Element, TAG_FRAME, DOC_GROUP_WORKSPACE, FrameItem)
	ADD_SKIN_ELEMENT_MEMBER (TAG_DEFAULT, TYPE_STRING)		///< name of a window class that should initially appear in the frame
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONDITION, TYPE_STRING)	///< an absolute path of a property that is evaluated to check if this frame can be used. The condition can be inverted by prepending it with "not "
	ADD_SKIN_ELEMENT_MEMBER (ATTR_DECOR, TYPE_STRING)		///< name of a decorating form that is wrapped around the form of the window class. The decor form should include the actual content as view "Content" from controller "$frame". The decor form name is prepended with "Workspace."
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FRIEND, TYPE_STRING)		///< name of a frame thats gets activated instead of this frame on mouse click
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WIDTH, TYPE_METRIC)		///< initial width
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HEIGHT, TYPE_METRIC)		///< initial height
	ADD_SKIN_ELEMENT_MEMBER (ATTR_GROUPS, TYPE_STRING)		///< space separated list of window class group names that may appear in this frame
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (FrameElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (FrameElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_FRAMECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_PERSPECTIVECHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_FRAMECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (FrameElement)
DEFINE_SKIN_ENUMERATION (TAG_FRAME, ATTR_OPTIONS, FrameItem::customStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameElement::FrameElement ()
: fillFactor (0),
  style (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameElement::setAttributes (const SkinAttributes& a)
{
	style = a.getOptions (ATTR_OPTIONS, FrameItem::customStyles);
	windowID = a.getString (TAG_DEFAULT);
	condition = a.getString (ATTR_CONDITION);
	decor = a.getString (ATTR_DECOR);
	friendID = a.getString (ATTR_FRIEND);
	size (a.getInt (ATTR_WIDTH), a.getInt (ATTR_HEIGHT));
	fillFactor = a.getFloat (ATTR_FILL);

	ForEachStringToken (a.getString (ATTR_GROUPS), " ", group)
		groups.add (group);
	EndFor

	SuperClass::setAttributes (a);

	// non-group frames must have a name!
	bool hasOrientationStyle = get_flag (style, FrameItem::kVertical|FrameItem::kHorizontal);
	if(name.isEmpty () && !hasOrientationStyle)
	{
		MutableCString groups (a.getString (ATTR_GROUPS));
		SKIN_WARNING (this, "<Frame ... groups=\"%s\" default=\"%s\"> must have a name!", groups.str (), windowID.str ())
	}

	ASSERT (!name.isEmpty () || hasOrientationStyle)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_OPTIONS, style, FrameItem::customStyles);
	
	String groupsString;
	for(int i = 0; i < groups.count (); i++)
	{
		if(i) groupsString << " ";
		groupsString << groups[i];
	}
	a.setString (ATTR_GROUPS, groupsString);
	a.setString (TAG_DEFAULT, windowID);
	a.setString (ATTR_CONDITION, condition);
	a.setString (ATTR_DECOR, decor);
	a.setString (ATTR_FRIEND, friendID);
	if(!size.isNull ())
	{
		a.setInt (ATTR_WIDTH, size.x);
		a.setInt (ATTR_HEIGHT, size.y);
	}
	a.setFloat (ATTR_FILL, fillFactor);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem* FrameElement::createItem (FrameItem* frameItem)
{
	if(frameItem)
		frameItem->setStyle (style);
	else
		frameItem = FrameItem::createItem (style);

	if(!ccl_cast<FrameGroupItem> (frameItem))
	{
		frameItem->setDefaultWindowID (windowID);
		frameItem->setWindowID (windowID);
		frameItem->setCondition (condition);
		frameItem->setFriendID (friendID);
		frameItem->saveSize (size);
	}

	frameItem->setName (String (getName ()));
	frameItem->setDecor (decor);
	frameItem->setFillFactor (fillFactor);

	for(int i = 0; i < groups.count (); i++)
		frameItem->addGroupID (String (groups[i]));

	createChildItems (*this, *frameItem);

	return frameItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameElement::createChildItems (Element& parentElement, FrameItem& parentItem)
{
	ForEach (parentElement, Element, e)
		FrameElement* frameElement = ccl_cast<FrameElement> (e);
		if(frameElement)
		{
			DockPanelItem* childItem = frameElement->createItem (nullptr);
			if(childItem)
				parentItem.addItem (childItem);
		}
		else if(EmbeddedFrameElement* embeddedElement = ccl_cast<EmbeddedFrameElement> (e))
		{
			EmbeddedFrameItem* embeddedFrame = NEW EmbeddedFrameItem ();
			embeddedFrame->setName (String (embeddedElement->getName ()));
			embeddedFrame->setParentClassID (embeddedElement->getParentClassID ());

			createChildItems (*embeddedElement, *embeddedFrame);
			parentItem.addItem (embeddedFrame);
		}
		else if(DividerElement* dividerElement = ccl_cast<DividerElement> (e))
		{
			DividerItem* dividerItem = NEW DividerItem;
			dividerItem->setName (String (dividerElement->getName ()));
			dividerItem->setStyle (dividerElement->getOptions ());
			dividerItem->setOutreach (dividerElement->getOutreach ());

			RectRef r (dividerElement->getSize ());
			dividerItem->setWidth (dividerElement->getOptions ().isHorizontal () ? r.getWidth () : r.getHeight ());
			parentItem.addItem (dividerItem);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameElement::isGroup ()
{
	return !get_flag<int> (style, FrameItem::kMultiple) && get_flag (style, FrameItem::kVertical|FrameItem::kHorizontal);
}

//************************************************************************************************
// EmbeddedFrameElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (EmbeddedFrameElement, ViewElement, TAG_EMBEDDED_FRAME, DOC_GROUP_WORKSPACE, EmbeddedFrameView)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PARENT_CLASS, TYPE_STRING)	///< parent class, whose view host the corresponding EmbeddedFrame view. Only used when describing an the EmbeddedFrame.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_WORKSPACE, TYPE_STRING)		///< workspace id. Only used when describing an EmbeddedFrame view.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TRANSITION, TYPE_ENUM)		///< transition used when content is replaced
END_SKIN_ELEMENT_WITH_MEMBERS (EmbeddedFrameElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (EmbeddedFrameElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_VIEWSSTATEMENTS)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_FRAMECHILDREN)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_FRAME)
END_SKIN_ELEMENT_ATTRIBUTES (EmbeddedFrameElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

EmbeddedFrameElement::EmbeddedFrameElement ()
: transitionType (Styles::kTransitionNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EmbeddedFrameElement::setAttributes (const SkinAttributes& a)
{
	parentClassID = a.getString (ATTR_PARENT_CLASS);
	workspaceID = a.getString (ATTR_WORKSPACE);
	transitionType = a.getOptions (ATTR_TRANSITION, ViewAnimator::transitionTypes, true, Styles::kTransitionNone);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EmbeddedFrameElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_PARENT_CLASS, parentClassID);
	a.setString (ATTR_WORKSPACE, workspaceID);
	a.setOptions (ATTR_TRANSITION, transitionType, ViewAnimator::transitionTypes, true);
	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* EmbeddedFrameElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		if(workspaceID.isEmpty ())
		{
			if(IApplication* app = GUI.getApplication ())
				workspaceID = app->getApplicationID ();

			ASSERT (!workspaceID.isEmpty ())
			if(workspaceID.isEmpty ())
				workspaceID = CSTR ("cclgui");
		}

		EmbeddedFrameView* frameView = NEW EmbeddedFrameView (size);
		frameView->setWorkspaceID (workspaceID);
		frameView->setName (String (getName ()));
		frameView->setTransitionType (transitionType);
		view = frameView;
	}
	return SuperClass::createView (args, view);
}

//************************************************************************************************
// ParameterElement
//************************************************************************************************

BEGIN_STYLEDEF (ParameterElement::types)
	{"int",		IParameter::kInteger},
	{"float",	IParameter::kFloat},
	{"string",	IParameter::kString},
	{"list",	IParameter::kList},
	{"scroll",	IParameter::kScroll},
END_STYLEDEF

BEGIN_STYLEDEF (ParameterElement::options)
	{"storable", IParameter::kStorable},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ParameterElement, Element, TAG_PARAMETER, DOC_GROUP_WORKSPACE, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TYPE, TYPE_ENUM)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING)  ///< initial parameter value
	ADD_SKIN_ELEMENT_MEMBER (ATTR_RANGE, TYPE_STRING)  ///< "min,max" for int and float; "range, pagesize" for scroll parameter; comma-separated list of values for a list
END_SKIN_ELEMENT_WITH_MEMBERS (ParameterElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ParameterElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_FRAMECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_PERSPECTIVECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (ParameterElement)

DEFINE_SKIN_ENUMERATION (TAG_PARAMETER, ATTR_TYPE, ParameterElement::types)
DEFINE_SKIN_ENUMERATION (TAG_PARAMETER, ATTR_OPTIONS, ParameterElement::options)

//////////////////////////////////////////////////////////////////////////////////////////////////

ParameterElement::ParameterElement ()
: type (IParameter::kInteger),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* ParameterElement::createParameter ()
{
	IParameter* param = nullptr;

	switch(type)
	{
	case IParameter::kScroll :
		{
			Variant rangeArgs[2] = {0, 0};
			range.scanFormat (CCLSTR ("%(1),%(2)"), rangeArgs, 2);
			param = NEW ScrollParam (0, name);
			UnknownPtr<IScrollParameter> (param)->setRange (rangeArgs[0], rangeArgs[1]);
			param->fromString (value);
		}
		break;

	case IParameter::kInteger:
		{
			Variant rangeArgs[2] = {0, 1};
			range.scanFormat (CCLSTR ("%(1),%(2)"), rangeArgs, 2);
			param = NEW IntParam (rangeArgs[0], rangeArgs[1], name);
			param->fromString (value);
		}
		break;

	case IParameter::kFloat:
		{
			Variant rangeArgs[2] = {0., 1.};
			range.scanFormat (CCLSTR ("%(1),%(2)"), rangeArgs, 2);
			param = NEW FloatParam (rangeArgs[0], rangeArgs[1], name);
			param->fromString (value);
		}
		break;

	case IParameter::kString:
		param = NEW StringParam (name);
		param->fromString (translate (value.trimWhitespace ()));
		break;

	case IParameter::kList:
		ListParam* list = NEW ListParam (name);
		ForEachStringToken (range, String (","), token)
			list->appendString (translate (token.trimWhitespace ()));
		EndFor
		param = list;
		{
			int64 index = 0;
			value.getIntValue (index);
			param->setValue (index);
		}
		break;
	}

	if(param)
		param->setStorable ((flags & IParameter::kStorable) != 0);

	return param;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParameterElement::setAttributes (const SkinAttributes& a)
{
	type = a.getOptions (ATTR_TYPE, types, true, IParameter::kInteger);
	flags = a.getOptions (ATTR_OPTIONS, options);
	value = a.getString (ATTR_VALUE);
	range = a.getString (ATTR_RANGE);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ParameterElement::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_TYPE, type, types, true);
	a.setOptions (ATTR_OPTIONS, flags, options);
	a.setString (ATTR_VALUE, value);
	a.setString (ATTR_RANGE, range);
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// SpaceElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (SpaceElement, ViewElement, TAG_SPACE, DOC_GROUP_GENERAL, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpaceElement::setAttributes (const SkinAttributes& a)
{
	a.getOptions (options, ATTR_OPTIONS);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SpaceElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
	{
		Rect r (size);
		if(r.isEmpty ())
		{
			int margin = getTheme ()->getThemeMetric (ThemeElements::kLayoutSpacing);
			r (0, 0, margin, margin);
		}

		view = NEW SpaceView (r, options);
	}
	return ViewElement::createView (args, view);
}

//************************************************************************************************
// NullSpaceElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (NullSpaceElement, SpaceElement, TAG_NULLSPACE, DOC_GROUP_GENERAL, 0)

//////////////////////////////////////////////////////////////////////////////////////////////////

View* NullSpaceElement::createView (const CreateArgs& args, View* view)
{
	if(!view)
		view = NEW NullView;
	return ViewElement::createView (args, view);
}

//************************************************************************************************
// StyleElement
//************************************************************************************************

BEGIN_STYLEDEF (StyleElement::textOptions)
	{"wordbreak",	TextFormat::kWordBreak},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (StyleElement, Element, TAG_STYLE, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_APPSTYLE, TYPE_BOOL)		///< make style accessible from all skin scopes
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OVERRIDE, TYPE_BOOL)		///< silence warning for styles that are replaced on purpose
	ADD_SKIN_ELEMENT_MEMBER (ATTR_INHERIT, TYPE_STRING)		///< inherit elements from this style
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FORECOLOR, TYPE_COLOR)	///< color of foreground
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BACKCOLOR, TYPE_COLOR)	///< color of background
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HILITECOLOR, TYPE_COLOR)	///< color used for a hilite or seleciton state
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTCOLOR, TYPE_COLOR)	///< color used for drawing text
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STROKEWIDTH, TYPE_METRIC)	///< width (in pixels) of drawn lines
	ADD_SKIN_ELEMENT_MEMBER (ATTR_BORDER, TYPE_METRIC)		///< border (in pixels)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTALIGN, TYPE_ENUM)		///< text alignment
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTOPTIONS, TYPE_ENUM)	///< text options
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTFACE, TYPE_STRING)	///< name of font face
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTTHEMEID, TYPE_STRING)	///< identifier of theme font
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTSIZE, TYPE_METRIC)	///< size of text
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTSTYLE, TYPE_ENUM)		///< style of font
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TEXTSMOOTHING, TYPE_ENUM)	///< smoothing of font
END_SKIN_ELEMENT_WITH_MEMBERS (StyleElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (StyleElement)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (StyleElement)
DEFINE_SKIN_ENUMERATION (TAG_STYLE, ATTR_TEXTOPTIONS, StyleElement::textOptions)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_STYLE, ATTR_TEXTALIGN, nullptr, TAG_ALIGN, ATTR_ALIGN)
DEFINE_SKIN_ENUMERATION (TAG_STYLE, ATTR_TEXTSTYLE, FontElement::fontStyles)
DEFINE_SKIN_ENUMERATION_PARENT (TAG_STYLE, ATTR_TEXTSMOOTHING, nullptr, TAG_FONT, ATTR_SMOOTHING)

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleElement::StyleElement ()
: appStyle (false),
  overrideStyle (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleElement::~StyleElement ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* StyleElement::newStyle ()
{
	return NEW VisualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle& StyleElement::getStyle ()
{
	if(!style)
	{
		style = newStyle ();
		style->release ();
	}
	return *style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StyleElement::setAttributes (const SkinAttributes& a)
{
	Element::setAttributes (a);

	VisualStyle& style = getStyle ();

	setAppStyle (a.getBool (ATTR_APPSTYLE, false));
	setOverride (a.getBool (ATTR_OVERRIDE, false));
	inherit = a.getString (ATTR_INHERIT);
	
	// inline Colors
	String foreColor = a.getString (ATTR_FORECOLOR);
	if(foreColor.isEmpty () == false)
		colors.append (Pair (ATTR_FORECOLOR, foreColor));
	
	String backColor = a.getString (ATTR_BACKCOLOR);
	if(backColor.isEmpty () == false)
		colors.append (Pair (ATTR_BACKCOLOR, backColor));
	
	String hiliteColor = a.getString (ATTR_HILITECOLOR);
	if(hiliteColor.isEmpty () == false)
		colors.append (Pair (ATTR_HILITECOLOR, hiliteColor));

	String textColor = a.getString (ATTR_TEXTCOLOR);
	if(textColor.isEmpty () == false)
		colors.append (Pair (ATTR_TEXTCOLOR, textColor));

	// inline Metrics
	if(a.exists (ATTR_STROKEWIDTH))
		style.setMetric (ATTR_STROKEWIDTH, a.getFloat (ATTR_STROKEWIDTH, 1.f));

	if(a.exists (ATTR_BORDER))
		style.setMetric (ATTR_BORDER, a.getFloat (ATTR_BORDER, 0.f));

	// inline Options
	if(a.exists (ATTR_TEXTALIGN))
	{
		int textAlign = a.getOptions (ATTR_TEXTALIGN, AlignElement::alignStyles, false, Alignment::kCenter);
		style.setOptions (ATTR_TEXTALIGN, textAlign);
	}

	if(a.exists (ATTR_TEXTOPTIONS))
	{
		int textOptions = a.getOptions (ATTR_TEXTOPTIONS, StyleElement::textOptions);
		style.setOptions (ATTR_TEXTOPTIONS, textOptions);
	}

	// inline Font
	if(a.exists (ATTR_TEXTFACE) || a.exists (ATTR_TEXTTHEMEID) || a.exists (ATTR_TEXTSIZE) || a.exists (ATTR_TEXTSTYLE))
	{
		Font font (Font::getDefaultFont ());

		MutableCString themeId = a.getString (ATTR_TEXTTHEMEID);
		if(!themeId.isEmpty ())
			FontElement::applyThemeFont (this, font, themeId);
		else
		{
			String textFace = a.getString (ATTR_TEXTFACE);
			if(!textFace.isEmpty ())
				font.setFace (textFace);
		}

		FontElement::applyFontSize (font, a.getString (ATTR_TEXTSIZE));
		font.setStyle (a.getOptions (ATTR_TEXTSTYLE, FontElement::fontStyles));
		font.setMode (a.getOptions (ATTR_TEXTSMOOTHING, FontElement::smoothingModes, true));

		style.setFont (StyleID::kTextFont, font);
	}	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StyleElement::getAttributes (SkinAttributes& a) const
{
	VisualStyle& style = const_cast<StyleElement*> (this)->getStyle ();

	a.setBool (ATTR_APPSTYLE, isAppStyle ());
	a.setBool (ATTR_OVERRIDE, isOverride ());
	if(!inherit.isEmpty ())
		a.setString (ATTR_INHERIT, inherit);

	a.setColor (ATTR_FORECOLOR, style.getForeColor ());
	a.setColor (ATTR_BACKCOLOR, style.getBackColor ());
	a.setColor (ATTR_HILITECOLOR, style.getHiliteColor ());
	a.setColor (ATTR_TEXTCOLOR, style.getTextColor ());
	a.setFloat (ATTR_STROKEWIDTH, style.getStrokeWidth ());
	a.setOptions (ATTR_TEXTALIGN, style.getTextAlignment ().align, AlignElement::alignStyles);
	a.setOptions (ATTR_TEXTOPTIONS, style.getTextOptions (), StyleElement::textOptions);
	
	if(style.getMetric (ATTR_BORDER) != 0)
		a.setFloat (ATTR_BORDER, style.getMetric (ATTR_BORDER));
	
	Font font = style.getTextFont ();
	a.setString (ATTR_TEXTFACE, font.getFace ());
	a.setFloat (ATTR_TEXTSIZE, font.getSize ());
	a.setOptions (ATTR_TEXTSTYLE, font.getStyle (), FontElement::fontStyles);
	a.setOptions (ATTR_TEXTSMOOTHING, font.getMode (), FontElement::smoothingModes, true);

	return Element::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleElement::loadFinished ()
{
	VisualStyle& style = getStyle ();
	ASSERT (!getName ().isEmpty ())
	style.setName (getName ());

	ArrayForEach (*this, Element, element)		
		MutableCString elementName (element->getName ());
		
		if(element->canCast (ccl_typeid<ColorElement> ()))
			colors.append (Pair (elementName, ((ColorElement*)element)->getColor ()));

		else if(element->canCast (ccl_typeid<GradientElement> ()))
			style.setGradient (elementName, ((GradientElement*)element)->getGradient ());
		
		else if(element->canCast (ccl_typeid<MetricElement> ()))
			style.setMetric (elementName, ((MetricElement*)element)->getValue ());
 
 		else if(element->canCast (ccl_typeid<StringElement> ()))
			style.setString (elementName, ((StringElement*)element)->getValue ());
		
		else if(element->canCast (ccl_typeid<FontElement> ()))
			style.setFont (elementName, ((FontElement*)element)->getFont ());
		
		else if(element->canCast (ccl_typeid<OptionsElement> ()))
			style.setOptions (elementName, ((OptionsElement*)element)->getOptions ());
		
		else if(element->canCast (ccl_typeid<ImageElement> ()))
			images.append (Pair (elementName, ((ImageElement*)element)->getAlias ()));

		else if(element->canCast (ccl_typeid<TriggerListElement> ()))
			style.setTrigger ((TriggerListElement*)element);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StyleElement::isOverrideEnabled () const
{
	return isOverride ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleElement::loadResources (SkinModel& model)
{
	VisualStyle& style = getStyle ();
	while(!images.isEmpty ())
	{
		Pair pair = images.removeFirst ();
		if(pair.ref.isEmpty ())
			continue;

		if(Image* image = model.getImage (pair.ref, this))
			style.setImage (pair.name, image);
	}

	while(!colors.isEmpty ())
	{
		Pair pair = colors.removeFirst ();
		if(pair.ref.isEmpty ())
			continue;

		ColorValueReference reference;
		model.getColorReference (reference, pair.ref, this);
		if(reference.scheme != nullptr)
			style.addColorSchemeReference (pair.name, *reference.scheme, reference.nameInScheme);
		else
			style.setColor (pair.name, reference.colorValue);
	}

	// register in theme...
	if(isAppStyle ())
	{
		bool replaced = false;
		getTheme ()->setStyle (getName (), &style, &replaced);
		if(replaced && !isOverride ())
			SKIN_WARNING (this, "Replaced public style '%s' (multiple definition?)", getName ().str ())
	}

	if(!inherit.isEmpty ())
		style.setInherited (model.getStyle (inherit, this));
}

//************************************************************************************************
// ThemeStyleElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (ThemeStyleElement, StyleElement, TAG_THEMESTYLE, DOC_GROUP_STYLES, 0)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ThemeStyleElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_TOPLEVEL)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (SCHEMA_GROUP_THEMEELEMENTCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (ThemeStyleElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ThemeStyleElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	setAppStyle (true);
	setName (CSTR (".ThemeElements"));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeStyleElement::loadFinished ()
{
	SuperClass::loadFinished ();

	// register style in theme before it's used by other styles
	SkinModel* model = (SkinModel*)getParent (ccl_typeid<SkinModel> ());
	ASSERT (model)
	if(model)
		SuperClass::loadResources (*model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeStyleElement::loadResources (SkinModel& model)
{}

//************************************************************************************************
// StyleAliasElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (StyleAliasElement, StyleElement, TAG_STYLEALIAS, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLES, TYPE_STRING)		///< The available style names, separted by spaces.
	ADD_SKIN_ELEMENT_MEMBER (ATTR_PARAMETER, TYPE_STRING)	///< Url of a parameter that selects on of the styles specified in "styles".
END_SKIN_ELEMENT_WITH_MEMBERS (StyleAliasElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleAliasElement::StyleAliasElement ()
{
	style = NEW VisualStyleAlias (getName ());
	style->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StyleAliasElement::setAttributes (const SkinAttributes& a)
{
	paramName = a.getString (ATTR_PARAMETER);

	ForEachStringToken (a.getString (ATTR_STYLES), " ", style)
		styleNames.add (style);
	EndFor

	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StyleAliasElement::getAttributes (SkinAttributes& a) const
{
	if(!paramName.isEmpty ())
		a.setString (ATTR_PARAMETER, paramName);

	String stylesString;
	for(int i = 0; i < styleNames.count (); i++)
	{
		if(i)
			stylesString << " ";
		stylesString << styleNames[i];
	}
	a.setString (ATTR_STYLES, stylesString);

	return SuperClass::getAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleAliasElement::loadResources (SkinModel& model)
{
	if(!paramName.isEmpty ())
	{
		// find parameter (must be a global object url)
		int pos = paramName.lastIndex ('/');
		if(pos >= 0)
		{
			MutableCString controllerPath (paramName.subString (0, pos));
			MutableCString pName (paramName.subString (pos + 1));
			if(paramName.contains ("://"))
			{
				Url objectUrl;
				objectUrl.setUrl (String (controllerPath));
				UnknownPtr<IController> controller (System::GetObjectTable ().getObjectByUrl (objectUrl));
				IParameter* parameter = controller ? controller->findParameter (pName) : nullptr;
				if(parameter)
				{
					AutoPtr<VisualStyleSelector> styleSelector (NEW VisualStyleSelector (static_cast<VisualStyleAlias*> (&getStyle ())));
					styleSelector->setParameter (parameter);

					// lookup styles to be selected based on parameter value
					for(CStringRef styleName : styleNames)
					{
						VisualStyle* style = model.getStyle (styleName, this);
						ASSERT (style) // (warning emitted in SkinModel::getStyle)
						if(style)
							styleSelector->addStyle (style);
						else
							styleSelector->addStyle (AutoPtr<VisualStyle> (NEW VisualStyle)); // dummy to keep indices as expected
					}

					styleSelector->initialize ();
				}
			}
		}
	}
	SuperClass::loadResources (model); // registers style in theme
}

//************************************************************************************************
// ColorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ColorElement, Element, TAG_COLOR, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR)
END_SKIN_ELEMENT_WITH_MEMBERS (ColorElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ColorElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_THEMEELEMENTCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (ColorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorElement::setAttributes (const SkinAttributes& a)
{
	color = a.getString (ATTR_COLOR);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorElement::getAttributes (SkinAttributes& a) const
{
	Element::getAttributes (a);
	a.setString (ATTR_COLOR, color);
	return true;
}

//************************************************************************************************
// MetricElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (MetricElement, Element, TAG_METRIC, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_FLOAT)
END_SKIN_ELEMENT_WITH_MEMBERS (MetricElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (MetricElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_THEMEELEMENTCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (MetricElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetricElement::MetricElement ()
: value (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetricElement::setAttributes (const SkinAttributes& a)
{
	value = a.getFloat (ATTR_VALUE);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetricElement::getAttributes (SkinAttributes& a) const
{
	Element::getAttributes (a);
	a.setFloat (ATTR_VALUE, value);
	return true;
}

//************************************************************************************************
// StringElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (StringElement, Element, TAG_STRING, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (StringElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (StringElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (StringElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringElement::setAttributes (const SkinAttributes& a)
{
	value = a.getString (ATTR_VALUE);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringElement::getAttributes (SkinAttributes& a) const
{
	Element::getAttributes (a);
	a.setString (ATTR_VALUE, value);
	return true;
}

//************************************************************************************************
// OptionsElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (OptionsElement, Element, TAG_OPTIONS, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_TYPE, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_OPTIONS, TYPE_ENUM)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_VALUE, TYPE_INT)
END_SKIN_ELEMENT_WITH_MEMBERS (OptionsElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (OptionsElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (OptionsElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

OptionsElement::OptionsElement ()
: options (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionsElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);

	typeName = a.getCString (ATTR_TYPE);
	if(!typeName.isEmpty ())
	{
		if(auto styleDef = Enumeration::getStyleDef (typeName))
			options = a.getOptions (ATTR_OPTIONS, styleDef, false, 0);
		else
			SKIN_WARNING (this, "Options type '%s' not found", typeName.str ())		
	}
	else
	{
		options = a.getInt (ATTR_VALUE);
	}	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OptionsElement::getAttributes (SkinAttributes& a) const
{
	if(!typeName.isEmpty () || a.isVerbose ())
		a.setString (ATTR_TYPE, typeName);
	
	if(!typeName.isEmpty ())
	{
		if(auto styleDef = Enumeration::getStyleDef (typeName))
			a.setOptions (ATTR_OPTIONS, options, styleDef, false);
		else if(a.isVerbose ())
			a.setString (ATTR_OPTIONS, String::kEmpty);
	}
	else
	{
		a.setInt (ATTR_VALUE, options);
		if(a.isVerbose ())
			a.setString (ATTR_OPTIONS, String::kEmpty);
	}
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// AlignElement
//************************************************************************************************

BEGIN_STYLEDEF (AlignElement::alignStyles)
	{"hcenter",	Alignment::kHCenter},
	{"left",	Alignment::kLeft},
	{"right",	Alignment::kRight},
	{"vcenter",	Alignment::kVCenter},
	{"top",		Alignment::kTop},
	{"bottom",	Alignment::kBottom},
	{"center",	Alignment::kCenter},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (AlignElement, OptionsElement, TAG_ALIGN, DOC_GROUP_STYLES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ALIGN, TYPE_ENUM)
END_SKIN_ELEMENT_WITH_MEMBERS (AlignElement)
DEFINE_SKIN_ENUMERATION (TAG_ALIGN, ATTR_ALIGN, AlignElement::alignStyles)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignElement::setAttributes (const SkinAttributes& a)
{
	options = a.getOptions (ATTR_ALIGN, alignStyles, false, Alignment::kCenter);
	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlignElement::getAttributes (SkinAttributes& a) const
{
	Element::getAttributes (a);
	a.setOptions (ATTR_ALIGN, options, alignStyles);
	return true;
}

//************************************************************************************************
// FontElement
//************************************************************************************************

BEGIN_STYLEDEF (FontElement::fontStyles)
	{"bold",		Font::kBold},
	{"italic",		Font::kItalic},
	{"underline",	Font::kUnderline},
	{"normal",		Font::kNormal},
END_STYLEDEF

BEGIN_STYLEDEF (FontElement::smoothingModes)
	{"default",		Font::kDefault},
	{"none",		Font::kNone},
	{"antialias",	Font::kAntiAlias},
END_STYLEDEF

BEGIN_STYLEDEF (FontElement::textTrimModes)
	{"default",			Font::kTrimModeDefault},
	{"keepend",			Font::kTrimModeKeepEnd},
	{"left",			Font::kTrimModeLeft},
	{"middle",			Font::kTrimModeMiddle},
	{"right",			Font::kTrimModeRight},
	{"numeric",			Font::kTrimModeNumeric},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (FontElement, Element, TAG_FONT, DOC_GROUP_STYLES, Font)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_THEMEID, TYPE_STRING)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SIZE, TYPE_METRIC)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_STYLE, TYPE_ENUM)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_FACE, TYPE_STRING)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SMOOTHING, TYPE_ENUM)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SPACING, TYPE_METRIC)  
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LINESPACING, TYPE_METRIC)
END_SKIN_ELEMENT_WITH_MEMBERS (FontElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (FontElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_STYLECHILDREN)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_THEMEELEMENTCHILDREN)
END_SKIN_ELEMENT_ATTRIBUTES (FontElement)
DEFINE_SKIN_ENUMERATION (TAG_FONT, ATTR_STYLE, FontElement::fontStyles)
DEFINE_SKIN_ENUMERATION (TAG_FONT, ATTR_SMOOTHING, FontElement::smoothingModes)

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontElement::applyFontSize (Font& font, StringRef _sizeString)
{
	String sizeString (_sizeString);
	sizeString.trimWhitespace ();
	if(sizeString.isEmpty ())
		return;

	static const String kIncrementSize ("+");
	static const String kDecrementSize ("-");

	double sizeValue = 0.;
	sizeString.getFloatValue (sizeValue);
	if(sizeString.startsWith (kIncrementSize) || sizeString.startsWith (kDecrementSize))
		font.setSize (font.getSize () + (float)sizeValue);
	else
		font.setSize ((float)sizeValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontElement::applyThemeFont (Element* caller, Font& font, StringID themeId)
{
	static const Font kInvalidFont ("~invalid-font");

	font = Theme::getGlobalStyle ().getFont (themeId, kInvalidFont);
	if(font.isEqual (kInvalidFont))
	{
		font = Font::getDefaultFont ();
		SKIN_WARNING (caller, "Theme font not found: '%s'", themeId.str ())
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontElement::setAttributes (const SkinAttributes& a)
{
	themeId = MutableCString (a.getString (ATTR_THEMEID));
	if(!themeId.isEmpty ())
	{
		applyThemeFont (this, font, themeId);
		applyFontSize (font, a.getString (ATTR_SIZE));
		font.setStyle (a.getOptions (ATTR_STYLE, fontStyles, false, font.getStyle ()));
		font.setMode (a.getOptions (ATTR_SMOOTHING, smoothingModes, true, font.getMode ()));
		font.setSpacing (a.getFloat (ATTR_SPACING, font.getSpacing ()));
		font.setLineSpacing (a.getFloat (ATTR_LINESPACING, font.getLineSpacing ()));
	}
	else
	{
		String face (a.getString (ATTR_FACE));
		if(!face.isEmpty ())
			font.setFace (face);
		applyFontSize (font, a.getString (ATTR_SIZE));
		font.setStyle (a.getOptions (ATTR_STYLE, fontStyles));
		font.setMode (a.getOptions (ATTR_SMOOTHING, smoothingModes, true));
		font.setSpacing (a.getFloat (ATTR_SPACING, font.getSpacing ()));
		font.setLineSpacing (a.getFloat (ATTR_LINESPACING, font.getLineSpacing ()));
	}

	return Element::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontElement::getAttributes (SkinAttributes& a) const
{
	a.setString (ATTR_FACE, font.getFace ());
	a.setString (ATTR_THEMEID, themeId);
	a.setFloat (ATTR_SIZE, font.getSize ());
	a.setOptions (ATTR_STYLE, font.getStyle (), fontStyles);
	a.setOptions (ATTR_SMOOTHING, font.getMode (), smoothingModes, true);
	a.setFloat (ATTR_SPACING, font.getSpacing ());
	a.setFloat (ATTR_LINESPACING, font.getLineSpacing ());
	return Element::getAttributes (a);
}

//************************************************************************************************
// ShapeColorMappingElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ShapeColorMappingElement, Element, TAG_SHAPECOLORMAPPING, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_COLOR, TYPE_COLOR)	///< replacement string
END_SKIN_ELEMENT_WITH_MEMBERS (ShapeColorMappingElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

ShapeColorMappingElement::ShapeColorMappingElement ()
: scheme (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeColorMappingElement::setAttributes (const SkinAttributes& a)
{
	SuperClass::setAttributes (a);
	
	Color color;
	Colors::fromCString (color, getName ());
	setColor (color);
	
	ColorValueReference reference;
	SkinModel::getColorFromAttributes (reference, a, ATTR_COLOR, this);
	setReferenceColor (reference.colorValue);
	
	if(reference.scheme)
	{
		setScheme (reference.scheme);
		setNameInScheme (reference.nameInScheme);
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeColorMappingElement::getAttributes (SkinAttributes& a) const
{
	a.setColor (ATTR_COLOR, color);
	return SuperClass::getAttributes (a);
}

//************************************************************************************************
// ColorSchemeElement
//************************************************************************************************

DEFINE_SKIN_ELEMENT (ColorSchemeElement, Element, TAG_COLORSCHEME, DOC_GROUP_RESOURCES, ColorScheme)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ColorSchemeElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE (SCHEMA_GROUP_RESOURCES)
	ADD_SKIN_CHILDGROUP_ATTRIBUTE (TAG_COLORSCHEMECOLOR)
END_SKIN_ELEMENT_ATTRIBUTES (ColorSchemeElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemeElement::mergeElements (Element& other)
{
	auto schemeElement = ccl_cast<ColorSchemeElement> (&other);
	ASSERT (schemeElement)
	if(schemeElement)
	{
		// take over the child color elements from other ColorSchemeElement (extend scheme)
		takeElements (*schemeElement);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColorSchemeElement::loadFinished ()
{
	ColorScheme& scheme = ColorSchemes::instance ().get (getName ());
	
	auto resolveColorSetAlpha = [&] (Color& color, CStringRef string, int alpha = -1)
	{
		if(!Colors::fromCString (color, string))
		{
			// look for color reference
			ColorValueReference reference;
			SkinModel::getColorFromString (reference, string, this);
			color = reference.colorValue;
		}
		if(alpha != -1)
		{
			float alphaF = alpha * 0.01f;
			color.setAlphaF (ccl_bound (alphaF));
		}
	};
	
	ArrayForEach (*this, Element, element)
		if(ColorSchemeColorElement* colorElement = ccl_cast<ColorSchemeColorElement> (element))
		{
			ColorScheme::Item& item = scheme.getItemMutable (colorElement->getName ());
			Color color;
			resolveColorSetAlpha (color, colorElement->getColor (), colorElement->getAlphaValue ());
			
			item.setBaseColor (color);
			item.setHueFixed (colorElement->isHueFixed ());
			item.setSaturationSegments (colorElement->getSaturationSegments ());
			item.setLuminanceSegments (colorElement->getLuminanceSegments ());
			item.setContrastSegments (colorElement->getContrastSegments ());
			item.setSLCombined (colorElement->isSLCombined ());
			item.setInvertible (colorElement->isInvertible ());
			item.setInvertedValue (colorElement->getInvertedValue ());
			
			CStringRef invertedColorString = colorElement->getInvertedColor ();
			if(!invertedColorString.isEmpty ())
			{
				item.setInvertible (false); // don't auto invert color - use inverted color instead
				resolveColorSetAlpha (color, invertedColorString, colorElement->getAlphaValue ());
			}

			item.setInvertedColor (color);
		}
	EndFor
	scheme.restore ();
}

//************************************************************************************************
// ColorSchemeColorElement
//************************************************************************************************

BEGIN_SKIN_ELEMENT_WITH_MEMBERS (ColorSchemeColorElement, ColorElement, TAG_COLORSCHEMECOLOR, DOC_GROUP_RESOURCES, 0)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_HUEFIXED, TYPE_BOOL)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SATURATION, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_LUMINANCE, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_INVERTIBLE, TYPE_BOOL)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_CONTRAST, TYPE_STRING)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_INVERTEDVALUE, TYPE_INT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_ALPHAVALUE, TYPE_INT)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_SLCOMBINED, TYPE_BOOL)
	ADD_SKIN_ELEMENT_MEMBER (ATTR_INVERTEDCOLOR, TYPE_STRING)
END_SKIN_ELEMENT_WITH_MEMBERS (ColorSchemeColorElement)
BEGIN_SKIN_ELEMENT_ATTRIBUTES (ColorSchemeColorElement)
	ADD_SKIN_SCHEMAGROUP_ATTRIBUTE ("") // remove inherited schema groups
END_SKIN_ELEMENT_ATTRIBUTES (ColorSchemeColorElement)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorSchemeColorElement::ColorSchemeColorElement ()
: hueFixed (false),
  invertible (true),
  invertedValue (0),
  alphaValue (-1),
  slCombined (false),
  contrastFixed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemeColorElement::setAttributes (const SkinAttributes& a)
{
	hueFixed = a.getBool (ATTR_HUEFIXED);
	saturationSegments = a.getString (ATTR_SATURATION);
	luminanceSegments = a.getString (ATTR_LUMINANCE);
	contrastSegments = a.getString (ATTR_CONTRAST);
	invertible = a.getBool (ATTR_INVERTIBLE, true);
	invertedValue = a.getInt (ATTR_INVERTEDVALUE, -1);
	alphaValue = a.getInt (ATTR_ALPHAVALUE, -1);
	slCombined = a.getBool (ATTR_SLCOMBINED);
	invertedColor = a.getString (ATTR_INVERTEDCOLOR);
	return SuperClass::setAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColorSchemeColorElement::getAttributes (SkinAttributes& a) const
{
	a.setBool (ATTR_HUEFIXED, hueFixed);
	a.setString (ATTR_SATURATION, saturationSegments);
	a.setString (ATTR_LUMINANCE, luminanceSegments);
	a.setString (ATTR_CONTRAST, contrastSegments);
	a.setBool (ATTR_INVERTIBLE, invertible);
	a.setInt (ATTR_INVERTEDVALUE, invertedValue);
	a.setInt (ATTR_ALPHAVALUE, alphaValue);
	a.setBool (ATTR_SLCOMBINED, slCombined);
	a.setString (ATTR_INVERTEDCOLOR, invertedColor);
	return SuperClass::getAttributes (a);
}
