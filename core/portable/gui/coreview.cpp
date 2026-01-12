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
// Filename    : core/portable/gui/coreview.cpp
// Description : View class
//
//************************************************************************************************

#define DEBUG_FOCUS (0 && DEBUG)

#include "coreview.h"

#include "core/gui/corebitmapprimitives.h"
#include "core/text/coretexthelper.h"

#include "core/public/coreprimitives.h"
#include "core/public/gui/coreuiproperties.h"

#include "core/portable/gui/coretouchinput.h"
#include "core/portable/corepersistence.h"

#if DEBUG
#include "core/system/coredebug.h"
#endif

namespace Core {
namespace Portable {

//************************************************************************************************
// FocusFinder - copied from CCL::FocusNavigator
//************************************************************************************************

class FocusFinder
{
public:
	static bool isFocusable (const View* view)
	{
		return view->wantsFocus () && view->isEnabled (); 
	}

	static bool isFocusOrChild (View* view, View* focusView)
	{
		if(focusView != nullptr)
			if(view == focusView || (view->asContainer () && view->asContainer ()->isChildView (focusView)))
				return true;
		return false;
	}

	static View* getFirst (View* parent)
	{
		return findNextDeep (parent, nullptr); 
	}

	static View* getLast (View* parent)
	{
		return findPreviousDeep (parent, nullptr); 
	}

	static View* getNext (View* view)
	{
		if(view)
		{
			// try childs
			if(View* deepChild = getFirst (view))
				return deepChild;

			// try following siblings
			if(View* sibling = getNextSibling (view))
				return sibling;

			RootView* rootView = view->getRootView ();
			if(rootView && rootView != view)
				return getNext (rootView);
		}
		return nullptr;
	}

	static View* getPrevious (View* view)
	{
		if(view)
		{
			View* parent = view->getParent ();
			if(parent)
			{
				// try preceding siblings
				if(View* v = findPreviousDeep (parent, view))
					return v;

				// try parent
				if(isFocusable (parent))
					return parent;

				// up one level (siblings of parent)
				return getPrevious (parent);
			}
			else
				return getLast (view);
		}
		return nullptr;
	}

protected:
	static View* findNextDeep (View* parent, View* startView)
	{
		if(!parent)
			return nullptr;

		bool skip = startView != nullptr; // skip all up to startView
		if(ContainerView* cv = parent->asContainer ())
			VectorForEach (cv->getChildren (), View*, child)
				if(skip)
				{
					if(child == startView)
						skip = false;
					continue;
				}

				// try this view
				if(isFocusable (child))
					return child;

				// try childs
				if(View* deepChild = getFirst (child))
					return deepChild;
			EndFor
		return nullptr;
	}

	static View* findPreviousDeep (View* parent, View* startView)
	{
		if(!parent)
			return nullptr;

		bool skip = startView != nullptr; // skip all up to startView
		if(ContainerView* cv = parent->asContainer ())
			VectorForEachReverse (cv->getChildren (), View*, child)
				if(skip)
				{
					if(child == startView)
						skip = false;
					continue;
				}

				// try childs
				if(View* deepChild = getLast (child))
					return deepChild;

				// try this view
				if(isFocusable (child))
					return child;
			EndFor
		return nullptr;
	}

