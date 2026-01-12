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
// Filename    : ccl/gui/skin/skinmodel.h
// Description : Skin Data Model
//
//************************************************************************************************

#ifndef _ccl_skinmodel_h
#define _ccl_skinmodel_h

#include "ccl/gui/skin/skinelement.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/graphics/colorgradient.h"
#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/framework/designsize.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/collections/map.h"

namespace CCL {

class Url;
class View;
class Image;
class VisualStyle;
class SkinWizard;
class FileType;
class MouseCursor;
class FontResource;
class WindowClass;
class Workspace;
class Perspective;
class FrameItem;
class DockPanelItem;
class SkinOverlay;
class MutableSkinAttributes;
interface IParameter;
interface IBitmapFilter;

//************************************************************************************************
// SkinModel
/** Skin XML Document.
The root of a skin file. */
//************************************************************************************************

class SkinModel: public SkinElements::Element,
				 public ISkinModel
{
public:
	DECLARE_SKIN_ELEMENT (SkinModel, Element)

	SkinModel (ISkinContext* context = nullptr);

	static SkinModel* getModel (const Element* e);

	// Skin Sections
	Element& getIncludes ();
	Element& getImports ();
	Element& getOverlays ();
	Element& getResources ();
	Element& getStylesElement ();
	Element& getShapes ();
	Element& getForms ();
	Element& getWindowClasses ();
	Element& getWorkspacesElement ();

	// Submodels (Scopes)
	void merge (SkinModel& model);						///< merge elements into this model
	void takeSubModels (SkinModel& model);				///< merge sub-models into this model
	Element& getModels ();								///< included Skin Documents
	SkinModel& getRootModel ();
	SkinModel* getScopeModel (CStringRef scopeName);

	// Resources
	Object* getResource (CStringRef name);
	IGradient* getGradient (CStringRef name, Element* caller = nullptr);
	Image* getImage (CStringRef name, Element* caller = nullptr);
	VisualStyle* getStyle (CStringRef name, Element* caller);

	bool getColorReference (ColorValueReference& reference, StringID name, const Element* caller); ///< resolves symbolic colors defined in skin
	static bool getColorFromAttributes (ColorValueReference& reference, const SkinAttributes& a, StringID attrName, const Element* caller);
	static bool getColorFromString (ColorValueReference& reference, StringID string, const Element* caller);

	void loadResources (bool force = false);			///< called after model finished to load
	void reuseResources (SkinModel& sourceModel);

	void addImportedPath (UrlRef path);
	IContainer* CCL_API getContainerForType (ElementType which) override;  // ISkinModel
	void CCL_API getImportedPaths (IUnknownList& paths) const override; // ISkinModel

	#if DEBUG
	String dumpHelpIdentifiers ();
	#endif

	// Element overrides:
	void removeAll () override;
	ISkinContext* getSkinContext () const override;
	bool mergeElements (Element& other) override;

	CLASS_INTERFACE (ISkinModel, Element)

protected:
	ISkinContext* context;
	Element *includes, *imports, *overlays, *resources, *stylesElement, *shapes, *forms, *windowClasses, *workspacesElement;
	Element models;
	ObjectArray importedPaths;
	bool loadingResources;

	template<typename T>
	T* getResourceElement (CStringRef name, SkinModel*& model);

	Image* getImageInternal (CStringRef name, Element* caller = nullptr);

	// ISkinModel
	ISkinModel* CCL_API getSubModel (StringID name) override { return getScopeModel (name); }
};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SkinElements {

/** Enclosing tag for all Resources in a skin xml file. Resources are Cursor, FontResource, IconSet, Image, ImagesPart, Color
    \see Cursor \see FontResourceElement \see IconSetElement \see ImageElement \see ImagePartElement \see ColorElement */
DECLARE_SKIN_ELEMENT_CLASS (Resources, Element)

/** Enclosing tag for all Forms in a skin xml file. \see FormElement */
DECLARE_SKIN_ELEMENT_CLASS (Forms, Element)

/** Enclosing tag for all Includes in a skin xml file. \see IncludeElement */
DECLARE_SKIN_ELEMENT_CLASS (Includes, Element)

/** Enclosing tag for all Imports in a skin xml file. \see ImportElement */
DECLARE_SKIN_ELEMENT_CLASS (Imports, Element)

/** Enclosing tag for all Externals in a skin xml file. \see ExternalElement */
DECLARE_SKIN_ELEMENT_CLASS (Externals, Element)

/** Enclosing tag for all Overlays in a skin xml file. \see OverlayElement */
DECLARE_SKIN_ELEMENT_CLASS (Overlays, Element)

/** Enclosing tag for all Shapes in a skin xml file. \see ShapeElement */
DECLARE_SKIN_ELEMENT_CLASS (Shapes, Element)

/** Enclosing tag for all Styles in a skin xml file. \see StyleElement */
DECLARE_SKIN_ELEMENT_CLASS (StylesElement, Element)

/** Enclosing tag for all WindowClasses in a skin xml file. \see WindowClassElement */
DECLARE_SKIN_ELEMENT_CLASS (WindowClassesElement, Element)

/** Enclosing tag for all Workspaces in a skin xml file. \see WorkspaceElement */
DECLARE_SKIN_ELEMENT_CLASS (WorkspacesElement, Element)

//************************************************************************************************
// OverlayElement
/** Replaces an existing form description with another one. */
//************************************************************************************************

class OverlayElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (OverlayElement, Element)

	OverlayElement ();
	~OverlayElement ();

	PROPERTY_STRING (target, Target)
	PROPERTY_STRING (source, Source)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;

protected:
	SkinOverlay* overlay;
};

//************************************************************************************************
// ResourceElement
/** Base class for Resources.
\see Cursor
\see FontResourceElement
\see IconSetElement
\see ImageElement
\see ImagePartElement
\see ColorElement */
//************************************************************************************************

class ResourceElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ResourceElement, Element)

	PROPERTY_STRING (url, Url)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// IncludeElement
