/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

//#define QSSLSOCKET_DEBUG
//#define QT_DECRYPT_SSL_TRAFFIC

#include "qssl_p.h"
#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"
#include "qsslellipticcurve.h"
#include "qsslpresharedkeyauthenticator.h"
#include "qsslpresharedkeyauthenticator_p.h"
#include "qsslkey.h"


#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qlibrary.h>
#include <QtCore/qglobalstatic.h>

#include <algorithm>
#include <memory>

#include <string.h>

QT_BEGIN_NAMESPACE

#if defined(Q_OS_MACX)
#define kSecTrustSettingsDomainSystem 2 // so we do not need to include the header file
    PtrSecCertificateCopyData QSslSocketPrivate::ptrSecCertificateCopyData = 0;
    PtrSecTrustSettingsCopyCertificates QSslSocketPrivate::ptrSecTrustSettingsCopyCertificates = 0;
    PtrSecTrustCopyAnchorCertificates QSslSocketPrivate::ptrSecTrustCopyAnchorCertificates = 0;
#elif defined(Q_OS_WIN)
    PtrCertOpenSystemStoreW QSslSocketPrivate::ptrCertOpenSystemStoreW = 0;
    PtrCertFindCertificateInStore QSslSocketPrivate::ptrCertFindCertificateInStore = 0;
    PtrCertCloseStore QSslSocketPrivate::ptrCertCloseStore = 0;
#endif

Q_GLOBAL_STATIC(QRecursiveMutex, qt_opensslInitMutex)

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;
int QSslSocketBackendPrivate::s_indexForSSLExtraData = -1;

QString QSslSocketBackendPrivate::getErrorsFromOpenSsl()
{
    QString errorString;
    char buf[256] = {}; // OpenSSL docs claim both 120 and 256; use the larger.
    unsigned long errNum;
    while ((errNum = q_ERR_get_error())) {
        if (!errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        q_ERR_error_string_n(errNum, buf, sizeof buf);
        errorString.append(QString::fromLatin1(buf)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

void QSslSocketBackendPrivate::logAndClearErrorQueue()
{
    const auto errors = getErrorsFromOpenSsl();
    if (errors.size())
        qCWarning(lcSsl) << "Discarding errors:" << errors;
}

extern "C" {

#ifndef OPENSSL_NO_PSK
static unsigned int q_ssl_psk_client_callback(SSL *ssl,
                                              const char *hint,
                                              char *identity, unsigned int max_identity_len,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskClientCallback(hint, identity, max_identity_len, psk, max_psk_len);
}

static unsigned int q_ssl_psk_server_callback(SSL *ssl,
                                              const char *identity,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskServerCallback(identity, psk, max_psk_len);
}

#ifdef TLS1_3_VERSION
static unsigned int q_ssl_psk_restore_client(SSL *ssl,
                                             const char *hint,
                                             char *identity, unsigned int max_identity_len,
                                             unsigned char *psk, unsigned int max_psk_len)
{
    Q_UNUSED(hint);
    Q_UNUSED(identity);
    Q_UNUSED(max_identity_len);
    Q_UNUSED(psk);
    Q_UNUSED(max_psk_len);

#ifdef QT_DEBUG
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    Q_ASSERT(d->mode == QSslSocket::SslClientMode);
#endif
    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);

    return 0;
}

static int q_ssl_psk_use_session_callback(SSL *ssl, const EVP_MD *md, const unsigned char **id,
                                          size_t *idlen, SSL_SESSION **sess)
{
    Q_UNUSED(ssl);
    Q_UNUSED(md);
    Q_UNUSED(id);
    Q_UNUSED(idlen);
    Q_UNUSED(sess);

#ifdef QT_DEBUG
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    Q_ASSERT(d->mode == QSslSocket::SslClientMode);
#endif

    // Temporarily rebind the psk because it will be called next. The function will restore it.
    q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_restore_client);

    return 1; // need to return 1 or else "the connection setup fails."
}

int q_ssl_sess_set_new_cb(SSL *ssl, SSL_SESSION *session)
{
    if (!ssl) {
        qCWarning(lcSsl, "Invalid SSL (nullptr)");
        return 0;
    }
    if (!session) {
        qCWarning(lcSsl, "Invalid SSL_SESSION (nullptr)");
        return 0;
    }

    auto socketPrivate = static_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl,
                                                                 QSslSocketBackendPrivate::s_indexForSSLExtraData));
    return socketPrivate->handleNewSessionTicket(ssl);
}
#endif // TLS1_3_VERSION

#endif // !OPENSSL_NO_PSK

} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : ssl(0),
      readBio(0),
      writeBio(0),
      session(0)
{
    // Calls SSL_library_init().
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    destroySslContext();
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(const SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    QStringList descriptionList = descriptionOneLine.split(QLatin1String(" "), QString::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0);

        QString protoString = descriptionList.at(1);
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("SSLv3"))
            ciph.d->protocol = QSsl::SslV3;
        else if (protoString == QLatin1String("SSLv2"))
            ciph.d->protocol = QSsl::SslV2;
        else if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1_0;
        else if (protoString == QLatin1String("TLSv1.1"))
            ciph.d->protocol = QSsl::TlsV1_1;
        else if (protoString == QLatin1String("TLSv1.2"))
            ciph.d->protocol = QSsl::TlsV1_2;
        else if (protoString == QLatin1String("TLSv1.3"))
            ciph.d->protocol = QSsl::TlsV1_3;

        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3);
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3);
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4);
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = q_SSL_CIPHER_get_bits(cipher, &ciph.d->supportedBits);
    }
    return ciph;
}

QSslErrorEntry QSslErrorEntry::fromStoreContext(X509_STORE_CTX *ctx)
{
    QSslErrorEntry const entry = {
        q_X509_STORE_CTX_get_error(ctx),
        q_X509_STORE_CTX_get_error_depth(ctx)
    };
    return entry;
}

int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.

        typedef QVector<QSslErrorEntry>* ErrorListPtr;
        ErrorListPtr errors = 0;

        // Error list is attached to either 'SSL' or 'X509_STORE'.
        if (X509_STORE *store = q_X509_STORE_CTX_get0_store(ctx)) // We try store first:
            errors = ErrorListPtr(q_X509_STORE_get_ex_data(store, 0));

        if (!errors) {
            // Not found on store? Try SSL and its external data then. According to the OpenSSL's
            // documentation:
            //
            // "Whenever a X509_STORE_CTX object is created for the verification of the peers certificate
            // during a handshake, a pointer to the SSL object is stored into the X509_STORE_CTX object
            // to identify the connection affected. To retrieve this pointer the X509_STORE_CTX_get_ex_data()
            // function can be used with the correct index."
            if (SSL *ssl = static_cast<SSL *>(q_X509_STORE_CTX_get_ex_data(ctx, q_SSL_get_ex_data_X509_STORE_CTX_idx())))
                errors = ErrorListPtr(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData + 1));
        }

        if (!errors) {
            qCWarning(lcSsl, "Neither X509_STORE, nor SSL contains error list, handshake failure");
            return 0;
        }

        errors->append(QSslErrorEntry::fromStoreContext(ctx));
    }
    // Always return OK to allow verification to continue. We handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

