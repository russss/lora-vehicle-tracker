#include "lora.h"
#include "config.h"

#include <algorithm>
#include "WiFi.h"
#include "hal/hal.h"
#include "SPI.h"

char loraStatus[20];

// LoRA EUI (bytes reversed) set from WiFi MAC
uint8_t dev_eui[EUI_SIZE];

void os_getArtEui(u1_t* buf) { memcpy(buf, app_eui, EUI_SIZE); }
void os_getDevEui(u1_t* buf) { memcpy(buf, dev_eui, EUI_SIZE); }
void os_getDevKey(u1_t* buf) { memcpy(buf, app_key, APPKEY_SIZE); }


const lmic_pinmap lmic_pins = {
    .nss = NSS_GPIO,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RESET_GPIO,
    .dio = {DIO0_GPIO, DIO1_GPIO, DIO2_GPIO},
};

void format_eui(char *string, uint8_t *eui) {
  sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
		  eui[7], eui[6], eui[5], eui[4], eui[3], eui[2], eui[1], eui[0]);
}

void initLoRA() {
  char euiStr[40];

  // Fetch WiFi MAC address to use as device EUI
  WiFi.macAddress((uint8_t*)&dev_eui);
  std::reverse(dev_eui, dev_eui + EUI_SIZE);

  format_eui(euiStr, dev_eui);
  Serial.print("Device EUI: ");
  Serial.println(euiStr);

  format_eui(euiStr, app_eui);
  Serial.print("App EUI: ");
  Serial.println(euiStr);

  Serial.println("Initialising LoRA module...");
  SPI.begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);

  os_init();
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
  LMIC_setAdrMode(0); // No ADR recommended for mobile devices
  LMIC_startJoining();
}

void loopLoRA() {
    os_runloop_once();
}

void sendLoRA(u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed) {
    LMIC_setTxData2(1, data, dlen, confirmed);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
	    sprintf(loraStatus, "Scan timeout");
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
	    sprintf(loraStatus, "Joining");
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
	    sprintf(loraStatus, "Joined");
            Serial.println(F("EV_JOINED"));
            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            sprintf(loraStatus, "Connected");
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
	    sprintf(loraStatus, "Reset");
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
	    sprintf(loraStatus, "Link dead");
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
	    sprintf(loraStatus, "Link alive");
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}