/** Includes the content of another Skin xml file.
The content of the included file is merged into the containing model. */
//************************************************************************************************

DECLARE_SKIN_ELEMENT_CLASS (IncludeElement, ResourceElement)

//************************************************************************************************
// ImportElement
/** Imports the contents of a complete skin package.
The content of the imported skin is merged into the containing model. */
//************************************************************************************************

DECLARE_SKIN_ELEMENT_CLASS (ImportElement, ResourceElement)

//************************************************************************************************
// ExternalElement
/** Declares a name that has to be defined outside the skin pack. */
//************************************************************************************************

DECLARE_SKIN_ELEMENT_CLASS (ExternalElement, Element)

//************************************************************************************************
// ResourceObjectElement
//************************************************************************************************

class ResourceObjectElement: public ResourceElement
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (ResourceObjectElement, ResourceElement)

	PROPERTY_SHARED_AUTO (Object, object, Object)

	virtual bool loadObject (SkinModel& model) = 0;
};

//************************************************************************************************
// ImageElement
/** Defines an image resource.
An image can be defined by specifying a file url, another image resource or a set of multiple frames.

\code{.xml}
<!-- From file -->
<Image name="MyImage1" url="folder/file.png">

<!-- From another image -->
<Image name="MyImage2" image="MyImage1">

<!-- Set of multiple frames -->
<Image name="MyMultiImage">
	<Image name="myFrame1" url="a.png"/>
	<Image name="myFrame1" url="b.png"/>
</Image>
\endcode
*/
//************************************************************************************************

class ImageElement: public ResourceElement,
					public ISkinImageElement
{
public:
	DECLARE_SKIN_ELEMENT (ImageElement, ResourceElement)

	ImageElement ();
	~ImageElement ();

	DECLARE_STYLEDEF (tileMethods)
	static IImage::TileMethod parseTileMethod (StringRef tile);
	static double parseDuration (StringRef string);

	CStringRef getAlias () const;

	Image* getImageInternal () const;
	void reuseImage (ImageElement& element);

	virtual bool loadImage (SkinModel& model); // (overwritten by ShapeImageElement, etc.)

	// ResourceElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

	CLASS_INTERFACE (ISkinImageElement, ResourceElement)

protected:
	Image* image;
	MutableCString alias;
	String frames;
	String tile;
	Rect margins;
	double duration;
	bool isTemplate;
	bool isAdaptive;

	void checkImageLoaded (Image* image, UrlRef imageUrl) const;
	virtual void applyImageModification ();

	// ISkinImageElement
	IImage* CCL_API getImage () const override;
	void CCL_API setImage (IImage* _image) override;
	StringRef CCL_API getImagePath () const override { return getUrl (); }
	void CCL_API setImagePath (StringRef imagePath) override { setUrl (imagePath); }
};

//************************************************************************************************
// ImagePartElement
/** An ImagePart defines a excerpt of an image as a new image resource.
\code{.xml}
<ImagePart name="MyPart" image="MyImage" size="0,3,7,1" />
\endcode
*/
//************************************************************************************************

class ImagePartElement: public ImageElement
{
public:
	DECLARE_SKIN_ELEMENT (ImagePartElement, ImageElement)

	// ImageElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Rect partRect;

	// ImageElement
	void applyImageModification () override;
};

//************************************************************************************************
// IconSetElement
/** Combines image files from a folder to an image resource with multiple frames.
The url specifies a folder. The image files in that folder must have the format "icon_SxS.png" with S being the size (same width and height) of the image.

Depending on the "frames" attribute, different sets of image file names are searched for in that folder:
If frames has the value "all", the sizes 16, 32, 48, 64, 128, 256, 512. Otherwise only 16 and 32 used.

\code{.xml}
<IconSet name="FileIcon:song" url="icons/song_doc.iconset"/>
\endcode
*/
//************************************************************************************************

class IconSetElement: public ImageElement
{
public:
	DECLARE_SKIN_ELEMENT (IconSetElement, ImageElement)

	// ImageElement
	bool loadImage (SkinModel& model) override;
};

//************************************************************************************************
// ImageFilterElement
/** Defines a filter applied when an image resource is loaded. */
//************************************************************************************************

class ImageFilterElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ImageFilterElement, Element)

	ImageFilterElement ();

	DECLARE_STYLEDEF (filterNames)

	IBitmapFilter* createFilter () const;

	PROPERTY_VARIABLE (Color, color, Color)
	PROPERTY_MUTABLE_CSTRING (schemeName, SchemeName)
	PROPERTY_MUTABLE_CSTRING (nameInScheme, NameInScheme)
	PROPERTY_VARIABLE (float, value, Value)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, 1<<0, hasColor)
	PROPERTY_FLAG (flags, 1<<1, hasValue)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// CursorElement
/** Defines a Cursor resource.
A cursor is defined with an image resource and an hotspot, which is the active point in the image. */
//************************************************************************************************

class CursorElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (CursorElement, Element)

	CursorElement ();
	~CursorElement ();

	PROPERTY_MUTABLE_CSTRING (sourceImage, SourceImage)
	PROPERTY_OBJECT (Point, hotspot, Hotspot)

	bool loadCursor (SkinModel& model);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	MouseCursor* cursor;
};

