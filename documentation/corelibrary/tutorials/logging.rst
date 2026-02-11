#######
Logging
#######

Logging often is an essential part of an application that facilitates problem diagnostics. Applications that use logging create logs during execution which can sometimes be used to trace error conditions or narrow down the cause of an issue or bug. The Core Library provides simple yet efficient means to generate and capture logging information for further analysis.

About logging
#############

The logging implementation collects messages that have been placed at certain locations in the code by the developer and forwards them to one or more log sinks. A log sink is a class deriving from :cref:`Core::Portable::LogSink` that accepts log messages via :cref:`Core::Portable::LogSink::write` and processes them. This usually means to print messages to the console, store them in a file, or send them over the network to a logging server. The exact method to use depends on the type of application. Note that the Core Library does not itself provide an implementation of a log sink.

Since large applications easily accumulate logs that contain thousands of entries, each log messages is assigned a severity which allows for filtering when analyzing a log at a later point in time. The severity of a log message is represented by the :cref:`Core::Portable::Severity` type. It has one of the following values:

+----------------------+------------------------------------------------------------------------------------------------+
| Severity             | Description                                                                                    |
+======================+================================================================================================+
| ``kSeverityFatal``   | This value indicates that the application entered a state that prevents normal execution.      |
|                      | Usually, this is the last log message before the application exists or a device restats.       |
+----------------------+------------------------------------------------------------------------------------------------+
| ``kSeverityError``   | An error occured and the operation cannot be completed. However, the application can continue  |
|                      | normal exection.                                                                               |
+----------------------+------------------------------------------------------------------------------------------------+
| ``kSeverityWarning`` | This severity indicates that something unexpected happened but the operation can be completed. |
+----------------------+------------------------------------------------------------------------------------------------+
| ``kSeverityInfo``    | This is the default severity for log messages. Use this severity when the application reached  |
|                      | a certain state or an operation succeeded.                                                     |
+----------------------+------------------------------------------------------------------------------------------------+
| ``kSeverityDebug``   | Use this severity for log messages intended to aid in diagnosing issues. ^This information is  |
|                      | often only useful for the application developer.                                               |
+----------------------+------------------------------------------------------------------------------------------------+
| ``kSeverityTrace``   | This severity is used when full visibility is needed of what the application is doing.         |
+----------------------+------------------------------------------------------------------------------------------------+

The severity value of a log message is used by the log sink to determine whether a particular message needs to be processed. Use the :cref:`Core::Portable::LogSink::setReportOptions` function to configure the minimum severity at which the log sink will process messages. The default log level is ``kSeverityInfo``.

Messages can be logged using the message functions, like :cref:`Core::Portable::Logging::info` or :cref:`Core::Portable::Logging::error`. They accept ``printf``-style parameters. There exists one function for each severity. Calling one of these functions will eventually invoke :cref:`Core::Portable::LogSink::write` of each log sink that is registered with the logger passing on the log message and its severity value.

Using logging
#############

To use logging in your code include the ``corelogging.h`` header file and declare an instance of :cref:`Core::Portable::Logger`. The ``Logger`` object is a singleton that hosts the log sinks. Then during application initialization add one or more instances of :cref:`Core::Portable::LogSink` to the logger. The application can then use the message functions. The following example shows how to setup logging in an application.

.. code-block:: cpp

  #include "core/portable/corelogging.h"
  
  using namespace Core;
  using namespace Portable;
  
  static Logger gLogger;
  
  int main ()
  {
  	ConsoleLogSink logSink;
  	gLogger.addSink (&logSink);
  
  	Logging::info ("Hello, World!");
  	return 0;
  }

Writing custom log sinks
########################

As stated before, the Core Library does not come with any log sinks, however, a simple log sink can be written in a few lines of code. The following example implements a log sink that was used in the previous example. This class writes all messages to the screen including the severity of the message.

.. code-block:: cpp

  using namespace Core;
  using namespace Portable;
  
  class ConsoleLogSink: public LogSink
  {
  public:
  	void write (Severity severity, CStringPtr message) override
  	{
  		constexpr CStringPtr kSeverityNames[6] = { "FATAL", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE" };
  		printf ("%s: %s\n", kSeverityNames[static_cast<int>(severity)], message);
  	}
  };

The ``ConsoleLogSink`` implements the pure virtual ``write`` function from :cref:`Core::Portable::LogSink` and uses the ``printf`` function to print the message to the console. Similarly, programmers can write log sinks that write messages to the file, or forward them to the logging facility of the operation system, i.e. using Event Logging for Windows or syslog on macOS or Linux. The log sink can also add other features to messages, for instance, by adding a timestamp.