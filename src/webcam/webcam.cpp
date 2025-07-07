#include "webcam.h"
#include "camconfig.h"
#include "camera_widget.h"
#include "camera_record.h"
#include "PPlayVideo.h"

#include <QMouseEvent>
#include <QPoint>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QString>
#include <QDebug>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QIcon>
#include <QListWidgetItem>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <sstream>
#include <QDir>
#include <map>
#include <vector >

#define CAM_CONFIG_PATH "cams.db"

//渲染窗口
static CameraWidget* cam_wids[16];

//视频录制
static std::vector<CameraRecord*> records;

//存储视频日期时间
struct  CamVideo
{
    QString filepath;
    QDateTime datetime;
};

static std::map<QDate, std::vector<CamVideo>>cam_videos;

WebCam::WebCam(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    //去除原窗口边框 
    setWindowFlags(Qt::FramelessWindowHint);
    //调整标题和主体的布局 
    auto vlay = new QVBoxLayout();
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);
    vlay->addWidget(ui.headwidget);
    vlay->addWidget(ui.bodywidget);
    this->setLayout(vlay);

    ////调整相机列表和相机预览的布局 
    //auto hlay = new QHBoxLayout();
    //hlay->setContentsMargins(0, 0, 0, 0);
    //hlay->addWidget(ui.leftwidget);
    //hlay->addWidget(ui.camswidget);
    //hlay->addWidget(ui.playbcakwidget);
    //ui.bodywidget->setLayout(hlay);

    //初始化右键菜单 
    auto m = left_menu_.addMenu("视图");
    auto a = m->addAction(QString("1窗口"));
    connect(a, &QAction::triggered, this, &WebCam::View1);
	a = m->addAction(QString("4窗口"));
    connect(a, &QAction::triggered, this, &WebCam::View4);
	a = m->addAction(QString("9窗口"));
    connect(a, &QAction::triggered, this, &WebCam::View9);
	a = m->addAction(QString("16窗口"));
    connect(a, &QAction::triggered, this, &WebCam::View16);
    a = left_menu_.addAction(QString("开始录制"));
    connect(a, &QAction::triggered, this, &WebCam::StartRecord);
    a = left_menu_.addAction(QString("停止录制"));
    connect(a, &QAction::triggered, this, &WebCam::StopRecord);

    //默认九窗口
    View9();

    CamConfig::Instance()->Load(CAM_CONFIG_PATH);
    ui.time_list->clear();
    RefreshCams();

    //启动定时器渲染
    startTimer(1);
    Preview();//默认显示预览
}

WebCam::~WebCam()
{}

void WebCam::MaxWindow()
{
    ui.max->setVisible(false);
    ui.normal->setVisible(true);
    showMaximized();
}

void WebCam::NormalWindow()
{
	ui.max->setVisible(true);
	ui.normal->setVisible(false);
    showNormal();
}

void WebCam::View1()
{
    View(1);
}

void WebCam::View4()
{
    View(4);
}

void WebCam::View9()
{
    View(9);
}

void WebCam::View16()
{
    View(16);
}

void WebCam::AddCam()
{
    SetCam(-1);
}

void WebCam::DelCam()
{
	int row = ui.cams_list->currentIndex().row();
	if (row < 0)
	{
		QMessageBox::information(this, QString("错误"), QString("请选择摄像机"));
		return;
	}
    std::stringstream ss;
    ss << "您确定要删除摄像机" << ui.cams_list->currentItem()->text().toUtf8().constData() << "吗";
    if (QMessageBox::information(this, QString("确认"), QString(ss.str().c_str()),QMessageBox::Yes,QMessageBox::No)!=QMessageBox::Yes)
    {
        return;
    }
    CamConfig::Instance()->DeleteCam(row);
    RefreshCams();
}

void WebCam::SetCam()
{
    int row = ui.cams_list->currentIndex().row();
    if (row < 0)
    {
        QMessageBox::information(this, "错误", QString("请选择摄像机"));
        return;
    }
    SetCam(row);
}

void WebCam::StartRecord()
{
    StopRecord();
    qDebug() << "开始全部摄像头录制";
    ui.status->setText("录制中...");
    //获取配置列表
    auto conf = CamConfig::Instance();
    int count = conf->GetCamCount();
    for (int i = 0; i < count; i++)
    {
        auto cam = conf->GetCam(i);
        //创建目录
        std::stringstream ss;
        ss << cam.save_path << "/" << i << "/";
        QDir dir;
        dir.mkpath(ss.str().c_str());
        CameraRecord* rec = new CameraRecord();
        rec->set_rtsp_url(cam.url);
        rec->set_save_path(ss.str());
        rec->set_file_sec(5);
        rec->Start();
        records.push_back(rec);
    }
    //分别开始录制线程
}