static void q_loadCiphersForConnection(SSL *connection, QList<QSslCipher> &ciphers,
                                       QList<QSslCipher> &defaultCiphers)
{
    Q_ASSERT(connection);

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(connection);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
            if (!ciph.isNull()) {
                // Unconditionally exclude ADH and AECDH ciphers since they offer no MITM protection
                if (!ciph.name().toLower().startsWith(QLatin1String("adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("exp-adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("aecdh"))) {
                    ciphers << ciph;

                    if (ciph.usedBits() >= 128)
                        defaultCiphers << ciph;
                }
            }
        }
    }
}

// Defined in qsslsocket.cpp
void q_setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);

long QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions)
{
    long options;
    if (protocol == QSsl::TlsV1SslV3)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::SecureProtocols)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::TlsV1_0OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::TlsV1_1OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1;
    else if (protocol == QSsl::TlsV1_2OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1;
    else if (protocol == QSsl::TlsV1_3OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1|SSL_OP_NO_TLSv1_2;
    else
        options = SSL_OP_ALL;

    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    if (!(sslOptions & QSsl::SslOptionDisableServerCipherPreference))
        options |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    return options;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // If no external context was set (e.g. by QHttpNetworkConnection) we will
    // create a default context
    if (!sslContextPointer) {
        // create a deep copy of our configuration
        QSslConfigurationPrivate *configurationCopy = new QSslConfigurationPrivate(configuration);
        configurationCopy->ref.store(0);              // the QSslConfiguration constructor refs up
        sslContextPointer = QSharedPointer<QSslContext>(
                    QSslContext::fromConfiguration(mode, configurationCopy, allowRootCertOnDemandLoading));
    }

    if (sslContextPointer->error() != QSslError::NoError) {
        q->setErrorString(sslContextPointer->errorString());
        q->setSocketError(QAbstractSocket::SslInvalidUserDataError);
        emit q->error(QAbstractSocket::SslInvalidUserDataError);
        sslContextPointer.clear(); // deletes the QSslContext
        return false;
    }

    // Create and initialize SSL session
    if (!(ssl = sslContextPointer->createSsl())) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session, %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return false;
    }

    if (configuration.protocol != QSsl::SslV2 &&
        configuration.protocol != QSsl::SslV3 &&
        configuration.protocol != QSsl::UnknownProtocol &&
        mode == QSslSocket::SslClientMode) {
        // Set server hostname on TLS extension. RFC4366 section 3.1 requires it in ACE format.
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;
        QByteArray ace = QUrl::toAce(tlsHostName);
        // only send the SNI header if the URL is valid and not an IP
        if (!ace.isEmpty()
            && !QHostAddress().setAddress(tlsHostName)
            && !(configuration.sslOptions & QSsl::SslOptionDisableServerNameIndication)) {
            // We don't send the trailing dot from the host header if present see
            // https://tools.ietf.org/html/rfc6066#section-3
            if (ace.endsWith('.'))
                ace.chop(1);
            if (!q_SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ace.data()))
                qCWarning(lcSsl, "could not set SSL_CTRL_SET_TLSEXT_HOSTNAME, Server Name Indication disabled");
        }
    }

    // Clear the session.
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        q->setErrorString(QSslSocket::tr("Error creating SSL session: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData, this);

#ifndef OPENSSL_NO_PSK
    // Set the client callback for PSK
    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);
    else if (mode == QSslSocket::SslServerMode)
        q_SSL_set_psk_server_callback(ssl, &q_ssl_psk_server_callback);

#if OPENSSL_VERSION_NUMBER >= 0x10101006L
    // Set the client callback for TLSv1.3 PSK
    if (mode == QSslSocket::SslClientMode
        && QSslSocket::sslLibraryBuildVersionNumber() >= 0x10101006L) {
        q_SSL_set_psk_use_session_callback(ssl, &q_ssl_psk_use_session_callback);
    }
#endif // openssl version >= 0x10101006L

#endif // OPENSSL_NO_PSK

    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    if (ssl) {
        q_SSL_free(ssl);
        ssl = 0;
    }
    sslContextPointer.clear();
}

/*!
    \internal

    Does the minimum amount of initialization to determine whether SSL
    is supported or not.
*/

bool QSslSocketPrivate::supportsSsl()
{
    return ensureLibraryLoaded();
}


/*!
    \internal

    Returns the version number of the SSL library in use. Note that
    this is the version of the library in use at run-time, not compile
    time.
*/
long QSslSocketPrivate::sslLibraryVersionNumber()
{
    if (!supportsSsl())
        return 0;

    return q_OpenSSL_version_num();
}

/*!
    \internal

    Returns the version string of the SSL library in use. Note that
    this is the version of the library in use at run-time, not compile
    time. If no SSL support is available then this will return an empty value.
*/
QString QSslSocketPrivate::sslLibraryVersionString()
{
    if (!supportsSsl())
        return QString();

    const char *versionString = q_OpenSSL_version(OPENSSL_VERSION);
    if (!versionString)
        return QString();

    return QString::fromLatin1(versionString);
}

/*!
    \internal

    Declared static in QSslSocketPrivate, makes sure the SSL libraries have
    been initialized.
*/
void QSslSocketPrivate::ensureInitialized()
{
    if (!supportsSsl())
        return;

    ensureCiphersAndCertsLoaded();
}

/*!
    \internal

    Returns the version number of the SSL library in use at compile
    time.
*/
long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    return OPENSSL_VERSION_NUMBER;
}

/*!
    \internal

    Returns the version string of the SSL library in use at compile
    time.
*/
QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    // Using QStringLiteral to store the version string as unicode and
    // avoid false positives from Google searching the playstore for old
    // SSL versions. See QTBUG-46265
    return QLatin1String(OPENSSL_VERSION_TEXT);
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetDefaultCiphers()
{
    SSL_CTX *myCtx = q_SSL_CTX_new(q_TLS_client_method());
    // Note, we assert, not just silently return/bail out early:
    // this should never happen and problems with OpenSSL's initialization
    // must be caught before this (see supportsSsl()).
    Q_ASSERT(myCtx);
    SSL *mySsl = q_SSL_new(myCtx);
    Q_ASSERT(mySsl);

    QList<QSslCipher> ciphers;
    QList<QSslCipher> defaultCiphers;

    q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(defaultCiphers);

#if QT_SUPPORTS(dtls)
    ciphers.clear();
    defaultCiphers.clear();
    myCtx = q_SSL_CTX_new(q_DTLS_client_method());
    if (myCtx) {
        mySsl = q_SSL_new(myCtx);
        if (mySsl) {
            q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);
            q_setDefaultDtlsCiphers(defaultCiphers);
            q_SSL_free(mySsl);
        }
        q_SSL_CTX_free(myCtx);
    }