//************************************************************************************************
// GradientStopElement
/** Defines a gradient stop. */
//************************************************************************************************

class GradientStopElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (GradientStopElement, Element)

	GradientStopElement ();

	PROPERTY_VARIABLE (float, position, Position)
	PROPERTY_MUTABLE_CSTRING (colorString, ColorString)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// GradientElement
/** Defines a gradient. */
//************************************************************************************************

class GradientElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT_ABSTRACT (GradientElement, Element)

	virtual IGradient* getGradient () = 0;
	
	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	AutoPtr<ColorGradient> gradient;
	
	ColorGradientStopCollection* createStops () const;
};

//************************************************************************************************
// LinearGradientElement
/** Defines a linear gradient. */
//************************************************************************************************

class LinearGradientElement: public GradientElement
{
public:
	DECLARE_SKIN_ELEMENT (LinearGradientElement, GradientElement)

	PROPERTY_OBJECT (PointF, startPoint, StartPoint)
	PROPERTY_OBJECT (PointF, endPoint, EndPoint)

	// GradientElement
	IGradient* getGradient () override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// RadialGradientElement
/** Defines a radial gradient. */
//************************************************************************************************

class RadialGradientElement: public GradientElement
{
public:
	DECLARE_SKIN_ELEMENT (RadialGradientElement, GradientElement)

	RadialGradientElement ();

	PROPERTY_OBJECT (PointF, center, Center)
	PROPERTY_OBJECT (float, radius, Radius)

	// GradientElement
	IGradient* getGradient () override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// FontResourceElement
/** Defines a font resource.
\code{.xml}
<FontResource url="fonts/SEMPRG_.TTF"/>
\endcode
*/
//************************************************************************************************

class FontResourceElement: public ResourceElement
{
public:
	DECLARE_SKIN_ELEMENT (FontResourceElement, ResourceElement)

	FontResourceElement ();
	~FontResourceElement ();

	PROPERTY_VARIABLE (int, fontStyle, FontStyle)

	bool loadFont (SkinModel& model);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	FontResource* font;
};

//************************************************************************************************
// ControlStatement
/** Base Class for all statements.
Statements don't produce views directly. They are used to express conditions, loops, etc. */
//************************************************************************************************

class ControlStatement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ControlStatement, Element)
};

//************************************************************************************************
// DefineStatement
/** Defines one or multiple skin variables.
Any attribute="value" pair that appears in the Define statement defines a new variable and its value.
The variables are valid inside the <define> tag and in form instantiations from there.

Apart from simple assignments of constant literals or variable values, some extra directives can be used in the value (see code example).

\code{.xml}
<define keyName="productKey" listName="productKeyList">
<define number="@property:number" left="@eval:$number % 2" permission="aux$number">

<!-- Additional directives:
@property:propertyPath:
resolve the value of a property (starting with current controller)

@select:$variable:str1,str2,...,strn:
select a string from a list, indexed by a given variable

@eval:
evaluate an arithmetic expresssion, with operators
+ - * / %, parenthesis, constant literals and variable values ($)
-->
\endcode
*/
//************************************************************************************************

class DefineStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (DefineStatement, ControlStatement)

	DefineStatement ();

	const ObjectArray& getVariables () const  { return variables; }

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	ObjectArray variables; // of SkinVariable
};

//************************************************************************************************
// UsingStatement
/** Switches to another controller.
When the application creates a view from a form description, it provides a controller.
The controller is responsible for providing parameters, special objects and creating custom views.

Inside a form, the using statement can be used to switch to another controller.
Inside the using tag, this new controller will be the current controller that is queried for parameters etc. */
//************************************************************************************************

class UsingStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (UsingStatement, ControlStatement)

	enum Type
	{
		kController,
		kNamespace
	};

	UsingStatement (Type type = kController);

	PROPERTY_VARIABLE (Type, type, Type)
	PROPERTY_BOOL (optional, Optional)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// SwitchStatement
/** Selects one of multiple available alternative cases based on a property or variable value.

\code{.xml}
<switch property="myProperty">
 <case value="1">
  <!-- ... -->
 </case>
 <case value="0">
  <!-- ... -->
 </case>
 <default>
  <!-- ... -->
 </defautl>
</switch>
\endcode
*/
//************************************************************************************************

class SwitchStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (SwitchStatement, ControlStatement)

	SwitchStatement ();

	PROPERTY_MUTABLE_CSTRING (controller, Controller)
	PROPERTY_MUTABLE_CSTRING (defined, Defined)
	PROPERTY_BOOL (defineNegated, DefineNegated)

	virtual Element* getCaseElement (VariantRef value);
	Element* getDefaultElement () const;

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// CaseStatement
/** One alternative branch in a switch statement.
The contained elements are used if the variable or property has the given value.
\see SwitchStatement */
//************************************************************************************************

class CaseStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (CaseStatement, ControlStatement)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;

protected:
	friend class SwitchStatement;
	Vector<String> cases;
};

//************************************************************************************************
// DefaultStatement
/** The default case in a switch statement.
The contained elements are used if none of the case statements applies.*/
//************************************************************************************************

class DefaultStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (DefaultStatement, ControlStatement)

	DefaultStatement () {}

	// Element
	void loadFinished () override;
};

//************************************************************************************************
// IfStatement
/** Only produces the contained elements if the property or variable has the specified "value".
<if> is a shorthand notation for a <switch> with only one case.
\see SwitchStatement */
//************************************************************************************************

class IfStatement: public SwitchStatement
{
public:
	DECLARE_SKIN_ELEMENT (IfStatement, SwitchStatement)

	PROPERTY_STRING (value, Value)

