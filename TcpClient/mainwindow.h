#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QLineEdit>

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
    void onIPAddressTextChanged(const QString &text);
    void onPortTextChanged(const QString &text);

    void sendFilePushButtonClicked();
    void discardSocket();
    void connectToHost();
    void readSocket();

private:
    void sendFile(QTcpSocket *socket, const QString &fileName);

    bool isValidIPAddress(const QString &address);
    bool isValidPort(const QString &port);

    void makeConnections();

private:
    Ui::MainWindow *ui;

    std::vector<QLineEdit*> m_host_address;
    QTcpSocket *m_tcp_socket;
    QString m_ip_address;
    quint16 m_port;
};
#endif // MAINWINDOW_H
