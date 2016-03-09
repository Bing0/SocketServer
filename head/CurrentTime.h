/*
 * CurrentTime.h
 *
 *  Created on: Sep 11, 2015
 *      Author: alcht
 */

#ifndef CURRENTTIME_H_
#define CURRENTTIME_H_

class CurrentTime {
public:
	CurrentTime();
	virtual ~CurrentTime();
	void getCurrentTime(char *time_str);
};

#endif /* CURRENTTIME_H_ */
