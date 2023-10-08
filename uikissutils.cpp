#include "uikissutils.h"
#include <QDebug>
UIKISSUtils::UIKISSUtils(QObject *parent) : QObject{parent} {}
/**
     * Wrap a frame in KISS and return as QByteArray. Many thanks to John
     * Langner WB2OSZ since I based this on his C code from Direwolf v1.2.
     *
     * @param in - input array of bytes representing the AX.25 UI frame.
     * @return - QByteArray of KISS wrapped data.
     */
QByteArray UIKISSUtils::kissWrap(const QByteArray in)
{
    /**
     * The KISS "FEND" (frame end) character. One at each end of a KISS frame
     */
    const char FEND = 0xC0;
    /**
     * The KISS "FESC" (frame escape) character. Used to escape "FEND" inside a
     * KISS frame.
     */
    const char FESC = 0xDB;
    /**
     * The KISS "TFEND" character.
     */
    const char TFEND = 0xDC;
    /**
     * The KISS "TFESC" character.
     */
    const char TFESC = 0xDD;
    // John Langner's way from C in Direwolf 1.2
    // add a few indexes in case there needs to be any
    // escaped FENDs for FESCs
    // This is done the Qt way with QByteArray for
    // inclusion in Qt projects.  Avoids array arithmetic
    //int[] cout = new int[in.length + 25];
    QByteArray out;

    out.append(FEND); // the opening FRAME END
    // int olen = 0;
    //cout[olen++] = FEND;
    //for (int j = 0; j < in.length; j++) {
    for (int i = 0; i < in.length(); i++) {
        //qDebug() << "Char:" << in.at(i);
        if (in.at(i) == FEND) {
            out.append(FESC).append(TFEND);
        }
        else if (in.at(i) == FESC) {
            out.append(FESC).append(TFESC);
        }
        else {
            out.append(in.at(i));
        }
        /*
        if (in[j] == FEND) {
            cout[olen++] = FESC;
            cout[olen++] = TFEND;
        }
        else if (in[j] == FESC) {
            cout[olen++] = FESC;
            cout[olen++] = TFESC;
        }
        else {
            cout[olen++] = in[j];
        }
    */
    }
    //cout[olen++] = FEND;
    out.append(FEND);
    // control byte inserted late since it's a null
    out.insert(1, (uchar) 0x00);
    //return Arrays.copyOf(cout, olen);
    return out;
}

/**
     * Wrap a command in KISS and return as QByteArray.
     *
     * @param in - value to send with the command code.
     * @param cmdCode = the integer command code to send.
     * @return - QByteArray of KISS wrapped data.
     */
QByteArray UIKISSUtils::kissWrapCommand(const QByteArray val, const int cmdCode)
{
    /**
     * The KISS "FEND" (frame end) character. One at each end of a KISS frame
     */
    const char FEND = 0xC0;
    /**
     * The KISS "FESC" (frame escape) character. Used to escape "FEND" inside a
     * KISS frame.
     */
    const char FESC = 0xDB;
    /**
     * The KISS "TFEND" character.
     */
    const char TFEND = 0xDC;
    /**
     * The KISS "TFESC" character.
     */
    const char TFESC = 0xDD;
    // John Langner's way from C in Direwolf 1.2
    // add a few indexes in case there needs to be any
    // escaped FENDs for FESCs
    // This is done the Qt way with QByteArray for
    // inclusion in Qt projects.  Avoids array arithmetic
    //int[] cout = new int[in.length + 25];
    QByteArray out;

    out.append(FEND); // the opening FRAME END
    // int olen = 0;
    //cout[olen++] = FEND;
    //for (int j = 0; j < in.length; j++) {
    for (int i = 0; i < val.length(); i++) {
        //qDebug() << "Char:" << in.at(i);
        if (val.at(i) == FEND) {
            out.append(FESC).append(TFEND);
        }
        else if (val.at(i) == FESC) {
            out.append(FESC).append(TFESC);
        }
        else {
            out.append(val.at(i));
        }
    }
    out.append(FEND);
    // control byte inserted late since it's a null
    out.insert(1, (uchar) cmdCode); // caller is responsible for ensuring cmd code is valid.
    return out;
}

/**
     *
     * This method gets rid of the KISS frame ends and frame escapes
     * and returns the scrubbed array of characters for further processing. The
     * input is expected to contain an optional FEND, command type, and content
     * followed by a FEND. Once the KISS frame is unwrapped, it should contain
     * an AX.25 frame to be processed according to those rulesets.  Alternately
     * for KISS hosts that allow RAW text, the text will simply be ASCII chars.
     *
     * @param in - input array representation an AX.25 frame to be wrapped in KISS
     * @return - returns int[] of unwrapped AX.25 UI frame or RAW ASCII
     */
