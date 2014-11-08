using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;
using System.Security.Principal;
using System.Security.AccessControl;
using System.IO;
using System.Diagnostics;
using System.Windows.Forms;
using IWshRuntimeLibrary;


namespace Installer
{
    /// <summary>
    /// Class that extends the NetProxy installer
    /// </summary>
    [RunInstaller (true)]
    public partial class installerExtender : System.Configuration.Install.Installer
    {
        /// <summary>
        /// Method that override OnBeforeInstall and run the tap test interface. 
        /// If the tap interface is not installed the OpenVPN driver installation 
        /// is proposed
        /// </summary>
        /// <param name="savedState">The installation state</param>
        [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
        protected override void OnBeforeInstall (IDictionary savedState)
        {
            base.OnBeforeInstall(savedState);

            FileInfo fileInfo = new FileInfo(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string execTestFile = Context.Parameters["RunInstallationTest"];
            execTestFile = Path.Combine(fileInfo.DirectoryName,execTestFile);
            Process tapTestProcess = Process.Start(execTestFile,"-testtap");
            tapTestProcess.WaitForExit();
            int tapTestResult = tapTestProcess.ExitCode;

            if(tapTestResult == -0) {
                TapInterfaceTestResDialog dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - SUCCESSFULLY RUN" + 
                    System.Environment.NewLine + "The TAP interface is correctly installed and configured",false,false);
                dialog.ShowDialog();
                dialog.Dispose();
                return;
            }

            // The tap interface driver are not installed
            if (tapTestResult == -5) {
                TapInterfaceTestResDialog openVPnDialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - NO TAP-WIN32 INTERFACE FOUND" +
                                System.Environment.NewLine + "The installation process can be completed only installing the tap interface drivers. " +
                                System.Environment.NewLine + "It is possible to install the openVpn drivers from this windows. ",true,true);
                if((openVPnDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK) && (openVPnDialog.installOpenVpn())) {
                        string execOpenVPNFile = Context.Parameters["RunOpenVpnInstallation"];
                        execOpenVPNFile = Path.Combine(fileInfo.DirectoryName,execOpenVPNFile);
                        Process openVPNProcess = Process.Start(execOpenVPNFile);
                        openVPNProcess.WaitForExit();
                        openVPnDialog.Dispose();
                        tapTestResult = openVPNProcess.ExitCode;
                }
                else {
                    throw new Exception(System.Environment.NewLine + System.Environment.NewLine + 
                        "TAP INTERFACE TEST - NO TAP-WIN32 INTERFACE FOUND" +  System.Environment.NewLine + 
                        "The installation process will stop");
                }

                if (tapTestResult!=0) {
                    throw new Exception(System.Environment.NewLine + System.Environment.NewLine + 
                        "OPENVPN INSTALLATION PROCESS - The tap interface drivers has not been correctly installed. " + 
                        System.Environment.NewLine + "The NetProxy installation process will stop");
                }

                if(tryAddressesSetting(execTestFile) == 0) {
                    TapInterfaceTestResDialog dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - SUCCESSFULLY RUN" +
                        System.Environment.NewLine + "The TAP interface is correctly installed and configured",false,false);
                    dialog.ShowDialog();
                    dialog.Dispose();
                } 
                return;
                
            }

            // The ip address and the mac address don't match
            if (tapTestResult == -4) {
                if(tryAddressesSetting(execTestFile) == 0) {
                    TapInterfaceTestResDialog dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - SUCCESSFULLY RUN" +
                        System.Environment.NewLine + "The TAP interface is correctly installed and configured",false,false);
                    dialog.ShowDialog();
                    dialog.Dispose();
                } 
                return;
            }

