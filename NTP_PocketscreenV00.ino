
// Change line 223 with your ssid and password  **********************
//  espATCommand("AT+CWJAP=\"FRITZ!BoxEd\",\"password\"", "OK", LONG_PAUSE);    // connect to wifi


// =============================================================================================================================
/* 
This Arduino code controls the Pocketscreen

Load and install in IDE:
http://files.zepsch.com/arduino/package_zepsch_index.json
Board: Pocketscreen
Built: type default

******>>> RENAME Time.h --> xTime.h in folder ..\Arduino\libraries\Time\


 Author .    : Ed Nieuwenhuys
 Changes-V007: Adapted from Character_Colour_Clock_V066
 Changes V008: Added button control
 Changes V010: Tries to install NTP time server. Watchdog? crashes after AT+CIPSTART="UDP","nl.pool.ntp.org",123
*/
 
//--------------------------------------------
// ARDUINO Includes defines and initialisations
//--------------------------------------------
 
#include <RTCZero.h>
#include <TimeLib.h>             // For time management  REMOVE Time.h from Arduino\libraries\Time\


// -------------------------------------------------------------
// NTP stuff
// Get Time from NTP server (Network Time Protocol, RFC 5905)
// NTP uses UDP/IP packets for data transfer because of the fast
// connection setup and response times.

// The official port number for the NTP (that ntpd and ntpdate listen and talk to) is 123.
// The unit of time is in seconds, and the epoch is 1 January 1900.
// its gives you UTC time, which is the time at Greenwich Meridian (GMT)
// The NTP timestamp is a 64 bit binary value, built from an unsigned 32-bit seconds value
// and a 32-bit fractional part. In this notation the value 3.5 would be represented by the 64-bit string:
// 0000|0000|0000|0000|0000|0000|0000|0011 . 1000|0000|0000|0000|0000|0000|0000|0000
// If you take all the bits as a 64 bit unsigned integer,
// store it in a floating point variable with at least 64 bits of mantissa (usually double)
// and do a floating point divide by 2^32, you'll get the right answer.
// On a standard arduino unfortunately we don't have 64 bits doubles.
// but most RTC have just second level info, so no need to get the second half (or first byte possibly)

// Only the first four bytes of an outgoing NTP packet need to be set for what we want to achieve
// appropriately, the rest can be whatever.
//The header fields of the NTP message are as follows:
//
//LI  Leap Indicator (2 bits)
//This field indicates whether the last minute of the current day is to have a leap second applied. The field values follow:
//0: No leap second adjustment
//1: Last minute of the day has 61 seconds
//2: Last minute of the day has 59 seconds
//3: Clock is unsynchronized
//
//VN  NTP Version Number (3 bits) (current version is 4).
//
//Mode  NTP packet mode (3 bits)
//The values of the Mode field follow:
//0: Reserved
//1: Symmetric active
//2: Symmetric passive
//3: Client
//4: Server
//5: Broadcast
//6: NTP control message
//7: Reserved for private use
//
//Stratum Stratum level of the time source (8 bits)
//The values of the Stratum field follow:
//0: Unspecified or invalid
//1: Primary server
//2–15: Secondary server
//16: Unsynchronized
//17–255: Reserved
//
//Poll  Poll interval (8-bit signed integer)
//The log2 value of the maximum interval between successive NTP messages, in seconds.
//
//Precision Clock precision (8-bit signed integer)
//The precision of the system clock, in log2 seconds.

static const unsigned long ntpFirstFourBytes = 0xEC0600E3; // NTP request header, first 32 bits
const uint8_t NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message


// after a successful querry we will get a 48 byte answer from the NTP server.
// To understand the structure of an NTP querry and answer
// see http://www.cisco.com/c/en/us/about/press/internet-protocol-journal/back-issues/table-contents-58/154-ntp.html
// so if we want to read the "Transmit Timestamp" then we need to
// Read the integer part which are bytes 40,41,42,43
// if we want to round to the nearest second if we want some accuracy
// Then fractionary part are byte 44,45,46,47
// if it is greater than 500ms byte 44 will be > 128
// and thus by only checking byte 44 of the answer we can round to the next second;
// 90% of the NTP servers have network delays below 100ms
// We can also account for an assumed averaged network delay of 50ms, and thus instead of
// comparing with 128 you can compare with (0.5s - 0.05s) * 256 = 115;

