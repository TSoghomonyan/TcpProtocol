#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
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

    void newConnection();
    void addToSocketList(QTcpSocket* socket);

    void sendFilePushButtonClicked();

private:
    void sendFile(QTcpSocket *socket, const QString &fileName);
private:
    Ui::MainWindow *ui;

    QTcpServer *m_tcp_server;
    QList<QTcpSocket*> m_clients_list;
};
#endif // MAINWINDOW_H
