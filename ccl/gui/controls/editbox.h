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
#include "ccl/gui/system/inputmethod.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/ieditbox.h"

namespace CCL {

class NativeTextControl;
class EditBoxRenderer;

//************************************************************************************************
// EditBox
/** An edit box is a control for editing text based values and strings. 
An edit box displays text just like a TextBox, but the parameter value can also be changed by the user by editing text. */
//************************************************************************************************

class EditBox: public TextBox,
			   public IdleClient,
			   public IEditBox,
			   public IAutofillClient,
			   public ITextInputClient
{
public:
	DECLARE_CLASS (EditBox, TextBox)

	EditBox (const Rect& size = Rect (), IParameter* param = nullptr,
			 StyleRef style = 0, StringRef title = nullptr);
	~EditBox ();

	static bool isAnyEditing ();
	static bool isEditNavigationKey (const KeyEvent& event); ///< keys passed to IEditControlHost::onEditNavigation

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

	const ConstVector<Rect>& getSelection ();
	bool moveCaretTo (const Point& position, bool clearSelection = false);

	Coord getDisplayOffset () const { return isEditing () ? displayOffset : 0; }

	// IEditBox
	void CCL_API insertText (StringRef text) override;
	void CCL_API removeText (int length) override;
	void CCL_API setCaret (int textPosition) override;

	// IAutofillClient
	int getAutofillClientType () const override;
	View* getAutofillClientView () override;
	void receiveAutofillText (StringRef text) override;

	// ITextInputClient
	const IView* CCL_API getInputView () const override;
	int CCL_API getTextLength () const override;
	tresult CCL_API getText (String& result, int start = 0, int length = -1) const override;
	void CCL_API getSelection (int& caret, int& selection, SelectionDomain selectionDomain = kSelectionOriginal) const override;
	void CCL_API setSelection (int caret, int selection) override;
	tresult CCL_API getCaretRect (Rect& caretRect) const override;
	int CCL_API getInputType () const override;
	int CCL_API getInputFlags () const override;
	void CCL_API getPreeditText (String& text) const override;
	void CCL_API setPreeditText (StringRef text) override;
	void CCL_API applyText (StringRef text) override;
	void CCL_API deleteText (int beforeLength, int afterLength) override;
	void CCL_API replaceText (StringRef text, int start, int length) override;
	void CCL_API getTextBounds (IMutableRegion& bounds, int start, int length) override;

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
	void modifyDisplayString (String& displayString) const override;
	void modifyLayout (ITextLayout& textLayout) const override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IEditBox)
		QUERY_INTERFACE (ITimerTask)
		QUERY_INTERFACE (IAutofillClient)
		QUERY_INTERFACE (ITextInputClient)
	END_CLASS_INTERFACES (TextBox)

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

	friend class EditBoxRenderer;

	static const int kCaretShift = 5;

	NativeTextControl* nativeControl;
	bool wantReopen; // reopen when regaining focus
	bool forceOpen;
	bool selectionUpdateNeeded;
	static bool inKeyDown;
	static int editCount;

	Coord displayOffset;

	String plainTextCache; // read-only cache that mirrors textModel.toDisplayString ()

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

	AutoPtr<IInputMethod> inputMethod;
	String preeditText;
	int preeditCaretOffset;
	int preeditSelectionOffset;

	enum PrivateFlags
	{
		kInCreateNativeControl = 1 << (kTextBoxLastPrivateFlag + 1),
		kTextModified = 1 << (kTextBoxLastPrivateFlag + 2),	///< text model was edited since it was last synchronized with the parameter
		kInitialSelectionPending = 1 << (kTextBoxLastPrivateFlag + 3)
	};

	PROPERTY_FLAG (privateFlags, kInCreateNativeControl, inCreateNativeControl)
	PROPERTY_FLAG (privateFlags, kTextModified, textModified)
	PROPERTY_FLAG (privateFlags, kInitialSelectionPending, initialSelectionPending)

	static bool isWordModifier (const KeyEvent& event);

	bool handleClick (const MouseEvent& event);
	bool handleKeyDown (const KeyEvent& keyEvent);
	bool tryEdit (const KeyEvent& keyEvent);
	void takeEditFocusInternal (bool state, bool directed = true);
	void startEditing ();
	void insertTextInternal (StringRef text, int actionID);
	int removeTextInternal (int textPosition, int length, int actionID);
	bool acceptClick (const MouseEvent& event);
	void paramFocused (bool state);
	void submit (bool check = false, const KeyState& keys = KeyState ());
	void setNativeControl (NativeTextControl* toSet);
	NativeTextControl* createNativeControl ();
	virtual void calculateNativeControlSize (Rect& size);
	IEditControlHost* getEditControlHost ();
	void setCaretPosition (int textPosition, bool implicit = false);
	void setSelectionPosition (int textPosition);
	void updateDisplayOffset ();
	virtual void updateCaretRect ();
	void setCaretColor (ColorRef color);
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

	int getNextGraphemePosition (int textPosition) const;
	int getPreviousGraphemePosition (int textPosition) const;
	int getNextWordPosition (int textPosition) const;
	int getPreviousWordPosition (int textPosition) const;
	int snapToGraphemeBoundary (int textPosition) const;  // Snap position to nearest grapheme cluster boundary
	int snapToGraphemeBoundary (int textPosition, PointF position) const;  // Snap based on mouse position within character
	bool moveCaretToCoord (CoordF layoutY, bool shiftDown);

	// TextBox
	void paramChanged () override;
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
	virtual void getControlText (String& string) const;		///< get current text from control
	virtual int getControlTextLength () const;				///< get length of current text in control
	virtual void setSelection (int start, int length);		///< select length (may be negative) characters from start
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

	void getSelectionPositions (int& caret, int& anchor, int start, int length) const;
	void selectAll ();
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
