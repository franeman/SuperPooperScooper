// Super Pooper Scooper

#include <Wire.h> // Used for I2C connection for the clock

// ***********************RTC********************************

// Define I2C pins
// Make sure to connect a 10k resister to 5v for SCL and SDA individually as this is required for I2C
#define SCL A5 // Clock line for I2C
#define SDA A4 // Data line for I2C

//Define Clock address
#define clockAddr 0b1101111 // The address for the clock is binary 1101000, 0b signifies a binary number

// Define the time
#define MONTH 1 // The month to set the clock to
#define DATE 1 // The date to set the clock to
#define YEAR 18 // The year to set the clock to (00-99)
#define DOW 1 // Day of the week to set the clock to
#define HOURS 00 // Hour to set the clock to (in 24hr clock)
#define MINUTES 00 // Minute to set the clock to
#define SECONDS 00 // Second to set the clock to


void setup()
{
  Wire.begin(); // Initialize I2C
  SetupClock(MONTH, DATE, YEAR, DOW, HOURS, MINUTES, SECONDS); // Set the clock date, The date will be set to the same date every time it is ran, however this is irrelevent as the user does not need the date and is only used to wake up the arduino

  attachInterrupt(1, RTCWakeup, FALLING); // Attaches the interupt to int1 (digital pin 3) for RTC wakeup
  
}

void loop()
{
  SetRTCAlarm(); // Set the RTC to alarm in 10 minutes
  Sleep(); // Go to sleep 
}

byte BCDtoDecimal(byte bcd) // Converts a 1 byte binary coded decimal number to its equivalent decimal number
{
  byte tens = (bcd & 0b11110000) >> 4; // Get the tens digit
  byte ones = bcd & 0b00001111; // Get the ones digit
  byte result = (tens * 10) + ones; // Add them to get the decimal equivalent
  return result;
}

byte DecimalToBCD(byte decimal)
{
  byte tens, ones, result;
  if (decimal >= 10)
  {
    tens = decimal / 10; // Get the value in the tens place
    ones = decimal - (tens * 10); // Get the value in the ones place
    tens = tens << 4; // Shift the binary value of the tens place into the tens position for BCD

  }
  else
  {
    tens = 0;
    ones = decimal;
  }

  result = tens | ones; // or both values together to get the complete BCD value
  return result;
}
byte GetOneByte(byte addr, byte reg) // Since all clock data is only 1 byte long, we only need to request 1 byte of data at a time
// addr is the address of the device and reg is the register that the data is held in
{
  Wire.beginTransmission(addr); // Begin communication with the device
  Wire.write(reg); // Specify which register to retrive date from
  Wire.endTransmission(); // Stop sending to the device

  Wire.requestFrom(addr, 1); // Request 1 byte of data from the device
  while (Wire.available() == 0); // Wait until there is data being sent
  byte data = Wire.read(); // Read the data being recived

  return data;
}

void WriteI2C(byte addr, byte reg, byte data) // Writes data to the register of the device
{
  Wire.beginTransmission(addr); // Begin communication with the device
  Wire.write(reg); // Specify the register to write to
  Wire.write(data); // Write the data
  Wire.endTransmission(); // Stop sending to the device
}

void RTCWakeup()
{
  WriteI2C(clockAddr,0x0D,0); // Clear the alarm flag on the RTC
  SMCR &= ~1; // Disable sleep
}

void SetupClock(byte month, byte date, byte year, byte dow, byte hour, byte minute, byte second) // Set the time for the clock
{
  WriteI2C(clockAddr, 0x06, DecimalToBCD(year)); // Write the year to the clock after being converted to BCD
  WriteI2C(clockAddr, 0x05, DecimalToBCD(month)); // Write the month to the clock after being converted to BCD
  WriteI2C(clockAddr, 0x04, DecimalToBCD(date)); // Write the date to the clock after being converted to BCD
  WriteI2C(clockAddr, 0x03, DecimalToBCD(dow) | (1<<3)); // Write the day of the week to the clock after being converted to BCD and enable the battery backup
  WriteI2C(clockAddr, 0x02, DecimalToBCD(hour)); // Write the hour to the clock after being converted to BCD
  WriteI2C(clockAddr, 0x01, DecimalToBCD(minute)); // Write the minute to the clock after being converted to BCD
  WriteI2C(clockAddr, 0x00, DecimalToBCD(second) | (1<<7)); // Write the second to the clock after being converted to BCD and start the oscillator
}

void SetRTCAlarm()
{
  WriteI2C(clockAddr,0x07,(1<<4)); // Clear the control register and set ALM0EN
  WriteI2C(clockAddr,0x0D, (1<<4)); // Set the alarm mask for match of minutes
  
  byte minutes = BCDtoDecimal(GetOneByte(clockAddr,0x01)); // Get the current minutes time
  if ((minutes + 10) < 60) // if the next 10 minutes won't change the hour 
    {
       minutes += 10; // increment 10 minutes
    }
  else
    {
      minutes -= 50; // decrement 50 minutes 
    }
  minutes = DecimalToBCD(minutes); // Convert back to BCD
  
  WriteI2C(clockAddr,0x0B,minutes); // Set the alarm minutes
}

void Sleep()
{
  SMCR = 0b00000101; // Enable sleep mode and set it to Power Down mode
  
  __asm__ __volatile__("sleep"); // Put the arduino to sleep  
}
