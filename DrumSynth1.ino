//-----------------------------------------------------------------------------
// Copyright 2020 Peter Balch
// MIDI Drum Sequencer
//   subject to the GNU General Public License
//   draws screen
//   sends commands to a VS1053 over SPI
//-----------------------------------------------------------------------------

#include <Arduino.h>
#include "SimpleILI9341.h"
#include <avr/pgmspace.h>
#include <SPI.h>
#include <EEPROM.h>

//-----------------------------------------------------------------------------
// Defines, constants and Typedefs
//-----------------------------------------------------------------------------

//#define Is480x320

#ifdef Is480x320
  const int bw = 36; // left hand button width
  const int cw = 26; // checkbox width
  const int hgt = 32; // checkbox height
  const int keywidth = 9; // width of a "piano" key
  const int VBarWidth = 12; // for sliders
  const int HBarLeft = 6; // for sliders
  const int HBarTop = 11; // for sliders
  const int SetupMargin = 90; // for setup screen
  const int SetupSquare = 30; // for setup screen
#else
  const int bw = 24; // left hand button width
  const int cw = 17; // checkbox width
  const int hgt = 24; // checkbox height
  const int keywidth = 6; // width of a "piano" key
  const int VBarWidth = 8; // for sliders
  const int HBarLeft = 4; // for sliders
  const int HBarTop = 8; // for sliders
  const int SetupMargin = 60; // for setup screen
  const int SetupSquare = 20; // for setup screen
#endif

const int TempoRectLeft = 0;
const int TempoRectTop = (tft_height-hgt);
const int TempoRectRight = (tft_width / 2);
const int TempoRectBottom = tft_height;

const int KeyboardRectLeft = TempoRectRight;
const int KeyboardRectTop = TempoRectTop;
const int KeyboardRectRight = tft_width-1;
const int KeyboardRectBottom = TempoRectBottom;

const int KeyboardRect2Left = (KeyboardRectLeft+hgt / 2+4);
const int KeyboardRect2Top = KeyboardRectTop;
const int KeyboardRect2Right = (KeyboardRectRight-hgt / 2-4);
const int KeyboardRect2Bottom = KeyboardRectBottom;

const int VoiceRectLeft = (bw+1);
const int VoiceRectRight = ((VoiceRectLeft+tft_width) / 2);

const int VolumeRectLeft = VoiceRectRight;
const int VolumeRectRight = tft_width;

const int TempoMax = 400;
const int TempoMin = 100;
const int VolumeMax = 127;
const int VolumeMin = 0;

// MIDI pins
const int MIDI_RST   = 3;
const int MIDI_CLK   = 13;
const int MIDI_CS    = 4;

// Display pins
const int TFT_CS    = 10;
const int TFT_CD    = 8;
const int TFT_RST   = 9;
const int TOUCH_CS  = 5;

// Encoder pins
#define HasEncoder
#ifdef HasEncoder
  const int ENCODER_A = 7;
  const int ENCODER_B = 6;
  const int ENCODER_Btn = 2;
#endif

const int WholeKbd = 254;
const int NumSetups  = 6;

const int Version = 1;

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

const uint16_t ColumnColor[4] = { TFT_RED, RGB(0xFF, 0x80, 0), TFT_YELLOW, TFT_WHITE};
const uint16_t LightColor[4] = {TFT_RED | 0x8410, RGB(0xFF, 0xC0, 0x80), TFT_YELLOW | 0x8410, TFT_WHITE | 0x8410};

const byte kbdNoteOffset[12] = {0, 10, 1, 11, 2, 3, 13, 4, 14, 5, 15, 6};
const uint16_t kbdNoteColor[11] = {
  RGB(0x5F, 0x5F, 0xFF),
  RGB(0x5F, 0xAF, 0xFF),
  RGB(0x5F, 0xFF, 0xFF),
  RGB(0x5F, 0xFF, 0xAF),
  RGB(0x5F, 0xFF, 0x5F),
  RGB(0xAF, 0xFF, 0x5F),
  RGB(0xFF, 0xFF, 0x5F),
  RGB(0xFF, 0xAF, 0x5F),
  RGB(0xFF, 0x5F, 0x5F),
  RGB(0xFF, 0x5F, 0xAF),
  RGB(0xFF, 0x5F, 0xFF)
};

byte VoiceRectTop, VoiceRectBottom, VolumeRectTop, VolumeRectBottom, CurSetup;

