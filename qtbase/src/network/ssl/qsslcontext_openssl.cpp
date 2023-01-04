/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtNetwork/qsslsocket.h>
#include <QtCore/qmutex.h>

#include "private/qssl_p.h"
#include "private/qsslcontext_openssl_p.h"
#include "private/qsslsocket_p.h"
#include "private/qsslsocket_openssl_p.h"
#include "private/qsslsocket_openssl_symbols_p.h"

QT_BEGIN_NAMESPACE

// defined in qsslsocket_openssl.cpp:
extern int q_X509Callback(int ok, X509_STORE_CTX *ctx);
extern QString getErrorsFromOpenSsl();

static DH *get_dh1024()
{
    // Default DH params
    // 1024-bit MODP Group
    // From RFC 2409
    QByteArray params = QByteArray::fromBase64(
       QByteArrayLiteral("MIGHAoGBAP//////////yQ/aoiFowjTExmKLgNwc0SkCTgiKZ8x0Agu+pjsTmyJR" \
                         "Sgh5jjQE3e+VGbPNOkMbMCsKbfJfFDdP4TVtbVHCReSFtXZiXn7G9ExC6aY37WsL" \
                         "/1y29Aa37e44a/taiZ+lrp8kEXxLH+ZJKGZR7OZTgf//////////AgEC"));

    const char *ptr = params.constData();
    DH *dh = q_d2i_DHparams(NULL, reinterpret_cast<const unsigned char **>(&ptr), params.length());

    return dh;
}

QSslContext::QSslContext()
    : ctx(0),
    pkey(0),
    session(0),
    m_sessionTicketLifeTimeHint(-1)
{
}

QSslContext::~QSslContext()
{
    if (ctx)
        // This will decrement the reference count by 1 and free the context eventually when possible
        q_SSL_CTX_free(ctx);

    if (pkey)
        q_EVP_PKEY_free(pkey);

    if (session)
        q_SSL_SESSION_free(session);
}

static inline QString msgErrorSettingEllipticCurves(const QString &why)
{
    return QSslSocket::tr("Error when setting the elliptic curves (%1)").arg(why);
}