	// SwitchStatement
	Element* getCaseElement (VariantRef value) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Vector<String> cases;
};

//************************************************************************************************
// ForEachStatement
/** Describes a loop that repeats the contained elements.
There are two ways to use a for statement:

1. Using an integer variable that starts at "start" and is incremented after each pass, until "count" passes hae been performed:
2. Using a string variable that iterates the string token in "in" (separeted by spaces):

\code{.xml}
<!-- Example 1 -->
<foreach variable="$index" count="numTools">
<!-- Example 2 -->
<foreach variable="$n" in="$names">
\endcode

*/
//************************************************************************************************

class ForEachStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (ForEachStatement, ControlStatement)

	PROPERTY_STRING (countString, CountString)
	PROPERTY_STRING (startString, StartString)
	PROPERTY_STRING (inString, InString)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// ZoomStatement
/** Defines a zoom factor that will be applied to views created inside the <zoom> tag.
	The sizes and selected visual style metrics of these views will be multiplied by the zoom factor.

\code{.xml}
<zoom factor="1.5">
	<Button size="10,10,100,100" title="Zoomed"> <!-- resulting size will be "15,15,150,150" -->
</zoom>
\endcode
*/
//************************************************************************************************

class ZoomStatement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (ZoomStatement, ControlStatement)

	ZoomStatement ();

	enum Modes
	{
		kRelative = 1,	///< a new zoom factor is applied relative to the current zoom factor (default)
		kAbsolute		///< a new absolute zoom factor is applied to scoped views
	};

	PROPERTY_VARIABLE (float, zoomFactor, ZoomFactor)
	PROPERTY_VARIABLE (int, mode, Mode)
	DECLARE_STYLEDEF (modes)
	
	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// VisualStyleSelectorElement
/** Defines a variable for a Style that dynamically delegates to one of the specified styles (which are defined as usual in the <Styles> section).
The variables are valid inside the <styleselector> tag and in form instantiations from there.
The active style is selected via a numeric parameter as index (specified as "name").

\code{.xml}
<!-- Example: changing the style of a button based on value of a parameter "selected":
(assuming "RedButtonStyle" and "BlueButtonStyle" are defined in <Styles>)
-->
<styleselector name="selected" variable="$ButtonStyle" styles="RedButtonStyle BlueButtonStyle">
	<Button name="close" style="$ButtonStyle"/>
</styleselector>
\endcode
*/ 
//************************************************************************************************

class VisualStyleSelectorElement: public ControlStatement
{
public:
	DECLARE_SKIN_ELEMENT (VisualStyleSelectorElement, ControlStatement)

	PROPERTY_MUTABLE_CSTRING (variableName, VariableName)
	PROPERTY_MUTABLE_CSTRING (propertyId, PropertyId)
	PROPERTY_MUTABLE_CSTRING (controller, Controller)

	const Vector<MutableCString>& getStyleNames () const { return styleNames; }

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Vector<MutableCString> styleNames;
};

//************************************************************************************************
// ElementSizeParser
//************************************************************************************************

class ElementSizeParser
{
public:
	ElementSizeParser (): parseFlags (0) {}

	PROPERTY_STRING (sizeString, SizeString)
	PROPERTY_STRING (widthString, WidthString)
	PROPERTY_STRING (heightString, HeightString)
	PROPERTY_OBJECT (DesignSize, designSize, DesignSize)

	enum Flags
	{
		kResolveSize = 1<<0,
		kResolveRect = 1<<1,
		kResolveWidth = 1<<2,
		kResolveHeight = 1<<3,

		kLastSizeParserFlag = kResolveHeight
	};

	PROPERTY_FLAG (parseFlags, kResolveSize, mustResolveSize)
	PROPERTY_FLAG (parseFlags, kResolveRect, mustResolveRect)
	PROPERTY_FLAG (parseFlags, kResolveWidth, mustResolveWidth)
	PROPERTY_FLAG (parseFlags, kResolveHeight, mustResolveHeight)

	/** Try to parse a size from attributes "size", "rect" or "width" + "height" */
	Rect& trySizeAttributes (const SkinAttributes& a);
	
	/** Try to parse a size resolving any variables from previously parsed sizeString, widthString and heightString */
	void resolveSize (SkinWizard& wizard);

protected:
	Rect size;
	int parseFlags;
	
	bool trySize (const SkinAttributes& a);
	bool tryRect (const SkinAttributes& a);
	void tryWidth (const SkinAttributes& a);
	void tryHeight (const SkinAttributes& a);
	
	void parseWidth (StringRef resolvedWidth);
	void parseHeight (StringRef resolvedHeight);
	void parseSize (StringRef resolvedString);
	void parseRect (StringRef resolvedString);
};

//************************************************************************************************
// ViewElement
/** The basic view class.
The View class is the general view class that all other view classes inherit from.
It has a size and some more general properties. Not all of these properties have a meaning in all derived view classes.
When views are nested, the "attach" attribute specifies some basic layout behavior.
Defines can be used for style and inside size, width and height attributes:
 
 \code{.xml}
 <define w="10" z="MyStyle">
 	<View style="$z" size="0,0,$w,20" attach="all">
 	<View width="$w" height="20" attach="hcenter top bottom"/>
 </define>
 \endcode
 */
//************************************************************************************************

