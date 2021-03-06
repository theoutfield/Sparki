#include <Sparki.h>  // Include the sparki library.

const float pi = 3.14159265358979323846;

// Robot + External World data:
const float initialHeading = 90.0; // Intial heading in the current coordinates system: 90 degs = pi/2 [rad]. 

// Robot data:
const int threshold = 500; // Line and edge sensors thereshold [0 - 1015 w/out units].
const int servoDelay = 350; // Minumum time to give to the servo to rotate 90 degrees [ms].
const int robotRadius = 9; // Necessary radius to rotate, from the robot's center [cm].
const int rangerToCentreDistanceFront = 4; // Distance from the measuring edge of the (centered) ultrasonic sensor to the robot's centre [cm].
const int rangerToCentreDistanceSide = 2; // Distance from the measuring edge of the (rotated) ultrasonic sensor to the robot's centre [cm]. Rounded to floor.
const int rangerToFrontDistance = 5; // Distnace from the measuring edge of the (centered) ultrasonic sensor to the gripper's extreme [cm].

// Robot variables:
bool  edgeLeft = false,
      lineLeft = false,
      lineCenter = false,
      lineRight = false,
      edgeRight = false;
int ping = 0; // [cm].
String state = "undefined";
float heading = initialHeading; // [degs].
int posX = 0; // [cm].
int posY = 0; // [cm].
int homeX = 0; // [cm].
int homeY = 0; // [cm].

// Map variables:
int roomMaxX = 0; // [cm].
int roomMaxY = 0; // [cm].


void printPingData()
{
  sparki.print("Ping="); // Ultrasonic ranger on screen.
  sparki.print(ping);
  sparki.println(" cm"); 
}

void showSensorsAndState()
{
  sparki.clearLCD();

  sparki.print("edgeL=");
  sparki.println(edgeLeft);
  sparki.print("lineL=");
  sparki.println(lineLeft);
  sparki.print("lineC=");
  sparki.println(lineCenter);
  sparki.print("lineR=");
  sparki.println(lineRight);
  sparki.print("edgeR=");
  sparki.println(edgeRight);  

  printPingData();
  
  sparki.println(String("state=") + state);

  sparki.updateLCD();
}

void showRoomData(float value0 = 0.0, float value1 = 0.0)
{
  sparki.clearLCD();

  if (state == "moveTo")
  {
    sparki.print("goToX=");
    sparki.println(value0);
    sparki.print("goToY=");
    sparki.println(value1);
  }
  else // Default:
  {
    sparki.print("roomMaxX=");
    sparki.println(roomMaxX);
    sparki.print("roomMaxY=");
    sparki.println(roomMaxY);
  }

  sparki.print("posX=");
  sparki.println(posX);
  sparki.print("posY=");
  sparki.println(posY);

  sparki.print("heading=");
  sparki.println(heading);
  
  printPingData();
  
  sparki.println(String("state=") + state);

  sparki.updateLCD();  
}

void readIRSensors()
{
  //each sensor is 1 if reading white, and 0 if reading black:
  edgeLeft =  sparki.edgeLeft() > threshold;
  lineLeft =  sparki.lineLeft() > threshold;
  lineCenter = sparki.lineCenter() > threshold;
  lineRight =  sparki.lineRight() > threshold;
  edgeRight = sparki.edgeRight() > threshold;
}

//Returns true if the robot is centered over the Home mark:
bool robotIsCenteredOverHomeMark()
{
  //The robot is centered once all it's edge and line sensors read black:
  return !edgeLeft && !lineLeft && !lineCenter && !lineRight && !edgeRight;
}

void centerRobotOverHomeMark()
{
  state = "centering";
  readIRSensors();
  while(!robotIsCenteredOverHomeMark())
  {
    readIRSensors();
    showSensorsAndState();
  }
  sparki.beep();
  showSensorsAndState();
}

void rotate(float angle)
{
  if (angle > 0.0)
    sparki.moveLeft(angle);
  else
    sparki.moveRight(-angle);

  heading += angle;
}

