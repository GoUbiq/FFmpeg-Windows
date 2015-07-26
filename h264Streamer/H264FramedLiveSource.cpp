/*
 * Filename:  H264FramedLiveSource.cpp
 * Auther:  mlj
 * Create date: 2013/ 1/22
 */

#include "H264FramedLiveSource.hh"
#include "singleton_grabber.h"


static void global_push_data(void* user_data, unsigned char* data, int size)
{
	if(!user_data)
		return;
	H264FramedLiveSource* pThis = (H264FramedLiveSource*)user_data;
	pThis->push_data(data, size);
}

H264FramedLiveSource::H264FramedLiveSource( UsageEnvironment& env,  
	char const* fileName, 
	unsigned preferredFrameSize, 
	unsigned playTimePerFrame )
	: FramedSource(env)
{
     //fp = fopen( fileName, "rb" ); 
	index = singleton_grabber::get_singleton_grabber()->add_member(this, global_push_data);
	b_stop = false;
}

H264FramedLiveSource* H264FramedLiveSource::createNew( UsageEnvironment& env,
	                                       char const* fileName, 
	                                       unsigned preferredFrameSize /*= 0*/, 
	                                       unsigned playTimePerFrame /*= 0*/ )
{ 
	H264FramedLiveSource* newSource = new H264FramedLiveSource(env, fileName, preferredFrameSize, playTimePerFrame);
 
	return newSource;
}

H264FramedLiveSource::~H264FramedLiveSource()
{
	//fclose(fp);
	b_stop = true;
	singleton_grabber::get_singleton_grabber()->del_member(index);
	
}


long filesize(FILE *stream)
{
	//long curpos, length;
	//curpos = ftell(stream);
	//fseek(stream, 0L, SEEK_END);
	//length = ftell(stream);
	//fseek(stream, curpos, SEEK_SET);
	return 0;
}

void H264FramedLiveSource::doGetNextFrame()
{

	//if( filesize(fp) >  fMaxSize)
	//  fFrameSize = fread(fTo,1,fMaxSize,fp); 
	//else
	//{
	//	fFrameSize = fread(fTo,1,filesize(fp),fp);
	//	fseek(fp, 0, SEEK_SET);
	//}
	//fFrameSize = fMaxSize;
	std::vector<unsigned char > pakect;
	{
		LockType lock(mtx_);
		while(message.size()<=fMaxSize && !b_stop)
			cond_.wait(lock);
		if (b_stop)
		{
			return;
		}
		pakect.insert(pakect.end(), &message[0], &message[fMaxSize]);
		message.erase(message.begin(), message.begin()+fMaxSize);
		
	}
	fFrameSize = fMaxSize;
	memcpy(fTo, &pakect[0], fFrameSize);
	nextTask() = envir().taskScheduler().scheduleDelayedTask( 0,
		(TaskFunc*)FramedSource::afterGetting, this);//表示延迟0秒后再执行 afterGetting 函数
	return;
}

void H264FramedLiveSource::push_data( unsigned char  *data, int size )
{
	LockType lock(mtx_);
	message.insert(message.end(), data, data+size);
	cond_.notify_all();
}

