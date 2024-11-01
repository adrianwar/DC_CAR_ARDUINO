#include <IRremote.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define Motor_left_Pin_1 5
#define Motor_left_Pin_2 3
#define Motor_left_Enable 6
#define Motor_right_Pin_1 10
#define Motor_right_Pin_2 11
#define Motor_right_Enable 13
#define DRV8833_Enable_pin 6
#define SR04_Trig_Pin 9
#define SR04_Echo_Pin 12
#define IR_RECEIVE_PIN 4
#define SETUP_Button 2
#define Tester_pin 7

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int IR_INTERVAL = 50;
int min_distance = 50;

int Forward_Ir = 88;
int Backward_Ir = 89;
int Left_Ir = 90;
int Right_Ir = 91;
int Mode_Change_Ir = 92;
int received = 0;

bool Setup_button;

byte Car_mode = 1; 

int Used_pins[7] = {3, 5, 6, 9, 10, 11, 13};
//prototype 
double distance();
void Motor_Control(int PWM_Motor_Left, int PWM_Motor_Right, bool Direction_control, bool Right_motor_opposite_direction = 0);
void stop();
void search(int speed);
void setup_IR_control();
void IR_control(int received);
int IR_RECIVER();
void display_control(const char message[], int x = 0, int y = 0, byte size = 1);

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  delay(2000);
  display.clearDisplay();
  display_control("Setup begin!", 0, 0, 1);

  pinMode(Tester_pin, INPUT);
  pinMode(DRV8833_Enable_pin, OUTPUT);
  digitalWrite(DRV8833_Enable_pin, HIGH);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  for (int i = 0; i < sizeof(Used_pins) / sizeof(Used_pins[0]); i++) {
    pinMode(Used_pins[i], OUTPUT);
  }
  
  pinMode(SR04_Echo_Pin, INPUT);
  pinMode(SETUP_Button, INPUT);
  
  delay(500);
  display_control("Setup complete", 0, 10, 1);
}

unsigned long prev_IR_scan = 0;
unsigned long actual_time = 0;

void loop() {
  actual_time = millis();

  if (actual_time - prev_IR_scan >= IR_INTERVAL) {
    if (IrReceiver.decode()) {
      received = IR_RECIVER();
      prev_IR_scan = actual_time;
    }
    if (digitalRead(SETUP_Button)) {
      setup_IR_control();
    }
  }

  if (Car_mode == 1) {
    double current_distance = distance();
    if (current_distance >= min_distance) {
      Motor_Control(100, 100, 1);
      display_control("Automatic Mode: Moving forward", 0, 20, 1);
    } else {
      search(100);
      display_control("Automatic Mode: Searching path", 0, 20, 1);
    }
  } else {
    display_control("Manual Mode", 0, 20, 1);
    if (received == Forward_Ir) {
      Motor_Control(255, 255, 1);
      display_control("Moving Forward", 0, 30, 1);
      received = 0; 
    } 
    else if (received == Backward_Ir) {
      Motor_Control(255, 255, 0);
      display_control("Moving Backward", 0, 30, 1);
      received = 0;
    } 
    else if (received == Left_Ir) {
      Motor_Control(0, 255, 1);
      display_control("Turning Left", 0, 30, 1);
      received = 0; 
    } 
    else if (received == Right_Ir) {
      Motor_Control(255, 0, 1);
      display_control("Turning Right", 0, 30, 1);
      received = 0; 
    } 
    else {
      stop();
      display_control("Stopped", 0, 30, 1);
    }
  }

  if (received == Mode_Change_Ir) {
    Car_mode = (Car_mode == 1) ? 2 : 1;
    display_control((Car_mode == 1) ? "Switched to AUTO Mode" : "Switched to MANUAL Mode", 0, 40, 1);
    delay(500);
    received = 0; 
  }

  delay(50);
}