QSslContext* QSslContext::fromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration, bool allowRootCertOnDemandLoading)
{
    QSslContext *sslContext = new QSslContext();
    sslContext->sslConfiguration = configuration;
    sslContext->errorCode = QSslError::NoError;

    bool client = (mode == QSslSocket::SslClientMode);

    bool reinitialized = false;
    bool unsupportedProtocol = false;
init_context:
    switch (sslContext->sslConfiguration.protocol()) {
    case QSsl::SslV2:
    case QSsl::SslV3:
        sslContext->ctx = 0;
        unsupportedProtocol = true;
        break;
    case QSsl::TlsV1_0:
    case QSsl::TlsV1_1:
    case QSsl::TlsV1_2:
    case QSsl::TlsV1_0OrLater:
    case QSsl::TlsV1_1OrLater:
    case QSsl::TlsV1_2OrLater:
    case QSsl::SecureProtocols:
    case QSsl::TlsV1SslV3:
    case QSsl::AnyProtocol:
    default:
        sslContext->ctx = q_SSL_CTX_new(client ? q_TLS_client_method() : q_TLS_server_method());
        break;
    }

    if (!sslContext->ctx) {
        // After stopping Flash 10 the SSL library looses its ciphers. Try re-adding them
        // by re-initializing the library.
        if (!reinitialized) {
            reinitialized = true;
            if (q_OPENSSL_init_ssl(0, 0) == 1)
                goto init_context;
        }

        sslContext->errorStr = QSslSocket::tr("Error creating SSL context (%1)").arg(
            unsupportedProtocol ? QSslSocket::tr("unsupported protocol") : QSslSocketBackendPrivate::getErrorsFromOpenSsl()
        );
        sslContext->errorCode = QSslError::UnspecifiedError;
        return sslContext;
    }

    // Enable bug workarounds.
    long options = QSslSocketBackendPrivate::setupOpenSslOptions(configuration.protocol(), configuration.d->sslOptions);
    q_SSL_CTX_set_options(sslContext->ctx, options);

    // Tell OpenSSL to release memory early
    // http://www.openssl.org/docs/ssl/SSL_CTX_set_mode.html
    q_SSL_CTX_set_mode(sslContext->ctx, SSL_MODE_RELEASE_BUFFERS);

    // Initialize ciphers
    QByteArray cipherString;
    bool first = true;
    QList<QSslCipher> ciphers = sslContext->sslConfiguration.ciphers();
    if (ciphers.isEmpty())
        ciphers = QSslSocketPrivate::defaultCiphers();
    foreach (const QSslCipher &cipher, ciphers) {
        if (first)
            first = false;
        else
            cipherString.append(':');
        cipherString.append(cipher.name().toLatin1());
    }

    if (!q_SSL_CTX_set_cipher_list(sslContext->ctx, cipherString.data())) {
        sslContext->errorStr = QSslSocket::tr("Invalid or empty cipher list (%1)").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
        sslContext->errorCode = QSslError::UnspecifiedError;
        return sslContext;
    }

    // Add all our CAs to this store.
    foreach (const QSslCertificate &caCertificate, sslContext->sslConfiguration.caCertificates()) {
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
        // See also: QSslSocketBackendPrivate::verify()
        if (caCertificate.expiryDate() >= QDateTime::currentDateTime()) {
            q_X509_STORE_add_cert(q_SSL_CTX_get_cert_store(sslContext->ctx), (X509 *)caCertificate.handle());
        }
    }

    if (QSslSocketPrivate::s_loadRootCertsOnDemand && allowRootCertOnDemandLoading) {
        // tell OpenSSL the directories where to look up the root certs on demand
        QList<QByteArray> unixDirs = QSslSocketPrivate::unixRootCertDirectories();
        for (int a = 0; a < unixDirs.count(); ++a)
            q_SSL_CTX_load_verify_locations(sslContext->ctx, 0, unixDirs.at(a).constData());
    }

    if (!sslContext->sslConfiguration.localCertificate().isNull()) {
        // Require a private key as well.
        if (sslContext->sslConfiguration.privateKey().isNull()) {
            sslContext->errorStr = QSslSocket::tr("Cannot provide a certificate with no key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return sslContext;
        }

        // Load certificate
        if (!q_SSL_CTX_use_certificate(sslContext->ctx, (X509 *)sslContext->sslConfiguration.localCertificate().handle())) {
            sslContext->errorStr = QSslSocket::tr("Error loading local certificate, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return sslContext;
        }

        if (configuration.d->privateKey.algorithm() == QSsl::Opaque) {
            sslContext->pkey = reinterpret_cast<EVP_PKEY *>(configuration.d->privateKey.handle());
        } else {
            // Load private key
            sslContext->pkey = q_EVP_PKEY_new();
            // before we were using EVP_PKEY_assign_R* functions and did not use EVP_PKEY_free.
            // this lead to a memory leak. Now we use the *_set1_* functions which do not
            // take ownership of the RSA/DSA key instance because the QSslKey already has ownership.
            if (configuration.d->privateKey.algorithm() == QSsl::Rsa)
                q_EVP_PKEY_set1_RSA(sslContext->pkey, reinterpret_cast<RSA *>(configuration.d->privateKey.handle()));
            else if (configuration.d->privateKey.algorithm() == QSsl::Dsa)
                q_EVP_PKEY_set1_DSA(sslContext->pkey, reinterpret_cast<DSA *>(configuration.d->privateKey.handle()));
#ifndef OPENSSL_NO_EC
            else if (configuration.d->privateKey.algorithm() == QSsl::Ec)
                q_EVP_PKEY_set1_EC_KEY(sslContext->pkey, reinterpret_cast<EC_KEY *>(configuration.d->privateKey.handle()));
#endif
        }

        if (!q_SSL_CTX_use_PrivateKey(sslContext->ctx, sslContext->pkey)) {
            sslContext->errorStr = QSslSocket::tr("Error loading private key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return sslContext;
        }
        if (configuration.d->privateKey.algorithm() == QSsl::Opaque)
            sslContext->pkey = 0; // Don't free the private key, it belongs to QSslKey

        // Check if the certificate matches the private key.
        if (!q_SSL_CTX_check_private_key(sslContext->ctx)) {
            sslContext->errorStr = QSslSocket::tr("Private key does not certify public key, %1").arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return sslContext;
        }

        // If we have any intermediate certificates then we need to add them to our chain
        bool first = true;
        foreach (const QSslCertificate &cert, configuration.d->localCertificateChain) {
            if (first) {
                first = false;
                continue;
            }
            q_SSL_CTX_ctrl(sslContext->ctx, SSL_CTRL_EXTRA_CHAIN_CERT, 0,
                           q_X509_dup(reinterpret_cast<X509 *>(cert.handle())));
        }
    }

    // Initialize peer verification.
    if (sslContext->sslConfiguration.peerVerifyMode() == QSslSocket::VerifyNone) {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_NONE, 0);
    } else {
        q_SSL_CTX_set_verify(sslContext->ctx, SSL_VERIFY_PEER, q_X509Callback);
    }

    // Set verification depth.
    if (sslContext->sslConfiguration.peerVerifyDepth() != 0)
        q_SSL_CTX_set_verify_depth(sslContext->ctx, sslContext->sslConfiguration.peerVerifyDepth());

    // set persisted session if the user set it
    if (!configuration.sessionTicket().isEmpty())
        sslContext->setSessionASN1(configuration.sessionTicket());

    // Set temp DH params
    DH *dh = 0;
    dh = get_dh1024();
    q_SSL_CTX_set_tmp_dh(sslContext->ctx, dh);
    q_DH_free(dh);

    const QVector<QSslEllipticCurve> qcurves = sslContext->sslConfiguration.ellipticCurves();
    if (!qcurves.isEmpty()) {
#ifndef OPENSSL_NO_EC
        // Set the curves to be used
        // SSL_CTX_ctrl wants a non-const pointer as last argument,
        // but let's avoid a copy into a temporary array
        if (!q_SSL_CTX_ctrl(sslContext->ctx,
                            SSL_CTRL_SET_CURVES,
                            qcurves.size(),
                            const_cast<int *>(reinterpret_cast<const int *>(qcurves.data())))) {
            sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
            sslContext->errorCode = QSslError::UnspecifiedError;
            return sslContext;
        }
#else
        // specific curves requested, but not possible to set -> error
        sslContext->errorStr = msgErrorSettingEllipticCurves(QSslSocket::tr("OpenSSL version too old, need at least v1.0.2"));
        sslContext->errorCode = QSslError::UnspecifiedError;
        return sslContext;
#endif
    }

    return sslContext;
}

#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_NEXTPROTONEG)

static int next_proto_cb(SSL *, unsigned char **out, unsigned char *outlen,
                         const unsigned char *in, unsigned int inlen, void *arg)
{
    QSslContext::NPNContext *ctx = reinterpret_cast<QSslContext::NPNContext *>(arg);

    // comment out to debug:
//    QList<QByteArray> supportedVersions;
//    for (unsigned int i = 0; i < inlen; ) {
//        QByteArray version(reinterpret_cast<const char *>(&in[i+1]), in[i]);
//        supportedVersions << version;
//        i += in[i] + 1;
//    }

    int proto = q_SSL_select_next_proto(out, outlen, in, inlen, ctx->data, ctx->len);
    switch (proto) {
    case OPENSSL_NPN_UNSUPPORTED:
        ctx->status = QSslConfiguration::NextProtocolNegotiationNone;
        break;
    case OPENSSL_NPN_NEGOTIATED:
        ctx->status = QSslConfiguration::NextProtocolNegotiationNegotiated;
        break;
    case OPENSSL_NPN_NO_OVERLAP:
        ctx->status = QSslConfiguration::NextProtocolNegotiationUnsupported;
        break;
    default:
        qCWarning(lcSsl, "OpenSSL sent unknown NPN status");
    }

    return SSL_TLSEXT_ERR_OK;
}

QSslContext::NPNContext QSslContext::npnContext() const
{
    return m_npnContext;
}
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...

// Needs to be deleted by caller
SSL* QSslContext::createSsl()
{
    SSL* ssl = q_SSL_new(ctx);
    q_SSL_clear(ssl);

    if (!session && !sessionASN1().isEmpty()
            && !sslConfiguration.testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        const unsigned char *data = reinterpret_cast<const unsigned char *>(m_sessionASN1.constData());
        session = q_d2i_SSL_SESSION(0, &data, m_sessionASN1.size()); // refcount is 1 already, set by function above
    }

    if (session) {
        // Try to resume the last session we cached
        if (!q_SSL_set_session(ssl, session)) {
            qCWarning(lcSsl, "could not set SSL session");
            q_SSL_SESSION_free(session);
            session = 0;
        }
    }

#if OPENSSL_VERSION_NUMBER >= 0x1000100fL && !defined(OPENSSL_NO_NEXTPROTONEG)
    QList<QByteArray> protocols = sslConfiguration.d->nextAllowedProtocols;
    if (!protocols.isEmpty()) {
        m_supportedNPNVersions.clear();
        for (int a = 0; a < protocols.count(); ++a) {
            if (protocols.at(a).size() > 255) {
                qCWarning(lcSsl) << "TLS NPN extension" << protocols.at(a)
                                 << "is too long and will be truncated to 255 characters.";
                protocols[a] = protocols.at(a).left(255);
            }
            m_supportedNPNVersions.append(protocols.at(a).size()).append(protocols.at(a));
        }
        m_npnContext.data = reinterpret_cast<unsigned char *>(m_supportedNPNVersions.data());
        m_npnContext.len = m_supportedNPNVersions.count();
        m_npnContext.status = QSslConfiguration::NextProtocolNegotiationNone;
        q_SSL_CTX_set_next_proto_select_cb(ctx, next_proto_cb, &m_npnContext);
    }
#endif // OPENSSL_VERSION_NUMBER >= 0x1000100fL ...

    return ssl;
}

// We cache exactly one session here
bool QSslContext::cacheSession(SSL* ssl)
{
    // don't cache the same session again
    if (session && session == q_SSL_get_session(ssl))
        return true;

    // decrease refcount of currently stored session
    // (this might happen if there are several concurrent handshakes in flight)
    if (session)
        q_SSL_SESSION_free(session);

    // cache the session the caller gave us and increase reference count
    session = q_SSL_get1_session(ssl);

    if (session && !sslConfiguration.testSslOption(QSsl::SslOptionDisableSessionPersistence)) {
        int sessionSize = q_i2d_SSL_SESSION(session, 0);
        if (sessionSize > 0) {
            m_sessionASN1.resize(sessionSize);
            unsigned char *data = reinterpret_cast<unsigned char *>(m_sessionASN1.data());
            if (!q_i2d_SSL_SESSION(session, &data))
                qCWarning(lcSsl, "could not store persistent version of SSL session");
            m_sessionTicketLifeTimeHint = q_SSL_SESSION_get_ticket_lifetime_hint(session);
        }
    }

    return (session != 0);
}

QByteArray QSslContext::sessionASN1() const
{
    return m_sessionASN1;
}

void QSslContext::setSessionASN1(const QByteArray &session)
{
    m_sessionASN1 = session;
}

int QSslContext::sessionTicketLifeTimeHint() const
{
    return m_sessionTicketLifeTimeHint;
}

QSslError::SslError QSslContext::error() const
{
    return errorCode;
}

QString QSslContext::errorString() const
{
    return errorStr;
}

QT_END_NAMESPACE
