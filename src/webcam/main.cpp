#include "webcam.h"
#include "camconfig.h"
#include "camera_record.h"
#include <pplayvideo.h>

#include <QtWidgets/QApplication>
#include <QDir>
//#define TEST_CAM_PATH "test.db"

int main(int argc, char *argv[])
{
    const char* save_path = "./video/0/";
    QDir dir;
    dir.mkpath(save_path);

//cr.set_rtsp_url("rtsp://admin:GZH&password@192.168.31.234/cam/realmonitor?channel=1&subtype=0");

    QApplication app(argc, argv);
	WebCam window;
	window.show();
	auto re = app.exec();
	return re;
    //测试播放器
    //PPlayVideo pv;
    //pv.Open("v1080.mp4");
    //pv.exec();
    return 0;
}
