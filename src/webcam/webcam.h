#pragma once

#include <QtWidgets/QWidget>
#include "ui_webcam.h"

class webcam : public QWidget
{
    Q_OBJECT

public:
    webcam(QWidget *parent = nullptr);
    ~webcam();

private:
    Ui::webcamClass ui;
};

