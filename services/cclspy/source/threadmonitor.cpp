//************************************************************************************************
//
// CCL Spy
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
// Filename    : threadmonitor.cpp
// Description : Thread Monitor
//
//************************************************************************************************

#include "threadmonitor.h"

#include "ccl/app/params.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/message.h"

#include "ccl/public/system/imediathreading.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/systemservices.h"

namespace CCL {

static const int kMaxThreads = 64;

//************************************************************************************************
// ThreadItem
//************************************************************************************************

struct ThreadItem: Threading::ThreadInfo
{
	ThreadItem& operator = (const Threading::ThreadInfo& ti)
	{
		static_cast<Threading::ThreadInfo&> (*this) = ti;
		return *this;
	}

	bool operator == (const Threading::ThreadInfo& ti)
	{
		return id == ti.id && activity == ti.activity && priority == ti.priority && CString (name) == ti.name;
	}

	bool operator != (const Threading::ThreadInfo& ti)
	{
		return !(*this == ti);
	}
};

//************************************************************************************************
// ThreadSorter
//************************************************************************************************

struct ThreadSorter
{
	enum Property
	{
		kName,
		kPriority,
		kActivity,
		kNumProperties
	};

	static StringRef getPropertyName (Property which)
	{
		static const String propertyNames[] =
		{
			String ("Name"),
			String ("Priority"),
			String ("Activity")
		};

		ASSERT (which >= 0 && which < kNumProperties)
		return propertyNames[which];
	}

	typedef int (*SortFunction) (const void* lhs, const void* rhs);

	#define SORTFUNCTION(name, til, tir) \
	static int name (const void* lhs, const void* rhs) \
	{ \
		Threading::ThreadInfo* til = (Threading::ThreadInfo*)lhs; \
		Threading::ThreadInfo* tir = (Threading::ThreadInfo*)rhs; \

	SORTFUNCTION (comparePriority, til, tir)
		return til->priority - tir->priority;
	}

	SORTFUNCTION (compareActivity, til, tir)
		return (int)(til->activity * 1000.f) - (int)(tir->activity * 1000.f);
	}

	SORTFUNCTION (compareName, til, tir)
		return CString (til->name).compare (tir->name);
	}

	static void sortByProperty (Threading::ThreadInfo infos[], int count, Property which)
	{
		SortFunction function = nullptr;
		switch(which)
		{
		case kName : function = compareName; break;
		case kPriority : function = comparePriority; break;
		case kActivity : function = compareActivity; break;
		}

		if(function)
			::qsort (infos, count, sizeof(Threading::ThreadInfo), function);
	}
};

//************************************************************************************************
// ThreadItemModel
//************************************************************************************************

class ThreadItemModel: public ItemModel
{
public:
	DECLARE_CLASS_ABSTRACT (ThreadItemModel, ItemModel)

	ThreadItemModel ();

	PROPERTY_POINTER (ThreadMonitor, monitor, Monitor)
	PROPERTY_VARIABLE (ThreadSorter::Property, sortProperty, SortProperty)

	enum Columns { kThreadID, kIcon, kName, kPriority, kNativePriority, kActivity, kValueBar };

	float update (); ///< returns idle activity

	// ItemModel
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API viewDetached (IItemView* itemView) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;

protected:
	ThreadItem threads[kMaxThreads];
	int activeThreadCount;