QByteArray UIKISSUtils::kissUnwrap(const QByteArray in)
{
    /**
     * The KISS "FEND" (frame end) character. One at each end of a KISS frame
     */
    const char FEND = 0xC0;
    /**
     * The KISS "FESC" (frame escape) character. Used to escape "FEND" inside a
     * KISS frame.
     */
    const char FESC = 0xDB;
    /**
     * The KISS "TFEND" character.
     */
    const char TFEND = 0xDC;
    /**
     * The KISS "TFESC" character.
     */
    const char TFESC = 0xDD;
    // John Langner's way fromn Direwolf 1.2
    int olen = 0;
    int ilen = in.length();
    int j;
    bool escaped_mode = false;

    // Output array length will be less than input array length because we
    // are removing at least two FEND's
    //int[] out = new int[ilen];
    QByteArray out = "";
    if (ilen < 2) {
        /* Need at least the "type indicator" char and FEND. */
        /* Probably more. */
        return out; // empty byte array indicates error
    }
    // If the last char is c0 (FEND), (which it properly should be) don't worry
    // about processing it later so deduct one from length of input array.
    if (in[ilen - 1] == FEND) {
        ilen--;
    }
    else {
        qDebug() << "KISS frame should end with FEND.";
        return "";
    }
    // If the opening char is c0 (FEND) then we don't need to start
    // processing there either, so only deal with the actual AX.25 frame
    // contents.
    if (in[0] == FEND) {
        j = 1;
    }
    else {
        // otherwise, sometimes we don't get that opening FEND if frames are
        // glued together, so start at zero.
        j = 0;
    }

    for (; j < ilen; j++) {
        // According to the KISS spec, no un-escaped FEND's in the middle of
        // the AX.25 frame characters.
        if (in[j] == FEND) {
            qDebug() << "KISS frame should not have FEND in the middle.";
            return "";
        }
        // Escaped mode means we found a FESC down below in the else if and
        // need to remove the following TFEND or TFESC character in the next
        // iteration of the loop above.
        if (escaped_mode) {
            if (in[j] == TFESC) {
                // Replace TFESC with FESC in output
                out[olen++] = FESC;
            }
            else if (in[j] == TFEND) {
                // Replace TFEND with FEND in output
                out[olen++] = FEND;
            }
            else {
                // If we had a FESC and it was not followed by a TFEND or a
                // TFESC, then that's an error.
                qDebug() << "KISS protocol error.  Found ";
                //System.err.printf("%02x ", in[j]);
                //System.err.println("after " + FESC);
                return "";
            }
            escaped_mode = false;
        }
        else if (in[j] == FESC) {
            // If a FESC is found, skip it but set the escape mode flag so
            // we deal with the next char properly as above explains.
            escaped_mode = true;
        }
        else {
            // Otherwise, it's a normal character so write it to the output
            // array.
            out[olen++] = in[j];
        }
    }
    if (out.startsWith("00"))
        return out.mid(1);
    return out;
}

