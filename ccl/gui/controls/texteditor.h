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
// Filename    : ccl/gui/controls/texteditor.h
// Description : Text Editor Control
//
//************************************************************************************************

#ifndef _ccl_texteditor_h
#define _ccl_texteditor_h

#include "ccl/gui/controls/editbox.h"

namespace CCL {
class TextEditorEditBox;
class ScrollView;

//************************************************************************************************
// TextEditor
/** A text editor is a scrollable control for displaying and editing multiline text. */
//************************************************************************************************

class TextEditor: public Control
{
public:
	DECLARE_CLASS (TextEditor, Control)

	TextEditor (const Rect& size = Rect (), IParameter* param = nullptr,
			 StyleRef style = 0, StringRef title = nullptr);
	~TextEditor ();

	void setVScrollBarStyle (VisualStyle* visualStyle);
	void setHScrollBarStyle (VisualStyle* visualStyle);

	// Control
	void attached (View* parent) override;
	void paramChanged () override;
	void onSize (const Point& delta) override;
	void onVisualStyleChanged () override;

protected:
	friend class TextEditorEditBox;
	TextEditorEditBox* editBox;
	ScrollView* scrollView;

	void updateScrollTargetSize ();
	void updateScrollPosition (RectRef caretRect);

	void makeFrameworkView ();
	
	SharedPtr<VisualStyle> hBarStyle;
	SharedPtr<VisualStyle> vBarStyle;
};

} // namespace CCL

#endif // _ccl_texteditor_h