	ThreadItem* resolve (ItemIndexRef index);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Thread Priorities
//////////////////////////////////////////////////////////////////////////////////////////////////

static StringRef getPriorityName (Threading::ThreadPriority priority)
{
	static const String descriptions[] =
	{
		String ("Low"),				// kPriorityLow
		String ("Below Normal"),	// kPriorityBelowNormal
		String ("Normal"),			// kPriorityNormal
		String ("Above Normal"),	// kPriorityAboveNormal
		String ("High"),			// kPriorityHigh
		String ("Time Critical"),	// kPriorityTimeCritical
		String ("Realtime Base"),	// kPriorityRealtime = kPriorityRealtimeBase
		String ("Realtime Middle"),	// kPriorityRealtimeMiddle
		String ("Realtime Top")		// kPriorityRealtimeTop
	};

	ASSERT (priority >= 0 && priority < ARRAY_COUNT (descriptions))
	return descriptions[priority];
}

static Color getPriorityColor (Threading::ThreadPriority priority)
{
	switch(priority)
	{
	case Threading::kPriorityLow : return Colors::kWhite;
	case Threading::kPriorityBelowNormal : return Colors::kLtGray;
	case Threading::kPriorityNormal : return Colors::kGray;
	case Threading::kPriorityHigh : return Colors::kGreen;
	case Threading::kPriorityTimeCritical : return Colors::kYellow;
	case Threading::kPriorityRealtimeBase : 
	case Threading::kPriorityRealtimeMiddle : return Colors::kBlue;
	case Threading::kPriorityRealtimeTop : return Colors::kRed;
	default :
		return Colors::kBlack;
	}
}

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ThreadMonitorTags
	{
		kSortBy = 'Sort',
		kSetPriority = 'Prio'
	};
}

