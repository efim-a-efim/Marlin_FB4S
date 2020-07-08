#include "mks_wifi.h"

#include "../../lcd/ultralcd.h"
#include "mks_wifi_sd.h"
#if ENABLED(MKS_SDIO_TEST_AT_STARTUP)
#include "mks_test_sdio.h"
#endif

uint8_t mks_in_buffer[ESP_PACKET_DATA_MAX_SIZE];
uint8_t mks_out_buffer[ESP_PACKET_DATA_MAX_SIZE];
uint32_t line_index=0;

uint8_t esp_packet[ESP_PACKET_DATA_MAX_SIZE];


void mks_wifi_init(void){

	SERIAL_ECHO_MSG("Init MKS WIFI");	
    DEBUG("Init MKS WIFI");
	
	SET_OUTPUT(MKS_WIFI_IO0);
	WRITE(MKS_WIFI_IO0, HIGH);

	SET_OUTPUT(MKS_WIFI_IO4);
	WRITE(MKS_WIFI_IO4, HIGH);

	SET_OUTPUT(MKS_WIFI_IO_RST);
	WRITE(MKS_WIFI_IO_RST, LOW);
	ui.set_status((const char *)"WIFI: waiting... ",false);

	safe_delay(1000);	
	WRITE(MKS_WIFI_IO_RST, HIGH);
	safe_delay(1000);	
	WRITE(MKS_WIFI_IO4, LOW);

	#if ENABLED(MKS_SDIO_TEST_AT_STARTUP)
	mks_test_sdio();
	#endif
	
	#ifdef LIST_FILES_AT_STARTUP
	mks_wifi_sd_deinit(); 
	mks_wifi_sd_init();  
	mks_wifi_sd_ls();
	mks_wifi_sd_deinit(); 
	#endif

}

#if ENABLED(MKS_WIFI_CONFIGURE)
void mks_wifi_set_param(void){
	uint32_t packet_size;
	ESP_PROTOC_FRAME esp_frame;


	uint32_t ap_len = strlen((const char *)MKS_WIFI_SSID);
	uint32_t key_len = strlen((const char *)MKS_WIFI_KEY);


	memset(mks_out_buffer, 0, sizeof(ESP_PACKET_DATA_MAX_SIZE));

	mks_out_buffer[0] = WIFI_MODE_STA;

	mks_out_buffer[1] = ap_len;
	strncpy((char *)&mks_out_buffer[2], (const char *)MKS_WIFI_SSID, ap_len);

	mks_out_buffer[2+ap_len] = key_len;
	strncpy((char *)&mks_out_buffer[2 + ap_len + 1], (const char *)MKS_WIFI_KEY, key_len);

	esp_frame.type=ESP_TYPE_NET;
	esp_frame.dataLen= 2 + ap_len + key_len + 1;
	esp_frame.data=mks_out_buffer;
	packet_size=mks_wifi_build_packet(esp_packet,&esp_frame);

	//выпихнуть в uart
	mks_wifi_send(esp_packet, packet_size);
}
#endif

/*
Получает данные из всех функций, как только
есть перевод строки 0x0A, формирует пакет для
ESP и отправляет
*/
void mks_wifi_out_add(uint8_t *data, uint32_t size){
	uint32_t packet_size;
	ESP_PROTOC_FRAME esp_frame;

	while (size--){
		if(*data == 0x0a){
			//Переводы строки внутри формирования пакета
			//Перевод строки => сформировать пакет, отправить, сбросить индекс
			esp_frame.type=ESP_TYPE_FILE_FIRST; //Название типа из прошивки MKS. Смысла не имееет.
			esp_frame.dataLen=strnlen((char *)mks_out_buffer,ESP_PACKET_DATA_MAX_SIZE);
			esp_frame.data=mks_out_buffer;
			packet_size=mks_wifi_build_packet(esp_packet,&esp_frame);

			//выпихнуть в uart
			mks_wifi_send(esp_packet, packet_size);
			//очистить буфер
			memset(mks_out_buffer,0,ESP_SERIAL_OUT_MAX_SIZE);
			//сбросить индекс
			line_index=0;
		}else{
			//писать в буфер			
			mks_out_buffer[line_index++]=*data++;
		}

		if(line_index >= ESP_SERIAL_OUT_MAX_SIZE){
			ERROR("Max line size");
			line_index=0;
		}
	}
}

uint8_t mks_wifi_input(uint8_t data){
	ESP_PROTOC_FRAME esp_frame;
	#if ENABLED(MKS_WIFI_CONFIGURE)
	static uint8_t get_packet_from_esp=0;
	#endif
	static uint8_t packet_start_flag=0;
	static uint8_t packet_type=0;
	static uint16_t packet_index=0;
	static uint16_t payload_size=ESP_PACKET_DATA_MAX_SIZE;
	uint8_t ret_val=1;

	if(data == ESP_PROTOC_HEAD){
		payload_size = ESP_PACKET_DATA_MAX_SIZE;
		packet_start_flag=1;
		packet_index=0;
		memset(mks_in_buffer,0,ESP_PACKET_DATA_MAX_SIZE);
	}

	if(packet_start_flag){
		mks_in_buffer[packet_index]=data;
	}

	if(packet_index == 1){
		packet_type = mks_in_buffer[1];
	}

	if(packet_index == 3){
		payload_size = uint16_t(mks_in_buffer[3] << 8) | mks_in_buffer[2];
	}

	if( (packet_index >= (payload_size+4)) || (packet_index >= ESP_PACKET_DATA_MAX_SIZE) ){
		esp_frame.type = packet_type;
		esp_frame.dataLen = payload_size;
		esp_frame.data = &mks_in_buffer[4];

		mks_wifi_parse_packet(&esp_frame);

		#if ENABLED(MKS_WIFI_CONFIGURE)
		if(!get_packet_from_esp){
			DEBUG("Fisrt packet from ESP, send config");
		
			mks_wifi_set_param();
			get_packet_from_esp=1;
		}
		#endif
		packet_start_flag=0;
		packet_index=0;
	}

	/* Если в пакете G-Сode, отдаем payload дальше в обработчик марлина */
	if((packet_type == ESP_TYPE_GCODE) && 
	   (packet_index >= 4) && 
	   (packet_index < payload_size+5) 
	  ){
		ret_val=0;
	}

	if(packet_start_flag){
		packet_index++;
	}

	return ret_val;
}