#endif // dtls
}

void QSslSocketPrivate::resetDefaultEllipticCurves()
{
    QVector<QSslEllipticCurve> curves;

#ifndef OPENSSL_NO_EC
    const size_t curveCount = q_EC_get_builtin_curves(NULL, 0);

    QVarLengthArray<EC_builtin_curve> builtinCurves(static_cast<int>(curveCount));

    if (q_EC_get_builtin_curves(builtinCurves.data(), curveCount) == curveCount) {
        curves.reserve(int(curveCount));
        for (size_t i = 0; i < curveCount; ++i) {
            QSslEllipticCurve curve;
            curve.id = builtinCurves[int(i)].nid;
            curves.append(curve);
        }
    }
#endif // OPENSSL_NO_EC

    // set the list of supported ECs, but not the list
    // of *default* ECs. OpenSSL doesn't like forcing an EC for the wrong
    // ciphersuite, so don't try it -- leave the empty list to mean
    // "the implementation will choose the most suitable one".
    setDefaultSupportedEllipticCurves(curves);
}

#ifndef Q_OS_DARWIN // Apple implementation in qsslsocket_mac_shared.cpp
QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    ensureInitialized();
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_WIN)
    HCERTSTORE hSystemStore;
    hSystemStore = CertOpenSystemStoreW(0, L"ROOT");
    if (hSystemStore) {
        PCCERT_CONTEXT pc = 0;
        while (1) {
            pc = CertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0, CERT_FIND_ANY, 0, pc);
            if (!pc)
                break;
            QByteArray der(reinterpret_cast<const char *>(pc->pbCertEncoded),
                            static_cast<int>(pc->cbCertEncoded));
            QSslCertificate cert(der, QSsl::Der);
            systemCerts.append(cert);
        }
        CertCloseStore(hSystemStore, 0);
    }
#elif defined(Q_OS_UNIX)
    QSet<QString> certFiles;
    QDir currentDir;
    QStringList nameFilters;
    QList<QByteArray> directories;
    QSsl::EncodingFormat platformEncodingFormat;
# ifndef Q_OS_ANDROID
    directories = unixRootCertDirectories();
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    platformEncodingFormat = QSsl::Pem;
# else
    // Q_OS_ANDROID
    QByteArray ministroPath = qgetenv("MINISTRO_SSL_CERTS_PATH"); // Set by Ministro
    directories << ministroPath;
    nameFilters << QLatin1String("*.der");
    platformEncodingFormat = QSsl::Der;
#  ifndef Q_OS_ANDROID_EMBEDDED
    if (ministroPath.isEmpty()) {
        QList<QByteArray> certificateData = fetchSslCertificateData();
        for (int i = 0; i < certificateData.size(); ++i) {
            systemCerts.append(QSslCertificate::fromData(certificateData.at(i), QSsl::Der));
        }
    } else
#  endif //Q_OS_ANDROID_EMBEDDED
# endif //Q_OS_ANDROID
    {
        currentDir.setNameFilters(nameFilters);
        for (int a = 0; a < directories.count(); a++) {
            currentDir.setPath(QLatin1String(directories.at(a)));
            QDirIterator it(currentDir);
            while (it.hasNext()) {
                it.next();
                // use canonical path here to not load the same certificate twice if symlinked
                certFiles.insert(it.fileInfo().canonicalFilePath());
            }
        }
        QSetIterator<QString> it(certFiles);
        while (it.hasNext())
            systemCerts.append(QSslCertificate::fromPath(it.next(), platformEncodingFormat));
# ifndef Q_OS_ANDROID
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss
# endif
    }
#endif
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "systemCaCertificates retrieval time " << timer.elapsed() << "ms";
    qCDebug(lcSsl) << "imported " << systemCerts.count() << " certificates";
#endif

    return systemCerts;
}
#endif // Q_OS_DARWIN