const char In_0[] PROGMEM = "BDAcoustic Bass Drum";
const char In_1[] PROGMEM = "BDBass Drum 1";
const char In_2[] PROGMEM = "SSSide Stick/Rimshot";
const char In_3[] PROGMEM = "SnAcoustic Snare";
const char In_4[] PROGMEM = "HCHand Clap";
const char In_5[] PROGMEM = "ESElectric Snare";
const char In_6[] PROGMEM = "LFLow Floor Tom";
const char In_7[] PROGMEM = "CHClosed Hi-hat";
const char In_8[] PROGMEM = "HFHigh Floor Tom";
const char In_9[] PROGMEM = "PHPedal Hi-hat";
const char In_10[] PROGMEM = "LTLow Tom";
const char In_11[] PROGMEM = "OHOpen Hi-hat";
const char In_12[] PROGMEM = "LMLow-Mid Tom";
const char In_13[] PROGMEM = "HMHi-Mid Tom";
const char In_14[] PROGMEM = "CyCrash Cymbal 1";
const char In_15[] PROGMEM = "HTHigh Tom";
const char In_16[] PROGMEM = "RCRide Cymbal 1";
const char In_17[] PROGMEM = "CCChinese Cymbal";
const char In_18[] PROGMEM = "RBRide Bell";
const char In_19[] PROGMEM = "TaTambourine";
const char In_20[] PROGMEM = "SCSplash Cymbal";
const char In_21[] PROGMEM = "CoCowbell";
const char In_22[] PROGMEM = "CyCrash Cymbal 2";
const char In_23[] PROGMEM = "VSVibra Slap";
const char In_24[] PROGMEM = "RCRide Cymbal 2";
const char In_25[] PROGMEM = "HBHigh Bongo";
const char In_26[] PROGMEM = "LBLow Bongo";
const char In_27[] PROGMEM = "MHMute High Conga";
const char In_28[] PROGMEM = "OHOpen High Conga";
const char In_29[] PROGMEM = "LCLow Conga";
const char In_30[] PROGMEM = "TmHigh Timbale";
const char In_31[] PROGMEM = "TmLow Timbale";
const char In_32[] PROGMEM = "HAHigh Agogo";
const char In_33[] PROGMEM = "LALow Agogo";
const char In_34[] PROGMEM = "CaCabasa";
const char In_35[] PROGMEM = "MaMaracas";
const char In_36[] PROGMEM = "SWShort Whistle";
const char In_37[] PROGMEM = "LWLong Whistle";
const char In_38[] PROGMEM = "SGShort Guiro";
const char In_39[] PROGMEM = "LGLong Guiro";
const char In_40[] PROGMEM = "ClClaves";
const char In_41[] PROGMEM = "HWHigh Wood Block";
const char In_42[] PROGMEM = "WBLow Wood Block";
const char In_43[] PROGMEM = "MCMute Cuica";
const char In_44[] PROGMEM = "OCOpen Cuica";
const char In_45[] PROGMEM = "TgMute Triangle";
const char In_46[] PROGMEM = "OTOpen Triangle";
const char In_47[] PROGMEM = "GPGrand Piano";
const char In_48[] PROGMEM = "APBright Piano";
const char In_49[] PROGMEM = "EPElectric Piano";
const char In_50[] PROGMEM = "HPHonky-tonk Piano";
const char In_51[] PROGMEM = "EPElectric Piano 1";
const char In_52[] PROGMEM = "EPElectric Piano 2";
const char In_53[] PROGMEM = "HaHarpsichord";
const char In_54[] PROGMEM = "CvClavi";
const char In_55[] PROGMEM = "CsCelesta";
const char In_56[] PROGMEM = "GlGlockenspiel";
const char In_57[] PROGMEM = "MBMusic Box";
const char In_58[] PROGMEM = "VbVibraphone";
const char In_59[] PROGMEM = "MaMarimba";
const char In_60[] PROGMEM = "XyXylophone";
const char In_61[] PROGMEM = "BeTubular Bells";
const char In_62[] PROGMEM = "DuDulcimer";
const char In_63[] PROGMEM = "DODrawbar Organ";
const char In_64[] PROGMEM = "POPercussive Organ";
const char In_65[] PROGMEM = "RoRock Organ";
const char In_66[] PROGMEM = "COChurch Organ";
const char In_67[] PROGMEM = "ROReed Organ";
const char In_68[] PROGMEM = "AcAccordion";
const char In_69[] PROGMEM = "HrHarmonica";
const char In_70[] PROGMEM = "TATango Accordion";
const char In_71[] PROGMEM = "AGAc Guitar nylon";
const char In_72[] PROGMEM = "AGAc Guitar steel";
const char In_73[] PROGMEM = "EGEl Guitar jazz";
const char In_74[] PROGMEM = "EGEl Guitar clean";
const char In_75[] PROGMEM = "EGEl Guitar muted";
const char In_76[] PROGMEM = "OGOverdriven Guitar";
const char In_77[] PROGMEM = "DGDistortion Guitar";
const char In_78[] PROGMEM = "GHGuitar Harmonics";
const char In_79[] PROGMEM = "ABAcoustic Bass";
const char In_80[] PROGMEM = "EBEl Bass (finger)";
const char In_81[] PROGMEM = "EBEl Bass (pick)";
const char In_82[] PROGMEM = "FBFretless Bass";
const char In_83[] PROGMEM = "SBSlap Bass 1";
const char In_84[] PROGMEM = "SBSlap Bass 2";
const char In_85[] PROGMEM = "SBSynth Bass 1";
const char In_86[] PROGMEM = "SBSynth Bass 2";
const char In_87[] PROGMEM = "ViViolin";
const char In_88[] PROGMEM = "VaViola";
const char In_89[] PROGMEM = "CeCello";
const char In_90[] PROGMEM = "CbContrabass";
const char In_91[] PROGMEM = "TSTremolo Ins";
const char In_92[] PROGMEM = "PSPizzicato Ins";
const char In_93[] PROGMEM = "OHOrchestral Harp";
const char In_94[] PROGMEM = "TiTimpani";
const char In_95[] PROGMEM = "SEIn Ensembles 1";
const char In_96[] PROGMEM = "SEIn Ensembles 2";
const char In_97[] PROGMEM = "SSSynth Ins 1";
const char In_98[] PROGMEM = "SSSynth Ins 2";
const char In_99[] PROGMEM = "CAChoir Aahs";
const char In_100[] PROGMEM = "VoVoice oohs";
const char In_101[] PROGMEM = "SVSynth Voice";
const char In_102[] PROGMEM = "OHOrchestra Hit";
const char In_103[] PROGMEM = "TrTrumpet";
const char In_104[] PROGMEM = "TbTrombone";
const char In_105[] PROGMEM = "TuTuba";
const char In_106[] PROGMEM = "MTMuted Trumpet";
const char In_107[] PROGMEM = "FHFrench Horn";
const char In_108[] PROGMEM = "BSBrass Section";
const char In_109[] PROGMEM = "SBSynth Brass 1";
const char In_110[] PROGMEM = "SBSynth Brass 2";
const char In_111[] PROGMEM = "SoSoprano Sax";
const char In_112[] PROGMEM = "ASAlto Sax";
const char In_113[] PROGMEM = "TSTenor Sax";
const char In_114[] PROGMEM = "SxBaritone Sax";
const char In_115[] PROGMEM = "ObOboe";
const char In_116[] PROGMEM = "EHEnglish Horn";
const char In_117[] PROGMEM = "BaBassoon";
const char In_118[] PROGMEM = "ClClarinet";
const char In_119[] PROGMEM = "PiPiccolo";
const char In_120[] PROGMEM = "FlFlute";
const char In_121[] PROGMEM = "ReRecorder";
const char In_122[] PROGMEM = "PFPan Flute";
const char In_123[] PROGMEM = "BBBlown Bottle";
const char In_124[] PROGMEM = "ShShakuhachi";
const char In_125[] PROGMEM = "WhWhistle";
const char In_126[] PROGMEM = "OcOcarina";
const char In_127[] PROGMEM = "SLSquare Lead";
const char In_128[] PROGMEM = "SLSaw Lead";
const char In_129[] PROGMEM = "LaCalliope";
const char In_130[] PROGMEM = "CLChiff Lead";
const char In_131[] PROGMEM = "CLCharang Lead";
const char In_132[] PROGMEM = "VLVoice Lead";
const char In_133[] PROGMEM = "FLFifths Lead";
const char In_134[] PROGMEM = "BLBass Lead";
const char In_135[] PROGMEM = "NPNew Age";
const char In_136[] PROGMEM = "WPWarm Pad";
const char In_137[] PROGMEM = "PPPolysynth";
const char In_138[] PROGMEM = "ChChoir";
const char In_139[] PROGMEM = "BoBowed";
const char In_140[] PROGMEM = "MPMetallic";
const char In_141[] PROGMEM = "HlHalo";
const char In_142[] PROGMEM = "SPSweep";
const char In_143[] PROGMEM = "RaRain";
const char In_144[] PROGMEM = "STSound Track";
const char In_145[] PROGMEM = "CrCrystal";
const char In_146[] PROGMEM = "AtAtmosphere";
const char In_147[] PROGMEM = "BrBrigthness";
const char In_148[] PROGMEM = "GoGoblins";
const char In_149[] PROGMEM = "EcEchoes";
const char In_150[] PROGMEM = "SfSci-fi";
const char In_151[] PROGMEM = "SiSitar";
const char In_152[] PROGMEM = "BjBanjo";
const char In_153[] PROGMEM = "ShShamisen";
const char In_154[] PROGMEM = "KoKoto";
const char In_155[] PROGMEM = "KaKalimba";
const char In_156[] PROGMEM = "BPBag Pipe";
const char In_157[] PROGMEM = "FiFiddle";
const char In_158[] PROGMEM = "ShShanai";
const char In_159[] PROGMEM = "TBTinkle Bell";
const char In_160[] PROGMEM = "AgAgogo";
const char In_161[] PROGMEM = "PPPitched Percussion";
const char In_162[] PROGMEM = "WoWoodblock";
const char In_163[] PROGMEM = "TkTaiko";
const char In_164[] PROGMEM = "MTMelodic Tom";
const char In_165[] PROGMEM = "SDSynth Drum";

const char *const Instrument[] PROGMEM = {
  In_0, In_1, In_2, In_3, In_4, In_5, In_6, In_7, In_8, In_9, In_10, In_11, In_12, In_13, In_14, In_15,
  In_16, In_17, In_18, In_19, In_20, In_21, In_22, In_23, In_24, In_25, In_26, In_27, In_28, In_29, In_30, In_31,
  In_32, In_33, In_34, In_35, In_36, In_37, In_38, In_39, In_40, In_41, In_42, In_43, In_44, In_45, In_46, In_47,
  In_48, In_49, In_50, In_51, In_52, In_53, In_54, In_55, In_56, In_57, In_58, In_59, In_60, In_61, In_62, In_63,
  In_64, In_65, In_66, In_67, In_68, In_69, In_70, In_71, In_72, In_73, In_74, In_75, In_76, In_77, In_78, In_79,
  In_80, In_81, In_82, In_83, In_84, In_85, In_86, In_87, In_88, In_89, In_90, In_91, In_92, In_93, In_94, In_95,
  In_96, In_97, In_98, In_99, In_100, In_101, In_102, In_103, In_104, In_105, In_106, In_107, In_108, In_109, In_110, In_111,
  In_112, In_113, In_114, In_115, In_116, In_117, In_118, In_119, In_120, In_121, In_122, In_123, In_124, In_125, In_126, In_127,
  In_128, In_129, In_130, In_131, In_132, In_133, In_134, In_135, In_136, In_137, In_138, In_139, In_140, In_141, In_142, In_143,
  In_144, In_145, In_146, In_147, In_148, In_149, In_150, In_151, In_152, In_153, In_154, In_155, In_156, In_157, In_158, In_159,
  In_160, In_161, In_162, In_163, In_164, In_165
};

const int NumVoices = (sizeof(Instrument) / sizeof(Instrument[0]));

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

byte PrevNote[8], voice[8], vol[8], note[16][8];
bool cb[16][8], cr[8];
byte CurCol, BeatsPerBar;
byte curInstrument, curCBx, curCBy, KeyboardNote;
int Tempo;
byte xOffsetKbd = 30;
bool pnlVoiceVisible, CaptureVol, CaptureTempo, Running;
enum TEncoder { encNone, encTempo, encRunning, encVoice, encVol, encKey, encBeatsPerBar, encSetup, encChanEn } CurEnc = encNone;

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
void MidiSPItransfer(byte b)
{
  digitalWrite(MIDI_CS, LOW);
  SPI.transfer(b);
  SPI.transfer(0x00); // to read result
  digitalWrite(MIDI_CS, HIGH);
}