void mks_wifi_parse_packet(ESP_PROTOC_FRAME *packet){
	static uint8_t show_ip_once=0;
	char str[100];
	uint8_t str_len;

	switch(packet->type){
		case ESP_TYPE_NET:
			
			if(packet->data[6] == ESP_NET_WIFI_CONNECTED){
				if(show_ip_once==0){
					show_ip_once=1;
					sprintf(str,"; IP %d.%d.%d.%d",packet->data[0],packet->data[1],packet->data[2],packet->data[3]);
					ui.set_status((const char *)str+2,true);
					SERIAL_ECHO_START();
					SERIAL_ECHOLN((char*)str);	

					//Вывод имени сети
					str_len = packet->data[8]; //Wifi network name len
					memcpy(str,&packet->data[9],str_len); 
					str[str_len]=0;
					SERIAL_ECHO_START();
					SERIAL_ECHO("; WIFI: ");
					SERIAL_ECHOLN((char*)str);
					
				}
				DEBUG("[Net] connected, IP: %d.%d.%d.%d",packet->data[0],packet->data[1],packet->data[2],packet->data[3]);
			}else if(packet->data[6] == ESP_NET_WIFI_EXCEPTION){
				DEBUG("[Net] wifi exeption");
			}else{
				DEBUG("[Net] wifi not config");
			}
			break;
		case ESP_TYPE_GCODE:
			break;
		case ESP_TYPE_FILE_FIRST:
				DEBUG("[FILE_FIRST]");
				//Передача файла останавливает все процессы, 
				//поэтому печать в этот момент не возможна.
				if (!CardReader::isPrinting()){
					mks_wifi_start_file_upload(packet);
				}
			break;
		case ESP_TYPE_FILE_FRAGMENT:
				DEBUG("[FILE_FRAGMENT]");
			break;
		case ESP_TYPE_WIFI_LIST:
			DEBUG("[WIFI_LIST]");
			break;
		default:
			DEBUG("[Unkn]");
		 	break;

	}

}

void mks_wifi_print_var(uint8_t count, ...){
	va_list args;
	uint8_t data;

	va_start(args, count);

    while (count--) {
        data = va_arg(args, unsigned);
		mks_wifi_out_add(&data, 1);
    }
    va_end(args);
}


void mks_wifi_print(const char *s){
	mks_wifi_out_add((uint8_t *)s, strnlen((char *)s,ESP_PACKET_DATA_MAX_SIZE));
}

void mks_wifi_print(int i){
	char str[14];

	sprintf(str,"%d",i);
	mks_wifi_out_add((uint8_t *)str, strnlen((char *)str,ESP_PACKET_DATA_MAX_SIZE));
}


void mks_wifi_println(const char *s){
	mks_wifi_out_add((uint8_t *)s, strnlen((char *)s,ESP_PACKET_DATA_MAX_SIZE));
}

void mks_wifi_println(float f){
	char str[30];

	sprintf(str,"%ld\n",(uint32_t)f);
	mks_wifi_out_add((uint8_t *)str, strnlen((char *)str,ESP_PACKET_DATA_MAX_SIZE));
}

uint16_t mks_wifi_build_packet(uint8_t *packet, ESP_PROTOC_FRAME *esp_frame){
	uint16_t packet_size;

	memset(packet,0,ESP_PACKET_DATA_MAX_SIZE);
	packet[0] = ESP_PROTOC_HEAD;
	packet[1] = esp_frame->type;

	for(uint32_t i=0; i < esp_frame->dataLen; i++){
		packet[i+4]=esp_frame->data[i]; //4 байта заголовка отступить
	}

	packet_size = esp_frame->dataLen + 4;

	if(esp_frame->type != ESP_TYPE_NET){
		packet[packet_size++] = 0x0d;
		packet[packet_size++] = 0x0a; 
		esp_frame->dataLen = esp_frame->dataLen + 2; //Два байта на 0x0d 0x0a
	}
	
	*((uint16_t *)&packet[2]) = esp_frame->dataLen;

	packet[packet_size] = ESP_PROTOC_TAIL;
	return packet_size;
}


void mks_wifi_send(uint8_t *packet, uint16_t size){
	safe_delay(10);				
	for( uint32_t i=0; i < (uint32_t)(size+1); i++){
		while(MYSERIAL1.availableForWrite()==0){
			safe_delay(10);				
		}
		MYSERIAL1.write(packet[i]);
	}
}

