/// ESP8266 Program for interfacing with TheKit Pico
/*
 *  thekit_receiver.ino
 *  Copyright (C) 2022 Zhang Maiyun <myzhang1029@hotmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <uri/UriBraces.h>

ESP8266WebServer server(80);

void print_pad5(long n) {
    if (n < 10000)
        Serial.print(F("0"));
    if (n < 1000)
        Serial.print(F("0"));
    if (n < 100)
        Serial.print(F("0"));
    if (n < 10)
        Serial.print(F("0"));
    Serial.print(n);
}

void send_and_check(String data) {
    for (long index = 0; index < data.length(); ++index) {
        char c = data[index];
        Serial.print(c);
        while (Serial.available() == 0) {
            /* wait until Pico confirms */
            delay(1);
        }
        (void)(Serial.read() == c);
    }
}

void setup(void) {
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    WiFi.begin(F("SSID"), F("PASSWORD"));

    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    MDNS.begin(F("stephlight"));

    // Fake command to abort base64 decode
    server.on(F("/-"), [](){
        Serial.print(F("-"));
        server.send(200, F("text/plain"), F("Nothing is happening."));
    });

    server.on(F("/"), [](){
        server.send(200, F("text/plain"),
                    F("This is an ESP8266 for Stephanie!"));
    });

    server.on(F("/l"), [](){
        Serial.print(F("l"));
        server.send(200, F("text/plain"), F("Light turned off."));
    });

    server.on(F("/h"), [](){
        Serial.print(F("h"));
        server.send(200, F("text/plain"), F("Light turned on."));
    });

    server.on(UriBraces(F("/g/{}")), [](){
        String sizestr = server.pathArg(0);
        long size = sizestr.toInt();
        if (server.method() != HTTP_POST)
            server.send(405, "text/plain", "Method Not Allowed");
        else {
            Serial.print(F("g"));
            print_pad5(size);
            send_and_check(server.arg(F("plain")));
            server.send(200, F("text/plain"), F("Sent ") + sizestr + F(" bytes."));
        }
    });

    server.on(F("/c"), [](){
        Serial.print(F("c"));
        server.send(200, F("text/plain"), F("Buffer cleared."));
    });

    server.on(F("/s"), [](){
        Serial.print(F("s"));
        server.send(200, F("text/plain"), F("Background tasks cleared."));
    });

    server.on(F("/P"), [](){
        Serial.print(F("P"));
        server.send(200, F("text/plain"), F("Playing background audio."));
    });

    server.on(F("/R"), [](){
        Serial.print(F("R"));
        server.send(200, F("text/plain"), F("Playing recorded audio."));
    });

    server.on(UriBraces(F("/B/{}")), [](){
        String intstr = server.pathArg(0);
        long interval = intstr.toInt();
        Serial.print(F("B"));
        print_pad5(interval);
        server.send(200, F("text/plain"),
                    F("Toggling every ") + String(interval * 100) +
                        F(" milliseconds."));
    });

    server.begin();
    MDNS.addService(F("http"), F("tcp"), 80);
}

void loop(void) {
    MDNS.update();
    server.handleClient();
}