#define SECONDROUNDINGTHRESHOLD 115

// the epoch for NTP starts in year 1900 while the epoch in UNIX starts in 1970
// Unix time starts on Jan 1 1970. 7O years difference in seconds, that's 2208988800 seconds
#define SEVENTYYEARS 2208988800UL

// to transform a number of seconds into a current time you need to do some maths
#define NUMBEROFSECONDSPERDAY 86400UL
#define NUMBEROFSECONDSPERHOUR 3600UL
#define NUMBEROFSECONDSPERMINUTE 60UL

// you might not leave in UTC time zone, set up your delta time in hours
#define UTC_DELTA_HOURS 2
#define UTC_DELTA_MINUTES 0
const long UTC_DELTA = ((UTC_DELTA_HOURS * NUMBEROFSECONDSPERHOUR) + (UTC_DELTA_MINUTES*NUMBEROFSECONDSPERMINUTE));

// MANAGE the ESP8266. Using an ESP - 01
// we control the reset pin through the arduino
#define hardRestPIN 22
#define SHORT_PAUSE 1000UL
#define LONG_PAUSE  5000UL
const char * OK_STR = "OK\r\n";

                      
//--------------------------------------------
// Pocketscreen
//--------------------------------------------// we need the library
#include <PocketScreen.h>
PocketScreen pocketscreen = PocketScreen();
bool     PrintDigital=false;
char*    PocketScreenTekst;
uint32_t LastButtonTime=0;
byte     ButtonsPressed = 0;                    // contains button pressed 1= PIN_BUTTON_0; 2 = PIN_BUTTON_1, 4 PIN_BUTTON_2
char    *pinNames[] = {"D0", "D1", "D2", "D3", "D4", "A0", "A1", "A2"};
int      pins[] = {D0, D1, D2, D3, D4, A0, A1, A2};  
/*   // defined 16 bit color
const uint16_t BLACK_16b        = 0x0000;
const uint16_t DARKGRAY_16b     = 0x4208;
const uint16_t GRAY_16b         = 0x8410;
const uint16_t LIGHTGRAY_16b    = 0xBDF7;
const uint16_t WHITE_16b        = 0xFFFF;
const uint16_t RED_16b          = 0xF800;
const uint16_t ORANGE_16b       = 0xFC00;
const uint16_t YELLOW_16b       = 0xFFE0;
const uint16_t GREENYELLOW_16b  = 0x87E0;
const uint16_t GREEN_16b        = 0x07E0;
const uint16_t CYAN_16b         = 0x07FF;
const uint16_t BLUE_16b         = 0x001F;
const uint16_t PURPLE_16b       = 0x801F;
const uint16_t MAGENTA_16b      = 0xF81F;
const uint16_t DARKRED_16b      = 0x8000;
const uint16_t DARKGREEN_16b    = 0x0400;
const uint16_t DARKBLUE_16b     = 0x0010;
*/
//--------------------------------------------
// RTC CLOCK
//--------------------------------------------

RTCZero RTCklok;

//--------------------------------------------
// CLOCK
//--------------------------------------------                                 
#define MAXTEXT 80
char sptext[82];                    // For common print use  
static  uint32_t msTick;                  // the number of millisecond ticks since we last incremented the second counter
byte    Isecond, Iminute, Ihour, Iday, Imonth, Iyear; 
byte    lastminute = 0, lasthour = 0, sayhour = 0;


