#pragma once

#include <QtWidgets/QWidget>
#include "ui_webcam.h"
#include <QMenu>

class WebCam : public QWidget
{
    Q_OBJECT

public:
    WebCam(QWidget *parent = nullptr);
    ~WebCam();

public slots:
    void MaxWindow();
    void NormalWindow();

    //右键不同窗口的槽函数
    void View1();
    void View4();
    void View9();
    void View16();

    //新增摄像机
    void AddCam();
    //删除摄像机
    void DelCam();
    //修改摄像机数据
    void SetCam();
    //开始录制
    void StartRecord();
    //结束录制
    void StopRecord();
	//预览界面
	void Preview();
	//回放界面
	void PlayBack();
    //选择摄像机
    void SelectCam(QModelIndex index);
    //选择日期
    void SelectDate(QDate date);
    //选择时间，播放视频
    void PlayVideo(QModelIndex index);

protected:
    //鼠标拖动窗口
    void mouseMoveEvent(QMouseEvent* event)override;
    void mousePressEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;

    //窗口大小发生变化
    void resizeEvent(QResizeEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event)override;

    //渲染窗口
	void View(int count);

    //刷新相机列表
    void RefreshCams();

    //编辑修改摄像机
    void SetCam(int index);

	//渲染定时器 回调函数
	void timerEvent(QTimerEvent* event)override;
private:
    Ui::webcamClass ui;

    bool isDragging = false;
    QPoint dragPosition;
    QMenu left_menu_;
};

