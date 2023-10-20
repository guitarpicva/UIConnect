#include "direwolfconnect.h"
#include "maidenhead.h"
#include "ui_direwolfconnect.h"
#include "uikissutils.h"

#include <QDateTime>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QStringBuilder>

DirewolfConnect::DirewolfConnect(QWidget *parent)
    : QMainWindow(parent)
      , ui(new Ui::DirewolfConnect)
{
    ui->setupUi(this);
    loadSettings();
    chip = gpiod_chip_open_by_name("gpiochip0");
    if(chip) {
        line = gpiod_chip_get_line(chip, GPIO21);
        qDebug()<<"Get Line:"<<gpiod_line_request_output(line, "reset", 0);
//        gpiod_line_set_value(line, 0);
        on_actionReset_MMDVM_Pi_GPIO_triggered();
    }
    sendTimer = new QTimer(this);
    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(i_serialPortTimeout);
    connectToModem();
    connect(timeoutTimer, &QTimer::timeout, this, &DirewolfConnect::processIncomingData);
    // TEST
    //UIKISSUtils ku;
    //qDebug() << "WRAP:" << ku.kissWrap("Hello World");
    //const QString testframe = "00968a68908a94e08284689aae406103f041424344454647414243444546474142434445464741424344454647414243444546474142434445464741424344454647414243444546474142434445464741424344454647";
//    QByteArrayList bal = UIKISSUtils::unwrapUIFrame(QByteArray::fromHex(testframe.toLatin1()));
//    qDebug()<<"unwrapUIFrame:"<<bal.at(0)<<bal.at(1)<<bal.at(4);
    //inBytes = testframe.toLatin1();
    //processIncomingData();
    // END TEST
    setWindowTitle("UI Connect - " % build % " NOT FOR RESALE Position: " % s_myPosition);
//    timeMap.insert(75, QList<float>() << 0.1144 << 2.368);
//    timeMap.insert(150, QList<float>() << 0.0572 << 1.184);
//    timeMap.insert(300, QList<float>() << 0.0286 << .907);
//    timeMap.insert(600, QList<float>() << 0.0143 << .454);
}

DirewolfConnect::~DirewolfConnect()
{
    delete ui;
}

void DirewolfConnect::closeEvent(QCloseEvent *event)
{
    if(line) {
        gpiod_line_release(line);
    }
    if(chip) {
        gpiod_chip_close(chip);
    }
    b_closingDown = true; // so save doesn't try to reload and re-connect
    if (dw)
        disconnect(dw, 0, 0, 0); // avoid the noid
    saveSettings();
    event->accept();
}

void DirewolfConnect::loadSettings()
{
    s = new QSettings("UIConnect.ini", QSettings::IniFormat);
    i_otaBaudRate = s->value("otaBaudRate", 700).toInt();
    ui->sendNowButton->setText("Send Now"); // % QString::number(i_otaBaudRate) % " baud");
    s_myPosition = s->value("myPosition", "").toStringList().join("/");
    ui->direwolfIPComboBox->clear();
    ui->modemSerialComboBox->clear();
    ui->groupBox_3->setVisible(true);
    ui->action_Preferences->setChecked(true);
    b_useKISSSocket = s->value("useKISSSocket", true).toBool();
    if (b_useKISSSocket) {
        ui->tcpKISSRadioButton->setChecked(true);
        ui->direwolfIPComboBox->setEnabled(true);
        ui->direwolfPortSpinBox->setEnabled(true);
        ui->modemSerialComboBox->setEnabled(false);
        ui->modemBaudComboBox->setEnabled(false);
    }
    else {
        ui->serialKISSRadioButton->setChecked(true);
        ui->direwolfIPComboBox->setEnabled(false);
        ui->direwolfPortSpinBox->setEnabled(false);
        ui->modemSerialComboBox->setEnabled(true);
        ui->modemBaudComboBox->setEnabled(true);
    }
    QStringList nics;
    QList<QHostAddress> addys = QNetworkInterface::allAddresses();
    foreach (const QHostAddress i, addys) {
        if (i.isBroadcast() || i.isLinkLocal() || i.isSiteLocal() || i.toString().contains(":"))
            continue;
        nics.append(i.toString());
    }
    ui->direwolfIPComboBox->addItems(nics);
    QStringList portlist;
    QList<QSerialPortInfo> items = QSerialPortInfo::availablePorts();
    foreach (QSerialPortInfo i, items) {
        portlist << i.portName();
    }
    ui->modemSerialComboBox->addItems(portlist);
    serialPort = s->value("serialPort", "COM3").toString();
    ui->modemSerialComboBox->setCurrentText(serialPort);
    i_baudRate = s->value("serialBaud", 57600).toInt();
    ui->modemBaudComboBox->setCurrentText(QString::number(i_baudRate));
    i_serialPortTimeout = s->value("serialPortTimeout", 50).toInt();
    ui->serialPortTimeoutSpinBox->setValue(i_serialPortTimeout);

    hostname = s->value("hostname", "127.0.0.1").toString();
    ui->direwolfIPComboBox->setCurrentText(hostname);
    port = s->value("port", 8001).toInt();
    ui->direwolfPortSpinBox->setValue(port);
    b_sendIL2P = s->value("sendIL2P", true).toBool();
    ui->il2pCheckBox->setChecked(b_sendIL2P);
    ui->sourceCallSignComboBox->clear();
    ui->sourceCallSignComboBox->addItems(s->value("sourceCallSign", "").toStringList());
    ui->sourceCallSignComboBox->setCurrentText(s->value("sourceCallSignSelected", "").toString());
    ui->destCallSignComboBox->clear();
    ui->destCallSignComboBox->addItems(s->value("destCallSign", "").toStringList());
    ui->destCallSignComboBox->setCurrentText(s->value("destCallSignSelected", "").toString());
    ui->digiOneComboBox->clear();
    ui->digiOneComboBox->addItems(s->value("digiOne", "").toStringList());
    ui->digiOneComboBox->setCurrentText(s->value("digiOneSelected", "").toString());
    ui->digiTwoComboBox->clear();
    ui->digiTwoComboBox->addItems(s->value("digiTwo", "").toStringList());
    ui->digiTwoComboBox->setCurrentText(s->value("digiTwoSelected", "").toString());
    ui->plainTextEdit_2->setPlainText(s->value("txText", "").toString());
    restoreGeometry(s->value("geometry", "").toByteArray());
    restoreState(s->value("windowState", "").toByteArray());
}