void QSslSocketBackendPrivate::startClientEncryption()
{
    Q_Q(QSslSocket);
    if (!initSslContext()) {
        q->setErrorString(QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    Q_Q(QSslSocket);
    if (!initSslContext()) {
        q->setErrorString(QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        q->setSocketError(QAbstractSocket::SslInternalError);
        emit q->error(QAbstractSocket::SslInternalError);
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    bool transmitting;
    do {
        transmitting = false;

        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            qint64 totalBytesWritten = 0;
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    int error = q_SSL_get_error(ssl, writtenBytes);
                    //write can result in a want_write_error - not an error - continue transmitting
                    if (error == SSL_ERROR_WANT_WRITE) {
                        transmitting = true;
                        break;
                    } else if (error == SSL_ERROR_WANT_READ) {
                        //write can result in a want_read error, possibly due to renegotiation - not an error - stop transmitting
                        transmitting = false;
                        break;
                    } else {
                        // ### Better error handling.
                        q->setErrorString(QSslSocket::tr("Unable to write data: %1").arg(getErrorsFromOpenSsl()));
                        q->setSocketError(QAbstractSocket::SslInternalError);
                        emit q->error(QAbstractSocket::SslInternalError);
                        return;
                    }
                }
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encrypted" << writtenBytes << "bytes";
#endif
                writeBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;

                if (writtenBytes < nextDataBlockSize) {
                    // break out of the writing loop and try again after we had read
                    transmitting = true;
                    break;
                }
            }

            if (totalBytesWritten > 0) {
                // Don't emit bytesWritten() recursively.
                if (!emittedBytesWritten) {
                    emittedBytesWritten = true;
                    emit q->bytesWritten(totalBytesWritten);
                    emittedBytesWritten = false;
                }
            }
        }

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            qint64 actualWritten = plainSocket->write(data.constData(), encryptedBytesRead);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: wrote" << encryptedBytesRead << "encrypted bytes to the socket" << actualWritten << "actual.";
#endif
            if (actualWritten < 0) {
                //plain socket write fails if it was in the pending close state.
                q->setErrorString(plainSocket->errorString());
                q->setSocketError(plainSocket->error());
                emit q->error(plainSocket->error());
                return;
            }
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        if (!connectionEncrypted || !readBufferMaxSize || buffer.size() < readBufferMaxSize)
            while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
                // Read encrypted data from the socket into a buffer.
                data.resize(pendingBytes);
                // just peek() here because q_BIO_write could write less data than expected
                int encryptedBytesRead = plainSocket->peek(data.data(), pendingBytes);

#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: read" << encryptedBytesRead << "encrypted bytes from the socket";
#endif
                // Write encrypted data from the buffer into the read BIO.
                int writtenToBio = q_BIO_write(readBio, data.constData(), encryptedBytesRead);

                // do the actual read() here and throw away the results.
                if (writtenToBio > 0) {
                    // ### TODO: make this cheaper by not making it memcpy. E.g. make it work with data=0x0 or make it work with seek
                    plainSocket->read(data.data(), writtenToBio);
                } else {
                    // ### Better error handling.
                    q->setErrorString(QSslSocket::tr("Unable to decrypt data: %1").arg(getErrorsFromOpenSsl()));
                    q->setSocketError(QAbstractSocket::SslInternalError);
                    emit q->error(QAbstractSocket::SslInternalError);
                    return;
                }

                transmitting = true;
            }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: testing encryption";
#endif
            if (startHandshake()) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption established";
#endif
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: connection lost";
#endif
                break;
            } else if (paused) {
                // just wait until the user continues
                return;
            } else {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption not done yet";
#endif
            }
        }

        // If the request is small and the remote host closes the transmission
        // after sending, there's a chance that startHandshake() will already
        // have triggered a shutdown.
        if (!ssl)
            continue;

        // We always read everything from the SSL decryption buffers, even if
        // we have a readBufferMaxSize. There's no point in leaving data there
        // just so that readBuffer.size() == readBufferMaxSize.
        int readBytes = 0;
        data.resize(4096);
        ::memset(data.data(), 0, data.size());
        do {
            // Don't use SSL_pending(). It's very unreliable.
            if ((readBytes = q_SSL_read(ssl, data.data(), data.size())) > 0) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: decrypted" << readBytes << "bytes";
#endif
                char *ptr = buffer.reserve(readBytes);
                ::memcpy(ptr, data.data(), readBytes);

                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                transmitting = true;
                continue;
            }

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: remote disconnect";
#endif
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                q->setErrorString(QSslSocket::tr("The TLS/SSL connection has been closed"));
                q->setSocketError(QAbstractSocket::RemoteHostClosedError);
                emit q->error(QAbstractSocket::RemoteHostClosedError);
                return;
            case SSL_ERROR_SYSCALL: // some IO error
            case SSL_ERROR_SSL: // error in the SSL library
                // we do not know exactly what the error is, nor whether we can recover from it,
                // so just return to prevent an endless loop in the outer "while" statement
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::SslInternalError);
                emit q->error(QAbstractSocket::SslInternalError);
                return;
            default:
                // SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT: can only happen with a
                // BIO_s_connect() or BIO_s_accept(), which we do not call.
                // SSL_ERROR_WANT_X509_LOOKUP: can only happen with a
                // SSL_CTX_set_client_cert_cb(), which we do not call.
                // So this default case should never be triggered.
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                q->setSocketError(QAbstractSocket::SslInternalError);
                emit q->error(QAbstractSocket::SslInternalError);
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
}

QSslError _q_OpenSSL_to_QSslError(int errorCode, const QSslCertificate &cert)
{
    QSslError error;
    switch (errorCode) {
    case X509_V_OK:
        // X509_V_OK is also reported if the peer had no certificate.
        break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        error = QSslError(QSslError::UnableToGetIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        error = QSslError(QSslError::UnableToDecryptCertificateSignature, cert); break;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        error = QSslError(QSslError::UnableToDecodeIssuerPublicKey, cert); break;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        error = QSslError(QSslError::CertificateSignatureFailed, cert); break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        error = QSslError(QSslError::CertificateNotYetValid, cert); break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        error = QSslError(QSslError::CertificateExpired, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        error = QSslError(QSslError::InvalidNotBeforeField, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        error = QSslError(QSslError::InvalidNotAfterField, cert); break;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        error = QSslError(QSslError::SelfSignedCertificate, cert); break;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        error = QSslError(QSslError::SelfSignedCertificateInChain, cert); break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        error = QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        error = QSslError(QSslError::UnableToVerifyFirstCertificate, cert); break;
    case X509_V_ERR_CERT_REVOKED:
        error = QSslError(QSslError::CertificateRevoked, cert); break;
    case X509_V_ERR_INVALID_CA:
        error = QSslError(QSslError::InvalidCaCertificate, cert); break;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        error = QSslError(QSslError::PathLengthExceeded, cert); break;
    case X509_V_ERR_INVALID_PURPOSE:
        error = QSslError(QSslError::InvalidPurpose, cert); break;
    case X509_V_ERR_CERT_UNTRUSTED:
        error = QSslError(QSslError::CertificateUntrusted, cert); break;
    case X509_V_ERR_CERT_REJECTED:
        error = QSslError(QSslError::CertificateRejected, cert); break;
    default:
        error = QSslError(QSslError::UnspecifiedError, cert); break;
    }
    return error;
}

QString QSslSocketBackendPrivate::msgErrorsDuringHandshake()
{
    return QSslSocket::tr("Error during SSL handshake: %1")
                         .arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
}

bool QSslSocketBackendPrivate::startHandshake()
{
    Q_Q(QSslSocket);

    // Check if the connection has been established. Get all errors from the
    // verification stage.

    QVector<QSslErrorEntry> lastErrors;
    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData + 1, &lastErrors);
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);
    q_SSL_set_ex_data(ssl, s_indexForSSLExtraData + 1, 0);

    if (!lastErrors.isEmpty())
        storePeerCertificates();
    for (int i = 0; i < lastErrors.size(); ++i) {
        const QSslErrorEntry &currentError = lastErrors.at(i);
        emit q->peerVerifyError(_q_OpenSSL_to_QSslError(currentError.code,
                                configuration.peerCertificateChain.value(currentError.depth)));
        if (q->state() != QAbstractSocket::ConnectedState)
            break;
    }

    errorList << lastErrors;

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState)
        return false;

    // Check if we're encrypted or not.
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            QString errorString = QSslSocketBackendPrivate::msgErrorsDuringHandshake();
            q->setErrorString(errorString);
            q->setSocketError(QAbstractSocket::SslHandshakeFailedError);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::startHandshake: error!" << errorString;
#endif
            emit q->error(QAbstractSocket::SslHandshakeFailedError);
            q->abort();
        }
        return false;
    }

    // store peer certificate chain
    storePeerCertificates();

    // Start translating errors.
    QList<QSslError> errors;

    // check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    foreach (const QSslCertificate &cert, configuration.peerCertificateChain) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    const bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                              || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                                  && mode == QSslSocket::SslClientMode);

    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (mode == QSslSocket::SslClientMode) {
            QString peerName = (verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);

            if (!isMatchingHostname(configuration.peerCertificate, peerName)) {
                // No matches in common names or alternate names.
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        if (doVerifyPeer) {
            QSslError error(QSslError::NoPeerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + errorList.size());
    for (int i = 0; i < errorList.size(); ++i) {
        const QSslErrorEntry &error = errorList.at(i);
        errors << _q_OpenSSL_to_QSslError(error.code, configuration.peerCertificateChain.value(error.depth));
    }

    if (!errors.isEmpty()) {
        sslErrors = errors;

#ifdef Q_OS_WIN
        //Skip this if not using system CAs, or if the SSL errors are configured in advance to be ignorable
        if (doVerifyPeer
            && s_loadRootCertsOnDemand
            && allowRootCertOnDemandLoading
            && !verifyErrorsHaveBeenIgnored()) {
            //Windows desktop versions starting from vista ship with minimal set of roots
            //and download on demand from the windows update server CA roots that are
            //trusted by MS.
            //However, this is only transparent if using WinINET - we have to trigger it
            //ourselves.
            QSslCertificate certToFetch;
            bool fetchCertificate = true;
            for (int i=0; i< sslErrors.count(); i++) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate: // site presented intermediate cert, but root is unknown
                case QSslError::SelfSignedCertificateInChain: // site presented a complete chain, but root is unknown
                    certToFetch = sslErrors.at(i).certificate();
                    break;
                case QSslError::SelfSignedCertificate:
                case QSslError::CertificateBlacklisted:
                    //With these errors, we know it will be untrusted so save time by not asking windows
                    fetchCertificate = false;
                    break;
                default:
#ifdef QSSLSOCKET_DEBUG
                    qCDebug(lcSsl) << sslErrors.at(i).errorString();
#endif
                    break;
                }
            }
            if (fetchCertificate && !certToFetch.isNull()) {
                fetchCaRootForCert(certToFetch);
                return false;
            }
        }
#endif
        if (!checkSslErrors())
            return false;
        // A slot, attached to sslErrors signal can call
        // abort/close/disconnetFromHost/etc; no need to
        // continue handshake then.
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    } else {
        sslErrors.clear();
    }

    continueHandshake();
    return true;
}

