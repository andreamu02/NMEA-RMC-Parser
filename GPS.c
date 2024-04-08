#include "GPS.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_NEXT_COMMA_LENGTH 16

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

typedef enum
{
    INVALID = -1,
    START,      // $
    TYPE,       // type of NMEA message
    UTC,        // hh, min, sec, msec
    GPS_STATUS, // A (valid), V (warning)
    LATITUDE,   // ddmm.mm
    NS,
    LONGITUDE,
    EW,
    SPEED, // knots
    COURSE_OVER_GROUND,
    DATE, // ddmmyy
    MAGNETIC_VARIATION,
    CHECKSUM,
} EXIT_CODE;

EXIT_CODE exit_code;

char internal_buffer[16];

void parse_UTC_date(GPS *gps);
void parse_coordinate(coordinate_gps *coordinate, char *g, char *m);
void parse_longitude(GPS *gps);
void parse_latitude(GPS *gps);
void parse_velocity(GPS *gps);
void parse_UTC_time(GPS *gps);
char* parse_field(char *buffer, GPS *gps);
char* read_str(char *buffer);

char parse_NMEA_buffer(char *buffer, GPS *gps)
{
    if (buffer[0] != '$')
        return false;

    buffer++;
    exit_code = START;

    while (exit_code != INVALID && exit_code != CHECKSUM)
    {
        buffer = parse_field(buffer, gps);
    }

    
    if (exit_code == INVALID)
        return false;

    return true;
}

char* read_str(char *buffer)
{

    memset(internal_buffer, '\0', sizeof(internal_buffer));

    int i = 0;
    while (buffer[i] != ',' && buffer[i] != '\r' && buffer[i] != '\n')
    {
        internal_buffer[i] = buffer[i];
        i++;
        if (i >= MAX_NEXT_COMMA_LENGTH)
        {
            exit_code = INVALID;
            return buffer;
        }
    }


    exit_code = (EXIT_CODE)(exit_code + 1);

    if ((buffer[i] == '\r' || buffer[i] == '\n') && exit_code < CHECKSUM)
    {
        exit_code = INVALID;
        return buffer;
    }


    if (buffer[i] == ',')
        buffer = buffer + (i + 1);
    
    return buffer;
}

char* parse_field(char *buffer, GPS *gps)
{

    buffer = read_str(buffer);

    switch (exit_code)
    {

    case TYPE:
        if (strcmp(internal_buffer, "GNRMC"))
            exit_code = INVALID;
        break;

    case UTC: // hhmmss.ss
        parse_UTC_time(gps);
        break;

    case GPS_STATUS: // A (valid), V (warning)
        if (strcmp(internal_buffer, "A"))
            exit_code = INVALID;
        break;
    case LATITUDE:
        parse_latitude(gps);
        break; // ddmm.mm
    case NS:
        if (strcmp(internal_buffer, "N") && strcmp(internal_buffer, "S"))
            exit_code = INVALID;
        else
            gps->latitude.direction = internal_buffer[0];

        break;

    case LONGITUDE:
        parse_longitude(gps);
        break;

    case EW:
        if (strcmp(internal_buffer, "E") && strcmp(internal_buffer, "W"))
            exit_code = INVALID;
        else
            gps->longitude.direction = internal_buffer[0];

        break;

    case SPEED: // knots
        parse_velocity(gps);
        break;
    
    case COURSE_OVER_GROUND: //

        break;
    case DATE: // ddmmyy
        parse_UTC_date(gps);
        break;

    
    case CHECKSUM:

        break;
    default:
        break;
    }

    return buffer;
}

void parse_UTC_time(GPS *gps)
{

    if (strlen(internal_buffer) != 9)
    {
        exit_code = INVALID;
        return;
    }

    char hh[3] = {internal_buffer[0], internal_buffer[1], '\0'};
    char mm[3] = {internal_buffer[2], internal_buffer[3], '\0'};
    char ss[6] = {internal_buffer[4], internal_buffer[5], internal_buffer[6], internal_buffer[7], internal_buffer[8], '\0'};

    int hour = 0;
    int minute = 0;
    float second = 0.0f;

    if (sscanf(hh, "%d", &hour) == EOF || sscanf(mm, "%d", &minute) == EOF || sscanf(ss, "%f", &second) == EOF)
    {
        exit_code = INVALID;
        return;
    }

    gps->time.hour = hour;
    gps->time.minute = minute;
    gps->time.second = second;
}

void parse_velocity(GPS *gps)
{
    float knots_velocity = 0.0f;

    if (sscanf(internal_buffer, "%f", &knots_velocity) == EOF)
        exit_code = INVALID;
    else
        gps->velocity = knots_velocity * 1.852f;
}

void parse_cog(GPS *gps)
{
    float degree = 0.0f;

    if (sscanf(internal_buffer, "%f", &degree) == EOF)
        exit_code = INVALID;
    else
        gps->cog = degree;
}

void parse_latitude(GPS *gps)
{
    if (strlen(internal_buffer) != 10)
    {
        exit_code = INVALID;
        return;
    }
    char g[3] = {internal_buffer[0], internal_buffer[1], '\0'};
    char m[10] = {internal_buffer[2], internal_buffer[3], internal_buffer[4], internal_buffer[5], internal_buffer[6], internal_buffer[7], internal_buffer[8], internal_buffer[9], '\0'};
    parse_coordinate(&gps->latitude, g, m);
}

void parse_longitude(GPS *gps)
{
    if (strlen(internal_buffer) != 11)
    {
        exit_code = INVALID;
        return;
    }

    char g[4] = {internal_buffer[0], internal_buffer[1], internal_buffer[2], '\0'};
    char m[11] = {internal_buffer[3], internal_buffer[4], internal_buffer[5], internal_buffer[6], internal_buffer[7], internal_buffer[8], internal_buffer[9], internal_buffer[10], '\0'};
    parse_coordinate(&gps->longitude, g, m);
}

void parse_coordinate(coordinate_gps *coordinate, char *g, char *m)
{
    int grad = 0;
    float minutes = 0;

    if (sscanf(g, "%d", &grad) == EOF || sscanf(m, "%f", &minutes) == EOF)
    {
        exit_code = INVALID;
        return;
    }

    coordinate->grad = grad;
    coordinate->minutes = minutes;
}

void parse_UTC_date(GPS *gps)
{

    if (strlen(internal_buffer) != 6)
    {
        exit_code = INVALID;
        return;
    }

    char d[3] = {internal_buffer[0], internal_buffer[1], '\0'};
    char m[3] = {internal_buffer[2], internal_buffer[3], '\0'};
    char y[6] = {'2', '0', internal_buffer[4], internal_buffer[5], '\0'};

    int day = 0;
    int month = 0;
    int year = 0;

    if (sscanf(d, "%d", &day) == EOF || sscanf(m, "%d", &month) == EOF || sscanf(y, "%d", &year) == EOF)
    {
        exit_code = INVALID;
        return;
    }

    gps->time.day = day;
    gps->time.month = month;
    gps->time.year = year;
}