double distance() {
  digitalWrite(SR04_Trig_Pin, LOW);
  delayMicroseconds(2);
  digitalWrite(SR04_Trig_Pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(SR04_Trig_Pin, LOW);
  long duration = pulseIn(SR04_Echo_Pin, HIGH);
  return (duration * 0.034) / 2;
}

void Motor_Control(int PWM_Motor_Left, int PWM_Motor_Right, bool Direction_control, bool Right_motor_opposite_direction) {
  if (!Right_motor_opposite_direction) {
    if (Direction_control) {
      analogWrite(Motor_left_Pin_1, PWM_Motor_Left);
      digitalWrite(Motor_left_Pin_2, LOW);
      analogWrite(Motor_right_Pin_1, PWM_Motor_Right);
      digitalWrite(Motor_right_Pin_2, LOW);
    } else {
      digitalWrite(Motor_left_Pin_1, LOW);
      analogWrite(Motor_left_Pin_2, PWM_Motor_Left);
      digitalWrite(Motor_right_Pin_1, LOW);
      analogWrite(Motor_right_Pin_2, PWM_Motor_Right);
    }
  } else {
    if (Direction_control) {
      analogWrite(Motor_left_Pin_1, PWM_Motor_Left);
      digitalWrite(Motor_left_Pin_2, LOW);
      digitalWrite(Motor_right_Pin_1, LOW);
      analogWrite(Motor_right_Pin_2, PWM_Motor_Right);
    } else {
      digitalWrite(Motor_left_Pin_1, LOW);
      analogWrite(Motor_left_Pin_2, PWM_Motor_Left);
      digitalWrite(Motor_right_Pin_1, PWM_Motor_Right);
      analogWrite(Motor_right_Pin_2, LOW);
    }
  }
}

void stop() {
  Motor_Control(0, 0, 1);
}

void search(int speed) {
  while (distance() < min_distance) {
    Motor_Control(speed, speed, 1, 1);
    delay(100);
    if (digitalRead(SETUP_Button)) {
      setup_IR_control();
      break;
    }
  }
  stop();
}

void setup_IR_control() {
  display_control("Setup Mode", 0, 0, 1);
  Serial.println("Entering setup mode to change IR controls.");
  

  display_control("Press button for Forward", 0, 10, 1);
  Serial.println("Press the button for Forward command:");
  while (!IrReceiver.decode()); 
  Forward_Ir = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Forward command set to: ");
  Serial.println(Forward_Ir);
  display_control("Forward set!", 0, 20, 1);
  delay(1000);
  IrReceiver.resume();
  received = 0; 
  

  display_control("Press button for Backward", 0, 10, 1);
  Serial.println("Press the button for Backward command:");
  while (!IrReceiver.decode());
  Backward_Ir = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Backward command set to: ");
  Serial.println(Backward_Ir);
  display_control("Backward set!", 0, 20, 1);
  delay(1000);
  IrReceiver.resume();
  received = 0; 
  
 
  display_control("Press button for Left", 0, 10, 1);
  Serial.println("Press the button for Left command:");
  while (!IrReceiver.decode());
  Left_Ir = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Left command set to: ");
  Serial.println(Left_Ir);
  display_control("Left set!", 0, 20, 1);
  delay(1000);
  IrReceiver.resume();
  received = 0; // 
  

  display_control("Press button for Right", 0, 10, 1);
  Serial.println("Press the button for Right command:");
  while (!IrReceiver.decode());
  Right_Ir = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Right command set to: ");
  Serial.println(Right_Ir);
  display_control("Right set!", 0, 20, 1);
  delay(1000);
  IrReceiver.resume();
  received = 0;


  display_control("Press button for Mode Change", 0, 10, 1);
  Serial.println("Press the button for Mode Change command:");
  while (!IrReceiver.decode());

  Mode_Change_Ir = IrReceiver.decodedIRData.decodedRawData;
  Serial.print("Mode Change command set to: ");
  Serial.println(Mode_Change_Ir);
  display_control("Mode Change set!", 0, 20, 1);
  delay(1000);
  IrReceiver.resume();
  received = 0; // 


  display_control("IR setup complete", 0, 30, 1);
  Serial.println("IR controls setup complete.");
  delay(2000);
  display.clearDisplay();
}


int IR_RECIVER() {
  int received = IrReceiver.decodedIRData.decodedRawData;
  IrReceiver.resume();
  return received;
}

void display_control(const char message[], int x, int y, byte size) {
  display.clearDisplay();
  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(message);
  display.display();
}
