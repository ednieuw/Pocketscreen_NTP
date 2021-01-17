# Pocketscreen_NTP
Get the time from an NTP server via UDP with AT commands for ESP8266EX

See here the watch were the code must be working in to get the NTP time into the watch
https://github.com/ednieuw/Pocketscreen_Wordwatch

In NTP_PocketscreenV00.ino I got a crash after sending a NTPpacket
I changed in NTP_PocketscreenV01.ino
espATCommand("AT+CIPMUX=0", OK_STR, SHORT_PAUSE);  --> AT+CIPMUX=1
Adding port 4, in some AT commands solved many of the crashes
  espATCommand("AT+CIPSTART=4,\"UDP\",\"193.78.240.12\",123,1112,0", OK_STR, LONG_PAUSE);
  espATCommand("AT+CIPSEND=4,48", OK_STR, SHORT_PAUSE);                           //send 48 (NTP_PACKET_SIZE) bytes
 
I also added a complete packetBuffer instead of only 4 bytes. But probably not necessary
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52; 
  Now the program waits here:
   for (int i = 0; i < NTP_PACKET_SIZE; i++) {
     while (!SerialESP.available());
 
 forever.
 Something wrong with the packet size that was sent?
