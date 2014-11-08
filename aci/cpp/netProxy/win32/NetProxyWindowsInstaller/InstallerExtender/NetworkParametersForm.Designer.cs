namespace Installer {

    /// <summary>
    /// Class that creates a dialog form to change the network parameters
    /// </summary>
    partial class NetworkParametersForm {
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
        /// Instantiates the components
        /// </summary>
        private void InitializeComponent()
        {
            this._lblAdapter = new System.Windows.Forms.Label();
            this._lblIP = new System.Windows.Forms.Label();
            this._lblMask = new System.Windows.Forms.Label();
            this._lblGateway = new System.Windows.Forms.Label();
            this._lblGatewayNotRequired = new System.Windows.Forms.Label();
            this._lblMac = new System.Windows.Forms.Label();
            this._txtAdapter = new System.Windows.Forms.TextBox();
            this._txtIPFirst = new System.Windows.Forms.TextBox();
            this._txtIPSecond = new System.Windows.Forms.TextBox();
            this._lblIPDotFirst = new System.Windows.Forms.Label();
            this._lblIPDotThird = new System.Windows.Forms.Label();
            this._txtIPFourth = new System.Windows.Forms.TextBox();
            this._txtIPThird = new System.Windows.Forms.TextBox();
            this._lblIPDotSecond = new System.Windows.Forms.Label();
            this._lblMaskDotSecond = new System.Windows.Forms.Label();
            this._lblMaskDotThird = new System.Windows.Forms.Label();
            this._txtMaskFourth = new System.Windows.Forms.TextBox();
            this._txtMaskThird = new System.Windows.Forms.TextBox();
            this._lblMaskDotFirst = new System.Windows.Forms.Label();
            this._txtMaskSecond = new System.Windows.Forms.TextBox();
            this._txtMaskFirst = new System.Windows.Forms.TextBox();
            this._lblGatewayDotSecond = new System.Windows.Forms.Label();
            this._lblGatewayDotThird = new System.Windows.Forms.Label();
            this._txtGatewayFourth = new System.Windows.Forms.TextBox();
            this._txtGatewayThird = new System.Windows.Forms.TextBox();
            this._lblGatewayDotFirst = new System.Windows.Forms.Label();
            this._txtGatewaySecond = new System.Windows.Forms.TextBox();
            this._txtGatewayFirst = new System.Windows.Forms.TextBox();
            this._lblMacColonFirst = new System.Windows.Forms.Label();
            this._txtMacFirst = new System.Windows.Forms.TextBox();
            this._lblMacColonSecond = new System.Windows.Forms.Label();
            this._txtMacSecond = new System.Windows.Forms.TextBox();
            this._lblMacColonFourth = new System.Windows.Forms.Label();
            this._txtMacFourth = new System.Windows.Forms.TextBox();
            this._lblMacColonThird = new System.Windows.Forms.Label();
            this._txtMacThird = new System.Windows.Forms.TextBox();
            this._txtMacSixth = new System.Windows.Forms.TextBox();
            this._lblMacColonFifth = new System.Windows.Forms.Label();
            this._txtMacFifth = new System.Windows.Forms.TextBox();
            this._lblDescription = new System.Windows.Forms.Label();
            this._btnOk = new System.Windows.Forms.Button();
            this._btnCancel = new System.Windows.Forms.Button();
            this.SuspendLayout();
             
            // _lblAdapter
            this._lblAdapter.AutoSize = true;
            this._lblAdapter.Location = new System.Drawing.Point(27,57);
            this._lblAdapter.Name = "_lblAdapter";
            this._lblAdapter.Size = new System.Drawing.Size(118,13);
            this._lblAdapter.TabIndex = 0;
            this._lblAdapter.Text = "Network Adapter Name";
             
            // _lblIP 
            this._lblIP.AutoSize = true;
            this._lblIP.Location = new System.Drawing.Point(27,83);
            this._lblIP.Name = "_lblIP";
            this._lblIP.Size = new System.Drawing.Size(58,13);
            this._lblIP.TabIndex = 1;
            this._lblIP.Text = "IP Address";
             
            // _lblMask
            this._lblMask.AutoSize = true;
            this._lblMask.Location = new System.Drawing.Point(27,110);
            this._lblMask.Name = "_lblMask";
            this._lblMask.Size = new System.Drawing.Size(111,13);
            this._lblMask.TabIndex = 2;
            this._lblMask.Text = "Subnet Mask Address";
             
            // _lblGateway
            this._lblGateway.AutoSize = true;
            this._lblGateway.Location = new System.Drawing.Point(27,136);
            this._lblGateway.Name = "_lblGateway";
            this._lblGateway.Size = new System.Drawing.Size(127,13);
            this._lblGateway.TabIndex = 3;
            this._lblGateway.Text = "Default Gateway Address (*)";

            // _lblGatewayNotRequired
            this._lblGatewayNotRequired.AutoSize = true;
            this._lblGatewayNotRequired.Location = new System.Drawing.Point(27,190);
            this._lblGatewayNotRequired.Name = "_lblGatewayNotRequired";
            this._lblGatewayNotRequired.Size = new System.Drawing.Size(127,13);
            this._lblGatewayNotRequired.TabIndex = 3;
            this._lblGatewayNotRequired.Font = new System.Drawing.Font("Microsoft Sans Serif",8F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblGatewayNotRequired.Text = "* = Not required";
            
            // _lblMac
            this._lblMac.AutoSize = true;
            this._lblMac.Location = new System.Drawing.Point(27,162);
            this._lblMac.Name = "_lblMac";
            this._lblMac.Size = new System.Drawing.Size(71,13);
            this._lblMac.TabIndex = 4;
            this._lblMac.Text = "MAC Address";
             
            // _txtAdapter
            this._txtAdapter.Location = new System.Drawing.Point(180,54);
            this._txtAdapter.Name = "_txtAdapter";
            this._txtAdapter.Size = new System.Drawing.Size(256,20);
            this._txtAdapter.TabIndex = 5;
            
            // _txtIPFirst 
            this._txtIPFirst.Location = new System.Drawing.Point(180,80);
            this._txtIPFirst.Name = "_txtIPFirst";
            this._txtIPFirst.Size = new System.Drawing.Size(42,20);
            this._txtIPFirst.TabIndex = 6;
            this._txtIPFirst.MaxLength = 3;
            this._txtIPFirst.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            
            // _txtIPSecond
            this._txtIPSecond.Location = new System.Drawing.Point(233,80);
            this._txtIPSecond.Name = "_txtIPSecond";
            this._txtIPSecond.Size = new System.Drawing.Size(42,20);
            this._txtIPSecond.TabIndex = 7;
            this._txtIPSecond.MaxLength = 3;
            this._txtIPSecond.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _lblIPDotFirst
            this._lblIPDotFirst.AutoSize = true;
            this._lblIPDotFirst.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblIPDotFirst.Location = new System.Drawing.Point(223,78);
            this._lblIPDotFirst.Name = "_lblIPDotFirst";
            this._lblIPDotFirst.Size = new System.Drawing.Size(13,20);
            this._lblIPDotFirst.TabIndex = 10;
            this._lblIPDotFirst.Text = ".";
             
            // _lblIPDotThird
            this._lblIPDotThird.AutoSize = true;
            this._lblIPDotThird.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblIPDotThird.Location = new System.Drawing.Point(334,78);
            this._lblIPDotThird.Name = "_lblIPDotThird";
            this._lblIPDotThird.Size = new System.Drawing.Size(13,20);
            this._lblIPDotThird.TabIndex = 13;
            this._lblIPDotThird.Text = ".";
             
            // _txtIPFourth 
            this._txtIPFourth.Location = new System.Drawing.Point(344,80);
            this._txtIPFourth.Name = "_txtIPFourth";
            this._txtIPFourth.Size = new System.Drawing.Size(42,20);
            this._txtIPFourth.TabIndex = 12;
            this._txtIPFourth.MaxLength = 3;
            this._txtIPFourth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtIPFourth.TextChanged += new System.EventHandler(this.IpIsertedOntxtIPFourth);
            
            // _txtIPThird 
            this._txtIPThird.Location = new System.Drawing.Point(291,80);
            this._txtIPThird.Name = "_txtIPThird";
            this._txtIPThird.Size = new System.Drawing.Size(42,20);
            this._txtIPThird.TabIndex = 11;
            this._txtIPThird.MaxLength = 3;
            this._txtIPThird.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtIPThird.TextChanged += new System.EventHandler(this.IpIsertedOntxtIPThird);
             
            // _lblIPDotSecond 
            this._lblIPDotSecond.AutoSize = true;
            this._lblIPDotSecond.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblIPDotSecond.Location = new System.Drawing.Point(272,78);
            this._lblIPDotSecond.Name = "_lblIPDotSecond";
            this._lblIPDotSecond.Size = new System.Drawing.Size(13,20);
            this._lblIPDotSecond.TabIndex = 14;
            this._lblIPDotSecond.Text = ".";
             
            // _lblMaskDotSecond 
            this._lblMaskDotSecond.AutoSize = true;
            this._lblMaskDotSecond.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMaskDotSecond.Location = new System.Drawing.Point(272,101);
            this._lblMaskDotSecond.Name = "_lblMaskDotSecond";
            this._lblMaskDotSecond.Size = new System.Drawing.Size(13,20);
            this._lblMaskDotSecond.TabIndex = 21;
            this._lblMaskDotSecond.Text = ".";
             
            // _lblMaskDotThird 
            this._lblMaskDotThird.AutoSize = true;
            this._lblMaskDotThird.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMaskDotThird.Location = new System.Drawing.Point(334,101);
            this._lblMaskDotThird.Name = "_lblMaskDotThird";
            this._lblMaskDotThird.Size = new System.Drawing.Size(13,20);
            this._lblMaskDotThird.TabIndex = 20;
            this._lblMaskDotThird.Text = ".";
             
            // _txtMaskFourth 
            this._txtMaskFourth.Location = new System.Drawing.Point(344,103);
            this._txtMaskFourth.Name = "_txtMaskFourth";
            this._txtMaskFourth.Size = new System.Drawing.Size(42,20);
            this._txtMaskFourth.TabIndex = 19;
            this._txtMaskFourth.MaxLength = 3;
            this._txtMaskFourth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _txtMaskThird 
            this._txtMaskThird.Location = new System.Drawing.Point(291,103);
            this._txtMaskThird.Name = "_txtMaskThird";
            this._txtMaskThird.Size = new System.Drawing.Size(42,20);
            this._txtMaskThird.TabIndex = 18;
            this._txtMaskThird.MaxLength = 3;
            this._txtMaskThird.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _lblMaskDotFirst 
            this._lblMaskDotFirst.AutoSize = true;
            this._lblMaskDotFirst.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMaskDotFirst.Location = new System.Drawing.Point(223,101);
            this._lblMaskDotFirst.Name = "_lblMaskDotFirst";
            this._lblMaskDotFirst.Size = new System.Drawing.Size(13,20);
            this._lblMaskDotFirst.TabIndex = 17;
            this._lblMaskDotFirst.Text = ".";
             
            // _txtMaskSecond 
            this._txtMaskSecond.Location = new System.Drawing.Point(233,103);
            this._txtMaskSecond.Name = "_txtMaskSecond";
            this._txtMaskSecond.Size = new System.Drawing.Size(42,20);
            this._txtMaskSecond.TabIndex = 16;
            this._txtMaskSecond.MaxLength = 3;
            this._txtMaskSecond.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _txtMaskFirst 
            this._txtMaskFirst.Location = new System.Drawing.Point(180,103);
            this._txtMaskFirst.Name = "_txtMaskFirst";
            this._txtMaskFirst.Size = new System.Drawing.Size(42,20);
            this._txtMaskFirst.TabIndex = 15;
            this._txtMaskFirst.MaxLength = 3;
            this._txtMaskFirst.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _lblGatewayDotSecond 
            this._lblGatewayDotSecond.AutoSize = true;
            this._lblGatewayDotSecond.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblGatewayDotSecond.Location = new System.Drawing.Point(272,127);
            this._lblGatewayDotSecond.Name = "_lblGatewayDotSecond";
            this._lblGatewayDotSecond.Size = new System.Drawing.Size(13,20);
            this._lblGatewayDotSecond.TabIndex = 28;
            this._lblGatewayDotSecond.Text = ".";
             
            // _lblGatewayDotThird 
            this._lblGatewayDotThird.AutoSize = true;
            this._lblGatewayDotThird.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblGatewayDotThird.Location = new System.Drawing.Point(334,127);
            this._lblGatewayDotThird.Name = "_lblGatewayDotThird";
            this._lblGatewayDotThird.Size = new System.Drawing.Size(13,20);
            this._lblGatewayDotThird.TabIndex = 27;
            this._lblGatewayDotThird.Text = ".";
             
            // _txtGatewayFourth 
            this._txtGatewayFourth.Location = new System.Drawing.Point(344,129);
            this._txtGatewayFourth.Name = "_txtGatewayFourth";
            this._txtGatewayFourth.Size = new System.Drawing.Size(42,20);
            this._txtGatewayFourth.TabIndex = 26;
            this._txtGatewayFourth.MaxLength = 3;
            this._txtGatewayFourth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _txtGatewayThird 
            this._txtGatewayThird.Location = new System.Drawing.Point(291,129);
            this._txtGatewayThird.Name = "_txtGatewayThird";
            this._txtGatewayThird.Size = new System.Drawing.Size(42,20);
            this._txtGatewayThird.TabIndex = 25;
            this._txtGatewayThird.MaxLength = 3;
            this._txtGatewayThird.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _lblGatewayDotFirst 
            this._lblGatewayDotFirst.AutoSize = true;
            this._lblGatewayDotFirst.Font = new System.Drawing.Font("Microsoft Sans Serif",12F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblGatewayDotFirst.Location = new System.Drawing.Point(223,127);
            this._lblGatewayDotFirst.Name = "_lblGatewayDotFirst";
            this._lblGatewayDotFirst.Size = new System.Drawing.Size(13,20);
            this._lblGatewayDotFirst.TabIndex = 24;
            this._lblGatewayDotFirst.Text = ".";
            
            // _txtGatewaySecond
            this._txtGatewaySecond.Location = new System.Drawing.Point(233,129);
            this._txtGatewaySecond.Name = "_txtGatewaySecond";
            this._txtGatewaySecond.Size = new System.Drawing.Size(42,20);
            this._txtGatewaySecond.TabIndex = 23;
            this._txtGatewaySecond.MaxLength = 3;
            this._txtGatewaySecond.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _txtGatewayFirst 
            this._txtGatewayFirst.Location = new System.Drawing.Point(180,129);
            this._txtGatewayFirst.Name = "_txtGatewayFirst";
            this._txtGatewayFirst.Size = new System.Drawing.Size(42,20);
            this._txtGatewayFirst.TabIndex = 22;
            this._txtGatewayFirst.MaxLength = 3;
            this._txtGatewayFirst.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            
            // _lblMacColonFirst
            this._lblMacColonFirst.AutoSize = true;
            this._lblMacColonFirst.Font = new System.Drawing.Font("Microsoft Sans Serif",9.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMacColonFirst.Location = new System.Drawing.Point(209,155);
            this._lblMacColonFirst.Name = "_lblMacColonFirst";
            this._lblMacColonFirst.Size = new System.Drawing.Size(11,16);
            this._lblMacColonFirst.TabIndex = 35;
            this._lblMacColonFirst.Text = ":";
             
            // _txtMacFirst
            this._txtMacFirst.Location = new System.Drawing.Point(180,155);
            this._txtMacFirst.Name = "_txtMacFirst";
            this._txtMacFirst.Size = new System.Drawing.Size(26,20);
            this._txtMacFirst.TabIndex = 29;
            this._txtMacFirst.MaxLength = 2;
            this._txtMacFirst.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtMacFirst.Enabled = false;
            this._txtMacFirst.Paste("02");
             
            // _lblMacColonSecond
            this._lblMacColonSecond.AutoSize = true;
            this._lblMacColonSecond.Font = new System.Drawing.Font("Microsoft Sans Serif",9.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMacColonSecond.Location = new System.Drawing.Point(255,155);
            this._lblMacColonSecond.Name = "_lblMacColonSecond";
            this._lblMacColonSecond.Size = new System.Drawing.Size(11,16);
            this._lblMacColonSecond.TabIndex = 37;
            this._lblMacColonSecond.Text = ":";
            
            // _txtMacSecond
            this._txtMacSecond.Location = new System.Drawing.Point(226,155);
            this._txtMacSecond.Name = "_txtMacSecond";
            this._txtMacSecond.Size = new System.Drawing.Size(26,20);
            this._txtMacSecond.TabIndex = 36;
            this._txtMacSecond.MaxLength = 2;
            this._txtMacSecond.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtMacSecond.Enabled = false;
            this._txtMacSecond.Paste("0A");
            
            // _lblMacColonFourth
            this._lblMacColonFourth.AutoSize = true;
            this._lblMacColonFourth.Font = new System.Drawing.Font("Microsoft Sans Serif",9.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMacColonFourth.Location = new System.Drawing.Point(347,155);
            this._lblMacColonFourth.Name = "_lblMacColonFourth";
            this._lblMacColonFourth.Size = new System.Drawing.Size(11,16);
            this._lblMacColonFourth.TabIndex = 41;
            this._lblMacColonFourth.Text = ":";
            
            // _txtMacFourth 
            this._txtMacFourth.Location = new System.Drawing.Point(318,155);
            this._txtMacFourth.Name = "_txtMacFourth";
            this._txtMacFourth.Size = new System.Drawing.Size(26,20);
            this._txtMacFourth.TabIndex = 40;
            this._txtMacFourth.MaxLength = 2;
            this._txtMacFourth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtMacFourth.Enabled = false;
            this._txtMacFourth.Paste("00");
             
            // _lblMacColonThird 
            this._lblMacColonThird.AutoSize = true;
            this._lblMacColonThird.Font = new System.Drawing.Font("Microsoft Sans Serif",9.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMacColonThird.Location = new System.Drawing.Point(301,155);
            this._lblMacColonThird.Name = "_lblMacColonThird";
            this._lblMacColonThird.Size = new System.Drawing.Size(11,16);
            this._lblMacColonThird.TabIndex = 39;
            this._lblMacColonThird.Text = ":";
             
            // _txtMacThird
            this._txtMacThird.Location = new System.Drawing.Point(272,155);
            this._txtMacThird.Name = "_txtMacThird";
            this._txtMacThird.Size = new System.Drawing.Size(26,20);
            this._txtMacThird.TabIndex = 38;
            this._txtMacThird.MaxLength = 2;
            this._txtMacThird.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._txtMacThird.Enabled = false;
            this._txtMacThird.Paste("0C");
             
            // _txtMacSixth 
            this._txtMacSixth.Location = new System.Drawing.Point(410,155);
            this._txtMacSixth.Name = "_txtMacSixth";
            this._txtMacSixth.Size = new System.Drawing.Size(26,20);
            this._txtMacSixth.TabIndex = 44;
            this._txtMacSixth.MaxLength = 2;
            this._txtMacSixth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
             
            // _lblMacColonFifth
            this._lblMacColonFifth.AutoSize = true;
            this._lblMacColonFifth.Font = new System.Drawing.Font("Microsoft Sans Serif",9.75F,System.Drawing.FontStyle.Regular,System.Drawing.GraphicsUnit.Point,((byte)(0)));
            this._lblMacColonFifth.Location = new System.Drawing.Point(393,155);
            this._lblMacColonFifth.Name = "_lblMacColonFifth";
            this._lblMacColonFifth.Size = new System.Drawing.Size(11,16);
            this._lblMacColonFifth.TabIndex = 43;
            this._lblMacColonFifth.Text = ":";
             
            // _txtMacFifth
            this._txtMacFifth.Location = new System.Drawing.Point(364,155);
            this._txtMacFifth.Name = "_txtMacFifth";
            this._txtMacFifth.Size = new System.Drawing.Size(26,20);
            this._txtMacFifth.TabIndex = 42;
            this._txtMacFifth.MaxLength = 2;
            this._txtMacFifth.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;

            // _lblDescription 
            this._lblDescription.AutoSize = true;
            this._lblDescription.Location = new System.Drawing.Point(27,20);
            this._lblDescription.Name = "_lblDescription";
            this._lblDescription.Size = new System.Drawing.Size(137,13);
            this._lblDescription.TabIndex = 45;
            this._lblDescription.Text = "Set the network parameters";
             
            // _btnOk 
            this._btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._btnOk.Location = new System.Drawing.Point(413,221);
            this._btnOk.Name = "_btnOk";
            this._btnOk.Size = new System.Drawing.Size(87,26);
            this._btnOk.TabIndex = 46;
            this._btnOk.Text = "Ok";
            this._btnOk.UseVisualStyleBackColor = true;
             
            // _btnCancel
            this._btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._btnCancel.Location = new System.Drawing.Point(317,221);
            this._btnCancel.Name = "_btnCancel";
            this._btnCancel.Size = new System.Drawing.Size(87,26);
            this._btnCancel.TabIndex = 47;
            this._btnCancel.Text = "Cancel";
            this._btnCancel.UseVisualStyleBackColor = true;

            // NetworkParametersForm
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F,13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(512,259);
            this.Controls.Add(this._btnCancel);
            this.Controls.Add(this._btnOk);
            this.Controls.Add(this._lblDescription);
            this.Controls.Add(this._txtMacSixth);
            this.Controls.Add(this._lblMacColonFifth);
            this.Controls.Add(this._txtMacFifth);
            this.Controls.Add(this._lblMacColonFourth);
            this.Controls.Add(this._txtMacFourth);
            this.Controls.Add(this._lblMacColonThird);
            this.Controls.Add(this._txtMacThird);
            this.Controls.Add(this._lblMacColonSecond);
            this.Controls.Add(this._txtMacSecond);
            this.Controls.Add(this._lblMacColonFirst);
            this.Controls.Add(this._txtMacFirst);
            this.Controls.Add(this._lblGatewayDotSecond);
            this.Controls.Add(this._lblGatewayDotThird);
            this.Controls.Add(this._txtGatewayFourth);
            this.Controls.Add(this._txtGatewayThird);
            this.Controls.Add(this._lblGatewayDotFirst);
            this.Controls.Add(this._txtGatewaySecond);
            this.Controls.Add(this._txtGatewayFirst);
            this.Controls.Add(this._lblMaskDotSecond);
            this.Controls.Add(this._lblMaskDotThird);
            this.Controls.Add(this._txtMaskFourth);
            this.Controls.Add(this._txtMaskThird);
            this.Controls.Add(this._lblMaskDotFirst);
            this.Controls.Add(this._txtMaskSecond);
            this.Controls.Add(this._txtMaskFirst);
            this.Controls.Add(this._lblIPDotSecond);
            this.Controls.Add(this._lblIPDotThird);
            this.Controls.Add(this._txtIPFourth);
            this.Controls.Add(this._txtIPThird);
            this.Controls.Add(this._lblIPDotFirst);
            this.Controls.Add(this._txtIPSecond);
            this.Controls.Add(this._txtIPFirst);
            this.Controls.Add(this._txtAdapter);
            this.Controls.Add(this._lblMac);
            this.Controls.Add(this._lblGateway);
            this.Controls.Add(this._lblGatewayNotRequired);
            this.Controls.Add(this._lblMask);
            this.Controls.Add(this._lblIP);
            this.Controls.Add(this._lblAdapter);
            this.Name = "Network Parameters";
            this.Text = "Network Parameters";
            this.ResumeLayout(false);
            this.CenterToScreen();
            this.PerformLayout();

        }

        private System.Windows.Forms.Label _lblDescription;

        private System.Windows.Forms.Label _lblAdapter;
        private System.Windows.Forms.TextBox _txtAdapter;

        private System.Windows.Forms.Label _lblIP;
        private System.Windows.Forms.TextBox _txtIPFirst;
        private System.Windows.Forms.Label _lblIPDotFirst;
        private System.Windows.Forms.TextBox _txtIPSecond;
        private System.Windows.Forms.Label _lblIPDotSecond;
        private System.Windows.Forms.TextBox _txtIPThird;
        private System.Windows.Forms.Label _lblIPDotThird;
        private System.Windows.Forms.TextBox _txtIPFourth;
        
        private System.Windows.Forms.Label _lblMask;
        private System.Windows.Forms.TextBox _txtMaskFirst;
        private System.Windows.Forms.Label _lblMaskDotFirst;
        private System.Windows.Forms.TextBox _txtMaskSecond;
        private System.Windows.Forms.Label _lblMaskDotSecond;
        private System.Windows.Forms.TextBox _txtMaskThird;
        private System.Windows.Forms.Label _lblMaskDotThird;
        private System.Windows.Forms.TextBox _txtMaskFourth;

        private System.Windows.Forms.Label _lblGateway;
        private System.Windows.Forms.Label _lblGatewayNotRequired;
        private System.Windows.Forms.TextBox _txtGatewayFirst;
        private System.Windows.Forms.Label _lblGatewayDotFirst;
        private System.Windows.Forms.TextBox _txtGatewaySecond;
        private System.Windows.Forms.Label _lblGatewayDotSecond;
        private System.Windows.Forms.TextBox _txtGatewayThird;
        private System.Windows.Forms.Label _lblGatewayDotThird;
        private System.Windows.Forms.TextBox _txtGatewayFourth;

        private System.Windows.Forms.Label _lblMac;
        private System.Windows.Forms.TextBox _txtMacFirst;
        private System.Windows.Forms.Label _lblMacColonFirst;
        private System.Windows.Forms.TextBox _txtMacSecond;
        private System.Windows.Forms.Label _lblMacColonSecond;
        private System.Windows.Forms.TextBox _txtMacThird;
        private System.Windows.Forms.Label _lblMacColonThird;
        private System.Windows.Forms.TextBox _txtMacFourth;
        private System.Windows.Forms.Label _lblMacColonFourth;
        private System.Windows.Forms.TextBox _txtMacFifth;
        private System.Windows.Forms.Label _lblMacColonFifth;
        private System.Windows.Forms.TextBox _txtMacSixth;

        private System.Windows.Forms.Button _btnOk;
        private System.Windows.Forms.Button _btnCancel;
    }
}