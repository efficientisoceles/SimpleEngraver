// G-Code thing based on the EasyStepper Controller
//This version supports floating-point X and Y values
//It also supports the new uploader

//X Pins
#define X_STEP 53
#define X_DIR 51

//Y Pins
#define Y_STEP 52
#define Y_DIR 50

//Indicator LED
#define INDICATOR 13

//Microstepping Info.
/* Microstepping control
    MS1 MS2
    L   L    Full Step
    H   L    Half Step (Half the degrees of a full step)
    L   H    Quarter Step
    H   H    Eight Step
*/
#define MS1 48
#define MS2 50

//Place your relay/transistor activation pin for the spindle here
#define SPINDLE 6

//Set stepper properties here
#define STEPS_PER_MM 40 //MM and inches per step can be determined by measuring the number of steps it takes to get from one side of the rail to the other, and dividing the length of the rail by that number
#define STEPS_PER_INCH 1000
#define STEPS_PER_LENGTH 2000 //Steps that the motor makes in order to travel from one side of the platform to the other
#define MAX_SPEED 200 //This value depends on your stepper and arduino. The maximum speed, in steps/second, that the motor may travel.

#include <AccelStepper.h>
#include <MultiStepper.h>

// EG X-Y position bed driven by 2 steppers
// Alas its not possible to build an array of these with different pins for each :-(
AccelStepper xStep(AccelStepper::DRIVER, X_STEP, X_DIR);
AccelStepper yStep(AccelStepper::DRIVER, Y_STEP, Y_DIR);

//Stepper group
MultiStepper steppers;

boolean isRunning = false;
boolean inches = false;
boolean absolute = true;

//Positions are measured in steps
int xPosition = 0;
int yPosition = 0;
int originX = 0;
int originY = 0;

