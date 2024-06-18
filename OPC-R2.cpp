/*******************************************************************************
 * @file    OPC-R2.cpp
 * @brief   Interfaces an Arduino & Alphasense OPC-R2
 *
 * @cite 	   Thanks to Joseph Habeck (habec021@umn.edu) from 6/24/18. 
 *                (https://github.com/JHabeck/Alphasense-OPC-N2/tre/master)
 *           Thanks to Marcel Oliveira for your github code  
 *                (https://github.com/shyney7/OPC-R2_ESP32/tree/main) 
 *           Thanks to Aidan Mobley for the initial version (Summer 2023)
 *
 * @editor  Percy Smith, percy.smith@colorado.edu
 * @date    June 18, 2024
 * @log     Includes all histogram data & calculating checksum 
 ******************************************************************************/

#include <stdio.h>
#include "OPC-R2.h"

/*************  Helper Functions  *************/
// Combine two bytes into a 16-bit unsigned int
uint16_t OPC::twoBytes2int(byte LSB, byte MSB){
  uint16_t int_val = ((MSB << 8) | LSB);
  return int_val;
}

// Return an IEEE754 float from an array of 4 bytes //cited from shyney7/OPC-R2_ESP32/blob/main/include/opcr2.h with update
float OPC::fourBytes2float(byte val0, byte val1, byte val2, byte val3) {
  uint8_t bytes[4] = {uint8_t(val0), uint8_t(val1), uint8_t(val2), uint8_t(val3)};
  // static_
  float result;
  memcpy(&result, bytes, 4);
  return result;
}

//Return a 32bit unsigned integer from four bytes
uint32_t _32bit_int(byte b0, byte b1, byte b2, byte b3) {
    return ((b3 << 24) | (b2 << 16) | (b1 << 8) | b0);
}
/*************  End Helper Functions  *************/


// Gets OPC ready to do something
bool OPC::getReady(const byte command){
  byte inData;
  SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE1));
  int tries = 0;
  int total_tries = 0;
  while(inData != OPC_ready & total_tries++ < 20){
    for(int i = 0; i < 10; i++){
      //0x01 acting as a dummy command
      inData = SPI.transfer(0x01);    // Try reading some bytes here to clear out anything remnant of other SPI activity
      delayMicroseconds(10);
    }
    delay(10);
    digitalWrite(CSpin, LOW);
    while(inData != OPC_ready & tries++ < 20)
    {
      inData = SPI.transfer(command);
      delay(10);  //command byte polling interval
    }
    if(inData != OPC_ready){
      if(inData == OPC_busy){         // waiting 2 seconds because opc is busy
        digitalWrite(CSpin, HIGH);
        delay(2000);
      }
      else{                           // resetting spi because different byte is returned
        digitalWrite(CSpin, HIGH);
        SPI.endTransaction();
        delay(3000);  //wait >2 s for comms to resume
        SPI.beginTransaction(SPISettings(750000, MSBFIRST, SPI_MODE1));
      }
    }
  }
  delay(10);
  if(inData == OPC_ready)
    return true;
  else
    return false;
}

OPC::OPC(){}

bool OPC::begin(){ //encapsulates getReady --> on 
  pinMode(CSpin, OUTPUT);
  digitalWrite(CSpin, HIGH);
  delayMicroseconds(50); //wait >10us (<100 us)
  return on();
}

bool OPC::on(){  //which power state does user wish to set? on flow chart
  bool on = getReady(0x03); //boolean condition - if failing then it's not ready but this is the first step in the chart
  digitalWrite(CSpin, LOW); //setting low to start comm
  SPI.transfer(0x03); //turns on laser & fan as peripherals
  delay(10); //command byte polling interval
  digitalWrite(CSpin, HIGH); //setting high when finishing receiving information
  SPI.endTransaction();
  delay(5500); //wait >5 (<10) seconds for flow to reach operating speed for the fans
  return on;
}

bool OPC::off(){
  bool off = getReady(0x03);
  SPI.transfer(0x00);
  digitalWrite(CSpin, HIGH);
  SPI.endTransaction();
  return off;
}

