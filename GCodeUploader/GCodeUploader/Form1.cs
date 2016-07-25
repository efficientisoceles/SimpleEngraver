using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Text.RegularExpressions;
using System.IO.Ports;
using System.Diagnostics;
using System.Threading;
using System.Diagnostics;
using System.Collections;

namespace GCodeUploader
{
	public partial class Form1: Form
	{
        //Selected GCode File to write
        String selectedFileName = "";
        public SerialPort selectedPort = new SerialPort("<<None>>", 9600); //Arduino Port

        
		public Form1()
		{
			InitializeComponent();
            progressBox.Text = "Waiting for user to start upload...";
		}

        private void toolStripMenuItem1_Click(object sender, EventArgs e)
        {

        }

        //Begin parsing GCode
        private void button1_Click(object sender, EventArgs e)
        {
            button1.Enabled = false;
            if (selectedFileName == "")
            {
                MessageBox.Show("Please select a GCode file first!");
                printLine("Upload Aborted: No GCode file selected");
                button1.Enabled = true;
                return;
            }
            printLine("----------New print started!----------");
            printLine("Opening port...");
            try
            {
                printLine(selectedPort.PortName);
                selectedPort.Open(); //Start comms with the Arduino
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error opening port!");
                printLine("Upload Aborted: Invalid Port Selected");
                printLine(ex.Message);
                button1.Enabled = true;
                return;
            }
            printLine("Port Opened, writing now...");
            //Query Arduino to show that a transmission is coming
            
            uploadWorker.RunWorkerAsync(); //Start upload
        }

        //Helper method to update the textbox with a new line
        public void printLine(String toPrint)
        {
            //List<String> newArray = progressBox.Lines.ToList<String>(); //Generate a new array of strings including the new string to be appended from the old array. The list is needed to append the new string
            //newArray.Add(toPrint); //append
            //progressBox.Lines = newArray.ToArray(); //Set the text box to use this new array
            progressBox.Text += toPrint + "\r\n";
            progressBox.SelectionStart = progressBox.Text.Length;
            progressBox.ScrollToCaret();
        }

        //Update the last line in the textbox
        private void updateLine(String toUpdate)
        {
            progressBox.Lines[progressBox.Lines.Length-1] = toUpdate;
        }

        private void openGCodeToolStripMenuItem_Click(object sender, EventArgs e)
        {

            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                selectedFileName = openFileDialog1.FileName; //Sets the name of the file to be uploaded
                label1.Text = ("Current GCode File:" + selectedFileName); //Update Label
                printLine("Opened new GCode File: " + selectedFileName);
            }
        }

        private void openFileDialog1_FileOk(object sender, CancelEventArgs e)
        {

        }

        private void uploadWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            StreamReader codeInput = new StreamReader(selectedFileName); //Open the GCode file to be read
            ArrayList buffer = new ArrayList(); //Input buffer

            int lines = 0;

            String currentLine = "";
            //First wait for a "Ready" signal from the CNC
            selectedPort.WriteLine("Beginning GCode");
            while (selectedPort.BytesToRead == 0 && serialRead() != "Ready for GCODE Input")
            {
            }
            selectedPort.WriteLine(" ");
            

            while (!codeInput.EndOfStream)
            {
                
                selectedPort.DiscardOutBuffer(); //Clear the buffer
                currentLine = codeInput.ReadLine(); //Take in the current line of input
                //Remove all text contained within brackets
                //The regex code highlights brackets and the text within
                Regex replacer = new Regex("\\(.*\\)");
                String newLine = replacer.Replace(currentLine, ""); //Remove those unneeded bracketed comments

                //Check if line is blank. If not, there must be gcode inside
                if (newLine != "")
                {
                    lines++;
                    //First show the user the current command being executed
                    progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] { "Writing current line: " + newLine });
                    Debug.WriteLine("Sent: " + newLine);
                    //Then push the GCode to the port. The Arduino will parse it from there
                    serialWrite(newLine);
                    //Wait for the command to send
                    
