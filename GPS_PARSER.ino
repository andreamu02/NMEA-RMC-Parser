#include "GPS.h"

#define MAX_MESSAGE_LENGTH 80

GPS gps;

void setup(){
    Serial.begin(9600);
    Serial1.begin(9600);
}

void loop(){
    char msg[MAX_MESSAGE_LENGTH];
    if(Serial1.available()){
        int byteNum = Serial1.readBytesUntil('\n', msg, MAX_MESSAGE_LENGTH);
        msg[byteNum] = '\0';
        if(parse_NMEA_buffer(msg, &gps)){
            Serial.print(String((int)gps.time.day));
            Serial.print(" - " + String((int)gps.time.month));
            Serial.println(" - " + String(gps.time.year));

            Serial.print((int)gps.time.hour);
            Serial.print(":" + String((int)gps.time.minute));
            Serial.println(":" + String(gps.time.second));

            Serial.println(gps.velocity);
            Serial.println(gps.cog);


            Serial.print(gps.latitude.grad);
            Serial.print(" " + String(gps.latitude.minutes));
            Serial.println(" " + String(gps.latitude.direction));

            Serial.print(gps.longitude.grad);
            Serial.print(" " + String(gps.longitude.minutes));
            Serial.println(" " + String(gps.longitude.direction));

            Serial.println("\n");
        }
    }
}
