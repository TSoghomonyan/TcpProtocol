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
    if (!socket) return;

    QByteArray readBuffer;
    QDataStream data(socket);
    data.startTransaction();
    data >> readBuffer;
    if(!data.commitTransaction()) return;

    if (readBuffer.size() < 128) {
        qDebug() << "Insufficient data received";
        return;
    }

    QString dataHeader = readBuffer.mid(0, 128);

    QStringList headerParts = dataHeader.split(',');
    if (headerParts.size() < 2) {
        qDebug() << "Invalid header format";
        return;
    }

    QStringList fileNameParts = headerParts[0].split(':');
    QStringList fileSizeParts = headerParts[1].split(':');

    if (fileNameParts.size() < 2 || fileSizeParts.size() < 2) {
        qDebug() << "Invalid header parts";
        return;
    }

    QString fileName = fileNameParts[1].trimmed();
    QString fileSize = fileSizeParts[1].trimmed();

    if (fileName.isEmpty()) {
        qDebug() << "Empty filename";
        return;
    }

    fileName = fileName.replace(QChar('\0'), "");

    readBuffer = readBuffer.mid(128);

    QString saveFilePath = QCoreApplication::applicationDirPath() + "/" + fileName;
    QFile file(saveFilePath);

    if(file.open(QIODevice::WriteOnly)) {
        file.write(readBuffer);
        file.close();
        qDebug() << "File saved successfully:" << fileName;
    } else {
        qDebug() << "Failed to save file:" << file.errorString();
    }
}

void MainWindow::sendFile(QTcpSocket *socket, const QString &fileName)
{
    if(fileName.isEmpty()) {
        qDebug() << "No file selected";
        return;
    }

    if(!socket || !socket->isOpen()){
        qDebug() << "Socket is not available or not open";
        return;
    }

    QFile fileData(fileName);
    if(!fileData.open(QIODevice::ReadOnly)){
        qDebug() << fileName << " File cannot be opened!";
        return;
    }

    QFileInfo fileInfo(fileData);
    QString fileNameWithExt(fileInfo.fileName());

    qint64 fileSize = fileData.size();
    if (fileSize <= 0) {
        qDebug() << "File is empty or invalid";
        fileData.close();
        return;
    }

    QDataStream socketStream(socket);

    QByteArray header;
    QString headerString = QString("fileName:%1,filesize:%2").arg(fileNameWithExt).arg(fileSize);
    header = headerString.toUtf8();

    if (header.size() > 128) {
        qDebug() << "Header too large";
        fileData.close();
        return;
    }

    header.resize(128);

    QByteArray data = fileData.readAll();
    data.prepend(header);

    socketStream << data;

    qDebug() << "File sent:" << fileNameWithExt << "Size:" << fileSize;
    fileData.close();
}

