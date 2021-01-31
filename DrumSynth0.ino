//-----------------------------------------------------------------------------
// Copyright 2020 Peter Balch
// MIDI tester
//   subject to the GNU General Public License
//   accepts byte from serial
//   sends byte to a VS1053 over SPI
//-----------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>

//-----------------------------------------------------------------------------
// Defines, constants and Typedefs
//-----------------------------------------------------------------------------

// MIDI pins
const int MIDI_RST   = 3;
const int MIDI_CLK   = 13;
const int MIDI_CS    = 4;

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------
// MidiSPItransfer
//-------------------------------------------------------------------------
void MidiSPItransfer(byte b)
{
  digitalWrite(MIDI_CS, LOW);
  SPI.transfer(b);
  SPI.transfer(0x00); // to read result
  digitalWrite(MIDI_CS, HIGH);
}

//-------------------------------------------------------------------------
// GetSerial
//-------------------------------------------------------------------------
byte GetSerial() {
  while ( Serial.available() == 0 ) ;  
  return Serial.read();
}

//-------------------------------------------------------------------------
// setup
//-------------------------------------------------------------------------
void setup(void)
{ 
  Serial.begin(57600);
  Serial.println("DrumSynth");

  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(MIDI_RST, OUTPUT);
  pinMode(MIDI_CLK, OUTPUT);
  pinMode(MIDI_CS, OUTPUT);

  digitalWrite(MIDI_CS, HIGH);
  digitalWrite(MIDI_RST, LOW);
  delay(10);
  digitalWrite(MIDI_RST, HIGH);

  SPI.begin ();
  SPI.setBitOrder(MSBFIRST);
}

//-----------------------------------------------------------------------------
// Main routines
// loop
//-----------------------------------------------------------------------------
void loop(void)
{
  if ( Serial.available() > 0 )
    MidiSPItransfer(GetSerial());
}