void measureRoom(bool robotAtHome)
{
  state = "measureRoom";
  showSensorsAndState();
  
  // Starts to measure the Y longitude: 
  sparki.servo(SERVO_CENTER);
  delay(servoDelay);
  ping = sparki.ping(); //"ping" variable is used to show the sensor value on the LCD.
  roomMaxY = rangerToCentreDistanceFront + ping;
  showRoomData();

  // Measures the X longitude: 
  sparki.servo(SERVO_RIGHT);
  delay(servoDelay);
  ping = sparki.ping();
  roomMaxX = rangerToCentreDistanceSide + ping;
  showRoomData();

  sparki.servo(SERVO_LEFT);
  delay(2*servoDelay); // Twice the time of 1/4 of revolution rotation.
  ping = sparki.ping();
  roomMaxX += rangerToCentreDistanceSide + ping;
  posX = rangerToCentreDistanceSide + ping;
  showRoomData();

  // Finishes to measure the Y longitude:
  rotate(90); // The robot rotation has more relative error than the servo rotation:
  delay(servoDelay); // Just to stop the robot for a few milliseconds, so it can measure distance.
  ping = sparki.ping(); //"ping" variable is used to show the sensor value on the LCD.
  roomMaxY += rangerToCentreDistanceSide + ping;
  posY = rangerToCentreDistanceSide + ping;
  
  // If at home, centers the robot again:
  if (robotAtHome)
  {
    sparki.moveLeft();
    centerRobotOverHomeMark();
    heading = initialHeading; // Special case with external mark centering: the heading goes back to it's intial state.
    sparki.moveStop();
    
    homeX = posX;
    homeY = posY;
  }
  
  // Leaves the ultrasonic sensor centered:
  sparki.servo(SERVO_CENTER);
  delay(servoDelay);  
}

// Non diagonal (Cartesian) moveTo version. There are no negative possible positions in this coordinates system:
void moveTo(int x, int y)
{
  state = "moveTo";
  showRoomData(x - posX, y - posY);
  
  // No security or other checks in this first version:
  rotate(-heading); // Rotates the robot to zero heading.
  if ((x - posX) > 0)
    sparki.moveForward(x - posX);
  else if ((x - posX) < 0)
    sparki.moveBackward(posX - x);
  posX = x;
  showRoomData(x - posX, y - posY);

  showRoomData(x - posX, y - posY);
  rotate(90);
  if ((y - posY) > 0)
    sparki.moveForward(y - posY);
  else if ((y - posY) < 0)
    sparki.moveBackward(posY - y);
  posY = y;
  showRoomData(x - posX, y - posY);
}

void beepAndWait(int delayTime = 250)
{
  sparki.beep();
  delay(delayTime);
}

void findDoors(int servoAngle)
{
  state = "findDoors";
  sparki.servo(SERVO_CENTER);
  delay(servoDelay);
  int frontDistance = sparki.ping();
  
  sparki.moveForward();
  while (frontDistance > (robotRadius - 3)) // An smaller radius improves precission on door position.
  {
    sparki.servo(servoAngle);
    delay(servoDelay);
    ping = sparki.ping();
    showRoomData();

    sparki.servo(SERVO_CENTER);
    delay(servoDelay);
    frontDistance = sparki.ping();
  }
  
  /* ##Future improvements:
  //Updates the position, based on the heading and on the ping readings:
  //Note 0: It's a precondition that the room has to be measured previous to the
  //call to this function.
  //Note 1: This simple conditions work only on Cartesian moves of the robot:
  sparki.moveStop();
  beepAndWait();  
  frontDistance = sparki.ping();
  if (heading != initialHeading)
    posX = roomMaxX - frontDistance - rangerToCentreDistanceFront;
  else
    posY = roomMaxY - frontDistance - rangerToCentreDistanceFront;
  */

  showRoomData();
}

void setup()
{
  centerRobotOverHomeMark();
  delay(1500); // Give time to the human to take her/his hands off.
  measureRoom(true);
  
  //Find doors in this room:
  findDoors(SERVO_LEFT);
  rotate(180.0);
  findDoors(SERVO_LEFT);
  
  //##Debug:
  /*
  sparki.clearLCD();
  sparki.print("initialX=");
  sparki.println(initialX);  
  sparki.print("initialY=");
  sparki.println(initialY);  
  sparki.println(String("state=") + state);
  sparki.updateLCD();
  */
  showRoomData();
  delay(30000);
  
//  moveTo(posX, initialY);
  
//  findDoors(SERVO_RIGHT);
//  moveTo(initialX, posY);
}

void loop()
{
  ping = sparki.ping(); //update the ultrasonic sensor data to be displayed.
  showRoomData();
  delay(50);
}

