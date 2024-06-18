/*******************************************************************************
 * @file    OPC-R2.h
 * @brief   Interfaces an Arduino & Alphasense OPC-R2
 *
 * @cite 	   Thanks to Joseph Habeck (habec021@umn.edu) from 6/24/18. 
 *                (https://github.com/JHabeck/Alphasense-OPC-N2/tre/master)
 *           Thanks to Marcel Oliveira for your github code  
 *                (https://github.com/shyney7/OPC-R2_ESP32/tree/main) 
 *           Thanks to Aidan Mobley for the initial version (Summer 2023)
 *
 * @editor  Percy Smith, percy.smith@colorado.edu
 * @date    June 17, 2024
 * @log     Includes all histogram data & calculating checksum  
 ******************************************************************************/

#ifndef OPCR2_h
#define OPCR2_h

// include Arduino SPI library
#include <SPI.h>

// communication settings
const byte OPC_ready = 0xF3;
const byte OPC_busy = 0x31;
// settings for checksum calculations
const uint16_t POLYNOMIAL = 0xA001;
const uint16_t initial_CCS = 0xFFFF;

// ALL histogram data structure
struct histogramData{
  byte rawComms[64];
  uint16_t bin[16]; //16 bins (0-15) transferred as 16 bit unsigned integers [particles]
  uint8_t MToF[4]; //4 bins (1,3,5,7) transferred as 8 bit unsigned integers [1/3 us]
  float sampleflowrate;  //flow rate at measurement transferred as float occupying 4 bytes [mL/s]
  uint16_t signal_temp; //temperature transferred as 16 bit unsigned integer [signal]
    float T_C;  //Temperature after running copnversion from pg 4 of manual 2 [degrees Celsius]
  uint16_t signal_relhum;  //relative humidity transferred as 16 bit unsigned integer [signal]
    float RH; //Relative Humidity after running conversion from pg 4 of manual 2 [%]
  float samplingperiod; //measures histograms' sampling period in seconds as float variable occupying 4 bytes [s]
  uint8_t reject[2]; //reject glitch and reject long transferred as 8 bit unsigned integers [?]
  float PM_ENV[3];  //PM_A, PM_B, PM_C (1.0, 2.5, 10.0) as float variable occupying 4 bytes [ug/m^3]
  uint16_t checksum; //checksum integer as 16 bit unsigned integer
  uint16_t verifycheck; //verified/calculated checksum integer to create a 16 bit unsigned integer

};

// define OPC class
class OPC{
  public:
    OPC();
    bool begin();
    bool on();
    bool off();
    histogramData histogramFormatted();
  
  private:
    uint16_t twoBytes2int(byte LSB, byte MSB);
    float fourBytes2float(byte val0, byte val1, byte val2, byte val3);
    bool getReady(const byte command);
    int CSpin = 9;
};


#endif /* OPCR2_h */