//-----------------------------------------------------------------------------
// LoadSaveEEPROM
//     mode=emCheck     check timer, save if reqd
//     mode=emSaveLater save after 3 sec
//     mode=emSaveNow   force a save now
//     mode=emLoad      load saved setup
//
//  total size of each setup = 1+8+8+1+16+16*8+2+1 = 165 bytes
//  memory available for each setup = 1023 / NumSetups = 170 bytes
//  address of each setup = 0, 170, 340, ...
//
//  first time program is run after programming Arduino, the default tune is loaded
//-----------------------------------------------------------------------------

const byte emCheck=0, emSaveLater=1, emSaveNow=2, emLoad=3;
int EEPROMaddr;

//......................................................
// load/save an int
void LoadSaveEEPROMint(bool load, int *x)
{
  if (load)
    EEPROM.get(EEPROMaddr, *x); else
    EEPROM.put(EEPROMaddr, *x);
  EEPROMaddr += sizeof(*x);
}

//......................................................
// load/save a byte
void LoadSaveEEPROMbyte(bool load, byte * x)
{
  if (load)
    EEPROM.get(EEPROMaddr, *x); else
    EEPROM.put(EEPROMaddr, *x);
  EEPROMaddr += sizeof(*x);
}

//......................................................
// load/save a whole setup
void LoadSaveEEPROMsetup(bool load, byte setup)
{
  byte x, y, i;

  EEPROMaddr = setup*(1023 / NumSetups);

  if (load) {
    LoadSaveEEPROMbyte(load, &x);
    if (x != Version) {
      ClearAll();
      if (setup == 0) 
        DefaultTune();
      return;
    }
  } else {
    x = Version;
    LoadSaveEEPROMbyte(load, &x);
  }

  for (y = 0; y <= 7; y++)
  {
    LoadSaveEEPROMbyte(load, &voice[y]);
    voice[y] = constrain(voice[y],0,NumVoices-1);
    if (load && (voice[y] >= 47))
      SendMidiMessage2(0xC0 + y, voice[y] - 47);
  }

  for (y = 0; y <= 7; y++) {
    LoadSaveEEPROMbyte(load, &vol[y]);
    vol[y] = constrain(vol[y],0,127);
  }

  for (y = 0; y <= 7; y++)
    i = (i << 1) + (byte)cr[y];
  LoadSaveEEPROMbyte(load, &i);
  if (load) {
    for (y = 0; y <= 7; y++) {
      cr[y] = (i & 0x80);
      i = i << 1;
    }
  }

  for (x = 0; x <= 15; x++) {
    for (y = 0; y <= 7; y++) {
      i = (i << 1) + (byte)cb[x][y];
    }
    LoadSaveEEPROMbyte(load, &i);
    if (load) {
      for (y = 0; y <= 7; y++) {
        cb[x][y] = (i & 0x80);
        i = i << 1;
      }
    }
  }

  for (x = 0; x <= 15; x++)
    for (y = 0; y <= 7; y++) {
      LoadSaveEEPROMbyte(load, &note[x][y]);
      note[x][y] = constrain(note[x][y],0,127);
    }

  LoadSaveEEPROMint(load, &Tempo);
  Tempo = constrain(Tempo, TempoMin, TempoMax);

  LoadSaveEEPROMbyte(load, &BeatsPerBar);
  BeatsPerBar = constrain(BeatsPerBar,0,15);
  if (load)
    DrawAllCheckBoxes();
}

//......................................................
// LoadSaveEEPROM
void LoadSaveEEPROM(byte setup, byte mode)
{
  static unsigned long prevTime = 0;

  switch (mode) {
    case emCheck:
      if ((prevTime != 0) && (millis()-prevTime > 3000)) 
        LoadSaveEEPROM(setup, emSaveNow);
      break;
    case emSaveLater: 
      prevTime = millis();
      break;
    case emSaveNow:
      LoadSaveEEPROMsetup(false, setup);    
      prevTime = 0;
      break;
    case emLoad:
      LoadSaveEEPROMsetup(true, setup);    
      prevTime = 0;
      break;
  } 
}

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
byte GetSerial() {
  byte b;
  while ( Serial.available() == 0 ) ;
  b = Serial.read();
  return b;
}

//-----------------------------------------------------------------------------
// DrawCheckBox
//-----------------------------------------------------------------------------
void DrawCheckBoxRound(int x, int y, bool checked, uint16_t col, bool curCB)
{
  if (checked)
    DrawDisc(x + cw / 2, y + cw / 2, cw / 2, col); else
    DrawDisc(x + cw / 2, y + cw / 2, cw / 2, TFT_DARKGREY);
}

//-----------------------------------------------------------------------------
// DrawCheckBox
//-----------------------------------------------------------------------------
void DrawCheckBoxSquare(int x, int y, bool checked, uint16_t col, bool curCB)
{
  if (checked) {
    DrawBox(x, y, cw - 3, cw - 3, col);
    if (checked && curCB)
    #ifdef Is480x320
      DrawBox(x + 6, y + 6, cw - 14, cw - 14, TFT_BLACK);
    #else
      DrawBox(x + 4, y + 4, cw - 10, cw - 10, TFT_BLACK);
    #endif
  } else
    DrawBox(x, y, cw - 3, cw - 3, TFT_DARKGREY);
}

//-----------------------------------------------------------------------------
// DrawCheckBoxCB
//-----------------------------------------------------------------------------
void DrawCheckBoxCB(int ax, int ay, bool selected)
{
  DrawCheckBoxSquare(bw + 2 + cw * ax, 4 + hgt * ay, cb[ax][ay], LightColor[ax / 4], selected && (voice[ay] >= 47) && (ax == curCBx) && (ay == curCBy));
}

//-----------------------------------------------------------------------------
// DrawCheckBoxCR
//-----------------------------------------------------------------------------
void DrawCheckBoxCR(int ay)
{
  DrawCheckBoxRound(tft_width - cw - 1, 4 + hgt * ay, cr[ay], TFT_CYAN, false);
}

//-----------------------------------------------------------------------------
// DrawCheckBoxLine
//-----------------------------------------------------------------------------
void DrawCheckBoxLine(int ay, bool Drawbg)
{
  if (Drawbg)
    DrawBox(bw + 1, hgt * ay, tft_width - bw - 1, hgt - 1, TFT_BLACK);
  for (int x = 0; x <= 15; x++) {
    DrawCheckBoxCB(x, ay, true);
    CheckTimer();
  }
  DrawCheckBoxCR(ay);
}

//-----------------------------------------------------------------------------
// DrawAllCheckBoxes
//-----------------------------------------------------------------------------
void DrawAllCheckBoxes(void)
{
  for (int y = 0; y <= 7; y++) 
    DrawCheckBoxLine(y, false);
}

//-----------------------------------------------------------------------------
// DrawButton
//-----------------------------------------------------------------------------
void DrawButton(int y)
{
  uint16_t BrushColor, PenColor;
  if (voice[y] >= 47) {
    BrushColor = RGB(96, 64, 64);
    PenColor = RGB(128, 96, 96);
  } else {
    BrushColor = RGB(64, 64, 64);
    PenColor = TFT_DARKGREY;
  }
  DrawBox(2, hgt * y + 3, bw - 5, hgt - 7, BrushColor);
  for (int i = 0; i <= 2; i++)
  {
    DrawHLine(i, 1 + hgt * y + i, bw - 2 * i, PenColor);
    DrawVLine(i, 1 + hgt * y + i, hgt - 2 - 2 * i, PenColor);
  }

  DrawInstrumentText(5, hgt + hgt * y - 10, voice[y], true, TFT_WHITE);
}

//-----------------------------------------------------------------------------
// DrawRunButton
//-----------------------------------------------------------------------------
void DrawRunButton(void)
{
  DrawBox(1, hgt * 8 + 1, hgt-2, hgt-2, TFT_BLACK);
  if (Running)
  {
    DrawBox(4, hgt * 8 + 4, 5, hgt - 8, TFT_GREEN);
    DrawBox(4 + 10, hgt * 8 + 4, 5, hgt - 8, TFT_GREEN);
  } else {
    DrawBox(5, hgt * 8 + 4, 3, 1 + 2 * hgt / 3, TFT_GREEN);
    DrawTriangle(8, hgt * 8 + 4, 8 + hgt / 3, hgt * 8 + 4 + hgt / 3, 8, hgt * 8 + 4 + 2 * hgt / 3, TFT_GREEN);
  }
}

