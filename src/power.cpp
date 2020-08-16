#include "power.h"

/**
 * Wrapper to control the AX192 power management IC.
 *
 * Power buses on the TTGO T-Beam:
 *
 * DCDC1: Peripheral bus (VCC_2.5V - labeled 3v3 on board)
 * DCDC2: Not connected
 * DCDC3: 3v3 main bus for ESP32 (VDD3V3)
 *
 * Note that disabling DCDC3 bricks the board!
 *
 * LDO1: GPS RTC battery charge (3v3, not software controllable?)
 * LDO2: LoRA module
 * LDO3: GPS module
 *
 * EXTEN: Seems to just be an external GPIO.
 */


bool Power::init() {
	Wire.begin(PMIC_SDA, PMIC_SCL);
	if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
		Serial.println("PMIC initialised");
	} else {
		Serial.println("PMIC initialisation FAILED");
		return false;
	}

	// Explicitly set the 3v3 bus to on. It should be on anyway,
	// or this code wouldn't be running, but this allows you to
	// un-brick the board by externally injecting 3v3 into the
	// pole of the capacitor nearest the "DC3" label.
	axp.setPowerOutPut(AXP192_DCDC3, AXP202_ON);

	// DCDC2 is not connected, set it to off.
	axp.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);

	// EXTEN doesn't do anything, set it to off.
	axp.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);

	// Enable ADCs
	axp.adc1Enable(AXP202_BATT_VOL_ADC1, true);
  	axp.adc1Enable(AXP202_BATT_CUR_ADC1, true);
  	axp.adc1Enable(AXP202_VBUS_VOL_ADC1, true);
  	axp.adc1Enable(AXP202_VBUS_CUR_ADC1, true);

	// In its powered-off state, the OLED display affects the I2C bus.
	// This prevents the PMIC from being initialised until power is
	// cycled. https://github.com/espressif/arduino-esp32/issues/3363
	setPeripheralBus(true);
	setLoRA(false);
	setGPS(true);

	// Set bus voltages
	axp.setDCDC1Voltage(3300);
	axp.setLDO2Voltage(3300);
	axp.setLDO3Voltage(3000);
	
	return true;
}

void Power::setPeripheralBus(bool state) {
	axp.setPowerOutPut(AXP192_DCDC1, state ? AXP202_ON : AXP202_OFF);
}

void Power::setLoRA(bool state) {
	axp.setPowerOutPut(AXP192_LDO2, state ? AXP202_ON : AXP202_OFF);
}

void Power::setGPS(bool state) {
	axp.setPowerOutPut(AXP192_LDO3, state ? AXP202_ON : AXP202_OFF);
}

PowerState Power::getState() {
    PowerState state = {
	(double)axp.getDCDC1Voltage() / 1000,
	(double)axp.getDCDC2Voltage() / 1000,
	(double)axp.getDCDC3Voltage() / 1000,
	(double)axp.getLDO2Voltage() / 1000,
	(double)axp.getLDO3Voltage() / 1000,
	axp.getVbusVoltage() / 1000,
	axp.getVbusCurrent() / 1000,
	axp.getBattVoltage() / 1000
    };
    return state;
}

void Power::logStatus() {
  static char buf[50];
  PowerState state = getState();
  sprintf(buf, "DCDC 1: %.2f V, 2: %.2f V, 3: %.2f V", state.dcdc1_v, state.dcdc2_v, state.dcdc3_v);
  Serial.println(buf);
  sprintf(buf, "LDO 1: 3.3 V (fixed), 2: %.2f V, 3: %.2f V", state.ldo2_v, state.ldo3_v);
  Serial.println(buf);
  sprintf(buf, "Vbus: %.2f V, %.2f A", state.bus_v, state.bus_a);
  Serial.println(buf);
  sprintf(buf, "Battery: %.2f V", state.batt_v);
  Serial.println(buf);
}

