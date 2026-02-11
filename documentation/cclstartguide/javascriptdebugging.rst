####################
JavaScript Debugging
####################

CCL-based applications using an embedded JavaScript engine can act as a Debug Adapter Protocol (DAP) server when launched with a corresponding command line argument.

To attach Visual Studio Code to a CCL-based application, launch the application with a command line argument::

-debug dap:12345

where 12345 is a port number on localhost.

In Visual Studio Code, add a launch configuration as follows:

.. code-block:: json-object

	{
		"version": "x.x.x",
		"configurations": [
			{
				"name": "attach to debug server",
				"type": "node",
				"debugServer": 12345,
				"request": "attach"
			}
		]
	}

When the application is running, launch the launch configuration in Visual Studio Code. Breakpoints can be set in any JavaScript source file used in the application.
