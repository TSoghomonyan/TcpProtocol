#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_tcp_socket = new QTcpSocket();
    makeConnections();

}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::discardSocket()
{
    m_tcp_socket->deleteLater();
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

void MainWindow::onIPAddressTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->ipLabelStatus->clear();
        ui->portText->setEnabled(false);
        return;
    }

    if (isValidIPAddress(text)) {
        ui->ipLabelStatus->setText("✓ Valid");
        ui->ipLabelStatus->setStyleSheet("color: green;");
        ui->connectToHostButton->setEnabled(isValidPort(ui->portText->text()));
    } else {
        ui->ipLabelStatus->setText("✗ Invalid");
        ui->ipLabelStatus->setStyleSheet("color: red;");
        ui->connectToHostButton->setEnabled(false);
    }
}

void MainWindow::onPortTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        ui->portLabelStatus->clear();
        ui->portText->setEnabled(false);
        return;
    }

    if (isValidPort(text)) {
        ui->portLabelStatus->setText("✓ Valid");
        ui->portLabelStatus->setStyleSheet("color: green;");
        ui->connectToHostButton->setEnabled(isValidIPAddress(ui->ipText->text()));
    } else {
        ui->portLabelStatus->setText("✗ Invalid");
        ui->portLabelStatus->setStyleSheet("color: red;");
        ui->connectToHostButton->setEnabled(false);
    }
}

bool MainWindow::isValidIPAddress(const QString &text)
{
    QString trimmedText = text.trimmed();

    if (trimmedText.isEmpty()) {
        return false;
    }

    QStringList octets = trimmedText.split(".");

    if (octets.size() != 4) {
        return false;
    }

    for (const QString &octet : octets) {
        if (octet.isEmpty()) {
            return false;
        }

        for (const QChar &ch : octet) {
            if (!ch.isDigit()) {
                return false;
            }
        }

        if (octet.length() > 1 && octet.startsWith('0')) {
            return false;
        }

        if (octet.length() > 3) {
            return false;
        }

        bool ok;
        int value = octet.toInt(&ok);

        if (!ok || value < 0 || value > 255) {
            return false;
        }
    }

    m_ip_address = text;
    return true;
}

bool MainWindow::isValidPort(const QString &text)
{
    QString trimmedText = text.trimmed();

    if (trimmedText.isEmpty()) {
        return false;
    }

    for (const QChar &ch : trimmedText) {
        if (!ch.isDigit()) {
            return false;
        }
    }

    if (trimmedText.length() > 1 && trimmedText.startsWith('0')) {
        return false;
    }

    if (trimmedText.length() > 5) {
        return false;
    }

    bool ok;
    int port = trimmedText.toInt(&ok);

    if (!ok || port < 1 || port > 65535) {
        return false;
    }

    m_port = port;
    return true;
}

void MainWindow::makeConnections()
{
    connect(ui->ipText, &QLineEdit::textEdited, this, &MainWindow::onIPAddressTextChanged);
    connect(ui->portText, &QLineEdit::textEdited, this, &MainWindow::onPortTextChanged);

    connect(ui->connectToHostButton, &QPushButton::clicked, this, &MainWindow::connectToHost);
    connect(ui->sendFileButton, &QPushButton::clicked, this, &MainWindow::sendFilePushButtonClicked);

    connect(m_tcp_socket, &QTcpSocket::readyRead, this, &MainWindow::readSocket);
    connect(m_tcp_socket, &QTcpSocket::disconnected, this, &MainWindow::discardSocket);
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

void MainWindow::connectToHost()
{
    QString ipAddress = ui->ipText->text().trimmed();
    int port = ui->portText->text().toInt();

    if (m_tcp_socket->state() == QTcpSocket::ConnectedState) {
        m_tcp_socket->disconnectFromHost();
    }

    ui->statusbar->showMessage(QString("Connecting to %1:%2...").arg(ipAddress).arg(port));
    ui->connectToHostButton->setEnabled(false);

    m_tcp_socket->connectToHost(QHostAddress(ipAddress), port);

    if(m_tcp_socket->waitForConnected(3000)) { // TODO it is taking time, and soft freezing
        ui->statusbar->showMessage(QString("Connected to %1:%2").arg(ipAddress).arg(port));
        ui->connectToHostButton->setText("Disconnect");
        ui->connectToHostButton->setEnabled(true);
    } else {
        ui->statusbar->showMessage("Connection failed: " + m_tcp_socket->errorString());
        ui->connectToHostButton->setText("Connect");
        ui->connectToHostButton->setEnabled(true);
    }
}
