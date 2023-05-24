#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include "QDebug"
#include <QMessageBox>

int MainWindow::execExternalCMD(char *cmd, char *result) {
    FILE *read_fp;
    int num_read;

    read_fp = popen(cmd, "r");
    if ( read_fp == NULL ) {
        qDebug() << "popen():" << cmd << "fail";
        return -1;
    }

    num_read = fread(result, sizeof(char), 64, read_fp);
    if( num_read == 0 ) {
        qDebug() << "fread():" << cmd << "fail";
        return -1;
    }
    pclose(read_fp);

    *(result+num_read) = '\0'; /* Add the string terminator */

    qDebug() << num_read << "," << result;

    return num_read;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bool auto_BR_EN = 0;
    char resultExternalCMD[64];
    char br_CMD[64];
    int br_l[8]={1,2,3,4,5,6,7,8};
    int holdtime = 10;
    int retval;
    QString tempStr, errStr;

    ui->setupUi(this);

    applyStatusLabel = new QLabel(this);
    brightnessControllerStatusLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(applyStatusLabel);
    ui->statusBar->addPermanentWidget(brightnessControllerStatusLabel);

    disableWidgetTimer=new QTimer(this);
    disableWidgetTimer->setSingleShot(true);
    //connect(disableWidgetTimer, SIGNAL(timeout()), this, SLOT(enableWidget()));

    /* Get brightness controller system status. */
    sprintf(br_CMD, "br-util -q -H");
    memset(resultExternalCMD, 0, sizeof(resultExternalCMD));
    execExternalCMD(&br_CMD[0], &resultExternalCMD[0]);
    sscanf(&resultExternalCMD[0], "%d", &retval);

    if ( retval < 0 ) {
        if ( retval == -1 or retval == -2 ) {
            if ( retval == -1 )
                errStr.sprintf("BrightnessControl cannot execute because ttyS2 open fail");
            else if ( retval == -2 )
                errStr.sprintf("Please close another process using ttyS2");

            QMessageBox closeMessage1(QMessageBox::Warning, tr("Alert"), errStr, QMessageBox::Close);
            if(closeMessage1.exec() == QMessageBox::Close) {
                QApplication::quit();
                exit(0);
            }
        }
        else {
            supportAutoBrightness = false;
            tempStr.append("Not suppot auto-brightness control");
        }
    }
    else {
        /* Note: I used the Brightness controller hardware status to check if the micro-controller supports automatically brightness control */
        if ( (retval & 0x03 ) == 0x03 )
            supportAutoBrightness = true;
        else
            supportAutoBrightness = false;

        if ( retval & 0x01 )
            tempStr.append("Panel: On    ");
        else
            tempStr.append("Panel: Off    ");

        if ( retval & 0x02 )
            tempStr.append("Lightsensor: On    ");
        else
            tempStr.append("Lightsensor: Off    ");

        if ( retval & 0x04 )
            tempStr.append("Display output: On");
        else
            tempStr.append("Display output: Off");
    }

    brightnessControllerStatusLabel->setText(tempStr);

    /* Check if the brightness controller is in auto-brightness control mode. */
    if ( supportAutoBrightness == true ) {

        sprintf(&br_CMD[0], "br-util -q -m");
        execExternalCMD(&br_CMD[0], &resultExternalCMD[0]);

        #if 1 /* Both of these methods can be used to convert character to integer */
        auto_BR_EN = atoi(&resultExternalCMD[0]);
        #else
        auto_BR_EN = QString(&resultExternalCMD[0]).toInt(&ok, 10);
        #endif

        if ( auto_BR_EN == 0 ) {
            applyStatusLabel->setText("Auto Brightness: Disabled    ");
            ui->autoBREN_checkBox->setChecked(false);
        }
        else {
            applyStatusLabel->setText("Auto Brightness: Enabled     ");
            ui->autoBREN_checkBox->setChecked(true);
        }
    }

    /* Get the brightness value from brightness controller if auto brightness control is disabled.
     * auto_BR_EN - 0: auto-brightness mode Disabled;
     * 1: auto-brightness mode Enabled.
     */
    if ( supportAutoBrightness == false || \
         ( supportAutoBrightness == true && auto_BR_EN == 0 ) ) {
        /* Note: The bright level only can read when the auto brightness Control is disabled
         *  Set the inital brightness value in spinBox and Slider.
         */
        sprintf(br_CMD, "br-util -q -g");
        execExternalCMD(&br_CMD[0], &resultExternalCMD[0]);
        qDebug() << br_CMD << &resultExternalCMD[0];
        ui->mbr_spinBox->setValue(atoi(&resultExternalCMD[0]));
        ui->mbr_horizontalSlider->setValue(atoi(&resultExternalCMD[0]));
        ui->autoBREN_checkBox->setChecked(false);
    }

    if ( supportAutoBrightness == true ) {
        /* Set the inital brightness mapping value in brightness mapping levels */
        sprintf(br_CMD, "br-util -q -l");
        execExternalCMD(&br_CMD[0], &resultExternalCMD[0]);
        qDebug() << resultExternalCMD;

        sscanf(&resultExternalCMD[0], "%d %d %d %d %d %d %d %d", &br_l[0], &br_l[1], &br_l[2], &br_l[3], &br_l[4], &br_l[5], &br_l[6], &br_l[7]);

        /* Set the auto light sensor/brightness change interval, which names holdtime in uC. */
        sprintf(br_CMD, "br-util -q -v");
        execExternalCMD(&br_CMD[0], &resultExternalCMD[0]);
        qDebug() << resultExternalCMD;

        sscanf(&resultExternalCMD[0], "%d", &holdtime);
    }
    ui->br_l1->setValue(br_l[0]);
    ui->br_l2->setValue(br_l[1]);
    ui->br_l3->setValue(br_l[2]);
    ui->br_l4->setValue(br_l[3]);
    ui->br_l5->setValue(br_l[4]);
    ui->br_l6->setValue(br_l[5]);
    ui->br_l7->setValue(br_l[6]);
    ui->br_l8->setValue(br_l[7]);

    ui->holdtime->setValue(holdtime);

    sprintf(br_CMD, "br-util -q -f");
    memset(resultExternalCMD, 0, sizeof(resultExternalCMD));
    execExternalCMD(&br_CMD[0], resultExternalCMD);
    qDebug() << resultExternalCMD;
    ui->frmVersion_label->setText(QString("Firmware Version: ")+resultExternalCMD);
    ui->frmVersion_label->setAlignment(Qt::AlignTop);

    if ( supportAutoBrightness == true ) {
        /* Emit the signal to call the on_autoBREN_checkBox_clicked() slot. */
        emit on_autoBREN_checkBox_clicked();
    }
    else {
        ui->autoBREN_checkBox->setEnabled(false);
        ui->br_mapping_groupBox->setEnabled(false);
        ui->br_manually_groupBox->setEnabled(true);
    }
}

