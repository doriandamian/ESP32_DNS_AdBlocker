#include <WiFi.h>
#include <AsyncUDP.h>
#include <WiFiUdp.h>

const char* ssid = "laborator-a7";
const char* password = "laborator2022";

AsyncUDP udp;
WiFiUDP forwarder;
IPAddress dnsServer(8, 8, 8, 8);

bool shouldBlockDomain(const String& domain) {
    return domain.endsWith(".ro.");
}

void setup() {
    Serial.begin(9600);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    
    Serial.println("WiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    if (udp.listen(53)) {
        Serial.println("DNS Server started on port 53.");
        
        udp.onPacket([](AsyncUDPPacket packet) {
            if (packet.length() > 0) {
                uint8_t* data = packet.data();
                int dataLength = packet.length();
                
                String domainName = "";
                int i = 12; // DNS header is 12 bytes
                while (i < dataLength) {
                    int length = data[i];
                    if (length == 0) break;
                    domainName += String((char*)(data + i + 1), length) + ".";
                    i += length + 1;
                }
                
                Serial.println("Domain requested: " + domainName);
                
                if (shouldBlockDomain(domainName)) {
                    Serial.println("BLOCKING DOMAIN: " + domainName + " !!!");
                    
                    // Generate a fake response to block the domain
                    uint8_t response[512];
                    memcpy(response, data, dataLength);
                    response[2] |= 0x80; // Set QR (response) flag
                    response[3] |= 0x03; // Set RCODE to NXDOMAIN (3)
                    
                    packet.write(response, dataLength);
                } else {
                    // Serial.println("Forwarding request for domain: " + domainName);
                    
                    // Forward the request to the public DNS server
                    forwarder.beginPacket(dnsServer, 53);
                    forwarder.write(data, dataLength);
                    forwarder.endPacket();
                    
                    // Wait for the response
                    delay(100);
                    int responseSize = forwarder.parsePacket();
                    if (responseSize > 0) {
                        uint8_t response[512];
                        responseSize = forwarder.read(response, sizeof(response));
                        Serial.write(response, 512);
                        // Send the response back to the client
                        packet.write(response, responseSize);
                    } else {
                        Serial.println("No response from public DNS server.");
                    }
                }
            }
        });
    } else {
        Serial.println("Failed to start DNS server.");
    }
}

void loop() {
    // No need to implement loop for AsyncUDP
}
