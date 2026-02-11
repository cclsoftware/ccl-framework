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
// Filename    : ccl/gui/controls/editbox.h
// Description : Edit Box
//
//************************************************************************************************

#ifndef _ccl_editbox_h
#define _ccl_editbox_h

#include "ccl/gui/controls/textbox.h"

#include "ccl/gui/views/sprite.h"
#include "ccl/gui/graphics/mutableregion.h"
#include "ccl/gui/system/autofill.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/ieditbox.h"

namespace CCL {

class NativeTextControl;

//************************************************************************************************
// EditBox
/** An edit box is a control for editing text based values and strings. 
An edit box displays text just like a TextBox, but the parameter value can also be changed by the user by editing text. */
//************************************************************************************************

class EditBox: public TextBox,
			   public IdleClient,
			   public IEditBox,
			   public IAutofillClient
{
public:
	DECLARE_CLASS (EditBox, TextBox)

	EditBox (const Rect& size = Rect (), IParameter* param = nullptr,
			 StyleRef style = 0, StringRef title = nullptr);
	~EditBox ();

	static bool isAnyEdtiting ();

	enum EditBoxParts
	{
		kPartNativeControlArea = 10
	};

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	PROPERTY_VARIABLE (int, keyboardType, KeyboardType)
	PROPERTY_VARIABLE (int, autofillType, AutofillType)
	PROPERTY_STRING (placeholder, PlaceholderString)
	
	static const Configuration::BoolValue useNativeTextControl;
	bool shouldUseNativeControl () const;
	void closeNativeTextControl ();

	void getCaretRect (Rect& rect) const;
	const ConstVector<Rect>& getSelection () const;
	bool moveCaretTo (const Point& position, bool clearSelection = false);

	// IEditBox
	void CCL_API insertText (StringRef text) override;
	void CCL_API removeText (int length) override;
	void CCL_API setCaret (int textPosition) override;

	// IAutofillClient
	int getAutofillClientType () const override;
	View* getAutofillClientView () override;
	void receiveAutofillText (StringRef text) override;

	// IdleClient
	void onIdleTimer () override;

	// TextBox
	void attached (View* parent) override;
	void removed (View* parent) override;
	ThemeRenderer* getRenderer () override;
	void setStyle (StyleRef style) override;
	void onSize (const Point& delta) override;
	void onMove (const Point& delta) override;
	bool onFocus (const FocusEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	IDragHandler* createDragHandler (const DragEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;
	bool onGesture (const GestureEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onActivate (bool state) override;
	bool canHandleDoubleTap () const override;
	void onVisualStyleChanged () override;
	bool isEditing () const override;
	AccessibilityProvider* getAccessibilityProvider () override;

	CLASS_INTERFACE3 (IEditBox, ITimerTask, IAutofillClient, TextBox)

protected:
	class TextDragHandler;
	friend class TextDragHandler;

	class TransactionScope;
	friend class TransactionScope;

	class UndoHandler;
	friend class UndoHandler;
	UndoHandler* undoHandler;

	class EditBoxMouseHandler;
	friend class EditBoxMouseHandler;

	static const int kCaretShift = 5;

	NativeTextControl* nativeControl;
	bool wantReopen; // reopen when regaining focus
	bool forceOpen;
	static bool inKeyDown;
	static int editCount;

	AutoPtr<ISprite> caret;
	int caretTextPosition; // caret position in UTF16 text
	int selectionTextPosition; // start of selection in UTF16 text
	Rect caretRect; // caret bounds relative to the upper left corner of this control
	Point lastExplicitCaretPosition;
	bool canceled;
	bool textLayoutInitialized;
	bool selectionLocked;

	int clickCount;
	double latestClickTime;

	SelectionRegion selection;

	IDragHandler* currentDragHandler;
	ITextLayout::Range dragSelectionRange;

	enum PrivateFlags
	{
		kInCreateNativeControl = 1 << (kLastPrivateFlag + 1)
	};

	PROPERTY_FLAG (privateFlags, kInCreateNativeControl, inCreateNativeControl)

	bool handleClick (const MouseEvent& event);
	bool handleKeyDown (const KeyEvent& keyEvent);
	bool tryEdit (const KeyEvent& keyEvent);
	void takeEditFocusInternal (bool state, bool directed = true);
	void insertTextInternal (StringRef text, int actionID);
	int removeTextInternal (int textPosition, int length, int actionID);
	bool acceptClick (const MouseEvent& event);
	void paramFocused (bool state);
	void submit (bool check = false);
	void setNativeControl (NativeTextControl* toSet);
	NativeTextControl* createNativeControl ();
	virtual void calculateNativeControlSize (Rect& size);
	IEditControlHost* getEditControlHost ();
	void setCaretPosition (int textPosition, bool implicit = false);
	void setSelectionPosition (int textPosition);
	virtual void updateCaretRect ();
	void updateSelection ();
	String getSelectedText (bool plainText) const;
	void dragSelection ();
	void selectWordOrLine (bool selectLine);
	void showCaret (bool state);

	bool isTextSelected () const { return caretTextPosition != selectionTextPosition; }

	void selectAll ();
	void deleteSelection ();
	int deleteSelectionInternal (int actionID);
	void cutSelection ();
	void copySelection ();
	void paste ();

	void resetTimer ();
	int getEffectiveKeyboardType () const;

	// TextBox
	void paramChanged () override;
	void buildTextLayout () override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
};

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

class NativeTextControl: public Object
{
public:
	static NativeTextControl* create (Control& owner, RectRef clientRect, int returnKeyType, int keyboardType); ///< platform-specific!

	DECLARE_STYLEDEF (returnKeyTypes)
	DECLARE_STYLEDEF (keyboardTypes)
	
	~NativeTextControl ();

	Control& getOwner () const;
	IParameter* getTextParameter () const;
	const IVisualStyle& getVisualStyle () const;

	PROPERTY_BOOL (immediateUpdate, ImmediateUpdate)
	PROPERTY_VARIABLE (int, returnKeyType, ReturnKeyType)
	PROPERTY_VARIABLE (int, keyboardType, KeyboardType)

	static bool isNativeTextControlPresent () { return textControlCount > 0; }

	bool isCanceled ();										///< true if Escape was pressed
	virtual void updateText ();								///< IParameter changed, update text control
	virtual void submitText ();								///< text control changed, update IParameter
	virtual void getControlText (String& string);			///< get current text from control
	virtual void setSelection (int start, int length);		///< select length characters from start. length = -1: until end; start = -1: select none
	virtual void setScrollPosition (PointRef where);		///< scroll to given position
	virtual Point getScrollPosition () const;				///< get current scroll position
	virtual bool handleKeyDown (const KeyEvent& keyEvent);	///< handle keyboard input
	virtual void setSize (RectRef clientRect) = 0;
	virtual void updateVisualStyle () = 0;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	static int textControlCount;

	Control& owner;
	bool canceled;
	bool isSubmitted;
	bool inSubmitText;

	NativeTextControl (Control& owner, int returnKeyType, int keyboardType);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool EditBox::shouldUseNativeControl () const
{ return useNativeTextControl && !getStyle ().isCustomStyle (Styles::kEditBoxBehaviorExtended); }

inline void EditBox::closeNativeTextControl ()
{ setNativeControl (nullptr); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// NativeTextControl Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	DEFINE_STRINGID (kNativeTextControl, "CCL.NativeTextControl")
		DEFINE_STRINGID (kNativeTextControlCreated, "NativeTextControlCreated")
		DEFINE_STRINGID (kNativeTextControlDestroyed, "NativeTextControlDestroyed")
}

} // namespace CCL

#endif // _ccl_editbox_h