void QSslSocketBackendPrivate::storePeerCertificates()
{
    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    X509 *x509 = q_SSL_get_peer_certificate(ssl);
    configuration.peerCertificate = QSslCertificatePrivate::QSslCertificate_from_X509(x509);
    q_X509_free(x509);
    if (configuration.peerCertificateChain.isEmpty()) {
        configuration.peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
        if (!configuration.peerCertificate.isNull() && mode == QSslSocket::SslServerMode)
            configuration.peerCertificateChain.prepend(configuration.peerCertificate);
    }
}

int QSslSocketBackendPrivate::handleNewSessionTicket(SSL *connection)
{
    // If we return 1, this means we own the session, but we don't.
    // 0 would tell OpenSSL to deref (but they still have it in the
    // internal cache).
    Q_Q(QSslSocket);

    Q_ASSERT(connection);

    if (q->sslConfiguration().testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        // We silently ignore, do nothing, remove from cache.
        return 0;
    }

    SSL_SESSION *currentSession = q_SSL_get_session(connection);
    if (!currentSession) {
        qCWarning(lcSsl,
                  "New session ticket callback, the session is invalid (nullptr)");
        return 0;
    }

    if (q_SSL_version(connection) < 0x304) {
        // We only rely on this mechanics with TLS >= 1.3
        return 0;
    }

#ifdef TLS1_3_VERSION
    if (!q_SSL_SESSION_is_resumable(currentSession)) {
        qCDebug(lcSsl, "New session ticket, but the session is non-resumable");
        return 0;
    }
#endif // TLS1_3_VERSION

    const int sessionSize = q_i2d_SSL_SESSION(currentSession, nullptr);
    if (sessionSize <= 0) {
        qCWarning(lcSsl, "could not store persistent version of SSL session");
        return 0;
    }

    // We have somewhat perverse naming, it's not a ticket, it's a session.
    QByteArray sessionTicket(sessionSize, 0);
    auto data = reinterpret_cast<unsigned char *>(sessionTicket.data());
    if (!q_i2d_SSL_SESSION(currentSession, &data)) {
        qCWarning(lcSsl, "could not store persistent version of SSL session");
        return 0;
    }

    configuration.sslSession = sessionTicket;
    configuration.sslSessionTicketLifeTimeHint = int(q_SSL_SESSION_get_ticket_lifetime_hint(currentSession));

    emit q->newSessionTicketReceived();
    return 0;
}

bool QSslSocketBackendPrivate::checkSslErrors()
{
    Q_Q(QSslSocket);
    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);
    bool doEmitSslError = !verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            pauseSocketNotifiers(q);
            paused = true;
        } else {
            q->setErrorString(sslErrors.first().errorString());
            q->setSocketError(QAbstractSocket::SslHandshakeFailedError);
            emit q->error(QAbstractSocket::SslHandshakeFailedError);
            plainSocket->disconnectFromHost();
        }
        return false;
    }
    return true;
}