//-----------------------------------------------------------------------------
// SetRunning
//-----------------------------------------------------------------------------
void SetRunning(bool b) {
  Running = b;
  DrawLEDCol(CurCol, TFT_DARKGREY);
  CurCol = 0;
  DrawLEDCol(CurCol, TFT_RED);
  DrawRunButton();
}

//-----------------------------------------------------------------------------
// DrawSetupButton
//-----------------------------------------------------------------------------
void DrawSetupButton(void)
{
  DrawBox(tft_width-hgt+3, hgt*8+2,    hgt-6, 3, TFT_MAGENTA);
  DrawBox(tft_width-hgt+3, hgt*8+2+7,  hgt-6, 3, TFT_MAGENTA);
  DrawBox(tft_width-hgt+3, hgt*8+2+14, hgt-6, 3, TFT_MAGENTA);
}

//-----------------------------------------------------------------------------
// DrawInstrumentText
//-----------------------------------------------------------------------------
void DrawInstrumentText(int x, int y, int inst, bool ID, uint16_t col)
{
  char buffer[30];

  Cursorx = x;
  Cursory = y;
  strcpy_P(buffer, (char *)pgm_read_word(&(Instrument[inst])));

  if (ID) {
    DrawChar(buffer[0], SmallFont, col);
    DrawChar(buffer[1], SmallFont, col);
  } else {
    DrawString(buffer + 2, MediumFont, col);
  }
}

//-----------------------------------------------------------------------------
// DrawAllButtons
//-----------------------------------------------------------------------------
void DrawAllButtons(void)
{
  for (byte y = 0; y <= 7; y++) {
    CheckTimer();
    DrawButton(y);
  }
}

//-----------------------------------------------------------------------------
// DrawLEDCol
//-----------------------------------------------------------------------------
void DrawLEDCol(int x, uint16_t col)
{
  int w, y;
  x = bw + 1 + cw * x;
  w = cw / 2 - 1;
  y = hgt * 8 + 2 - 1;
  DrawDisc(x + w, y + w, w, col);
  DrawLine(x + 4, y + 2, x + 2, y + 4, TFT_WHITE);
  DrawLine(x + cw - 7, y + cw - 4, x + cw - 5, y + cw - 6, col & 0x7BEF);
  DrawCircle(x + w, y + w, w, TFT_BLACK);
}

//-----------------------------------------------------------------------------
// DrawAllLEDs
//-----------------------------------------------------------------------------
void DrawAllLEDs(void)
{
  for (byte x = 0; x <= 15; x++)
  {
    DrawBox(bw+cw*x, hgt*8, cw, cw, ColumnColor[x/4]);
    if (x > BeatsPerBar)
      DrawLEDCol(x, TFT_BLACK); else
      DrawLEDCol(x, TFT_DARKGREY);
    CheckTimer();
  }
  #ifdef HasEncoder
    if (CurEnc == encBeatsPerBar) 
      HighlightEncoder(true);
  #endif
}

//-----------------------------------------------------------------------------
// DrawSlider
//-----------------------------------------------------------------------------
void DrawSlider(long v, int aMin, int aMax, int Left, int Top, int Right, int Bottom, int *prevx, uint16_t col, bool all)
{
  int x;
  x = (v - aMin) * (Right - Left - VBarWidth) / (aMax - aMin);
  if (all)
    DrawBox(Left, Top, Right - Left, Bottom - Top - 1, col); else
    DrawBox(Left + *prevx, Top + 2, VBarWidth - 1, hgt - 5, col);
  DrawBox(Left + HBarLeft + 1, Top + HBarTop + 1, Right - HBarLeft - (Left + HBarLeft) - 3, Bottom - HBarTop - (Top + HBarTop) - 3, TFT_BLACK);
  DrawFrame(Left + HBarLeft, Top + HBarTop, Right - HBarLeft - (Left + HBarLeft) - 1, Bottom - HBarTop - (Top + HBarTop) - 1, TFT_WHITE);
  DrawBox(Left + x + 1, Top + 2 + 1, VBarWidth - 3, hgt - 7, TFT_BLACK);
  DrawFrame(Left + x, Top + 2, VBarWidth - 1, hgt - 5, TFT_WHITE);
  *prevx = x;
}

//-----------------------------------------------------------------------------
// DrawPanelTempo
//-----------------------------------------------------------------------------
void DrawPanelTempo(bool all)
{
  static int prevx = 0;
  DrawSlider(Tempo, TempoMax, TempoMin, TempoRectLeft, TempoRectTop, TempoRectRight, TempoRectBottom, &prevx, TFT_NAVY, all);
  #ifdef HasEncoder
    if (all && (CurEnc == encTempo))
      HighlightEncoder(true);
  #endif
}

//-----------------------------------------------------------------------------
// DrawScreen
//-----------------------------------------------------------------------------
void DrawScreen(void)
{
  for (int y = 0; y <= 7; y++) {    
    DrawBox(0, y*tft_height/8, tft_width, tft_height/8, TFT_BLACK);
    CheckTimer();
  }
  DrawAllCheckBoxes();   CheckTimer();
  DrawAllButtons();      CheckTimer();
  DrawAllLEDs();         CheckTimer();
  DrawPanelTempo(true);  CheckTimer();
  DrawPanelVoice(true);  CheckTimer();
  DrawKeyboard(true);    CheckTimer();
  DrawRunButton();       CheckTimer();
  DrawSetupButton();     CheckTimer();
}

//-----------------------------------------------------------------------------
// SendMidiMessage2
//-----------------------------------------------------------------------------
void SendMidiMessage2(byte x, byte y)
{
  MidiSPItransfer(x);
  MidiSPItransfer(y);
}

//-----------------------------------------------------------------------------
// SendMidiMessage3
//-----------------------------------------------------------------------------
void SendMidiMessage3(byte x, byte y, byte z)
{
  MidiSPItransfer(x);
  MidiSPItransfer(y);
  MidiSPItransfer(z);
}

//-----------------------------------------------------------------------------
// VoiceChanged
//-----------------------------------------------------------------------------
void VoiceChanged(void)
{
  if (pnlVoiceVisible)
  {
    voice[curCBy] = curInstrument;
    if (curInstrument < 47)
    {
      DrawKeyboard(true);
    } else
    {
      SendMidiMessage2(0xC0 + curCBy, voice[curCBy] - 47);
      DrawKeyboard(true);
    }

    DrawButton(curCBy);
    DrawPanelVoice(true);
  }
}

//-----------------------------------------------------------------------------
// DrawPanelVoice
//-----------------------------------------------------------------------------
void DrawPanelVoice(bool all)
{
  static int prevx = 0;

  if (pnlVoiceVisible)
  {
    DrawSlider(vol[curCBy], VolumeMin, VolumeMax, VolumeRectLeft, VolumeRectTop, VolumeRectRight, VolumeRectBottom, &prevx, TFT_DARKGREY, all);
    #ifdef HasEncoder
      if (all && (CurEnc == encVol))
        HighlightEncoder(true);
    #endif
    if (all) {
      DrawBox(VoiceRectLeft, VoiceRectTop, VoiceRectRight - VoiceRectLeft, VoiceRectBottom - VoiceRectTop - 1, TFT_WHITE);
      DrawInstrumentText(VoiceRectLeft + 13, VoiceRectTop + 16, curInstrument, false, TFT_BLACK);
      DrawTrians(VoiceRectLeft, VoiceRectTop, VoiceRectRight, VoiceRectBottom, TFT_BLACK);
    #ifdef HasEncoder
      if (CurEnc == encVoice)
        HighlightEncoder(true);
    #endif
    }
  }
}

//-----------------------------------------------------------------------------
// DrawTrians
//-----------------------------------------------------------------------------
void DrawTrians(int left, int top, int right, int bottom, uint16_t col)
{
  int h;
  #ifdef Is480x320
    const int t = 3;
  #else
    const int t = 2;
  #endif

  h = (bottom - top) / 2 - 4;
  DrawTriangle(
    left + 2 + h, top + t,
    left + 2, top + t + h,
    left + 2 + h, top + 2 * h + t,
    col);
  DrawTriangle(
    right - 2, top + t + h,
    right - 2 - h, top + 2 * h + t,
    right - 2 - h, top + t,
    col);
}

