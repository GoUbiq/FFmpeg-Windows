#include "grabber.h"
#include "encoder.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
};



grabber::grabber(void* user_data, func push_data):pThread(NULL), decThread(NULL), procThread(NULL),b_stop(false), pEnc(NULL),
pFormatCtx(NULL),pCodecCtx(NULL), pCodec(NULL),img_convert_ctx(NULL), videoindex(-1),
m_user_data(user_data), m_push_data(push_data)

{

}

grabber::~grabber()
{
	if (pThread)
	{
		delete pThread;
		pThread = NULL;
	}

	if (pEnc)
	{
		delete pEnc;
		pEnc = NULL;
	}

	if (pCodecCtx)
	{
		avcodec_close(pCodecCtx);
		av_free(pCodecCtx);
		pCodecCtx = NULL;
	}



	if (img_convert_ctx)
	{
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = NULL;
	}
	if (pFormatCtx)
	{
		//avformat_close_input(&pFormatCtx);
		pFormatCtx = NULL;
	}
}
#define  USE_DSHOW 0
int grabber::init()
{
	pFormatCtx = avformat_alloc_context();
#if USE_DSHOW  
				//Use dshow  
				//  
				//Need to Install screen-capture-recorder  
				//screen-capture-recorder  
				 //Website: http://sourceforge.net/projects/screencapturer/  
			//AVDictionary* options = NULL;
			//av_dict_set(&options,"rtbufsize","1500M",0);
				//  
			AVInputFormat *ifmt=av_find_input_format("dshow");  
			if(avformat_open_input(&pFormatCtx,"video=screen-capture-recorder",ifmt,NULL)!=0){  
					printf("Couldn't open input stream.\n");  
					return -1;  
				}  
#else  

	//Use gdigrab
	AVDictionary* options = NULL;
	//Set some options
	//grabbing frame rate
	//av_dict_set(&options,"framerate","5",0);
	//The distance from the left edge of the screen or desktop
	//av_dict_set(&options,"offset_x","20",0);
	//The distance from the top edge of the screen or desktop
	//av_dict_set(&options,"offset_y","40",0);
	//Video frame size. The default is to capture the full screen
	//av_dict_set(&options,"video_size","640x480",0);
	AVInputFormat *ifmt=av_find_input_format("gdigrab");
	if(avformat_open_input(&pFormatCtx,"desktop",ifmt,&options)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	
	int i = 0;
	for(; i<pFormatCtx->nb_streams; i++) 
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
	}
	if(videoindex==-1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	{
		for (int i=0;i<10;i++)
		{
			AVFrame* frame = av_frame_alloc();
			frame->format = pCodecCtx->pix_fmt;
			frame->width = pCodecCtx->width;
			frame->height = pCodecCtx->height;
			av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, (AVPixelFormat)frame->format, 32);
			free_fifo.push(frame);
		}
	}

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
									/*pCodecCtx->width, pCodecCtx->height*/1280, 720, PIX_FMT_YUV420P, 
									SWS_BICUBIC, NULL, NULL, NULL); 

	pEnc = new encoder(/*pCodecCtx->width, pCodecCtx->height*/1280, 720, m_user_data, m_push_data);
	if (-1 == pEnc->init())
	{
		printf("Could not open encoder.\n");
		return -1;
	}
	pEnc->startLoop();
	return 0 ;
}

void grabber::startLoop()
{
	procThread = new boost::thread(&grabber::procloop, this);
	decThread = new boost::thread(&grabber::decloop, this);
	pThread = new boost::thread(&grabber::loop, this);
}
void grabber::stopLoop()
{
	b_stop = true;
	pEnc->stopLoop();
	procThread->join();
	decThread->join();
	pThread->join();
}

void grabber::loop()
{
	
	while(!b_stop)
	{
		AVPacket packet;
		av_init_packet( &packet );
		int ret = 0;
		int got_picture = 0;
		
		
		if(av_read_frame(pFormatCtx, &packet)>=0)
		{
			if(packet.stream_index==videoindex)
			{
				AVPacket* pkt = new AVPacket;
				av_new_packet(pkt, packet.size);
				
				av_copy_packet(pkt, &packet);
				decFifo.push(pkt);
			}
		}
		av_free_packet(&packet);
		
	}
}

void grabber::decloop()
{
	AVFrame* frame = av_frame_alloc();
	while(!b_stop)
	{
		AVPacket* packet = decFifo.pop();
		if (!packet)
		{
			break;
		}
		
		int got_picture = 0;
		int ret = avcodec_decode_video2(pCodecCtx, frame, &got_picture, packet);
		if(ret < 0)
		{
			printf("Decode Error.\n");
			break;
		}
		if (got_picture)
		{
			AVFrame* p = free_fifo.pop();
			if (!p)
			{
				break;
			}
			av_picture_copy((AVPicture*)p, (AVPicture*)frame, (AVPixelFormat)frame->format, frame->width, frame->height);
			valid_fifo.push(p);
		}
		//delete []packet;
		av_free_packet(packet);
		delete packet;
	}
	av_frame_free(&frame);
}

void grabber::procloop()
{
	while(!b_stop)
	{
		AVFrame* frame = valid_fifo.pop();
		if (!frame)
		{
			break;
		}
		AVFrame	*pFrameYUV = pEnc->getFreeFrame();
		if (!pFrameYUV)
		{
			break;
		}
		int ret = sws_scale(img_convert_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, pCodecCtx->height, 
			pFrameYUV->data, pFrameYUV->linesize);
		if(ret < 0)
		{
			printf("sws_scale Error.\n");
			break;
		}
		pEnc->pushValidFrame(pFrameYUV);
		free_fifo.push(frame);
	}
	
}