//  -------------------------------------   End Definitions  ---------------------------------------
//--------------------------------------------
// ARDUINO Setup
//--------------------------------------------
void setup() 
{
 for (int i=0; i<5; i++)  pinMode(pins[i], OUTPUT);                // the digital pins must be declared as output, while the analog pins must not
 SerialUSB.begin(115200);                                             // Setup the serial port to 9600 baud // while (!SerialUSB);   
 pocketscreen.begin();
 pocketscreen.setBitDepth(BitDepth16);   // set font
 pocketscreen.setFont(pocketScreen7pt);
 pocketscreen.drawRect(0, 0, 96, 64, true, LIGHTGRAY_16b);           // BLACK_16b);     // clear screen
 pocketscreen.setFontColor(WHITE_16b, BLACK_16b);                    // Set color
 Tekstprintln("\n*********\nSerial started"); 
 Tekstprintln("Pocket screen started");
 RTCklok.begin();                                                    // Start the RTC-module 
 Tekstprintln("RTC enabled");
// RTCklok.setEpoch(ConvertDateTime(__DATE__, __TIME__));              // Set clock to compile DATE TIME
    // enable ESP
 pinMode(PIN_ESP_ENABLE, OUTPUT);
 digitalWrite(PIN_ESP_ENABLE, HIGH);
 
 SerialESP.begin(115200);
  while (!SerialESP);
    SerialUSB.println("ESP Serial open.");

// PrintTimeInScreen();  delay(5000);
 ConnectToWiFiNetwork();
 Tekstprintln("Connected to network"); 
 msTick = LastButtonTime = millis();
}

//--------------------------------------------
// ARDUINO Loop
//--------------------------------------------
void loop() 
{
 EverySecondCheck();  
}


void ConnectToWiFiNetwork(void)
{
  espATCommand("AT+RESTORE", "ready", LONG_PAUSE); // reset
  emptyESP_RX(SHORT_PAUSE);
  espATCommand("AT", OK_STR, SHORT_PAUSE);                                      //is all OK?
  espATCommand("AT+CWMODE=1", OK_STR, SHORT_PAUSE);                             //Set the wireless mode
  espATCommand("AT+CWQAP", OK_STR, SHORT_PAUSE);                                //disconnect  - it shouldn't be but just to make sure
  espATCommand("AT+CWJAP=\"FRITZ!BoxEd\",\"eddie2506\"", "OK", LONG_PAUSE);    // connect to wifi
  espATCommand("AT+CIPMUX=0", OK_STR, SHORT_PAUSE);                            //set the single connection mode
}

// =====================================================================================

// --------------------------------------
// emptyESP_RX waits for duration ms
// and get rid of anything arriving
// on the ESP Serial port during that delay
// --------------------------------------

void emptyESP_RX(unsigned long duration)
{
  unsigned long currentTime;
  currentTime = millis();
  while (millis() - currentTime <= duration) {
    if (SerialESP.available() > 0) SerialESP.read();
  }
}

// --------------------------------------
// waitForString wait max for duration ms
// while checking if endMarker string is received
// on the ESP Serial port
// returns a boolean stating if the marker
// was found
// --------------------------------------

boolean waitForString(const char * endMarker, unsigned long duration)
{
  int localBufferSize = strlen(endMarker); // we won't need an \0 at the end
  localBufferSize = 120;
  char localBuffer[localBufferSize];
  int index = 0;
  boolean endMarkerFound = false;
  unsigned long currentTime;

  memset(localBuffer, '\0', localBufferSize); // clear buffer

  currentTime = millis();
  while (millis() - currentTime <= duration) {
    if (SerialESP.available() > 0) {
      if (index == localBufferSize) index = 0;
      localBuffer[index] = (uint8_t) SerialESP.read();
      SerialUSB.write(localBuffer[index]);
      endMarkerFound = true;
      for (int i = 0; i < localBufferSize; i++) {
        if (localBuffer[(index + 1 + i) % localBufferSize] != endMarker[i]) {
          endMarkerFound = false;
          break;
        }
      }
      index++;
    }
    if (endMarkerFound) break;
  }
  SerialUSB.write(13);
  return endMarkerFound;
}


