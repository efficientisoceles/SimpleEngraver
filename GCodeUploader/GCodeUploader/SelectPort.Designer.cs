namespace GCodeUploader
{
    partial class SelectPort
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
            this.portsList = new System.Windows.Forms.ListBox();
            this.okayButton = new System.Windows.Forms.Button();
            this.refreshPortsButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // portsList
            // 
            this.portsList.FormattingEnabled = true;
            this.portsList.Location = new System.Drawing.Point(13, 13);
            this.portsList.Name = "portsList";
            this.portsList.Size = new System.Drawing.Size(259, 95);
            this.portsList.TabIndex = 0;
            // 
            // okayButton
            // 
            this.okayButton.Location = new System.Drawing.Point(13, 115);
            this.okayButton.Name = "okayButton";
            this.okayButton.Size = new System.Drawing.Size(118, 23);
            this.okayButton.TabIndex = 1;
            this.okayButton.Text = "OK";
            this.okayButton.UseVisualStyleBackColor = true;
            this.okayButton.Click += new System.EventHandler(this.okayButton_Click);
            // 
            // refreshPortsButton
            // 
            this.refreshPortsButton.Location = new System.Drawing.Point(154, 114);
            this.refreshPortsButton.Name = "refreshPortsButton";
            this.refreshPortsButton.Size = new System.Drawing.Size(118, 23);
            this.refreshPortsButton.TabIndex = 2;
            this.refreshPortsButton.Text = "Refresh";
            this.refreshPortsButton.UseVisualStyleBackColor = true;
            this.refreshPortsButton.Click += new System.EventHandler(this.refreshPortsButton_Click);
            // 
            // SelectPort
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 162);
            this.Controls.Add(this.refreshPortsButton);
            this.Controls.Add(this.okayButton);
            this.Controls.Add(this.portsList);
            this.Name = "SelectPort";
            this.Text = "Select A Port...";
            this.Load += new System.EventHandler(this.SelectPort_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListBox portsList;
        private System.Windows.Forms.Button okayButton;
        private System.Windows.Forms.Button refreshPortsButton;
    }
}