void WebCam::StopRecord()
{
    qDebug() << "结束全部摄像头录制";
    ui.status->setText("监控中...");
    for (auto rec : records)
    {
        rec->Stop();
        delete rec;
    }
    records.clear();
}

void WebCam::timerEvent(QTimerEvent* event)
{
    int wid_size = sizeof(cam_wids) / sizeof(QWidget*);
    for (int i = 0; i < wid_size; i++)
    {
        if (cam_wids[i])
        {
            cam_wids[i]->Draw();
        }
    }
}

void WebCam::Preview()
{
    ui.camswidget->show();
    ui.playbcakwidget->hide();
	ui.btn_preview->setChecked(true);
}

void WebCam::PlayBack()
{
	ui.camswidget->hide();
	ui.playbcakwidget->show();
    ui.btn_playback->setChecked(true);
}

void WebCam::SelectCam(QModelIndex index)
{
    qDebug() << "SelectCam(QModelIndex index)" << index.row();
	auto conf = CamConfig::Instance();
	auto cam = conf->GetCam(index.row());//获取相机参数
	if (cam.name[0] == '\0')return;
	//视频存储路径
	std::stringstream ss;
	ss << cam.save_path << "/" << index.row() << "/";
	//遍历此目录
	QDir dir(ss.str().c_str());
	if (!dir.exists())return;
	//获取目录下文件列表
	QStringList fileters;
	fileters << "*.mp4" << "*.avi";
	dir.setNameFilters(fileters);//删选文件
    ui.cal->ClearDate();
    cam_videos.clear();
	//所有文件列表
	auto files = dir.entryInfoList();
	for (auto file : files)
	{
        //cam_2025_07_05_03_38_53.mp4
        QString filename = file.fileName();
        auto tmp = filename.left(filename.size() - 4);
        tmp = tmp.right(tmp.length() - 4);
        auto dt = QDateTime::fromString(tmp,"yyyy_MM_dd_hh_mm_ss");
        qDebug() << dt.date();
        ui.cal->AddDate(dt.date());

        CamVideo video;
        video.datetime = dt;
        video.filepath = file.absoluteFilePath();
        cam_videos[dt.date()].push_back(video);
	}
    //更新页面
    ui.cal->update();
}

void WebCam::SelectDate(QDate date)
{
	qDebug() << "SelectDate(QDate date)" << date.toString();
    auto dates = cam_videos[date];
    ui.time_list->clear(); 
    for (auto d : dates)
    {
        auto item = new QListWidgetItem(d.datetime.time().toString());
        //item添加自定义数据 文件路径
        item->setData(Qt::UserRole, d.filepath);
        ui.time_list->addItem(item);
    }
}

void WebCam::PlayVideo(QModelIndex index)
{
    qDebug() << "PlayVideo(QModelIndex index)" << index.row();
    auto item = ui.time_list->currentItem();
    if (!item)return;
    QString path = item->data(Qt::UserRole).toString();
    qDebug() << path;
    static PPlayVideo play;
    play.Open(path.toUtf8());
    play.show();
}

void WebCam::mouseMoveEvent(QMouseEvent * event)
{
	// 如果正在拖动且鼠标左键按下 
    if (isDragging && event->buttons() & Qt::LeftButton)
	{
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
	}
    QWidget::mouseMoveEvent(event);
}

void WebCam::mousePressEvent(QMouseEvent* event)
{
	// 如果按下的是鼠标左键且点击位置在窗口顶部50像素区域内（允许拖动区域） 
    if (event->button() == Qt::LeftButton && event->pos().y() <= 50)
	{
			isDragging = true;
			dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
    }
    QWidget::mousePressEvent(event);
}

