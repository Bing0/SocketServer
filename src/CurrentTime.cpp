/*
 * CurrentTime.cpp
 *
 *  Created on: Sep 11, 2015
 *      Author: alcht
 */

#include "../head/CurrentTime.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

CurrentTime::CurrentTime() {
	// TODO Auto-generated constructor stub

}

CurrentTime::~CurrentTime() {
	// TODO Auto-generated destructor stub
}

void itoa(int i, char*string) {
//	int power, j;
//	j = i;
//	for (power = 1; j >= 10; j /= 10)
//		power *= 10;
//	for (; power > 0; power /= 10) {
//		*string++ = '0' + i / power;
//		i %= power;
//	}
//	*string = '\0';
	sprintf(string,"%02d",i);
}

void CurrentTime::getCurrentTime(char *time_str) {
	time_t timep;
	struct tm *p_curtime;
	char *time_tmp;
	time_tmp = (char*) malloc(2);
	memset(time_tmp, 0, 2);
	memset(time_str, 0, 20);
	time(&timep);
	p_curtime = localtime(&timep);
	strcat(time_str, "\n(");
	itoa(p_curtime->tm_year+1900, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, "/");
	itoa(p_curtime->tm_mon, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, "/");
	itoa(p_curtime->tm_mday, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, "-");
	itoa(p_curtime->tm_hour, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, ":");
	itoa(p_curtime->tm_min, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, ":");
	itoa(p_curtime->tm_sec, time_tmp);
	strcat(time_str, time_tmp);
	strcat(time_str, ")  ");
	free(time_tmp);
}