// --------------------------------------
// espATCommand executes an AT commmand
// checks if endMarker string is received
// on the ESP Serial port for max duration ms
// returns a boolean stating if the marker
// was found
// --------------------------------------

boolean espATCommand(const char * command, const char * endMarker, unsigned long duration)
{
  SerialESP.println(command);
  return waitForString(endMarker, duration);
}


// --------------------------------------
// epochUnixNTP returns the UNIX time
// number of seconds sice Jan 1 1970
// adjusted for timezoneDeltaFromUTC
// --------------------------------------

unsigned long epochUnixNTP()
{
  unsigned long secsSince1900 = 0UL;
  unsigned long epochUnix;
  Tekstprintln("In Epoch1"); 

// Error text ------------------------------------
//In Epoch1
//AT+CIPSTART="UDP","nl.pool.ntp.org",123
//
//
//AT+CIPSEND=48
//
//busy p...
//
//In Epoch2 -------------
//⸮ ⸮            ⸮ ⸮⸮⸮⸮⸮         ⸮⸮  PP⸮P⸮PP   p⸮
// ets Jan  8 2013,rst cause:4, boot mode:(3,6)
//
//wdt reset
//load 0x40100000, len 212, room 16 
//tail 4
//chksum 0x5e
//load 0x3ffe8000, len 788, room 4 
//tail 0
//chksum 0x1c
//load 0x3ffe8314, len 72, room 8 
//tail 0
//chksum 0x55
//csum 0x55
//jump to user1
//
//In Epoch2a ********
//
//ready
// --------------------------------------------



  espATCommand("AT+CIPSTART=\"UDP\",\"nl.pool.ntp.org\",123", OK_STR, LONG_PAUSE);
  espATCommand("AT+CIPSEND=48", OK_STR, SHORT_PAUSE); //send 48 (NTP_PACKET_SIZE) bytes
 // emptyESP_RX(1000UL); // empty the buffer (we get a > character)
  SerialESP.write((char*) &ntpFirstFourBytes, NTP_PACKET_SIZE); // the first 4 bytes matters, then we don't care, whatever is in the memory will do
Tekstprintln("In Epoch2 -------------"); 
  // skip  AT command answer ("Recv 48 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,48:")
  
  waitForString(":", SHORT_PAUSE);
  Tekstprintln("In Epoch2a ********");
  // read the NTP packet, extract the TRANSMIT TIMESTAMP in Seconds from bytes 40,41,42,43
  for (int i = 0; i < NTP_PACKET_SIZE; i++) {
     while (!SerialESP.available());
    int c = SerialESP.read();
    SerialUSB.write(c);
    if ((i >= 40) && (i < 44))  secsSince1900 = (secsSince1900 << 8) + (unsigned long) ((uint8_t) (c & (int) 0x00FF)); // Read the integer part of sending time
    else if (i == 44) secsSince1900 += (((uint8_t) c) > SECONDROUNDINGTHRESHOLD ? 1 : 0);
  }
Tekstprintln("In Epoch3"); 
  // subtract seventy years:
  epochUnix = secsSince1900 - SEVENTYYEARS;


  espATCommand("AT+CIPCLOSE", OK_STR, SHORT_PAUSE); // close connection
  Tekstprintln("In Epoch4"); 
  return epochUnix + UTC_DELTA;
}


//--------------------------------------------
// CLOCK Update routine done every second
//--------------------------------------------
void EverySecondCheck(void)
{
 int32_t msLeap       = millis()- msTick;
// if (msLeap >100)  pocketscreen.setLEDColor(LEDRed);      // Turn OFF the second on SecondsPin. Minimize DigitalWrites.
 if (msLeap > 999)                                          // Every second enter the loop
  {
   GetTijd(0);                                             // Update Isecond, Iminute, Ihour, Iday, Imonth, Iyear
   msTick = millis();
   if (Iminute != lastminute)   EveryMinuteUpdate();        // Enter the every minute routine after one minute
                                  // Check serial port every second
  }
  
 }
