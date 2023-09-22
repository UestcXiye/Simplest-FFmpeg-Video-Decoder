// Simplest FFmpeg Decoder.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于FFmpeg的解码器
* Simplest FFmpeg Decoder
*
* 原程序：
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序实现了视频文件的解码(支持HEVC，H.264，MPEG2等)。
* 是最简单的FFmpeg视频解码方面的教程。
* 通过学习本例子可以了解FFmpeg的解码流程。
*
* This software is a simplest video decoder based on FFmpeg.
* Suitable for beginner of FFmpeg.
*
*/

#include "stdafx.h"
#pragma warning(disable:4996)

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};


int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;
	//输入文件路径
	char filepath[] = "Titanic.ts";

	int frame_cnt;

	av_register_all();
	avformat_network_init();
	// 申请avFormatContext空间，记得要释放
	pFormatCtx = avformat_alloc_context();
	// 打开媒体文件
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	// 读取媒体文件信息，给pFormatCtx赋值
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	// 根据视频流信息的codec_id找到对应的解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	// 使用给定的pCodec初始化pCodecCtx
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	/*
	* 在此处添加输出视频信息的代码
	* 取自于pFormatCtx，使用fprintf()
	*/
	FILE *fp_txt = fopen("output.txt", "wb+");

	fprintf(fp_txt, "封装格式：%s\n", pFormatCtx->iformat->long_name);
	fprintf(fp_txt, "比特率：%d\n", pFormatCtx->bit_rate);
	fprintf(fp_txt, "视频时长：%d\n", pFormatCtx->duration);
	fprintf(fp_txt, "视频编码方式：%s\n", pFormatCtx->streams[videoindex]->codec->codec->long_name);
	fprintf(fp_txt, "视频分辨率：%d * %d\n", pFormatCtx->streams[videoindex]->codec->width, pFormatCtx->streams[videoindex]->codec->height);

	// 在avcodec_receive_frame()函数作为参数，获取到frame
	// 获取到的frame有些可能是错误的要过滤掉，否则相应帧可能出现绿屏
	pFrame = av_frame_alloc();
	//作为yuv输出的frame承载者，会进行缩放和过滤出错的帧，YUV相应的数据也是从该对象中读取
	pFrameYUV = av_frame_alloc();
	// 用于渲染的数据，且格式为YUV420P
	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	// Output Info
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");
	// 由于解码出来的帧格式不一定是YUV420P的,在渲染之前需要进行格式转换
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	FILE *fp_h264 = fopen("output.h264", "wb+");
	FILE *fp_yuv = fopen("output.yuv", "wb+");

	// 帧计数器
	frame_cnt = 0;
	// 开始一帧一帧读取
	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == videoindex)
		{
			/*
			* 在此处添加输出H264码流的代码
			* 取自于packet，使用fwrite()
			*/
			fwrite(packet->data, 1, packet->size, fp_h264);

			// 输出每一个解码前视频帧参数：帧大小
			fprintf(fp_txt, "帧%d大小：%d\n", frame_cnt, packet->size);

			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture)
			{
				// 格式转换，解码后的数据经过sws_scale()函数处理，去除无效像素
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				printf("Decoded frame index: %d\n", frame_cnt);

				/*
				* 在此处添加输出YUV的代码
				* 取自于pFrameYUV，使用fwrite()
				*/
				fwrite(pFrameYUV->data[0], 1, pCodecCtx->width*pCodecCtx->height, fp_yuv);// Y
				fwrite(pFrameYUV->data[1], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);// U
				fwrite(pFrameYUV->data[2], 1, pCodecCtx->width*pCodecCtx->height / 4, fp_yuv);// V

				// 输出每一个解码后视频帧参数：帧类型
				char pict_type_str[10];
				switch (pFrame->pict_type)
				{
				case AV_PICTURE_TYPE_NONE:
					sprintf(pict_type_str, "NONE");
					break;
				case AV_PICTURE_TYPE_I:
					sprintf(pict_type_str, "I");
					break;
				case AV_PICTURE_TYPE_P:
					sprintf(pict_type_str, "P");
					break;
				case AV_PICTURE_TYPE_B:
					sprintf(pict_type_str, "B");
					break;
				case AV_PICTURE_TYPE_SI:
					sprintf(pict_type_str, "SI");
					break;
				case AV_PICTURE_TYPE_S:
					sprintf(pict_type_str, "S");
					break;
				case AV_PICTURE_TYPE_SP:
					sprintf(pict_type_str, "SP");
					break;
				case AV_PICTURE_TYPE_BI:
					sprintf(pict_type_str, "BI");
					break;
				default:
					break;
				}
				fprintf(fp_txt, "帧%d类型：%s\n", frame_cnt, pict_type_str);

				frame_cnt++;

			}
		}
		av_free_packet(packet);
	}

	// 关闭文件
	fclose(fp_txt);
	fclose(fp_h264);
	fclose(fp_yuv);

	// 释放相关资源
	sws_freeContext(img_convert_ctx);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
