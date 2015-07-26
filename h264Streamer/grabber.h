#ifndef __GRABBER_H__
#define __GRABBER_H__
#include "boost/thread.hpp"
#include "callback_func.h"
#include "block_fifo_t.h"
typedef struct AVFrame AVFrame;
typedef struct AVPacket AVPacket;
typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct SwsContext SwsContext;
class encoder;

class grabber
{
public:
	grabber(void* user_data, func push_data);
	~grabber();
public:
	int init();
	
public:
	void startLoop();
	void stopLoop();
private:
	void loop();
	void decloop();
	void procloop();
private:
	boost::thread* pThread;
	boost::thread* decThread;
	boost::thread* procThread;

	block_fifo_t<AVPacket > decFifo;
	block_fifo_t<AVFrame> free_fifo;
	block_fifo_t<AVFrame> valid_fifo;

	bool b_stop;
	encoder* pEnc;
	

	AVFormatContext	*pFormatCtx;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
    SwsContext *img_convert_ctx;
	int videoindex;

	void* m_user_data;
	func m_push_data;
};
#endif