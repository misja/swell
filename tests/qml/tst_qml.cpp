// SPDX-License-Identifier: MIT
// Creates a QQmlComponent for QML files on the "minimal" platform: syntax,
// imports and types are resolved; nothing is instantiated or rendered, so
// runtime binding warnings are not checked here.
#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>

#include "registration.h"

class TstQml : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void spikeWindowCompiles();
    void negativeControlFails();
    void appFileCompiles_data();
    void appFileCompiles();

private:
    QQmlComponent::Status compile(const QString &relativePath, QString *errors);
    QQmlEngine *m_engine = nullptr;
};

void TstQml::initTestCase()
{
    swell::registerTypes();
    m_engine = new QQmlEngine(this);
}

QQmlComponent::Status TstQml::compile(const QString &relativePath, QString *errors)
{
    const QUrl url = QUrl::fromLocalFile(QStringLiteral(SWELL_SOURCE_DIR "/") + relativePath);
    QQmlComponent component(m_engine, url);
    for (const QQmlError &e : component.errors())
        errors->append(e.toString() + QLatin1Char('\n'));
    return component.status();
}

void TstQml::spikeWindowCompiles()
{
    QString errors;
    QCOMPARE(compile(QStringLiteral("tests/qml/spike/Window.qml"), &errors), QQmlComponent::Ready);
    QVERIFY2(errors.isEmpty(), qPrintable(errors));
}

void TstQml::negativeControlFails()
{
    QString errors;
    QCOMPARE(compile(QStringLiteral("tests/qml/spike/Broken.qml"), &errors), QQmlComponent::Error);
    QVERIFY(errors.contains(QStringLiteral("noSuchProperty")));
}

void TstQml::appFileCompiles_data()
{
    QTest::addColumn<QString>("file");
    const QString root = QStringLiteral(SWELL_SOURCE_DIR "/");
    QDirIterator it(root + QStringLiteral("qml"), { QStringLiteral("*.qml") }, QDir::Files,
                    QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext()) {
        const QString path = it.next().mid(root.size());
        QTest::newRow(qPrintable(path)) << path;
        ++count;
    }
    QVERIFY2(count > 0, "no QML files found");
}

void TstQml::appFileCompiles()
{
    QFETCH(QString, file);
    QString errors;
    QCOMPARE(compile(file, &errors), QQmlComponent::Ready);
    QVERIFY2(errors.isEmpty(), qPrintable(errors));
}

int main(int argc, char *argv[])
{
    // No display in the build engine: compile on the minimal platform.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "minimal");
    QGuiApplication app(argc, argv);
    TstQml test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_qml.moc"
