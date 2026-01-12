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
// Filename    : ccl/app/components/stringlistcomponent.cpp
// Description : Manages display and editing of a list of strings
//
//************************************************************************************************

#include "ccl/app/components/stringlistcomponent.h"

#include "ccl/app/params.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/message.h"
#include "ccl/base/boxedtypes.h"

#include "ccl/public/gui/framework/iparametermenu.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum TextCompletionTags
	{
		kEditString = 100,
		kCompletions,
		kClear
	};

	enum StringListTags
	{
		kRemoveItem = 200
	};
};

//************************************************************************************************
// TextCompletionComponent::CompletionsParam
//************************************************************************************************

class TextCompletionComponent::CompletionsParam: public CCL::ListParam,
												 public CCL::IParameterMenuCustomize
{
public:
	CompletionsParam (StringID name, TextCompletionComponent* component)
	: ListParam (name), component (component)
	{}

	CLASS_INTERFACE (IParameterMenuCustomize, ListParam)

protected:
	TextCompletionComponent* component;

	// IParameterMenuCustomize
	StringID CCL_API getMenuType () const override { return MenuPresentation::kTree; }
	tbool CCL_API buildMenu (CCL::IMenu& menu, CCL::IParameterMenuBuilder& builder) override { return false; }

	tbool CCL_API onMenuKeyDown (const KeyEvent& event) override
	{
		if(event.vKey == VKey::kBackspace)
		{
			String string (component->getEditString ());
			int len = string.length ();
			if(len > 0)
				component->setEditString (string.truncate (len - 1), false);
			return true;
		}
		else if(event.isCharValid () && Unicode::isPrintable (event.character))
		{
			// forward printable text input from list popup to text edit
			component->appendCharacter (event.character);
			return true;
		}
		return false;
	}
};

