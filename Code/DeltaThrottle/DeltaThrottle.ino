
JoyState_t joySt;

const bool DEBUG = false;  // Set to true to debug the raw values
const int throttleOffButton = 3; // Set to the pin number to use for throttle toggle
int throttleState = 0; // Used to track the state of throttle lockout 0=unlocked 1=locked
int throttleButtonState = 0;

float xZero, yZero, zZero;
float xValue, yValue, zValue;

// Geometry
// Lengths for push rods and pivot arms should be measured from rotation center to rotation center
float handle_rad = 1.75;   // End effector
float base_rad = 1.75;     // Base
float pushrod_lng = 3.5;  // Change this if you used shorter or longer push rods (hex standoffs for 3d print)
float pivot_lng = 2.25;     // Change this if you modified the pivot arm length.

// Configuration
float deadzone = 0.1;  // Smaller values will be set to 0
int gain = 150; // 155
 
// Trigonometric constants
float sqrt3 = sqrt(3.0);
float pi = 3.141592653;    // PI
float sin120 = sqrt3/2.0;   
float cos120 = -0.5;        
float tan60 = sqrt3;
float sin30 = 0.5;
float tan30 = 1/sqrt3;

void setup(){
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(throttleOffButton, INPUT);
  pinMode(2,  INPUT);
  pinMode(3,  INPUT);
  pinMode(4,  INPUT);
  digitalWrite(throttleOffButton, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);

  if (DEBUG) {
    Serial.begin(9600);
  }
  
  getForwardKinematic();
  
  // Calculate neutral position
  xZero = 0;
  yZero = 0;
  zZero = zValue;

  joySt.xAxis = 0;
  joySt.yAxis = 0;
  joySt.zAxis = 0;
}

// The delta kinematic math
// You shouldn't need to change anything here.
void getForwardKinematic() {
  // p1 is the bottom joint, p2 is the top right, p3 is the top left
  int p1ADC = analogRead(A0);
  int p2ADC = analogRead(A1);
  int p3ADC = analogRead(A2);
 
  // Get the angle of each joint in radians
  float theta1 = (p1ADC-60) * 0.003475;
  float theta2 = (p2ADC-60) * 0.003475;
  float theta3 = (p3ADC-60) * 0.003475;
    
  float t = base_rad-handle_rad;
  float y1 = -(t + pivot_lng*cos(theta1));
  float z1 = pivot_lng*sin(theta1);
 
  float y2 = (t + pivot_lng*cos(theta2))*sin30;
  float x2 = y2*tan60;
  float z2 = pivot_lng*sin(theta2);
 
  float y3 = (t + pivot_lng*cos(theta3))*sin30;
  float x3 = -y3*tan60;
  float z3 = pivot_lng*sin(theta3);
 
  float dnm = (y2-y1)*x3-(y3-y1)*x2;
 
  float w1 = y1*y1 + z1*z1;
  float w2 = x2*x2 + y2*y2 + z2*z2;
  float w3 = x3*x3 + y3*y3 + z3*z3;
   
  // x = (a1*z + b1)/dnm
  float a1 = (z2-z1)*(y3-y1)-(z3-z1)*(y2-y1);
  float b1 = -((w2-w1)*(y3-y1)-(w3-w1)*(y2-y1))/2.0;
 
  // y = (a2*z + b2)/dnm;
  float a2 = -(z2-z1)*x3+(z3-z1)*x2;
  float b2 = ((w2-w1)*x3 - (w3-w1)*x2)/2.0;
 
  // a*z^2 + b*z + c = 0
  float a = a1*a1 + a2*a2 + dnm*dnm;
  float b = 2*(a1*b1 + a2*(b2-y1*dnm) - z1*dnm*dnm);
  float c = (b2-y1*dnm)*(b2-y1*dnm) + b1*b1 + dnm*dnm*(z1*z1 - pushrod_lng*pushrod_lng);
  
  // Discriminant
  float d = b*b - 4.0*a*c;
    
  zValue = 0.5*(-b+sqrt(d))/a;
  xValue = (a1*zValue + b1)/dnm;
  yValue = (a2*zValue + b2)/dnm;
    
  if (DEBUG) {
    Serial.print("T1: ");
    Serial.println(theta1);
    Serial.print("T2: ");
    Serial.println(theta2);
    Serial.print("T3: ");
    Serial.println(theta3);
    Serial.print("Z1: ");
    Serial.println(z1);
    Serial.print("Z2: ");
    Serial.println(z2);
    Serial.print("Z3: ");
    Serial.println(z3);
    Serial.print("DNM: ");
    Serial.println(dnm);
  }
}

void loop() {
  getForwardKinematic();
  
  int throttleButtonState = !digitalRead(throttleOffButton);
  int btn1 = !digitalRead(2);
  int btn2 = !digitalRead(3);
  int btn3 = !digitalRead(4);
  
  // Subtract zero position
  xValue -= xZero;
  yValue -= yZero;
  zValue -= zZero;

  // X Value deadzone
  if(xValue < -1*deadzone) {
    xValue = xValue + deadzone;
  }
  else if (xValue > deadzone) {
    xValue = xValue - deadzone;
  }
  else {
    xValue = 0;
  }
  
  // Y Value deadzone
  if (yValue < -1*deadzone) {
    yValue = yValue + deadzone;
  }
  else if (yValue > deadzone) {
    yValue = yValue - deadzone;
  }
  else {
    yValue = 0;
  }
  
  // Z Value deadzone
  if (zValue < -1*deadzone) {
    zValue = zValue + deadzone;
  }
  else if (zValue > deadzone) {
    zValue = zValue - deadzone;
  }
  else {
    zValue = 0;
  }

  // Apply gain
  int xVal = xValue*gain;
  int yVal = yValue*gain;
  int zVal = zValue*gain;
  
  // Constrain outputs to +- 100
  xVal = constrain(xVal,-100,100);
  yVal = constrain(yVal,-100,100);
  zVal = constrain(zVal,-100,100);
  
  // Map outputs to 8 bit values
  joySt.xAxis = map(xVal, -100, 100, 0, 255);
  joySt.yAxis = map(yVal, -100, 100, 0, 255);
  joySt.zAxis = map(zVal, -100, 100, 0, 255);
  
  if (DEBUG) {
    Serial.print("X: ");
    Serial.println(xVal);
    Serial.print("Y: ");
    Serial.println(yVal);
    Serial.print("Z: ");
    Serial.println(zVal);
    Serial.print("B1: ");
    Serial.println(btn1);
    Serial.print("B2: ");
    Serial.println(btn2);
    Serial.print("B3: ");
    Serial.println(btn3);
  }

  joySt.buttons = btn1 | (btn2<<1) | (btn3 <<2);

  // If the throttle is currently unlocked and the toggle has been activated, lock the throttle
  if (throttleButtonState == 1 and throttleState == 0) {
    throttleState = 1;
  }
  // If the throttle locked and the toggle is activated, unlock the throttle
  else if (throttleButtonState == 1 and throttleState == 1) {
    throttleState = 0;
  }

  // If the throttle is locked set X and Z to 0, but allow normal throttle control on Y
  if (throttleState == 1) {
    joySt.xAxis = map(0, -100, 100, 0, 255);
    joySt.zAxis = map(0, -100, 100, 0, 255);
  }    

  // Send to USB
  Joystick.setState(&joySt);
  
  if (DEBUG) {
    delay(1000);
  } 
}
