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
// Filename    : ccl/gui/controls/segmentbox.h
// Description : Segment Box is used for parameters that are edited in segments like time and date.
//
//************************************************************************************************

#ifndef _ccl_segmentbox_h
#define _ccl_segmentbox_h

#include "ccl/gui/controls/control.h"
#include "ccl/gui/theme/visualstyleclass.h"

#include "ccl/gui/system/systemevent.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

interface IParamSplitter;

//************************************************************************************************
// SegmentBox styles
//************************************************************************************************

namespace Styles
{
	enum SegmentBoxStyles
	{
		kSegmentBoxBehaviorStatic			= 1<<1,	///< control is only for display and cannot be edited
		kSegmentBoxAppearanceSeparateDigits	= 1<<2,	///< draw each digit separately (default: one string per segment)
		kSegmentBoxAppearanceScaleText		= 1<<3,	///< uses a dynamic font size to fill the available space
		kSegmentBoxBehaviorNoContextMenu	= 1<<4	///< suppress context menu
	};
};

//************************************************************************************************
// SegmentBox
/** Displays a numeric value in distinct segments that can be incremented / decremented separately */
//************************************************************************************************

class SegmentBox: public Control,
			      public ICommandHandler
{
public:
	DECLARE_CLASS (SegmentBox, Control)

	SegmentBox (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	~SegmentBox ();

	DECLARE_STYLEDEF (customStyles)

	int findRect (const Point& where);
	int getCharWidth ();
	int getDelimiterWidth ();
	void insertCharacter (short character);
	void deleteCharacter (bool back);
	bool advance (bool back);
	void incrementPart (int part, int amount);
	void shiftUp ();
	int getActivePart ();
	void toggleSign ();
	void getDrawRect (Rect& rect);

	// Control
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	const IVisualStyle& CCL_API getVisualStyle () const override;
	void draw (const UpdateRgn& updateRgn) override;
	void updateClient () override;
	bool onFocus (const FocusEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;

	PROPERTY_VARIABLE (bool, dragging, Dragging)

	CLASS_INTERFACE (ICommandHandler, Control)
protected:
	static const int kMaxNumParts = 5;
	enum PartCode
	{
		kPartNone = -1,
		kPartSign = 0,
		kPartSegment0 = 10,
	};
	struct PartValues
	{
		int values[kMaxNumParts];
		int sign = 1;
	};

	bool inserting;
	short activePart;

	int segmentValues[kMaxNumParts];
	int sizes[kMaxNumParts];
	int delimiter[kMaxNumParts];
	int charWidth;
	int delimiterWidth;
	int leftMargin;

	int oldParts[kMaxNumParts];
	int oldSign;
	int oldId;
	int oldVisualState;
	Rect padding;
	Rect cachedRect;
	float cachedFontSize;
	
	Bitmap* cachedBitmap;
	float contentScaleFactor;

	Bitmap* getCachedBitmap (const Point& size, bool& contentLost);
	void adjustTextColor (SolidBrush& textBrush, Pen& textPen,const IVisualStyle& vs);

	bool onEditCopy (const CommandMsg&);
	bool checkState (IParamSplitter* splitter);
	void updateSizeInfo (IParamSplitter* splitter);
	bool scaleTextFont (IParamSplitter* splitter);
	void drawBackground (IGraphics& graphics, RectRef r);
	void drawOneSegment (IGraphics& port, RectRef size, StringRef segmentStr, FontRef font, SolidBrushRef textBrush);
	void drawSegments (IGraphics& graphics, RectRef r);
	void updateSegments (IGraphics& graphics, RectRef r);
	void getPartValues (IParamSplitter& splitter, PartValues& values); 
	void setPartValues (IParamSplitter& splitter, const PartValues& values); 

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
};

DECLARE_VISUALSTYLE_CLASS (SegmentBox)

} // namespace CCL

#endif // _ccl_segmentbox_h
