#ifdef _WIN32
#include "stdafx.h"
#endif
#include "AutoLock.h"

//*********************************************************************************
Autolock::Autolock
/**********************************************************************************
 * Constructor, Enter the critical section
 */
(
 std::mutex *cs           // The mutex to lock (unlock).
)
//*********************************************************************************
{
	m_cs = cs; // Remember the Critical section, needed for the destructor.
	m_cs->lock();
}

//*********************************************************************************
Autolock::~Autolock( void )
/**********************************************************************************
 * Destructor, Leave the Critical Section when the the instance goes out of scope.
 */
{
	m_cs->unlock();
}