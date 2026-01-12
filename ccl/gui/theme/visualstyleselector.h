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
// Filename    : ccl/gui/theme/visualstyleselector.h
// Description : VisualStyleSelector class
//
//************************************************************************************************

#ifndef _ccl_visualstyleselector_h
#define _ccl_visualstyleselector_h

#include "ccl/gui/theme/visualstyle.h"

#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"

namespace CCL {

interface IParameter;
class VisualStyleAlias;

//************************************************************************************************
// VisualStyleSelector
/** Manages a visual style that delegates to one of the target styles.
	The value of the parameter is interpreted as an index in the list of available styles.
*/
//************************************************************************************************

class VisualStyleSelector: public Object
{
public:
	DECLARE_CLASS (VisualStyleSelector, Object)

	VisualStyleSelector (VisualStyleAlias* styleAlias = nullptr);
	~VisualStyleSelector ();

	void setParameter (IParameter* p);
	void setSelectorProperty (CStringRef propertyId, IUnknown* controller);
	void addStyle (VisualStyle* style);
	void initialize ();
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	VisualStyleAlias* styleAlias;
	IParameter* param;
	ObjectArray styles;
	SharedPtr<IUnknown> controller;
	MutableCString propertyId;
	bool changingStyle;

	void updateSelectedStyle ();
	bool isPropertyMode () const;
	void selectStyle (int index);
};

//************************************************************************************************
// VisualStyleAlias
/** The delegating style used by VisualStyleSelector. */
//************************************************************************************************

class VisualStyleAlias: public VisualStyle
{
public:
	DECLARE_CLASS_ABSTRACT (VisualStyleAlias, VisualStyle)

	VisualStyleAlias (StringID name = nullptr);

	static StringID kStyleChanged;
	void signalStyleChanged ();

	void setStyleSelector (VisualStyleSelector* selector);

	// VisualStyle
	const IVisualStyle* CCL_API getOriginal () const override;
	void use (IVisualStyleClient* client) override;
	void unuse (IVisualStyleClient* client) override;
	void CCL_API addObserver (IObserver* observer) override;

private:
	LinkedList<IVisualStyleClient*> clients;
	SharedPtr<VisualStyleSelector> styleSelector;
	bool wasObserved;
};

} // namespace CCL

#endif // _ccl_visualstyleselector_h
