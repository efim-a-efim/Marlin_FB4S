#include "debug_to_uart.h"


void debug_to_uart(char *fmt,...){

    char buffer[200];
    va_list ParamList;
    char *ptr = (char *)buffer;

    buffer[0] = ';';
    buffer[1] = ' ';
    
    va_start(ParamList, fmt);
    vsnprintf (buffer+2, 199, fmt, ParamList);
    va_end(ParamList);

    //SERIAL_ECHOLN((char *)&buffer);	

    while(*ptr){
      while(MYSERIAL0.availableForWrite()==0){
        safe_delay(10);				
      }
		MYSERIAL0.write(*ptr++);
    }
   


}
