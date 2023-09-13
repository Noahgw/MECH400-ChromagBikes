#include <SparkFun_I2C_Mux_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_I2C_Mux
#include <Wire.h>
#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU7802
#include <HCSR04.h> 

QWIICMUX myMux;
byte loadCellOneAddress = 4;
byte loadCellTwoAddress = 3;
const int RELAY_PIN1 = 6;  // the Arduino pin, which connects to the IN1 pin of relay (controls solenoid 1)
const int RELAY_PIN2 = 7;  // the Arduino pin, which connects to the IN2 pin of relay (controls solenoid 2)
const int RELAY_PIN3 = 8;  // the Arduino pin, which connects to the IN3 pin of relay (controls solenoid 3)
const int RELAY_PIN4 = 9;  // the Arduino pin, which connects to the IN4 pin of relay (controls solenoid 4)



int32_t maximumTensileForceN = 150; //input required maximum tensile force
int32_t maximumCompressiveForceN = -150; //input required maximum compressive force
float loadCellCalibrationFactor = 57;

//const unsigned long demoRunTime = 30000;
//long timeOffset = 0;
int32_t cycleLoadCell1 = 0;
int32_t cycleLoadCell2 = 0;

//HCSR04 hc(26, 27); //initialisation class HCSR04 (trig pin , echo pin)

NAU7802 myScale; //Create instance of the NAU7802 class

void muxCheck(); //Check to see if the mux is communicating
void muxSensorCheck(); //Iterates through each sensor connected to the Mux to ensure they are communicating

void scaleCheck(); //Check to see if two Load Cells are communicating
void scaleRead(); //Read the current load Cell values and write back
void scaleTare(); //Calibrate the scale

void forceCheck(); //check to see if the force has reached its limit and switch direction if it has

int32_t calibratedScaleOffset [2] = {1,1}; //Holds the calibrated scale offset value for Scale 1 and Scale 2

//Initial Setup
void setup()
{
  Serial.begin(9600);
  while (! Serial);

  Serial.println("Waiting Serial Port to be Ready...");

  Wire.begin();
  Wire.setClock(400000); //Qwiic Scale is capable of running at 400kHz if desired
  Serial.println("Clock Speed set to 400kHz");

  muxCheck(); //Checks to see if the mux is communicating
  muxSensorCheck(); //Iterates through each sensor connected to the Mux to ensure they are communicating
  scaleTare(); //Calibrate Scale


//Initiate Run
  Serial.println("Calibration Complete... Press Key to Begin Test");
  while (Serial.available()) Serial.read(); //Clear anything in RX buffer
  while (Serial.available() == 0) delay(10); //Wait for user to press key
  timeOffset = millis();

  // initialize digital pins as an outputs.
  pinMode(RELAY_PIN1, OUTPUT); // Pushing Valve 
  pinMode(RELAY_PIN2, OUTPUT); // Pulling Valve
  pinMode(RELAY_PIN3, OUTPUT); // Pushing Valve 
  pinMode(RELAY_PIN4, OUTPUT); // Pulling Valve

  digitalWrite(RELAY_PIN2, HIGH); // Close Pulling valve
  digitalWrite(RELAY_PIN1, LOW); // Open pushing valve
  digitalWrite(RELAY_PIN4, HIGH); // Close Pulling valve
  digitalWrite(RELAY_PIN3, LOW); // Open pushing valve

  myMux.setPort(loadCellOneAddress); //Connect master to Load Cell #1

}

//Main Loop
void loop()
{
  //const unsigned long time = millis() - timeOffset;

  if (time < demoRunTime){

    forceCheck(); //check to see if the max tensile / compressive force has been reached and actuate if it has.
   
    } else {
        digitalWrite(RELAY_PIN2, HIGH); // Exhaust pulling valve
        digitalWrite(RELAY_PIN1, HIGH); // Exhaust pushing valve
        digitalWrite(RELAY_PIN4, HIGH); // Exhaust pulling valve
        digitalWrite(RELAY_PIN3, HIGH); // Exhaust pushing valve
        Serial.println("Demo complete... Freezing...");
        while(1);
    }
}

//Checks to see if the mux is calibrating
void muxCheck()
{
  if (myMux.begin() == false)
  {
    Serial.println("Mux not detected. Freezing...");
    while (1);
  }
  Serial.println("Mux detected");
}

