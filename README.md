# Simplest-FFmpeg-Video-Decoder
一个最简单的基于FFmpeg的视频解码器。本程序实现了视频文件的解码，支持HEVC，H.264，MPEG2等。

运行程序后，对于测试文件，会在原目录下输出以下几种文件：
1. output.h264：解码前的H.264码流数据（只对MPEG-TS，AVI格式作要求）；
2. output.yuv：解码后的YUV420P像素数据
3. output.txt：封装格式参数（封装格式、比特率、时长）、视频编码参数（编码方式、宽高）、每一个解码前视频帧参数（帧大小）、每一个解码后视频帧参数（帧类型）。
