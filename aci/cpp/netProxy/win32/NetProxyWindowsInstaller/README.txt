The NetProxy installer is included in the NetProxy.sln.

The file "ModifyMsiToEnableLaunchApplication.js" is a javascript file which is run by
Visual Studio after the solutions has been built. The reason for that is Visual Studio 
doesn't allow to set a custom action in the last dialog box (the "finish" dialog box) but 
it's necessary to create the .msi file and manually modify that. The custon action we are 
insterested to is the tap interface test which is launched using "netproxy.exe -testtap"

The folder ConfFilesPermissionSetter contains a C# project which main class is run during 
the NetProxy installation in order to change the configuration files permission if they are not 
writeble. 