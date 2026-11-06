// SPDX-License-Identifier: MIT
#include "lsdp.h"

#include <QStringList>

namespace BluOS {
namespace Lsdp {

namespace {

const int HeaderLength = 6;
const char Magic[] = "LSDP";
const quint8 Version = 1;

// Reads fields from one message without running past its end.
class Reader
{
public:
    Reader(const char *data, int size) : m_data(data), m_size(size) {}

    bool atEnd() const { return m_pos >= m_size; }
    bool ok() const { return m_ok; }

    quint8 byte()
    {
        if (m_pos + 1 > m_size) {
            m_ok = false;
            return 0;
        }
        return quint8(m_data[m_pos++]);
    }

    quint16 word()
    {
        const quint16 high = byte();
        const quint16 low = byte();
        return quint16((high << 8) | low);
    }

    QByteArray bytes(int count)
    {
        if (m_pos + count > m_size) {
            m_ok = false;
            return QByteArray();
        }
        QByteArray result(m_data + m_pos, count);
        m_pos += count;
        return result;
    }

private:
    const char *m_data;
    int m_size;
    int m_pos = 0;
    bool m_ok = true;
};

bool parseAnnounce(Reader &r, Announce *announce)
{
    announce->nodeId = r.bytes(r.byte());
    const QByteArray address = r.bytes(r.byte());
    if (!r.ok())
        return false;
    if (address.size() == 4) {
        const quint32 ipv4 = (quint32(quint8(address[0])) << 24) | (quint32(quint8(address[1])) << 16)
                | (quint32(quint8(address[2])) << 8) | quint32(quint8(address[3]));
        announce->address = QHostAddress(ipv4);
    }
    const int count = r.byte();
    for (int i = 0; i < count && r.ok(); ++i) {
        Record record;
        record.classId = r.word();
        const int txtCount = r.byte();
        for (int t = 0; t < txtCount && r.ok(); ++t) {
            const QByteArray key = r.bytes(r.byte());
            const QByteArray value = r.bytes(r.byte());
            record.txt.insert(QString::fromUtf8(key), QString::fromUtf8(value));
        }
        if (r.ok())
            announce->records.append(record);
    }
    return r.ok() && !announce->address.isNull();
}

bool parseDelete(Reader &r, Delete *del)
{
    del->nodeId = r.bytes(r.byte());
    const int count = r.byte();
    for (int i = 0; i < count && r.ok(); ++i)
        del->classes.append(r.word());
    return r.ok();
}

} // namespace

bool isPlayerClass(quint16 classId)
{
    return classId == ClassPlayer || classId == ClassPlayerSecondary
            || classId == ClassPlayerPairSlave || classId == ClassHub;
}

Packet parse(const QByteArray &datagram)
{
    Packet packet;
    if (datagram.size() < HeaderLength)
        return packet;
    const int headerLength = quint8(datagram[0]);
    if (headerLength < HeaderLength || headerLength > datagram.size()
            || datagram.mid(1, 4) != QByteArray(Magic) || quint8(datagram[5]) != Version)
        return packet;
    packet.valid = true;

    int pos = headerLength;
    while (pos < datagram.size()) {
        const int length = quint8(datagram[pos]);
        if (length < 2) {
            // A zero or one byte message cannot carry a type; nothing after it can be trusted.
            ++packet.skipped;
            break;
        }
        if (pos + length > datagram.size()) {
            packet.truncated = true;
            break;
        }
        const char type = datagram[pos + 1];
        Reader reader(datagram.constData() + pos + 2, length - 2);
        if (type == 'A') {
            Announce announce;
            if (parseAnnounce(reader, &announce))
                packet.announces.append(announce);
            else
                ++packet.skipped;
        } else if (type == 'D') {
            Delete del;
            if (parseDelete(reader, &del))
                packet.deletes.append(del);
            else
                ++packet.skipped;
        } else if (type == 'Q' || type == 'R') {
            ++packet.queries;
        } else {
            ++packet.skipped;
        }
        pos += length;
    }
    return packet;
}

QByteArray buildQuery(const QVector<quint16> &classes)
{
    QByteArray packet;
    packet.append(char(HeaderLength));
    packet.append(Magic, 4);
    packet.append(char(Version));
    const int length = 3 + 2 * classes.size();
    packet.append(char(length));
    packet.append('Q');
    packet.append(char(classes.size()));
    for (quint16 classId : classes) {
        packet.append(char(classId >> 8));
        packet.append(char(classId & 0xFF));
    }
    return packet;
}

QString nodeIdToString(const QByteArray &nodeId)
{
    QStringList parts;
    for (char c : nodeId)
        parts.append(QStringLiteral("%1").arg(quint8(c), 2, 16, QLatin1Char('0')).toUpper());
    return parts.join(QLatin1Char(':'));
}

} // namespace Lsdp
} // namespace BluOS
