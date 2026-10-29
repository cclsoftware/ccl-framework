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
// Filename    : miniaudiograph.h
// Description : Audio Graph based on miniaudio
//
//************************************************************************************************

#ifndef _miniaudiograph_h
#define _miniaudiograph_h

#include "miniaudioglue.h"

#include "../base/audiographbase.h"
#include "../base/audiometering.h"
#include "../base/audiopanning.h"
#include "../base/audioparams.h"

#include "ccl/extras/audio/audiodatabuffer.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/audio/iaudiosource.h"
#include "ccl/public/gui/iparamprovider.h"

namespace CCL {

//************************************************************************************************
// MiniAudioNode
//************************************************************************************************

class MiniAudioNode: public AudioNodeBase
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioNode, AudioNodeBase)

	~MiniAudioNode ();

	ma_node_graph* getNodeGraph ();

	ma_node* getNode () { return nodePointer; }
	bool isValid () const { return initDone; }

	int getBusOffset (AudioStreamDirection direction, int streamIndex) const;

	struct ProcessingSuspender;
	struct ActiveCallGuard;

	void waitForProcessFinished ()
	{
		while(activeCallCount.getValue () > 0)
			System::ThreadSleep (0);
	}

	// AudioNodeBase
	void onNodeAttached () override;
	void onNodeDetached () override;
	void onProcessingChanged (bool state) override;
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	const IAudioStreamLayout& CCL_API getStreamLayout () const override;
	tresult CCL_API connect (IAudioNode* destination, int outputIndex = 0, int inputIndex = 0) override;
	tresult CCL_API disconnect (int outputIndex = 0) override;
	tresult CCL_API disconnectAll () override;

protected:
	ma_node* nodePointer = nullptr;
	bool initDone = false;
	AudioStreamLayout streamLayout;
	Threading::AtomicInt activeCallCount; // bumped by ActiveCallGuard around each processFrames () call

	bool isStarted () const { return isValid () && ma_node_get_state (nodePointer) == ma_node_state_started; }

	virtual void initializeNode () = 0;
	virtual void uninitializeNode ();

	void reinitializeNode ()
	{
		if(isAttached ())
		{
			Graph::ConnectionKeeper keeper (*graph, *this);
			uninitializeNode ();
			initializeNode ();
		}
	}
};

//************************************************************************************************
// MiniAudioNode::ProcessingSuspender
/**	Stops the node, waiting out any processFrames () call already in flight on the audio thread,
	then restores the previous state on destruction. Safe to use from non-realtime setup code only. */
//************************************************************************************************

struct MiniAudioNode::ProcessingSuspender
{
	MiniAudioNode& node;
	tbool wasProcessing = false;

	ProcessingSuspender (MiniAudioNode& node)
	: node (node),
	  wasProcessing (node.isStarted ())
	{
		if(wasProcessing)
		{
			node.onProcessingChanged (false); // stop future calls
			node.waitForProcessFinished (); // wait out a call already in flight
		}
	}

	~ProcessingSuspender ()
	{
		if(wasProcessing)
			node.onProcessingChanged (true);
	}
};

//************************************************************************************************
// MiniAudioNode::ActiveCallGuard
//************************************************************************************************

struct MiniAudioNode::ActiveCallGuard
{
	Threading::AtomicInt& activeCallCount;

	ActiveCallGuard (MiniAudioNode& node)
	: activeCallCount (node.activeCallCount)
	{
		activeCallCount.increment ();
	}

	~ActiveCallGuard ()
	{
		activeCallCount.decrement ();
	}
};

//************************************************************************************************
// MiniAudioOutputNode
/**	Writes an N-channel mix into one output stream of IAudioGraph::process()'s AudioIOData.
	Attaches a dummy 1-channel silent trigger bus directly to the raw ma_node_graph's own
	built-in endpoint, so its callback keeps firing without contributing to the discarded mix. */