class ViewElement: public Element,
				   public ElementSizeParser,
				   public ISkinViewElement
{
public:
	DECLARE_SKIN_ELEMENT (ViewElement, Element)

	ViewElement ();
	~ViewElement ();

	RectRef CCL_API getSize () const override { return size; } // ISkinViewElement
	void CCL_API setSize (RectRef _size) override { size = _size; } // ISkinViewElement
	
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (tooltip, Tooltip)
	PROPERTY_VARIABLE (int, sizeMode, SizeMode)
	PROPERTY_MUTABLE_CSTRING (styleClass, StyleClass)
	PROPERTY_STRING (sizeLimitsString, SizeLimitsString)
	PROPERTY_OBJECT (SizeLimit, sizeLimits, SizeLimits)
	PROPERTY_OBJECT (StyleFlags, options, Options)
	
	enum LayerBackingType
	{
		kLayerBackingFalse,
		kLayerBackingTrue,
		kLayerBackingOptional
	};

	DECLARE_STYLEDEF (layerBackingTypes)
	PROPERTY_VARIABLE (LayerBackingType, layerBackingType, LayerBackingType)

	enum AccessibilityType
	{
		kAccessibilityDisabled,
		kAccessibilityEnabled		
	};

	DECLARE_STYLEDEF (accessibilityTypes)
	PROPERTY_VARIABLE (AccessibilityType, accessibilityType, AccessibilityTypes)

	const SkinAttributes* getDataAttributes () const;
	const SkinAttributes* getFlexAttributes () const;

	struct CreateArgs
	{
		SkinWizard& wizard;
		IUnknown* controller;

		CreateArgs (SkinWizard& w, IUnknown* c = nullptr)
		: wizard (w), controller (c)
		{}
	};

	class CreateArgsEx: public Object,
						public ISkinCreateArgs
	{
	public:
		CreateArgsEx (ViewElement* element, const CreateArgs& args);

		// ISkinCreateArgs
		ISkinViewElement* CCL_API getElement () const override;
		tbool CCL_API getVariable (Variant& value, StringID name) const override;
		IVisualStyle* CCL_API getVisualStyleForElement () const override;

		CLASS_INTERFACE (ISkinCreateArgs, Object)

	protected:
		ViewElement* element;
		const CreateArgs& args;
	};

	virtual Rect& getDefaultSize (Rect& r) const;
	VisualStyle* determineVisualStyle (const CreateArgs& args);
	virtual View* createView (const CreateArgs& args, View* view = nullptr);

	virtual void viewCreated (View* view);	///< when view has been created (after all childs have been added)
	virtual void viewAdded (View* parent, View* child, ViewElement* childElement, SkinWizard& wizard);	///< when view has been added

	virtual bool appendOptions (String& optionsString) const;

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

	CLASS_INTERFACE (ISkinViewElement, Element)

protected:
	SharedPtr<VisualStyle> visualStyle;
	MutableSkinAttributes* dataAttributes;
	MutableSkinAttributes* flexAttributes;

	MutableSkinAttributes& getMutableDataAttributes ();
	MutableSkinAttributes& getMutableFlexAttributes ();
	
	struct AccessibilityInfo
	{
		MutableCString id;
		MutableCString proxyId;
		MutableCString labelProviderId;
		MutableCString valueProviderId;

		bool isEmpty () const { return id.isEmpty () && proxyId.isEmpty () && labelProviderId.isEmpty () && valueProviderId.isEmpty (); }
	};
	AccessibilityInfo* accessibilityInfo;
	AccessibilityInfo& getAccessibilityInfo ();

	enum Flags
	{
		kResolveName = kLastSizeParserFlag << 1,
		kResolveTitle = kLastSizeParserFlag << 2,
		kResolveTip = kLastSizeParserFlag << 3,
		kResolveSizeLimits = kLastSizeParserFlag << 4,

		kLastViewElementFlag = kResolveSizeLimits
	};

	PROPERTY_FLAG (parseFlags, kResolveName, mustResolveName)
	PROPERTY_FLAG (parseFlags, kResolveTitle, mustResolveTitle)
	PROPERTY_FLAG (parseFlags, kResolveTip, mustResolveTip)
	PROPERTY_FLAG (parseFlags, kResolveSizeLimits, mustResolveSizeLimits)
	
	void calculateViewSize (Rect& r, View* view) const;
	void applyZoomFactor (Rect& r, View* view, const CreateArgs& args) const;

	// ISkinViewElement
	tbool CCL_API getDataDefinition (String& string, StringID id) const override;
	StyleFlags CCL_API getStandardOptions () const override;
};

//************************************************************************************************
// ImageViewElement
/** Shows an Image or color.
An ImageView can draw
- a static image (attribute "image")
- an image provided by an image parameter (attribute "provider")
- a color ("backcolor" from visual style, option "colorize")

The image will be resized to the view area.

An optional parameter "selectname" can be used to chose the image frame to be displayed:
By default, the frame "nornal" is used when the parameter value is zero, otherwise "pressed".
With option "framesbyname", the parameter value is interpreted as a frame name. */
//************************************************************************************************

class ImageViewElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (ImageViewElement, ViewElement)

	ImageViewElement ();

	PROPERTY_MUTABLE_CSTRING (imageName, imageName)
	PROPERTY_MUTABLE_CSTRING (selectName, SelectName)
	PROPERTY_MUTABLE_CSTRING (providerName, ProviderName)
	PROPERTY_MUTABLE_CSTRING (dataTargetName, DataTargetName)
	PROPERTY_OBJECT (StyleFlags, imageStyle, ImageStyle)
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;

protected:
	SharedPtr<Image> image;
};

//************************************************************************************************
// FormElement
/** A Form is the top level view building block in a skin, accessible by name.
There are two main use cases for forms:
1. A form is the entry point in the skin when the application wants to create a specific view.
2. A form can by instantiated from another form using the <View> tag.
This allows structuring large view hierarchies into smaller forms, and reusing the same form in different places.

As an ImageView, a Form can also have a backgoround image or color. */
//************************************************************************************************

