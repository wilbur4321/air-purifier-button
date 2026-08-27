#include <Arduino.h>
#include <WiFi.h>
#include "secrets.h"

void setup()
{
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Hello world");
}

void loop()
{
    Serial.println("Hello world from loop");
    delay(1000);
}