unsigned int QSslSocketBackendPrivate::tlsPskClientCallback(const char *hint,
                                                            char *identity, unsigned int max_identity_len,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    if (hint)
        authenticator.d->identityHint = QByteArray::fromRawData(hint, int(::strlen(hint))); // it's NUL terminated, but do not include the NUL

    authenticator.d->maximumIdentityLength = int(max_identity_len) - 1; // needs to be NUL terminated
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int identityLength = qMin(authenticator.identity().length(), authenticator.maximumIdentityLength());
    ::memcpy(identity, authenticator.identity().constData(), identityLength);
    identity[identityLength] = 0;

    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

unsigned int QSslSocketBackendPrivate::tlsPskServerCallback(const char *identity,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    authenticator.d->identityHint = configuration.preSharedKeyIdentityHint;
    authenticator.d->identity = identity;
    authenticator.d->maximumIdentityLength = 0; // user cannot set an identity
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

#ifdef Q_OS_WIN

void QSslSocketBackendPrivate::fetchCaRootForCert(const QSslCertificate &cert)
{
    Q_Q(QSslSocket);
    //The root certificate is downloaded from windows update, which blocks for 15 seconds in the worst case
    //so the request is done in a worker thread.
    QWindowsCaRootFetcher *fetcher = new QWindowsCaRootFetcher(cert, mode);
    QObject::connect(fetcher, SIGNAL(finished(QSslCertificate,QSslCertificate)), q, SLOT(_q_caRootLoaded(QSslCertificate,QSslCertificate)), Qt::QueuedConnection);
    QMetaObject::invokeMethod(fetcher, "start", Qt::QueuedConnection);
    pauseSocketNotifiers(q);
    paused = true;
}

//This is the callback from QWindowsCaRootFetcher, trustedRoot will be invalid (default constructed) if it failed.
void QSslSocketBackendPrivate::_q_caRootLoaded(QSslCertificate cert, QSslCertificate trustedRoot)
{
    if (!trustedRoot.isNull() && !trustedRoot.isBlacklisted()) {
        if (s_loadRootCertsOnDemand) {
            //Add the new root cert to default cert list for use by future sockets
            QSslSocket::addDefaultCaCertificate(trustedRoot);
        }
        //Add the new root cert to this socket for future connections
        if (!configuration.caCertificates.contains(trustedRoot))
            configuration.caCertificates += trustedRoot;
        //Remove the broken chain ssl errors (as chain is verified by windows)
        for (int i=sslErrors.count() - 1; i >= 0; --i) {
            if (sslErrors.at(i).certificate() == cert) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate:
                case QSslError::CertificateUntrusted:
                case QSslError::UnableToVerifyFirstCertificate:
                case QSslError::SelfSignedCertificateInChain:
                    // error can be ignored if OS says the chain is trusted
                    sslErrors.removeAt(i);
                    break;
                default:
                    // error cannot be ignored
                    break;
                }
            }
        }
    }

    // Continue with remaining errors
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (checkSslErrors() && ssl)
        continueHandshake();
}

class QWindowsCaRootFetcherThread : public QThread
{
public:
    QWindowsCaRootFetcherThread()
    {
        qRegisterMetaType<QSslCertificate>();
        setObjectName(QStringLiteral("QWindowsCaRootFetcher"));
        start();
    }
    ~QWindowsCaRootFetcherThread()
    {
        quit();
        wait(15500); // worst case, a running request can block for 15 seconds
    }
};

Q_GLOBAL_STATIC(QWindowsCaRootFetcherThread, windowsCaRootFetcherThread);

QWindowsCaRootFetcher::QWindowsCaRootFetcher(const QSslCertificate &certificate, QSslSocket::SslMode sslMode)
    : cert(certificate), mode(sslMode)
{
    moveToThread(windowsCaRootFetcherThread());
}

QWindowsCaRootFetcher::~QWindowsCaRootFetcher()
{
}

void QWindowsCaRootFetcher::start()
{
    QByteArray der = cert.toDer();
    PCCERT_CONTEXT wincert = CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)der.constData(), der.length());
    if (!wincert) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl, "QWindowsCaRootFetcher failed to convert certificate to windows form");
#endif
        emit finished(cert, QSslCertificate());
        deleteLater();
        return;
    }

    CERT_CHAIN_PARA parameters;
    memset(&parameters, 0, sizeof(parameters));
    parameters.cbSize = sizeof(parameters);
    // set key usage constraint
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    LPSTR oid = (LPSTR)(mode == QSslSocket::SslClientMode ? szOID_PKIX_KP_SERVER_AUTH : szOID_PKIX_KP_CLIENT_AUTH);
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &oid;

#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer stopwatch;
    stopwatch.start();
#endif
    PCCERT_CHAIN_CONTEXT chain;
    BOOL result = CertGetCertificateChain(
        0, //default engine
        wincert,
        0, //current date/time
        0, //default store
        &parameters,
        0, //default dwFlags
        0, //reserved
        &chain);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QWindowsCaRootFetcher" << stopwatch.elapsed() << "ms to get chain";
#endif

    QSslCertificate trustedRoot;
    if (result) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "QWindowsCaRootFetcher - examining windows chains";
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
            qCDebug(lcSsl) << " - TRUSTED";
        else
            qCDebug(lcSsl) << " - NOT TRUSTED" << chain->TrustStatus.dwErrorStatus;
        if (chain->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED)
            qCDebug(lcSsl) << " - SELF SIGNED";
        qCDebug(lcSsl) << "QSslSocketBackendPrivate::fetchCaRootForCert - dumping simple chains";
        for (unsigned int i = 0; i < chain->cChain; i++) {
            if (chain->rgpChain[i]->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
                qCDebug(lcSsl) << " - TRUSTED SIMPLE CHAIN" << i;
            else
                qCDebug(lcSsl) << " - UNTRUSTED SIMPLE CHAIN" << i << "reason:" << chain->rgpChain[i]->TrustStatus.dwErrorStatus;
            for (unsigned int j = 0; j < chain->rgpChain[i]->cElement; j++) {
                QSslCertificate foundCert(QByteArray((const char *)chain->rgpChain[i]->rgpElement[j]->pCertContext->pbCertEncoded
                    , chain->rgpChain[i]->rgpElement[j]->pCertContext->cbCertEncoded), QSsl::Der);
                qCDebug(lcSsl) << "   - " << foundCert;
            }
        }
        qCDebug(lcSsl) << " - and" << chain->cLowerQualityChainContext << "low quality chains"; //expect 0, we haven't asked for them
#endif

        //based on http://msdn.microsoft.com/en-us/library/windows/desktop/aa377182%28v=vs.85%29.aspx
        //about the final chain rgpChain[cChain-1] which must begin with a trusted root to be valid
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
            && chain->cChain > 0) {
            const PCERT_SIMPLE_CHAIN finalChain = chain->rgpChain[chain->cChain - 1];
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa377544%28v=vs.85%29.aspx
            // rgpElement[0] is the end certificate chain element. rgpElement[cElement-1] is the self-signed "root" certificate element.
            if (finalChain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
                && finalChain->cElement > 0) {
                    trustedRoot = QSslCertificate(QByteArray((const char *)finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->pbCertEncoded
                        , finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->cbCertEncoded), QSsl::Der);
            }
        }
        CertFreeCertificateChain(chain);
    }
    CertFreeCertificateContext(wincert);

    emit finished(cert, trustedRoot);
    deleteLater();
}
#endif

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown) {
            q_SSL_shutdown(ssl);
            shutdown = true;
            transmit();
        }
    }
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (plainSocket->bytesAvailable() <= 0)
        destroySslContext();
    else {
        // Move all bytes into the plain buffer
        qint64 tmpReadBufferMaxSize = readBufferMaxSize;
        readBufferMaxSize = 0; // reset temporarily so the plain socket buffer is completely drained
        transmit();
        readBufferMaxSize = tmpReadBufferMaxSize;
    }
    //if there is still buffered data in the plain socket, don't destroy the ssl context yet.
    //it will be destroyed when the socket is deleted.
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    if (!ssl)
        return QSslCipher();

    const SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
    return sessionCipher ? QSslCipher_from_SSL_CIPHER(sessionCipher) : QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    if (!ssl)
        return QSsl::UnknownProtocol;
    int ver = q_SSL_version(ssl);

    switch (ver) {
    case 0x2:
        return QSsl::SslV2;
    case 0x300:
        return QSsl::SslV3;
    case 0x301:
        return QSsl::TlsV1_0;
    case 0x302:
        return QSsl::TlsV1_1;
    case 0x303:
        return QSsl::TlsV1_2;
    case 0x304:
        return QSsl::TlsV1_3;
    }

    return QSsl::UnknownProtocol;
}