QByteArray UIKISSUtils::buildUIFrame(QString dest_call, QString source_call, QString digi1, QString digi2, QByteArray data)
{
    /* ax.25 UI frame has 7 chars for dest, 7 chars for source, 7 chars for
    * digi, (7 chars for a second digi) one byte for frame type of UI (03), and one byte for PID (f0)
    */
    // BEFORE GOING ANY FURTHER, Check to see if we have a first digi
    bool hasNoDigi = digi1.trimmed().isEmpty(); // if digi1 is empty, digi2 is moot
    // GET THE SECOND DIGI EXISTENCE FIRST
    bool hasNoDigi2 = hasNoDigi;
    if (!hasNoDigi2) // if false, double-check the second one
        hasNoDigi2 = digi2.trimmed().isEmpty();

    QByteArray out; // outpub buffer
    // Destination call sign SSID evaluation
    int D_SSID = 0;
    QStringList parts = dest_call.split("-");
    if (parts.size() > 1) // there is an SSID
    {
        D_SSID = parts.at(1).toInt();
    }
    // if no SSID, then the whole thing is call sign in parts[0]
    QByteArray msg = parts.at(0).toLatin1().toUpper().mid(0, 6);
    // max 6 chars for Dest Call sign
    for (int i = 0; i < 6; i++) {
        if (i < dest_call.length()) {
            out.append((uchar) msg[i] << 1);
        }
        else {
            out.append((uchar) 0x40); // bit shifted space 0x20
        }
    }
    if (D_SSID == 0) {
        // Dest 0000 SSID char with Control bit set
        out.append((uchar) 0xe0);
    }
    else {
        // build the SSID into the Dest Address field
        uchar val = (uchar) D_SSID;
        //qDebug() << "OTHER SSID:" << QString::number(val, 2);
        val = val << 1; // move the SSID left one bit
        //qDebug() << "SHIFT SSID:" << QString::number(val, 2);
        val = val | 0xe0; // bit mask "11100000", last bit unset since Dest address
        //qDebug() << "MASK SSID:" << QString::number(val, 2);
        out.append(val);
    }

    // SOURCE Call sign
    D_SSID = 0;
    parts = source_call.split("-");
    if (parts.size() > 1) // there is an SSID
    {
        D_SSID = parts.at(1).toInt();
    }
    // re-use msg for source and digi
    msg = source_call.toLatin1().toUpper().mid(0, 6);
    for (int i = 0; i < 6; i++) {
        if (i < source_call.length()) {
            out.append((uchar) msg[i] << 1);
        }
        else {
            out.append((uchar) 0x40);
        }
    }

    if (D_SSID == 0) {
        if (hasNoDigi) { // this will be the final address, so set the bit
            out.append((uchar) 0x61);
        }
        else {
            // Dest 0000 SSID char with Control bit set
            out.append((uchar) 0x60);
        }
    }
    else {
        // build the SSID into the Dest Address field
        uchar val = (uchar) D_SSID;
        //qDebug() << "OTHER SSID:" << QString::number(val, 2);
        val = val << 1; // move the SSID left one bit
        //qDebug() << "SHIFT SSID:" << QString::number(val, 2);
        if (hasNoDigi) {
            val = val | 0x61; // bit mask "01100001" since no digi follows Source address
            // this will be the final address, so set the bit
        }
        else {
            val = val | 0x60; // bit mask "01100000", last bit unset since Source address
        }
        //qDebug() << "MASK SSID:" << QString::number(val, 2);
        out.append(val);
    } // END Source Call Sign processing

    // If there is a first digi, add it and it's SSID now
    if (!hasNoDigi) {
        // now add digi address
        //qDebug() << "DIGI:" << digi;
        D_SSID = 0;
        parts = digi1.split("-");
        if (parts.size() > 1) {
            D_SSID = parts.at(1).toInt();
        }
        // get the Digi call sign
        msg = parts.at(0).toLatin1().toUpper().trimmed();
        for (int i = 0; i < 6; i++) {
            if (i < msg.length()) {
                out.append((uchar) msg[i] << 1);
            }
            else {
                out.append((uchar) 0x40); // bit shifted space 0x20
            }
        }
        //qDebug() << "DIGI:" << msg << "D_SSID" << D_SSID;

        // no SSID in the address so encode 0x61
        if (D_SSID == 0) {
            if (hasNoDigi2)
                out.append((uchar) 0x61);
            else
                out.append((uchar) 0x60);
            //qDebug() << "ZERO SSID:" << QString::number(0x61, 2);
        }
        else {
            // encode SSID in bits 3-6 and LSB = 1
            // "H" bit NOT set, so 3 MSB's are 011
            // 011 SSID
            // LSB is 1 to mark final digi in list
            //System.out.println("011" + String.format("%4s", Integer.toBinaryString(D_SSID)).replace(' ', '0') + "1");
            //out[21] = Integer.parseInt("011" + String.format("%4s", Integer.toBinaryString(D_SSID)).replace(' ', '0') + "1", 2);
            uchar val = (uchar) D_SSID;
            //qDebug() << "OTHER SSID:" << QString::number(val, 2);
            val = val << 1; // move the SSID left one bit
            //qDebug() << "SHIFT SSID:" << QString::number(val, 2);
            if (hasNoDigi2)       // this is the last address so set the bit
                val = val | 0x61; // bit mask "01100001"
            else
                val = val | 0x60; // bit mask "01100000"
            //qDebug() << "MASK SSID:" << QString::number(val, 2);
            out.append(val);
            //qDebug() << "OTHER SSID:" << QString::number(val, 2);
        }
        //qDebug() << "SSID:" << QString::number(out[21]);
    } // END digi encoding
    // If there is a second digi, add it and it's SSID now
    if (!hasNoDigi2) {
        // now add digi address
        //qDebug() << "DIGI:" << digi;
        D_SSID = 0;
        parts = digi2.split("-");
        if (parts.size() > 1) {
            D_SSID = parts.at(1).toInt();
        }
        // get the Digi call sign
        msg = parts.at(0).toLatin1().toUpper().trimmed();
        for (int i = 0; i < 6; i++) {
            if (i < msg.length()) {
                out.append((uchar) msg[i] << 1);
            }
            else {
                out.append((uchar) 0x40); // bit shifted space 0x20
            }
        }
        //qDebug() << "DIGI:" << msg << "D_SSID" << D_SSID;
        // the last address field so LSB is a 1 (UIChat only allows one digi)
        // no SSID in the address so encode 0x61
        if (D_SSID == 0) {
            out.append((uchar) 0x61);
            //qDebug() << "ZERO SSID:" << QString::number(0x61, 2);
        }
        else {
            // encode SSID in bits 3-6 and LSB = 1
            // "H" bit NOT set, so 3 MSB's are 011
            // 011 SSID
            // LSB is 1 to mark final digi in list
            //System.out.println("011" + String.format("%4s", Integer.toBinaryString(D_SSID)).replace(' ', '0') + "1");
            //out[21] = Integer.parseInt("011" + String.format("%4s", Integer.toBinaryString(D_SSID)).replace(' ', '0') + "1", 2);
            uchar val = (uchar) D_SSID;
            //qDebug() << "OTHER SSID:" << QString::number(val, 2);
            val = val << 1; // move the SSID left one bit
            //qDebug() << "SHIFT SSID:" << QString::number(val, 2);
            val = val | 0x61; // bit mask "01100001"
            //qDebug() << "MASK SSID:" << QString::number(val, 2);
            out.append(val);
            //qDebug() << "OTHER SSID:" << QString::number(val, 2);
        }
        //qDebug() << "SSID:" << QString::number(out[21]);
    } // END digi encoding
    // Now encode the payload text
    // Control field
    // PID = 0x03, no layer 3
    out.append((uchar) 0x03);
    out.append((uchar) 0xf0);
    msg = data.mid(0, 256); // limit to 256 bytes
    for (int i = 0; i < msg.length(); i++) {
        out.append((uchar) msg[i]);
    }
    //qDebug() << "OUT:" << out;
    return out;
}

