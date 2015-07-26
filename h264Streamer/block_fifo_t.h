#ifndef __BLOCK_FIFO_H__
#define __BLOCK_FIFO_H__
#include <deque>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition.hpp"
typedef struct AVFrame AVFrame;
template<typename T>
class block_fifo_t
{
public:
	typedef boost::mutex::scoped_lock LockType;
	block_fifo_t():b_wakeup(false)
	{

	}
	void push( T* frame )
	{
		LockType lock(mtx_);
		fifo.push_front(frame);
		cond_.notify_all();

	}

	T* pop()
	{
		T* ret = NULL;
		LockType lock(mtx_);
		while(fifo.size()==0 && !b_wakeup)
			cond_.wait(lock);
		if (b_wakeup)
		{
			b_wakeup = false;
			return ret;
		}
		ret = fifo.front();
		fifo.pop_front();
		return ret;
	}

	int size()
	{
		return fifo.size();
	}

	void wakeup()
	{
		b_wakeup = true;
		cond_.notify_all();
	}
private:
	std::deque<T*>  fifo;
	boost::mutex mtx_;
	boost::condition cond_;
	bool b_wakeup;
};
#endif