histogramData OPC::histogramFormatted(){
  histogramData data;
  byte rawComms[64];
  byte command = 0x30;

  //SPI send command
  pinMode(CSpin, OUTPUT);
  getReady(command); //prep for command
  digitalWrite(CSpin, LOW); //just in casse...
  delay(50); //wait >10 ms (<100 ms)

  // read all bytes available
  for (int i=0; i<64; ++i){
    rawComms[i] = SPI.transfer(command);
    data.rawComms[i] = rawComms[i];
    delayMicroseconds(50); //interval between bytes should be > 10us (<100 us)
  }

  // pausing SPI commands to process dataset for use & formatting in structure histogramData
  //bins 0 - 15 (Using MSB & LSB for each bin...) thus bytes 0 - 31
  for(int i = 0; i < 16; i++){
    data.bin[i] = twoBytes2int(rawComms[i*2], rawComms[(i*2)+1]);  //uses i*2 because rawComms idx vs data idx
      //[number of particles per bin over the sampling period]
  }
  delay(10);
  // time sampling Bin1, Bin3, Bin5, Bin7 thus bytes 32 - 35
  for(int j = 0; j < 4; j++){
    data.MToF[j] = rawComms[j + 32]; //should convert from byte to explicitly 8 bit unsigned 
      // [1/3 us]
  }
  delay(10);
  // flow rate thus bytes 36 - 39
  data.sampleflowrate = fourBytes2float(rawComms[36], rawComms[37], rawComms[38], rawComms[39]);
    // mL/s
  delay(10);
  // Temperature signal thus bytes 40 - 41... but!!
  // SPI would output MSB first so maybe we flip indices bc twoBytes2int(LSB, MSB)
  data.signal_temp = twoBytes2int(rawComms[40], rawComms[41]);
    // units of signal i guess
  delay(10);
  float x = data.signal_temp;
  // Temperature data running calculations from manual 2
  data.T_C =  -45. + 175. * x / (65535.0);
    // [degrees Celsius]
  delay(10);
  // Relative Humidity signal thus bytes 42 - 43... but!!
  // SPI would output MSB first so maybe we flip indices bc twoBytes2int(LSB, MSB)
  data.signal_relhum = twoBytes2int(rawComms[43], rawComms[42]);
    // units of signal i guess
  delay(10);
  // Relative Humidity data running calculations from manual 2
  float y = data.signal_relhum;
  data.RH = 100.*y/(65535.0);
    // [%RH]
  delay(10);
  // sampling period thus bytes 44 - 47
  data.samplingperiod = fourBytes2float(rawComms[44], rawComms[45], rawComms[46], rawComms[47]);
    // [s]
  delay(10);
  // reject variables to indicate counts of bad data along axes thus bytes 48 & 49
  data.reject[0] = rawComms[48];
    // [count Glitch] (particle count/y axis)
  delay(10);
  data.reject[1] = rawComms[49];
    // [count Long] (time/t axis)
  delay(10);
  // PM concentrations therefore bytes 50 - 61 split into diff "classes"
  // PM_A (1.0um) as bytes 50 - 53
  data.PM_ENV[0] = fourBytes2float(rawComms[50], rawComms[51], rawComms[52], rawComms[53]);
    // [ug/m^3]
  delay(10);
  // PM_B (2.5um) as bytes 54 - 57
  data.PM_ENV[1] = fourBytes2float(rawComms[54], rawComms[55], rawComms[56], rawComms[57]);
    // [ug/m^3]
  delay(10);
  // PM_C (10um) as bytes 58 - 61
  data.PM_ENV[2] = fourBytes2float(rawComms[58], rawComms[59], rawComms[60], rawComms[61]);
    // [ug/m^3]
  delay(10);
  // Checksum variables therefore bytes 62 - 63
  data.checksum = twoBytes2int(rawComms[63], rawComms[62]);
  delay(10);


  uint16_t sum = initial_CCS; // I think we ignore checksum itself thus 0 - 61 
  for(int c = 0; c < 64; c++)
  {
    sum ^= rawComms[c];
    for(int b = 0; b < 8; b++)
    {
      if (sum & 1)
      {
        sum >>=1;
        sum^= POLYNOMIAL;
      }
      else
      {
        sum >>=1;
      }
    }
  }
  data.verifycheck = sum;

  delay(10);

  return data;
}