void DirewolfConnect::saveSettings()
{
    QStringList items;
    for (int i = 0; i < ui->sourceCallSignComboBox->count(); i++) {
        items << ui->sourceCallSignComboBox->itemText(i);
    }
    items.removeDuplicates();
    s->setValue("sourceCallSign", items);
    s->setValue("sourceCallSignSelected", ui->sourceCallSignComboBox->currentText());
    items.clear();

    for (int i = 0; i < ui->destCallSignComboBox->count(); i++) {
        items << ui->destCallSignComboBox->itemText(i);
    }
    items.removeDuplicates();
    s->setValue("destCallSign", items);
    s->setValue("destCallSignSelected", ui->destCallSignComboBox->currentText());
    items.clear();

    for (int i = 0; i < ui->digiOneComboBox->count(); i++) {
        items << ui->digiOneComboBox->itemText(i);
    }
    items.removeDuplicates();
    s->setValue("digiOne", items);
    s->setValue("digiOneSelected", ui->digiOneComboBox->currentText());
    items.clear();

    for (int i = 0; i < ui->digiTwoComboBox->count(); i++) {
        items << ui->digiTwoComboBox->itemText(i);
    }
    items.removeDuplicates();
    s->setValue("digiTwo", items);
    s->setValue("digiTwoSelected", ui->digiTwoComboBox->currentText());
    items.clear();

    s->setValue("useKISSSocket", ui->tcpKISSRadioButton->isChecked());
    s->setValue("serialBaud", ui->modemBaudComboBox->currentText().toInt());
    s->setValue("serialPort", ui->modemSerialComboBox->currentText());
    s->setValue("serialPortTimeout", ui->serialPortTimeoutSpinBox->value());
    s->setValue("hostname", ui->direwolfIPComboBox->currentText());
    s->setValue("port", ui->direwolfPortSpinBox->value());
    s->setValue("sendIL2P", ui->il2pCheckBox->isChecked());
    s->setValue("txText", ui->plainTextEdit_2->toPlainText());
    s->setValue("geometry", this->saveGeometry());
    s->setValue("windowState", this->saveState());
}

void DirewolfConnect::connectToModem()
{
    timeoutTimer->setInterval(i_serialPortTimeout);
    const QByteArray tnccmd = UIKISSUtils::kissWrapCommand("TNC:", 6);
    if (b_useKISSSocket) {
        if (dws) {
            // remove existing serial connection
            dws->close();
            disconnect(dws, 0, 0, 0);
            dws = nullptr;
        }
        hostname = ui->direwolfIPComboBox->currentText().trimmed();
        port = ui->direwolfPortSpinBox->value();
        //qDebug() << "Connect on:" << hostname << port;
        if (dw) {
            dw->disconnectFromHost();
            disconnect(dw, 0, 0, 0); // avoids the disconnected signal when changing hosts
        }
        else
            dw = new QTcpSocket(this);
        ui->plainTextEdit->appendPlainText("Connecting to KISS Host: " % hostname % ":" % QString::number(port));
        dw->connectToHost(hostname, port);
        connect(dw, &QTcpSocket::readyRead, this, &DirewolfConnect::on_readyRead);
        connect(dw, &QTcpSocket::connected, this, [=] {
            ui->plainTextEdit->appendPlainText("Connected to KISS Host: " % hostname % ":" % QString::number(port));
            dw->write(tnccmd);
            dw->flush();
        });
        connect(dw, &QTcpSocket::disconnected, this, [=] {
            ui->plainTextEdit->appendPlainText("Disconnected from KISS Host: " % hostname % ":" % QString::number(port));
            if (dw) // will recover a short restart of Direwolf (< 30 sec)
                dw->connectToHost(hostname, port);
        });
    }
    else {
        if (dw) {
            // remove existing socket connection
            dw->disconnectFromHost();
            disconnect(dw, 0, 0, 0);
            dw = nullptr;
        }
        serialPort = ui->modemSerialComboBox->currentText();
        if (!dws)
            dws = new QSerialPort(this);
        else
            dws->close();
        ui->plainTextEdit->appendPlainText("Connecting to serial port: " % serialPort);
        dws->setPortName(serialPort);
        dws->setBaudRate(i_baudRate);
        if (dws->open(QSerialPort::ReadWrite)) {
            ui->plainTextEdit->appendPlainText("Connected to serial port " % serialPort);
            connect(dws, &QSerialPort::readyRead, this, &DirewolfConnect::on_readyRead);
            //            dws->write(QByteArray::fromHex("c00Bc0")); // prompt a banner from Nino TNC
            //            dws->flush();
        }
    }
}

