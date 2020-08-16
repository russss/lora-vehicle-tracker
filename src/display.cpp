#include "main.h"
#include "display.h"
#include "lora.h"
#include "power.h"
#include "fonts.h"
#include <SSD1306.h>

#define MAX_SCREENS 4

SSD1306 display (OLED_I2C_ADDR, OLED_SDA, OLED_SCL);
char buf[60];
volatile char display_state = 1;
volatile bool display_redraw = false;

void handleButton() {
    // Interrupt handler for GPIO38 button, advances the screen state (with debouncing)
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) {
        display_state = (display_state + 1) % MAX_SCREENS;
	display_redraw = true;
    }
    last_interrupt_time = interrupt_time;
}

void initDisplay() {
    power.setPeripheralBus(true);
    delay(100);
    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(55, 15, "Starting...");
    display.display();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, FALLING);
}

void drawLine(int pos, const char *string) {
    display.drawString(0, pos * 15, String(string));
}

void display1() {
    char pos = 0;
    if (gps.charsProcessed() < 10) {
        drawLine(pos++, "*****************");
        drawLine(pos++, "NO GPS DATA");
        drawLine(pos++, "*****************");
        return;
    }

    sprintf(buf, "Time: %02i:%02i:%02i, Sats: %i", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.satellites.value());
    drawLine(pos++, buf);
    
    if (gps.satellites.value() > 2) {
	    sprintf(buf, "Position: %.3f, %.3f", gps.location.lat(), gps.location.lng());
	    drawLine(pos++, buf);

	    sprintf(buf, "Altitude: %.0fm", gps.altitude.meters());
	    drawLine(pos++, buf);
    } else {
        drawLine(pos++, "GPS: No lock");
    }

    sprintf(buf, "LoRA: %s", (char *)&loraStatus);
    drawLine(pos++, buf);
}

void display2() {
    char pos = 0;
    PowerState state = power.getState();
    sprintf(buf, "Vbus: %.2fV, %.2fA", state.bus_v, state.bus_a);
    drawLine(pos++, buf);
    sprintf(buf, "Battery: %.2fV", state.batt_v);
    drawLine(pos++, buf);
}

void display3() {
    char pos = 0;
    PowerState state = power.getState();
    drawLine(pos++, "DC-DC Rails:");
    sprintf(buf, "1: %.1fV, 2: %.1fV, 3: %.1fV", state.dcdc1_v, state.dcdc2_v, state.dcdc3_v);
    drawLine(pos++, buf);
    drawLine(pos++, "LDO Rails:");
    sprintf(buf, "1: 3.3V, 2: %.1fV, 3: %.1fV", state.ldo2_v, state.ldo3_v);
    drawLine(pos++, buf);
}

void displayStatus() {
    // Limit redraw to 4Hz unless button has been pressed
    static unsigned long last_draw_time = 0;
    unsigned long time = millis();
    if (time - last_draw_time < 250 && !display_redraw) {
	return;
    }

    display.clear();
    display.setFont(ArialMT_Plain_10);
    switch (display_state) {
	case 0:
	    // Display_state 0 is blank
	    break;
	case 1:
            display1();
	    break;
	case 2:
	    display2();
	    break;
	case 3:
	    display3();
	    break;
    }

    if (display_state != 0) {
    	display.setFont(DejaVu_Sans_8);
	sprintf(buf, "%i/%i", display_state, MAX_SCREENS - 1);
	display.drawString(110, 55, buf);
    }

    display.display();
    last_draw_time = time;
    display_redraw = false;
}