//Check to see if the sensors connected to the mux are communicating.
void muxSensorCheck(){

  myMux.setPort(loadCellOneAddress); //Connect master to port Load Cell #1
  byte currentPortNumber = myMux.getPort();
  Serial.print("CurrentPort: ");
  Serial.println(currentPortNumber);
  scaleCheck();

  myMux.setPort(loadCellTwoAddress); //Connect master to port Load Cell #1
  currentPortNumber = myMux.getPort();
  Serial.print("CurrentPort: ");
  Serial.println(currentPortNumber);
  scaleCheck();
}

//Check to see if scale is communicating
void scaleCheck(){
  //Look for scale on the First Port
  Serial.println("Begin scanning for I2C devices");
  if (myScale.begin() == false)
    {
      Serial.println("Scale not detected. Please check wiring. Freezing...");
      while (1);
    } 
  else
      {
      Serial.println("Scale detected!");
      }
  }

//Calibrate the two Load Cells 
void scaleTare(){
  
  myMux.setPort(loadCellOneAddress);//Connect master to Load Cell #1

  myScale.setGain(NAU7802_GAIN_8); //old4/old2//Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.
  Serial.println("Gain set to 8");
  myScale.setSampleRate(NAU7802_SPS_320); //old80/old40//Sample rate can be set to 10, 20, 40, 80, or 320Hz
  Serial.println("Sample Rate set to 320 samples per sec");
  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.
  Serial.println("DoneCalibrateAFE");

  //Zero the system
  myScale.calculateZeroOffset(64); //Zero or Tare the scale. Average over 64 readings.
  Serial.print(F("New zero offset for Load Cell 1: "));
  Serial.println(myScale.getZeroOffset());
  calibratedScaleOffset[0] = myScale.getZeroOffset(); //Save offset to global array

  myMux.setPort(loadCellTwoAddress);//Connect master to Load Cell #2

  myScale.setGain(NAU7802_GAIN_8); //old4/old2//Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.
  Serial.println("Gain set to 8");
  myScale.setSampleRate(NAU7802_SPS_320); //old80/old40//Sample rate can be set to 10, 20, 40, 80, or 320Hz
  Serial.println("Sample Rate set to 320 samples per sec");
  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.
  Serial.println("DoneCalibrateAFE");

  //Zero the system
  myScale.calculateZeroOffset(64); //Zero or Tare the scale. Average over 64 readings.
  Serial.print(F("New zero offset for Load Cell 2: "));
  Serial.println(myScale.getZeroOffset());
  calibratedScaleOffset[1] = myScale.getZeroOffset(); //Save offset to global array

}

void forceCheck(){
    myMux.setPort(loadCellOneAddress);//Connect master to Load Cell #1
    int32_t currentForce = 0;
    currentForce = myScale.getAverage(4); //Take the average of 4 readings
    currentForce = (currentForce-calibratedScaleOffset[0])/loadCellCalibrationFactor; //Utilize the offset and the calibration factor for accurate readings
   

    if (currentForce > maximumTensileForceN) // When pulling force reaches required Force (N)
    {
      digitalWrite(RELAY_PIN1, HIGH); //Shut off pulling force
      digitalWrite(RELAY_PIN2, LOW); //Turn on pushing force
      //Serial.println(" N --> STARTED PUSHING...");
    } else if (currentForce < maximumCompressiveForceN) // When pushing force reaches required Force (N)
    {
      digitalWrite(RELAY_PIN2, HIGH); //Shut off pushing force
      digitalWrite(RELAY_PIN1, LOW); //Turn on pulling force
      //Serial.println(" N --> STARTED PULLING...");
    }
    Serial.print("Load Cell 1: ");
    Serial.println(currentForce);

    myMux.setPort(loadCellTwoAddress);//Connect master to Load Cell #2
    currentForce = myScale.getAverage(4); //Take the average of 4 readings
    currentForce = (currentForce-calibratedScaleOffset[1])/loadCellCalibrationFactor; //Utilize the offset and the calibration factor for accurate readings
   

    if (currentForce > maximumTensileForceN) // When pulling force reaches required force (N)
    {
      digitalWrite(RELAY_PIN3, HIGH); //Shut off pulling force
      digitalWrite(RELAY_PIN4, LOW); //Turn on pushing force

      //Serial.println(" N --> STARTED PUSHING...");
    } else if (currentForce < maximumCompressiveForceN) // When pushing force reaches required Force (N)
    {
      digitalWrite(RELAY_PIN4, HIGH); //Shut off pushing force
      digitalWrite(RELAY_PIN3, LOW); //Turn on pulling force

    //Serial.println(" N --> STARTED PULLING...");
    }
    //Serial.print("Load Cell #2: ");
    //Serial.println(currentForce);
}
