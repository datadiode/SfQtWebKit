/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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


#ifndef QSSLCERTIFICATE_OPENSSL_P_H
#define QSSLCERTIFICATE_OPENSSL_P_H

#include "qsslcertificate.h"

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsslsocket_p.h"
#include "qsslcertificateextension.h"
#include <QtCore/qdatetime.h>
#include <QtCore/qmap.h>

#ifndef QT_NO_OPENSSL
#include <openssl/x509.h>
#else
struct X509;
struct X509_EXTENSION;
struct ASN1_OBJECT;
#endif

#ifdef Q_OS_WINRT
#include <wrl.h>
#include <windows.security.cryptography.certificates.h>
#endif

QT_BEGIN_NAMESPACE

// forward declaration

class QSslCertificatePrivate
{
public:
    QSslCertificatePrivate()
        : null(true), x509(0)
    {
        QSslSocketPrivate::ensureInitialized();
    }

    ~QSslCertificatePrivate()
    {
#ifndef QT_NO_OPENSSL
        if (x509)
            q_X509_free(x509);
#endif
    }

    bool null;
    QByteArray versionString;
    QByteArray serialNumberString;

    QMap<QByteArray, QString> issuerInfo;
    QMap<QByteArray, QString> subjectInfo;
    QDateTime notValidAfter;
    QDateTime notValidBefore;

#ifdef QT_NO_OPENSSL
    bool subjectMatchesIssuer;
    QSsl::KeyAlgorithm publicKeyAlgorithm;
    QByteArray publicKeyDerData;
    QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames;
    QList<QSslCertificateExtension> extensions;

    QByteArray derData;

    bool parse(const QByteArray &data);
    bool parseExtension(const QByteArray &data, QSslCertificateExtension *extension);
#endif
    X509 *x509;

    void init(const QByteArray &data, QSsl::EncodingFormat format);

    static QByteArray asn1ObjectId(ASN1_OBJECT *object);
    static QByteArray asn1ObjectName(ASN1_OBJECT *object);
    static QByteArray QByteArray_from_X509(X509 *x509, QSsl::EncodingFormat format);
    static QString text_from_X509(X509 *x509);
    static QSslCertificate QSslCertificate_from_X509(X509 *x509);
    static QList<QSslCertificate> certificatesFromPem(const QByteArray &pem, int count = -1);
    static QList<QSslCertificate> certificatesFromDer(const QByteArray &der, int count = -1);
    static bool isBlacklisted(const QSslCertificate &certificate);
    static QSslCertificateExtension convertExtension(X509_EXTENSION *ext);
    static QByteArray subjectInfoToString(QSslCertificate::SubjectInfo info);

    friend class QSslSocketBackendPrivate;

    QAtomicInt ref;

#ifdef Q_OS_WINRT
    Microsoft::WRL::ComPtr<ABI::Windows::Security::Cryptography::Certificates::ICertificate> certificate;

    static QSslCertificate QSslCertificate_from_Certificate(ABI::Windows::Security::Cryptography::Certificates::ICertificate *iCertificate);
#endif
};

QT_END_NAMESPACE

#endif // QSSLCERTIFICATE_OPENSSL_P_H
