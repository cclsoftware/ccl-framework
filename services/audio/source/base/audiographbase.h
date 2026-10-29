//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audiographbase.h
// Description : Audio Graph Base Classes
//
//************************************************************************************************

#ifndef _audiographbase_h
#define _audiographbase_h

#include "ccl/extras/audio/audioprocessor.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/audio/iaudiograph.h"

namespace CCL {

class AudioGraphBase;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define DELEGATE_AUDIONODE_METHODS(Base) \
void CCL_API setID (StringID id) override \
{ Base::setID (id); } \
StringID CCL_API getID () const override \
{ return Base::getID (); } \
const IAudioStreamLayout& CCL_API getStreamLayout () const override \
{ return Base::getStreamLayout (); } \
tresult CCL_API connect (IAudioNode* destination, int outputIndex = 0, int inputIndex = 0) override \
{ return Base::connect (destination, outputIndex, inputIndex); } \
tresult CCL_API disconnect (int outputIndex = 0) override \
{ return Base::disconnect (outputIndex); } \
tresult CCL_API disconnectAll () override \
{ return Base::disconnectAll (); }

//************************************************************************************************
// AudioNodeBase
//************************************************************************************************

class AudioNodeBase: public Object,
					 public IAudioNode,
					 public PluginInstance
{
public:
	DECLARE_CLASS_ABSTRACT (AudioNodeBase, Object)

	AudioNodeBase ();

	typedef AudioGraphBase Graph;

	PROPERTY_POINTER (Graph, graph, Graph)
	bool isAttached () const { return graph != nullptr; }

	virtual void onNodeAttached () {}
	virtual void onNodeDetached () {}

	virtual void prepare (const AudioProcessConfiguration& configuration) {}
	virtual void unprepare () {}

	virtual void onDisplayLatencyChanged (int latencyInSamples) {}
	virtual void onActiveChanged (bool state) {}
	virtual void onProcessingChanged (bool state) {}

	// IAudioNode
	void CCL_API setID (StringID id) override;
	StringID CCL_API getID () const override;

	CLASS_INTERFACE2 (IAudioNode, IPluginInstance, Object)

protected:
	MutableCString nodeId;
};

//************************************************************************************************
// AudioGraphBase
//************************************************************************************************

class AudioGraphBase: public AudioProcessorBase,
					  public IAudioGraph,
					  public PluginInstance
{
public:
	DECLARE_CLASS_ABSTRACT (AudioGraphBase, AudioProcessorBase)

	AudioGraphBase ();
	~AudioGraphBase ();

	typedef AudioNodeBase Node;

	struct Connection
	{
		SharedPtr<Node> source;
		int outputIndex = 0;
		SharedPtr<Node> destination;
		int inputIndex = 0;

		bool operator == (const Connection& other) const;
		
		bool matchesSource (Node* source, int outputIndex) const;
		bool matchesSource (Node* source) const;
		bool matchesAny (Node* node) const;
	};

	struct ConnectionKeeper;

	void storeConnection (const Connection& c);
	void unstoreSourceConnection (Node* source, int outputIndex);
	void unstoreSourceConnections (Node* source);
	void cleanupConnections (Node* node);

	SampleRate getSampleRate () const { return processConfiguration.sampleRate; }
	int getMaxBlockSize () const { return processConfiguration.maxBlockSize; }
	int getDisplayLatency () const { return storedDisplayLatency; }

	// For use by realtime-thread only:
	const AudioProcessContext& getCurrentProcessContext () const;
	const AudioIOData* getCurrentIOData () const { return currentIOData; }
	/** Offset into current audio IO data block, if the graph is processed in chunks. */
	int getCurrentFrameOffset () const { return currentFrameOffset; }

	// IAudioGraph
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;
	CapabilityFlags CCL_API getCapabilities () const override;
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API setStreamActive (AudioStreamDirection direction, int index, tbool active) override;
	tresult CCL_API getProcessConfiguration (AudioProcessConfiguration& configuration) const override;
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API getLatency (int& latencyInSamples, AudioStreamDirection direction) const override;
	tbool CCL_API isActive () const override;
	tresult CCL_API setActive (tbool state) override;
	tresult CCL_API addNode (IAudioNode* node) override;
	tresult CCL_API removeNode (IAudioNode* node) override;
	IAudioNode* CCL_API findNode (StringID nodeId) const override;
	void CCL_API setDisplayLatency (int latencyInSamples) override;

	CLASS_INTERFACE2 (IAudioGraph, IPluginInstance, AudioProcessorBase)

protected:
	static constexpr SampleRate kDefaultSampleRate = 44100.;

	ObjectArray nodes;
	Vector<Connection> activeConnections;
	AudioProcessContext fallbackContext;
	int storedDisplayLatency = 0;

	// only valid in process call:
	typedef const AudioProcessContext* AudioProcessContextPtr;
	AudioProcessContextPtr currentProcessContext = nullptr;
	typedef const AudioIOData* AudioIODataPtr;
	AudioIODataPtr currentIOData = nullptr;
	int currentFrameOffset = 0;

	void cleanupAll (); ///< call in dtor of derived class

	// AudioProcessorBase
	void onConfigurationChanged () override;
	void onActiveChanged (bool state) override;
	void onProcessingChanged (bool state) override;

private:
	bool cleanupDone = false;
};

//************************************************************************************************
// AudioGraphBase::ConnectionKeeper
//************************************************************************************************

struct AudioGraphBase::ConnectionKeeper
{
	Vector<Connection> restoreList;

	ConnectionKeeper (AudioGraphBase& graph, Node& node);
	~ConnectionKeeper ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AudioGraphBase::Connection inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool AudioGraphBase::Connection::operator == (const Connection& other) const
{
	return	source == other.source &&
			outputIndex == other.outputIndex &&
			destination == other.destination &&
			inputIndex == other.inputIndex;
}

inline bool AudioGraphBase::Connection::matchesSource (Node* _source, int _outputIndex) const
{
	return source == _source && outputIndex == _outputIndex;
}

inline bool AudioGraphBase::Connection::matchesSource (Node* _source) const
{
	return source == _source;
}

inline bool AudioGraphBase::Connection::matchesAny (Node* node) const
{
	return source == node || destination == node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _audiographbase_h