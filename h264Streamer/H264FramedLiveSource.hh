/*
 * Filename: H264FramedLiveSource.hh
 * Auther: chenbin
 * Create date: 2013/ 1/22
 */

#ifndef _H264FRAMEDLIVESOURCE_HH
#define _H264FRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

class grabber;
class H264FramedLiveSource : public FramedSource
{
public:
	typedef boost::mutex::scoped_lock LockType;
public:
	static H264FramedLiveSource* createNew(UsageEnvironment& env,
		char const* fileName,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0); 

protected:
	H264FramedLiveSource(UsageEnvironment& env,
		char const* fileName, 
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);
	// called only by createNew()
	~H264FramedLiveSource();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();
public:
	void push_data(unsigned char *data, int size);
	//int TransportData( unsigned char* to, unsigned maxSize );

protected:
    //FILE *fp;
	grabber* pGrabber;
	boost::mutex mtx_;
	boost::condition cond_;
	std::vector<unsigned char >  message;
	bool b_stop;
	int  index;
};

#endif