void DirewolfConnect::on_readyRead()
{
    timeoutTimer->stop();
    if (b_useKISSSocket)
        inBytes.append(dw->readAll());
    else // serial is dwsobject
        inBytes.append(dws->readAll());
    timeoutTimer->start();
    //ui->plainTextEdit->appendPlainText("Read From Port..." + inBytes);
}

void DirewolfConnect::processIncomingData()
{
    timeoutTimer->stop(); // restarted by the kiss port sending more data
    QString frame = UIKISSUtils::kissUnwrap(inBytes).toHex();
    inBytes.clear(); // ready the input buffer for the next frame
    //qDebug() << "FRAME:" << frame;
    qDebug()<<"unwrapUIFrame:"<<UIKISSUtils::unwrapUIFrame(QByteArray::fromHex(frame.toLatin1()));
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Hey, we might have multiple frames in there!
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QStringList parts = frame.contains("03f0")?frame.split("03f0"):frame.split("13f0");
    qDebug() << "Parts:" << parts.size() << parts;
    //<< parts;
    if (parts.size() < 2) { // not an AX.25 frame so eval the whole blob after the "00" Data block byte from KISS
        QString fodder;
        if (frame.contains("0000000000000000")) // just so the nulls don't blank out the rest on print
            fodder = frame.replace("0000000000000000", "30303030303030303030303030303030");
        else
            fodder = QByteArray::fromHex(frame.mid(2).toLatin1());
        qDebug() << "fodder:" << fodder; // might contain binary on the end as the payload!

        const QStringList pieces = fodder.split("/");

        if (pieces.size() > 5) // scan for rIoT commands which may also require a response
        {
            qDebug()<<"rIoT command processing...";
            const QString dest = pieces.at(0);
            const QString source = pieces.at(2);
            addDestCallsign(source);                                      // add the incoming source call signs to our dest list
            bool forMe = ui->sourceCallSignComboBox->findText(dest) > -1; // this call sign is in my source list
            if (!forMe) {
                qDebug()<<"Not for me!";
                //ui->plainTextEdit->appendPlainText("This message doesn't seem to be for me..." + source);
                return;
            }
            const int func = pieces.at(1).toInt();
            const int callback = pieces.at(3).toInt();
            const int idx = fodder.indexOf(":"); // ":" colon is the rIoT delimiter between frame header and payload
            const int qos = pieces.at(4).toInt();
            //const bool retain = pieces.at(5) == "1"; // not dealing with retain yet
            QString id = "99";
            if (pieces.size() == 7) {
                id = pieces.at(6);
            }
            if (qos == 1) { // doesn't matter what the function call is, we are just telling source we got this frame
                QTimer::singleShot(500, this, [=] {
                    sendMessage(source.toLatin1() + "/1/" + dest.toLatin1() + "/0/0/0/" + id.toLatin1());
                });
            }
            // tests for cmd/response receptions
            qDebug() << "Function:" << func;
            switch (func) {
            case 3:
                // this is p2p chat text.  only prints when sent to ME
                ui->plainTextEdit->appendPlainText(source % " says: " % QString(printClean(fodder.mid(idx).toLatin1())));
                break;
            case 4:
                // this is group chat text.  only prints when sent to the group from dest
                ui->plainTextEdit->appendPlainText(source % " says: " % QString(printClean(fodder.mid(idx).toLatin1())));
                break;
            case 5:
                // this is a PING request to me
                qDebug() << "Function 5 received!";
                ui->plainTextEdit->appendPlainText("PING Request from : " + source);
                // reply to a PING request as a 6 (PONG) "dest/6/source/0/0/0"
                QTimer::singleShot(1500, this, [=] {
                    sendMessage(source.toLatin1() + "/6/" + dest.toLatin1() + "/" + QString::number(callback).toLatin1() + "/0/0");
                });
                break;

            case 6:
                //qDebug() << "Function 6 received!";
                // reply to a PING request as a 6 (PONG) "dest/6/source/0/0/0" has been received so print it
                ui->plainTextEdit->appendPlainText("PONG From : " + source);
                break;
            default:
                qDebug() << "FUNCTION SWITCH DEFAULT REACHED";
                ui->plainTextEdit->appendPlainText("Unknown Function");
                ui->plainTextEdit->appendPlainText(QByteArray::fromHex(frame.mid(2).toLatin1()));
            }
        }
        else {
            // not sure what this is so just print it
            ui->plainTextEdit->appendPlainText(fodder);
        }
    }
    else {
        ui->plainTextEdit->appendPlainText(""); // clear color
        ui->plainTextEdit->appendHtml(BLUE % now.toString(timestamp) % parseAddresses(QByteArray::fromHex(parts.at(0).toLatin1()))
                                      % END_HTML);
        ui->plainTextEdit->appendPlainText(""); // clear color
        // print the data payload part
        ui->plainTextEdit->appendPlainText(printClean(QByteArray::fromHex(parts.at(1).toLatin1())));
    }

    //    }
    //    else if (dws) {
    //        QDateTime now = QDateTime::currentDateTimeUtc();
    //        const QByteArray in = inBytes; // KISS comes in fully formed from Direwolf
    //        inBytes.clear();
    //        qDebug() << "KISS data in:" << in;
    //        //qDebug() << "UNWRAPPED:" << UIKISSUtils::kissUnwrap(in).toHex();
    //        QString frame = UIKISSUtils::kissUnwrap(in).toHex();
    //        ui->plainTextEdit->appendPlainText("Data:" + printClean(frame.toLatin1()));
    //        QStringList parts = frame.split("03f0");
    //        qDebug() << "Parts:" << parts.size() << parts;
    //        if (parts.size() < 2) { // not an AX.25 frame
    //            //            ui->plainTextEdit->appendHtml(BLUE % now.toString(timestamp)
    //            //                                          % parseAddresses(QByteArray::fromHex(parts.at(0).toLatin1())) % END_HTML);
    //            ui->plainTextEdit->appendPlainText(printClean(QByteArray::fromHex(frame.mid(2).toLatin1())));
    //        }
    //        else {
    //            ui->plainTextEdit->appendHtml(BLUE % now.toString(timestamp)
    //                                          % parseAddresses(QByteArray::fromHex(parts.at(0).toLatin1())) % END_HTML);
    //            //ui->plainTextEdit->appendPlainText(""); // clear color
    //            ui->plainTextEdit->appendPlainText(printClean(QByteArray::fromHex(parts.at(1).toLatin1())));
    //        }
    //    }
    ui->plainTextEdit->moveCursor(QTextCursor::End);
}

