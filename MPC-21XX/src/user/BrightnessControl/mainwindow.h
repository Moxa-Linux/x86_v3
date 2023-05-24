#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_autoBREN_checkBox_clicked();

    void on_auto_Apply_clicked();

    void on_mbr_Apply_clicked();

    void enableWidget();

private:
    bool supportAutoBrightness;
    QLabel *brightnessControllerStatusLabel;
    QLabel *applyStatusLabel;
    QTimer *disableWidgetTimer;
    int execExternalCMD(char *cmd, char *result);

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
