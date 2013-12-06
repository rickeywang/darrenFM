
//Due to a bug in Arduino, these need to be included here too/first
#include <SPI.h>
#include <Wire.h>

//Add the Si4735 Library to the sketch.
#include <Si4735.h>

//Create an instance of the Si4735 named radio
Si4735 radio;
//... and an RDS decoder to go with it.
Si4735RDSDecoder decoder;
//Other variables we will use below
char command;
byte mode, status;
word frequency, rdsblock[4];
bool goodtune;
Si4735_RX_Metrics RSQ;
Si4735_RDS_Data station;
Si4735_RDS_Time rdstime;
char FW[3], REV;

void setup()
{
  //pinMode(43, OUTPUT);
  //digitalWrite(43, LOW);
  //Create a serial connection
  Serial.begin(9600);
  Serial.println("HI!"); //RIC

  //Initialize the radio to the FM mode. (see SI4735_MODE_* constants).
  //The mode will set the proper receiver bandwidth. Ensure that the antenna
  //switch on the shield is configured for the desired mode.
  radio.begin(SI4735_MODE_FM);
  Serial.println("Ready, type ? for help.");
}

void loop()
{
  //Attempt to update RDS information if any surfaced
  if(!(millis() % 250)) {
    radio.sendCommand(SI4735_CMD_GET_INT_STATUS);
    if(radio.readRDSBlock(rdsblock)) decoder.decodeRDSBlock(rdsblock);
  }
  
  //Wait until a character comes in on the Serial port.
  if(Serial.available()){
    //Decide what to do based on the character received.
    command = Serial.read();
    switch(command){
      case 'v': 
        if(radio.volumeDown()) Serial.println(F("Volume decreased"));
        else Serial.println(F("ERROR: already at minimum volume"));
        Serial.flush();
        break;
      case 'V':
        if(radio.volumeUp()) Serial.println(F("Volume increased"));
        else Serial.println(F("ERROR: already at maximum volume"));
        Serial.flush();
        break;
      case 's': 
        Serial.println(F("Seeking down with band wrap-around"));
        Serial.flush();
        radio.seekDown();
        break;
      case 'S': 
        Serial.println(F("Seeking up with band wrap-around"));
        Serial.flush();
        radio.seekUp();
        break;
      case 'm': 
        radio.mute();
        Serial.println(F("Audio muted"));
        Serial.flush();
        break;
      case 'M': 
        radio.unMute();
        Serial.println(F("Audio unmuted"));
        Serial.flush();
        break;
      case 'f': 
        frequency = radio.getFrequency(&goodtune);
        mode = radio.getMode();
        Serial.print(F("Currently tuned to "));
        switch(mode) {
          case SI4735_MODE_FM: 
            Serial.print(frequency / 100);
            Serial.print(".");
            Serial.print(frequency % 100);
            Serial.println(F("MHz FM"));
            break;
          case SI4735_MODE_SW: 
            Serial.print(frequency / 1000);
            Serial.print(".");
            Serial.print(frequency % 1000);
            Serial.println(F("MHz SW"));
            break;
          case SI4735_MODE_AM:
          case SI4735_MODE_LW: 
            Serial.print(frequency);
            Serial.print("kHz ");
            Serial.println((mode == SI4735_MODE_AM) ? "AM" : "LW");
            break;
        }
        if(goodtune) Serial.println(F("* Valid tune"));
        Serial.flush();        
        break;
      case 'L':
      case 'A':
      case 'W':
      case 'F': 
        Serial.print(F("Switching mode to "));
        switch(command) {
          case 'L': 
            Serial.println("LW");
            radio.setMode(SI4735_MODE_LW);
            break;
          case 'A': 
            Serial.println("AM");
            radio.setMode(SI4735_MODE_AM);
            break;
          case 'W': 
            Serial.println("SW");
            radio.setMode(SI4735_MODE_SW);
            break;
          case 'F': 
            Serial.println("FM");
            radio.setMode(SI4735_MODE_FM);
            break;
        }
        Serial.flush();        
        break;
      case 'q': 
        radio.getRSQ(&RSQ);
        Serial.println(F("Signal quality metrics {"));
        Serial.print(F("RSSI = "));
        Serial.print(RSQ.RSSI);
        Serial.println("dBuV");
        Serial.print("SNR = ");
        Serial.print(RSQ.SNR);
        Serial.println("dB");
        if(radio.getMode() == SI4735_MODE_FM) {
          Serial.println((RSQ.PILOT ? "Stereo" : "Mono"));
          Serial.print(F("f Offset = "));
          Serial.print(RSQ.FREQOFF);
          Serial.println("kHz");
        }
        Serial.println("}");
        Serial.flush();        
        break;
      case 't':
        radio.sendCommand(SI4735_CMD_GET_INT_STATUS);
        status = radio.getStatus();
        Serial.println(F("Status byte {"));
        if(status & SI4735_STATUS_CTS) Serial.println(F("* Clear To Send"));
        if(status & SI4735_STATUS_ERR) 
          Serial.println(F("* Error on last command"));
        if(status & SI4735_STATUS_RSQINT)
          Serial.println(F("* Received Signal Quality interrupt"));
        if(status & SI4735_STATUS_RDSINT)
          Serial.println(F("* RDS/RDBS interrupt"));            
        if(status & SI4735_STATUS_STCINT)
          Serial.println(F("* Seek/Tune Complete interrupt"));
        Serial.println("}");
        Serial.flush();        
        break;
      case 'r': 
        radio.getRevision(FW, NULL, &REV);
        Serial.print(F("This is a Si4735-"));
        Serial.print(REV);
        Serial.println(FW);
        Serial.flush();        
        break;
      case 'R': 
        if(radio.isRDSCapable()) {
          decoder.getRDSData(&station);
          Serial.println(F("RDS information {"));
          Serial.print("PI: ");
          Serial.println(station.programIdentifier, HEX);
          Serial.print("PTY: ");
          Serial.println(station.PTY);
          Serial.println("DI {");
          if(station.DICC & SI4735_RDS_DI_DYNAMIC_PTY)
            Serial.println(F("* Dynamic PTY"));
          if(station.DICC & SI4735_RDS_DI_COMPRESSED)
            Serial.println(F("* Audio Compression"));
          if(station.DICC & SI4735_RDS_DI_ARTIFICIAL_HEAD)
            Serial.println(F("* Artificial Head Recording"));
          if(station.DICC & SI4735_RDS_DI_STEREO)
            Serial.println(F("* Stereo Encoding"));
          Serial.println("}");
          if(station.TP) Serial.println(F("Traffic programme carried"));
          if(station.TA) Serial.println(F("Traffic announcement underway"));
          Serial.print(F("Currently broadcasting "));
          if(station.MS) Serial.println("music");
          else Serial.println("speech");
          Serial.print("PS: ");
          Serial.println(station.programService);
          Serial.print("PTYN: ");
          Serial.println(station.programTypeName);
          Serial.print("RT: ");
          Serial.println(station.radioText);
          Serial.println("}");
        } else Serial.println(F("RDS not available."));
        Serial.flush();        
        break;
      case 'T': 
        if(decoder.getRDSTime(&rdstime)) {
          Serial.println(F("RDS CT (group 4A) information:"));
          Serial.print(rdstime.tm_hour);
          Serial.print(":");
          Serial.print(rdstime.tm_min);
          Serial.print("UTC ");
          Serial.print(rdstime.tm_year);
          Serial.print("-");
          Serial.print(rdstime.tm_mon);
          Serial.print("-");
          Serial.println(rdstime.tm_mday);
        } else Serial.println(F("RDS CT not available."));
        Serial.flush();        
        break;
      case '?': 
        Serial.println(F("Available commands:"));
        Serial.println(F("* v/V     - decrease/increase the volume"));
        Serial.println(F("* s/S     - seek down/up with band wrap-around"));
        Serial.println(F("* m/M     - mute/unmute audio output"));
        Serial.println(F("* f       - display currently tuned frequency and mode"));
        Serial.println(F("* L/A/W/F - switch mode to LW/AM/SW/FM"));
        Serial.println(F("* q       - display signal quality metrics"));
        Serial.println(F("* t       - display decoded status byte"));
        Serial.println(F("* r       - display chip and firmware revision"));
        Serial.println(F("* R       - display RDS data, if available"));
        Serial.println(F("* T       - display RDS time, if available"));
        Serial.println(F("* ?       - display this list"));
        Serial.flush();        
        break;
    }
  }   
}
