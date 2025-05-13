#include <main.h>

void setup() 
{
  Serial.begin(115200);
  
  stepper_X.begin(RPM, MICROSTEPS);
  stepper_Y.begin(RPM, MICROSTEPS);
  
  pinMode (dir_pin_X, OUTPUT);
  pinMode (step_pin_X, OUTPUT);
  pinMode (dir_pin_Y, OUTPUT);
  pinMode (step_pin_Y, OUTPUT);
  pinMode (home_switch_stepperX, INPUT_PULLUP);
  pinMode (home_switch_stepperY, INPUT_PULLUP);
  pinMode (home_switch_Z, INPUT_PULLUP);
  pinMode (SLP, OUTPUT);
  digitalWrite (SLP, LOW);  

  Servo_Z.attach(Servo_Z_Pin);
  Servo_Gripper.attach(Servo_G_Pin);

  Data ='h';


  // Setup WiFiManager
  wifiManager.setConnectTimeout(15);
  if (!wifiManager.autoConnect("Gripper-Setup")) {
      Serial.println("Failed to connect to WiFi and hit timeout. Restarting...");
      ESP.restart();
  }

  // Setup MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("Setup complete.");
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  if (Serial.available()) {
    Data = Serial.readString();
    Data.trim();
    Serial.print("USB Serial Message: ");
    Serial.println(Data);
    processData(Data);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();
  Serial.print("MQTT Message Received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == MQTT_TOPIC_COMMAND) {
    Data = message;
    processData(Data);
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32-Gripper")) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_TOPIC_COMMAND);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void processData(String receivedData) {
  receivedData.trim();
  mqttClient.publish(MQTT_TOPIC_STATUS, "processing");

  Serial.print("Data to process: ");
  Serial.println(receivedData);

  switch (Data[0]) {
    case 'h': Home(); break;
    case 'x': GoTo_X(); break;
    case 'y': GoTo_Y(); break;
    case 'z': GoTo_Z(); break; 
    case 'i': z_in(); break; 
    case 'o': z_out(); break;      
    case 'g': GoTo_XYZ(); break; 
    case 'b': 
      Bring();
      mqttClient.publish(MQTT_TOPIC_STATUS, "completed");
      break;
    case 't': GoTo_XYZXYZ(); break;  
    default:
      Serial.println("Invalid command received.");
      break;
  }
}
void Home()
{
  if (Data[0]=='h')
  {
  Serial.println("Homing...");
  stepper_Y.setRPM(homing_speed);
  stepper_X.setRPM(homing_speed);
  Servo_Z.write(0);
  Servo_Gripper.write(40);
  Serial.println("Z home");
  
  while (digitalRead(home_switch_stepperX) == HIGH){
    stepper_X.rotate(-20) ;
  }
   
   while (digitalRead(home_switch_stepperY) == HIGH){
    stepper_Y.rotate(-20);
    }
    delay (100);
    
    stepper_Y.setRPM(10);
    stepper_X.setRPM(10);
    stepper_X.rotate(Home_X);
    stepper_Y.rotate(Home_Y);
     

  CurrentPosition_X = Home_X ;
  CurrentPosition_Y = Home_Y;

    Serial.print("the updated current position X: ");
    Serial.println(CurrentPosition_X);
    Serial.print("the updated current position Y: ");
    Serial.println(CurrentPosition_Y);    
    Serial.print("the updated current position Z: ");
    Serial.println(0);
    Serial.println("Homing Done");
  Data = "";
  }
  
}

void Bring() {
  int x = 0, y = 0, close_angle = 0; // Parsed from input
  int dela = 500; // Base delay duration (adjust if needed)

  // Check if the command starts with 'b'
  if (Data.length() > 1 && Data[0] == 'b') {
      String parameters = Data.substring(1); // Get parameters: "x,y,close_angle"

      // Find the commas
      int comma1 = parameters.indexOf(',');
      int comma2 = parameters.indexOf(',', comma1 + 1); // Find the second comma

      // Check if both commas were found (meaning 3 parameters exist)
      if (comma1 != -1 && comma2 != -1) {
          // Parse the values
          x = parameters.substring(0, comma1).toInt();
          y = parameters.substring(comma1 + 1, comma2).toInt();
          close_angle = parameters.substring(comma2 + 1).toInt();

          // --- Start New Bring Sequence ---
          Serial.println("Starting Custom Bring sequence...");
          Serial.print("Target X:"); Serial.print(x);
          Serial.print(", Y:"); Serial.print(y);
          Serial.print(", Close Angle:"); Serial.println(close_angle);

          // 1. Go to X, Y position
          // (Using the current global 'speed')
          Serial.println("Step 1: Moving to X, Y...");
          Data = String('x') + String(x); // Prepare Data for GoTo_X
          GoTo_X();
          Data = String('y') + String(y); // Prepare Data for GoTo_Y
          GoTo_Y();
          delay(dela); // Wait after reaching position

          // 2. Open gripper fully (fixed angle 140)
          Serial.println("Step 2: Opening Gripper...");
          Data = String('z') + String(140); // Use 'z' command for GoTo_Z
          GoTo_Z(); // Call GoTo_Z to control the gripper servo
          delay(dela * 2); // Allow time for gripper servo

          // 3. Move Z-axis "out" (Lower Z-axis)
          Serial.println("Step 3: Lowering Z-Axis...");
          Data = "o"; // Prepare Data for z_out()
          z_out();    // Call function to move Servo_Z
          delay(dela * 3); // Allow time for Z movement

          // 4. Close gripper to the specified angle
          Serial.println("Step 4: Closing Gripper...");
          Data = String('z') + String(close_angle); // Prepare Data for GoTo_Z
          GoTo_Z(); // Call GoTo_Z to control the gripper servo
          delay(dela * 2); // Allow time for gripper servo

          // 5. Move Y-axis "down" 10 steps (Increase Y coordinate)
          Serial.println("Step 5: Moving Y axis down (+10)...");
          // IMPORTANT: Use CurrentPosition_Y which is updated inside GoTo_Y
          int new_y = CurrentPosition_Y + 10;
          // Optional: Add check to ensure new_y doesn't exceed Max_Y
          if (new_y > Max_Y) {
              new_y = Max_Y;
              Serial.println("Warning: Y+10 exceeds Max_Y. Moving to Max_Y instead.");
          }
          Data = String('y') + String(new_y); // Prepare Data for GoTo_Y
          GoTo_Y();
          delay(dela); // Wait after Y adjustment

          // 6. Move Z-axis "in" (Raise Z-axis)
          Serial.println("Step 6: Raising Z-Axis...");
          Data = "i"; // Prepare Data for z_in()
          z_in();     // Call function to move Servo_Z
          delay(dela * 3); // Allow time for Z movement

          // --- End New Bring Sequence ---

      } else {
          Serial.println("Invalid format for 'b' command. Use: bx,y,close_angle");
      }

      Serial.println("Bring Sequence Done");
      Data = ""; // Clear global Data variable after finishing

  } else if (Data[0] == 'b') {
       Serial.println("Invalid format for 'b' command. Use: bx,y,close_angle");
       Data = ""; // Clear Data if format is wrong
  }
}



void GoTo_XYZ(){
  
  int x = 0, y = 0, z = 0, Speed = 0;
  
  // Assuming data is in the format "gx,y,z,speed"
  if (Data[0]=='g') {
    Data.remove(0, 1);  
    
    
    int comma1 = Data.indexOf(",");
    int comma2 = Data.indexOf(",", comma1 + 1);
    int comma3 = Data.indexOf(",", comma2 + 1);
    
    if (comma3 != -1) {  
      x = Data.substring(0, comma1).toInt();
      y = Data.substring(comma1 + 1, comma2).toInt();
      z = Data.substring(comma2 + 1, comma3).toInt();
      Speed = Data.substring(comma3 + 1).toInt();
      Speed = constrain(Speed, 0, 200);
      speed = Speed;

      char myChar = 'x';
      Data =String(myChar) + String(x);
      GoTo_X();

      myChar = 'y';
      Data =String(myChar) + String(y);
      GoTo_Y();

      myChar = 'z';
      Data =String(myChar) + String(z);
      GoTo_Z();
    }
    Serial.println("GoTo_XYZ Done");
    Data = ""; 
  }
}


void GoTo_Z()
 {
   if (Data[0]=='z')
   {
    
    int z = Data.substring(1).toInt();
    if (z >= 20 && z <= 150) {  
    Servo_Gripper.write(z);
    delay(100);
    Serial.print("the updated current position Z: ");
    Serial.println(z);
    }
     Data = ""; 
   }
 }

 void z_in() {
  if (Data == "i"){
  Servo_Z.write(GRIPPER_IN_ANGLE);
  Serial.println("done_in");
  }
  Data = "";
}

void z_out() {
  if (Data == "o"){
  Servo_Z.write(GRIPPER_OUT_ANGLE);
  Serial.println("done_out");
  }
  Data = "";
}

 void GoTo_X()
 {
   if (Data[0]=='x')
   {
    int x = Data.substring(1).toInt();
    if (x >= Home_X && x <= Max_X) {  
    Serial.print("x = ");
    Serial.println(x);

    int x_steps = x - CurrentPosition_X ;
    Serial.print("steps to add = ");
    Serial.println(x_steps);

    stepper_X.setRPM(speed);
    stepper_X.rotate(x_steps);
    CurrentPosition_X = x;
    Serial.print("the updated current position X: ");
    Serial.println(CurrentPosition_X);
    Data = "";
   } 
  }
 }

 void GoTo_Y()
 {
   if (Data[0]=='y')
   {
    int y = Data.substring(1).toInt();
    if (y >= Home_Y && y <= Max_Y) {  
    Serial.print("y = ");
    Serial.println(y);

    int y_steps = y - CurrentPosition_Y ;
    Serial.print("steps to add = ");
    Serial.println(y_steps);

    stepper_Y.setRPM(speed);
    stepper_Y.rotate(y_steps);
    CurrentPosition_Y = y;
    Serial.print("the updated current position Y: ");
    Serial.println(CurrentPosition_Y);
    Data = "";
   } 
  }
 } 
void GoTo_XYZXYZ() {
  int x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0, Speed = 0;
  int Dela = 500;

  if (Data[0] == 't') {
      Data.remove(0, 1);

      int comma1 = Data.indexOf(",");
      int comma2 = Data.indexOf(",", comma1 + 1);
      int comma3 = Data.indexOf(",", comma2 + 1);
      int comma4 = Data.indexOf(",", comma3 + 1);
      int comma5 = Data.indexOf(",", comma4 + 1);
      int comma6 = Data.indexOf(",", comma5 + 1);

      x1 = Data.substring(0, comma1).toInt();
      y1 = Data.substring(comma1 + 1, comma2).toInt();
      z1 = Data.substring(comma2 + 1, comma3).toInt();
      x2 = Data.substring(comma3 + 1, comma4).toInt();
      y2 = Data.substring(comma4 + 1, comma5).toInt();
      z2 = Data.substring(comma5 + 1, comma6).toInt();
      
      // Move to the first set of coordinates
      Serial.println("Moving to first set of coordinates...");
      Data = String('x') + String(900);
      GoTo_X();
      Data = String('y') + String(3580);
      GoTo_Y();
      delay(Dela); // Wait after reaching position

//z1
        Data = String('z') + String(140); // Use 'z' command for GoTo_Z
        GoTo_Z(); // Call GoTo_Z to control the gripper servo
        delay(Dela * 2); // Allow time for gripper servo

        Data = "o"; // Prepare Data for z_out()
        z_out();    // Call function to move Servo_Z
        delay(Dela * 4); // Allow time for Z movement

        Data = String('z') + String(40); // Prepare Data for GoTo_Z
        GoTo_Z(); // Call GoTo_Z to control the gripper servo
        delay(Dela * 2); // Allow time for gripper servo

        speed = 20;

      Data = String('y') + String(3620);
      GoTo_Y();  
      speed = 90;

        Data = "i"; // Prepare Data for z_in()
        z_in();     // Call function to move Servo_Z
        delay(Dela * 3); // Allow time for Z movement

      // Move to the second set of coordinates
      Serial.println("Moving to second set of coordinates...");
      Data = String('x') + String(x2);
      GoTo_X();
      Data = String('y') + String(y2);
      GoTo_Y();
// z2
        
        Data = "o"; // Prepare Data for z_out()
        z_out();    // Call function to move Servo_Z
        delay(Dela * 4); // Allow time for Z movement

        Data = String('z') + String(140); // Use 'z' command for GoTo_Z
        GoTo_Z(); // Call GoTo_Z to control the gripper servo
        delay(Dela * 2); // Allow time for gripper servo

        Data = "i"; // Prepare Data for z_in()
        z_in();     // Call function to move Servo_Z
        delay(Dela * 3); // Allow time for Z movement

        Data = String('z') + String(40); // Prepare Data for GoTo_Z
        GoTo_Z(); // Call GoTo_Z to control the gripper servo
        delay(Dela * 5); // Allow time for gripper servo
      
      Serial.println("GoTo_XYZXYZ Done");
      Data = "";
  }
}  