//************************************************************************************************
// TextCompletionComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TextCompletionComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextCompletionComponent::TextCompletionComponent (StringRef name)
: Component (name),
  autoCompleteSuspended (false)
{
	paramList.addString (CSTR ("editString"), Tag::kEditString);
	paramList.addParam (CSTR ("clear"), Tag::kClear);

	completionList = NEW CompletionsParam (CSTR ("completions"), this);
	paramList.add (completionList, Tag::kCompletions);
	completionList->setSignalAlways (); // even when initial value is selected
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::setEditString (StringRef string, bool suspendAutoComplete)
{
	if(suspendAutoComplete)
	{
		ScopedVar<bool> guard (autoCompleteSuspended, true);
		paramList.byTag (Tag::kEditString)->setValue (string, true);
	}
	else
		paramList.byTag (Tag::kEditString)->setValue (string, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextCompletionComponent::getEditString () const
{
	return paramList.byTag (Tag::kEditString)->getValue ().asString ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextCompletionComponent::hasEditFocus () const
{
	IWindow* activeWindow = System::GetDesktop ().getDialogParentWindow ();
	UnknownPtr<IControl> control (activeWindow ? activeWindow->getFocusIView () : nullptr);
	return control && control->getParameter () == paramList.byTag (Tag::kEditString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::focusEdit (bool cursorToEnd)
{
	UnknownPtr<ISubject> subject (paramList.byTag (Tag::kEditString));
	if(subject)
	{
		subject->signal (Message (IParameter::kRequestFocus));
	
		int endPos = getEditString ().length ();
		if(endPos > 0 && cursorToEnd)
			subject->signal (Message (IParameter::kSetSelection, endPos, endPos));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::closeCompletionList ()
{
	UnknownPtr<ISubject> listSubject (ccl_as_unknown (completionList));
	if(listSubject)
		listSubject->signal (Message (IParameter::kReleaseFocus));

	System::GetSignalHandler ().flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::appendCharacter (uchar c)
{
	IParameter* editParam = paramList.byTag (Tag::kEditString);
	uchar character[2] = { c, 0 };

	String string (editParam->getValue ().asString ());
	string << character;
	editParam->setValue (string, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::onTextEdited (StringRef inputString)
{
	if(autoComplete && !autoCompleteSuspended)
	{
		completionList->removeAll ();
		StringList completions;
		if(!inputString.isEmpty () && autoComplete->suggestCompletions (completions, inputString))
		{
			completions.forEach ([this] (StringRef string)
			{
				completionList->appendString (string);
			});

			int inputLength = inputString.length ();

			// find longest match
			int foundIndex = -1;
			int foundLength = -1;
			int numCompletions = completionList->getMax ().asInt () + 1;
			for(int i = 0; i < numCompletions; i++)
			{
				String completion = completionList->getValueAt (i);
				int completionLength = completion.length ();
					
				// determine number of matching characters
				int matchLength = 0;
				for(int c = 0; c < inputLength && c < completionLength && Unicode::toLowercase (inputString[c]) == Unicode::toLowercase (completion[c]); c++)
					matchLength++;

				if(matchLength > foundLength)
				{
					foundIndex = i;
					foundLength = matchLength;
				}			
			}

			if(foundIndex >= 0)
			{
				completionList->setValue (foundIndex);

				// open popup
				UnknownPtr<ISubject> listSubject (completionList->asUnknown ());
				if(listSubject)
				{
					listSubject->signal (Message (IParameter::kUpdateMenu));
					listSubject->signal (Message (IParameter::kRequestFocus));
				}
			}
			else
			{
				closeCompletionList ();
				if(!hasEditFocus ())
					focusEdit (true);
			}
		}
	}

	if(inputString.isEmpty ())
	{
		closeCompletionList ();
		if(!autoCompleteSuspended)
			focusEdit ();
	}
	
	propertyChanged ("showPlaceholderLabel");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::onClear ()
{
	setEditString (nullptr);

	propertyChanged ("showPlaceholderLabel");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextCompletionComponent::onCompletionSelected (StringRef string)
{
	// default behavior: replace edit string with completion
	setEditString (string);
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextCompletionComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "showPlaceholderLabel")
	{
		var = getEditString () == String::kEmpty;
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextCompletionComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kEditString :
		onTextEdited (param->getValue ().asString ());
		return true;

	case Tag::kCompletions :
		onCompletionSelected (completionList->getSelectedValue ().asString ());
		focusEdit ();
		return true;

	case Tag::kClear:
		onClear ();
		focusEdit ();
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextCompletionComponent::notify (ISubject* subject, MessageRef msg)
{
	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// StringListComponent::StringItemModel
//************************************************************************************************

class StringListComponent::StringItemModel: public CCL::Component,
											public CCL::AbstractItemModel
{
public:
	StringItemModel (StringListComponent& component);

	// IItemModel
	tbool CCL_API getSubItems (CCL::IUnknownList& items, CCL::ItemIndexRef index) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (IItemModel, Component)

protected:
	StringListComponent& stringListComponent;
};

//************************************************************************************************
// StringListComponent::StringItemModel
//************************************************************************************************

StringListComponent::StringItemModel::StringItemModel (StringListComponent& stringListComponent)
: stringListComponent (stringListComponent)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::StringItemModel::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	ForEach (stringListComponent.stringList, Boxed::String, string)
		items.add (string->asUnknown (), true);
	EndFor

	if(stringListComponent.isEditable ())
		items.add (this->asUnknown (), true); // "text edit"
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API StringListComponent::StringItemModel::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "StringListItem")
	{
		String* string = unknown_cast<Boxed::String> (data);
		if(string || data.asUnknown () == this->asUnknown ())
		{
			Attributes variables;
			variables.set ("component", &stringListComponent);
			if(string)
			{
				variables.set ("itemTitle", *string);
				stringListComponent.getSkinVariables (variables, *string);
			}

			StringID formName = string ? stringListComponent.getItemFormName () : stringListComponent.getEditFormName ();

			ITheme* theme = getTheme ();
			IView* view = theme ? theme->createView (formName, this->asUnknown (), &variables) : nullptr;
			if(UnknownPtr<IForm> form = view)
				form->setController (data); // dropbox uses controller to identify items
			return view;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::StringItemModel::onItemFocused (ItemIndexRef index) 
{
	stringListComponent.setFocusString (stringListComponent.stringList.at (index.getIndex ()));
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API StringListComponent::StringItemModel::createDragSessionData (ItemIndexRef index) 
{
	return stringListComponent.createDragSessionData (stringListComponent.stringList.at (index.getIndex ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::StringItemModel::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	IDataTarget* dataTarget = stringListComponent.getDataTarget ();
	return dataTarget ? dataTarget->canInsertData (data, session, targetView, index.getIndex ()) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::StringItemModel::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	IDataTarget* dataTarget = stringListComponent.getDataTarget ();
	return dataTarget ? dataTarget->insertData (data, session, index.getIndex ()) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::StringItemModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	String string (stringListComponent.stringList.at (index.getIndex ()));
	return !string.isEmpty () && stringListComponent.onOpenItem (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (StringListComponent)
	DEFINE_COMMAND ("Edit", "Delete", StringListComponent::onRemoveItem)
END_COMMANDS (StringListComponent)

IMPLEMENT_COMMANDS (StringListComponent, TextCompletionComponent)

//************************************************************************************************
// StringListComponent
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (StringListComponent, TextCompletionComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringListComponent::StringListComponent (StringRef name)
: TextCompletionComponent (name),
  dataTarget (nullptr),
  itemFormName ("CCL/StringListItem"),
  editFormName ("CCL/StringListEditBox"),
  editable (true)
{
	itemModel = NEW StringItemModel (*this);
	addObject ("StringList", itemModel);

	paramList.addCommand ("Edit", "Delete", "removeItem", Tag::kRemoveItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringListComponent::~StringListComponent ()
{
	itemModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::setList (const StringList& strings)
{
	ScopedVar<bool> guard (autoCompleteSuspended, true);

	stringList = strings;
	setEditString (nullptr); // reset edit

	itemModel->signal (Message (kChanged));
	signalPropertyChanged ("hasContent");

	updateConcatenation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::addString (StringRef string)
{
	StringList newList (stringList);
	newList.addOnce (string);
	onListEdited (newList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* StringListComponent::createDragSessionData (StringRef string)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::getSkinVariables (Attributes& variables, StringRef item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::updateConcatenation ()
{
	if(concatTarget)
	{
		String total (stringList.concat (" "));
		String editString (getEditString ());
		if(!editString.isEmpty ())
		{
			if(!total.isEmpty ())
				total << " ";
			total << editString;
		}
		concatTarget->fromString (total, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::onTextEdited (StringRef editString)
{
	updateConcatenation ();
	SuperClass::onTextEdited (editString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::onCompletionSelected (StringRef completion)
{
	addString (completion);
	setEditString (nullptr); // reset edit

	updateConcatenation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::onClear ()
{
	onListEdited (StringList ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::onEditNavigation (const KeyEvent& event, IView* control)
{
	switch(event.vKey)
	{
		case VKey::kEnter:
		case VKey::kReturn:
		{
			// move edit string to list
			String editString (getEditString ());
			if(!editString.isEmpty ())
			{
				addString (editString);
				setEditString (nullptr);
				focusEdit ();
			}
		}
		return true;

		case VKey::kBackspace:
			if(getEditString ().isEmpty ())
			{
				// backspace in empty edit removes previous string
				StringList newList (stringList);
				if(newList.removeLast ())
				{
					onListEdited (newList);
					focusEdit ();
				}
				return true;
			}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringListComponent::onRemoveItem (CmdArgs args)
{
	if(!isEditable () || focusString.isEmpty ())
		return false;

	if(!args.checkOnly ())
	{
		StringList newList (stringList);
		newList.remove (focusString);
		onListEdited (newList);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StringListComponent::onListEdited (const StringList& newList)
{
	setEditString (nullptr);
	if(!(newList == stringList))
	{
		setList (newList);
		onListEditComplete ();
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StringListComponent::onOpenItem (StringRef string)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "canEdit")
	{
		var = isEditable ();
		return true;
	}
	else if(propertyId == "hasContent")
	{
		var = !stringList.isEmpty ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (StringListComponent)
	DEFINE_METHOD_NAME ("focusEdit")
END_METHOD_NAMES (StringListComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API StringListComponent::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "focusEdit")
	{
		focusEdit ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