//************************************************************************************************

class MiniAudioOutputNode: public MiniAudioNode,
						   public IAudioOutputNode
{
public:
	DECLARE_CLASS (MiniAudioOutputNode, MiniAudioNode)

	MiniAudioOutputNode ();

	// IAudioOutputNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioNode)
	tresult CCL_API setOutputStream (int streamIndex) override;
	int CCL_API getOutputStream () const override;

	// MiniAudioNode
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;

	CLASS_INTERFACE (IAudioOutputNode, MiniAudioNode)

protected:
	Threading::AtomicInt streamIndex;

	void connectToRealEndpoint (bool state);

	// MiniAudioNode
	void initializeNode () override;
	void uninitializeNode () override;

private:
	static void processFrames (ma_node* pNode,
								const float** ppFramesIn, ma_uint32* pFrameCountIn,
								float** ppFramesOut, ma_uint32* pFrameCountOut);

	struct OutputNode: ma_node_base
	{
		MiniAudioOutputNode* owner;
	};

	OutputNode node = {};
	ma_node_vtable vtable = {};
};

//************************************************************************************************
// MiniAudioGraph
//************************************************************************************************

class MiniAudioGraph: public AudioGraphBase
{
public:
	DECLARE_CLASS (MiniAudioGraph, AudioGraphBase)

	~MiniAudioGraph ();

	ma_node_graph* getNodeGraph ();
	bool isValid () const { return initDone; }

	// AudioGraphBase
	tresult CCL_API setProcessConfiguration (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API process (const AudioProcessData& data) override;

protected:
	/** miniaudio processes nodes in chunks of at most the node cache capacity per bus, which is set for this
		target in audioservice-config.cmake. The graph is pulled in chunks of at most this size, so that the
		chunk offset within the block is known. The macro must be defined, it's not visible otherwise. */
	static constexpr int kMaxChunkFrames = MA_DEFAULT_NODE_CACHE_CAP_IN_FRAMES_PER_BUS;
	static constexpr int kChunkAlignment = 8;

	ma_node_graph nodeGraph = {};
	bool initDone = false;
	Array<float32> discardBuffer;
};

//************************************************************************************************
// MiniAudioCustomNode
/**	Custom node with flexible in/out streams.
	Implemented with multiple ma_node mono buses instead of (de)interleaving the audio. */
//************************************************************************************************

class MiniAudioCustomNode: public MiniAudioNode,
						   public IAudioProcessor
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioCustomNode, MiniAudioNode)

	MiniAudioCustomNode ();
	~MiniAudioCustomNode ();

	// IAudioProcessor
	tbool CCL_API isProcessing () const override;
	tresult CCL_API setProcessing (tbool state) override;

	// MiniAudioNode
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;

	CLASS_INTERFACE (IAudioProcessor, MiniAudioNode)

protected:
	// MiniAudioNode
	void initializeNode () override;

private:
	static void processFrames (ma_node* pNode,
							   const float** ppFramesIn, ma_uint32* pFrameCountIn,
							   float** ppFramesOut, ma_uint32* pFrameCountOut);

	struct CustomNode: ma_node_base
	{
		MiniAudioCustomNode* owner;
	};

	CustomNode node = {};
	ma_node_vtable vtable = {};
	AudioIOData ioData;

	virtual void preProcess (AudioParamData& data, 
							 const AudioProcessContext& context, 
							 int numSamples) {}
};

//************************************************************************************************
// MiniAudioParamNode
/**	Custom node with parameter handling. */
//************************************************************************************************

class MiniAudioParamNode: public MiniAudioCustomNode,
						  public IParameterProvider
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioParamNode, MiniAudioCustomNode)

	// IParameterProvider
	int CCL_API countParameters () const override;
	tresult CCL_API getParameterAt (ParameterDefinition& definition, int index) const override;
	tresult CCL_API setParameterValue (int tag, VariantRef value) override;
	tresult CCL_API setParameterValue (StringID name, VariantRef value) override;
	tresult CCL_API getParameterValue (Variant& value, int tag) const override;
	tresult CCL_API getParameterValue (Variant& value, StringID name) const override;

	CLASS_INTERFACES (MiniAudioCustomNode)