void QSslSocketBackendPrivate::continueHandshake()
{
    Q_Q(QSslSocket);
    // if we have a max read buffer size, reset the plain socket's to match
    if (readBufferMaxSize)
        plainSocket->setReadBufferSize(readBufferMaxSize);

    if (q_SSL_session_reused(ssl))
        configuration.peerSessionShared = true;

#ifdef QT_DECRYPT_SSL_TRAFFIC
    if (q_SSL_get_session(ssl)) {
        size_t master_key_len = q_SSL_SESSION_get_master_key(q_SSL_get_session(ssl), 0, 0);
        size_t client_random_len = q_SSL_get_client_random(ssl, 0, 0);
        QByteArray masterKey(int(master_key_len), 0); // Will not overflow
        QByteArray clientRandom(int(client_random_len), 0); // Will not overflow

        q_SSL_SESSION_get_master_key(q_SSL_get_session(ssl),
                                     reinterpret_cast<unsigned char*>(masterKey.data()),
                                     masterKey.size());
        q_SSL_get_client_random(ssl, reinterpret_cast<unsigned char *>(clientRandom.data()),
                                clientRandom.size());

        QByteArray debugLineClientRandom("CLIENT_RANDOM ");
        debugLineClientRandom.append(clientRandom.toHex().toUpper());
        debugLineClientRandom.append(" ");
        debugLineClientRandom.append(masterKey.toHex().toUpper());
        debugLineClientRandom.append("\n");

        QString sslKeyFile = QDir::tempPath() + QLatin1String("/qt-ssl-keys");
        QFile file(sslKeyFile);
        if (!file.open(QIODevice::Append))
            qCWarning(lcSsl) << "could not open file" << sslKeyFile << "for appending";
        if (!file.write(debugLineClientRandom))
            qCWarning(lcSsl) << "could not write to file" << sslKeyFile;
        file.close();
    } else {
        qCWarning(lcSsl, "could not decrypt SSL traffic");
    }
#endif

    // Cache this SSL session inside the QSslContext
    if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionSharing)) {
        if (!sslContextPointer->cacheSession(ssl)) {
            sslContextPointer.clear(); // we could not cache the session
        } else {
            // Cache the session for permanent usage as well
            if (!(configuration.sslOptions & QSsl::SslOptionDisableSessionPersistence)) {
                if (!sslContextPointer->sessionASN1().isEmpty())
                    configuration.sslSession = sslContextPointer->sessionASN1();
                configuration.sslSessionTicketLifeTimeHint = sslContextPointer->sessionTicketLifeTimeHint();
            }
        }
    }

#if !defined(OPENSSL_NO_NEXTPROTONEG)

    configuration.nextProtocolNegotiationStatus = sslContextPointer->npnContext().status;
    if (sslContextPointer->npnContext().status == QSslConfiguration::NextProtocolNegotiationUnsupported) {
        // we could not agree -> be conservative and use HTTP/1.1
        configuration.nextNegotiatedProtocol = QByteArrayLiteral("http/1.1");
    } else {
        const unsigned char *proto = 0;
        unsigned int proto_len = 0;

        q_SSL_get0_alpn_selected(ssl, &proto, &proto_len);
        if (proto_len && mode == QSslSocket::SslClientMode) {
            // Client does not have a callback that sets it ...
            configuration.nextProtocolNegotiationStatus = QSslConfiguration::NextProtocolNegotiationNegotiated;
        }

        if (!proto_len) { // Test if NPN was more lucky ...
            q_SSL_get0_next_proto_negotiated(ssl, &proto, &proto_len);
        }

        if (proto_len)
            configuration.nextNegotiatedProtocol = QByteArray(reinterpret_cast<const char *>(proto), proto_len);
        else
            configuration.nextNegotiatedProtocol.clear();
    }
#endif // !defined(OPENSSL_NO_NEXTPROTONEG)

    if (mode == QSslSocket::SslClientMode) {
        EVP_PKEY *key;
        if (q_SSL_get_server_tmp_key(ssl, &key))
            configuration.ephemeralServerKey = QSslKey(key, QSsl::PublicKey);
    }

    connectionEncrypted = true;
    emit q->encrypted();
    if (autoStartHandshake && pendingClose) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

bool QSslSocketPrivate::ensureLibraryLoaded()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    const QMutexLocker locker(qt_opensslInitMutex);

    if (!s_libraryLoaded) {
        // Initialize OpenSSL.
        if (q_OPENSSL_init_ssl(0, nullptr) != 1)
            return false;

        if (q_OpenSSL_version_num() < 0x10101000L) {
            qCWarning(lcSsl, "QSslSocket: OpenSSL >= 1.1.1 is required; %s was found instead", q_OpenSSL_version(OPENSSL_VERSION));
            return false;
        }

        q_SSL_load_error_strings();
        q_OpenSSL_add_all_algorithms();

        QSslSocketBackendPrivate::s_indexForSSLExtraData
            = q_CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_SSL, 0L, nullptr, nullptr,
                                        nullptr, nullptr);

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            qWarning("Random number generator not seeded, disabling SSL support");
            return false;
        }

        s_libraryLoaded = true;
    }
    return true;
}

