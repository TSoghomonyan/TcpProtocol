#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void readSocket();
    void discardSocket();
    void sendFilePushButtonClicked();

private:
    void sendFile(QTcpSocket *socket, const QString &fileName);

private:
    Ui::MainWindow *ui;
    QTcpSocket *m_tcp_socket;
};
#endif // MAINWINDOW_H
