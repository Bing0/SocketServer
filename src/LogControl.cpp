/*
 * FileControl.cpp
 *
 *  Created on: Sep 23, 2015
 *      Author: alcht
 */

#include "../head/LogControl.h"
#include <stdarg.h>
#include <stdio.h>

LogControl::LogControl() {
	fp = NULL;
	mutexLog = PTHREAD_MUTEX_INITIALIZER;
}

LogControl::~LogControl() {
	if (fp) {
		fclose(fp);
	}
}

int LogControl::CreateLogFile(const char *filename) {
	int res = 0;
	if (fp) {
		res = -1;
	} else if ((fp = fopen(filename, "a+")) == NULL) {
		printf("���������������������\n");
		return -1;
	}
	return res;
}

int LogControl::SaveToLog(const int level, const char * format, ...) {
	va_list ap;
	int n = 0;
	if (fp == NULL) {
		return -1;
	}

	pthread_mutex_lock(&mutexLog);
	if (level >= LogLevelPrint) {
		va_start(ap, format);
		n = vprintf(format, ap);

		if (level >= LogLevelSave) {
			va_start(ap, format);
			n = vfprintf(fp, format, ap);
		}
	}
	pthread_mutex_unlock(&mutexLog);
	va_end(ap);

	return n;
}

int LogControl::Flush(void) {
	if (fp == NULL) {
		return -1;
	}
	fflush(fp);
	return 0;
}

void LogControl::CloseFile(void) {
	if (fp == NULL) {
		return;
	}
	fclose(fp);
}
