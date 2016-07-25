SimpleEngraver V0.8

This is a simple arduino-based G-Code interpreter with a basic GUI component to send individual lines
of GCode from a .ngc file to said Arduino. At present, the interpreter is capable of executing all common
"G" commands. The Arduino sketch is placed within GCode_Float while the Visual C# GUI component is within GCode Uploader.

The Arduino sketch was implemented for use with an Arduino Mega connected to 2 EasyDriver stepper
motor drivers with a high-power laser diode controlled using a transistor switch.

Note that this project is still very much a work in progress. Complete functionality
and accuracy of the Arduino sketch cannot yet be confirmed as the previous laser I was
to test this with got burnt out. Additionally, M commands have not yet been implemented.