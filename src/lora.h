#pragma once
#include <Arduino.h>
#include "lmic.h"

// LoRA module pins
#define SCK_GPIO        5
#define MISO_GPIO       19
#define MOSI_GPIO       27
#define NSS_GPIO        18
#define RESET_GPIO      23
#define DIO0_GPIO       26
#define DIO1_GPIO       33
#define DIO2_GPIO       32

#define EUI_SIZE 8
#define APPKEY_SIZE 16

void initLoRA();
void loopLoRA();
void sendLoRA(u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed);

void format_eui(char *string, uint8_t *eui);

extern char loraStatus[20];
