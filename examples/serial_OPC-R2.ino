/*******************************************************************************
 * @project OPC-R2 Library for Arduino
 *
 * @file    serial_OPC-R2.ino
 * @version V4.0 
 * 
 * @brief   This is a main file meant to run the OPC-R2 using SPI with an UNO
 *          To see further details on hardware, please see the github 
 *
 * @author 	Percy Smith
 * @date 	  June 18, 2024
******************************************************************************/

/*************  Included Libraries  *************/
#include <SPI.h> 
#include "OPC-R2.h"

/*************  Global Declarations  *************/
OPC opc;
histogramData data;

void setup() {
  Serial.begin(9600);
  Serial.println("Initialized Serial Monitor");
  
  SPI.begin();
  
  opc.begin();
  if(!opc.begin()){
      #if SERIAL_LOG_ENABLED
        Serial.println("Error: Failed to initialize OPC!");
      #endif
    }
  opc.on();
}

void loop() {
  if(!opc.begin())  {
      Serial.println("Cannot find OPC");
      opc.begin();
    }
    else
    {
      data = opc.histogramFormatted();
    }

  // this is all the serial printing for testing
  for(int i = 0; i < 16; i++){
    Serial.print("Bin " + String(i) + ": " + String(data.bin[i]) + ",");
    delay(10);
  }
  Serial.print("\n  ");
  for(int j = 0; j < 4; j++){
    Serial.print("MToF " + String(j*2+1) + ": " + String(data.MToF[j]) + ",");
    delay(10);
  }
  delay(10);
  Serial.print("\n  ");
  Serial.print("Byte T: " + String(data.rawComms[41]) + String(data.rawComms[40]) + ",");
  Serial.print("Temperature: " + String(data.T_C) + "(" + String(data.signal_temp) + "),");
  delay(10);
  Serial.print("Byte RH: " + String(data.rawComms[43]) + String(data.rawComms[42]) + ",");
  Serial.print("Percent Relative Humidity: " + String(data.RH) + "(" + String(data.signal_relhum) + "),");
  delay(10);
  Serial.print("\n  ");
  Serial.print("Sample Flow Rate: " + String(data.sampleflowrate) + ",");
  delay(10);
  Serial.print("Sample Period: " + String(data.samplingperiod) + ",");
  delay(10);
  Serial.print("\n  ");
  Serial.print("Reject count Glitch: " + String(data.reject[0]) + ",");
  delay(10);
  Serial.print("Reject count Long: " + String(data.reject[1]) + ",");
  delay(10);
  Serial.print("\n  ");
  Serial.print("PM_A (PM1.0): " +  String(data.PM_ENV[0]) + ",");
  delay(10);
  Serial.print("PM_B (PM2.5): " + String(data.PM_ENV[1]) + ",");
  delay(10);
  Serial.print("PM_C (PM10.0): " + String(data.PM_ENV[2]) + ",");
  delay(10);
  Serial.print("\n  ");
  Serial.print("Checksum: " + String(data.checksum) + ",");
  Serial.println("Verify Checksum: " + String(data.verifycheck) + ",");
}
