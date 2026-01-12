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
// Filename    : ccl/app/paramcontainer.h
// Description : Parameter Container
//
//************************************************************************************************

#ifndef _ccl_paramcontainer_h
#define _ccl_paramcontainer_h

#include "ccl/base/object.h"

#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/icontroller.h"

namespace CCL {

class Attributes;

//************************************************************************************************
// ParamContainer
/** ParamList extension derived from Object. */
//************************************************************************************************

class ParamContainer: public Object,
					  public ParamList,
					  public AbstractController
{
public:
	DECLARE_CLASS (ParamContainer, Object)
	DECLARE_METHOD_NAMES (ParamContainer)

	using ParamList::at;
	using ParamList::byTag;

	template <class T> T* at (int idx) const;
	template <class T> T* byTag (int tag) const;

	/** Store values to attributes. */
	void storeValues (Attributes& a, bool storable = false) const;
	void storeValue (Attributes& a, StringID name) const;
	void storeValue (Attributes& a, IParameter* p) const;
	
	/** Restore values from attributes. */
	void restoreValues (const Attributes& a, bool storable = false, bool update = false);
	bool restoreValue (const Attributes& a, StringID name, bool update = false);
	bool restoreValue (const Attributes& a, IParameter* p, bool update = false);

	/** Clone parameters from other container. */
	void addParametersFrom (const ParamContainer& container);

	/** Set all parameters to their default values. */
	void setDefaultValues (bool storable = false, bool update = false);

	/** Enable/Disable all parameters. */
	void enableAll (bool state);

	/** Store parameter values to user settings. */
	void storeSettings (StringRef settingsID) const;

	/** Store parameter values to user settings. Keeps other existing values in settings (e.g. for currently non-storable params). */
	void storeSettingsIncrementally (StringRef settingsID) const;

	/** Restore parameter values from user settings. */
	void restoreSettings (StringRef settingsID, bool update = true);

	// IController
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index) const override;
	IParameter* CCL_API findParameter (StringID name) const override;
	IParameter* CCL_API getParameterByTag (int tag) const override;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IController, Object)

protected:
	// ParamList
	IParameter* newParameter (UIDRef cid) const override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ParamContainer
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> T* ParamContainer::at (int idx) const
{ return unknown_cast<T> (at (idx)); }

template <class T> T* ParamContainer::byTag (int tag) const
{ return unknown_cast<T> (byTag (tag)); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_paramcontainer_h
