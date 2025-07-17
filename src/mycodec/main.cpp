#include <iostream>
#include <thread>
#include <chrono>
#include "tools.h"
#include "demux_task.h"
#include "decode_task.h"
#include "video_view.h"
#include "mux_task.h"
#include "audioplay.h"
#include <fstream>

int main(int argc, char* argv[])
{
	auto audio = AudioPlay::Instance();
	AAudioSpec spec;
	spec.freq = 44100;
	audio->Open(spec);
	std::ifstream ifs("test_pcm.pcm",std::ios::binary);
	if (!ifs)return -1;
	unsigned char buf[1024] = { 0 };
	for (;;)
	{
		ifs.read((char*)buf, sizeof(buf));
		int len = ifs.gcount();
		if (len <= 0)break;
		audio->Push(buf, len);
	}
	getchar();
	return 0;
}
