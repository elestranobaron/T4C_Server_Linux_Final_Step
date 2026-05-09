#ifndef __AUTOLOCK_H
#define __AUTOLOCH_H

#include <mutex>

class Autolock {
	public:
		Autolock(std::mutex *);
		~Autolock(void);
	private:
		std::mutex *m_cs;
};

#endif