//-----------------------------------------------------------------------------
// DrawSetupMenu
//-----------------------------------------------------------------------------
void DrawSetupMenu()
{
  DrawBox(SetupMargin, SetupMargin, tft_width - 2 * SetupMargin, tft_height - 2 * SetupMargin, TFT_BLACK);
  CheckTimer();
  DrawFrame(SetupMargin, SetupMargin, tft_width - 2 * SetupMargin, tft_height - 2 * SetupMargin, TFT_WHITE);
  DrawFrame(SetupMargin+SetupSquare*4, tft_height-SetupMargin-SetupSquare*2, SetupSquare*2, SetupSquare, TFT_WHITE);
  DrawStringAt(138,SetupMargin+20+12,"Setups", MediumFont, TFT_WHITE);
  DrawStringAt(SetupMargin+SetupSquare*4+4, tft_height-SetupMargin-SetupSquare*2+2+12, "Clear", MediumFont, TFT_WHITE);
  for (byte i = 0; i <= NumSetups - 1; i++)
  {
    CheckTimer();
    DrawFrame(i * SetupSquare + (tft_width - NumSetups * SetupSquare)/2, (tft_height - SetupSquare)/2, SetupSquare+1, SetupSquare+1, TFT_WHITE);
    Cursorx = i * SetupSquare + (tft_width - NumSetups * SetupSquare)/2 + 7;
    Cursory = (tft_height - SetupSquare)/2 + 16;
    DrawChar(char('1' + i), MediumFont, TFT_WHITE);
  }
  CheckTimer();
  DrawBox(CurSetup * SetupSquare + (tft_width - NumSetups * SetupSquare)/2, (tft_height - SetupSquare)/2, SetupSquare, SetupSquare, TFT_WHITE);
  Cursorx = CurSetup * SetupSquare + (tft_width - NumSetups * SetupSquare)/2 + 7;
  Cursory = (tft_height - SetupSquare)/2 + 16;
  DrawChar(char('1' + CurSetup), MediumFont, TFT_BLACK);
  CheckTimer();
}

//-----------------------------------------------------------------------------
// ExecuteSetupMenu
//-----------------------------------------------------------------------------
void ExecuteSetupMenu()
{
  int x, y;

  LoadSaveEEPROM(CurSetup, emSaveNow);
  DrawSetupMenu();

  while (GetTouch(&x, &y)) // wait for up
    CheckTimer();
  while (! GetTouch(&x, &y)) // wait for down
    CheckTimer();

  for (byte i=0; i<=NumSetups-1; i++)
    if (PtInRect(
      i*SetupSquare + (tft_width-NumSetups*SetupSquare)/2,
      (tft_height-SetupSquare)/2,
      i*SetupSquare + (tft_width-NumSetups*SetupSquare)/2+SetupSquare,
      (tft_height-SetupSquare)/2+SetupSquare,x,y))
    {
      CurSetup = i;
      LoadSaveEEPROM(CurSetup, emLoad);
      DrawSetupMenu();
    }

  if (PtInRect(
      SetupMargin+SetupSquare*4,
      tft_height-SetupMargin-SetupSquare*2,
      SetupMargin+SetupSquare*4+SetupSquare*2,
      tft_height-SetupMargin-SetupSquare*2+SetupSquare,
      x,y)) 
    ClearAll();

  while (GetTouch(&x, &y)) // wait for up
    CheckTimer();
  DrawScreen();
  #ifdef HasEncoder
    ChangeCurEnc(encSetup);
  #endif
}

//-----------------------------------------------------------------------------
// DrawKeyboard
//   all     redraw all or just the note that's changed 
//-----------------------------------------------------------------------------

//................................................................................
// x coord of a note
int NoteToX(int note)
{
  int i;

  i = note % 12;
  i = kbdNoteOffset[i];
  if (i >= 10)
    return (i - 10) * keywidth + (note / 12) * (keywidth * 7) + keywidth - xOffsetKbd * keywidth + KeyboardRect2Left; else
    return i * keywidth + (note / 12) * (keywidth * 7) + keywidth / 2 - xOffsetKbd * keywidth + KeyboardRect2Left;
}

//................................................................................
// draw a black key of the keyboard

void DrawBlackKey(int x, uint16_t col)
{
  #ifdef Is480x320
    DrawBox(x - keywidth / 3, KeyboardRect2Top, 2 * keywidth / 3 - 2, hgt / 2, col);
  #else
    DrawBox(x - keywidth / 3 - 1, KeyboardRect2Top, 2 * keywidth / 3, hgt / 2, col);
  #endif
}

//................................................................................
// draw a "white" key of the keyboard - color depends on octave

void DrawWhiteKey(int x, uint16_t col)
{
  #ifdef Is480x320
    DrawBox(x - keywidth / 2, KeyboardRect2Top, keywidth - 1, hgt - 1, col);
  #else
    DrawBox(x - keywidth / 2, KeyboardRect2Top, keywidth - 2, hgt - 1, col);
  #endif
}

//................................................................................
// draw a black or "white" key of the keyboard

void DrawNoteKey(byte note, bool whiteKey)
{
  int x;
  x = NoteToX(note);
  if (x < KeyboardRect2Left)
    return;
  if (x > KeyboardRect2Right)
    return;

  if (kbdNoteOffset[note % 12] >= 10)
  {
    if (!whiteKey)
    {
      DrawBlackKey(x, TFT_BLACK);
    }
  } else 
  if (whiteKey)
  {
    DrawWhiteKey(x, kbdNoteColor[(note + 3) / 12]);
  }
}

//................................................................................
// highlight the key of the current note 

void DrawNoteSelected(byte note, bool Down)
{
  int x;
  uint16_t BrushColor;

  if (note > 127)
    return;

  x = NoteToX(note);
  if (x < KeyboardRect2Left)
    return;
  if (x > KeyboardRect2Right - keywidth)
    return;

  if (kbdNoteOffset[note % 12] >= 10)
  {
    if (Down)
      BrushColor = TFT_RED; else
      BrushColor = TFT_BLACK;
    DrawBlackKey(x, BrushColor);
  } else
  {
    if (Down)
      BrushColor = TFT_RED; else
      BrushColor = kbdNoteColor[(note + 3) / 12];
    DrawWhiteKey(x, BrushColor);

    BrushColor = TFT_BLACK;
    if (note > 0)
      if (kbdNoteOffset[(note - 1) % 12] >= 10)
      {
        x = NoteToX(note - 1);
        if (x >= KeyboardRect2Left)
          DrawBlackKey(x, TFT_BLACK);
      }
    if (kbdNoteOffset[(note + 1) % 12] >= 10)
    {
      x = NoteToX(note + 1);
      if (x >= KeyboardRect2Left)
        DrawBlackKey(x, TFT_BLACK);
    }
  }
  #ifdef HasEncoder
    if (CurEnc == encKey)
      HighlightEncoder(true);
  #endif
}

//................................................................................
// draw all the keys of the keyboard

void DrawWholeKeyboard()
{
  byte i;
  DrawBox(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight - KeyboardRectLeft - 1, KeyboardRectBottom - KeyboardRectTop - 1, TFT_BLACK);
  for (i = 0; i <= 127; i++)
    DrawNoteKey(i, true);
  for (i = 0; i <= 127; i++)
    DrawNoteKey(i, false);
  DrawTrians(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight, KeyboardRectBottom, TFT_WHITE);
}

//................................................................................
// draw the keyboard

void DrawKeyboard(bool all)
{
  static int prevNote = 0;

  if (!keyboardVisible()) {
    DrawBox(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight - KeyboardRectLeft - 1, KeyboardRectBottom - KeyboardRectTop - 1, TFT_BLACK);
    return;
  }

  if (all)
    DrawBox(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight - KeyboardRectLeft, KeyboardRectBottom - KeyboardRectTop, TFT_BLACK);

  if (NoteToX(KeyboardNote) < KeyboardRect2Left + keywidth * 2)
  {
    while (NoteToX(KeyboardNote) < KeyboardRect2Left + keywidth * 2)
      xOffsetKbd = xOffsetKbd - 12;
    xOffsetKbd = max(xOffsetKbd, 0);
    DrawWholeKeyboard();
    prevNote = 255;
  } else 
  if (NoteToX(KeyboardNote) > KeyboardRect2Left + keywidth * 2)
  {
    while (NoteToX(KeyboardNote) > KeyboardRect2Right - keywidth * 2)
      xOffsetKbd = xOffsetKbd + 12;
    xOffsetKbd = min(xOffsetKbd, 53);
    DrawWholeKeyboard();
    prevNote = 255;
  } else 
  if (all)
  {
    DrawWholeKeyboard();
    prevNote = 255;
  }

  DrawNoteSelected(prevNote, false);
  if (KeyboardNote != prevNote)
    DrawNoteSelected(KeyboardNote, true);

  prevNote = KeyboardNote;
}

