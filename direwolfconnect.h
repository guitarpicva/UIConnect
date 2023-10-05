#ifndef DIREWOLFCONNECT_H
#define DIREWOLFCONNECT_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>

QT_BEGIN_NAMESPACE
    namespace Ui { class DirewolfConnect; }
QT_END_NAMESPACE

    class DirewolfConnect : public QMainWindow
{
    Q_OBJECT

public:
    DirewolfConnect(QWidget *parent = nullptr);
    ~DirewolfConnect();
    void closeEvent(QCloseEvent *event);
private slots:
    void connectToModem();
    void on_readyRead();
    void on_sendNowButton_clicked();
    void on_il2pCheckBox_clicked(bool checked);
    void on_connectButton_clicked();
    void on_clearTextButton_clicked();
    void on_serialPortTimeoutSpinBox_textChanged(const QString &arg1);
    void processIncomingData();
    void on_saveSettingsButton_clicked();
    void on_actionSend_Numbered_Group_triggered();
    void sendIteration();
    void on_actionTXDELAY_triggered();
    void on_actionP_Persistence_triggered();
    void on_actionSlot_Time_triggered();
    void on_actionTX_Tail_triggered();
    void on_actionFull_Duplex_triggered();
    void on_actionSet_Hardware_triggered();
    void on_actionExit_KISS_Mode_triggered();
    void on_actionNino_TNC_GETALL_triggered();
    void on_actionSend_Text_Line_By_Line_triggered();
    void sendLineByLine();
    void on_actionSend_Numbere_triggered();
    void on_actionSend_Line_By_Line_triggered();
    void on_actionNo_AX_25_raw_triggered();
    void on_actionDirewolf_TNC_triggered();
    void on_actionSubstitution_Text_triggered();
    void on_actionSet_OTA_Baud_Rate_triggered();
    void on_killTxButton_clicked();
    void sendMessage(const QByteArray msgtext);
    void on_actionE_xit_triggered();
    void on_action_Preferences_triggered(bool checked);
    void on_pingButton_clicked();
    void on_actionTEST_triggered();
    void on_myCallsignButton_clicked();
    void on_destStationButton_clicked();
    void on_actionSet_Nino_TNC_Serial_triggered();
    void on_actionGet_Nino_TNC_Firmware_Version_triggered();
    void on_actionGo_to_1200_bd_AX_25_triggered();
    void on_actionTest_ACKMODE_triggered();
    void on_actionGet_Nino_TNC_Serial_triggered();
    void on_actionSet_My_Position_triggered();

    void on_tcpKISSRadioButton_clicked();

    void on_serialKISSRadioButton_clicked();

    void on_actionEnter_KISS_Mode_triggered();

    void on_useSAMECheckBox_stateChanged(int arg1);

    void on_actionSet_My_Position_Maidenhead_triggered();

private:
    Ui::DirewolfConnect *ui;
    QMap<int, QList<float>> timeMap;
    QString build = __DATE__;
    QTcpSocket *dw = nullptr;
    QSerialPort *dws = nullptr;
    QByteArray inBytes;
    QTimer *timeoutTimer = nullptr;
    QTimer *sendTimer = nullptr;
    QByteArray iterationBytes;
    QSettings *s = nullptr;
    const QString front = "C000";
    const QString fend = "C0";
    const QString timestamp = "[dd hh:mm:ss] ";
    QString serialPort = "COM1";
    QString hostname = "192.168.0.50";
    QStringList lineByLines;
    QString s_myPosition;
    QString s_aprsLat;
    QString s_aprsLon;
    int port = 8001;
    int i_serialPortTimeout = 50;
    int i_baudRate = 57600;
    int iterator = 0;
    int iteratorMax = 0;
    int i_otaBaudRate = 700;
    bool b_useSAME = false;
    bool b_useKISSSocket = true;
    bool b_sendIL2P = true;
    bool b_closingDown = false;
    const QString BLUE = "<p><pre style=\"color:#eee;\">";
    const QString GREEN = "<p><pre style=\"color:lightgreen;\">";
    const QString END_HTML = "</pre></p>";
    const QByteArray UICHAT = QByteArray::fromHex(QString("aa92869082a8e0").toLatin1());
    void loadSettings();
    void saveSettings();
    QByteArray parseAddresses(const QByteArray in);
    QByteArray printClean(QByteArray in);
    float getTransmissionTime(const int baud, int byteCount);
    QString do_textSubstitutions(QString msgtext);
    void addSourceCallsign(const QString source);
    void addDestCallsign(const QString dest);
};
#endif // DIREWOLFCONNECT_H