QByteArrayList UIKISSUtils::unwrapUIFrame(QByteArray kiss_in)
{
    qDebug()<<"kiss_in:"<<kiss_in;
    // Erase the KISS mode byte if necessary
    QByteArray in;
    if(kiss_in[0] == 0x00) {
        in = kiss_in.mid(1);
    }
    else {
        in = kiss_in;
    }
    qDebug()<<"in:"<<in;
    //                                     opt   opt
    //                    dest,src,digi1,digi2,payload
    QByteArrayList out = {"", "", "", "", ""};
    // extract the call signs and the payload and assign them as
    // [0]=dest, [1]=source, [2]=digi1, [3]=digi2, [4]=payload
    char b;
    int i = 0, end = 6;
    // DEST
    while (i < end) {
        b = (in[i] >> 1) & 0x7F;
        //printf("[%02d] %c\n", i, (int) b);
        out[0].append(b);
        i++;
    }
    b = (in[i] >> 1) & 0x0F;
    out[0].append('-');
    out[0].append(QString::number(b).toLatin1());
    //printf("[%02d] %d\n", i, b);

    // SOURCE
    i++;
    end = i + 6;
    while (i < end) {
        b = (in[i] >> 1) & 0x7F;
        out[1].append(b);
        //printf("[%02d] %c\n", i, (int) b);
        i++;
    }
    int c = in[i] & 0x01; // if there is a continuation bit
    b = (in[i] >> 1) & 0x0F;
    out[1].append('-');
    out[1].append(QString::number(b).toLatin1());
        //printf("[%02d] %d\n", i, b);
    if(c == 0x00) { // there is at least one Digi to follow
        // DIGI1
        i++;
        end = i + 6;
        while (i < end) {
            b = in[i] >> 1;
            out[2].append(b);
            //printf("[%02d] %c\n", i, (int) b);
            i++;
        }
        bool h = false; // the has been repeated bit for repeaters only
        c = in[i] & 0x01;
        //h = in[i] & 0x80; // bit 7 0 = not repeated, 1 = this station repeated it
        h = in[i] > (char)127;
        b = (in[i] >> 1) & 0x0F;
        out[2].append('-');
        out[2].append(QString::number(b).toLatin1());
        //if(h > 127) out[2].append(1,'*');
        if(h) out[2].append('*');
        //printf("[%02d] %d\n", i, b);
        if(c == 0x00) { // there is a second digi to follow
            // DIGI2
            i++;
            end = i + 6;
            while (i < end) {
                b = in[i] >> 1;
                out[3].append(b);
                //printf("[%02d] %c\n", i, (int) b);
                i++;
            }
            //c = in[i] & 0x01;  // not needed for the last digi
            h = in[i] > (char)127;
            b = (in[i] >> 1) & 0x0F;
            out[3].append('-');
            out[3].append(QString::number(b).toLatin1());
            if(h) out[3].append('*');
        }
    }

    i+=3; // skip past the previously read SSID byte and the 0x03 0xF0 Control and PID bytes
    end = in.length();
    // The payload is all the rest
    while(i < end) {
        out[4].append(in[i]);
        i++;
    }

    return out;
}
