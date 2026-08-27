#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <XPT2046_Touchscreen.h>
#include "off_image.h"
#include "on_image.h"
#include "secrets.h"

#define XPT2046_CLK 25
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CS 33
#define XPT2046_IRQ 36

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

bool purifierOn = false;
bool wasTouched = false;

void showOffImage()
{
    tft.pushImage(0, 0, off_image_width, off_image_height, off_image);
}

void showOnImage()
{
    tft.pushImage(0, 0, on_image_width, on_image_height, on_image);
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

    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);

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
    bool touched = touchscreen.touched();

    if (touched && !wasTouched)
    {
        purifierOn = !purifierOn;
        purifierOn ? showOnImage() : showOffImage();
    }

    wasTouched = touched;
    delay(20);
}
