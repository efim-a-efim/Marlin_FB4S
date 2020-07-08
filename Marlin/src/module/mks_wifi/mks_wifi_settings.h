#ifndef MKS_WIFI_SETTINGS_H
#define MKS_WIFI_SETTINGS_H

#define MKS_WIFI_SERIAL_NUM        (uint8_t)1

#define WIFI_MODE_STA				(uint8_t)2
#define WIFI_MODE_AP				(uint8_t)1

#define LIST_FILES_AT_STARTUP		(uint8_t)1

#define ESP_PROTOC_HEAD				(uint8_t)0xa5
#define ESP_PROTOC_TAIL				(uint8_t)0xfc

#define ESP_TYPE_NET				(uint8_t)0x0
#define ESP_TYPE_GCODE				(uint8_t)0x1
#define ESP_TYPE_FILE_FIRST			(uint8_t)0x2
#define ESP_TYPE_FILE_FRAGMENT		(uint8_t)0x3
#define ESP_TYPE_WIFI_LIST		    (uint8_t)0x4

#define ESP_PACKET_DATA_MAX_SIZE	1024
#define ESP_SERIAL_OUT_MAX_SIZE		1024

#define ESP_NET_WIFI_CONNECTED		(uint8_t)0x0A
#define ESP_NET_WIFI_EXCEPTION		(uint8_t)0x0E

#define NOP	__asm volatile ("nop")

#endif