QByteArray DirewolfConnect::parseAddresses(const QByteArray in)
{
    QByteArray out;
    for (int i = 1; i < in.length(); i++) { // skip first byte, usually null or other cmd byte
        //qDebug() << in.at(i) << (((uchar) in.at(i)) >> 1);
        if (i % 7 == 0) {
            // evaluate the SSID nibble in the middle 00011110
            uchar ssid = ((uchar) in.at(i) & 30) >> 1;
            out.append("-").append(QString::number(ssid).toLatin1()).append("|");
        }
        else
            out.append(((uchar) in.at(i)) >> 1);
    }
    qDebug() << "Addresses OUT:" << out;
    return out;
}

QByteArray DirewolfConnect::printClean(QByteArray in)
{
    QByteArray out;
    for (int i = 0; i < in.length(); i++) {
        if (QString(in.at(i)).contains(QRegularExpression("[^ -~\r\n]")))
            out.append("?");
        else
            out.append(in.at(i));
    }
    return out;
}

QString DirewolfConnect::do_textSubstitutions(const QString msgtext)
{
    QString out = msgtext;
    if (out.contains("{POS}")) {
        out.replace("{POS}", s_myPosition);
    }
    if (out.contains("{APRSPOS}")) {
        out.replace("{APRSPOS}", s_aprsLat + "/" + s_aprsLon);
    }
    if (out.contains("{CS}")) {
        out.replace("{CS}", ui->sourceCallSignComboBox->currentText().trimmed().toUpper());
    }
    return out;
}

void DirewolfConnect::on_sendNowButton_clicked()
{
    QByteArray msgtext;
    const QString toSend = ui->plainTextEdit_2->textCursor().selectedText().toLatin1();
    if (toSend.isEmpty()) { // there was no selected text!!
        //    QDateTime now = QDateTime::currentDateTimeUtc();
        msgtext = do_textSubstitutions(ui->plainTextEdit_2->toPlainText().trimmed()).toLatin1();
    }
    else {
        msgtext = do_textSubstitutions(toSend).toLatin1();
    }    
    sendMessage(msgtext);    
}

void DirewolfConnect::addSourceCallsign(const QString source)
{
    //qDebug() << "Add source call sign" << source;
    if (source.isEmpty())
        return;
    QStringList vals = s->value("sourceCallSign", "").toStringList();
    if (vals.contains(source))
        return;
    vals << source;
    vals.removeDuplicates();
    ui->sourceCallSignComboBox->addItem(source, source);
    s->setValue("sourceCallSign", vals);
}

void DirewolfConnect::addDestCallsign(const QString dest)
{
    //qDebug() << "Add dest call sign" << dest;
    if (dest.isEmpty())
        return;
    QStringList vals = s->value("destCallSign", "").toStringList();
    if (vals.contains(dest))
        return;
    vals << dest;
    vals.removeDuplicates();
    ui->destCallSignComboBox->addItem(dest, dest);
    s->setValue("destCallSign", vals);
}

