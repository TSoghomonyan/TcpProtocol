#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_tcp_socket = new QTcpSocket();

    connect(ui->sendFileButton, &QPushButton::clicked, this, &MainWindow::sendFilePushButtonClicked);
    connect(m_tcp_socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(m_tcp_socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);

    QString IPAddress = "127.0.0.1";
    m_tcp_socket->connectToHost(QHostAddress(IPAddress), 4300);
    if(m_tcp_socket->waitForConnected()){
        ui->statusbar->showMessage("Socket (client) is conected");
    }else{
        ui->statusbar->showMessage("Socket (client) is not conected: " + m_tcp_socket->errorString());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::readSocket()
{
    QByteArray readBuffer;
    QDataStream data(m_tcp_socket);
    data.startTransaction();
    data >> readBuffer;
    if(!data.commitTransaction()) return;

    QString dataHeader = readBuffer.mid(0, 128);

    QString fileName = dataHeader.split(',')[0].split(':')[1];
    QString fileExt  = fileName.split(":")[1];
    QString fileSize = dataHeader.split(',')[1].split(':')[1];

    readBuffer = readBuffer.mid(128);

    QString saveFilePath = QCoreApplication::applicationDirPath() + "/" + fileName;
    QFile file(saveFilePath);

    if(file.open(QIODevice::WriteOnly)) {
        file.write(readBuffer);
        file.close();
    }
}

void MainWindow::discardSocket()
{
    m_tcp_socket->deleteLater();
}

void MainWindow::sendFile(QTcpSocket *socket, const QString &fileName)
{
    if(socket && socket->isOpen()){
        QFile fileData(fileName);
        if(fileData.open(QIODevice::ReadOnly)){
            QFileInfo fileInfo(fileData);
            QString fileNameWithExt(fileInfo.fileName());
            QDataStream socketStream(socket);
            QByteArray header;
            header.prepend(("fileName:" + fileNameWithExt + ",filesize:" + QString::number(fileData.size())).toUtf8());
            header.resize(128);

            QByteArray data = fileData.readAll();
            data.prepend(header);

            socketStream << data;
        }else {
            qDebug() << fileName << " File not open!";
        }
    }else if(!socket) {
        qDebug() << "Invalid socket!";
    }else{
        qDebug() << socket->socketDescriptor() << " socket not open!";
    }
}

void MainWindow::sendFilePushButtonClicked()
{
    if(m_tcp_socket && m_tcp_socket->isOpen()){
        QString filePath = QFileDialog::getOpenFileName(this
                                                        , "Select file"
                                                        , QCoreApplication::applicationFilePath()
                                                        , "file (*.jpg *.txt *.png *.bmp)");
        sendFile(m_tcp_socket, filePath);
    }else if(m_tcp_socket){
        qDebug() << "Socket is not open!";
    }else{
        qDebug() << "Invalid socket!";
    }
}
