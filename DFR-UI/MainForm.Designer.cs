
namespace DFR_UI
{
    partial class MainForm
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
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.labelMode = new System.Windows.Forms.Label();
            this.frOff = new System.Windows.Forms.RadioButton();
            this.frMaximum = new System.Windows.Forms.RadioButton();
            this.frBalanced = new System.Windows.Forms.RadioButton();
            this.frMinimum = new System.Windows.Forms.RadioButton();
            this.frDebug = new System.Windows.Forms.RadioButton();
            this.appLabel = new System.Windows.Forms.Label();
            this.reattach = new System.Windows.Forms.Button();
            this.log = new System.Windows.Forms.TextBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.tableLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.log, 0, 2);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel2, 0, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(2);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 50F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(533, 292);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.appLabel);
            this.flowLayoutPanel1.Controls.Add(this.reattach);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(2, 2);
            this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(529, 46);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Controls.Add(this.labelMode);
            this.flowLayoutPanel2.Controls.Add(this.frOff);
            this.flowLayoutPanel2.Controls.Add(this.frMaximum);
            this.flowLayoutPanel2.Controls.Add(this.frBalanced);
            this.flowLayoutPanel2.Controls.Add(this.frMinimum);
            this.flowLayoutPanel2.Controls.Add(this.frDebug);
            this.flowLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(2, 52);
            this.flowLayoutPanel2.Margin = new System.Windows.Forms.Padding(2);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(529, 117);
            this.flowLayoutPanel2.TabIndex = 1;
            // 
            // labelMode
            // 
            this.labelMode.AutoSize = true;
            this.labelMode.Enabled = false;
            this.flowLayoutPanel2.SetFlowBreak(this.labelMode, true);
            this.labelMode.Location = new System.Drawing.Point(2, 4);
            this.labelMode.Margin = new System.Windows.Forms.Padding(2, 4, 2, 0);
            this.labelMode.Name = "labelMode";
            this.labelMode.Padding = new System.Windows.Forms.Padding(0, 0, 0, 6);
            this.labelMode.Size = new System.Drawing.Size(136, 19);
            this.labelMode.TabIndex = 0;
            this.labelMode.Text = "Foveated Rendering mode:";
            // 
            // frOff
            // 
            this.frOff.Enabled = false;
            this.frOff.Location = new System.Drawing.Point(10, 25);
            this.frOff.Margin = new System.Windows.Forms.Padding(10, 2, 2, 2);
            this.frOff.Name = "frOff";
            this.frOff.Size = new System.Drawing.Size(74, 16);
            this.frOff.TabIndex = 1;
            this.frOff.TabStop = true;
            this.frOff.Text = "Off";
            this.frOff.UseVisualStyleBackColor = true;
            this.frOff.CheckedChanged += new System.EventHandler(this.frOff_CheckedChanged);
            // 
            // frMaximum
            // 
            this.frMaximum.Enabled = false;
            this.frMaximum.Location = new System.Drawing.Point(88, 25);
            this.frMaximum.Margin = new System.Windows.Forms.Padding(2);
            this.frMaximum.Name = "frMaximum";
            this.frMaximum.Size = new System.Drawing.Size(74, 16);
            this.frMaximum.TabIndex = 2;
            this.frMaximum.TabStop = true;
            this.frMaximum.Text = "Maximum";
            this.frMaximum.UseVisualStyleBackColor = true;
            this.frMaximum.CheckedChanged += new System.EventHandler(this.frMaximum_CheckedChanged);
            // 
            // frBalanced
            // 
            this.frBalanced.Enabled = false;
            this.frBalanced.Location = new System.Drawing.Point(166, 25);
            this.frBalanced.Margin = new System.Windows.Forms.Padding(2);
            this.frBalanced.Name = "frBalanced";
            this.frBalanced.Size = new System.Drawing.Size(74, 16);
            this.frBalanced.TabIndex = 3;
            this.frBalanced.TabStop = true;
            this.frBalanced.Text = "Balanced";
            this.frBalanced.UseVisualStyleBackColor = true;
            this.frBalanced.CheckedChanged += new System.EventHandler(this.frBalanced_CheckedChanged);
            // 
            // frMinimum
            // 
            this.frMinimum.Enabled = false;
            this.frMinimum.Location = new System.Drawing.Point(244, 25);
            this.frMinimum.Margin = new System.Windows.Forms.Padding(2);
            this.frMinimum.Name = "frMinimum";
            this.frMinimum.Size = new System.Drawing.Size(74, 16);
            this.frMinimum.TabIndex = 4;
            this.frMinimum.TabStop = true;
            this.frMinimum.Text = "Minimum";
            this.frMinimum.UseVisualStyleBackColor = true;
            this.frMinimum.CheckedChanged += new System.EventHandler(this.frMinimum_CheckedChanged);
            // 
            // frDebug
            // 
            this.frDebug.AutoSize = true;
            this.frDebug.Enabled = false;
            this.frDebug.Location = new System.Drawing.Point(322, 25);
            this.frDebug.Margin = new System.Windows.Forms.Padding(2);
            this.frDebug.Name = "frDebug";
            this.frDebug.Padding = new System.Windows.Forms.Padding(33, 0, 0, 0);
            this.frDebug.Size = new System.Drawing.Size(120, 17);
            this.frDebug.TabIndex = 5;
            this.frDebug.TabStop = true;
            this.frDebug.Text = "Debug Mode";
            this.frDebug.UseVisualStyleBackColor = true;
            this.frDebug.CheckedChanged += new System.EventHandler(this.frDebug_CheckedChanged);
            // 
            // appLabel
            // 
            this.appLabel.Location = new System.Drawing.Point(6, 3);
            this.appLabel.Margin = new System.Windows.Forms.Padding(6, 3, 3, 0);
            this.appLabel.Name = "appLabel";
            this.appLabel.Size = new System.Drawing.Size(300, 43);
            this.appLabel.TabIndex = 0;
            this.appLabel.Text = "Not connected to SteamVR";
            this.appLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // reattach
            // 
            this.reattach.Enabled = false;
            this.reattach.Location = new System.Drawing.Point(312, 3);
            this.reattach.Name = "reattach";
            this.reattach.Size = new System.Drawing.Size(130, 42);
            this.reattach.TabIndex = 1;
            this.reattach.Text = "Try Re-Attach";
            this.reattach.UseVisualStyleBackColor = true;
            this.reattach.Click += new System.EventHandler(this.reattach_Click);
            // 
            // log
            // 
            this.log.Dock = System.Windows.Forms.DockStyle.Fill;
            this.log.Location = new System.Drawing.Point(3, 174);
            this.log.Multiline = true;
            this.log.Name = "log";
            this.log.ReadOnly = true;
            this.log.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.log.Size = new System.Drawing.Size(527, 115);
            this.log.TabIndex = 2;
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Interval = 1000;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(533, 292);
            this.Controls.Add(this.tableLayoutPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.Text = "Foveated Rendering Utility";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private System.Windows.Forms.Label labelMode;
        private System.Windows.Forms.RadioButton frOff;
        private System.Windows.Forms.RadioButton frMaximum;
        private System.Windows.Forms.RadioButton frBalanced;
        private System.Windows.Forms.RadioButton frMinimum;
        private System.Windows.Forms.RadioButton frDebug;
        private System.Windows.Forms.Label appLabel;
        private System.Windows.Forms.Button reattach;
        private System.Windows.Forms.TextBox log;
        private System.Windows.Forms.Timer timer1;
    }
}

