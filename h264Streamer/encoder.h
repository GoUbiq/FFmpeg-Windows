#ifndef __ENCODER_H__
#define __ENCODER_H__
#include "boost/thread.hpp"
#include "block_fifo_t.h"
#include <string>
#include "callback_func.h"

typedef struct AVCodec AVCodec;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVFrame AVFrame;
class encoder
{
public:
	encoder(int width, int height, void* user_data, func push_data);
	~encoder();
public:
	int init();

public:
	void startLoop();
	void stopLoop();
public:
	AVFrame	*getFreeFrame();
	void pushValidFrame(AVFrame	*pFrameYUV);
private:
	void loop();
private:
	boost::thread* pThread;
	bool b_stop;

	//H.264全局变量
	AVCodec *m_ffvcodec;
	AVCodecContext *m_ffvpCodecCtx;
	int m_width;
	int m_height;

	block_fifo_t<AVFrame> free_fifo;
	block_fifo_t<AVFrame> valid_fifo;

	void* m_user_data;
	func m_push_data;
};
#endif