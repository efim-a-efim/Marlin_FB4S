#ifndef MKS_WIFI_H
#define MKS_WIFI_H

#include "../../MarlinCore.h"
#include "../../inc/MarlinConfig.h"
#include "../../libs/Segger/log.h"
#include "mks_wifi_settings.h"

typedef struct
{
	uint8_t type; 
	uint16_t dataLen;
	uint8_t *data; 
} ESP_PROTOC_FRAME;

void mks_wifi_init(void);

void mks_wifi_set_param(void);

uint8_t mks_wifi_input(uint8_t data);
void mks_wifi_parse_packet(ESP_PROTOC_FRAME *packet);

void mks_wifi_out_add(uint8_t *data, uint32_t size);

uint16_t mks_wifi_build_packet(uint8_t *packet, ESP_PROTOC_FRAME *esp_frame);

void mks_wifi_send(uint8_t *packet, uint16_t size);

void mks_wifi_print_var(uint8_t count, ...);

void mks_wifi_print(const char *s);
void mks_wifi_print(int i);

void mks_wifi_println(const char *s);
void mks_wifi_println(float);

#endif