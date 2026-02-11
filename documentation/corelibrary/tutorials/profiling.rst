#########
Profiling
#########

The Core Library offers several ways to profile applications and help to understand their behavior. Among the profiling options are measurements of the minimum, maximum, and average time taken to execute blocks of code, or CPU usage in general. For this to work, the Core Library depends on the availability of a system clock, usually a high-performance counter. These are commonly available to all platforms supported by the library. This page demonstrates the different ways to profile an application.


Simple Profiling
################

The profiling macros offer a simple way to measure the execution time of a block of code. Use :cref:`CORE_PROFILE_START` and :cref:`CORE_PROFILE_STOP` to enclose the code of interest. Both macros take an integer as their first argument to allow several measurements within the same function. When execution reaches :cref:`CORE_PROFILE_STOP` the time taken to execute the block of code is being printed on the console along with the string argument passed to the macro.

.. code-block:: cpp

  // Enable profiling for this module
  #define CORE_PROFILE 1
  
  #include "core/public/coreprofiling.h"
  
  void processData ()
  {
  	CORE_PROFILE_START (0)
  	mySlowMethod1 ();
  	CORE_PROFILE_STOP (0, "Time taken for mySlowMethod1")
  
  	CORE_PROFILE_START (1)
  	mySlowMethod2 ();
  	CORE_PROFILE_STOP (1, "Time taken for mySlowMethod2")
  }

The previous example gives the following output on the console.

::

  Time taken for mySlowMethod1 17 µs
  Time taken for mySlowMethod2 38 µs

Of course, these numbers need to be interpreted with caution. They only give an indication to the performance of the code as code execution can be influenced by, for instance, cache usage or overall system load.

Using the performance profiler
##############################

The previous section introduced a profiling mechanism that gives a brief insight into the performance of the code, however, in many cases a more thorough insight is needed. To eleminate random noise of the system it can be necessary to repeat measurement several times and gather the performance data at a common location. The :cref:`Core::Portable::PerformaceProfiler` class of the Core library can do that. It allows for several measurements at different places in the code, collects the timing data, and calculates minimum, maximum, and average performance values. The following code demonstrates how to use the ``PerformanceProfiler`` class.

.. code-block:: cpp

  static PerformanceProfiler gProfiler;
  static bool gRun = false;
  
  constexpr int kRealtimeCounter = 0;
  constexpr int kGraphicsCounter = 1;
  
  void thread1 ()
  {
  	while(gRun)
  	{
  		TimedInterval<true> interval (&gProfiler, kRealtimeCounter);
  		processRealtime ();
  	}
  }
  
  void thread2 ()
  {
  	while(gRun)
  	{
  		TimeInterval<true> interval (&gProfiler, kGraphicsCounter);
  		drawGraphics ();
  	}
  }
  
  int main()
  {
  	// Initialize thre profiler for two counters
  	gProfiler.setup (2);
  
  	// Initialize the application
  	gRun = true;
  	initializeThreads ();
  
  	// The the application run for a moment
  	sleep (30 * 1000);
  	gRun = false;
  
  	// Get the performance data and exit
  	ProfilingData data;
  	gProfiler.getData (data);
  
  	uint32 minimum = 0;
  	uint32 maximum = 0;
  	uint32 average = 0;
  
  	data.getField (minimum, kRealtimeCounter, IProfilingData::Key::kMinInterval);
  	data.getField (maximum, kRealtimeCounter, IProfilingData::Key::kMaxInterval);
  	data.getField (average, kRealtimeCounter, IProfilingData::Key::kAvgInterval);
  	printf ("Realtime: min=%u, max=%u, avg=%u µs\n", counter, minimum, maximum, average\n");
  
  	data.getField (minimum, kGraphicsCounter, IProfilingData::Key::kMinInterval);
  	data.getField (maximum, kGraphicsCounter, IProfilingData::Key::kMaxInterval);
  	data.getField (average, kGraphicsCounter, IProfilingData::Key::kAvgInterval);
  	printf ("Graphics: min=%u, max=%u, avg=%u µs\n", counter, minimum, maximum, average\n");
  
  	return 0;
  }

In this example the application uses the :cref:`Core::Portable::PerformanceProfiler` class to gather information about the performance of the two threads that process realtime data and draw graphics. In each thread the :cref:`Core::TimedInterval` template class is used to automatically start and stop profiling the code in the current scope. The ``interval`` object takes a pointer to the ``PerformanceProfiler`` class that receives the profiling data and an identifier.

After the application ran for a sufficient amount of time, the ``main`` function uses a :cref:`Core::Portable::ProfilerData` object to fetch the counter information collected so far. For each counter the :cref:`Core::Portable::ProfilerData::getField` function is returns several data items, for instance, the average execution time in microseconds and its upper and lower bounds which is then printed to the console.