#include "communication.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "eepromstore.h"
#include "rtcc.h"

// Local helper functions
void sendNTPpacket(IPAddress& address, WiFiUDP& udp, byte* packetBuffer);
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

Communication::Communication() { begun = false; }
Communication::~Communication() {}

void Communication::begin(void) {
    if (begun) return;
    eepromStore.readPage((uint8_t*)ssid, EEPROM_FIRST_WIFIPAGE + 2 * Clock.store.lastUsedWifi);
    eepromStore.readPage((uint8_t*)psk, EEPROM_FIRST_WIFIPAGE + 2 * Clock.store.lastUsedWifi + 1);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, psk);
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    begun = true;
}

time_t Communication::getNtpTime() {
    unsigned int ntpLocalPort = 2390;  // local port to listen for UDP packets for NTP
    IPAddress timeServerIP;
    const char* ntpServerName = "pool.ntp.org";
    byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming and outgoing packets
    WiFiUDP udp;
    udp.begin(ntpLocalPort);

    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP, udp, packetBuffer);  // send an NTP packet to a time server

    delay(100);
    int cb = udp.parsePacket();
    int tries = 100;
    while (!cb && tries-- > 0) {
        delay(100);
        cb = udp.parsePacket();
    }
    if (!cb) {
        Serial.println("no time received");
        return 0;
    } else {
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;

        // now convert NTP time into everyday time:
        Serial.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        // print Unix time:
        Serial.println(epoch);
        return epoch;
    }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address, WiFiUDP& udp, byte* packetBuffer) {
    Serial.println("sending NTP packet...");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;  // LI, Version, Mode
    packetBuffer[1] = 0;           // Stratum, or type of clock
    packetBuffer[2] = 6;           // Polling Interval
    packetBuffer[3] = 0xEC;        // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, 123);  //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
}

Communication Comms;
