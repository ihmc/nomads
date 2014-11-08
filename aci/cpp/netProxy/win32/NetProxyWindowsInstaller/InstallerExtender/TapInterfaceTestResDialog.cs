using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Installer {
    public partial class TapInterfaceTestResDialog:Form {
        public TapInterfaceTestResDialog(string tapTestResultMessage, bool failedTest, bool showOpenVpnCheckBox)
        {
            InitializeComponent(tapTestResultMessage, failedTest, showOpenVpnCheckBox);
        }

        public bool installOpenVpn()
        {
            return _chBoxOpenVpn.Checked;
        }
    }
}
