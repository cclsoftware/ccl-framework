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
// Filename    : core/portable/gui/corestaticview.cpp
// Description : Static view class
//
//************************************************************************************************

#include "core/portable/gui/corestaticview.h"

#include "core/portable/corecontrollershared.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// StaticThemePainter
//************************************************************************************************

DEFINE_STATIC_SINGLETON (StaticThemePainter)

//************************************************************************************************
// StaticView
//************************************************************************************************

StaticView::StaticView (RectRef size)
: TViewBase<StaticView> (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticContainerView* StaticView::asContainer ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticRootView* StaticView::getRootView () const
{
	return parent ? parent->getRootView () : nullptr;
}

//************************************************************************************************
// StaticContainerView
//************************************************************************************************

StaticContainerView::StaticContainerView (RectRef size)
: ContainerViewBase<StaticView> (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticContainerView* StaticContainerView::asContainer ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticView* StaticContainerView::findView (const StaticViewFilter& filter, bool deep) const
{
	VectorForEachFast (children, StaticView*, v)
		if(filter.matches (v))
			return v;
		if(deep == true)
			if(StaticContainerView* vc = v->asContainer ())
			{
				StaticView* result = vc->findView (filter, true);
				if(result)
					return result;
			}
	EndFor
	return nullptr;
}

//************************************************************************************************
// StaticRootView
//************************************************************************************************

StaticRootView::StaticRootView (RectRef size, BitmapPixelFormat pixelFormat, RenderMode renderMode)
: StaticContainerView (size),
  RootViewBase (size, pixelFormat, renderMode),
  rootController (nullptr),
  focusView (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticRootView::~StaticRootView ()
{
	ASSERT (rootController == nullptr)
	setController (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticRootView* StaticRootView::getRootView () const
{
	return const_cast<StaticRootView*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::initOffscreen (Bitmap* offscreen1, Bitmap* offscreen2)
{
	ASSERT (offscreen1 && offscreen1->getFormat () == pixelFormat && offscreen1->getSize () == targetSize)
	ASSERT ((offscreen2 && renderMode == kFlipMode) || (!offscreen2 && renderMode != kFlipMode))

	offscreenList.removeAll ();
	if(offscreen1)
		offscreenList.add (offscreen1);
	if(offscreen2)
		offscreenList.add (offscreen2);

	activeBufferIndex = 0;
	lastDirtyRegion.setEmpty ();
	dirtyRegion.setEmpty ();

	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::setController (RootValueController* controller)
{
	if(rootController != controller)
	{
		if(rootController)
			rootController->removeObserver (this);
		rootController = controller;
		if(rootController)
			rootController->addObserver (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::valueChanged (ValueController* controller, int paramTag)
{
	updateView (this, controller, paramTag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::updateView (StaticView* view, ValueController* controller, int paramTag)
{
	if(StaticControlBase* c = core_cast<StaticControlBase> (view))
	{
		if(c->getController () == controller && c->getParamTag () == paramTag)
			c->valueChanged ();
	}

	// Note that some controls are containers, too.
	if(StaticContainerView* cv = view->asContainer ())
	{
		VectorForEachFast (cv->getChildren (), StaticView*, child)
			updateView (child, controller, paramTag);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::invalidate (RectRef rect)
{
	addDirtyRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::draw (const DrawEvent& e)
{
	// clear background
	e.graphics.fillRect (e.updateRect, getStyle ().getBackColor ());
	
	StaticContainerView::draw (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StaticView* StaticRootView::getFocusView () const
{
	return focusView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::setFocusView (StaticView* view)
{
	if(view != focusView)
	{
		if(focusView)
			focusView->onFocus (false);
		focusView = view;
		if(focusView)
			focusView->onFocus (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticRootView::findNextFocusView (StaticContainerView* container, bool forward, TypeID viewType /* = 0 */)
{
	const ConstVector<StaticView*>& children = container->getChildren ();

	if(children.isEmpty ())
		return;

	int count = children.count ();
	int index = children.index (focusView);

	if(index == -1)
		return;

	for(;;)
	{
		if(forward)
		{
			if(++index >= count)
				index = 0; // Wrap around
		}
		else
		{
			if(--index < 0)
				index = count - 1; // Wrap around
		}

		if(children[index] == focusView)
			break; // We're at the same view again. Perhaps the only child view?

		if(viewType != 0 && children[index]->castTo (viewType) == nullptr)
			continue; // Not the type of view we are looking for

		if(children[index]->wantsFocus ())
		{
			setFocusView (children[index]);
			break;
		}
	}
}

//************************************************************************************************
// StaticViewConnector
//************************************************************************************************

StaticViewConnector::StaticViewConnector (ValueController* initialController)
: initialController (initialController)
{
	ASSERT (initialController)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticViewConnector::connect (StaticView* view, bool state)
{
	struct ResolvedPath: ParamPath64
	{
		ValueController* resolvedController;

		ResolvedPath (ValueController* controller, CStringPtr path)
		: ParamPath64 (path)
		{
			if(!childName.isEmpty ())
				resolvedController = TControllerFinder<ValueController>::lookupInplace (controller, childName.getBuffer ());
			else
				resolvedController = controller;
		}
	};
	
	// Check for mix-in class supported by container/non-container views
	if(StaticControlBase* cb = core_cast<StaticControlBase> (view))
	{
		if(state)
		{
			ValueController* controller = nullptr;
			int paramTag = 0;

			ResolvedPath path (initialController, view->getName ());
			if(path.resolvedController && path.resolvedController->getTagByName (paramTag, path.paramName))
				controller = path.resolvedController;

			cb->setController (controller);
			cb->setParamTag (paramTag);
		}
		else
		{
			cb->setController (nullptr);
			cb->setParamTag (0);
		}
	}

	// Check for special derived view classes
	if(core_cast<StaticControl> (view) != nullptr)
	{
		return; // fast exit for controls already handled above
	}
	else if(StaticVariantView* vv = core_cast<StaticVariantView> (view))
	{
		if(state) // select initial variant
			vv->valueChanged ();

		// connect all variants, not just the visible one
		VectorForEachFast (vv->getVariants (), StaticView*, child)
			connect (child, state);
		EndFor

	}		
	else if(StaticContainerView* cv = view->asContainer ())
	{
		VectorForEachFast (cv->getChildren (), StaticView*, child)
			connect (child, state);
		EndFor
	}
	else if(StaticCustomView* cv = core_cast<StaticCustomView> (view))
	{
		if(state)
		{
			IStaticViewPainter* painter = nullptr;

			ResolvedPath path (initialController, view->getName ());
			if(path.resolvedController)
				painter = core_cast<IStaticViewPainter> (path.resolvedController->getObject (path.paramName));

			cv->setPainter (painter);
		}
		else
			cv->setPainter (nullptr);
	}
	else if(StaticListView* lv = core_cast<StaticListView> (view))
	{
		if(state)
		{
			StaticListViewModel* model = nullptr;

			ResolvedPath path (initialController, view->getName ());
			if(path.resolvedController)
				model = core_cast<StaticListViewModel> (path.resolvedController->getObject (path.paramName));

			lv->setModel (model);
		}
		else
			lv->setModel (nullptr);
	}
}

//************************************************************************************************
// StaticLabel
//************************************************************************************************

StaticLabel::StaticLabel (RectRef size)
: StaticView (size),
  title (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticLabel::draw (const DrawEvent& e)
{
	const Style& style = getStyle ();		

	if(isColorize ())
		e.graphics.fillRect (e.updateRect, style.getBackColor ());

	Rect r;
	getClientRect (r);

	if(!ConstString (title).isEmpty ())
		e.graphics.drawString (r, title, style.getTextColor (), style.getFontName (), style.getTextAlign ());

	if(isFocused ())
		StaticThemePainter::instance ().drawFocusFrame (e.graphics, r);
}

//************************************************************************************************
// StaticImageView
//************************************************************************************************

StaticImageView::StaticImageView (RectRef size)
: StaticContainerView (size),
  image (nullptr),
  imageAlpha (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticImageView::draw (const DrawEvent& e)
{
	if(image)
	{
		BitmapMode mode;
		if(imageAlpha < 1.f)
		{
			mode.paintMode = BitmapMode::kBlend;
			mode.alphaF = imageAlpha;
		}

		e.graphics.drawBitmap (Point (e.updateRect.left, e.updateRect.top), *image, e.updateRect, &mode);
	}
	else
	{
		if(isColorize ())
			e.graphics.fillRect (e.updateRect, getStyle ().getBackColor ());
	}

	StaticContainerView::draw (e);
}

//************************************************************************************************
// StaticControlBase
//************************************************************************************************

StaticControlBase::StaticControlBase ()
: controller (nullptr),
  paramTag (0)
{}

//************************************************************************************************
// StaticVariantView
//************************************************************************************************

StaticVariantView::StaticVariantView (RectRef size)
: StaticContainerView (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const StaticVariantView::ViewChildren& StaticVariantView::getVariants () const
{
	return variants;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticVariantView::getCurrentVariant () const
{
	return controller ? controller->getIntValue (paramTag) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticVariantView::addView (StaticView* view)
{
	variants.add (view);

	// init current variant
	int index = getCurrentVariant ();
	if(children.isEmpty () && variants.count ()-1 == index)
		selectVariant (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticVariantView::valueChanged ()
{
	int index = getCurrentVariant ();
	selectVariant (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticVariantView::selectVariant (int index)
{
	StaticView* oldView = children.first ();
	StaticView* newView = variants.at (index);
	if(oldView != newView)
	{
		if(oldView)
			removeView (oldView);

		if(newView)
			StaticContainerView::addView (newView); // call superclass!
	}
}

//************************************************************************************************
// StaticControl
//************************************************************************************************

StaticControl::StaticControl (RectRef size)
: StaticView (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticControl::valueChanged ()
{
	invalidate ();
}

//************************************************************************************************
// StaticTextBox
//************************************************************************************************

StaticTextBox::StaticTextBox (RectRef size)
: StaticControl (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticTextBox::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	drawText (e.graphics, r);

	if(isFocused ())
		StaticThemePainter::instance ().drawFocusFrame (e.graphics, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticTextBox::getText (TextValue& text) const
{
	if(controller)
	{
		StringResult result (text.getBuffer (), text.getSize ());
		controller->toString (result, paramTag);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticTextBox::drawText (Graphics& graphics, RectRef textRect)
{
	TextValue string;
	getText (string);

	if(!string.isEmpty ())
	{
		const Style& style = getStyle ();
		graphics.drawString (textRect, string, style.getTextColor (), style.getFontName (), style.getTextAlign ());
	}
}

//************************************************************************************************
// StaticButton
//************************************************************************************************

StaticButton::StaticButton (RectRef size)
: StaticControl (size),
  title (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticButton::draw (const DrawEvent& e)
{
	const Style& style = getStyle ();		

	Rect r;
	getClientRect (r);

	if(!ConstString (title).isEmpty ())
		e.graphics.drawString (r, title, style.getTextColor (), style.getFontName (), style.getTextAlign ());

	if(isFocused ())
		StaticThemePainter::instance ().drawFocusFrame (e.graphics, r);
}

//************************************************************************************************
// StaticValueBar
//************************************************************************************************

StaticValueBar::StaticValueBar (RectRef size)
: StaticControl (size),
  background (nullptr),
  image (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float StaticValueBar::getNormalizedValue () const
{
	if(controller)
		return controller->getNormalized (paramTag);
	else
		return 0.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticValueBar::draw (const DrawEvent& e)
{
	StaticThemePainter::instance ().drawBackground (e.graphics, e.updateRect, getStyle (), background);

	Rect r;
	getClientRect (r);
	float value = getNormalizedValue ();		
	StaticThemePainter::instance ().drawValueBar (e.graphics, r, options, value, getStyle (), image);

	if(isFocused ())
		StaticThemePainter::instance ().drawFocusFrame (e.graphics, r);
}

//************************************************************************************************
// StaticCustomView
//************************************************************************************************

StaticCustomView::StaticCustomView (RectRef size)
: StaticView (size),
  painter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticCustomView::draw (const DrawEvent& e)
{
	if(painter)
		painter->drawView (*this, e);
}

//************************************************************************************************
// StaticListViewModel
//************************************************************************************************

StaticListViewModel::StaticListViewModel ()
: view (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListViewModel::changed ()
{
	if(view)
		view->modelChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListViewModel::invalidate ()
{
	if(view)
		view->invalidate ();
}

//************************************************************************************************
// StaticListView
//************************************************************************************************

StaticListView::StaticListView (RectRef size)
: StaticView (size),
  painter (*this),
  model (nullptr)
{
	painter.setClientWidth (size.getWidth ());
	painter.setClientHeight (size.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::setModel (StaticListViewModel* _model)
{
	if(model != _model)
	{
		if(model)
		{
			ASSERT (model->getView () == this)
			model->setView (nullptr);
		}
		model = _model;
		painter.setBaseModel (_model);
		if(model)
		{
			ASSERT (!model->getView ())
			model->setView (this);
		}
		modelChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::modelChanged ()
{
	painter.resetScrollPosition ();
	painter.resetSelectedItem ();
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::setSize (RectRef newSize)
{
	StaticView::setSize (newSize);
	painter.setClientWidth (newSize.getWidth ());
	painter.setClientHeight (newSize.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::selectItem (int index)
{
	if(painter.selectItem (index))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::makeSelectedItemVisible ()
{
	if(painter.makeSelectedItemVisible ())
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::makeItemVisible (int index)
{
	if(painter.makeItemVisible (index))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::scrollBy (int delta)
{
	if(painter.scrollBy (delta))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::scrollTo (int index)
{
	if(painter.scrollTo (index))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticListView::getScrollPosition () const
{
	return painter.getScrollPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticListView::draw (const DrawEvent& e)
{
	painter.drawList (e, getStyle ());
}