	static View* getNextSibling (View* view)
	{
		View* parent = view->getParent ();
		if(parent)
		{
			// try following siblings
			if(View* v = findNextDeep (parent, view))
				return v;

			// continue with siblings of parent (one level upwards)
			return getNextSibling (parent);
		}
		return nullptr;
	}
};

} // namespace Portable
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// StyleManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (StyleManager)

//************************************************************************************************
// ViewAttributes
//************************************************************************************************

bool ViewAttributes::parseColor (Color& color, CStringPtr colorString)
{
	if(colorString && colorString[0] == '#')
	{
		int r = 0, g = 0, b = 0, a = -1;

		Text::StringParser p (colorString + 1);
		if(!p.parseHexByte (r))
			return false;

		p.parseHexByte (g);
		p.parseHexByte (b);
		p.parseHexByte (a);

		color.red = (uint8)r; color.green = (uint8)g; color.blue = (uint8)b;
		color.alpha = a >= 0 ? (uint8)a : 0xFF;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ViewAttributes::decodeColor (Color& color, const AttributeValue& a)
{
	if(a.getType () == AttributeValue::kInt)
		color ((uint32)a.getInt ());
	else
		parseColor (color, a.getString ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color ViewAttributes::getColor (const Attributes& a, CStringPtr name, Color defaultColor)
{
	Color color = defaultColor;
	if(const Attribute* colorAttr = a.lookup (name))
		decodeColor (color, *colorAttr);
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewAttributes::getAlign (const Attributes& a, CStringPtr name, int defAlign)
{
	if(CStringPtr alignString = a.getString (name))
		return EnumInfo::parseMultiple<ConstString> (alignString, Skin::Enumerations::alignment);
	else
		return defAlign;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewAttributes::getStyle (Style& style, const Attributes& a)
{
	style.setBackColor (getColor (a, kBackColor, style.getBackColor ()));
	style.setBackColorDisabled (getColor (a, kBackColorDisabled, style.getBackColor ()));
	style.setForeColor (getColor (a, kForeColor, style.getForeColor ()));
	style.setForeColorDisabled (getColor (a, kForeColorDisabled, style.getForeColor ()));
	style.setTextColor (getColor (a, kTextColor, style.getTextColor ()));
	style.setTextColorOn (getColor (a, kTextColorOn, style.getTextColor ())); // default textcolor.on to match textcolor
	style.setTextColorDisabled (getColor (a, kTextColorDisabled, style.getTextColor ()));
	style.setHiliteColor (getColor (a, kHiliteColor, style.getHiliteColor ()));
	style.setFontName (a.getString (kFont));
	style.setTextAlign (getAlign (a, kTextAlign, style.getTextAlign ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewAttributes::getOptions (const Attributes& a, const EnumInfo info[], CStringPtr name)
{
	return EnumInfo::parseMultiple<ConstString> (a.getString (name), info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewAttributes::getExlusiveOption (const Attributes& a, const EnumInfo info[], CStringPtr name, int defaultValue)
{
	return EnumInfo::parseOne<ConstString> (a.getString (name), info, defaultValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ViewAttributes::getInt (const Attributes& a, CStringPtr name, int defValue)
{
	return a.contains (name) ? (int)a.getInt (name) : defValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes* ViewAttributes::getStyleAttributes (const Attributes& a)
{
	if(const Attribute* attr = a.lookup (kStyle))
	{
		if(attr->getType () == Attribute::kString) // name of shared style
			return StyleManager::instance ().getStyle (attr->getString ());
		else
			return attr->getAttributes ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapReference ViewAttributes::getBitmap (const Attributes& a, CStringPtr name)
{
	return BitmapManager::instance ().getBitmap (a.getString (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewAttributes::autoSizeToBitmap (Rect& size, Bitmap* bitmap)
{
	if(bitmap == nullptr)
		return false;

	Rect imageSize;
	bitmap->getFrame (imageSize, 0);
	size.setWidth (imageSize.getWidth ());
	size.setHeight (imageSize.getHeight ());
	return true;
}

//************************************************************************************************
// StyleManager
//************************************************************************************************

StyleManager::StyleManager ()
: styleMap (128, ResourceAttributes::hashIntKey)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleManager::~StyleManager ()
{
	HashMapIterator<uint32, Attributes*> iter (styleMap);
	while(!iter.done ())
		delete iter.next ();
	styleMap.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StyleManager::loadStyles (FilePackage& package)
{
	int count = 0;
	Archiver::Format primaryFormat = Archiver::kJSON;
	IO::Stream* jsonStream = package.openStream (Skin::FileNames::kStyleFile1);
	if(jsonStream == nullptr)
	{
		jsonStream = package.openStream (Skin::FileNames::kStyleFile2);
		primaryFormat = Archiver::kUBJSON;
	}
	if(jsonStream != nullptr)
	{
		Deleter<IO::Stream> deleter (jsonStream);
		Attributes a (AttributeAllocator::getDefault ());
		AttributePoolSuspender suspender; // don't allocate from memory pool
		if(Archiver (jsonStream, primaryFormat).load (a))
		{
			if(const AttributeQueue* styleArray = a.getQueue (nullptr))
				VectorForEach (styleArray->getValues (), AttributeValue*, value)
					if(Attributes* styleAttr = value->detachAttributes ())
					{
						preprocessStyleAttributes (*styleAttr);

						CStringPtr name = styleAttr->getString (ResourceAttributes::kName);
						addStyle (name, styleAttr);
						count++;
					}
				EndFor
		}
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleManager::preprocessStyleAttributes (Attributes& styleAttributes)
{
	// pack color (as uint32) into int64
	for(int i = 0, num = styleAttributes.countAttributes (); i < num; i++)
	{
		// check for attributes containing "color"
		Attribute* attr = const_cast<Attribute*> (styleAttributes.getAttribute (i));
		if(attr->getID ().contains ("color") || attr->getID ().contains ("Color"))
			if(CStringPtr string = attr->getString ())
			{
				ConstString colorString (string);
				if(colorString.length () == 7 && colorString.at (0) == '#')
				{
					Color color;
					ViewAttributes::parseColor (color, colorString);
					attr->set ((int64)(uint32)color);
				}
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleManager::addStyle (CStringPtr name, Attributes* style)
{
	uint32 key = ResourceAttributes::nameToInt (name);
	ASSERT (styleMap.lookup (key) == 0)
	styleMap.add (key, style);

	addInheritedStyleAttributes (style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StyleManager::addInheritedStyleAttributes (Attributes* style)
{
	CStringPtr parentName = style->getString (ViewAttributes::kInherit); // name of inherited style
	if(parentName && !ConstString (parentName).isEmpty ()) 
	{
		// copy all attributes from parent style that this style does not override, except name
		if(const Attributes* parentStyle = StyleManager::instance ().getStyle (parentName))
			for(int i = 0, num = parentStyle->countAttributes (); i < num; i++)
			{
				const Attribute* parentAttribute = parentStyle->getAttribute (i);
				if(parentAttribute->getID () != ResourceAttributes::kName && !style->contains (parentAttribute->getID ()))
					style->addAttribute (*parentAttribute);
			}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes* StyleManager::getStyle (CStringPtr name) const
{
	if(ConstString (name).isEmpty ())
		return nullptr;
	return styleMap.lookup (ResourceAttributes::nameToInt (name));
}

//************************************************************************************************
// ThemePainter
//************************************************************************************************

DEFINE_STATIC_SINGLETON (ThemePainter)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainter::updateStyle ()
{
	if(const Attributes* a = StyleManager::instance ().getStyle ("Standard.Theme"))
		updateStyle (*a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainter::updateStyle (const Attributes& a)
{
	focusColor = ViewAttributes::getColor (a, "focuscolor", focusColor);
	if(CStringPtr borderStyle = a.getString ("focusborder"))
		focusBorder = static_cast<Skin::BorderStyles> (EnumInfo::parseMultiple<ConstString> (borderStyle, Skin::Enumerations::border));
	focusBorderWeight = ViewAttributes::getInt (a, "focusborderweight", focusBorderWeight);
}

//************************************************************************************************
// View
//************************************************************************************************

View::View (RectRef size)
: TViewBase<View> (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View::~View ()
{
	delete style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::getProperty (Property& value)
{
	switch(value.type)
	{
	case kViewSizeProperty :
		reinterpret_cast<ViewSizeProperty&> (value).size = getSize ();
		break;

	case kViewNameProperty :
		name.copyTo (reinterpret_cast<ViewNameProperty&> (value).name, ViewNameProperty::kMaxNameLength);
		break;

	case kViewClassProperty :		
		ConstString (getClassName ()).copyTo (reinterpret_cast<ViewClassProperty&> (value).name, ViewClassProperty::kMaxNameLength);
		break;

	case kColorProperty :
		{
			ColorProperty& cp = reinterpret_cast<ColorProperty&> (value);
			switch(cp.colorId)
			{
			case ColorProperty::kBackColor :
				cp.color = getStyle ().getBackColor ();
				break;
			case ColorProperty::kForeColor :
				cp.color = getStyle ().getForeColor ();
				break;
			}
		}
		break;

	case InterfaceProperty::kID:
		ImplementGetInterface<View, ICoreView> (this, value);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerView* View::asContainer ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootView* View::getRootView () const
{
	return parent ? parent->getRootView () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setStyle (Style* _style)
{
	if(style)
		delete style;
	TViewBase<View>::setStyle (_style);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::isEnabled () const
{
	return (options & kDisabled) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::enable (bool state)
{
	if(state)
		options &= ~kDisabled;
	else
		options |= kDisabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onTouchInput (const TouchEvent& e)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onWheelInput (const WheelEvent& e)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onKeyInput (const VirtualKeyEvent& e)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool View::onGestureInput (const GestureEvent& e)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::getHandledGestures (GestureVector& gestures, PointRef where)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::onIdle ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::setAttributes (const Attributes& a)
{
	Rect r (ViewAttributes::getSize (a));
	DpiSetting::instance ().scaleRect (r);
	setSize (r);

	if(const Attributes* styleAttr = ViewAttributes::getStyleAttributes (a))
	{
		Style* style = NEW Style;
		ViewAttributes::getStyle (*style, *styleAttr);
		setStyle (style);
	}
	
	int options = ViewAttributes::getOptions (a, Skin::Enumerations::viewOptions);
	if(options & kDisabled)
	{
		isAlwaysDisabled (true);
		enable (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr View::getConnectionType () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void View::connect (void* object)
{}

//************************************************************************************************
// ContainerView
//************************************************************************************************

ContainerView::ContainerView (RectRef size)
: ContainerViewBase<View> (size),
  owner (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerView::~ContainerView ()
{
	if(owner)
		owner->viewDestroyed (this);

	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::getProperty (Property& value)
{
	#if CORE_DEBUG_INTERNAL
	switch(value.type)
	{
	case kViewSourceProperty :
		sourceFile.copyTo (reinterpret_cast<ViewSourceProperty&> (value).sourceFile, ViewSourceProperty::kMaxSourceFileLength);
		break;

	default :
		View::getProperty (value);
		break;
	}
	#else
	View::getProperty (value);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ContainerView::countSubViews () const
{
	return children.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICoreView* ContainerView::getSubViewAt (int index) const
{
	return children.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContainerView* ContainerView::asContainer ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::addView (View* view)
{
	ContainerViewBase<View>::addView (view);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::removeView (View* view)
{
	if(RootView* rootView = getRootView ())
		rootView->viewRemoved (view);

	ContainerViewBase<View>::removeView (view);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::removeAll ()
{
	RootView* rootView = getRootView ();
	
	VectorForEachFast (children, View*, view)
		if(rootView)
			rootView->viewRemoved (view);
		delete view;
	EndFor
	children.removeAll ();
	invalidate ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

View* ContainerView::findView (const Point& where, bool deep, const ViewFilter* filter) const
{
	VectorForEachReverse (children, View*, v)
		if(v->getSize ().pointInside (where))
		{
			if(deep == true)
				if(ContainerView* vc = v->asContainer ())
				{
					Point where2 (where);
					where2.offset (-v->getSize ().left, -v->getSize ().top);
					View* result = vc->findView (where2, true, filter);
					if(result)
						return result;
				}

			if(!filter || filter->matches (v))
				return v;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ContainerView::findView (const ViewFilter& filter, bool deep) const
{
	VectorForEachFast (children, View*, v)
		if(filter.matches (v))
			return v;
		if(deep == true)
			if(ContainerView* vc = v->asContainer ())
			{
				View* result = vc->findView (filter, true);
				if(result)
					return result;
			}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerView::isChildView (const View* view, bool deep) const
{
	VectorForEachFast (children, View*, v)
		if(view == v)
			return true;

		if(deep == true)
			if(ContainerView* vc = v->asContainer ())
				if(vc->isChildView (view))
					return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ContainerView::onTouchInput (const TouchEvent& e)
{
	VectorForEachReverse (children, View*, view)
		if(view->isEnabled () && view->getSize ().pointInside (e.where))
		{
			TouchEvent e2 (e);
			e2.where.offset (-view->getSize ().left, -view->getSize ().top);
			if(view->onTouchInput (e2))
			{
				if(e.type == TouchEvent::kDown)
				{
					if(RootView* root = getRootView ())
						if(root->getTouchInputView () == nullptr)
							root->setTouchInputView (view);				
				}
				
				return true;
			}
		}
	EndFor		
	return false;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::onIdle ()
{
	VectorForEachFast (children, View*, view)
		view->onIdle ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContainerView::resizeToChildren ()
{
	Rect childSize;
	VectorForEachFast (children, View*, view)
		childSize.join (view->getSize ());
	EndFor
	size.setWidth (childSize.right);
	size.setHeight (childSize.bottom);
}

//************************************************************************************************
// RootView
//************************************************************************************************

bool RootView::gesturesEnabled = false;
void RootView::enableGestures (bool state) { gesturesEnabled = state; }

//////////////////////////////////////////////////////////////////////////////////////////////////

RootView::RootView (RectRef size, BitmapPixelFormat pixelFormat, RenderMode renderMode)
: ContainerView (size),
  RootViewBase (size, pixelFormat, renderMode),
  sizable (false),
  touchInputView (nullptr),
  touchInput (nullptr),
  modalView (nullptr),
  focusView (nullptr),
  savedFocusView (nullptr),
  modalResetPending (false)
{
	setupOffscreenList ();

	if(gesturesEnabled)
		touchInput = NEW TouchInputState (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootView::~RootView ()
{
	killModalView ();
	
	VectorForEachFast (offscreenList, Bitmap*, offscreen)
		delete offscreen;
	EndFor

	delete touchInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootView* RootView::getRootView () const
{
	return const_cast<RootView*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::setSize (RectRef newSize)
{
	if(isSizable () == false)
		return;

	if(newSize != size)
	{
		size = newSize;
		setupOffscreenList ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::setupOffscreenList ()
{
	VectorForEachFast (offscreenList, Bitmap*, offscreen)
		delete offscreen;
	EndFor
	offscreenList.removeAll ();

	activeBufferIndex = 0;
	lastDirtyRegion.setEmpty ();
	dirtyRegion.setEmpty ();
	targetSize = size;

	if(!size.isEmpty ())
	{
		int offscreenCount = 0;
		if(renderMode == kOffscreenMode)
			offscreenCount = 1;
		else if(renderMode == kFlipMode)
			offscreenCount = 2;
		
		for(int i = 0; i < offscreenCount; i++)
			offscreenList.add (NEW Bitmap (size.getWidth (), size.getHeight (), pixelFormat));

		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::invalidate (RectRef rect)
{
	addDirtyRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::draw (const DrawEvent& e)
{
	// clear background
	e.graphics.fillRect (e.updateRect, getStyle ().getBackColor ());
	
	ContainerView::draw (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::scrollClient (RectRef rect, PointRef delta)
{
	ASSERT (offscreenList.count () == 1)
	if(offscreenList.count () != 1)
		return false;

	Bitmap* offscreen = offscreenList[0];
	ASSERT (offscreen->getFormat () == kBitmapRGBAlpha)
	if(offscreen->getFormat () != kBitmapRGBAlpha)
		return false;

	BitmapData& data = offscreen->accessForWrite ();
	BitmapPrimitives32::scrollRect (data, rect, delta);

	// invalidate areas
	{
		Rect r1, r2;
		if(delta.y < 0)
			r1 (rect.left, rect.bottom + delta.y, rect.right, rect.bottom);
		else
			r1 (rect.left, rect.top, rect.right, rect.top + delta.y);

		if(delta.x < 0)
			r2 (rect.right + delta.x, rect.top, rect.right, rect.bottom);
		else
			r2 (rect.left, rect.top, rect.left + delta.x, rect.bottom);

		if(!r1.isEmpty ())
			invalidate (r1);
		if(!r2.isEmpty ())
			invalidate (r2);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* RootView::getModalView () const
{
	return modalView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::setModalView (View* view)
{
	if(view != modalView)
	{
		killModalView ();

		modalView = view;
		
		if(modalView)
		{
			if(touchInput)
				touchInput->setRootView (modalView);
			addView (modalView);

			savedFocusView = focusView; // save old focus view
			
			if(modalView->asContainer () == nullptr || modalView->wantsFocus ()) // focus modal view directly
				setFocusView (modalView);
			else
				findFirstFocusView (); // find new focus inside in modal view
		}
		else // modal view removed, try to restore focus
		{
			if(savedFocusView)
				setFocusView (savedFocusView),
				savedFocusView = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::killModalView ()
{
	if(modalView)
	{
		View* oldModal = modalView;
		removeView (modalView);
		delete oldModal;
	}
	modalView = nullptr;
	
	if(touchInput)
		touchInput->setRootView (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::resetModalViewDeferred ()
{
	if(modalView != nullptr)
		modalResetPending = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* RootView::getFocusView () const
{
	return focusView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::setFocusView (View* view)
{
	if(view != focusView)
	{
		if(focusView)
		{
			focusView->onFocus (false);
			if(modalView == focusView)
				if(!(modalView->asContainer () && modalView->asContainer ()->isChildView (view)))
					killModalView (); // don't kill the modalView if the modalView is the parent of the new focus view.
			
		}
		
		focusView = view;
		if(focusView)
			focusView->onFocus (true);

		if(modalView && view && view != modalView)
			if(!(modalView->asContainer () && modalView->asContainer ()->isChildView (view)))
				killModalView ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::findFirstFocusView ()
{
	View* startView = modalView;
	if(!startView)
		startView = this;

	View* newFocusView = FocusFinder::getNext (startView);
	setFocusView (newFocusView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::viewRemoved (View* view)
{
	if(view == modalView)
	{	
		modalView = nullptr;
		if(touchInput)
			touchInput->setRootView (this);
	}
	
	if(view == touchInputView || (view->asContainer () && view->asContainer ()->isChildView (touchInputView)))
		touchInputView = nullptr;

	if(touchInput)
		touchInput->viewRemoved (view);

	if(FocusFinder::isFocusOrChild (view, savedFocusView))
		savedFocusView = nullptr;

	if(FocusFinder::isFocusOrChild (view, focusView))
		setFocusView (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::receiveTouchInput (const TouchEvent& e)
{
	if(touchInput)
	{
		touchInput->onTouchInput (e);
		return true;
	}
	return onTouchInput (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
	{
		// try to find new touch view
		struct WantsTouchFilter: ViewFilter
		{
			bool matches (const View* view) const override { return view->wantsTouch () && view->isEnabled (); }
		} touchFilter;

		View* newTouchView = findView (e.where, true, &touchFilter);
		if(newTouchView && newTouchView->wantsFocus () && newTouchView->isEnabled ())
				setFocusView (newTouchView);
		
		touchInputView = newTouchView;
	}

	if(touchInputView)
	{
		Point offset;
		touchInputView->clientToRoot (offset);
		
		TouchEvent e2 (e);
		e2.where.offset (-offset.x, -offset.y);
		touchInputView->onTouchInput (e2);
		return true;
	}
	else
	{
		if(ContainerView::onTouchInput (e) == false)
			killModalView ();
	}

	if(e.type == TouchEvent::kUp)
		touchInputView = nullptr;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::onGestureInput (const GestureEvent& e)
{
	if(e.getType () == kGestureSingleTap && e.getState () == kGestureBegin)
	{
		TouchEvent touchEvent (TouchEvent::kDown, e.where);
		onTouchInput (touchEvent);

		touchEvent.type = TouchEvent::kUp;
		return onTouchInput (touchEvent);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::getHandledGestures (GestureVector& gestures, PointRef where)
{
	gestures.add (kGestureSingleTap|kGesturePriorityNormal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::onWheelInput (const WheelEvent& e)
{
	if(focusView)
		focusView->onWheelInput (e);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootView::onKeyInput (const VirtualKeyEvent& e)
{
	// focus view has priority
	if(focusView)
		if(focusView->onKeyInput (e))
			return true;

	// focus navigation
	if(e.type == VirtualKeyEvent::kNext || e.type == VirtualKeyEvent::kPrev)
	{
		View* startView = focusView;
		if(!startView)
			startView = this;

		bool forward = e.type == VirtualKeyEvent::kNext;
		View* newFocusView = forward ? FocusFinder::getNext (startView) : FocusFinder::getPrevious (startView);
		if(newFocusView)
			setFocusView (newFocusView);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootView::onIdle ()
{
	ContainerView::onIdle ();

	if(modalResetPending)
	{
		modalResetPending = false;
		setModalView (nullptr);
	}

	if(touchInput)
		touchInput->onIdle ();
}
