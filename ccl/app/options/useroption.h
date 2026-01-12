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
// Filename    : ccl/app/options/useroption.h
// Description : User Option
//
//************************************************************************************************

#ifndef _ccl_useroption_h
#define _ccl_useroption_h

#include "ccl/app/component.h"
#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/iuseroption.h"

namespace CCL {

class LocalString;
class UserOptionList;
class UserOptionElement;

//************************************************************************************************
// UserOption
//************************************************************************************************

class UserOption: public Component,
				  public IUserOption
{
public:
	DECLARE_CLASS (UserOption, Component)

	UserOption (StringRef name = nullptr, StringRef title = nullptr);
	~UserOption ();

	static StringRef Options ();
	static StringRef General ();
	static StringRef Locations ();
	static StringRef Advanced ();

	template <class OptionClass> static OptionClass* init ();

	UserOptionList* getOptionList ();

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

	void setCategory (StringRef category);
	UserOptionElement* addElement (UserOptionElement* element);

	// IUserOption
	StringRef CCL_API getName () const override;
	StringRef CCL_API getTitle () const override;
	IImage* CCL_API getIcon () const override;
	tbool CCL_API needsApply () const override;
	tbool CCL_API apply () override;
	void CCL_API opened () override;
	void CCL_API closed () override;

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

	CLASS_INTERFACE (IUserOption, Component)

protected:
	bool applyPending;
	ObjectArray elements;

	static StringRef getOptionString (const LocalString& string);
};

//************************************************************************************************
// UserOptionList
//************************************************************************************************

class UserOptionList: public Component,
					  public IUserOptionList
{
public:
	DECLARE_CLASS (UserOptionList, Component)

	UserOptionList (StringRef name = nullptr, StringRef title = nullptr);
	~UserOptionList ();

	void addOption (UserOption* option);

	typedef Vector<IUserOption*> PlugInOptionList;
	PlugInOptionList& getPlugInList ();

	IUserOption* findOptionByName (StringRef name) const;

	void runDialog (IUserOption* selected = nullptr);

	// IUserOptionList
	StringRef CCL_API getName () const override;
	StringRef CCL_API getTitle () const override;
	int CCL_API countOptions () const override;
	IUserOption* CCL_API getOption (int index) const override;
	StringRef CCL_API getLastSelected () const override;
	void CCL_API setLastSelected (StringRef name) override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IUserOptionList, Component)

private:
	String lastSelected;
	PlugInOptionList plugInList;
};

//************************************************************************************************
// UserOptionManager
//************************************************************************************************

class UserOptionManager: public Object,
						 public Singleton<UserOptionManager>
{
public:
	UserOptionManager ();

	StringRef getTitle () const;

	void store ();
	void restore ();

	void add (UserOption* option);
	void removeAll ();

	void addPlugIns ();
	void removePlugIns ();

	void addList (UserOptionList* list);
	void removeList (UserOptionList* list);

	IUserOption* findOptionByName (StringRef name, UserOptionList** list = nullptr) const;

	void runDialog (UserOptionList* selectedList = nullptr,
					IUserOption* selectedOption = nullptr);

protected:
	UserOptionList optionList;
	ObjectArray optionLists;
};

//************************************************************************************************
// ConfigurationPublisher
/** Publish configuration values as parameter */
//************************************************************************************************

struct ConfigurationPublisher
{
	typedef void (*ApplyCallback) ();

	static bool addParam (StringID section, StringID key, IParameter* param, ApplyCallback applyCallback = nullptr);
	static bool addBoolParam (StringID section, StringID key, ApplyCallback applyCallback = nullptr);
	static bool addToggleCommand (StringID section, StringID key, StringID commandCategory, StringID commandName); ///< addBoolParam is required first

	static IController* getSharedInstance ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// UserOption inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class OptionClass> OptionClass* UserOption::init ()
{
	OptionClass* option = NEW OptionClass;
	UserOptionManager::instance ().add (option);
	return option;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_useroption_h
