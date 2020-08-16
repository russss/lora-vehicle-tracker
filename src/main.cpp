#include "main.h"
#include "lora.h"
#include "display.h"

const int32_t pos_multiplier = 100000;

TinyGPSPlus gps;
HardwareSerial GPS(1);
Power power;

void initGPS() {
    power.setGPS(true);
    GPS.begin(9600, SERIAL_8N1, 34, 12);
}

void setup() {
    Serial.begin(115200);
    Serial.println("** Boot start, waiting for serial...");
    delay(1500);

    Serial.println("** Initialising PMIC...");
    while (!power.init()) {
        delay(500);
    }

    power.logStatus();
    Serial.println("** Initialising GPS...");
    initGPS();
    Serial.println("** Initialising screen...");
    initDisplay();
    Serial.println("** Initialising LoRA...");
    power.setLoRA(true);
    initLoRA();
    Serial.println("** Boot complete");
}

uint8_t createPacket(uint8_t *buffer) {
    uint8_t cursor = 0;
    int32_t lat = gps.location.lat() * pos_multiplier;
    int32_t lon = gps.location.lng() * pos_multiplier;
    PowerState state = power.getState();

    // Low 7 bits of this byte are the battery voltage in decivolts (!)
    // High bit is whether there are more than two satellites.
    buffer[cursor++] = (state.batt_v * 10) + ((gps.satellites.value() > 2) << 7);

    buffer[cursor++] = lat >> 16;
    buffer[cursor++] = lat >> 8;
    buffer[cursor++] = lat;
    buffer[cursor++] = lon >> 16;
    buffer[cursor++] = lon >> 8;
    buffer[cursor++] = lon;

    Serial.print("Sending packet: ");
    for (int i = 0; i < cursor; i++) {
        Serial.print(buffer[i], HEX);
    }
    Serial.print("\n");

    return cursor;
}

void loop() {
    displayStatus();
    loopLoRA();
    while (GPS.available()) {
	int character = GPS.read();
        gps.encode(character);
	//Serial.write(character);
    }

    static unsigned long last_status_output = 0;
    static unsigned long last_packet_time = 0;

    unsigned long time = millis();
    if (time - last_status_output > 5000) {
	char buf[50];
	PowerState state = power.getState();
	sprintf(buf, "Bus: %.3f V, %.3f A", state.bus_v, state.bus_a);
	Serial.println(buf);
	last_status_output = time;
    }

    if (time - last_packet_time > 600 * 1000) {
        uint8_t packet_buffer[8];
        uint8_t size = createPacket((uint8_t*)&packet_buffer);
        sendLoRA(1, (uint8_t*)&packet_buffer, size, false);
        last_packet_time = time;
    }

    // TODO: can we idle here, or will it affect LMIC?
}


