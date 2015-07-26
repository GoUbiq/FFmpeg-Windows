
#include "singleton_grabber.h"
#include "grabber.h"

static void global_singleton_grabber_push_data(void* user_data, unsigned char* data, int size);


reciever::reciever():
usr_data(NULL), recieve(NULL)
{
}
reciever::reciever(void* usr_data,void (*recieve)(void* user_data, unsigned char* data, int size)):
usr_data(usr_data), recieve(recieve)
{

}



singleton_grabber* singleton_grabber::get_singleton_grabber()
{
	static singleton_grabber* p = NULL;
	if (!p)
	{
		p = new singleton_grabber;
	}
	return p;
}

singleton_grabber::singleton_grabber()
{
	index = 0;
	pGrabeer = NULL;
}
singleton_grabber::~singleton_grabber()
{
	delete pGrabeer;
}
void singleton_grabber::push_data(unsigned char* data, int size)
{
	LockType lock(mtx_);
	map<int, reciever>::iterator it = recievers.begin();
	while(it!=recievers.end())
	{
		reciever& r = it->second;
		if (r.recieve)
		{
			r.recieve(r.usr_data, data, size);
		}
		it++;
	}
}

int singleton_grabber::add_member(void* usr_data, void (*recieve)(void* user_data, unsigned char* data, int size))
{
	LockType lock(mtx_);
	index++;
	recievers[index] = reciever(usr_data, recieve);
	if (!pGrabeer)
	{
		pGrabeer = new grabber(this, global_singleton_grabber_push_data);
		pGrabeer->init();
		pGrabeer->startLoop();
	}
	return index;

}
void singleton_grabber::del_member(int idx)
{
	LockType lock(mtx_);
	map<int, reciever>::iterator it = recievers.find(idx);
	if (it != recievers.end())
	{
		recievers.erase(it);
	}
	if (0 == recievers.size())
	{
		pGrabeer->stopLoop();
		delete pGrabeer;
		pGrabeer = NULL;
	}
}


static void global_singleton_grabber_push_data(void* user_data, unsigned char* data, int size)
{
	if(!user_data)
		return;
	singleton_grabber* pThis = (singleton_grabber*)user_data;
	pThis->push_data(data, size);
}
