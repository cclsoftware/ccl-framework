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
// Filename    : ccl/app/components/stringlistcomponent.h
// Description : Manages display and editing of a list of strings
//
//************************************************************************************************

#ifndef _ccl_stringlistcomponent_h
#define _ccl_stringlistcomponent_h

#include "ccl/app/component.h"

#include "ccl/base/collections/stringlist.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/commanddispatch.h"

namespace Core {
struct Color; }

namespace CCL {

class ListParam;
class StringList;
struct KeyEvent;

//************************************************************************************************
// TextCompletionComponent
/** Offers completions (via IAutoComplete) while the user types in an edit box. */
//************************************************************************************************

class TextCompletionComponent: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (TextCompletionComponent, Component)

	TextCompletionComponent (StringRef name = nullptr);

	String getEditString () const;
	void setEditString (StringRef string, bool suspendAutoComplete = true);

	bool hasEditFocus () const;
	void focusEdit (bool cursorToEnd = false);
	void closeCompletionList ();
	void appendCharacter (uchar c);

	PROPERTY_SHARED_AUTO (IAutoComplete, autoComplete, AutoComplete)

protected:
	bool autoCompleteSuspended;

	virtual void onTextEdited (StringRef editString);
	virtual void onCompletionSelected (StringRef string);
	virtual void onClear ();

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;

private:
	ListParam* completionList;

	class CompletionsParam;
};

//************************************************************************************************
// StringListComponent
/** Displays a list of strings. For each String in the list a view is ceated (itemFormName).
	The user can type in an edit box at the end of the list to add a string. While typing, a popup
	with completions is displayed. When a suggestion from the list is selected, it is added to the list,
	and the user can start typing the next term. */
//************************************************************************************************

class StringListComponent: public TextCompletionComponent,
						   public CommandDispatcher<StringListComponent>,
						   public IEditControlHost
{
public:
	DECLARE_CLASS_ABSTRACT (StringListComponent, TextCompletionComponent)
	DECLARE_METHOD_NAMES (StringListComponent)

	StringListComponent (StringRef name = nullptr);
	~StringListComponent ();

	PROPERTY_POINTER (IDataTarget, dataTarget, DataTarget)
	PROPERTY_SHARED_AUTO (IParameter, concatTarget, ConcatTarget)

	PROPERTY_MUTABLE_CSTRING (itemFormName, ItemFormName)
	PROPERTY_MUTABLE_CSTRING (editFormName, EditFormName)
	PROPERTY_BOOL (editable, Editable)

	void setList (const StringList& strings);
	const StringList& getList () const { return stringList; }

	void addString (StringRef string);

	PROPERTY_STRING (focusString, FocusString)

	CLASS_INTERFACE (IEditControlHost, TextCompletionComponent)

private:
	StringList stringList;

	class StringItemModel;
	StringItemModel* itemModel;

	void updateConcatenation ();
	void onListEdited (const StringList& newList);

	virtual bool onOpenItem (StringRef string);
	virtual IUnknown* createDragSessionData (StringRef string);
	virtual void getSkinVariables (Attributes& variables, StringRef item);

	// TextCompletionComponent
	void onTextEdited (StringRef editString) override;
	void onCompletionSelected (StringRef string) override;
	void onClear () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

protected:
	virtual void onListEditComplete () {}
	
	// IEditControlHost
	tbool CCL_API onEditNavigation (const KeyEvent& event, IView* control) override;
	void CCL_API onEditControlLostFocus (IView* control) override {}

	// Commands
	DECLARE_COMMANDS (StringListComponent)
	DECLARE_COMMAND_CATEGORY ("Edit", Component)
	bool onRemoveItem (CmdArgs args);
};

} // namespace CCL

#endif // _ccl_stringlistcomponent_h