//-----------------------------------------------------------------------------
// keyboardVisible
//   is the keyboard visible
//-----------------------------------------------------------------------------

bool keyboardVisible(void) {
  return (voice[curCBy] >= 47) && (pnlVoiceVisible || cb[curCBx][curCBy]);
}

//-----------------------------------------------------------------------------
// KeyboardNoteChange
//-----------------------------------------------------------------------------
void KeyboardNoteChange(void)
{
  byte x, y;
  if (pnlVoiceVisible)
    for (y = 0; y <= 7; y++)
      for (x = 0; x <= 15; x++)
        note[x][y] = KeyboardNote;
  note[curCBx][curCBy] = KeyboardNote;
  LoadSaveEEPROM(CurSetup, emSaveLater);
  DrawKeyboard(false);
}

//-----------------------------------------------------------------------------
// NoteOn
//-----------------------------------------------------------------------------
//void NoteOn(int chan, byte n, byte vol)
//{
//  NoteOff(chan);
//  SendMidiMessage3(0x90 + chan, n, vol);
//  PrevNote[chan] = n;
//}

//-----------------------------------------------------------------------------
// NoteOff
//-----------------------------------------------------------------------------
void NoteOff(byte chan)
{
  SendMidiMessage3(0x80 + chan, PrevNote[chan], 0);
}

//-----------------------------------------------------------------------------
// NoteOn
//   bug in the chip: sometimes needs percussion twice
//-----------------------------------------------------------------------------
void NoteOn(byte chan, byte column, byte vol)
{
  if (voice[chan] < 47) {
    SendMidiMessage3(0x99, voice[chan] + 35, vol); 
    SendMidiMessage3(0x99, voice[chan] + 35, vol); 
  } else {
    NoteOff(chan);
    SendMidiMessage3(0x90 + chan, note[column][chan], vol);
    PrevNote[chan] = note[column][chan];
  }
}

//-----------------------------------------------------------------------------
// AllNoteOff
//-----------------------------------------------------------------------------
void AllNoteOff(void)
{
  for (int chan = 0; chan <= 7; chan++)
    NoteOff(chan);
}

//-----------------------------------------------------------------------------
// ClearAll
//-----------------------------------------------------------------------------
void ClearAll(void) {
  AllNoteOff();
  memset(cb,0,sizeof(cb));
  memset(note,65,sizeof(note));
  for (byte y = 0; y <= 7; y++) {
    vol[y] = 127;
    cr[y] = true;
  }
  BeatsPerBar = 15;
  Tempo = 133;
  SetRunning(false);
}

//-----------------------------------------------------------------------------
// TempoMouseMove
//-----------------------------------------------------------------------------
void TempoMouseMove(int x, int y)
{
  int t;
  t = constrain(TempoMax - ((TempoMax - TempoMin) * (long)x / ((TempoRectRight - TempoRectLeft) - VBarWidth)), TempoMin, TempoMax);
  if (t != Tempo) {
    Tempo = t;
    DrawPanelTempo(false);
  }
}

//-----------------------------------------------------------------------------
// PtInRect
//-----------------------------------------------------------------------------
bool PtInRect(int left, int top, int right, int bottom, int x, int y)
{
  return (x >= left) && (x <= right) && (y >= top) && (y <= bottom);
}

//-----------------------------------------------------------------------------
// MouseDownKeyboard
//-----------------------------------------------------------------------------
void MouseDownKeyboard(int x, int y)
{
  byte i = 0;

  #ifdef HasEncoder
    ChangeCurEnc(encKey);
  #endif
  
  if (x < KeyboardRectLeft + hgt)
    KeyboardNote = constrain(KeyboardNote, 1, 127)-1; else 
  if (x > KeyboardRectRight - hgt)
    KeyboardNote = constrain(KeyboardNote, 0, 126)+1; else 
  {
    while (NoteToX(i) < x) {
      if (y > (KeyboardRectTop + KeyboardRectBottom) / 2) {
        if (kbdNoteOffset[i % 12] < 10)
          KeyboardNote = i;
      } else {
        if (kbdNoteOffset[i % 12] >= 10)
          KeyboardNote = i;
      }
      i++;
    }
  }
  KeyboardNoteChange();
}

//-----------------------------------------------------------------------------
// MouseDownTempo
//-----------------------------------------------------------------------------
void MouseDownTempo(int x, int y)
{
  #ifdef HasEncoder
    ChangeCurEnc(encTempo);
  #endif
  CaptureTempo = true;
}

//-----------------------------------------------------------------------------
// MouseDownVoice
//-----------------------------------------------------------------------------
void MouseDownVoice(int x, int y)
{
  #ifdef HasEncoder
    ChangeCurEnc(encVoice);
  #endif

  if (PtInRect(VolumeRectLeft, VolumeRectTop, VolumeRectRight, VolumeRectBottom, x, y))
  {
    CaptureVol = true;
    #ifdef HasEncoder
      ChangeCurEnc(encVol);
    #endif
  } else 
  if (PtInRect(VoiceRectLeft, VoiceRectTop, VoiceRectRight, VoiceRectBottom, x, y))
  {
    if (x < (VoiceRectLeft + VoiceRectRight) / 2)
    {
      if (curInstrument == 0)
        curInstrument = NumVoices - 1; else
        curInstrument--;
    } else
    {
      if (curInstrument == NumVoices - 1)
        curInstrument = 0; else
        curInstrument++;
    }
    VoiceChanged();
  } else {
    pnlVoiceVisible = false;
    if (cb[curCBx][curCBy])
      KeyboardNote = note[curCBx][curCBy];
    DrawKeyboard(true);
    LoadSaveEEPROM(CurSetup, emSaveLater);
    DrawButton(curCBy);
    DrawCheckBoxLine(curCBy, true);
  }
}

//-----------------------------------------------------------------------------
// MouseDownLEDs
//-----------------------------------------------------------------------------
void MouseDownLEDs(int ax)
{
  #ifdef HasEncoder
    ChangeCurEnc(encBeatsPerBar);
  #endif
  
  BeatsPerBar = constrain(ax, 0, 15);
  DrawAllLEDs();
}

//-----------------------------------------------------------------------------
// MouseDownCRs
//-----------------------------------------------------------------------------
void MouseDownCRs(int ay)
{
  if (ay <= 7)
  {
    cr[ay] = not cr[ay];
    DrawCheckBoxCR(ay);
    NoteOff(ay);
    #ifdef HasEncoder
      ChangeCurEnc(encChanEn);
    #endif
  } else
    ExecuteSetupMenu();
}

//-----------------------------------------------------------------------------
// MouseDownCBs
//-----------------------------------------------------------------------------
void MouseDownCBs(int ax, int ay)
{
  DrawCheckBoxCB(curCBx, curCBy, false);

  if (voice[ay] >= 47)
  {
    if (cb[ax][ay])
    {
      if ((ax == curCBx) && (ay == curCBy))
        cb[ax][ay] = false;
    } else
      cb[ax][ay] = true;
  } else
    cb[ax][ay] = not cb[ax][ay];
  #ifdef HasEncoder
    if (!cb[ax][ay])
      ChangeCurEnc(encNone);
  #endif
  curCBx = ax;
  curCBy = ay;

  KeyboardNote = note[ax][ay];
  DrawKeyboard(true);
  KeyboardNoteChange();
  DrawCheckBoxCB(ax, ay, true);
  LoadSaveEEPROM(CurSetup, emSaveLater);
}

//-----------------------------------------------------------------------------
// MouseDownButtons
//-----------------------------------------------------------------------------
void MouseDownButtons(int x, int y)
{
  byte ay = y / hgt;
  if (ay <= 7)
  {
    curCBy = ay;
    curInstrument = voice[ay];
    DrawCheckBoxCB(curCBx, curCBy, true);
    pnlVoiceVisible = true;
    VoiceRectTop = ay * hgt;
    VoiceRectBottom = ay * hgt + hgt;
    VolumeRectTop = VoiceRectTop;
    VolumeRectBottom = VoiceRectBottom;

    MouseMove(x, y);
    DrawPanelVoice(true);

    for (byte i = 0; i <= 15; i++)
      if (cb[i][ay])
        KeyboardNote = note[i][ay];
    DrawKeyboard(true);

    #ifdef HasEncoder
      ChangeCurEnc(encVoice);
    #endif
  } else {
    SetRunning(!Running);
    #ifdef HasEncoder
      ChangeCurEnc(encRunning);
    #endif
  }
}

