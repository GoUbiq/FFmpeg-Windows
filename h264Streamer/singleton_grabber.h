#ifndef __SINGLETON_GRABBER_H__
#define __SINGLETON_GRABBER_H__
#include <map>
#include "boost/thread/mutex.hpp"
using namespace std;
class grabber;

struct reciever
{
	reciever();
	reciever(void* usr_data,void (*recieve)(void* user_data, unsigned char* data, int size));
	void* usr_data;
	void (*recieve)(void* user_data, unsigned char* data, int size);
};


struct singleton_grabber
{
	typedef boost::mutex::scoped_lock LockType;
public:
	static singleton_grabber* get_singleton_grabber();
private:
	singleton_grabber();
	~singleton_grabber();
public:
	void push_data(unsigned char* data, int size);

public:
	int add_member(void* usr_data, void (*recieve)(void* user_data, unsigned char* data, int size));
	void del_member(int idx);
private:
	map<int, reciever> recievers;
	grabber* pGrabeer;
	boost::mutex mtx_;
	int    index;
};
#endif