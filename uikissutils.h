#ifndef UIKISSUTILS_H
#define UIKISSUTILS_H

#include <QByteArray>
#include <QObject>

class UIKISSUtils : public QObject
{
    Q_OBJECT
public:
    explicit UIKISSUtils(QObject *parent = nullptr);
    /**
     * Convenience variables holding the value "UICHAT" and the chosen SSID
     * character NOT CURRENTLY USED but destined for a round-table chat application
     */
    const QByteArray UICHAT0 = QByteArray::fromHex(QString("aa92869082a8e0").toLatin1());
    const QByteArray UICHAT1 = QByteArray::fromHex(QString("aa92869082a8e2").toLatin1());
    const QByteArray UICHAT2 = QByteArray::fromHex(QString("aa92869082a8e4").toLatin1());
    const QByteArray UICHAT3 = QByteArray::fromHex(QString("aa92869082a8e6").toLatin1());
    const QByteArray UICHAT4 = QByteArray::fromHex(QString("aa92869082a8e8").toLatin1());
    const QByteArray UICHAT5 = QByteArray::fromHex(QString("aa92869082a8ea").toLatin1());
    const QByteArray UICHAT6 = QByteArray::fromHex(QString("aa92869082a8ec").toLatin1());
    const QByteArray UICHAT7 = QByteArray::fromHex(QString("aa92869082a8ee").toLatin1());
    const QByteArray UICHAT8 = QByteArray::fromHex(QString("aa92869082a8f0").toLatin1());
    const QByteArray UICHAT9 = QByteArray::fromHex(QString("aa92869082a8f2").toLatin1());
    const QByteArray UICHAT10 = QByteArray::fromHex(QString("aa92869082a8f4").toLatin1());
    const QByteArray UICHAT11 = QByteArray::fromHex(QString("aa92869082a8f6").toLatin1());
    const QByteArray UICHAT12 = QByteArray::fromHex(QString("aa92869082a8f8").toLatin1());
    const QByteArray UICHAT13 = QByteArray::fromHex(QString("aa92869082a8fa").toLatin1());
    const QByteArray UICHAT14 = QByteArray::fromHex(QString("aa92869082a8fc").toLatin1());
    const QByteArray UICHAT15 = QByteArray::fromHex(QString("aa92869082a8fe").toLatin1());
    static QByteArray kissWrap(const QByteArray in);
    static QByteArray kissWrapCommand(const QByteArray val, const int cmdCode);
    static QByteArray kissUnwrap(const QByteArray in);
    static QByteArray buildUIFrame(
        QString dest_call, QString source_call, QString digi1 = QString(), QString digi2 = QString(), QString text = QString());
    static QByteArrayList unwrapUIFrame(QByteArray in);
signals:
};

#endif // UIKISSUTILS_H
