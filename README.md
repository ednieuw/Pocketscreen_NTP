# Pocketscreen_NTP
Get the time from an NTP server via UDP with AT commands for ESP8266EX

See here the watch were the code must be working in to get the NTP time into the watch
https://github.com/ednieuw/Pocketscreen_Wordwatch

In NTP_PocketscreenV00.ino I got a crash after sending a NTPpacket<br>
I changed in NTP_PocketscreenV01.ino<br>
espATCommand("AT+CIPMUX=0", OK_STR, SHORT_PAUSE);  --> AT+CIPMUX=1<br>
Adding port 4, in some AT commands solved many of the crashes<br>
  espATCommand("AT+CIPSTART=4,\"UDP\",\"193.78.240.12\",123,1112,0", OK_STR, LONG_PAUSE);<br>
  espATCommand("AT+CIPSEND=4,48", OK_STR, SHORT_PAUSE);                           //send 48 (NTP_PACKET_SIZE) bytes<br>
 
I also added a complete packetBuffer instead of only 4 bytes. But probably not necessary<br>
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode<br>
  packetBuffer[1] = 0;           // Stratum, or type of clock<br>
  packetBuffer[2] = 6;           // Polling Interval<br>
  packetBuffer[3] = 0xEC;        // Peer Clock Precision<br>
  // 8 bytes of zero for Root Delay & Root Dispersion<br>
  packetBuffer[12]  = 49;<br>
  packetBuffer[13]  = 0x4E;<br>
  packetBuffer[14]  = 49;<br>
  packetBuffer[15]  = 52; <br>
  Now the program waits here:<br>
   for (int i = 0; i < NTP_PACKET_SIZE; i++) {<br>
     while (!SerialESP.available());<br>
 
 forever.<br>
 Something wrong with the packet size that was sent?<br>
