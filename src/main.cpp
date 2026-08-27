#include <Arduino.h>
#include <PubSubClient.h>
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

const char *MQTT_CLIENT_ID = "air-purifier-button";
const char *MQTT_AVAILABILITY_TOPIC = "air_purifier_button/status";
const char *MQTT_STATE_TOPIC = "air_purifier_button/state";
const char *MQTT_COMMAND_TOPIC = "air_purifier_button/set";
const char *MQTT_DISCOVERY_TOPIC = "homeassistant/switch/air_purifier_button/config";

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

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

void setPurifierState(bool on, bool publish)
{
    purifierOn = on;
    purifierOn ? showOnImage() : showOffImage();

    if (publish)
    {
        mqttClient.publish(MQTT_STATE_TOPIC, purifierOn ? "ON" : "OFF", true);
    }
}

void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    bool on = message == "ON";
    bool off = message == "OFF";
    if (!on && !off)
    {
        return;
    }

    // Commands need to be echoed back as confirmed state; a retained state
    // message we're replaying to ourselves on (re)connect does not.
    bool isCommand = strcmp(topic, MQTT_COMMAND_TOPIC) == 0;
    setPurifierState(on, isCommand);
}

void publishDiscoveryConfig()
{
    String payload = String("{") +
                      "\"name\":\"Air Purifier\"," +
                      "\"unique_id\":\"air_purifier_button\"," +
                      "\"state_topic\":\"" + MQTT_STATE_TOPIC + "\"," +
                      "\"command_topic\":\"" + MQTT_COMMAND_TOPIC + "\"," +
                      "\"availability_topic\":\"" + MQTT_AVAILABILITY_TOPIC + "\"," +
                      "\"payload_available\":\"online\"," +
                      "\"payload_not_available\":\"offline\"," +
                      "\"device\":{\"identifiers\":[\"air_purifier_button\"],"
                      "\"name\":\"Air Purifier Button\","
                      "\"manufacturer\":\"JCZN\",\"model\":\"ESP32-2432S028R\"}" +
                      "}";

    mqttClient.publish(MQTT_DISCOVERY_TOPIC, payload.c_str(), true);
}

void connectToMqtt()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT...");

        bool connected = strlen(MQTT_USERNAME) > 0
            ? mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD, MQTT_AVAILABILITY_TOPIC, 0, true, "offline")
            : mqttClient.connect(MQTT_CLIENT_ID, MQTT_AVAILABILITY_TOPIC, 0, true, "offline");

        if (connected)
        {
            Serial.println("connected");
            mqttClient.publish(MQTT_AVAILABILITY_TOPIC, "online", true);
            publishDiscoveryConfig();
            mqttClient.subscribe(MQTT_COMMAND_TOPIC);
            // Adopt whatever state was last retained (e.g. from before a
            // reboot) instead of resetting to OFF.
            mqttClient.subscribe(MQTT_STATE_TOPIC);
        }
        else
        {
            Serial.printf("failed, rc=%d, retrying in 5s\n", mqttClient.state());
            delay(5000);
        }
    }
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

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);
    connectToMqtt();
}

void loop()
{
    if (!mqttClient.connected())
    {
        connectToMqtt();
    }
    mqttClient.loop();

    bool touched = touchscreen.touched();
    if (touched && !wasTouched)
    {
        setPurifierState(!purifierOn, true);
    }
    wasTouched = touched;

    delay(20);
}