class FormElement: public ImageViewElement
{
public:
	DECLARE_SKIN_ELEMENT (FormElement, ImageViewElement)

	FormElement ();

	PROPERTY_OBJECT   (StyleFlags, windowStyle, WindowStyle)
	PROPERTY_OBJECT   (StyleFlags, formStyle,FormStyle)
	PROPERTY_STRING   (firstFocus, FirstFocus)
	PROPERTY_STRING	  (helpIdentifier, HelpIdentifier)

	// ViewElement
	const Element* getTranslationScope () const override { return this; }
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// FormDelegateElement
/** Dynamically creates a content view.
Every time the Delegate gets attached, it creates a new child view using the given form name and controller.
When the delegagte gets removed, the child view is destroyed.

The controller attribute is optional, default is the current controller.

A popular usage scenario is a <Delegate> inside a <Variant> controlled by a property.
Everytime the property change is triggered by the application, the content gets recreated (see code example).

\code{.xml}
<!-- the option "selectalways" here ensures that the view
is recreated although the property value did not really change -->
<Variant property="hasContent" options="selectalways">
   <Delegate form.name="MyContent" controller="IMyController"/>
</Variant>
\endcode
*/
//************************************************************************************************

class FormDelegateElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (FormDelegateElement, ViewElement)

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_MUTABLE_CSTRING (controllerName, ControllerName)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	bool appendOptions (String& string) const override;
};

//************************************************************************************************
// ZoomableViewElement
/** Creates the content view specified in form.name and zooms it to fit into the available space.
	The aspect ratio of the content view is preserved.

\code{.xml}
<Zoomable form.name="MyContent" factors="0.5 1.0 1.5 2" size="0,0,100,100" attach="all"/>
\endcode
*/
//************************************************************************************************

class ZoomableViewElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (ZoomableViewElement, ViewElement)

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

private:
	Vector<float> supportedZoomfactors;
};

//************************************************************************************************
// CursorViewElement
/** Changes the mouse cursor.
While the mouse is over the CursorView, the mouse cursor changes to the specified cursor. \see CursorElement */
//************************************************************************************************

class CursorViewElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (CursorViewElement, ViewElement)

	PROPERTY_MUTABLE_CSTRING (cursorName, CursorName)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// HelpAnchorElement
/** Allows setting a helpid for the area of this view. */
//************************************************************************************************

class HelpAnchorElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (HelpAnchorElement, ViewElement)

	PROPERTY_STRING (helpIdentifier, HelpIdentifier)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// WindowClassElement
/** A WindowClass describes a potential screen content.
A WindowClass is identified by it's "name".
The most important attributes are "form.name" and "controller", which are used to create a view.
A Command catgegory and name can be specified, that will open or close the window class.

The window manager of the application maintains a parameter for each window class that reflects the
open/closed state of the window. It can be used to toggle it.

It depends on the application how a window class will appear on the screen.
In the simplest case, it might just open a window for each window class.
A more sophisticated approach is a workspace \see WorkspaceElement.
In this case, the "group" attribute specifies in which workspace frames the window class can appear.

\code{.xml}
<Toggle name="://WindowManager/MyClass"/>
\endcode
*/
//************************************************************************************************

class WindowClassElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (WindowClassElement, Element)

	WindowClassElement ();
	~WindowClassElement ();

	WindowClass& getWindowClass () const;

	// Element
	const Element* getTranslationScope () const override { return this; }
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;

protected:
	mutable WindowClass* windowClass;
};

//************************************************************************************************
// WorkspaceElement
/** A Workspace is a collection of multiple perspectives, that the application can switch between.

A special view called "PerspectiveContainer" can be placed e.g. in the application window.
The view tree created by each perspective will be inserted into the PerspectiveContainer.

\see PerspectiveElement

\code{.xml}
<View name="PerspectiveContainer" size="0,0,1024,768"/>
\endcode
*/
//************************************************************************************************

class WorkspaceElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (WorkspaceElement, Element)

	WorkspaceElement ();
	~WorkspaceElement ();

	PROPERTY_OBJECT (StyleFlags, windowStyle, WindowStyle)

	Workspace* createWorkspace (SkinModel& model);

	void loadResources (SkinModel& model);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Workspace* workspace;
	bool storable;

	static ObjectList workspaceCleanupList;

	StringID getWorkspaceID () const;
	void discardWorkspace ();
};

//************************************************************************************************
// PerspectiveElement
/** A perspective defines an arrangement of nested frames, that can each house a window class.
\see PerspectiveElement*/
//************************************************************************************************

class PerspectiveElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (PerspectiveElement, Element)

	PerspectiveElement ();

	PROPERTY_STRING (title, Title)
	PROPERTY_MUTABLE_CSTRING (iconName, IconName)
	PROPERTY_OBJECT (StyleFlags, style, Style)
	PROPERTY_VARIABLE (OrientationType, orientation, Orientation)
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)
	PROPERTY_MUTABLE_CSTRING (styleClass, StyleClass)
	PROPERTY_OBJECT (StyleFlags, backgroundOptions, BackgroundOptions)
	PROPERTY_MUTABLE_CSTRING (backCommandCategory, BackCommandCategory)
	PROPERTY_MUTABLE_CSTRING (backCommandName, BackCommandName)

	Perspective* createPerspective (SkinModel& model);

	// Element
	const Element* getTranslationScope () const override { return this; }
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// FrameElement
/** A frame is an area in a perspective that can be either empty or occupied by the view of a window class.

The "groups" attribute is a list of group names from window classes, separated by spaces. It specifies which window classes can appear in the frame.
Complex arrangements can be built with horizontal or vertical frame groups, which will create a layout container for their contained frames.

\see PerspectiveElement

\code{.xml}
<Perspective name="MyPerspective">
	<Frame options="vertical">
		<Frame name="MyFrame1" groups="group1"/>
		<Frame name="MyFrame2" groups="group2 group3" default="windowClassA"/>
	</Frame>
</Perspective>
\endcode
 */