void QSslSocketPrivate::ensureCiphersAndCertsLoaded()
{
    const QMutexLocker locker(qt_opensslInitMutex);

    if (s_loadedCiphersAndCerts)
        return;
    s_loadedCiphersAndCerts = true;

    resetDefaultCiphers();
    resetDefaultEllipticCurves();

#if QT_SUPPORTS(library)
    //load symbols needed to receive certificates from system store
#if defined(Q_OS_QNX)
    s_loadRootCertsOnDemand = true;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // check whether we can enable on-demand root-cert loading (i.e. check whether the sym links are there)
    QList<QByteArray> dirs = unixRootCertDirectories();
    QStringList symLinkFilter;
    symLinkFilter << QLatin1String("[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f].[0-9]");
    for (int a = 0; a < dirs.count(); ++a) {
        QDirIterator iterator(QLatin1String(dirs.at(a)), symLinkFilter, QDir::Files);
        if (iterator.hasNext()) {
            s_loadRootCertsOnDemand = true;
            break;
        }
    }
#endif
#endif // QT_SUPPORTS(library)
    // if on-demand loading was not enabled, load the certs now
    if (!s_loadRootCertsOnDemand)
        setDefaultCaCertificates(systemCaCertificates());
#ifdef Q_OS_WIN
    //Enabled for fetching additional root certs from windows update on windows.
    //This flag is set false by setDefaultCaCertificates() indicating the app uses
    //its own cert bundle rather than the system one.
    //Same logic that disables the unix on demand cert loading.
    //Unlike unix, we do preload the certificates from the cert store.
    s_loadRootCertsOnDemand = true;
#endif
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    ensureInitialized();
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << QSslCertificatePrivate::QSslCertificate_from_X509(entry);
    }
    return certificates;
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &certificateChain,
                                                  const QString &hostName)
{
    if (s_loadRootCertsOnDemand)
        setDefaultCaCertificates(defaultCaCertificates() + systemCaCertificates());

    return verify(QSslConfiguration::defaultConfiguration().caCertificates(), certificateChain, hostName);
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &caCertificates,
                                                  const QList<QSslCertificate> &certificateChain,
                                                  const QString &hostName)
{
    QList<QSslError> errors;

    if (certificateChain.count() <= 0) {
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Setup the store with the default CA certificates
    X509_STORE *certStore = q_X509_STORE_new();
    if (!certStore) {
        qCWarning(lcSsl) << "Unable to create certificate store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    foreach (const QSslCertificate &caCertificate, caCertificates) {
        // From https://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html:
        //
        // If several CA certificates matching the name, key identifier, and
        // serial number condition are available, only the first one will be
        // examined. This may lead to unexpected results if the same CA
        // certificate is available with different expiration dates. If a
        // ``certificate expired'' verification error occurs, no other
        // certificate will be searched. Make sure to not have expired
        // certificates mixed with valid ones.
        //
        // See also: QSslContext::fromConfiguration()
        if (caCertificate.expiryDate() >= now) {
            q_X509_STORE_add_cert(certStore, reinterpret_cast<X509 *>(caCertificate.handle()));
        }
    }

    QVector<QSslErrorEntry> lastErrors;
    if (!q_X509_STORE_set_ex_data(certStore, 0, &lastErrors)) {
        qCWarning(lcSsl) << "Unable to attach external data (error list) to a store";
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Register a custom callback to get all verification errors.
    q_X509_STORE_set_verify_cb(certStore, q_X509Callback);

    // Build the chain of intermediate certificates
    STACK_OF(X509) *intermediates = 0;
    if (certificateChain.length() > 1) {
        intermediates = (STACK_OF(X509) *) q_OPENSSL_sk_new_null();

        if (!intermediates) {
            q_X509_STORE_free(certStore);
            errors << QSslError(QSslError::UnspecifiedError);
            return errors;
        }

        bool first = true;
        foreach (const QSslCertificate &cert, certificateChain) {
            if (first) {
                first = false;
                continue;
            }

            q_OPENSSL_sk_push((OPENSSL_STACK *)intermediates, reinterpret_cast<X509 *>(cert.handle()));
        }
    }

    X509_STORE_CTX *storeContext = q_X509_STORE_CTX_new();
    if (!storeContext) {
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    if (!q_X509_STORE_CTX_init(storeContext, certStore, reinterpret_cast<X509 *>(certificateChain[0].handle()), intermediates)) {
        q_X509_STORE_CTX_free(storeContext);
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Now we can actually perform the verification of the chain we have built.
    // We ignore the result of this function since we process errors via the
    // callback.
    (void) q_X509_verify_cert(storeContext);

    q_X509_STORE_CTX_free(storeContext);
    q_OPENSSL_sk_free((OPENSSL_STACK *)intermediates);

    // Now process the errors

    if (QSslCertificatePrivate::isBlacklisted(certificateChain[0])) {
        QSslError error(QSslError::CertificateBlacklisted, certificateChain[0]);
        errors << error;
    }

    // Check the certificate name against the hostname if one was specified
    if ((!hostName.isEmpty()) && (!isMatchingHostname(certificateChain[0], hostName))) {
        // No matches in common names or alternate names.
        QSslError error(QSslError::HostNameMismatch, certificateChain[0]);
        errors << error;
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + lastErrors.size());
    for (int i = 0; i < lastErrors.size(); ++i) {
        const QSslErrorEntry &error = lastErrors.at(i);
        errors << _q_OpenSSL_to_QSslError(error.code, certificateChain.value(error.depth));
    }

    q_X509_STORE_free(certStore);

    return errors;
}

bool QSslSocketBackendPrivate::importPkcs12(QIODevice *device,
                                            QSslKey *key, QSslCertificate *cert,
                                            QList<QSslCertificate> *caCertificates,
                                            const QByteArray &passPhrase)
{
    if (!supportsSsl())
        return false;

    // These are required
    Q_ASSERT(device);
    Q_ASSERT(key);
    Q_ASSERT(cert);

    // Read the file into a BIO
    QByteArray pkcs12data = device->readAll();
    if (pkcs12data.size() == 0)
        return false;

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pkcs12data.constData()), pkcs12data.size());

    // Create the PKCS#12 object
    PKCS12 *p12 = q_d2i_PKCS12_bio(bio, 0);
    if (!p12) {
        qCWarning(lcSsl, "Unable to read PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), 0));
        q_BIO_free(bio);
        return false;
    }

    // Extract the data
    EVP_PKEY *pkey;
    X509 *x509;
    STACK_OF(X509) *ca = 0;

    if (!q_PKCS12_parse(p12, passPhrase.constData(), &pkey, &x509, &ca)) {
        qCWarning(lcSsl, "Unable to parse PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), 0));
        q_PKCS12_free(p12);
        q_BIO_free(bio);
        return false;
    }

    // Convert to Qt types
    if (!key->d->fromEVP_PKEY(pkey)) {
        qCWarning(lcSsl, "Unable to convert private key");
        q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                              reinterpret_cast<void (*)(void *)>(q_X509_free));
        q_X509_free(x509);
        q_EVP_PKEY_free(pkey);
        q_PKCS12_free(p12);
        q_BIO_free(bio);

        return false;
    }

    *cert = QSslCertificatePrivate::QSslCertificate_from_X509(x509);

    if (caCertificates)
        *caCertificates = QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(ca);

    // Clean up
    q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                          reinterpret_cast<void (*)(void *)>(q_X509_free));

    q_X509_free(x509);
    q_EVP_PKEY_free(pkey);
    q_PKCS12_free(p12);
    q_BIO_free(bio);

    return true;
}


QT_END_NAMESPACE