MainWindow::~MainWindow()
{
    delete this->brightnessControllerStatusLabel;
    delete this->applyStatusLabel;
    delete this->disableWidgetTimer;
    delete ui;
}

void MainWindow::on_autoBREN_checkBox_clicked()
{
    char br_CMD[64];

    if ( ui->autoBREN_checkBox->isChecked() == true ) {

        /* Lock the UI to mutual execlusive the brightness controller accessing */
        ui->autoBREN_checkBox->setEnabled(false);
        ui->br_manually_groupBox->setEnabled(false);

        sprintf(&br_CMD[0], "br-util -q -e\n");
        qDebug() << br_CMD;
        system(&br_CMD[0]);

        /* Unlock the UI to mutual execlusive the brightness controller accessing */
        applyStatusLabel->setText("Auto Brightness: Enabling    ");
    #if 0
        ui->br_mapping_groupBox->setEnabled(true);
        ui->autoBREN_checkBox->setEnabled(true); /* Unload the widget without delay */
    #else
        connect(disableWidgetTimer, SIGNAL(timeout()), this, SLOT(enableWidget()));
        disableWidgetTimer->start(1600); /* Unlock the widget with a delay to wait the brightness controller ready */
    #endif

    }
    else {
        /* Lock the UI to mutual execlusive the brightness controller accessing */
        ui->autoBREN_checkBox->setEnabled(false);
        ui->br_mapping_groupBox->setEnabled(false);

        sprintf(&br_CMD[0], "br-util -q -d\n");
        qDebug() << br_CMD;
        system(&br_CMD[0]);

        /* Unlock the UI to mutual execlusive the brightness controller accessing */
        applyStatusLabel->setText("Auto Brightness: Disabling   ");
    #if 0
        ui->br_manually_groupBox->setEnabled(true);
        ui->autoBREN_checkBox->setEnabled(true); /* Unload the widget without delay */
    #else
        connect(disableWidgetTimer, SIGNAL(timeout()), this, SLOT(enableWidget()));
        disableWidgetTimer->start(1600); /* Unlock the widget with a delay to wait the brightness controller ready */
    #endif
    }
}

void MainWindow::on_auto_Apply_clicked()
{
    char br_cmd[64];

    sprintf(br_cmd, "br-util -q -t %d\n", ui->holdtime->value());
    qDebug() << br_cmd;
    system(br_cmd);

    sprintf(br_cmd, "br-util -q -w %d,%d,%d,%d,%d,%d,%d,%d\n", ui->br_l1->value(), ui->br_l2->value(), ui->br_l3->value(), ui->br_l4->value(), ui->br_l5->value(), ui->br_l6->value(), ui->br_l7->value(), ui->br_l8->value());
    qDebug() << br_cmd;
    system(br_cmd);
}

void MainWindow::on_mbr_Apply_clicked()
{
    char br_cmd[64];

    sprintf(br_cmd, "br-util -q -s %d\n", ui->mbr_spinBox->value());
    qDebug() << br_cmd;
    system(br_cmd);
}

void MainWindow::enableWidget()
{
    //qDebug() << "Enable Widge";
    if ( ui->autoBREN_checkBox->isChecked() == true ) {
        ui->br_mapping_groupBox->setEnabled(true);
        applyStatusLabel->setText("Auto Brightness: Enabled     ");
    }
    else {
        ui->br_manually_groupBox->setEnabled(true);
        applyStatusLabel->setText("Auto Brightness: Disabled    ");
    }
    ui->autoBREN_checkBox->setEnabled(true);
}
