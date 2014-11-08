namespace Installer 
{
    /// <summary>
    /// Class that shows the tap interface test result in a dialog box.
    /// </summary>
    partial class TapInterfaceTestResDialog 
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if(disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        /// <param name="tapTestResultMessage">tap interface test result message to display in the dialog box.</param>
        /// <param name="failedTest">true if the tap interface test failed; false otherwise.</param>
        private void InitializeComponent(string tapTestResultMessage, bool failedTest, bool showOpenVpnCheckBox)
        {
            this._iconSuccessFail = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this._iconSuccessFail)).BeginInit();
            this.SuspendLayout();

            // _iconSuccessFail 
            this._iconSuccessFail.Enabled = false;
            if (!failedTest) {
                this._iconSuccessFail.Image = global::Installer.Properties.Resources.Okicon;
            }
            else {
                this._iconSuccessFail.Image = global::Installer.Properties.Resources.SignErroricon;
            }
            this._iconSuccessFail.Location = new System.Drawing.Point(20,30);
            this._iconSuccessFail.MaximumSize = new System.Drawing.Size(40,40);
            this._iconSuccessFail.Name = "iconSuccessFail";
            this._iconSuccessFail.Size = new System.Drawing.Size(40,40);
            this._iconSuccessFail.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this._iconSuccessFail.TabIndex = 0;
            this._iconSuccessFail.TabStop = false;

            this._btnNext = new System.Windows.Forms.Button();

            // _btnNext
            this._btnNext.Font = new System.Drawing.Font("Arial",8.25F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._btnNext.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._btnNext.Location = new System.Drawing.Point(_xWinSize - 95,_yWinSize - 35);
            this._btnNext.Name = "buttonNext";
            this._btnNext.Size = new System.Drawing.Size(75,23);
            this._btnNext.TabIndex = 0;
            this._btnNext.Text = "Next";
            this._btnNext.UseVisualStyleBackColor = true;
            this._btnNext.Visible = true;

            this._lblTestResultMessage = new System.Windows.Forms.Label();

            // _lblTestResultMessage
            this._lblTestResultMessage.Font = new System.Drawing.Font("Arial",8.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblTestResultMessage.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._lblTestResultMessage.BackColor = System.Drawing.SystemColors.Menu;
            this._lblTestResultMessage.Location = new System.Drawing.Point(75,30);
            this._lblTestResultMessage.Name = "resultMessageDialog";
            this._lblTestResultMessage.Size = new System.Drawing.Size(224,110);
            this._lblTestResultMessage.TabIndex = 0;
            this._lblTestResultMessage.Text = tapTestResultMessage;
            this._lblTestResultMessage.Enabled = false;
            this._lblTestResultMessage.Visible = true;

            if(showOpenVpnCheckBox) {
                this._chBoxOpenVpn = new System.Windows.Forms.CheckBox();
                this.SuspendLayout();
                // _chBoxOpenVpn 
                this._chBoxOpenVpn.AutoSize = true;
                this._chBoxOpenVpn.Checked = true;
                this._chBoxOpenVpn.CheckState = System.Windows.Forms.CheckState.Checked;
                this._chBoxOpenVpn.Location = new System.Drawing.Point(75,143);
                this._chBoxOpenVpn.Name = "chBoxOpenVpn";
                this._chBoxOpenVpn.Size = new System.Drawing.Size(150,17);
                this._chBoxOpenVpn.TabIndex = 0;
                this._chBoxOpenVpn.Text = "Install openvpn-2.2.2";
                this._chBoxOpenVpn.UseVisualStyleBackColor = true;
                this._chBoxOpenVpn.Visible = true;
            }
            
            // TapInterfaceTestResDialog 
            this.Controls.Add(this._btnNext);
            this.Controls.Add(this._lblTestResultMessage);
            this.Controls.Add(this._iconSuccessFail);
            if(showOpenVpnCheckBox) {
                this.Controls.Add(this._chBoxOpenVpn);
            }
            this.Name = "TapInterfaceTestResDialog";
            this.Text = "TAP Interface Test Result";
            ((System.ComponentModel.ISupportInitialize)(this._iconSuccessFail)).EndInit();
            this.ClientSize = new System.Drawing.Size(_xWinSize,_yWinSize);
            this.CenterToScreen();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private System.Windows.Forms.Button _btnNext;
        private System.Windows.Forms.Label _lblTestResultMessage;
        private System.Windows.Forms.PictureBox _iconSuccessFail;
        private System.Windows.Forms.CheckBox _chBoxOpenVpn;
        private int _xWinSize = 310;
        private int _yWinSize = 220;
    }
}