#include "encoder.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
};

encoder::encoder(int width, int height, void* user_data, func push_data):pThread(NULL), b_stop(false),
m_ffvcodec(NULL), m_ffvpCodecCtx(NULL),m_width(width),m_height(height),
m_user_data(user_data), m_push_data(push_data)
{
	
}

encoder::~encoder()
{
	if (pThread)
	{
		delete pThread;
		pThread = NULL;
	}

	if (m_ffvpCodecCtx)
	{
		avcodec_close(m_ffvpCodecCtx);
		av_free(m_ffvpCodecCtx);
		m_ffvpCodecCtx = NULL;
	}

	while(free_fifo.size()>0)
	{
		AVFrame* frame = free_fifo.pop();
		av_freep(&frame->data[0]);
		av_frame_free(&frame);
	}
	while(valid_fifo.size()>0)
	{
		AVFrame* frame = valid_fifo.pop();
		av_freep(&frame->data[0]);
		av_frame_free(&frame);
	}



}

int encoder::init()
{
	m_ffvcodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!m_ffvcodec)  {
		printf("Couldn't find encoder H264.\n");
		return -1;
	} 
	m_ffvpCodecCtx = avcodec_alloc_context3(m_ffvcodec);

	/* put sample parameters */
	/* resolution must be a multiple of two */
	m_ffvpCodecCtx->width = m_width;
	m_ffvpCodecCtx->height = m_height;
	/* frames per second */
	m_ffvpCodecCtx->time_base.num = 1;
	m_ffvpCodecCtx->time_base.den = 25;
	m_ffvpCodecCtx->gop_size = 10; /* emit one intra frame every ten frames */
	m_ffvpCodecCtx->max_b_frames=0;
	m_ffvpCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P; 

	av_opt_set(m_ffvpCodecCtx->priv_data,"preset","ultrafast",0); 
	av_opt_set(m_ffvpCodecCtx->priv_data, "tune", "zerolatency", 0);
	av_opt_set(m_ffvpCodecCtx->priv_data, "crf", "28" , 0);
	if (avcodec_open2(m_ffvpCodecCtx, m_ffvcodec, NULL) < 0) {
		printf("Couldn't open encoder H264.\n");
		return -1; 
	} 
	for (int i =0; i<10;i++)
	{
		AVFrame* frame = avcodec_alloc_frame();
		frame->format = AV_PIX_FMT_YUV420P;
		frame->width  = m_width;
		frame->height = m_height;
		int dst_bufsize = av_image_alloc(frame->data, frame->linesize, m_ffvpCodecCtx->width, m_ffvpCodecCtx->height,
			m_ffvpCodecCtx->pix_fmt, 32);
		if(dst_bufsize < 0)
		{
			printf("Couldn't alloc image\n");
			return -1; 
		}
		free_fifo.push(frame);
	}

	return 0 ;

}

void encoder::startLoop()
{
	pThread = new boost::thread(&encoder::loop, this);
}

void encoder::stopLoop()
{

	b_stop = true;
	free_fifo.wakeup();
	valid_fifo.wakeup();
	pThread->join();
}

void encoder::loop()
{
	while(!b_stop)
	{
		
		AVFrame* frame = valid_fifo.pop();
		if (!frame)
		{
			break;
		}

		bool b_go = true;
		AVFrame* go_frame = frame;
		int index =0;
		while(b_go)
		{
			index++;

			AVPacket av_pkt;
			av_init_packet( &av_pkt );
			av_pkt.data = NULL;    // packet data will be allocated by the encoder
			av_pkt.size = 0;

			int is_data = 0;
			if( avcodec_encode_video2( m_ffvpCodecCtx, &av_pkt, go_frame, &is_data ) < 0
				|| is_data == 0 )
			{
				//printf("encode %d packet\r\n", index);
				b_go = false;
			}
			else
			{
				if(m_push_data)
					m_push_data(m_user_data, av_pkt.data,av_pkt.size);
				go_frame = NULL;
			}
			av_free_packet( &av_pkt );
		}

		
		
		free_fifo.push(frame);
		
	}
	
}

AVFrame	* encoder::getFreeFrame()
{
	return free_fifo.pop();
}

void encoder::pushValidFrame( AVFrame *pFrameYUV )
{
	valid_fifo.push(pFrameYUV);
}
