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
// Filename    : ccl/app/components/consolecomponent.cpp
// Description : Console Component
//
//************************************************************************************************

#include "ccl/app/components/consolecomponent.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/logfile.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// ConsoleListModel
//************************************************************************************************

class ConsoleListModel: public Object,
						public ItemViewObserver<AbstractItemModel>
{
public:
	ConsoleListModel (ConsoleComponent& component);

	void addEvent (LogEvent* logEvent); ///< event is shared!
	void removeAll ();

	const LogEventList& getEvents () const { return events; }

	// Icons
	PROPERTY_SHARED_AUTO (IImage, infoIcon, InfoIcon)
	PROPERTY_SHARED_AUTO (IImage, errorIcon, ErrorIcon)
	PROPERTY_SHARED_AUTO (IImage, warningIcon, WarningIcon)

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	void CCL_API viewAttached (IItemView* itemView) override;
	void CCL_API viewDetached (IItemView* itemView) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IItemModel, Object)

protected:
	enum Columns
	{
		kIconColumn,
		kTimeColumn,
		kTextColumn
	};

	ConsoleComponent& component;
	LogEventList events;
	bool firstDraw;
	IParameter* vScrollParam;

	typedef ItemViewObserver<AbstractItemModel> SuperClass;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ConsoleComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ConsoleComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ConsoleComponent::ConsoleComponent (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("Console") : name),
  scrollOnEvent (true),
  directUpdate (false),
  redirected (false),
  showLineNumbers (false),
  viewVisible (false)
{
	listModel = NEW ConsoleListModel (*this);

	paramList.addParam (CSTR ("removeAll"));
	paramList.addParam (CSTR ("export"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConsoleComponent::~ConsoleComponent ()
{
	setActive (false);

	cancelSignals ();

	listModel->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleComponent::setActive (bool state)
{
	if(state)
		redirected = true;
	else if(!redirected)
		return; // don't remove other redirection

	System::GetConsole ().redirect (state ? this : nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleComponent::setViewVisible (bool state)
{
	viewVisible = state;
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ConsoleComponent::isViewVisible () const
{
	return viewVisible;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleComponent::redirect (IConsole* console)
{
	CCL_NOT_IMPL ("ConsoleComponent can not be redirected!")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleComponent::writeLine (const char* text)
{
	return writeLine (String (text));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleComponent::writeLine (StringRef text)
{
	reportEvent (text);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleComponent::readLine (String& text)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleComponent::addEvent (LogEvent* logEvent, bool flushEvents)
{
	IWindow::UpdateCollector* uc = nullptr;
	SharedPtr<IWindow> window;
	IItemView* listView = listModel->getItemView ();
	if(listView)
		if((window = UnknownPtr<IView> (listView)->getIWindow ()))
			uc = NEW IWindow::UpdateCollector (window);

	listModel->addEvent (logEvent);

	bool mustScroll = listView && scrollOnEvent;
	bool mustFlush = flushEvents || mustScroll;

	if(mustFlush)
		System::GetSignalHandler ().flush (); // deliver kModelChanged message, which updates size

	listView = listModel->getItemView ();	// might have been removed during flush

	if(mustScroll && listView)
		listView->makeItemVisible (listModel->getEvents ().getEvents ().count () - 1);

	delete uc;

	if(mustFlush)
		System::GetGUI ().flushUpdates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleComponent::reportEvent (const Alert::Event& e)
{
	if(e.isLowLevel ()) // ignore events caused by CCL_WARN
		return;

	AutoPtr<LogEvent> logEvent = NEW LogEvent (e);
	if(logEvent->time == DateTime ())
		System::GetSystem ().getLocalTime (logEvent->time);

	if(directUpdate && System::IsInMainThread ())
		addEvent (logEvent, true);
	else
		(NEW Message ("addEvent", logEvent->asUnknown ()))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleComponent::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleComponent::scrollDown ()
{
	if(IItemView* listView = listModel->getItemView ())
		listView->makeItemVisible (listModel->getEvents ().getEvents ().count () - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleComponent::clear ()
{
	listModel->removeAll ();
	setScrollOnEvent (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "addEvent")
	{
		LogEvent* logEvent = unknown_cast<LogEvent> (msg[0]);
		addEvent (logEvent, false);
	}
	else if(msg == "scrollDown")
	{
		scrollDown ();
	}
	else if(msg == IParameter::kBeginEdit)
	{
		// scrollview gets manipulated by user
		setScrollOnEvent (false);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ConsoleComponent::getObject (StringID name, UIDRef classID)
{
	if(name == "eventList")
		return ccl_as_unknown (listModel);
	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleComponent::paramChanged (IParameter* param)
{
	if(param->getName () == "removeAll")
	{
		clear ();
	}
	else if(param->getName () == "export")
	{
		AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (CCL::ClassID::FileSelector);
		ASSERT (fs != nullptr)
		FileType fileType;
		listModel->getEvents ().getFormat (fileType);
		fs->addFilter (fileType);
		if(fs->run (IFileSelector::kSaveFile))
			listModel->getEvents ().saveToFile (*fs->getPath ());
	}
	return true;
}

//************************************************************************************************
// ConsoleListModel
//************************************************************************************************

ConsoleListModel::ConsoleListModel (ConsoleComponent& component)
: component (component),
  vScrollParam (nullptr),
  firstDraw (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleListModel::addEvent (LogEvent* logEvent)
{
	ASSERT (logEvent != nullptr)
	logEvent->retain ();
	events.getEvents ().add (logEvent);
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConsoleListModel::removeAll ()
{
	events.getEvents ().removeAll ();
	deferChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleListModel::viewAttached (IItemView* itemView)
{
	SuperClass::viewAttached (itemView);

	ASSERT (vScrollParam == nullptr)

	IScrollView* scrollView = GetViewInterfaceUpwards<IScrollView> (UnknownPtr<IView> (itemView));
	if((vScrollParam = scrollView->getVScrollParam ()))
	{
		ISubject::addObserver (vScrollParam, &component);
		vScrollParam->retain ();
	}

	component.setScrollOnEvent (true);
	(NEW Message ("scrollDown"))->post (&component);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleListModel::viewDetached (IItemView* itemView)
{
	SuperClass::viewDetached (itemView);

	if(vScrollParam)
	{
		ISubject::removeObserver (vScrollParam, &component);
		vScrollParam->release ();
		vScrollParam = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ConsoleListModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kViewAttached || msg == IItemView::kViewRemoved)
	{
		bool state = msg == IItemView::kViewAttached;
		component.setViewVisible (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleListModel::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn (33); // kIconColumn
	list.addColumn (64); // kTimeColumn
	list.addColumn (1000); // kTextColumn (horizontal scrollbar should be disabled)
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ConsoleListModel::countFlatItems ()
{
	return events.getEvents ().count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ConsoleListModel::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(firstDraw)
	{
		firstDraw = false;
		const IVisualStyle& style = ViewBox (info.view).getVisualStyle ();
		setInfoIcon (style.getImage ("infoIcon"));
		setErrorIcon (style.getImage ("errorIcon"));
		setWarningIcon (style.getImage ("warningIcon"));
	}

	LogEvent* logEvent = (LogEvent*)events.getEvents ().at (index.getIndex ());
	if(!logEvent)
		return false;

	if(column == kIconColumn)
	{
		IImage* icon = nullptr;
		switch(logEvent->type)
		{
		case Alert::kInformation : icon = getInfoIcon (); break;
		case Alert::kWarning : icon = getWarningIcon (); break;
		case Alert::kError : icon = getErrorIcon (); break;
		}

		if(icon)
			info.graphics.drawImage (icon, info.rect.getLeftTop ());
	}
	else if(column == kTimeColumn)
	{
		if(component.isShowLineNumbers () && logEvent->lineNumber > 0)
		{
			static StringRef kLineStr = CCLSTR ("Line ");
			String lineString (kLineStr);

			info.graphics.drawString (info.rect, lineString << logEvent->lineNumber , info.style.font, info.style.textBrush, Alignment::kLeftCenter);
		}
		else
		{
			String timeString = Format::DateTime::print (logEvent->time, Format::DateTime::kTime);
			info.graphics.drawString (info.rect, timeString, info.style.font, info.style.textBrush, Alignment::kLeftCenter);
		}
	}
	else if(column == kTextColumn)
		info.graphics.drawString (info.rect, logEvent->message, info.style.font, info.style.textBrush, Alignment::kLeftCenter);

	return true;
}