protected:
	AudioParamState uiValues;
	
	void initParamList (const ParameterDefinitionList* paramList);
	tresult setValue (const ParameterDefinition& definition, VariantRef value);
	tresult getValue (Variant& value, const ParameterDefinition& definition) const;
	bool scheduleParamEvent (int tag, AudioParamValue value);
	void clearParamEvents ();

	// MiniAudioCustomNode
	void preProcess (AudioParamData& data,
					 const AudioProcessContext& context, 
					 int numSamples) override;

private:
	const ParameterDefinitionList* paramList = nullptr;
	AudioParamQueue paramQueue;
	AudioParamQueue::EventList blockEvents; ///< audio thread only
};

//************************************************************************************************
// MiniAudioInternalParamNode
/**	Node with internal parameter handling. */
//************************************************************************************************

class MiniAudioInternalParamNode: public MiniAudioParamNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioInternalParamNode, MiniAudioParamNode)

protected:
	void processParams (const AudioProcessData& data);

	virtual void applyValue (int tag, AudioParamValue value) {}
};

//************************************************************************************************
// MiniAudioProcessorNode
//************************************************************************************************

class MiniAudioProcessorNode: public MiniAudioParamNode,
							  public IAudioProcessorNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioProcessorNode, MiniAudioParamNode)

	// IAudioProcessorNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioParamNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	tresult CCL_API setProcessor (IAudioProcessor* newProcessor) override;
	IAudioProcessor* CCL_API getProcessor () const override;

	// MiniAudioParamNode
	void prepare (const AudioProcessConfiguration& configuration) override;
	void onDisplayLatencyChanged (int latencyInSamples) override;
	void onActiveChanged (bool state) override;
	void onProcessingChanged (bool state) override;
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioProcessorNode, MiniAudioParamNode)

protected:
	SharedPtr<IAudioProcessor> processor;
	SharedPtr<IAudioComponent> component; // (optional)
	SharedPtr<IParameterDefinitionProvider> provider; // (optional)
	ParameterDefinitionList cachedParamList;
};

//************************************************************************************************
// MiniAudioInputNode
//************************************************************************************************

class MiniAudioInputNode: public MiniAudioCustomNode,
						  public IAudioInputNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioInputNode, MiniAudioCustomNode)

	MiniAudioInputNode ();

	// IAudioInputNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioCustomNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	tresult CCL_API setInputStream (int streamIndex) override;
	int CCL_API getInputStream () const override;

	// MiniAudioGraphCustomNode
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioInputNode, MiniAudioCustomNode)

protected:
	Threading::AtomicInt streamIndex;
};

//************************************************************************************************
// MiniAudioSourceNode
//************************************************************************************************

class MiniAudioSourceNode: public MiniAudioCustomNode,
						   public IAudioSourceNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioSourceNode, MiniAudioCustomNode)

	// IAudioSourceNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioCustomNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	tresult CCL_API setSource (IAudioSource* source) override;
	IAudioSource* CCL_API getSource () const override;

	// MiniAudioCustomNode
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioSourceNode, MiniAudioCustomNode)

protected:
	SharedPtr<IAudioSource> source;
};

//************************************************************************************************
// MiniAudioGainer
/** Array of smoothed, single-channel ma_gainer instances. */
//************************************************************************************************

class MiniAudioGainer
{
public:
	static constexpr TSeconds kSmoothTimeSeconds = 0.01; // 10 ms

	MiniAudioGainer (TSeconds smoothTime = kSmoothTimeSeconds);
	~MiniAudioGainer ();

