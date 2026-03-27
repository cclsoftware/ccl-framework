###############
Project Folders
###############

.. _ccl-project-folders:

We use the following folder structure for our projects:

.. code-block:: rst

  project
  |-- assets          content files installed with the application (optional)
  |-- cmake           CMake configuration files
  |-- doc             project-specific documentation (optional)
  |-- packaging       installer scripts with subfolder per platform, platform-specific resources (optional)
  |-- resource        cross-platform resources compiled into the application (icons, configuration files, etc.)
  |-- skin            skin XML files and images for GUI applications (optional)
  |-- source          source files (.h, .cpp, .mm)

Folder names are all lowercase letters. Do not mix source files and platform-specific project or settings files in one folder.

