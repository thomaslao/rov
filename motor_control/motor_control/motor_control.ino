#include <Servo.h>

byte servoPin =10; // signal pin for the ESC.
byte gripperPin=9;
byte potentiometerPin = A0; // analog input pin for the potentiometer.
byte potentiometerPin_g = A1; // analog input pin for the potentiometer.
Servo servo;
Servo servo1;


void setup() {
servo.attach(servoPin);
servo.writeMicroseconds(1500); // send "stop" signal to ESC. Also necessary to arm the ESC.
servo1.attach(gripperPin);
servo1.writeMicroseconds(1500); // send "stop" signal to ESC. Also necessary to arm the ESC

delay(7000); // delay to allow the ESC to recognize the stopped signal.
}

void loop() {

int potVal = analogRead(potentiometerPin); // read input from potentiometer.
int potVal1 = analogRead(potentiometerPin_g); // read input from potentiometer.
int pwmVal = map(potVal,0, 1023, 1100, 1900); // maps potentiometer values to PWM value.
int pwmVal1 = map(potVal1,0, 1023, 1100, 1900); // maps potentiometer values to PWM value.
servo.writeMicroseconds(pwmVal); // Send signal to ESC.
servo1.writeMicroseconds(pwmVal1); // Send signal to ESC.
}
