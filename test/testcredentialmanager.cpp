/*
 *    This software is in the public domain, furnished "as is", without technical
 *    support, and with no warranty, express or implied, as to its usefulness for
 *    any purpose.
 *
 */
#include "account.h"
#include "libsync/creds/credentialmanager.h"

#include "syncenginetestutils.h"

#include <QTest>

namespace OCC {

class TestCredentialManager : public QObject
{
    Q_OBJECT

    bool _finished = false;

    QTemporaryFile _credStoreFile;
    QSettings _credStore;

    void setFallbackEnabled(QKeychain::Job *job)
    {
        // store the test credentials in a plain text settings file on unsupported platforms
        job->setSettings(&_credStore);
        job->setInsecureFallback(true);
    }

private Q_SLOTS:
    void init()
    {
        _finished = false;
        QVERIFY(_credStoreFile.open());
        _credStore.setPath(QSettings::IniFormat, QSettings::UserScope, _credStoreFile.fileName());
        _credStore.clear();
    }

    void testSetGet_data()
    {
        QTest::addColumn<QVariant>("data");

        QTest::newRow("bool") << QVariant::fromValue(true);
        QTest::newRow("int") << QVariant::fromValue(1);
        QTest::newRow("map") << QVariant::fromValue(QVariantMap { { "foo", QColor(Qt::red) }, { "bar", "42" } });
    }

    void testSetGet()
    {
        QFETCH(QVariant, data);
        FakeFolder fakeFolder { FileInfo::A12_B12_C12_S12() };
        auto creds = fakeFolder.account()->credentialManager();

        const QString key = QStringLiteral("test");
        auto setJob = creds->set(key, data);
        setFallbackEnabled(setJob);

        connect(setJob, &QKeychain::Job::finished, this, [creds, data, key, setJob, this] {
#ifdef Q_OS_LINUX
            if (!qEnvironmentVariableIsSet("DBUS_SESSION_BUS_ADDRESS")) {
                QEXPECT_FAIL("", "QKeychain might not use the plaintext fallback and fail if dbus is not present", Abort);
                QCOMPARE(setJob->error(), QKeychain::NoError);
            }
#endif
            QCOMPARE(setJob->error(), QKeychain::NoError);
            auto getJob = creds->get(key);
            setFallbackEnabled(getJob->_job);
            connect(getJob, &CredentialJob::finished, this, [getJob, data, creds, this] {
                QCOMPARE(getJob->error(), QKeychain::NoError);
                QCOMPARE(getJob->data(), data);
                const auto jobs = creds->clear();
                for (auto &job : jobs) {
                    setFallbackEnabled(job);
                }
                connect(jobs[0], &QKeychain::Job::finished, this, [creds, data, jobs, this] {
                    QCOMPARE(jobs[0]->error(), QKeychain::NoError);
                    QVERIFY(creds->knownKeys().isEmpty());
                    _finished = true;
                });
            });
        });
        QTest::qWaitFor([this] { return _finished; });
    }
};
}

QTEST_GUILESS_MAIN(OCC::TestCredentialManager)
#include "testcredentialmanager.moc"