//************************************************************************************************

class FrameElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (FrameElement, Element)

	FrameElement ();

	PROPERTY_MUTABLE_CSTRING (windowID, WindowID)
	PROPERTY_MUTABLE_CSTRING (condition, Condition)
	PROPERTY_MUTABLE_CSTRING (decor, Decor)
	PROPERTY_STRING (friendID, FriendID)
	PROPERTY_VARIABLE (int, style, Style)
	PROPERTY_OBJECT (Point, size, Size)
	PROPERTY_VARIABLE (float, fillFactor, FillFactor)

	DockPanelItem* createItem (FrameItem* frameItem);
	bool isGroup ();

	static void createChildItems (Element& parentElement, FrameItem& parentItem);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;

protected:
	Vector<MutableCString> groups;
};

//************************************************************************************************
// EmbeddedFrameElement
/** An EmbeddedFrame defines a sub tree of frames that can appear in the form of a parent class.
The EmbeddedFrame element must appear in two places: placed inside the form of a "parent" window class
and placed in the root frame of a perspective with the same name and a parent class. It defines a separate subtree of frames.

When a window class is about to be opened, the workspace system ensures that the parent class, whose view contains the EmbeddeFrame view gets opened first.

\code{.xml}
<!-- inside form of parent window -->
<Form name="MyParentForm">
	<!-- ... -->
	<EmbeddedFrame name="Embedded"/>
</Form>

<!-- in root frame of perspective -->
<EmbeddedFrame name="Embedded" parent.class="MyParentClass">
	<Frame options="vertical">
		<Frame name="A" groups="a"/>
		<Frame name="B" groups="b"/>
	</Frame>
</EmbeddedFrame>
\endcode
 */
//*********************1***************************************************************************

class EmbeddedFrameElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (EmbeddedFrameElement, ViewElement)

	EmbeddedFrameElement ();

	PROPERTY_MUTABLE_CSTRING (workspaceID, WorkspaceID)
	PROPERTY_MUTABLE_CSTRING (parentClassID, ParentClassID) ///< only when used in a workspace description
	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)

	// ViewElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// ParameterElement
/** Creates a parameter that can be used in the skin.
Sometimes parameters are required for controlling some activity in the skin, but have no meaning to the application logic.
These parameters can be defined inside a <Perspective> element. In a form description, these parameters can be addressed via the "CustomParams" controller of a perspective.

\code{.xml}
<Perspective name="myPerspective">
	<!-- ... -->
	<Parameter name="myParam" type="int" range="0,32767"/>
</Perspective>

<!-- address via "CustomParams" controller of a perspective: -->
<Divider name="://Workspace/myWorkspaceID/myPerspectiveID/CustomParams/myParam"/>
\endcode
*/
//************************************************************************************************

class ParameterElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ParameterElement, Element)

	DECLARE_STYLEDEF (types)
	DECLARE_STYLEDEF (options)

	ParameterElement ();

	PROPERTY_VARIABLE (int, type, Type)
	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_STRING (range, Range)
	PROPERTY_STRING (value, Value)

	IParameter* createParameter ();

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// SpaceElement
/** An view with the initial predefined size "Spacing".
A Space view  gets the width and height "Spacing" from the theme, if it has no or an empty specified size.
Apart from that it behaves the same as a view.*/
//************************************************************************************************

class SpaceElement: public ViewElement
{
public:
	DECLARE_SKIN_ELEMENT (SpaceElement, ViewElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
	bool setAttributes (const SkinAttributes& a) override;
};

//************************************************************************************************
// NullSpaceElement
/** An empty view (with initially empty size).
The Null element is only used to clarify the intended meaning when an empty view is used somewhere.
It behaves the same as a view. */
//************************************************************************************************

class NullSpaceElement: public SpaceElement
{
public:
	DECLARE_SKIN_ELEMENT (NullSpaceElement, SpaceElement)

	// ViewElement
	View* createView (const CreateArgs& args, View* view) override;
};

//************************************************************************************************
// StyleElement
/** Visual Style definition.
A Style defines the visual appearance of a view. It is collection of definitions for images, colors, metrics and fonts.
It can be assigned to a view element via it's "style" attribute.
Styles with the attribute "appstyle" can also be accessed directly by the application for specific purposes.

\code{.xml}
<Style name="MyStyle">
	<Font name="textfont" face="Arial" size="10" style="bold"/>
	<Color name="textcolor" color="#666666" />
	<Image name="background" image="SomeImage"/>
	<Metric name="padding.left" value="7"/>
	<Align name="textalign" align="left"/>
	<Options name="someOption" value="23"/>
	<Options name="someOption" type="Align.align" options="left top"/>
</Style>
\endcode
*/
//************************************************************************************************

class StyleElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (StyleElement, Element)

	StyleElement ();
	~StyleElement ();

	DECLARE_STYLEDEF (textOptions)

	VisualStyle& getStyle ();
	PROPERTY_BOOL (appStyle, AppStyle)
	PROPERTY_BOOL (overrideStyle, Override)