void setup() {
  Serial.begin(9600); //Serial to take in drawing commands and stuffs

  //Setup Pins
  pinMode(X_STEP, OUTPUT);
  pinMode(Y_STEP, OUTPUT);
  pinMode(X_DIR, OUTPUT);
  pinMode(Y_DIR, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(SPINDLE, OUTPUT);
  pinMode(INDICATOR, OUTPUT);

  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
  digitalWrite(SPINDLE, HIGH);


  // Configure each stepper
  xStep.setMaxSpeed(MAX_SPEED);
  yStep.setMaxSpeed(MAX_SPEED);

  // Then give them to MultiStepper to manage
  steppers.addStepper(xStep);
  steppers.addStepper(yStep);

  //Reset the positions of the steppers on start
  setLocation(STEPS_PER_LENGTH/STEPS_PER_MM, STEPS_PER_LENGTH/STEPS_PER_MM);
  setLocation(0, 0);

  Serial.begin(9600);
}

/* List of G-COmmands
    G00 - Rapid move (Just moving)
    G01 - Feed move (Writing and moving)
    G02 - Clockwise Circular Move (4 arguments MUST be supplied)
    G03 - Counterclockwise Move (Same as above)
    G04 - Dwell Time (Delay; keep the spindle in one position
    G20 - Sets units to INCHES
    G21 - Sets units to MM
    G28 - Return to 0,0
    G90 - Set positioning to absolute (Moves to coordinate points relative to the origin; Behaves like a position vector)
    G91 - Sets positioning to relative (Moves to an xyz distance away from the previous point; Basically behaves like a displacement vector)
    G92 - Set new position of 0,0. All commands in G91 will thus be performed relative to this new position.
*/
boolean executeGCommand(int id, String input)
{


  Serial.println("Executing commend: G" + id);

  float * values = NULL;

  switch (id)
  {
    case 00:
      values = getPosition(input, 2);
      setMotorSpeed(MAX_SPEED, MAX_SPEED);
      digitalWrite(SPINDLE, LOW);
      setLocation(values[0], values[1]);
      break;
    case 01:
      values = getPosition(input, 3);
      if (values[2] != 999999)
        setMotorSpeed((int) values[2], (int) values[2]);
      digitalWrite(SPINDLE, HIGH);
      setLocation(values[0], values[1]);
      break;
    case 02:
      values = getPosition(input, 5);
      if (values[2] != 999999)
        setMotorSpeed((int) values[2], (int) values[2]);
      circularMove(values[0], values[1], values[3], values[4], true);
      break;
    case 03:
      values = getPosition(input, 5);
      if (values[2] != 999999)
        setMotorSpeed((int) values[2], (int) values[2]);
      circularMove(values[0], values[1], values[3], values[4], false);
      break;
    case 04:
      values = getPosition(input, 7);
      delay(values[0]);
      break;
    case 20:
      inches = true;
      break;
    case 21:
      inches = false;
      Serial.println("Exec 21");
      break;
    case 28:
      setMotorSpeed((int) values[2], (int) values[2]);
      setLocation(originX, originY);
      break;
    case 90:
      absolute = true;
      break;
    case 91:
      absolute = false;
      break;
    case 92:
      xStep.setCurrentPosition(xPosition);
      yStep.setCurrentPosition(yPosition);
      originX = xPosition;
      originY = yPosition;
      xPosition = 0;
      yPosition = 0;
      break;
    default:
      return false;
      break;
  }

  if (values != NULL)
  {
    Serial.println("Displaying Values");
    for (int i = 0; i < sizeof(values) / sizeof(float); i++)
      Serial.println(values[i]);

    delete(values);
  }

  Serial.println("Done!");
  return true;
}



//Circular movements. Use a boolean to specify whether a fixed point or interpolation is used. True = fixed point mode
//Warning - Drawing the circle with a radius is pretty processor-intensive since it needs to calculate the center point
//Actually nah, this method can only draw with a given center point.
void circularMove(float xI, float yI, float iI, float jI, boolean clockwise)
{
  int x;
  int y;
  int i;
  int j;
  //Convert to appropriate units
  if (inches)
  {
    x = (int) (xI * STEPS_PER_INCH);
    y = (int) (yI * STEPS_PER_INCH);
    i = (int) (iI * STEPS_PER_INCH);
    j = (int) (jI * STEPS_PER_INCH);
  }
  else
  {
    x = (int) (xI * STEPS_PER_MM);
    y = (int) (yI * STEPS_PER_MM);
    i = (int) (iI * STEPS_PER_MM);
    j = (int) (jI * STEPS_PER_MM);
  }

  int centerX; //Center of the circle, relative to the steps system
  int centerY;
  long positions[2];

  //Convert to polar coords first
  int initLength;
  int initAngle;
  int finalLength;
  int finalAngle;

  if (!absolute)
  {
    centerX = xPosition + i;
    centerY = yPosition + j;
    initLength = sqrt(pow(i, 2) + pow(j, 2)); //
    initAngle = (int) (180 * (atan((double) j / (double) i) / PI)); //Conversion to angle

    if (i < 0)
      initAngle += 180;
    else if (j < 0)
      initAngle += 360;

    finalLength = sqrt(pow((x - i), 2) + pow((y - j), 2));
    finalAngle = (int) (180 * (atan((double) (y - j) / (double) (x - i)) / PI));

    if (x - i < 0)
      finalAngle += 180;
    else if (y - j < 0)
      finalAngle += 360;
  }
  else
  {
    centerX = i;
    centerY = j;
    int dx = xPosition - i;
    int dy = yPosition - j;
    initLength = sqrt(pow(dx, 2) + pow(dy, 2)); //
    initAngle = (int) (180 * (atan((double) dy / (double) dx) / PI)); //Conversion to angle

    if (dx < 0)
      initAngle += 180;
    else if (dy < 0)
      initAngle += 360;

    dx = x - i;
    dy = y - j;
    finalLength = sqrt(pow(dx, 2) + pow(dy, 2)); //
    finalAngle = (int) (180 * (atan((double) dy / (double) dx) / PI)); //Conversion to angle

    if (x - i < 0)
      finalAngle += 180;
    else if (y - j < 0)
      finalAngle += 360;
  }

  int minimum = min(finalAngle, initAngle);
  int maximum = max(finalAngle, initAngle);
  double radiusChange = (double) (finalLength - initLength) / abs(maximum - minimum); //How many steps the radius changes per degree

  Serial.print("MIN");
  Serial.println(minimum);
  Serial.print("MAX");
  Serial.println(maximum);

  if (clockwise)
  {
    if (initAngle <= finalAngle)
      finalAngle -= 360;
    for (int i = maximum; i > minimum; i--)
    {
      double newRadius = (initLength + (maximum - i) * radiusChange);
      positions[0] = (long) (centerX + newRadius * cos((PI / 180) * i));
      positions[1] = (long) (centerY + newRadius * sin((PI / 180) * i));
      Serial.print("X =");
      Serial.println(positions[0]);
      Serial.print("Y =");
      Serial.println(positions[1]);
      steppers.moveTo(positions);
      steppers.runSpeedToPosition();
    }
  }
  else
  {
    if (initAngle >= finalAngle)
      initAngle -= 360;
    for (int i = minimum; i < maximum; i++)
    {
      double newRadius = (initLength + (maximum - i) * radiusChange);
      positions[0] = (long) (centerX + newRadius * cos((PI / 180) * i));
      positions[1] = (long) (centerY + newRadius * sin((PI / 180) * i));
      steppers.moveTo(positions);
      steppers.runSpeedToPosition();
    }
  }

  if (absolute)
  {
    xPosition = x;
    yPosition = y;
  }
  else
  {
    xPosition += x;
    yPosition += y;
  }
}



/*
  else
  {
    radius = i;
    int dx = x-xPosition;
    int dy = y-yPosition;
    int magnitude = dx^2 + dy^2;
    angle = acos(((dx)^2 + (dy)^2 - 2*radius^2) / (-2 * radius);
    int sideAngle = 90 - angle/2; //Going to use sin of the side angle to find the center point
    int midpointX = (x + xPosition) / 2;//Grab the midpoint of the line. We're finding the median of the triangle formed by the 2 points and the circle midpoint
    int midpointY = (y + yPosition) / 2;
    int midpointLength = sin(sideAngle) * radius;
    float vectorRatio = magnitude / midpointLength;
    centerX = midpointX + dy * vectorRatio; //Multiply by the vector of the perpendicular bisector to find the location of the midpoint
    centerY = midpointY - dx * vectorRatio;

    //Fucking hell that was annoying to calculate
  }


  //Draw the damn thing

  }
*/

//Move Pointer
void setLocation(float xI, float yI)
{
  int x;
  int y;
  //Convert to appropriate units
  if (inches)
  {
    x = (int) (STEPS_PER_INCH * xI);
    y = (int) (STEPS_PER_INCH * yI);
  }
  else
  {
    x = (int) (STEPS_PER_MM * xI);
    y = (int) (STEPS_PER_MM * yI);
  }


  long positions[2];

  //Move the thing based on whether the current mode is relative or absolute
  if (absolute)
  {
    //New positions to move to. Offset by 10 to prevent collision
    positions[0] = x;
    positions[1] = y;
    xPosition = x;
    yPosition = y;
  }
  else
  {
    xPosition = xPosition + x;
    yPosition = yPosition + y;
    positions[0] = xPosition;
    positions[1] = yPosition;
  }

  Serial.println("MOVING TO");
  Serial.println(x);
  Serial.println(y);

  //Move the steppers to new position
  steppers.moveTo(positions);
  steppers.runSpeedToPosition();
}

//Change speed of steppers
void setMotorSpeed(int xSpd, int ySpd)
{
  if (xSpd < MAX_SPEED && ySpd < MAX_SPEED)
  {
    xStep.setMaxSpeed(xSpd);
    yStep.setMaxSpeed(ySpd);
  }
}

//Go into the string and grab the instruction
void parseCode (String input, int* command)
{
  String id = ""; //Something to store the command id.

  //Grab the command id by reading until the part seperating the input command
  for (int i = 1; input[i] != ' ' && i < input.length(); i++)
  {
    id += input[i];
  }

  Serial.println(id);

  //Now return the command id in the int
  *command = id.toInt();
}

//Takes an input string and gets a number that succeeds the char denoted by start. Deft is the default value to return if no such char exists.
float getValueAt(String input, char start, float deft)
{
  int pos = input.indexOf(start);
  int last = input.indexOf(' ', pos);
  if (last == -1)
    last = input.length();

  Serial.println(input.substring(pos + 1, last));
  if (pos != -1)
    return (input.substring(pos + 1, last)).toFloat();

  return deft;
}

//Parse the string and return an int array holding all the necessary values for the command
//The type represents what kind of G-command was entered.
float * getPosition(String input, int type)
{
  //Initialize an array to hold the values to parse.
  float * values = (float*) malloc(type * sizeof(float));
  String out = "";

  //Designated G-Codes. G-Codes that dont fit in with the rest of the pick-and-keep system.
  //G04, wait.
  if (type == 7)
  {
    values[0] = getValueAt(input, 'S', 0);
    return values;
  }

  //G-Code 00 - Rapid move takes only X and Y. Always runs at max speed
  if (type > 1)
  {
    values[0] = getValueAt(input, 'X', (float) xPosition / (inches ? STEPS_PER_INCH : STEPS_PER_MM));
    values[1] = getValueAt(input, 'Y', (float) yPosition/ (inches ? STEPS_PER_INCH : STEPS_PER_MM));
    Serial.println("Found X = " + String(values[0]));
    Serial.println("Found Y = " + String(values[1]));
  }
  //G-Code 01 - Feed Move takes X and Y as well as speed.
  if (type > 2)
  {
    values[2] = getValueAt(input, 'F', MAX_SPEED);
  }
  //G-Code 02 and 03 - Circular move. They both take the same arguments for distance to maintain. Gonna keep the xPos and yPos here because it asks for the x and y position to rotate away from
  if (type > 3)
  {
    values[3] = getValueAt(input, 'I', (float) originX / (inches ? STEPS_PER_INCH : STEPS_PER_MM));
    values[4] = getValueAt(input, 'J', (float) originY / (inches ? STEPS_PER_INCH : STEPS_PER_MM));
  }
  return values;
}

//Custom readline method. Reads from serial until a \n character is encountered.
String readLine()
{
  delay(1000); //Allow time for the buffer to fill
  char curChar; //Current Character in the buffer that its on
  String output = ""; //The completed string to be output
  while (Serial.available() > 0 && ((curChar = Serial.read()) != '\n' || curChar == ' ')) //Until it encounters a new line, fill up the string
    output += curChar;
  Serial.println(output);
  return output;
}

//Custom print method. Prints character by character so that it doesn't exceed the output limit 
void printLongString (String toPrint)
{
  int bytesWritten = 0; //Bytes from the string written so far
  toPrint += "\n"; //Add a concluding character to the string

  //if(toPrint.length() < 64)
  //{
  //    Serial.print(toPrint); //Don't use the long-string if its not necessary
  //    return;
  //}
  
  while(bytesWritten < toPrint.length())
  {
    if(Serial.availableForWrite() < 63) //Arduino only has a 64 byte buffer, so it checks if theres space to add more and then fills the rest of the buffer
    {
      int difference = 64 - Serial.availableForWrite(); //Get number of empty bytes
      Serial.print(toPrint.substring(bytesWritten, min(bytesWritten + difference, toPrint.length())));
      bytesWritten += difference; //Note the number of additional bytes written
    }
  }
}
void loop() {
  String in = "";
  long positions[2]; // Array of desired stepper positions
  digitalWrite(INDICATOR, 0);


  isRunning = false;

  //This char is to note down the kind of command being used right now.
  char currentMode;

  //This int holds the command ID
  int id;

  //This string holds the serial input input
  String input;

  //This boolean allows for detection of unexpected commands
  boolean success;

  //Wait for the start code from the uploader
  while (true)
  {
    if (!Serial.available())
    {
      analogWrite(INDICATOR, 0);
    }
    else if (readLine() == "Beginning GCode")
    {
      analogWrite(INDICATOR, 50);
      isRunning = true;
      break;
    }
  }

  analogWrite(INDICATOR, 150);
  Serial.println("Ready for GCODE Input");

  while (isRunning)
  {
    //Wait for a line of code to be entered
    while (!Serial.available())
    {
    }
    //Grab some input
    input = readLine();
    //Tell the computer that the engraver has recieved the code input
    Serial.println("Acknowledged");

    //1st letter = command type
    currentMode = input[0];

    //Parsing time - Go divide the G-Code into the various commands (F, M, G) and get the command ID
    parseCode(input, &id);

    Serial.println(id);
    Serial.println(input);


    success = false;


    //pick the right mode to execute the command in. Also store the result in an boolean for debug purposes
    switch (currentMode)
    {
      case 'M':
        //success = executeMCommand(id);
        break;
      case 'G':
        success = executeGCommand(id, input);
        break;
      case 'F':
        // success = executeFCommand(id);
        break;
      default:
        success = false;
    }

    //Error message gets thrown
    if (!success)
      Serial.println("Improper code entered!");

    printLongString("Executed: " + input);
  }
}

