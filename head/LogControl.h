/*
 * FileControl.h
 *
 *  Created on: Sep 23, 2015
 *      Author: alcht
 */

#ifndef LOGCONTROL_H_
#define LOGCONTROL_H_
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define LogLevelPrint	5
#define LogLevelSave	10

class LogControl {
private:
	FILE *fp;
public:
	LogControl();
	virtual ~LogControl();
	int CreateLogFile(const char *filename);
	int SaveToLog(const int level, const char * format, ...);
	int Flush(void);
	void CloseFile(void);
private:
	pthread_mutex_t mutexLog;
};

#endif /* LOGCONTROL_H_ */
