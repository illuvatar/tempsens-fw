#include "communication.h"

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <rBase64.h>

#include "eepromstore.h"
#include "rtcc.h"
#include "settings.h"
#include "tools.h"

// Local helper functions
void sendNTPpacket(IPAddress& address, WiFiUDP& udp, byte* packetBuffer);
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

// DST Root CA X3 (Letsencrypt) - Expires Thursday 30 September 2021 14:01:15
const char DST_ROOT_CA_X3[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT
DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow
PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD
Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O
rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq
OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b
xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw
7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD
aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG
SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69
ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr
AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz
R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5
JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo
Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ
-----END CERTIFICATE-----
)EOF";

// ISRG Root X1 (Letsencrypt) - Expires Jun  4 11:04:38 2035 GMT

const char ISRG_Root_X1[] PROGMEM = R"EOF(
    -----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

Communication::Communication() { begun = false; }
Communication::~Communication() {}

void Communication::begin(void) {
    if (begun) return;
    char baseUrlTemp[65];
    eepromStore.readPage((uint8_t*)ssid, EEPROM_FIRST_WIFIPAGE + 2 * Clock.store.lastUsedWifi);
    eepromStore.readPage((uint8_t*)psk, EEPROM_FIRST_WIFIPAGE + 2 * Clock.store.lastUsedWifi + 1);
    eepromStore.readPage((uint8_t*)baseUrlTemp, EEPROM_URL_PAGE);
    baseUrl = String(baseUrlTemp);
    if (!baseUrl.endsWith("/")) baseUrl += "/";
    ssl = false;
    if (baseUrl.startsWith("https://")) {
        port = 443;
        ssl = true;
    } else if (baseUrl.startsWith("http://")) {
        port = 80;
    }
    if (port > 0) {
        int startOfServer = baseUrl.indexOf(':') + 3;
        int endOfServer = baseUrl.indexOf(':', startOfServer);
        if (endOfServer < 0) {
            // No port specified
            endOfServer = baseUrl.indexOf('/', startOfServer);
        } else {
            // Extract port as well
            int endOfPort = baseUrl.indexOf('/', startOfServer);
            //parse port as int
            String portPart = baseUrl.substring(endOfServer + 1, endOfPort);
            port = portPart.toInt();
        }
        server = baseUrl.substring(startOfServer, endOfServer);
    }

    WiFi.persistent(false);  // Make sure the credentials are NOT stored persistently by the chip as they already are stored in EEPROM.
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

bool Communication::registerDevice(void) {
    char pageBuffer[65];

    eepromStore.readPage((uint8_t*)pageBuffer, EEPROM_REGISTER_SECRET_PAGE);
    String secretString = String(pageBuffer);

    String query = "{\"serial\":" + String(settings.store.serialno) + ",\"token\":\"" + secretString + "\"}";

    String result = jsonQuery("register", query);
    Serial.print("Registration result: '");
    Serial.print(result);
    Serial.println("'");
    // {"status":"registered","blen":60,"crc":2482489620,"token":"FaCjf\/WwxYDb9kHkGzOKVOj0iNj9LpCNAPA9\/Lz5BVlzyyUUDw6n+Df+bTpNmmhmhBYYl3lpgy47V6ifFMn3kw=="}
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, result);
    JsonObject obj = doc.as<JsonObject>();
    if (obj["status"] != "registered") {
        Serial.println("Registration failed");
        return false;
    }
    String tokenBase64 = obj["token"];
    if (tokenBase64 == NULL) {
        Serial.println("No token received");
        return false;
    }
    if (tokenBase64.length() > sizeof(registrationBase64)) {
        Serial.println("Token too long");
        return false;
    }
    tokenBase64.toCharArray(registrationBase64, sizeof(registrationBase64));

    // Decode token
    int tokenLen = rbase64_decode(pageBuffer, registrationBase64, strlen(registrationBase64));

    Serial.print("Decoded token len: ");
    Serial.println(tokenLen);
    pageBuffer[tokenLen] = 0x00;

    if (!checkCrcBuf((uint8_t*)pageBuffer, EEPROM_PAGESIZE)) {
        Serial.println("Token CRC Failed");
        return false;
    }

    Serial.println("Registration OK");
    eepromStore.writePage((uint8_t*)pageBuffer, EEPROM_DEVICE_TOKEN_PAGE);
    settings.registered = true;

    return true;
}

String Communication::jsonQuery(String service, String query) {
    if (port == 0) return "PORTMISSING";
    Serial.print("Query: '");
    Serial.print(query);
    Serial.println("'");

    X509List cert(ISRG_Root_X1);
    cert.append(DST_ROOT_CA_X3);

    WiFiClientSecure client;
    client.setX509Time(Clock.getTime());
    client.setTrustAnchors(&cert);
    String url = baseUrl + "api/" + service + ".php";
    Serial.print("Url: '");
    Serial.print(url);
    Serial.println("'");
    if (!client.connect(server, port)) {
        return "Connection Failed";
    }

    String queryLen = String(query.length());

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "User-Agent: Tempsens2.0\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + queryLen + "\r\n" +
                 "Connection: close\r\n\r\n" +
                 query + "\r\n");
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
    String line = client.readStringUntil('\n');
    line = client.readStringUntil('\n');
    line.trim();

    return line;
}

Communication Comms;
