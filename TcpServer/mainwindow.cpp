#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_tcp_server = new QTcpServer();

    if(m_tcp_server->listen(QHostAddress::AnyIPv4, 4300)) {
        connect(m_tcp_server, &QTcpServer::newConnection, this, &MainWindow::newConnection);
        ui->statusbar->showMessage("Tcp Server started!");
    }else {
        QMessageBox::information(this, "Tcp Server Error: ", m_tcp_server->errorString());
    }

    connect(ui->SendFilePushButton, &QPushButton::clicked, this, &MainWindow::sendFilePushButtonClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newConnection()
{
    while(m_tcp_server->hasPendingConnections()){
        addToSocketList(m_tcp_server->nextPendingConnection());
    }
}

void MainWindow::discardSocket()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;

    m_clients_list.removeOne(socket);
    ui->textEditMessages->append("Client disconnected - Socket ID: "
                                 + QString::number(socket->socketDescriptor()));
    int index = ui->ClientsListComboBox->findText(QString::number(socket->socketDescriptor()));
    if (index >= 0)
        ui->ClientsListComboBox->removeItem(index);

    socket->deleteLater();
}

void MainWindow::addToSocketList(QTcpSocket *socket)
{
    m_clients_list.append(socket);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
    ui->textEditMessages->append("Client is connected with Server - Socket ID: "
                                 + QString::number(socket->socketDescriptor()));
    ui->ClientsListComboBox->addItem(QString::number(socket->socketDescriptor()),
                                        QVariant::fromValue((qulonglong)socket));
}

void MainWindow::sendFilePushButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this
                                                    , "Select file"
                                                    , QCoreApplication::applicationFilePath()
                                                    , "file (*.jpg *.txt *.png *.bmp)");


    if(ui->TransferTypeComboBox->currentText() == "Broadcast") {
        foreach(QTcpSocket *tempSocket, m_clients_list) {
            sendFile(tempSocket, filePath);
        }
    }else if(ui->TransferTypeComboBox->currentText() == "Receiver") {
        QString receiverID = ui->ClientsListComboBox->currentText();
        foreach (QTcpSocket *tempSocket, m_clients_list) {
            if(tempSocket->socketDescriptor() == receiverID.toLongLong()){
                sendFile(tempSocket, filePath);
            }
        }
    }
}

void MainWindow::readSocket()
{
    QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
    QByteArray readBuffer;
    QDataStream data(socket);
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

