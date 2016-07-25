using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;

namespace GCodeUploader
{
    public partial class SelectPort : Form
    {
        String[] availablePorts; //Ports that exist on this computer, to be loaded later
        SerialPort creatorPort; //Port from the creator of this form to be updated once the user selects a port
        BindingList<String> ports = new BindingList<string>(); //List of ports that are displayed
        Form1 parent;

        public SelectPort (Form1 parent)
        {
            InitializeComponent();
            portsList.DataSource = ports; //Make it display the array of ports
            refreshPorts(); //Show all available ports
            portsList.SelectedIndex = 0; //Autoselect the first port entry (Or "No Ports found!" if ports are unavailable)
            this.parent = parent;
        }

        //When the okay button is clicked, set the new port
        private void okayButton_Click(object sender, EventArgs e)
        {
            String portName = ports[portsList.SelectedIndex]; //Get the user-selected port
            parent.printLine("Selected Port: " + portName);
            if (portName != null && portName != "No Ports found!")
            {
                parent.selectedPort = new SerialPort(portName, 9600); //Set the new port if the user has selected one
            }
            this.Close(); //Close the form
        }

        //Refresh the listing of available ports
        private void refreshPorts()
        {
            ports.Clear(); //CLear the list of ports to ready for new ports
            availablePorts = SerialPort.GetPortNames(); //Get the list of existant ports]
            if (availablePorts.Length == 0)
            {
                ports.Add("No Ports found!");
            }
            else
            {
                for (int i = 0; i < availablePorts.Length; i++)
                {
                    ports.Add(availablePorts[i]); //Add the ports to the current list
                }
            }
        }

        private void refreshPortsButton_Click(object sender, EventArgs e)
        {
            refreshPorts(); //Self explainatory
        }

        private void SelectPort_Load(object sender, EventArgs e)
        {

        }
    }
}
