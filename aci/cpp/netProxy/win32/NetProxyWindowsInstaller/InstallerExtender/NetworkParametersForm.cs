using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Installer {
    /// <summary>
    /// Create a dialog form where the ip, subnet mask, gateway and 
    /// mac addresses can be set. 
    /// </summary>
    public partial class NetworkParametersForm:Form {
        /// <summary>
        /// Constructor that initialize the components.
        /// </summary>
        public NetworkParametersForm()
        {
            InitializeComponent();
        }

        public void IpIsertedOntxtIPThird(object sender,EventArgs e)
        {
            if(String.IsNullOrEmpty(_txtIPThird.Text)) {
                _txtMacFifth.Text = "";
                return;
            }
            string ipValue = _txtIPThird.Text;
            int value = Convert.ToInt32(ipValue);
            string hexOutput = String.Format("{0:X}",value);
            if(hexOutput.Length == 1) {
                hexOutput = "0" + hexOutput;
            }
            _txtMacFifth.Text = hexOutput;
        }

        public void IpIsertedOntxtIPFourth(object sender,EventArgs e)
        {
            if(String.IsNullOrEmpty(_txtIPFourth.Text)) {
                _txtMacSixth.Text = "";
                return;
            }
            string ipValue = _txtIPFourth.Text;
            int value = Convert.ToInt32(ipValue);
            string hexOutput = String.Format("{0:X}",value);
            if(hexOutput.Length == 1) {
                hexOutput = "0" + hexOutput;
            }
            _txtMacSixth.Text = hexOutput;
        }

        /// <summary>
        /// Set the network adapter name in the form
        /// </summary>
        /// <param name="adapter">Adapter name</param>
        public void setAdapterDescriptor(string adapter)
        {
            _txtAdapter.Paste(adapter);
        }

        /// <summary>
        /// Set the ip address in the form 
        /// </summary>
        /// <param name="ip">Ip address</param>
        public void setIPAddress(string ip)
        {
            string[] ipBytes = ip.Split('.');
            if(ipBytes.Length == 4) {
                _txtIPFirst.Paste(ipBytes[0]);
                _txtIPSecond.Paste(ipBytes[1]);
                _txtIPThird.Paste(ipBytes[2]);
                _txtIPFourth.Paste(ipBytes[3]);
            }
        }

        /// <summary>
        /// Set the subnet mask address in the form 
        /// </summary>
        /// <param name="ip">Subnet mask address</param>
        public void setMaskAddress(string mask)
        {
            string[] ipBytes = mask.Split('.');
            if(ipBytes.Length == 4) {
                _txtMaskFirst.Paste(ipBytes[0]);
                _txtMaskSecond.Paste(ipBytes[1]);
                _txtMaskThird.Paste(ipBytes[2]);
                _txtMaskFourth.Paste(ipBytes[3]);
            }
        }

        /// <summary>
        /// Set the default gateway mask address in the form 
        /// </summary>
        /// <param name="ip">Default gateway address</param>
        public void setGatewayAddress(string gateway)
        {
            string[] ipBytes = gateway.Split('.');
            if(ipBytes.Length == 4) {
                _txtGatewayFirst.Paste(ipBytes[0]);
                _txtGatewaySecond.Paste(ipBytes[1]);
                _txtGatewayThird.Paste(ipBytes[2]);
                _txtGatewayFourth.Paste(ipBytes[3]);
            }
        }

        /// <summary>
        /// Set the MAC address in the form 
        /// </summary>
        /// <param name="ip">MAC address</param>
        public void setMACAddress(string mac)
        {
            string[] tmpMacBytes = mac.Split(':');
            if(tmpMacBytes.Length == 6) {
                string fifthByte, sixthByte;

                if (tmpMacBytes[4].Length == 1) {
                    fifthByte = "0" + tmpMacBytes[4];
                }
                else {
                    fifthByte = tmpMacBytes[4];
                }
                _txtMacFifth.Paste(fifthByte);

                if(tmpMacBytes[5].Length == 1) {
                    sixthByte = "0" + tmpMacBytes[5];
                }
                else {
                    sixthByte = tmpMacBytes[5];
                }
                _txtMacSixth.Paste(sixthByte);
            }
        }

        /// <summary>
        /// Get the network adapter name
        /// </summary>
        /// <returns>Adapter name</returns>
        public string getAdapterDescriptor()
        {
            return _txtAdapter.Text;
        }

        /// <summary>
        /// Get the ip address
        /// </summary>
        /// <returns>Ip address</returns>
        public string getIPAddress()
        {
            string ip = _txtIPFirst.Text + "." + _txtIPSecond.Text + "." + 
                        _txtIPThird.Text + "." + _txtIPFourth.Text;
            return ip;
        }

        /// <summary>
        /// Get the subnet mask address
        /// </summary>
        /// <returns>Subnet mask address</returns>
        public string getMaskAddress()
        {
            string mask = _txtMaskFirst.Text + "." + _txtMaskSecond.Text + "." + 
                          _txtMaskThird.Text + "." + _txtMaskFourth.Text;
            return mask;
        }

        /// <summary>
        /// Get the default gateway address
        /// </summary>
        /// <returns>Default gateway address</returns>
        public string getGatewayAddress()
        {
            if ((String.IsNullOrEmpty(_txtGatewayFirst.Text)) ||
                (String.IsNullOrEmpty(_txtGatewaySecond.Text)) ||
                (String.IsNullOrEmpty(_txtGatewayThird.Text)) ||
                (String.IsNullOrEmpty(_txtGatewayFourth.Text))) {
                return null;
            }
            string gateway = _txtGatewayFirst.Text + "." + _txtGatewaySecond.Text + "." + 
                             _txtGatewayThird.Text + "." + _txtGatewayFourth.Text;
            return gateway;
        }

        /// <summary>
        /// Get the MAC address
        /// </summary>
        /// <returns>MAC address</returns>
        public string getMACAddress()
        {
            string mac = _txtMacFirst.Text + ":" + _txtMacSecond.Text + ":" + 
                         _txtMacThird.Text + ":" + _txtMacFourth.Text + ":" + 
                         _txtMacFifth.Text + ":" + _txtMacSixth.Text;
            return mac;
        }
    }
}
