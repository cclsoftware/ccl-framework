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
// Filename    : ccl/app/browser/pluginmanagement.h
// Description : Plug-in Management Component
//
//************************************************************************************************

#ifndef _ccl_pluginmanagement_h
#define _ccl_pluginmanagement_h

#include "ccl/app/component.h"

#include "ccl/base/signalsource.h"
#include "ccl/base/storage/file.h"

#include "ccl/public/system/idiagnosticdataprovider.h"

#include "ccl/public/gui/framework/inotificationcenter.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class ListViewItem;
class ListViewModel;
class PriorityListModel;
class PlugInListViewModel;
interface IUserOption;
interface IClassDescription;
interface IColumnHeaderList;
interface IContextMenu;

//************************************************************************************************
// IPlugInVersionProvider
//************************************************************************************************

interface IPlugInVersionProvider: IUnknown
{
	virtual tresult CCL_API getVersionString (String& version, const IClassDescription& description) const = 0;

	virtual tresult CCL_API getLastModifiedTime (FileTime& lastModified, const IClassDescription& description) const = 0;
	
	DECLARE_IID (IPlugInVersionProvider)
};

//************************************************************************************************
// IPlugInManagementExtension
//************************************************************************************************

interface IPlugInManagementExtension: IUnknown
{
	virtual void addPlugInListColumns (IColumnHeaderList& columns) const = 0;

	virtual void setPlugInListColumnData (ListViewItem& item, const IClassDescription& description) const = 0;

	virtual bool editPlugInListColumn (ListViewItem& item, const IClassDescription& description,
									   StringID columnID, const IItemModel::EditInfo& editInfo) const = 0;
	
	virtual void appendPlugInListItemMenu (IContextMenu& menu, const IClassDescription& description,
										   IUnknownList& selected) const = 0;

	DECLARE_IID (IPlugInManagementExtension)
};

//************************************************************************************************
// PlugInManagementComponent
//************************************************************************************************

class PlugInManagementComponent: public Component,
								 public IDiagnosticDataProvider,
								 public INotificationActionHandler,
								 public ComponentSingleton<PlugInManagementComponent>
{
public:
	DECLARE_CLASS (PlugInManagementComponent, Component)

	PlugInManagementComponent ();
	~PlugInManagementComponent ();

	void addCategory (StringRef category);
	void addType (StringRef category, int priority = 0, bool fixed = false);
	void addLinkedOption (IUserOption* userOption);
	void addVersionProvider (IPlugInVersionProvider* provider);
	void addManagementExtension (IPlugInManagementExtension* extension);

	void onViewVisible (bool state);
	void onListSelectionChanged (ListViewModel* listModel);
	void updateBlockList ();
	void updateResultList ();
	
	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;
	
	// INotificationActionHandler
	tbool CCL_API canExecute (StringID actionId, const INotification& n) const override;
	tresult CCL_API execute (StringID actionId, INotification& n) override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	
	CLASS_INTERFACE2 (IDiagnosticDataProvider, INotificationActionHandler, Component)

protected:
	struct PlugInType
	{
		String name;
		int priority;
		bool found;
		bool fixed;

		PlugInType (StringRef name = nullptr, int priority = 0, bool found = false, bool fixed = false)
		: name (name),
		  priority (priority),
		  found (found),
		  fixed (fixed)
		{}

		bool operator == (const PlugInType& other) { return name == other.name; }
		bool operator > (const PlugInType& other) { return priority > other.priority; }
	};

	Vector<PlugInType> types;
	Vector<String> categories;
	SignalSink pluginSignalSink;
	ListViewModel* typeList;
	PriorityListModel* typePriorityList;
	ListViewModel* vendorList;
	PlugInListViewModel* pluginList;
	PlugInListViewModel* diagnosticList;
	ListViewModel* blockList;
	AutoPtr<SearchDescription> searchDescription;
	Vector<IUserOption*> linkedOptions;
	Vector<IPlugInVersionProvider*> versionProviders;
	Vector<IPlugInManagementExtension*> managementExtensions;
	friend class PlugInListViewModel;

	void updateEnabledStates ();
	void updateFilters ();
	void resetFilters (bool deselectAll = false);
	void storeFilters (Attributes& a) const;
	void restoreFilters (const Attributes& a);
	void storePrioritySettings (Attributes& a) const;
	void restorePrioritySettings (const Attributes& a);
	bool matchesType (StringRef type) const;
	bool matchesVendor (StringRef vendor) const;
	bool matchesName (StringRef name) const;
	void setInformationMissing (bool state = true);

	void updatePresentationDetails ();
	void applyVisibility ();
	void updatePriorities ();
	template<typename T> void hideDuplicates (T& list, bool onlyUnused = false);
	void autoHideDuplicates ();

	bool getPlugInVersion (String& version, const IClassDescription& description) const;
};

} // namespace CCL

#endif // _ccl_pluginmanagement_h