	/**	Resize the array for the given sample rate.
		Caller must re-apply target gains afterwards. */
	void resize (int count, SampleRate sampleRate);
	int getCount () const { return count; }

	void setGain (int index, float gain);
	void setGains (float gain);

	INLINE void process (int index, float32 dst[], const float32 src[], int numSamples)
	{ ma_gainer_process_pcm_frames (&gainers[index], dst, src, ma_uint64(numSamples)); }

protected:
	ma_gainer* gainers = nullptr;
	int count = 0;
	SampleRate currentRate = 0;
	TSeconds smoothTime;
};

//************************************************************************************************
// MiniAudioGainNode
//************************************************************************************************

class MiniAudioGainNode: public MiniAudioInternalParamNode,
						 public IAudioGainNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioGainNode, MiniAudioInternalParamNode)

	MiniAudioGainNode ();
	~MiniAudioGainNode ();

	// IAudioGainNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioInternalParamNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	tresult CCL_API setGain (float gain) override;
	float CCL_API getGain () const override;

	// MiniAudioInternalParamNode
	void prepare (const AudioProcessConfiguration& configuration) override;
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioGainNode, MiniAudioInternalParamNode)

protected:
	static const ParameterDefinitionList& getParamList ();

	enum ParamTag
	{
		kGain = 1
	};

	MiniAudioGainer gainer;

	// MiniAudioInternalParamNode
	void initializeNode () override;
	void uninitializeNode () override;
	void applyValue (int tag, AudioParamValue value) override;
};

//************************************************************************************************
// MiniAudioPanNode
//************************************************************************************************

class MiniAudioPanNode: public MiniAudioInternalParamNode,
						public IAudioPanNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioPanNode, MiniAudioInternalParamNode)

	MiniAudioPanNode ();

	// IAudioPanNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioInternalParamNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	tresult CCL_API setPan (float pan) override;
	float CCL_API getPan () const override;

	// MiniAudioInternalParamNode
	void prepare (const AudioProcessConfiguration& configuration) override;
	void unprepare () override;
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioPanNode, MiniAudioInternalParamNode)

protected:
	static const ParameterDefinitionList& getParamList ();

	enum ParamTag
	{
		kPan = 1
	};

	AudioPanMatrix matrix;
	MiniAudioGainer gainer;
	AudioDataBuffer inputSnapshot;	///< private copy of input, only filled when needed - see process ()
	AudioDataBuffer scratchBuffer;	///< one channel's worth, reused only by entries that need it - see process ()

	void applyGains ();

	// MiniAudioInternalParamNode
	void initializeNode () override;
	void uninitializeNode () override;
	void applyValue (int tag, AudioParamValue value) override;
};

//************************************************************************************************
// MiniAudioMeterNode
//************************************************************************************************

class MiniAudioMeterNode: public MiniAudioCustomNode,
						  public IAudioMeterNode
{
public:
	DECLARE_CLASS_ABSTRACT (MiniAudioMeterNode, MiniAudioCustomNode)

	// IAudioMeterNode
	DELEGATE_AUDIONODE_METHODS (MiniAudioCustomNode)
	tresult CCL_API setStreamLayout (const IAudioStreamLayout& layout) override;
	IAudioMeterSource& CCL_API getMeterSource (int streamIndex = 0) override;

	// MiniAudioCustomNode
	void prepare (const AudioProcessConfiguration& configuration) override;
	void onDisplayLatencyChanged (int latencyInSamples) override;
	void onProcessingChanged (bool state) override;
	tresult CCL_API process (const AudioProcessData& data) override;

	CLASS_INTERFACE (IAudioMeterNode, MiniAudioCustomNode)

protected:
	AudioLevelDetector levelDetector;

	void updateLevelDetector ();

	// MiniAudioCustomNode
	void initializeNode () override;
	void uninitializeNode () override;
};

} // namespace CCL

#endif // _miniaudiograph_h