//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/theme/renderer/atomrenderer.cpp
// Description : Atom Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/atomrenderer.h"

#include "ccl/gui/views/graphicsport.h"

#include "ccl/gui/controls/control.h"

using namespace CCL;

//************************************************************************************************
// IRenderAtomClient
//************************************************************************************************

DEFINE_IID_ (IRenderAtomClient, 0x36672f, 0x42e4, 0x478c, 0xbe, 0x51, 0xdf, 0x5a, 0x7c, 0x30, 0xbf, 0xa7)

//************************************************************************************************
// AtomRenderer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (AtomRenderer, CompositedRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

AtomRenderer::AtomRenderer (VisualStyle* visualStyle, View* owner)
: CompositedRenderer (visualStyle, owner),
  styleConditionsDirty (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::draw (const UpdateRgn& updateRgn)
{
	updateAtoms ();

	GraphicsPort port (owner);
	for(RenderAtom* atom : atoms)
		atom->draw (port, updateRgn);

	owner->View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AtomRenderer::hitTest (PointRef where, Point* clickOffset)
{
	for(int i = 0; i < atoms.count (); i++)
	{
		if(getAtomAt (i)->getSize ().pointInside (pointIntToF (where)))
		{
			if(clickOffset)
				(*clickOffset) = where - pointFToInt (getAtomAt (i)->getSize ().getLeftTop ());

			return i;
		}
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AtomRenderer::getPartRect (Rect& rect, int partCode)
{
	RenderAtom* atom = findAtom (partCode);
	if(atom == nullptr)
		return false;

	rect = rectFToInt (atom->getSize ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::invalidateStyleConditions ()
{
	styleConditionsDirty = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::invalidate (const RenderAtom& atom)
{
	if(owner)
		owner->invalidate (rectFToEnclosingInt (atom.getSize ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::addAtom (RenderAtom& atom, int atIndex)
{
	ASSERT (atom.getTag () == -1 || findAtom (atom.getTag ()) == nullptr)
	ASSERT (atoms.count () < atoms.getCapacity ())
	ASSERT (atom.getClient () == nullptr)
	
	atom.setClient (this);
	if(atIndex > -1 && atIndex < atoms.count ())
		atoms.insertAt (atIndex, &atom);
	else
		atoms.add (&atom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AtomRenderer::removeAtom (int atomTag)
{
	int index = 0;
	for(RenderAtom* atom : atoms)
	{
		if(atom->getTag () == atomTag)
		{
			atom->setClient (nullptr);
			atoms.removeAt (index);
			return true;
		}

		index++;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom* AtomRenderer::findAtom (int atomTag)
{
	for(RenderAtom* atom : atoms)
	{
		if(atom->getTag () == atomTag)
			return atom;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RenderAtom* AtomRenderer::getAtomAt (int index)
{
	ASSERT (atoms.isValidIndex (index))
	return atoms.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/*static*/ void AtomRenderer::getImageSize (RectF& imageSize, const Image* image, RectFRef contentSize, float fillFactor)
{
	imageSize.setEmpty ();

	if(image == nullptr)
		return;

	imageSize.setWidth (image->getWidth ());
	imageSize.setHeight (image->getHeight ());

	// A filled image has no size of its own, it is derived from the content area.
	// Scaled to fill that area in the smaller dimension, then taken times the factor.
	// Without a content area to relate to - the image keeps its own size instead.
	if(fillFactor > 0.f && !contentSize.isEmpty ())
	{
		float scaleFactor = fillFactor * ccl_min (contentSize.getWidth () / imageSize.getWidth (), contentSize.getHeight () / imageSize.getHeight ());

		imageSize.setWidth (imageSize.getWidth () * scaleFactor);
		imageSize.setHeight (imageSize.getHeight () * scaleFactor);
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::applyLegacyContrastColors (RenderAtom& atom, const LegacyStyleID::ContrastColorIDs& ids)
{
	RenderAtom::ColorContrastHelper& helper = atom.getColorContrastHelper ();

	Color color;
	if(visualStyle->getColor (color, ids.bright))
		helper.setContrastColorBright (color);

	if(visualStyle->getColor (color, ids.alphaBlend) || visualStyle->getColor (color, LegacyStyleID::kColorAlphaBlend))
		helper.setContrastColorAlphaBlend (color);

	// the dark and the transparent contrast color default to the default color of the atom
	if(visualStyle->getColor (color, ids.dark))
		helper.setContrastColorDark (color);
	else if(!atom.getStyleColor (color, *visualStyle, AtomPropertyID::kContrastColorDark, &conditionContext))
		helper.setContrastColorDark (helper.getDefaultColor ());

	bool hasTransparentColor = visualStyle->getColor (color, ids.transparent) || (ids.transparentFallback && visualStyle->getColor (color, ids.transparentFallback));
	if(hasTransparentColor)
		helper.setContrastColorTransparent (color);
	else if(!atom.getStyleColor (color, *visualStyle, AtomPropertyID::kContrastColorTransparent, &conditionContext))
		helper.setContrastColorTransparent (helper.getDefaultColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif // LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::updateAtoms ()
{
	// gets the necessary colors and other style properties from the visual style using the style condition context.
	if(styleConditionsDirty)
	{
		VisualStyleTransaction transaction (this);
		applyVisualStyle (transaction);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(visualStyle)
	{
		for(RenderAtom* atom : atoms)
			atom->updateFromVisualStyle (transaction, *visualStyle, conditionContext);
	}

	styleConditionsDirty = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AtomRenderer::finalizeVisualStyleChanges (const RenderAtomTransaction& transaction)
{
	for(RenderAtom* atom : atoms)
		atom->finalizeVisualStyleUpdate (transaction);
}

//************************************************************************************************
// ControlRenderer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (ControlRenderer, AtomRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ControlRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	if(Control* control = ccl_cast<Control> (owner))
		control->getStyleConditions (conditionContext);

	SuperClass::applyVisualStyle (transaction);
}