void DirewolfConnect::sendMessage(const QByteArray msgtext)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QByteArray out;
    const QString source = ui->sourceCallSignComboBox->currentText().trimmed().toUpper();
    const QString dest = ui->destCallSignComboBox->currentText().trimmed().toUpper();
    addSourceCallsign(source);
    addDestCallsign(dest);
    if (!b_sendIL2P) { // using ax.25 framing
        //msgtext.prepend(dest % " DE " % source % "~");
        if (dest.isEmpty())
            return;
        if (source.isEmpty())
            return;
        const QString digi1 = ui->digiOneComboBox->currentText().trimmed().toUpper();
        const QString digi2 = ui->digiTwoComboBox->currentText().trimmed().toUpper();
        out = UIKISSUtils::buildUIFrame(dest, source, digi1, digi2, msgtext.mid(0, 256), b_useReplyBit);
        qDebug()<<"UI frame:"<<b_useReplyBit<<out.toHex();
    }
    else if(b_useSAME) {
        out = QByteArray::fromHex("ABABABABABABABABABABABABABABAB");
        out.append(msgtext);
    }
    else { // no ax.25 framing
        // IL2P frame data size max is 1023, but in Robust mode only 512
        // THIS IS BRUTE FORCE and should be managed better
        out = msgtext.mid(0, 512);
    }
    ui->plainTextEdit->appendHtml(GREEN % now.toString(timestamp) % "To: " % dest % " - " % QString(printClean(msgtext)) % END_HTML);
    if (b_useKISSSocket) {
        //qDebug().noquote() << "send:" << out.toHex() << UIKISSUtils::kissWrap(out).toHex();
        if (dw) {
            dw->write(UIKISSUtils::kissWrap(out));
            dw->flush();
        }
    }
    else if (dws) {
        dws->write(UIKISSUtils::kissWrap(out));
        dws->flush();
    }
    //qDebug()<<"UI frame:"<<out.toHex();
}

void DirewolfConnect::on_il2pCheckBox_clicked(bool checked)
{
    b_sendIL2P = checked;
}

void DirewolfConnect::on_connectButton_clicked()
{
    connectToModem();
}

void DirewolfConnect::on_clearTextButton_clicked()
{
    ui->plainTextEdit->clear();
}

void DirewolfConnect::on_serialPortTimeoutSpinBox_textChanged(const QString &arg1)
{
    QSettings s("DirewolfConnect.ini", QSettings::IniFormat);
    s.setValue("serialPortTimeout", arg1.toInt());
    i_serialPortTimeout = arg1.toInt();
}

void DirewolfConnect::on_saveSettingsButton_clicked()
{
    saveSettings();
    if (!b_closingDown) {
        loadSettings();
        //on_action_Preferences_triggered(false);
        connectToModem();
    }
}

void DirewolfConnect::on_actionSend_Numbered_Group_triggered()
{
    bool ok;
    iterationBytes = ui->plainTextEdit_2->toPlainText().toLatin1();
    iteratorMax = QInputDialog::getInt(this,
                                       "Send text multiple times",
                                       "Choose how many times to send the text with a pause between each send.",
                                       10,
                                       2,
                                       100,
                                       1,
                                       &ok);
    if (ok) {
        iteratorMax--; // we'll start at zero!
        //const float secs = getTransmissionTime(i_otaBaudRate, iterationBytes.length() + 4);
        //(0.0286 * (iterationBytes.length() + 4)) + 0.907 + 2;
        disconnect(sendTimer, 0, 0, 0);
        connect(sendTimer, &QTimer::timeout, this, &DirewolfConnect::sendIteration);
        //sendTimer->start(secs * 1000); // will kick sendIteration();
        sendTimer->start(s->value("numberedWait", 2000).toInt());        // any reasonable delay will work
        sendIteration();               // crank the first one now
    }
}

void DirewolfConnect::sendIteration()
{
    QString iter = QString::number(iterator);
    if(iter.length() < 2) { iter.prepend("0"); }
    const QByteArray sendData = iterationBytes + " - " + iter.toLatin1();
    ui->plainTextEdit_2->setPlainText(sendData);
    on_sendNowButton_clicked();
    iterator++;
    if (iterator > iteratorMax) {
        sendTimer->stop();
        iterator = 0;
        iteratorMax = 0;
        ui->plainTextEdit_2->setPlainText(iterationBytes);
    }
}