//************************************************************************************************
// ThreadMonitor
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ThreadMonitor, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadMonitor::ThreadMonitor ()
: Component (CCLSTR ("ThreadMonitor")),
  threadModel (nullptr),
  idleValue (nullptr)
{
	threadModel = NEW ThreadItemModel;
	threadModel->setMonitor (this);

	idleValue = paramList.addFloat (0, 100, "idle");
	
	ListParam* sortList = NEW ListParam ("sortBy");
	paramList.add (sortList, Tag::kSortBy);
	for(int i = 0; i < ThreadSorter::kNumProperties; i++)
		sortList->appendString (ThreadSorter::getPropertyName ((ThreadSorter::Property)i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadMonitor::~ThreadMonitor ()
{
	threadModel->setMonitor (nullptr); // model could be released later!
	threadModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ThreadMonitor::getObject (StringID name, UIDRef classID)
{
	if(name == "Threads")
		return ccl_as_unknown (threadModel);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadMonitor::onIdleTimer ()
{
	idleValue->setNormalized (threadModel->update ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThreadMonitor::paramChanged (IParameter* param)
{
	if(param->getTag () == Tag::kSortBy)
		threadModel->setSortProperty ((ThreadSorter::Property)param->getValue ().asInt ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadMonitor::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kExtendMenu)
	{
		UnknownPtr<IParameter> param (subject);
		if(param && param->getTag () == Tag::kSetPriority)
		{
			UnknownPtr<IMenu> menu (msg[0]);
			ASSERT (menu != nullptr)
			if(menu)
			{
				int numItems = menu->countItems ();
				for(int i = 0; i < numItems; i++)
					if(IMenuItem* item = menu->getItem (i))
					{
						AutoPtr<IImage> icon = GraphicsFactory::createSolidShapeImage (getPriorityColor (i), IMenuItem::kIconSize, IMenuItem::kIconSize);
						item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
					}
			}
			return;
		}
	}

	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// ThreadItemModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ThreadItemModel, ItemModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadItemModel::ThreadItemModel ()
: monitor (nullptr),
  sortProperty (ThreadSorter::kName),
  activeThreadCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ThreadItemModel::update ()
{
	IItemView* itemView = getItemView ();
	if(itemView == nullptr)
		return 1.f;
	
	ViewBox vb (itemView);
	if(!vb.isAttached ())
		return 1.f;

	int count = kMaxThreads;
	Threading::ThreadInfo infos[kMaxThreads];
	System::GetMediaThreadService ().getThreadsSnapshot (infos, count);
	
	ThreadSorter::sortByProperty (infos, count, sortProperty);

	float sum = 0;
	for(int i = 0; i < count; i++)
	{
		float activity = infos[i].activity;
		activity *= 10000.f;
		int intActivity = (int)activity;
		activity = (float)intActivity / 10000.f;
		infos[i].activity = activity;
		sum += activity;

		if(threads[i] != infos[i])
		{
			threads[i] = infos[i];
			itemView->invalidateItem (ItemIndex (i));
		}
	}

	float idleActivity = ccl_bound<float> (1.f - sum, 0, 1);

	if(count != activeThreadCount)
	{
		activeThreadCount = count;
		signal (Message (kChanged));
	}

	vb.redraw ();
	return idleActivity;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadItemModel::viewAttached (IItemView* itemView)
{
	SuperClass::viewAttached (itemView);
	if(monitor)
		monitor->startTimer (500);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ThreadItemModel::viewDetached (IItemView* itemView)
{
	if(monitor)
		monitor->stopTimer ();
	SuperClass::viewDetached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThreadItemModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (50, "ID");
	list.addColumn (18);
	list.addColumn (120, String ("Name"), "name", 50, IColumnHeaderList::kSizable);
	list.addColumn (80, String ("Priority"));
	list.addColumn (50, String ("Native Prio"));
	list.addColumn (50, String ("Activity"));
	list.addColumn (50); // kValueBar
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ThreadItemModel::countFlatItems ()
{
	return activeThreadCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThreadItem* ThreadItemModel::resolve (ItemIndexRef index)
{
	int i = index.getIndex ();
	bool validIndex = i >= 0 && i < activeThreadCount;
	ASSERT (validIndex)
	return validIndex ? &threads[i] : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThreadItemModel::getItemTitle (String& title, ItemIndexRef index)
{
	ThreadItem* item = resolve (index);
	if(item == nullptr)
		return false;

	title = String (item->name);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThreadItemModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	ThreadItem* item = resolve (index);
	if(item == nullptr)
		return false;

	String text;
	Alignment alignment = Alignment::kLeftCenter;
	
	static const String kActivityFormat ("%float(1:2)");

	switch(column)
	{
	case kThreadID :
		text << item->id;
		break;

	case kIcon :
		{
			Rect iconRect (info.rect);
			iconRect.contract (2);
			info.graphics.fillRect (iconRect, SolidBrush (getPriorityColor (item->priority)));
		}
		break;

	case kName :
		text = String (item->name);
		break;

	case kPriority :
		text = getPriorityName (item->priority);
		break;

	case kNativePriority :
		text << item->nativePriority;
		break;

	case kActivity :
		text.appendFormat (kActivityFormat, item->activity * 100.f);
		alignment = Alignment::kRightCenter;
		break;

	case kValueBar :
		drawHorizontalBar (info.graphics, info.rect, item->activity, Colors::kGray, Colors::kBlue, 6);
		break;
	}

	if(!text.isEmpty ())
		info.graphics.drawString (info.rect, text, info.style.font, info.style.textBrush, alignment);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ThreadItemModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	ThreadItem* item = resolve (index);
	if(item == nullptr)
		return false;

	if(column == kPriority)
	{
		AutoPtr<MenuParam> prioList = NEW MenuParam;
		prioList->connect (monitor, Tag::kSetPriority);

		for(Threading::ThreadPriority priority = Threading::kPriorityLow; priority <= Threading::kPriorityTimeCritical; priority++)
			prioList->appendString (getPriorityName (priority));

		prioList->setValue (item->priority);

		if(doPopup (prioList, info))
		{
			Threading::ThreadPriority priority = prioList->getValue ().asInt ();

			AutoPtr<Threading::IThread> thread = System::CreateThreadWithIdentifier (item->id);
			ASSERT (thread != nullptr)
			if(thread)
			{
				if(thread->getPriority () <= Threading::kPriorityTimeCritical) // realtime top can not be set here!
					thread->setPriority (priority);
			}
		}
		return true;
	}

	return false;
}