//-----------------------------------------------------------------------------
// MouseDown
//-----------------------------------------------------------------------------
void MouseDown(int x, int y)
{
  byte ax = (x - bw) / cw;
  byte ay = y / hgt;

  if (y > TempoRectTop)
  {
    if (x <= TempoRectRight)
      MouseDownTempo(x, y); else
      MouseDownKeyboard(x, y);
  } else 
  if (pnlVoiceVisible)
    MouseDownVoice(x, y); else
  {
    if (x < bw)
      MouseDownButtons(x, y);

    if ((CaptureVol) || (CaptureTempo))
      MouseMove(x, y); else 
    if (x > tft_width - cw)
      MouseDownCRs(ay); else 
    if (x >= bw)
    {
      if (ay == 8)
        MouseDownLEDs(ax); else 
      if ((ay < 8) && (ax < 16))
        MouseDownCBs(ax, ay);
      LoadSaveEEPROM(CurSetup, emSaveLater);
    }
  }
}

//-----------------------------------------------------------------------------
// MouseMove
//-----------------------------------------------------------------------------
void MouseMove(int x, int y)
{
  if (CaptureVol)
  {
    vol[curCBy] = constrain((VolumeMax - VolumeMin) * (x - 4 - VolumeRectLeft) / (VolumeRectRight - VolumeRectLeft - VBarWidth) + VolumeMin, VolumeMin, VolumeMax);
    DrawPanelVoice(false);
  }
  if (CaptureTempo)
  {
    TempoMouseMove(x, y);
  }
}

//-----------------------------------------------------------------------------
// MouseDown
//-----------------------------------------------------------------------------
void MouseUp(void)
{
  CaptureVol = false;
  CaptureTempo = false;
}

//-----------------------------------------------------------------------------
// CheckTouch
//-----------------------------------------------------------------------------
void CheckTouch(void)
{
  static bool prevTouch = false;
  int x, y;

  if (GetTouch(&x, &y)) {
    if (! prevTouch)
      MouseDown(x, y); else
      MouseMove(x, y);
    prevTouch = true;
  } else   
  if (prevTouch)
  {
    MouseUp();
    prevTouch = false;
  }
}

//-------------------------------------------------------------------------
// CheckTimer
//-------------------------------------------------------------------------
void CheckTimer(void)
{
  static unsigned long nextTime = 0;
  if (millis() > nextTime) {
    nextTime = nextTime + Tempo;

    CheckSupply();

    if (!Running)
      return;

    if (CurCol > BeatsPerBar)
      DrawLEDCol(CurCol, TFT_BLACK); else
      DrawLEDCol(CurCol, TFT_DARKGREY);

    if (CurCol >= BeatsPerBar)
      CurCol = 0; else
      CurCol++;

    DrawLEDCol(CurCol, TFT_RED);

    for (byte y = 0; y <= 7; y++)
      if (cb[CurCol][y] && cr[y])
        NoteOn(y, CurCol, vol[y]);
  }
}

//-------------------------------------------------------------------------
// HighlightEncoder
//-------------------------------------------------------------------------
#ifdef HasEncoder
void HighlightEncoder(bool draw) {
  switch(CurEnc) {
    case encNone:
      break;
    case encTempo:
      if (draw)
        DrawFrame(TempoRectLeft, TempoRectTop, TempoRectRight-TempoRectLeft, TempoRectBottom-TempoRectTop, TFT_RED); else
        DrawFrame(TempoRectLeft, TempoRectTop, TempoRectRight-TempoRectLeft, TempoRectBottom-TempoRectTop, TFT_NAVY);
      break;
    case encRunning:
      if (draw)
        DrawFrame(0, hgt * 8, hgt, hgt, TFT_RED); else
        DrawFrame(0, hgt * 8, hgt, hgt, TFT_BLACK); 
      break;
    case encKey:
     if (keyboardVisible()) 
       if (draw)
          DrawFrame(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight-KeyboardRectLeft, KeyboardRectBottom-KeyboardRectTop, TFT_RED); else
          DrawFrame(KeyboardRectLeft, KeyboardRectTop, KeyboardRectRight-KeyboardRectLeft, KeyboardRectBottom-KeyboardRectTop, TFT_BLACK); 
     break;
    case encVol:
      if (pnlVoiceVisible)
        if (draw)
          DrawFrame(VolumeRectLeft, VolumeRectTop, VolumeRectRight-VolumeRectLeft, VolumeRectBottom-VolumeRectTop-1, TFT_RED); else
          DrawFrame(VolumeRectLeft, VolumeRectTop, VolumeRectRight-VolumeRectLeft, VolumeRectBottom-VolumeRectTop-1, TFT_DARKGREY);
      break;
    case encBeatsPerBar:
      if (draw)
        DrawFrame(bw-1, hgt * 8-1, cw*16+1, cw+1, TFT_RED); else
        DrawFrame(bw-1, hgt * 8-1, cw*16+1, cw+1, TFT_BLACK); 
      break;
    case encSetup:
      if (draw)
        DrawFrame(tft_width-hgt+2, hgt*8, hgt-4, hgt-4, TFT_RED); else
        DrawFrame(tft_width-hgt+2, hgt*8, hgt-4, hgt-4, TFT_BLACK);     
      break;
    case encChanEn:
      if (draw)
        DrawFrame(tft_width - cw - 2, 3, cw+2, hgt * 8 - 3, TFT_RED); else
        DrawFrame(tft_width - cw - 2, 3, cw+2, hgt * 8 - 3, TFT_BLACK); 
      break;
    case encVoice:
      if (pnlVoiceVisible)
        if (draw)
          DrawFrame(VoiceRectLeft, VoiceRectTop, VoiceRectRight-VoiceRectLeft, VoiceRectBottom-VoiceRectTop-1, TFT_RED); else
          DrawFrame(VoiceRectLeft, VoiceRectTop, VoiceRectRight-VoiceRectLeft, VoiceRectBottom-VoiceRectTop-1, TFT_WHITE);
      break;
  }
}
#endif
    
//-------------------------------------------------------------------------
// ChangeCurEnc
//-------------------------------------------------------------------------
#ifdef HasEncoder
void ChangeCurEnc(TEncoder enc) {
  HighlightEncoder(false);
  CurEnc = enc;
  HighlightEncoder(true);
}
#endif

//-------------------------------------------------------------------------
// ChangeEncoderValue
//-------------------------------------------------------------------------
#ifdef HasEncoder

//................................................................................
// use the knob to change the tempo

void ChangeEncTempo(bool inc) {
  if (inc)
    Tempo = constrain(Tempo-1, TempoMin, TempoMax); else
    Tempo = constrain(Tempo+1, TempoMin, TempoMax);
  DrawPanelTempo(false);
}

//................................................................................
// use the knob to start/stop running

void ChangeEncRunning(bool inc) {
  SetRunning(inc);
}

//................................................................................
// use the knob to change the current note

void ChangeEncKey(bool inc) {
  if (inc)
    KeyboardNote = constrain(KeyboardNote, 0, 126)+1; else
    KeyboardNote = constrain(KeyboardNote, 1, 127)-1;
  KeyboardNoteChange();
}

//................................................................................
// use the knob to change the volume of the channel

void ChangeEncVol(bool inc) {
  if (inc)
    vol[curCBy] = constrain(vol[curCBy], VolumeMin, VolumeMax-1)+1; else
    vol[curCBy] = constrain(vol[curCBy], VolumeMin+1, VolumeMax)-1;
  DrawPanelVoice(false);
}

//................................................................................
// use the knob to change the beats per bar

void ChangeEncBeatsPerBar(bool inc) {
  if (!inc)
    DrawLEDCol(BeatsPerBar, TFT_BLACK);
  if (inc)
    BeatsPerBar = constrain(BeatsPerBar+1,0,15); else
    BeatsPerBar = constrain(BeatsPerBar-1,0,15);
  DrawLEDCol(BeatsPerBar, TFT_DARKGREY);
}

//................................................................................
// use the knob to change the setup

void ChangeEncSetup(bool inc) {
  LoadSaveEEPROM(CurSetup, emSaveNow);
  if (inc)
    CurSetup = constrain(CurSetup+1,0,NumSetups-1); else
    CurSetup = constrain(CurSetup-1,0,NumSetups-1);
  LoadSaveEEPROM(CurSetup, emLoad);
  DrawScreen();
}