	virtual void loadResources (SkinModel& model);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadFinished () override;
	bool isOverrideEnabled () const override;

protected:
	SharedPtr<VisualStyle> style;
	MutableCString inherit;

	struct Pair
	{
		Pair (CStringRef _name, CStringRef _ref)
		: name (_name), ref (_ref) {}
		Pair (CStringRef _name = nullptr, StringRef _ref = nullptr)
		: name (_name), ref (_ref) {}
		Pair (const Pair& other)
		: name (other.name), ref (other.ref) {}

		MutableCString name;
		MutableCString ref;
	};

	LinkedList<Pair> images;
 	LinkedList<Pair> colors;

	virtual VisualStyle* newStyle ();
};

//************************************************************************************************
// ThemeStyleElement (tag: ThemeElements)
//************************************************************************************************

class ThemeStyleElement: public StyleElement
{
public:
	DECLARE_SKIN_ELEMENT (ThemeStyleElement, StyleElement)

	// StyleElement
	bool setAttributes (const SkinAttributes& a) override;
	void loadFinished () override;
	void loadResources (SkinModel& model) override;
};

//************************************************************************************************
// StyleAliasElement
/** Visual Style Alias definition.
A visual style that dynamically delegates to one of the listed styles based on a paramter value.
The parameter must be globally accessible. The value of the parameter is interpreted as an index in the list of available styles.

\code{.xml}
<StyleAlias name="MyStyle"
		parameter="://hostapp/Configuration/styleIndex"
		styles="MyFirstStyle MySecondStyle MyThirdStyle">
</StyleAlias>
\endcode
*/
//************************************************************************************************

class StyleAliasElement: public StyleElement
{
public:
	DECLARE_SKIN_ELEMENT (StyleAliasElement, StyleElement)

	StyleAliasElement ();

	// StyleElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	void loadResources (SkinModel& model) override;

private:
	MutableCString paramName;
	Vector<MutableCString> styleNames;
};

//************************************************************************************************
// ColorElement
/** Defines a color in a Visual Style.
\see StyleElement */
//************************************************************************************************

class ColorElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ColorElement, Element)

	PROPERTY_MUTABLE_CSTRING (color, Color)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// MetricElement
/** Defines a metric in a Visual Style.
\see StyleElement */
//************************************************************************************************

class MetricElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (MetricElement, Element)

	MetricElement ();

	PROPERTY_VARIABLE (float, value, Value)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// StringElement
/** Defines a string in a Visual Style.
\see StyleElement */
//************************************************************************************************

class StringElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (StringElement, Element)

	PROPERTY_MUTABLE_CSTRING (value, Value)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// FontElement
/** Defines a font in a Visual Style.
\see StyleElement */
//************************************************************************************************

class FontElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (FontElement, Element)

	DECLARE_STYLEDEF (fontStyles)
	DECLARE_STYLEDEF (smoothingModes)
	DECLARE_STYLEDEF (textTrimModes)

	PROPERTY_OBJECT (Font, font, Font)
	PROPERTY_MUTABLE_CSTRING (themeId, ThemeID)

	static void applyFontSize (Font& font, StringRef sizeString);
	static void applyThemeFont (Element* caller, Font& font, StringID themeId);

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// OptionsElement
/** Defines an integer value or options enumeration in a Visual Style.
\see StyleElement */
//************************************************************************************************

class OptionsElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (OptionsElement, Element)

	OptionsElement ();

	PROPERTY_VARIABLE (int, options, Options)
	PROPERTY_MUTABLE_CSTRING (typeName, TypeName)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// AlignElement
/** Defines an alignment in a Visual Style.
\see StyleElement */
//************************************************************************************************

class AlignElement: public OptionsElement
{
public:
	DECLARE_SKIN_ELEMENT (AlignElement, OptionsElement)

	DECLARE_STYLEDEF (alignStyles)

	// OptionsElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// ShapeColorMappingElement
//************************************************************************************************

class ShapeColorMappingElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ShapeColorMappingElement, Element)

	ShapeColorMappingElement ();

	PROPERTY_VARIABLE (Color, color, Color)
	PROPERTY_POINTER (ColorScheme, scheme, Scheme)
	PROPERTY_MUTABLE_CSTRING (nameInScheme, NameInScheme)
	PROPERTY_VARIABLE (Color, referenceColor, ReferenceColor)

	// Element
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

//************************************************************************************************
// ColorSchemeElement
//************************************************************************************************

class ColorSchemeElement: public Element
{
public:
	DECLARE_SKIN_ELEMENT (ColorSchemeElement, Element)

	// Element
	bool mergeElements (Element& other) override;
	void loadFinished () override;
};

//************************************************************************************************
// ColorSchemeColorElement
//************************************************************************************************

class ColorSchemeColorElement: public ColorElement
{
public:
	DECLARE_SKIN_ELEMENT (ColorSchemeColorElement, ColorElement)

	ColorSchemeColorElement ();

	PROPERTY_STRING (luminanceSegments, LuminanceSegments)
	PROPERTY_STRING (saturationSegments, SaturationSegments)
	PROPERTY_STRING (contrastSegments, ContrastSegments)

	PROPERTY_BOOL (hueFixed, HueFixed)
	PROPERTY_BOOL (slCombined, SLCombined)
	PROPERTY_BOOL (contrastFixed, ContrastFixed)
	PROPERTY_BOOL (invertible, Invertible)
	PROPERTY_VARIABLE (int, invertedValue, InvertedValue)
	PROPERTY_VARIABLE (int, alphaValue, AlphaValue)
	PROPERTY_MUTABLE_CSTRING (invertedColor, InvertedColor)

	// ColorElement
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinmodel_h
