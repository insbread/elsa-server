/***
 * @Author: insbread
 * @Date: 2022-07-15 20:12:00
 * @LastEditTime: 2022-07-15 20:12:00
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/elsac_x_lock.cpp
 * @版权声明
 */
#include "elsac_x_lock.h"

#define InitMutex(x) pthread_mutex_init(x, NULL)
#define DestroyMutex(x) pthread_mutex_destroy(x)
#define MutexLock(x) pthread_mutex_lock(x)
#define MutexUnLock(x) pthread_mutex_unlock(x)

using namespace ElsaNLock;

ElsaCMutex::ElsaCMutex()
{
	InitMutex(&m_lock);
}

ElsaCMutex::~ElsaCMutex()
{
	DestroyMutex(&m_lock);
}

void ElsaCMutex::Lock()
{
	MutexLock(&m_lock);
}


void ElsaCMutex::Unlock()
{
	MutexUnLock(&m_lock);
}
