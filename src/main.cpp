#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include "off_image.h"
#include "secrets.h"

TFT_eSPI tft = TFT_eSPI();

void showOffImage()
{
    tft.pushImage(0, 0, off_image_width, off_image_height, off_image);
}

void setup()
{
    Serial.begin(115200);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    showOffImage();

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
}
