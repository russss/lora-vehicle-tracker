#pragma once
#include <axp20x.h>

#define PMIC_SDA 21
#define PMIC_SCL 22

struct PowerState {
    double dcdc1_v;
    double dcdc2_v;
    double dcdc3_v;
    double ldo2_v;
    double ldo3_v;
    double bus_v;
    double bus_a;
    double batt_v;
};

class Power {
   public:
        bool init();
	void setPeripheralBus(bool state);
	void setLoRA(bool state);
	void setGPS(bool state);
	void logStatus();
	PowerState getState();
   protected:
	AXP20X_Class axp;
};
