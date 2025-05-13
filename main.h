#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "BasicStepperDriver.h"
#include <WiFiManager.h>
#include <PubSubClient.h> // Include MQTT library

WiFiManager wifiManager;

#define step_pin_X 26
#define dir_pin_X 16
#define home_switch_stepperX 13

#define step_pin_Y 25
#define dir_pin_Y 27
#define home_switch_stepperY 5 

#define SLP 12
#define MOTOR_STEPS 200
#define Home_X 50
#define Home_Y 50
#define RPM 400
#define MICROSTEPS 32                       // or 16

#define Servo_Z_Pin 17     // pin Z STEP
#define home_switch_Z 23 
#define Servo_G_Pin 14     // pin Z DIR

#define Max_X 4000  //82 cm
#define Max_Y 7800   //173 cm

#define GRIPPER_IN_ANGLE 10                         // Define the gripper closed angle
#define GRIPPER_OUT_ANGLE 175    

long CurrentPosition_X = Home_X ;
long CurrentPosition_Y = Home_Y;

// MQTT Configuration
#define MQTT_BROKER "192.168.1.115" //192.168.100.81 
#define MQTT_PORT 1883
#define MQTT_TOPIC_COMMAND "gripper/commands"
#define MQTT_TOPIC_STATUS "gripper/status"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void Home();
void GoTo_X();
void GoTo_Y();
void GoTo_Z();
void GoTo_XYZ();
void Bring();
void z_in();
void z_out();
void GoTo_XYZXYZ();
void processData(String receivedData);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

BasicStepperDriver stepper_X(MOTOR_STEPS, dir_pin_X, step_pin_X);
BasicStepperDriver stepper_Y(MOTOR_STEPS, dir_pin_Y, step_pin_Y);

Servo Servo_Z ;
Servo Servo_Gripper ;

String Data;
String previousData;

const char delimiter = 'g';
const char delimiter_bring = 'b';
int speed = 60;
int homing_speed = 40;

#endif // MAIN_H