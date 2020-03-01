# GB28181Android
GB28181 for Android, include RTP/PS/H264/G711

这个工程只是个demo，同事已经改做了一些优化，有完善了的同学愿意分享可以提交一下，我不做andorid，不维护这个。

如果只是做个28181的推流，建议直接用ffmpeg/doc/example/muxing.c, 可以直接将aac+h264(也支持其他一些音频视频格式)封装成PS流，PS流分包发送应该不麻烦。

ffmpeg自带的example比这一堆臃肿的代码强100倍。

花了点业余时间整理了一整套系统
https://www.cnblogs.com/dong1/p/11878456.html