                    //Stopwatch in the case of a timeout
                    Stopwatch timer = new Stopwatch();
                    timer.Start(); //Begin measuring
                    int runs = 0;
                    while (true)
                    {
                        runs++;
                        //Debug.WriteLine("Runs: " + runs);
                        String inputLine;
                        ArrayList inputLines; //Arraylist to hold all available incoming lines
                        //Check if theres bytes to read
                        if ((inputLines = serialReadAll(buffer)).Count != 0 || selectedPort.BytesToRead > 0)
                        {
                            //Debug.WriteLine("Buffer Size: " + selectedPort.BytesToRead);

                            bool toBreak = false; //Flag variable showing whether to break the outer loop
                            for (int i = 0; i < inputLines.Count; i++) //Iterate through all incoming lines
                            {
                                inputLine = (String) inputLines[i]; //Take the string from the ArrayList

                                progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] { "Response: " + inputLine }); //Display the text to the user
                                Debug.WriteLine("Recieved: " + inputLine);

                                //If it is a valid return text, allow the next line of GCode to be uploaded
                                if (inputLine.Contains(("Executed: " + newLine).Substring(0,Math.Min(newLine.Length, 63))))
                                    toBreak = true; //Break the buffer-checking loop
                                //If it happens to be an "Acknowledged" code, then stop the timer as the CNC will be working (Cancelling the timeout)
                                else if (inputLine.Contains("Acknowledged"))
                                {
                                    //progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] { "Recieved 'STOP' " }); //Display the text to the user
                                    timer.Stop();
                                }
                            }
                            if (toBreak)
                                break;
                        }
                        else
                        {
                            //Debug.WriteLine("Nothing this run");
                            //If the command ends up timing out
                            if (timer.ElapsedMilliseconds > 5000 && timer.IsRunning == true)
                            {
                                progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] {"Upload timed out. Is your CNC properly connected?" }); //Display the error message
                                uploadWorker.CancelAsync(); //Cancel the upload
                                button1.Invoke(new enableDelegate(this.enableButton)); //Re-enable the upload button
                                selectedPort.Close(); //Close the serial port so that it can re-connect later
                                return;
                            }
                        }
                    }
                    //progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] { "Exited Loop!" }); //Display the text to the user
                }
            }
             //Read lines from the GCode file until empty

            //Tell the user the upload has been completed
            progressBox.Invoke(new displayDelegate(this.displayProgress), new object[] {"Upload Complete!"});
            button1.Invoke(new enableDelegate(this.enableButton));
            selectedPort.Close(); //Close the serial port so that it can re-connect later
        }

        //Custom serial read. Waits for all the input to come in using a timer, then reads until it encounters a newline character
        private String serialRead()
        {
            Stopwatch loadTime = new Stopwatch(); //Timer to delay a bit
            loadTime.Start();
            char currentChar;
            String output = "";
            while (loadTime.ElapsedMilliseconds < 100)
            {
            } //100ms delay to fill the buffer
            while (true) //Read up to a newline, continuously
            {
                if (selectedPort.BytesToRead > 0)
                {
                    currentChar = (char) selectedPort.ReadByte();
                    if (currentChar == '\n')
                        break;
                    output += currentChar;
                }
            }
            return (output);
        }

        //Custom serial read that reads everything in the buffer, continuously, until it reads something that does not end in a newline.
        //Returns an array of the lines that were read
        //It reads until there is not a newline as everything returned by the arduino uses a newline as the end character
        private ArrayList serialReadAll(ArrayList textBuffer)
        {
            Debug.WriteLine("Reading all");
            StreamReader serialStream = new StreamReader(selectedPort.BaseStream); //Create a streamreader to read from the serial port
            //First read in everything in the input buffer until the end, add it to the text buffer
            textBuffer.AddRange(selectedPort.ReadExisting().ToArray());
            int end = 0; //Last known newline
            ArrayList outputs = new ArrayList(); //Output arraylist containing lines that were found in the buffer

            Debug.WriteLine("Length: " + textBuffer.Count);

            //Some stuff to test
            for (int i = 0; i < textBuffer.Count; i++)
            {
                Debug.Write(textBuffer[i]);
            }

            //Now check if it's possible to form a line with the text so far
            while ((end = textBuffer.IndexOf('\n')) != -1) //Find the last known occurance of the newline character
            {
                Debug.WriteLine("Newline at " + end);
                String curLine = ""; //String to add the characters in the buffer to
                for (int i = 0; i < end; i++)
                {
                    curLine += textBuffer[i]; //Append characters to the string until the newline
                }
                outputs.Add(curLine); //Add the current line
                textBuffer.RemoveRange(0, end + 1); //Remove all characters up to the newline
            }
            return (outputs);
        }

        //Custom Serial Writer. Writes it one byte at a time and appends a new line at the end
        private void serialWrite(String toWrite)
        {
            selectedPort.Write(toWrite + "\n");
            while (selectedPort.BytesToWrite > 0)
            {
            }
        }


        //Custom serial clear. CLears the buffer
        private void serialClear()
        {
            Stopwatch clearTime = new Stopwatch();
            clearTime.Start();
            while (clearTime.ElapsedMilliseconds < 500)
            {
                selectedPort.ReadExisting();
            }
        }

        //Show progress thus far in the upload
        private void displayProgress(String toDisplay)
        {
            printLine(toDisplay);
        }

        //Delegate needed as we're updating from a seperate thread
        public delegate void displayDelegate(String toPrint);

        //Method to re-enable pressing of the upload button
        private void enableButton()
        {
            button1.Enabled = true;
        }

        //Delegate for enabling from a seperate thread
        public delegate void enableDelegate();

        //Update the currently selected port
        private void portsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            currentPortToolStripMenuItem.Text = selectedPort.PortName;
        }

        //Opens the select port dialog
        private void selectNewPortToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SelectPort selector = new SelectPort(this);
            selector.Show();
        }
	}
}