            // Other error in the tap interface test
            displayOtherErrorDialogs (tapTestResult);
            
        }

        
        /// <summary>
        /// Method that tries to set the ip, subnet mask, default gateway 
        /// and mac addresses using the .exe file specified as paramerer. 
        /// The exec file set also the MTU registry to 1500 and the 
        /// AllowNonAdmin registy to 1.
        /// </summary>
        /// <param name="execTestFile">Executable file to change the addresses</param>
        /// <returns>0 if everything went fine, a negative number otherwise</returns>
        private int tryAddressesSetting(string execTestFile)
        {
            int tapTestResult = -4;
            NetworkParametersForm netParamForm = new NetworkParametersForm();
            showNetworkParametersFromDialog(netParamForm);
            DialogResult dr = netParamForm.ShowDialog();
            if(dr == System.Windows.Forms.DialogResult.OK) {
                string adapterDescriptor = netParamForm.getAdapterDescriptor();
                string ip = netParamForm.getIPAddress();
                string mask = netParamForm.getMaskAddress();
                string gateway = netParamForm.getGatewayAddress();
                string mac = netParamForm.getMACAddress();
                string ipOptions;
                if(String.IsNullOrEmpty(gateway)) {
                    ipOptions = "-updateIP -d \"" + adapterDescriptor + "\" -ip " + ip + " -mask " + mask;
                }
                else {
                    ipOptions = "-updateIP -d \"" + adapterDescriptor + "\" -ip " + ip + " -mask " + mask + " -gateway " + gateway;
                }
                Process setIPProcess = Process.Start(execTestFile,ipOptions);
                setIPProcess.WaitForExit();
                tapTestResult = setIPProcess.ExitCode;
                if(tapTestResult != 0) {
                    TapInterfaceTestResDialog netParametersDialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - NETWORK PARAMERS SETTING" +
                        System.Environment.NewLine + "The ip, netmask and gateway addresses have not been setted properly. " +
                        System.Environment.NewLine + "This won't stop the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    netParametersDialog.ShowDialog();
                    netParametersDialog.Dispose();
                    return -1;
                }
                string macOptions = "-updateMac -d \"" + adapterDescriptor + "\" -a " + mac;
                Process setMacProcess = Process.Start(execTestFile,macOptions);
                setMacProcess.WaitForExit();
                tapTestResult = setMacProcess.ExitCode;
                if(tapTestResult != 0) {
                    TapInterfaceTestResDialog netParametersDialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - NETWORK PARAMERS SETTING" +
                        System.Environment.NewLine + "The mac address has not been setted properly. " +
                        System.Environment.NewLine + "This won't stop the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    netParametersDialog.ShowDialog();
                    netParametersDialog.Dispose();
                    return -2;
                }

                // Run again the test to check the network parameters for the just installed tap interface
                Process tapTestProcess = Process.Start(execTestFile,"-testtap");
                tapTestProcess.WaitForExit();
                tapTestResult = tapTestProcess.ExitCode;
            }
            if((dr == System.Windows.Forms.DialogResult.Cancel) || (tapTestResult == -4)) {
                //netParamForm.Dispose();
                TapInterfaceTestResDialog netParametersDialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - NETWORK PARAMERS SETTING" +
                    System.Environment.NewLine + "The network parameters have not been setted properly. " +
                    System.Environment.NewLine + "This won't stop the installation process but it will preclude " +
                    System.Environment.NewLine + "the correct netproxy working.",true,false);
                netParametersDialog.ShowDialog();
                netParametersDialog.Dispose();
                return -3;
            }
            //netParamForm.Dispose();
            return 0;
        }

        /// <summary>
        /// Method that instantiates a Dialog to set the addresses.
        /// </summary>
        /// <param name="netParamForm">Reference to the dialog form</param>
        private void showNetworkParametersFromDialog(NetworkParametersForm netParamForm)
        {
            string adapter;
            string ip;
            string mask;
            string gateway;
            string mac;

            try {
                var netProperties = new Dictionary<string,string>();
                string[] networkProperties = System.IO.File.ReadAllLines("C:\\\\Temp\\netParams.txt");
                if(networkProperties.Length != 0) {
                    foreach(var row in networkProperties) {
                        netProperties.Add(row.Split('=')[0],row.Split('=')[1]);
                    }
                    ip = netProperties["ip"];
                    netParamForm.setIPAddress(ip);
                    mask = netProperties["subnet mask"];
                    netParamForm.setMaskAddress(mask);
                    gateway = netProperties["default gateway"];
                    netParamForm.setGatewayAddress(gateway);
                }
                adapter = "TAP-Win32 Adapter V9";
                netParamForm.setAdapterDescriptor(adapter);

            }
            catch(Exception ex) {
                adapter = "TAP-Win32 Adapter V9";
                netParamForm.setAdapterDescriptor(adapter);
                ip = null;
                mask = null;
                gateway = null;
                mac = null;
            }
        }
         /// <summary>
         /// Method to show a dialog form with error messages due to the tap 
         /// test interface failure.
         /// </summary>
         /// <param name="tapTestResult">Result of the tap interface test</param>
        private void displayOtherErrorDialogs(int tapTestResult)
        {
            TapInterfaceTestResDialog dialog;
            switch(tapTestResult) {
                case -6:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - FOUND MORE THAN ONE TAP INTERFACE: DO NOT KNOW WHICH ONE TO USE" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
                case -7:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - FAILED OPENING THE TAP INTERFACE DEVICE" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
                case -8:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - FAILED CREATING THE READING EVENT" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
                case -9:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - FAILED CREATING THE WRITING EVENT" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
                case -10:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - UNKNOWN ERROR" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
                default:
                    dialog = new TapInterfaceTestResDialog("TAP INTERFACE TEST - UNKNOWN ERROR" +
                        System.Environment.NewLine + "This doesn't influence the installation process but it will preclude " +
                        System.Environment.NewLine + "the correct netproxy working.",true,false);
                    break;
            }
            dialog.ShowDialog();
            dialog.Dispose();
        }


        /// <summary>
        /// The method override Install in order to set user permission to the folders "folders" 
        /// passed as a parameter of a custom action. 
        /// This method is invoked by the custom actions "Change Conf Files Permissions" and 
        /// "Change Log Files Permissions"
        /// </summary>
        [System.Security.Permissions.SecurityPermission (System.Security.Permissions.SecurityAction.Demand)]
        public override void Install (IDictionary stateSaver)
        {
            // This gets the named parameters passed in from your custom action
            string[] folders = Context.Parameters["Folders"].Split (';');
            foreach(string folder in folders) {

                // This gets the "Authenticated Users" group, no matter what it's called
                SecurityIdentifier sid = new SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid,null);

                // Create the rules
                FileSystemAccessRule writerule = new FileSystemAccessRule(sid,FileSystemRights.FullControl,InheritanceFlags.None,PropagationFlags.InheritOnly,AccessControlType.Allow);

                if(!string.IsNullOrEmpty(folder) && Directory.Exists(folder)) {
                    // Get your file's ACL
                    DirectorySecurity dsecurity = Directory.GetAccessControl(folder);

                    // Add the new rule to the ACL
                    dsecurity.AddAccessRule(writerule);

                    // Set the ACL back to the file
                    Directory.SetAccessControl(folder,dsecurity);

                    string[] confFiles = Directory.GetFiles(folder);
                    writerule = new FileSystemAccessRule(sid,FileSystemRights.FullControl,AccessControlType.Allow);
                    for(int i = 0;i < confFiles.Length;i++) {
                        string currFile = confFiles[i];
                        FileSecurity fsecurity = System.IO.File.GetAccessControl(currFile);
                        fsecurity.AddAccessRule(writerule);
                        System.IO.File.SetAccessControl(currFile,fsecurity);

                        if(folder.EndsWith("conf")) {
                            createShortcut (currFile, null, null);
                            
                        }
                    }
                }

                string exeShortcut = Context.Parameters["ExeShortcut"];
                string iconLocation = Context.Parameters["IconLocation"];
                createShortcut(exeShortcut,iconLocation,"\\exe.ico");
            }


            // Explicitly call the overriden method to properly return control to the installer
            base.Install(stateSaver);
        }

        /// <summary>
        /// Creates a link of the currFile in the start menu
        /// </summary>
        /// <param name="currFile"></param>
        private void createShortcut(string currFile, string iconLocation, string iconName)
        {
            string startMenuDir = Context.Parameters["StartMenuDir"];
            //throw new Exception("startMenuDir = " + startMenuDir + " currFile = " + currFile + " iconLocation = " + iconLocation);
            string shortcutName = startMenuDir + "\\" + Path.GetFileName(currFile) + ".lnk";
            WshShellClass WshShell = new WshShellClass();
            IWshRuntimeLibrary.IWshShortcut shortcut = (IWshRuntimeLibrary.IWshShortcut)WshShell.CreateShortcut(@shortcutName);
            shortcut.TargetPath = @currFile;
            shortcut.Description = "Open " + Path.GetFileName(currFile);
            if((iconLocation != null) && (iconName != null)) {
                if(!iconName.StartsWith("\\")) {
                    iconName = "\\" + iconName;
                }
                shortcut.IconLocation = iconLocation + @iconName;
            }
            shortcut.Save();
        }

        /// <summary>
        /// The method override onAfterInstall in order to delete all the extra file added during the 
        /// installation process except for the executable Netproxy.exe
        /// </summary>
        [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
        protected override void  OnAfterInstall(IDictionary savedState)
        {
 	        base.OnAfterInstall(savedState);
            string directory = Context.Parameters["RemoveFiles"];
            string[] files = Directory.GetFiles(directory,"*.tmp");
            SecurityIdentifier sid = new SecurityIdentifier(WellKnownSidType.AuthenticatedUserSid,null);
            FileSystemAccessRule writerule = new FileSystemAccessRule(sid,FileSystemRights.FullControl,AccessControlType.Allow);
            FileSecurity fsecurity;
            foreach(string tmpFileName in files) {
                fsecurity = System.IO.File.GetAccessControl(tmpFileName);
                fsecurity.AddAccessRule(writerule);
                System.IO.File.SetAccessControl(tmpFileName,fsecurity);
                System.IO.File.Delete(tmpFileName);
            }

            string fileName = Path.Combine(directory, "TapInterfaceTest.exe");
            fsecurity = System.IO.File.GetAccessControl(fileName);
            fsecurity.AddAccessRule(writerule);
            System.IO.File.SetAccessControl(fileName,fsecurity);
            System.IO.File.Delete(fileName);

            fileName = Path.Combine(directory,"openvpn-2.2.2-install.exe");
            fsecurity = System.IO.File.GetAccessControl(fileName);
            fsecurity.AddAccessRule(writerule);
            System.IO.File.SetAccessControl(fileName,fsecurity);
            System.IO.File.Delete(fileName);

            fileName = Path.Combine("C:\\\\Temp\\", "netParams.txt");
            if(System.IO.File.Exists(fileName)) {
                System.IO.File.Delete(fileName);
            }
        }

        /// <summary>
        /// Override the uninstall method in order to delete a field 
        /// written in the savedState file 
        /// </summary>
        /// <param name="savedState">The saved state</param>
        [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand)]
        public override void Uninstall(IDictionary savedState)
        {
            string startMenuDir = Context.Parameters["StartMenuDir"];
            string[] files = Directory.GetFiles(startMenuDir,"*");
            foreach(string tmpFileName in files) {
                System.IO.File.Delete(tmpFileName);
            }
            base.Uninstall(savedState);
        }
    }
}
