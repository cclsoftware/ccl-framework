rem .natstepfilter files don't seem to work per project - Visual Studio only searches them in the user documents folder.
rem Create symbolic links there pointing to the .natstepfilter files in this folder.

rem this must be run as administrator!
rem the current dir will then be system32, change it back to the script directory:
cd /d %0\..
set CCL_WIN=%0\..
set USERFOLDER="%USERPROFILE%\Documents\Visual Studio 2022\Visualizers\"
mkdir %USERFOLDER%
mklink %USERFOLDER%\cclbase.natstepfilter %CCL_WIN%\cclbase.natstepfilter
mklink %USERFOLDER%\ccltext.natstepfilter %CCL_WIN%\ccltext.natstepfilter
mklink %USERFOLDER%\cclgui.natstepfilter %CCL_WIN%\cclgui.natstepfilter
mklink %USERFOLDER%\corelib.natstepfilter %CCL_WIN%\..\..\..\..\embedded\core\cmake\visualstudio\corelib.natstepfilter
pause
