

//Due to a bug in Arduino, these need to be included here too/first
#include <Wire.h>
#include <SPI.h>

//UWAFT Si4737 I2C Functions
#include <Si4737_i2c.h>

// Setup address and reset pin
#define _i2caddr 0x11
#define mcuradio_rst 9

#define POWER_UP 0x01
#define FM_SEEK_START 0x21

//Create our Si4737 radio
Si4737 radio;

//Other variables we will use below
byte response[16], status, command[8], numDigits;
char collectedDigits[17] = "", serial_in, oneHexValue[5] = "0x";

void setup() {
  Serial.begin(115200);
  /*
  pinMode(mcuradio_rst, OUTPUT); // Set reset to output
   digitalWrite(mcuradio_rst, LOW); // Put device in reset mode and wait a little
   delay(1);
   digitalWrite(mcuradio_rst, HIGH); // Bring device out of reset and wait for it to be ready
   delay(1);
   Serial.println("Start I2C Mode");
   Wire.begin();                     // Start I2C
   */
  radio.begin(SI4735_MODE_FM);

  Serial.println("Radio Powered up, FM mode");
  //radio.sendCommand(SI4735_CMD_POWER_UP, 0xC0, 0xB5);  // Send Power-up command, enable interrupts, set external clock source, set device to FM mode, enable digital and analog audio outputs.
  delay(100);                          // Wait for Boot 

  seekup();
  Serial.println("Seek to first available station");
  Serial.println();
}

void loop()
{       
  //Wait to do something until a character is received on the serial port.
  if(Serial.available() > 0){
    //Copy the incoming character to a variable and fold case
    serial_in = toupper(Serial.read());
    //Depending on the incoming character, decide what to do.
    switch(serial_in){
      case 'U':
        Serial.println("Seeking up");
        seekup();
      case 'R':
        //Get the latest response from the radio.
        radio.getResponse(response);
        //Print all 16 bytes in the response to the terminal.      
        Serial.print(F("Si4735 RSP"));
        for(int i = 0; i < 4; i++) {
          if(i) Serial.print(F("           "));
          else Serial.print(" ");
          for(int j = 0; j < 4; j++) {
            Serial.print("0x");
            Serial.print(response[i * 4 + j], HEX);
            Serial.print(" [");
            Serial.print(response[i * 4 + j], BIN);
            Serial.print("]");
            if(j != 3) Serial.print(", ");
            else
              if(i != 3) Serial.print(",");
          };
          Serial.println("");
        };
        Serial.flush();        
        break;
      case 'S':
        status = radio.getStatus();
        Serial.print(F("\nSi4735 STS 0x"));
        Serial.print(status, HEX);
        Serial.print(" [");
        Serial.print(status, BIN);
        Serial.println("]");
        Serial.flush();        
        break;
      case 'X':
        collectedDigits[0] = '\0';
        Serial.println(F("Command string truncated, start over."));
        Serial.flush();        
        break;
      //W is for write, sends the signal to radio IC
      case 'W':
        numDigits = strlen(collectedDigits);
        //Silently ignore empty lines, this also gives us CR & LF support
        if(numDigits) {
          if(numDigits % 2 == 0){    
            memset(command, 0x00, 8);
            for(int i = 0; i < (numDigits / 2) ; i++) {
              strncpy(&oneHexValue[2], &collectedDigits[i * 2], 2);
              command[i] = strtoul(oneHexValue, NULL, 16);
            }
            //End command string echo
            Serial.println("");
            //Send the current command to the radio.
            radio.sendCommand(command[0], command[1], command[2],
                              command[3], command[4], command[5],
                              command[6], command[7]);
          } else Serial.println(F("Odd number of hex digits, need even!"));
        }
        Serial.flush();
        collectedDigits[0] = '\0'; //Clears command for next round
        break;
      case '?':
        Serial.println(F("Available commands:"));
        Serial.println(F("* r      - display response (long read)"));
        Serial.println(F("* s      - display status (short read)"));
        Serial.println(F("* x      - flush (empty) command string and start over"));
        Serial.println(F("* 0-9a-f - compose command, at most 16 hex digits (forming an 8 byte"));
        Serial.println(F("           command) are accepted"));
        Serial.println(F("* CR/LF  - send command"));
        Serial.println(F("* ?      - display this list"));
        Serial.flush();
        break;
      //If we get any other character and it's a valid hexidecimal character,
      //copy it to the command string.
      default:
          /*Serial.print("serial_in=");
          Serial.print(serial_in); 
          Serial.print("\n"); 
        */
		  if((serial_in >= '0' && serial_in <= '9') || 
           (serial_in >= 'A' && serial_in <= 'F'))
          if(strlen(collectedDigits) < 16) {
            strncat(collectedDigits, &serial_in, 1);
            //Echo the command string as it is composed
            Serial.print(serial_in);
          } else Serial.println(F("Too many hex digits, 16 maximum!"));
        else Serial.println(F("Invalid command!"));
        Serial.flush();
        break;
    }
  }   
}

void seekup() {
  Wire.beginTransmission(_i2caddr); 
  Wire.write(FM_SEEK_START);      
  Wire.write(0xC);
  Wire.endTransmission();   
  delay(100);
}




