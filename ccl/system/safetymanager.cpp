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
// Filename    : ccl/system/safetymanager.cpp
// Description : Safety Manager
//
//************************************************************************************************

#include "ccl/system/safetymanager.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/stack.h"
#include "ccl/public/collections/variantvector.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/text/itextstreamer.h"

#include "ccl/system/threading/thread.h"

namespace CCL {
	
//************************************************************************************************
// SafetyManager::CrashReport
//************************************************************************************************

class SafetyManager::CrashReport: public Object,
								  public ICrashReport
{
public:
	CrashReport (TitleMap& actionTitles);
	~CrashReport ();
	
	static const String kLogFileName;

	void beginAction (CStringRef actionId, const String arguments[], int argumentCount);
	void endAction ();
	void parseLogFile ();
	UrlRef getLogFilePath () const;
	
	bool didCrash () const;

	void getActiveUnstableModules (IUnknownList& modules) const;

	void onCrash (const uchar* crashingModulePath, const uchar* crashDumpFile);
	void onCallingModule (const uchar* callingModulePath);
	void onUnexpectedBehavior (const uchar* modulePath);
	void onShutdown ();

	bool tryCleanup (bool reinitialize = true, bool ignoreUnstable = false);

	// ICrashReport
	const IArrayObject& CCL_API getLastActionsBeforeCrash () const override;
	UrlRef CCL_API getModuleCausingCrash () const override;
	UrlRef CCL_API getSystemDumpPath () const override;
	const IUnknownList& CCL_API getUnstableModules () const override;
	const IUnknownList& CCL_API getCallingModules () const override;
	tbool CCL_API didShutdownCleanly () const override;

	CLASS_INTERFACE (ICrashReport, Object)

protected:
	static const String kStartup;
	static const String kShutdown;
	static const String kBeginAction;
	static const String kEndAction;
	static const String kCrashModule;
	static const String kCallingModule;
	static const String kCrashDump;
	static const String kUnexpectedBehavior;

	struct ActionContext
	{
		CString actionId;
		Vector<String> args;

		ActionContext (CStringRef actionId = nullptr, const Vector<String>& args = {})
		: actionId (actionId)
		{}
	};

	mutable Threading::CriticalSection actionLock;

	TitleMap& actionTitles;
	AutoPtr<IStream> fileStream;
	AutoPtr<ITextStreamer> textStreamer;
	Url logFilePath;
	Stack<ActionContext> actionStack;

	bool crashed;
	bool cleanShutdown;
	VariantStringVector lastActions;
	Url moduleCausingCrash;
	Url dumpFilePath;
	UnknownList activeUnstableModules;
	UnknownList unstableModules;
	UnknownList callingModules;
	void initializeLogFile ();
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ISafetyManager& CCL_API System::CCL_ISOLATED (GetSafetyManager) ()
{
	return SafetyManager::instance ();
}

//************************************************************************************************
// SafetyManager
//************************************************************************************************

SafetyManager::SafetyManager ()
: combinedFilter (createCombinedFilter ()),
  signalSource (Signals::kSafetyManagement),
  crashReport (nullptr),
  features (0)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

SafetyManager::~SafetyManager ()
{
	setSafetyOptions (0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SafetyManager::setSafetyOptions (int _features)
{
	features = _features;

	if(features & kCrashDetection)
	{
		if(!crashReport.isValid ())
			crashReport = NEW CrashReport (actionTitles);
	}
	else
		crashReport.release ();

	enableCrashRecovery (features & kCrashRecovery);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::setValue (CStringRef safetyOptionId, tbool state)
{
	Threading::ScopedLock guard (optionLock);

	if(state == 0)
	{
		if(activeOptions.remove (safetyOptionId))
			signalSource.deferSignal (NEW Message (Signals::kSafetyOptionsChanged));
	}
	else if(getValue (safetyOptionId) == 0)
	{
		activeOptions.add (safetyOptionId);
		signalSource.deferSignal (NEW Message (Signals::kSafetyOptionsChanged));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyManager::getValue (CStringRef safetyOptionId) const
{
	Threading::ScopedLock guard (optionLock);

	return activeOptions.index (safetyOptionId) >= 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SafetyManager::addFilter (IObjectFilter* filter)
{
	Threading::ScopedLock guard (optionLock);

	ForEachUnknown (filters, unk)
		if(filter == unk)
			return kResultAlreadyExists;
	EndFor
	
	if(filters.add (filter, false))
		return kResultOk;

	return kResultFailed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SafetyManager::countFilters () const
{
	if((features & kObjectFilters) == 0)
		return 0;
	
	Threading::ScopedLock guard (optionLock);

	return IterCountData (filters.createIterator ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const IObjectFilter* CCL_API SafetyManager::getFilter (int index) const
{
	if((features & kObjectFilters) == 0)
		return nullptr;
	
	Threading::ScopedLock guard (optionLock);

	AutoPtr<IUnknownIterator> it = filters.createIterator ();
	UnknownPtr<IObjectFilter> filter;
	for(int i = 0; filter = it->nextUnknown (); i++)
	{
		if(i == index)
			return filter;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const IObjectFilter& CCL_API SafetyManager::getCombinedFilter () const
{
	return *combinedFilter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SafetyManager::registerAction (CStringRef actionId, StringRef title)
{
	actionTitles.add ({ actionId, title });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::beginAction (CStringRef actionId, const String arguments[], int argumentCount)
{
	if(crashReport)
		crashReport->beginAction (actionId, arguments, argumentCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::endAction ()
{
	if(crashReport)
		crashReport->endAction ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ICrashReport* CCL_API SafetyManager::detectCrash ()
{
	if(crashReport)
	{
		crashReport->parseLogFile ();
		if(crashReport->didCrash ())
			return return_shared (static_cast<ICrashReport*> (crashReport));
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SafetyManager::checkStability ()
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	if(crashReport)
	{
		AutoPtr<UnknownList> modules (NEW UnknownList);
		crashReport->getActiveUnstableModules (*modules);
		if(!modules->isEmpty ())
			signalSource.signal (Message (Signals::kModuleException, Variant (static_cast<IUnknownList*> (modules), true)));

		crashReport->tryCleanup (true, true);
	}

	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SafetyManager::reportException (void* exceptionInformation, const uchar* systemDumpFile)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyManager::handleException ()
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IObjectFilter* SafetyManager::createCombinedFilter ()
{
	return ObjectFilter::create ([this] (IUnknown* object)
	{
		if((features & kObjectFilters) == 0)
			return false;

		ForEachUnknown (filters, unk)
			UnknownPtr<IObjectFilter> filter = unk;
			if(filter && filter->matches (object))
				return true;
		EndFor
		return false;
	});
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::enableCrashRecovery (bool state)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::reportCrash (const uchar* crashingModule, const uchar* systemDumpFile)
{
	if(crashReport)
		crashReport->onCrash (crashingModule, systemDumpFile);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::reportCallingModule (const uchar* callingModule)
{
	if(crashReport)
		crashReport->onCallingModule (callingModule);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::reportUnexpectedBehavior (const uchar* modulePath)
{
	if(crashReport)
		crashReport->onUnexpectedBehavior (modulePath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API SafetyManager::countDiagnosticData () const
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyManager::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index == 0)
	{
		description.categoryFlags = DiagnosticDescription::kErrorInformation;
		description.fileName = CrashReport::kLogFileName;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* CCL_API SafetyManager::createDiagnosticData (int index)
{
	if(crashReport)
		return System::GetFileSystem ().openStream (crashReport->getLogFilePath (), IStream::kOpenMode | IStream::kShareWrite);
	return nullptr;
}

//************************************************************************************************
// SafetyManager::CrashReport
//************************************************************************************************

const String SafetyManager::CrashReport::kLogFileName ("Actions.log");

const String SafetyManager::CrashReport::kStartup ("[startup]");
const String SafetyManager::CrashReport::kShutdown ("[shutdown]");
const String SafetyManager::CrashReport::kBeginAction ("[action]");
const String SafetyManager::CrashReport::kEndAction ("[done]");
const String SafetyManager::CrashReport::kCrashModule ("[crash]");
const String SafetyManager::CrashReport::kCallingModule ("[call]");
const String SafetyManager::CrashReport::kCrashDump ("[dump]");
const String SafetyManager::CrashReport::kUnexpectedBehavior ("[except]");

////////////////////////////////////////////////////////////////////////////////////////////////////

SafetyManager::CrashReport::CrashReport (TitleMap& actionTitles)
: crashed (false),
  cleanShutdown (true),
  actionTitles (actionTitles)
{
	initializeLogFile ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SafetyManager::CrashReport::~CrashReport ()
{
	ASSERT (actionStack.isEmpty ())

	if(!tryCleanup (false))
		onShutdown ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::beginAction (CStringRef actionId, const String arguments[], int argumentCount)
{
	Threading::ScopedLock guard (actionLock);
	
	if(crashed)
		return;

	Vector<String> args;
	for(int i = 0; i < argumentCount; i++)
		args.add (arguments[i]);
	actionStack.push (ActionContext (actionId, args));
	
	AutoPtr<Threading::NativeThread> thread = Threading::NativeThreadRegistrar::openThread (System::GetThreadSelfID ());
	if(textStreamer && fileStream && thread)
	{
		fileStream->seek (0, IStream::kSeekEnd);

		String line = kBeginAction;
		line.appendFormat ("[%(1):%(2)]", thread->getThreadID (), thread->getName ());
		line.appendCString (Text::kUTF8, actionId);
		if(args.count () > 0)
		{
			line.append (", ");
			for(int i = 0; i < argumentCount - 1; i++)
				line.appendFormat ("%(1), ", arguments[i]);
			line.append (arguments[argumentCount - 1]);
		}
		textStreamer->writeLine (line);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::endAction ()
{
	{
		Threading::ScopedLock guard (actionLock);

		if(crashed)
			return;

		if(textStreamer && fileStream)
		{
			fileStream->seek (0, IStream::kSeekEnd);

			String line = kEndAction;
			line.appendFormat ("[%(1)]", System::GetThreadSelfID ());
			textStreamer->writeLine (line);
		}
		actionStack.pop ();
	}

	tryCleanup ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::parseLogFile ()
{
	Threading::ScopedLock guard (actionLock);
	
	String line;

	struct ActionStack
	{
		Stack<String> stack;
		Core::Threads::ThreadID threadId;

		ActionStack (Core::Threads::ThreadID threadId = -1)
		: threadId (threadId)
		{}

		ActionStack& operator= (const ActionStack& other)
		{
			threadId = other.threadId;
			stack.removeAll ();
			for(StringRef string : other.stack)
				stack.push (string);
			return *this;
		}
	};
	Vector<ActionStack> actions;

	auto reset = [&]
	{
		cleanShutdown = false;
		moduleCausingCrash = Url::kEmpty;
		dumpFilePath = Url::kEmpty;
		lastActions.removeAll ();
		actions.removeAll ();
		unstableModules.removeAll ();
		callingModules.removeAll ();
	};

	auto getThreadId = [&] (StringRef line, StringRef label)
	{
		int start = (label.length ()) + 1;
		int length = line.subString (start).index ("]");
		Core::Threads::ThreadID threadId = -1;
		line.subString (start, length).getIntValue (threadId);
		return threadId;
	};
	
	auto getText = [&] (StringRef line, bool skipThreadId)
	{
		int start = line.index ("]") + 1;
		String text = line.subString (start);
		if(skipThreadId)
		{
			start = text.index ("]") + 1;
			text = text.subString (start);
		}
		return text;
	};

	reset ();
	cleanShutdown = true;
	
	if(fileStream)
		fileStream->seek (0, IStream::kSeekSet);

	while(textStreamer && textStreamer->readLine (line))
	{
		if(line.startsWith (kStartup))
		{
			Core::Threads::ThreadID threadId = getThreadId (line, kStartup);
			if(threadId == System::GetMainThread ().getThreadID ())
				break;
			reset ();
		}
		else if(line.startsWith (kShutdown))
			cleanShutdown = true;
		else if(line.startsWith (kCrashModule))
			moduleCausingCrash.fromDisplayString (getText (line, false));
		else if(line.startsWith (kCrashDump))
			dumpFilePath.fromDisplayString (getText (line, false));
		else if(line.startsWith (kUnexpectedBehavior))
		{
			AutoPtr<Url> modulePath (NEW Url);
			modulePath->fromDisplayString (getText (line, false));
			
			bool found = false;
			ForEachUnknown (unstableModules, unk)
				UnknownPtr<IUrl> url (unk);
				if(url && modulePath->isEqualUrl (*url))
				{
					found = true;
					break;
				}
			EndFor

			if(!found)
				unstableModules.add (modulePath->asUnknown (), true);
		}
		else if(line.startsWith (kCallingModule))
		{
			AutoPtr<Url> modulePath (NEW Url);
			modulePath->fromDisplayString (getText (line, false));
			
			bool found = false;
			ForEachUnknown (callingModules, unk)
				UnknownPtr<IUrl> url (unk);
				if(url && modulePath->isEqualUrl (*url))
				{
					found = true;
					break;
				}
			EndFor

			if(!found)
				callingModules.add (modulePath->asUnknown (), true);
		}
		else if(line.startsWith (kEndAction) || line.startsWith (kBeginAction))
		{
			bool begin = line.startsWith (kBeginAction);

			Core::Threads::ThreadID threadId = getThreadId (line, begin ? kBeginAction : kEndAction);
			
			if(threadId == -1)
				continue;

			ActionStack* currentThreadActions = nullptr;
			for(ActionStack& threadActions : actions)
			{
				if(threadActions.threadId == threadId)
				{
					currentThreadActions = &threadActions;
					break;
				}
			}

			if(currentThreadActions == nullptr)
				if(actions.add (ActionStack (threadId)))
					currentThreadActions = &actions.last ();

			if(currentThreadActions)
			{
				if(begin)
				{
					String actionContext = getText (line, true);
					String title;
					Vector<Variant> args;
					ForEachStringToken (actionContext, CCLSTR (","), token)
						if(title.isEmpty ())
							title = token.trimWhitespace ();
						else
							args.add (Variant ().fromString (token.trimWhitespace ()));
					EndFor

					for(TitleMapping& actionTitle : actionTitles)
					{
						if(actionTitle.id == MutableCString (title, Text::kUTF8))
						{
							actionContext = String ().appendFormat (actionTitle.title, args, args.count ());
							break;
						}
					}
					currentThreadActions->stack.push (actionContext);
				}
				else
					currentThreadActions->stack.pop ();
			}
		}
	}

	for(ActionStack& action : actions)
		if(action.stack.count () > 0)
			lastActions.add (action.stack.pop ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef SafetyManager::CrashReport::getLogFilePath () const
{
	return logFilePath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SafetyManager::CrashReport::didCrash () const
{
#if RELEASE
	if(!didShutdownCleanly ())
		return true;
#endif
	if(!moduleCausingCrash.isEmpty () || !dumpFilePath.isEmpty ())
		return true;
	if(!didShutdownCleanly () && !lastActions.isEmpty ())
		return true;
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::getActiveUnstableModules (IUnknownList& modules) const
{
	Threading::ScopedLock guard (actionLock);
	
	ForEachUnknown (activeUnstableModules, unk)
		modules.add (unk, true);
	EndFor
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const IArrayObject& CCL_API SafetyManager::CrashReport::getLastActionsBeforeCrash () const
{
	return lastActions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API SafetyManager::CrashReport::getModuleCausingCrash () const
{
	return moduleCausingCrash;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API SafetyManager::CrashReport::getSystemDumpPath () const
{
	return dumpFilePath;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const IUnknownList& CCL_API SafetyManager::CrashReport::getUnstableModules () const
{
	return unstableModules;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const IUnknownList& CCL_API SafetyManager::CrashReport::getCallingModules () const
{
	return callingModules;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyManager::CrashReport::didShutdownCleanly () const
{
	return cleanShutdown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SafetyManager::CrashReport::tryCleanup (bool reinitialize, bool ignoreUnstable)
{
	Threading::ScopedLock guard (actionLock);
	
	if(actionStack.isEmpty () && !crashed)
	{
		if(!ignoreUnstable && !activeUnstableModules.isEmpty ())
			return false;

		if(fileStream && textStreamer)
		{
			fileStream->seek (0, IStream::kSeekEnd);
			if(fileStream->tell () == 0)
				return true;
		}

		textStreamer.release ();
		fileStream.release ();
		activeUnstableModules.removeAll ();

		System::GetFileSystem ().removeFile (logFilePath);

		if(reinitialize)
			initializeLogFile ();

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::initializeLogFile ()
{
	System::GetSystem ().getLocation (logFilePath, System::kAppSettingsPlatformFolder);
	logFilePath.descend (kLogFileName);

	int mode = IStream::kWriteMode|IStream::kReadMode|IStream::kShareRead|INativeFileStream::kWriteFlushed;
	if(!System::GetFileSystem ().fileExists (logFilePath))
		mode |= IStream::kCreateMode;

	fileStream = System::GetFileSystem ().openStream (logFilePath, mode);
	if(fileStream)
	{
		textStreamer = System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kSystemLineFormat, ITextStreamer::kFlushLineEnd});

		fileStream->seek (0, IStream::kSeekEnd);
		textStreamer->writeLine ("");
		String line = kStartup;
		line.appendFormat ("[%(1)]", System::GetThreadSelfID ());
		textStreamer->writeLine (line);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::onCrash (const uchar* crashingModulePath, const uchar* systemDumpFile)
{
	crashed = true;

	Threading::ScopedLock guard (actionLock);

	if(textStreamer && fileStream)
	{
		fileStream->seek (0, IStream::kSeekEnd);

		if(crashingModulePath)
		{
			textStreamer->writeString (kCrashModule);
			textStreamer->writeLine (crashingModulePath);
		}

		if(systemDumpFile && systemDumpFile[0] != '0')
		{
			textStreamer->writeString (kCrashDump);
			textStreamer->writeLine (systemDumpFile);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::onCallingModule (const uchar* callingModulePath)
{
	Threading::ScopedLock guard (actionLock);

	if(textStreamer && fileStream)
	{
		fileStream->seek (0, IStream::kSeekEnd);

		if(callingModulePath)
		{
			textStreamer->writeString (kCallingModule);
			textStreamer->writeLine (callingModulePath);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::onUnexpectedBehavior (const uchar* modulePath)
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return;

	Threading::ScopedLock guard (actionLock);

	if(textStreamer && fileStream)
	{
		fileStream->seek (0, IStream::kSeekEnd);

		if(modulePath)
		{
			textStreamer->writeString (kUnexpectedBehavior);
			textStreamer->writeLine (modulePath);

			AutoPtr<Url> url (NEW Url);
			url->fromDisplayString (modulePath);
			bool found = false;
			ForEachUnknown (activeUnstableModules, unk)
				UnknownPtr<IUrl> existingUrl (unk);
				if(existingUrl && existingUrl->isEqualUrl (*url))
				{
					found = true;
					break;
				}
			EndFor

			if(!found)
				activeUnstableModules.add (url->asUnknown (), true);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyManager::CrashReport::onShutdown ()
{
	Threading::ScopedLock guard (actionLock);

	if(textStreamer && fileStream)
	{
		fileStream->seek (0, IStream::kSeekEnd);
		textStreamer->writeString (kShutdown);
	}
}