//--------------------------------------------
// CLOCK Update routine done every minute
//--------------------------------------------
 void EveryMinuteUpdate(void)
 {
  static byte lastday = 0;
  lastminute = Iminute;  
  Tekstprintln("In minute"); 
  int  ntpHours, ntpMinutes, ntpSeconds;
  unsigned long epochUnix;   
  epochUnix = epochUnixNTP();
  ntpHours = (epochUnix  % NUMBEROFSECONDSPERDAY) / NUMBEROFSECONDSPERHOUR;
  ntpMinutes = (epochUnix % NUMBEROFSECONDSPERHOUR) / NUMBEROFSECONDSPERMINUTE;
  ntpSeconds = epochUnix % NUMBEROFSECONDSPERMINUTE;
 sprintf(sptext,"NTP:%0.2d:%0.2d:%0.2d %0.2d-%0.2d-%0.4d",ntpHours,ntpMinutes,ntpSeconds,RTCklok.getDay(),RTCklok.getMonth(),RTCklok.getYear()+2000);
 Tekstprintln(sptext);
     
 }

//--------------------------------------------
// CLOCK common print routines
//--------------------------------------------
void Tekstprint(char tekst[])  { SerialUSB.print(tekst);}
void Tekstprintln(char tekst[]){ SerialUSB.println(tekst);}



//--------------------------------------------
// DS3231 Get time from DS3231
//--------------------------------------------
void GetTijd(byte printit)
{
 Ihour   = RTCklok.getHours();
 Iminute = RTCklok.getMinutes();
 Isecond = RTCklok.getSeconds();
 Iday    = RTCklok.getDay();
 Imonth  = RTCklok.getMonth();
 Iyear   = RTCklok.getYear();
// if (Ihour > 24) { Ihour = random(12)+1; Iminute = random(60)+1; Isecond = 30;}  // set a time if time module is absent or defect
 if (printit)  Print_RTC_tijd(); 
}

//--------------------------------------------
// DS3231 utility function prints time to serial
//--------------------------------------------
void Print_RTC_tijd(void)
{
 sprintf(sptext,"%0.2d:%0.2d:%0.2d %0.2d-%0.2d-%0.4d",RTCklok.getHours(),RTCklok.getMinutes(),RTCklok.getSeconds(),RTCklok.getDay(),RTCklok.getMonth(),RTCklok.getYear()+2000);
 Tekstprintln(sptext);
}

//--------------------------------------------
// CLOCK utility function prints time to serial
//--------------------------------------------
void Print_tijd(void)
{
 sprintf(sptext,"%0.2d:%0.2d:%0.2d",Ihour,Iminute,Isecond);
 Tekstprintln(sptext);
}

//--------------------------------------------
// DS3231 Set time in module and print it
//--------------------------------------------
void SetRTCTime(void)
{ 
 Ihour   = constrain(Ihour  , 0,24);
 Iminute = constrain(Iminute, 0,59); 
 Isecond = constrain(Isecond, 0,59); 
 RTCklok.setTime(Ihour, Iminute, Isecond);
 GetTijd(0);                                     // synchronize time with RTC clock
 Print_tijd();
}

/*
//--------------------------------------------
// DS3231 Convert compile DATA & TIME to time_t
//--------------------------------------------
time_t ConvertDateTime(char const *date, char const *time)
{
 char s_month[5];
 int year, day;
 int hour, minute, second;
 tmElements_t t;
 static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
 sscanf(date, "%s %d %d", s_month, &day, &year);                  // sscanf(time, "%2hhd %*c %2hhd %*c %2hhd", &t.Hour, &t.Minute, &t.Second);
 sscanf(time, "%2d %*c %2d %*c %2d", &hour, &minute, &second);   //  Find where is s_month in month_names. Deduce month value.
 t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;
 t.Day = day;         // year can be given as '2010' or '10'. It is converted to years since 1970
 if (year > 99) t.Year = year - 1970;
 else t.Year = year + 50;
 t.Hour = hour;
 t.Minute = minute;
 t.Second = second;
 return makeTime(t);
}
*/
// ------------------- End  Time functions 