void WebCam::mouseReleaseEvent(QMouseEvent* event)
{
	// 如果释放的是鼠标左键 
    if (event->button() == Qt::LeftButton)
    {
        isDragging = false;
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}

void WebCam::resizeEvent(QResizeEvent* event)
{
    int x = width() - ui.buttonwidget->width();
    int y = ui.headwidget->y();
    ui.buttonwidget->move(x, y);

    //窗口调整后调整部件的布局
	int leftHeight = height() - ui.headwidget->height(); 
	ui.leftwidget->setGeometry(0, ui.leftwidget->y(), ui.leftwidget->width(), leftHeight);
    int camsWidth = width() - ui.leftwidget->width();
	ui.camswidget->setGeometry(ui.camswidget->x(), ui.camswidget->y(), camsWidth, leftHeight);
	ui.playbcakwidget->setGeometry(ui.playbcakwidget->x(), ui.playbcakwidget->y(), camsWidth, leftHeight);
	
    QWidget::resizeEvent(event); // 确保基类处理其他布局
}

void WebCam::contextMenuEvent(QContextMenuEvent* event)
{
    //鼠标位置显示右键菜单 
    left_menu_.exec(QCursor::pos());
    event->accept();
}

void WebCam::View(int count)
{
    qDebug() << "View" << count;
    int cols = sqrt(count);
    int wid_size = sizeof(cam_wids)/sizeof(QWidget*);
    //初始化布局器
    auto lay =(QGridLayout*)ui.camswidget->layout();
    if (!lay)
    {
        lay = new QGridLayout();
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(2);
        ui.camswidget->setLayout(lay);
    }
    //初始化窗口
    for (int i = 0; i < count; i++)
    {
        if (!cam_wids[i])
        {
            cam_wids[i] = new CameraWidget();
            cam_wids[i]->setStyleSheet("background-color:rgb(51,51,51)");
        }
        lay->addWidget(cam_wids[i],i/cols,i%cols);
    }

    //清理多余窗口
    for (int i = count; i < wid_size; i++)
    {
        if (cam_wids[i])
        {
            delete cam_wids[i];
            cam_wids[i] = nullptr;
        }
    }
}

void WebCam::RefreshCams()
{
    auto c = CamConfig::Instance();
    ui.cams_list->clear();
    int count = c->GetCamCount();
    for (int i = 0; i < count; i++)
    {
        auto cam = c->GetCam(i);
        auto item = new QListWidgetItem(QIcon(":/webcam/img/cam1.png"),cam.name);
        
        ui.cams_list->addItem(item);
    }

}

void WebCam::SetCam(int index)
{
    auto instance = CamConfig::Instance();
	QDialog dlg(this);
	dlg.resize(800, 150);
	QFormLayout lay;
	dlg.setLayout(&lay);

	QLineEdit name_edit;
	lay.addRow(QString("名称"), &name_edit);
	QLineEdit url_edit;
	lay.addRow(QString("主码流"), &url_edit);
	QLineEdit sub_url_edit;
	lay.addRow(QString("辅码流"), &sub_url_edit);
	QLineEdit save_path_edit;
	lay.addRow(QString("保存目录"), &save_path_edit);

	QPushButton save;
	save.setText(QString("保存"));
	connect(&save, &QPushButton::clicked, &dlg, &QDialog::accept);
	lay.addRow("", &save);
    //编辑 读入源数据显示
    if (index >= 0)
    {
        auto cam = instance->GetCam(index);
        name_edit.setText(QString(cam.name));
        url_edit.setText(QString(cam.url));
        sub_url_edit.setText(QString(cam.sub_url));
        save_path_edit.setText(QString(cam.save_path));
    }
	for (;;)
	{
		//点击保存按钮
		if (dlg.exec() == QDialog::Accepted)
		{
			if (name_edit.text().isEmpty())
			{
				QMessageBox::information(0, QString("错误"), QString("请输入名称"));
				continue;
			}
			if (url_edit.text().isEmpty())
			{
				QMessageBox::information(0, QString("错误"), QString("请输入主码流"));
				continue;
			}
			if (sub_url_edit.text().isEmpty())
			{
				QMessageBox::information(0, QString("错误"), QString("请输入辅码流"));
				continue;
			}
			if (save_path_edit.text().isEmpty())
			{
				QMessageBox::information(0, QString("错误"), QString("请输入保存地址"));
				continue;
			}
			break;
		}
		return;
	}
	CamData data;
	strcpy_s(data.name, name_edit.text().toUtf8());
	strcpy_s(data.url, url_edit.text().toUtf8());
	strcpy_s(data.sub_url, sub_url_edit.text().toUtf8());
	strcpy_s(data.save_path, save_path_edit.text().toUtf8());
    //修改
    if (index >= 0)
    {
        instance->SetCam(index, data);
    }
    //新增
    else
    {
		instance->Push(data);
    }
	instance->Save(CAM_CONFIG_PATH);
	RefreshCams();
}