void DirewolfConnect::on_actionTXDELAY_triggered()
{
    bool ok = false;
    int val
        = QInputDialog::getInt(this, "Set TX Delay Value", "Choose the value for TX Delay in milliseconds.", 300, 10, 500, 10, &ok);
    if (ok) {
        val /= 10;
        const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(val, 16).toLatin1()), 1);
        qDebug() << "TXDELAY:" << cmd;
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionP_Persistence_triggered()
{
    bool ok = false;
    const int val
        = QInputDialog::getInt(this, "Set Persistence Value", "Choose the value for Persistence (def. 63).", 63, 0, 255, 1, &ok);
    if (ok) {
        const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(val, 16).toLatin1()), 2);
        qDebug() << "Command:" << cmd;
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionSlot_Time_triggered()
{
    bool ok = false;
    const int val = QInputDialog::getInt(this,
                                         "Set Slot Time Value",
                                         "Choose the value for Slot Time in milliseconds (def. 10).",
                                         10,
                                         10,
                                         500,
                                         10,
                                         &ok);
    if (ok) {
        const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(val / 10).toLatin1()), 3);
        //qDebug() << "Command:" << cmd;
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionTX_Tail_triggered()
{
    bool ok = false;
    const int val = QInputDialog::getInt(this,
                                         "Set TX Tail Value",
                                         "Choose the value for TX Tail in milliseconds. (def. 0)",
                                         40,
                                         0,
                                         500,
                                         10,
                                         &ok);
    if (ok) {
        const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(val / 10, 16).toLatin1()), 4);
        qDebug() << "TXTAIL:" << cmd;
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionFull_Duplex_triggered()
{
    const bool val = QMessageBox::question(this, "Toggle Full Duplex", "Set Full Duplex to On? (def. No)") == QMessageBox::Yes;
    const int i_val = val ? 1 : 0;
    const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(i_val).toLatin1()), 5);
    //qDebug() << "Command:" << cmd;
    if (dw) {
        dw->write(cmd);
        dw->flush();
    }
    else if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionSet_Hardware_triggered()
{
    bool ok = false;
//    const QString val = QInputDialog::getText(this,
//                                              "Set Custom Hardware Value",
//                                              "Enter the value for Custom Hardware setting. (def. 0)",
//                                              QLineEdit::Normal,
//                                              "",
//                                              &ok);
    const int val
        = QInputDialog::getInt(this, "Set TX Delay Value", "Set the HW command integer.", 0, 0, 255, 1, &ok);
    if (ok) {
        //const QByteArray cmd = UIKISSUtils::kissWrapCommand(val.toLatin1(), 6);
        const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex(QString::number(val, 16).toLatin1()), 6);
        qDebug() << "Command:" << cmd.toHex();
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionExit_KISS_Mode_triggered()
{
    const bool val = QMessageBox::question(this,
                                           "Exit KISS Mode",
                                           "Command the device to exit KISS Mode? (Hardware TNC Devices Only)")
                     == QMessageBox::Yes;
    if (val) {
        const QByteArray cmd = UIKISSUtils::kissWrapCommand("", 255);
        if (dw) {
            dw->write(cmd);
            dw->flush();
        }
        else if (dws) {
            dws->write(cmd);
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionNino_TNC_GETALL_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand("", 11);
    //ui->plainTextEdit->appendPlainText("Send GETALL Command: " + cmd.toHex(':'));
    if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionSend_Text_Line_By_Line_triggered()
{
    lineByLines = ui->plainTextEdit_2->toPlainText().split("\n");
    iterator = 0;
    iteratorMax = lineByLines.size() - 1; // we start at index zero
    disconnect(sendTimer, 0, 0, 0);
    connect(sendTimer, &QTimer::timeout, this, &DirewolfConnect::sendLineByLine);
    sendLineByLine();
}

void DirewolfConnect::sendLineByLine()
{
    const QString data = lineByLines.at(iterator);
    //const QByteArray sendData = data.toLatin1();
    ui->plainTextEdit_2->setPlainText(data);
    // how long will it take? plus 1 second between
    const float secs = getTransmissionTime(i_otaBaudRate/2, data.length()) + 1.5;
    // send it
    on_sendNowButton_clicked();
    iterator++;
    // check if we're done
    if (iterator > iteratorMax) {
        sendTimer->stop();
        iterator = 0;
        iteratorMax = 0;
        ui->plainTextEdit_2->setPlainText(lineByLines.join("\n"));
    }
    else {
        //qDebug() << "Next iteration:" << secs<<(qRound(secs) * 1000);
        sendTimer->start(qRound(secs)*1000); // any reasonable delay is fine, input buffer it 8192 bytes
    }
}

float DirewolfConnect::getTransmissionTime(const int baud, int byteCount)
{
//qDebug()<<"baud:"<<baud<<"byteCount:"<<byteCount;
    return qRound((float)((byteCount * 10)/baud));
}

void DirewolfConnect::on_actionSend_Numbere_triggered()
{
    QMessageBox::information(this,
                             "What is Sending Numbered Transmissions?",
                             "The menu item Actions/Send Numbered Transmissions will send the entire text of the send text area "
                             "multiple times and adds an incrementing numeral to the end of each transmission.");
}

void DirewolfConnect::on_actionSend_Line_By_Line_triggered()
{
    QMessageBox::information(this,
                             "What is Send Line By Line?",
                             "The menu item Actions/Send Line By Line will send each line in the send text area in order as an "
                             "individual frame for each line.");
}

void DirewolfConnect::on_actionNo_AX_25_raw_triggered()
{
    QMessageBox::information(this,
                             "What is No AX.25 (raw)?",
                             "The checkbox \"No AX.25 (raw)\" will send the text in the send text area without any AX.25 framing, "
                             "so a raw data transmission.");
}

void DirewolfConnect::on_actionDirewolf_TNC_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand("TNC:", 6);
    ui->plainTextEdit->appendPlainText("Send Direwolf TNC: Command: " + cmd.toHex(':'));
    if (dw) {
        dw->write(cmd);
        dw->flush();
    }
    else if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionSubstitution_Text_triggered()
{
    QMessageBox::information(this,
                             "Substitution Text Usage",
                             "When typing text into the send text area,\nany text found with the following values\nwill be "
                             "substituted as follows:\n\n{POS} = position stored with Actions/My Location menu item.\n\n"
                             "{APRSPOS} = APRS position previously calculated with the Maidenhead utility.\n\n"
                             "{CS} = currently displayed My Call Sign value\n\nExample APRS frame: !{POS}#HF Test Station {CS}");
}

void DirewolfConnect::on_actionSet_OTA_Baud_Rate_triggered()
{
    bool ok = false;
    const QString choice = QInputDialog::getItem(this,
                                                 "Choose OTA Baud Rate",
                                                 "Choose the Over-the-Air baud rate to use for time calculations.",
                                                 QStringList() << "75"
                                                               << "150"
                                                               << "300"
                                                               << "600",
                                                 0,
                                                 false,
                                                 &ok);
    if (ok) {
        i_otaBaudRate = choice.toInt();
        s->setValue("otaBaudRate", i_otaBaudRate);
        //ui->sendNowButton->setText("Send Now\n" % QString::number(i_otaBaudRate) % " baud");
    }
}

void DirewolfConnect::on_killTxButton_clicked()
{
    qDebug() << "User wants to stop transmitting!";
    if (b_useKISSSocket) {
        return;
    }
    else { // serial NinO TNC c0 09 00 c0 in KISS speak
        if (dws) {
            dws->write(QByteArray::fromHex("C00900C0"));
            dws->flush();
        }
    }
}

void DirewolfConnect::on_actionE_xit_triggered()
{
    close();
}

void DirewolfConnect::on_action_Preferences_triggered(bool checked)
{
    ui->groupBox_3->setVisible(checked);
}

void DirewolfConnect::on_pingButton_clicked()
{
    ui->plainTextEdit->appendHtml(GREEN % "Sending PING to " % ui->destCallSignComboBox->currentText() % END_HTML);
    ui->plainTextEdit->appendPlainText("");
    ui->plainTextEdit->moveCursor(QTextCursor::End);
    sendMessage(ui->destCallSignComboBox->currentText().trimmed().toUpper().toLatin1() + "/5/"
                + ui->sourceCallSignComboBox->currentText().trimmed().toUpper().toLatin1() + "/0/0/0");
}

void DirewolfConnect::on_actionTEST_triggered()
{
    inBytes = UIKISSUtils::kissWrap("AB4MW/3/AB4MW-T/0/1/0/7:Hello World"); // qos 1 should send an ACK frame for id = 7
    processIncomingData();
    //    inBytes = UIKISSUtils::kissWrap("NNX4XA/6/NNX3XA/0/0/0");
    //    processIncomingData();
}

void DirewolfConnect::on_myCallsignButton_clicked()
{
    if (QMessageBox::question(this,
                              "Remove Source Callsign?",
                              "Do you want to remove the selected call sign from the My Callsign list?")
        == QMessageBox::Yes) {
        ui->sourceCallSignComboBox->removeItem(ui->sourceCallSignComboBox->currentIndex());
    }
}

void DirewolfConnect::on_destStationButton_clicked()
{
    if (QMessageBox::question(this,
                              "Remove Destination Callsign?",
                              "Do you want to remove the selected call sign from the Dest. Station list?")
        == QMessageBox::Yes) {
        ui->destCallSignComboBox->removeItem(ui->destCallSignComboBox->currentIndex());
    }
}

void DirewolfConnect::on_actionSet_Nino_TNC_Serial_triggered()
{
    if (QMessageBox::question(
            this,
            "Set Serial Number",
            "If the serial number is currently all zero's (0x00)\nyou may set it to a value of letters and/or digits.\n\nProceed?")
        == QMessageBox::Yes) {
        bool ok = false;
        QString val = QInputDialog::getText(this,
                                            "Set Serial Number",
                                            "Enter exactly 8 characters (0-9 and A-Z) to set a new serial number.",
                                            QLineEdit::Normal,
                                            "",
                                            &ok);
        if (ok) {
            if (val.length() != 8) {
                QMessageBox::warning(this, "Error", "Serial number must be exactly 8 characters");
                return;
            }
            if (dws) {
                val = val.toUpper();
                const QByteArray cmd = UIKISSUtils::kissWrapCommand(val.toLatin1(), 10);
                qDebug().noquote() << cmd.toHex();
                dws->write(cmd);
                dws->flush();
                on_actionGet_Nino_TNC_Serial_triggered();
            }
        }
    }
}

void DirewolfConnect::on_actionGet_Nino_TNC_Firmware_Version_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand("", 8);
    if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionGo_to_1200_bd_AX_25_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex("02"), 7);
    if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionTest_ACKMODE_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand(QByteArray::fromHex("4142434445"), 12);
    if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionGet_Nino_TNC_Serial_triggered()
{
    const QByteArray cmd = UIKISSUtils::kissWrapCommand("", 14);
    qDebug() << "Get Serial:" << cmd;
    if (dws) {
        dws->write(cmd);
        dws->flush();
    }
}

void DirewolfConnect::on_actionSet_My_Position_triggered()
{
    QString lat, lon;
    if (!s_myPosition.contains("/")) {
        lat.clear();
        lon.clear();
    }
    else {
        lat = s_myPosition.split("/").at(0);
        lon = s_myPosition.split("/").at(1);
    }
    //qDebug() << "Lat:" << lat << "Lon:" << lon;
    QStringList position;
    bool ok = false;
    const QString latitude
        = QInputDialog::getText(this,
                                "Position Latitude",
                                "Enter the Latitude information in the following format:\n\nddmm.mmD\n\nwhere dd = "
                                "degrees, mm.mm = minutes and D = direction (N or S)",
                                QLineEdit::Normal,
                                lat,
                                &ok);
    if (ok) {
        position << latitude;
    }
    else {
        return;
    }
    ok = false;
    const QString longitude
        = QInputDialog::getText(this,
                                "Position Longitude",
                                "Enter the Longitude information in the following format:\n\ndddmm.mmD\n\nwhere dd = "
                                "degrees, mm.mm = minutes and D = direction (W or E)",
                                QLineEdit::Normal,
                                lon,
                                &ok);
    if (ok) {
        position << longitude;
        ui->plainTextEdit->appendPlainText("Position: " % position.join("/"));
        s->setValue("myPosition", position);
        s_myPosition = position.join("/");
        int idx = windowTitle().indexOf("Position: ") + 10;
        if (idx == 9) {
            setWindowTitle(windowTitle() % " Position: " % s_myPosition);
        }
        else {
            setWindowTitle(windowTitle().mid(0, idx) % s_myPosition);
        }
    }    
    //qDebug() << "s_myPosition:" << s_myPosition;
}

void DirewolfConnect::on_tcpKISSRadioButton_clicked()
{
    b_useKISSSocket = true;
    ui->direwolfIPComboBox->setEnabled(true);
    ui->direwolfPortSpinBox->setEnabled(true);
    ui->modemSerialComboBox->setEnabled(false);
    ui->modemBaudComboBox->setEnabled(false);
}

void DirewolfConnect::on_serialKISSRadioButton_clicked()
{
    b_useKISSSocket = false;
    ui->direwolfIPComboBox->setEnabled(false);
    ui->direwolfPortSpinBox->setEnabled(false);
    ui->modemSerialComboBox->setEnabled(true);
    ui->modemBaudComboBox->setEnabled(true);
}

void DirewolfConnect::on_actionEnter_KISS_Mode_triggered()
{
    if (dw) {
            dw->write("@K\r");
            dw->flush();
        }
    else if (dws) {
        dws->write("@K\r");
        dws->flush();
    }
}

void DirewolfConnect::on_useSAMECheckBox_stateChanged(int arg1)
{
    b_useSAME = arg1;
}

void DirewolfConnect::on_actionSet_My_Position_Maidenhead_triggered()
{
    QStringList position;
    bool ok = false;
    const QString maidenhead
        = QInputDialog::getText(this,
                                "Position from Maidenhead",
                                "Enter the Maidenhead locator (4 or 6 char)",
                                QLineEdit::Normal,
                                "",
                                &ok);
    if (ok) {
        std::pair<double, double> stdloc(mh2ll(maidenhead.toStdString()));
        QPair<double, double> loc = qMakePair(stdloc.first, stdloc.second);
        //qDebug()<< maidenhead<<loc;
        setWindowTitle(windowTitle().append(maidenhead));
        std::pair<double, double> stdaprs(ll2aprs(stdloc));
        QPair<double, double> aprs = qMakePair(stdaprs.first, stdaprs.second);
        s_aprsLat = QString::number(aprs.first, 'f', 2);
        if(aprs.first > 0) {
            s_aprsLat += "N";
        }
        else {
            s_aprsLat += "S";
        }
        s_aprsLon = QString::number(aprs.second, 'f', 2);
        if(aprs.second < 0) {
            s_aprsLon[0] = '0';
            s_aprsLon.append('W');
        }
        else {
            s_aprsLon.append('E');
        }
        while(s_aprsLon.length() < 9) {
                s_aprsLon.prepend('0');
            }
        //qDebug()<<"APRS:"<<s_aprsLat<<"/"<<s_aprsLon;
        statusBar()->showMessage("APRS: " + s_aprsLat + "/" + s_aprsLon);
    }
}

void DirewolfConnect::on_actionReset_MMDVM_Pi_GPIO_triggered()
{
    if(line) {
        // Set pin 40 low, then high to reset the MMDVM Pi board;
        gpiod_line_set_value(line, 0);
        QTimer::singleShot(250, this, [=]{gpiod_line_set_value(line, 1);});
    }
    // then reconnect to the serial port since the connection would have
    // been dropped by the board restarting.  this can be done without
    // harm even if the test above fails.
    QTimer::singleShot(6000, this, [=]{on_connectButton_clicked();});
}

void DirewolfConnect::on_actionSet_Reply_Bit_in_UI_Frames_triggered(bool checked)
{
    b_useReplyBit = checked;
}