//................................................................................
// use the knob to enable/disable channels

void ChangeEncChanEn(bool inc) {
  if (inc) {
    if (cr[7]) 
      return;
    for (int y = 6; y >= -1; y--)
      if ((y < 0) || cr[y]) {
        cr[y+1] = true;
        DrawCheckBoxCR(y+1);
        return;
      }
  } else {
    for (int y = 7; y >= 0; y--)
      if (cr[y]) {
        cr[y] = false;
        DrawCheckBoxCR(y);
        return;
      }
  }
}

//................................................................................
// use the knob to change the voice

void ChangeEncVoice(bool inc) {
  if (inc)
  {
    if (curInstrument == NumVoices - 1)
      curInstrument = 0; else
      curInstrument++;
  } else {
    if (curInstrument == 0)
      curInstrument = NumVoices - 1; else
      curInstrument--;
  }
  VoiceChanged();
}

//................................................................................
// use the knob to change a value

void ChangeEncoderValue(bool inc) {
  switch(CurEnc) {
    case encTempo:       ChangeEncTempo(inc); break;
    case encRunning:     ChangeEncRunning(inc); break;
    case encKey:         ChangeEncKey(inc); break;
    case encVol:         ChangeEncVol(inc); break;
    case encBeatsPerBar: ChangeEncBeatsPerBar(inc); break;
    case encSetup:       ChangeEncSetup(inc); break;
    case encChanEn:      ChangeEncChanEn(inc); break;
    case encVoice:       ChangeEncVoice(inc); break;
  }
}
#endif

//-------------------------------------------------------------------------
// CheckEncoder
//   this encoder produces one complete "cycle" per click
//-------------------------------------------------------------------------
#ifdef HasEncoder
void CheckEncoder(void) {
  bool a,b;
  static bool preva = false;
  static bool prevb = false;
  static byte count = 0;
  static byte prevcount = 0;
  static byte btncount = 0;
  a = digitalRead(ENCODER_A) == HIGH;
  b = digitalRead(ENCODER_B) == HIGH;
  if (a != preva) {
    if (a == b) count++; else count--;
  }  
  if ((a && b) && ((count - prevcount) & 255) > 0) {
    ChangeEncoderValue(((count - prevcount) & 255) < 128);
    prevcount = count;      
  }
  preva = a;
  prevb = b;

  if (digitalRead(ENCODER_Btn) == LOW) {
    if (btncount > 0) {
      btncount--;
      if (btncount == 0) {
        enum TEncoder c;
        if (CurEnc == encChanEn)
          c = encTempo; else
          c = CurEnc+1;          
        while (((c == encKey) && !keyboardVisible()) ||
               ((c == encVoice) && !pnlVoiceVisible) ||
               ((c == encVol) && !pnlVoiceVisible)) 
          c = c+1;
        ChangeCurEnc(c);
      }
    }    
  } else
    btncount = 20;
}
#endif

//-------------------------------------------------------------------------
// CheckDrumpads
//   at least 50mS between hits
//   threshold for a hit is 20
//   pulsewidth is at least pulseWidth * 100 uS
//-------------------------------------------------------------------------
void CheckDrumpads(void) {
  byte chan = 0;
  int i,j,n;
  static int mean[8] = {1023,1023,1023,1023,1023,1023,1023,1023};
  static unsigned long nextTime = 0;
  const int threshold = 20;
  const int pulseWidth = 100;
  const byte numDrumpadPins = 2;

  if (numDrumpadPins == 0)
    return;

  for (chan=0; chan < numDrumpadPins; chan++) {
    i = 1023-analogRead(A0 + chan);
  
    if (i > mean[chan]) 
      mean[chan]++; else
      mean[chan]--;
      
    if ((millis() > nextTime) && (i-mean[chan] > threshold)) {
      for (n=0; n < pulseWidth; n++) {
        j = 1023-analogRead(A0 + chan);
        if (j > i) 
          i = j;
        if (j-mean[chan] > threshold)
          n = 0;
      if (j > mean[chan]) 
        mean[chan]++; else
        mean[chan]--;
      }
      
      NoteOn(chan, 0, constrain((i-mean[chan]), 0, 127));
      if (Running && cr[chan]) {
        cb[CurCol][chan] = true;
        DrawCheckBoxCB(CurCol, chan, true);
      }
      nextTime = millis() + 50;
    }
  }
}

//-------------------------------------------------------------------------
// CheckSupply
//-------------------------------------------------------------------------
void CheckSupply(void) {
  if (readSupply() < 4700) {
    DrawBox(0, 0, 100, 20, TFT_BLACK);
    ILI9341SetCursor(1, 20);
    DrawStringAt(4, 19, "Low Battery" , MediumFont, TFT_WHITE);
    SetRunning(false);
  }
}
 
//-------------------------------------------------------------------------
// readSupply
//   "5V" value
//-------------------------------------------------------------------------
long readSupply() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

//-------------------------------------------------------------------------
// DefaultTune
//-------------------------------------------------------------------------
void DefaultTune(void) {
  ClearAll();
  voice[0] = 30;
  voice[1] = 14;
  voice[2] = 39;
  voice[3] = 41;
  voice[4] = 4;
  voice[5] = 47;
  voice[6] = 36;
  voice[7] = 37;
  cb[0][0] = cb[0][3] = cb[1][1] = cb[1][3] = cb[2][1] = cb[2][5] = cb[3][2] = cb[3][5] = cb[4][0] = 1;
  cb[5][4] = cb[7][2] = cb[8][0] = cb[8][5] = cb[8][6] = cb[9][3] = cb[9][6] = cb[10][1] = cb[10][5] = 1;
  cb[11][7] = cb[12][0] = cb[12][4] = cb[13][0] = cb[13][4] = cb[14][4] = cb[15][1] = 1;
  cr[4] = 0;
  cr[5] = 0;
  BeatsPerBar = 15;
  vol[0] = 69;
  vol[1] = 121;
  vol[2] = 119;
  vol[3] = 127;
  vol[4] = 127;
  vol[5] = 105;
  vol[6] = 127;
  vol[7] = 127;
  Tempo = 133;
  LoadSaveEEPROM(CurSetup, emSaveNow);
}

//-------------------------------------------------------------------------
// setup
//-------------------------------------------------------------------------
void setup(void)
{
  byte x, y;

  Serial.begin(57600);
  Serial.println("DrumSynth");

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(MIDI_RST, OUTPUT);
  pinMode(MIDI_CLK, OUTPUT);
  pinMode(MIDI_CS, OUTPUT);

  digitalWrite(MIDI_CS, HIGH);
  digitalWrite(MIDI_RST, LOW);
  delay(10);
  digitalWrite(MIDI_RST, HIGH);

  #ifdef HasEncoder
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    pinMode(ENCODER_Btn, INPUT_PULLUP);
  #endif
  for (y=A0; y < A7; y++) 
    pinMode(y, INPUT_PULLUP);

  SPI.begin ();
  SPI.setBitOrder(MSBFIRST);

  ILI9341fast = false;
  ILI9341Begin(TFT_CS, TFT_CD, TFT_RST, tft_width, tft_height, ILI9341_Rotation3);
  BeginTouch(TOUCH_CS, 3, 340, 340, 3900, 3800);

  for (y = 0; y <= 7; y++)
  {
    voice[y] = y * 3;
    cr[y] = true;
    vol[y] = 127;
    for (x = 0; x <= 15; x++) {
      note[x][y] = 60 + y * 5;
      cb[x][y] = false;
    }
  }
  CurCol = 0;
  BeatsPerBar = 15;
  Tempo = 300;
  pnlVoiceVisible = false;
  CaptureVol = false;
  CaptureTempo = false;
  Running = false;
  curCBx = 0;
  curCBy = 0;
  CurSetup = 0;

  LoadSaveEEPROM(CurSetup, emLoad);

  DrawScreen();
}

//-----------------------------------------------------------------------------
// Main routines
// loop
//-----------------------------------------------------------------------------
void loop(void)
{
  byte b;

  if ( Serial.available() > 0 )
  {
    b = GetSerial();
    MidiSPItransfer(b);
  }

  CheckTouch();
  CheckTimer();
  #ifdef HasEncoder
    CheckEncoder();
  #endif
  CheckDrumpads();
  LoadSaveEEPROM(CurSetup, emCheck);
}
