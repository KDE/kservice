/*  This file is part of the KDE project
    Copyright (C) 2003,2007 Matthias Kretz <kretz@kde.org>
    Copyright 2013 Sebastian Kügler <sebas@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "kplugininfo.h"

#include <QList>
#include <QDebug>
#include <QDirIterator>
#include <QJsonArray>
#include <QStandardPaths>

#include <kaboutdata.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kpluginmetadata.h>
#include <kservice.h>
#include <kservicetypetrader.h>

//#ifndef NDEBUG
#define KPLUGININFO_ISVALID_ASSERTION \
    do { \
        if (!d) { \
            qFatal("Accessed invalid KPluginInfo object"); \
        } \
    } while (false)
//#else
//#define KPLUGININFO_ISVALID_ASSERTION
//#endif

static const char s_hiddenKey[] = "Hidden";
static const char s_nameKey[] = "Name";
static const char s_commentKey[] = "Comment";
static const char s_iconKey[] = "Icon";
static const char s_libraryKey[] = "X-KDE-Library";
static const char s_authorKey[] = "X-KDE-PluginInfo-Author";
static const char s_emailKey[] = "X-KDE-PluginInfo-Email";
static const char s_pluginNameKey[] = "X-KDE-PluginInfo-Name";
static const char s_versionKey[] = "X-KDE-PluginInfo-Version";
static const char s_websiteKey[] = "X-KDE-PluginInfo-Website";
static const char s_categoryKey[] = "X-KDE-PluginInfo-Category";
static const char s_licenseKey[] = "X-KDE-PluginInfo-License";
static const char s_dependenciesKey[] = "X-KDE-PluginInfo-Depends";
static const char s_serviceTypesKey[] = "ServiceTypes";
static const char s_xKDEServiceTypes[] = "X-KDE-ServiceTypes";
static const char s_enabledbyDefaultKey[] = "X-KDE-PluginInfo-EnabledByDefault";
static const char s_enabledKey[] = "Enabled";

class KPluginInfoPrivate : public QSharedData
{
public:
    KPluginInfoPrivate()
        : hidden(false)
        , enabledbydefault(false)
        , pluginenabled(false)
        , kcmservicesCached(false)
    {}

    QString entryPath; // the filename of the file containing all the info
    QString libraryPath;

    bool hidden : 1;
    bool enabledbydefault : 1;
    bool pluginenabled : 1;
    mutable bool kcmservicesCached : 1;

    QVariantMap metaData;
    KConfigGroup config;
    KService::Ptr service;
    mutable QList<KService::Ptr> kcmservices;
};

KPluginInfo::KPluginInfo(const QString &filename /*, QStandardPaths::StandardLocation resource*/)
    : d(new KPluginInfoPrivate)
{
    KDesktopFile file(/*resource,*/ filename);

    KConfigGroup cg = file.desktopGroup();
    if (!cg.exists()) {
        qWarning() << filename << "has no desktop group, cannot construct a KPluginInfo object from it.";
        d.reset();
        return;
    }
    d->entryPath = file.fileName();

    d->hidden = cg.readEntry(s_hiddenKey, false);
    if (d->hidden) {
        return;
    }

    d->metaData.insert(s_nameKey, file.readName());
    d->metaData.insert(s_commentKey, file.readComment());
    d->metaData.insert(s_iconKey, cg.readEntryUntranslated(s_iconKey));
    d->metaData.insert(s_authorKey, cg.readEntryUntranslated(s_authorKey));
    d->metaData.insert(s_emailKey, cg.readEntryUntranslated(s_emailKey));
    d->metaData.insert(s_pluginNameKey, cg.readEntryUntranslated(s_pluginNameKey));
    d->metaData.insert(s_versionKey, cg.readEntryUntranslated(s_versionKey));
    d->metaData.insert(s_websiteKey, cg.readEntryUntranslated(s_websiteKey));
    d->metaData.insert(s_categoryKey, cg.readEntryUntranslated(s_categoryKey));
    d->metaData.insert(s_licenseKey, cg.readEntryUntranslated(s_licenseKey));
    d->metaData.insert(s_dependenciesKey, cg.readEntry(s_dependenciesKey, QStringList()));
    d->metaData.insert(s_enabledbyDefaultKey, cg.readEntryUntranslated(s_enabledbyDefaultKey));
    d->enabledbydefault = cg.readEntry(s_enabledbyDefaultKey, false);
    d->libraryPath = cg.readEntry(s_libraryKey);
}

KPluginInfo::KPluginInfo(const QVariantList &args, const QString &libraryPath)
    : d(new KPluginInfoPrivate)
{
    static const QString metaData = QStringLiteral("MetaData");
    d->libraryPath = libraryPath;

    foreach (const QVariant &v, args) {
        if (v.canConvert<QVariantMap>()) {
            const QVariantMap &m = v.toMap();
            const QVariant &_metadata = m.value(metaData);
            if (_metadata.canConvert<QVariantMap>()) {
                d->metaData = _metadata.value<QVariantMap>();
                break;
            }
        }
    }

    d->hidden = d->metaData.value(s_hiddenKey).toBool();
    if (d->hidden) {
        return;
    }
    d->enabledbydefault = d->metaData.value(s_enabledbyDefaultKey).toBool();
}

#ifndef KSERVICE_NO_DEPRECATED
KPluginInfo::KPluginInfo(const KService::Ptr service)
    : d(new KPluginInfoPrivate)
{
    if (!service) {
        d = 0; // isValid() == false
        return;
    }
    d->service = service;
    d->entryPath = service->entryPath();
    d->libraryPath = service->library();

    if (service->isDeleted()) {
        d->hidden = true;
        return;
    }

    d->metaData.insert(s_nameKey, service->name());
    d->metaData.insert(s_commentKey, service->comment());
    d->metaData.insert(s_iconKey, service->icon());
    d->metaData.insert(s_authorKey, service->property(s_authorKey));
    d->metaData.insert(s_emailKey, service->property(s_emailKey));
    d->metaData.insert(s_pluginNameKey, service->property(s_pluginNameKey));
    d->metaData.insert(s_versionKey, service->property(s_versionKey));
    d->metaData.insert(s_websiteKey, service->property(s_websiteKey));
    d->metaData.insert(s_categoryKey, service->property(s_categoryKey));
    d->metaData.insert(s_licenseKey, service->property(s_licenseKey));
    d->metaData.insert(s_dependenciesKey, service->property(s_dependenciesKey));
    QVariant tmp = service->property(s_enabledbyDefaultKey);
    d->metaData.insert(s_enabledbyDefaultKey, tmp.toBool());
    d->enabledbydefault = d->metaData.value(s_enabledbyDefaultKey).toBool();
}
#endif

KPluginInfo::KPluginInfo()
    : d(0) // isValid() == false
{
}

bool KPluginInfo::isValid() const
{
    return d.data() != 0;
}

KPluginInfo::KPluginInfo(const KPluginInfo &rhs)
    : d(rhs.d)
{
}

KPluginInfo &KPluginInfo::operator=(const KPluginInfo &rhs)
{
    d = rhs.d;
    return *this;
}

bool KPluginInfo::operator==(const KPluginInfo &rhs) const
{
    return d == rhs.d;
}

bool KPluginInfo::operator!=(const KPluginInfo &rhs) const
{
    return d != rhs.d;
}

bool KPluginInfo::operator<(const KPluginInfo &rhs) const
{
    if (category() < rhs.category()) {
        return true;
    }
    if (category() == rhs.category()) {
        return name() < rhs.name();
    }
    return false;
}

bool KPluginInfo::operator>(const KPluginInfo &rhs) const
{
    if (category() > rhs.category()) {
        return true;
    }
    if (category() == rhs.category()) {
        return name() > rhs.name();
    }
    return false;
}

KPluginInfo::~KPluginInfo()
{
}

#ifndef KSERVICE_NO_DEPRECATED
QList<KPluginInfo> KPluginInfo::fromServices(const KService::List &services, const KConfigGroup &config)
{
    QList<KPluginInfo> infolist;
    for (KService::List::ConstIterator it = services.begin();
            it != services.end(); ++it) {
        KPluginInfo info(*it);
        info.setConfig(config);
        infolist += info;
    }
    return infolist;
}
#endif

QList<KPluginInfo> KPluginInfo::fromFiles(const QStringList &files, const KConfigGroup &config)
{
    QList<KPluginInfo> infolist;
    for (QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
        KPluginInfo info(*it);
        info.setConfig(config);
        infolist += info;
    }
    return infolist;
}

QList<KPluginInfo> KPluginInfo::fromKPartsInstanceName(const QString &name, const KConfigGroup &config)
{
    QStringList files;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, name + QStringLiteral("/kpartplugins"), QStandardPaths::LocateDirectory);
    Q_FOREACH (const QString &dir, dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.desktop"));
        while (it.hasNext()) {
            files.append(it.next());
        }
    }
    return fromFiles(files, config);
}

bool KPluginInfo::isHidden() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->hidden;
}

void KPluginInfo::setPluginEnabled(bool enabled)
{
    KPLUGININFO_ISVALID_ASSERTION;
    //qDebug() << Q_FUNC_INFO;
    d->pluginenabled = enabled;
}

bool KPluginInfo::isPluginEnabled() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    //qDebug() << Q_FUNC_INFO;
    return d->pluginenabled;
}

bool KPluginInfo::isPluginEnabledByDefault() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    //qDebug() << Q_FUNC_INFO;
    return d->enabledbydefault;
}

QString KPluginInfo::name() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_nameKey).toString();
}

QString KPluginInfo::comment() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_commentKey).toString();
}

QString KPluginInfo::icon() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_iconKey).toString();
}

QString KPluginInfo::entryPath() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->entryPath;
}

QString KPluginInfo::author() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_authorKey).toString();
}

QString KPluginInfo::email() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_emailKey).toString();
}

QString KPluginInfo::category() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_categoryKey).toString();
}

QString KPluginInfo::pluginName() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_pluginNameKey).toString();
}

QString KPluginInfo::libraryPath() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->libraryPath;
}

QString KPluginInfo::version() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_versionKey).toString();
}

QString KPluginInfo::website() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_websiteKey).toString();
}

QString KPluginInfo::license() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_licenseKey).toString();
}

#if 0
KAboutLicense KPluginInfo::fullLicense() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return KAboutLicense::byKeyword(d->license);
}
#endif

QStringList KPluginInfo::dependencies() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.value(s_dependenciesKey).toStringList();
}

QStringList KPluginInfo::serviceTypes() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    QStringList serviceTypes = d->metaData.value(s_xKDEServiceTypes).toStringList();
    if (serviceTypes.isEmpty()) {
        serviceTypes = d->metaData.value(s_serviceTypesKey).toStringList();
    }
    return serviceTypes;
}

KService::Ptr KPluginInfo::service() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->service;
}

QList<KService::Ptr> KPluginInfo::kcmServices() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    if (!d->kcmservicesCached) {
        d->kcmservices = KServiceTypeTrader::self()->query(QStringLiteral("KCModule"), QLatin1Char('\'') + pluginName() +
                         QStringLiteral("' in [X-KDE-ParentComponents]"));
        //qDebug() << "found" << d->kcmservices.count() << "offers for" << d->pluginName;

        d->kcmservicesCached = true;
    }

    return d->kcmservices;
}

void KPluginInfo::setConfig(const KConfigGroup &config)
{
    KPLUGININFO_ISVALID_ASSERTION;
    d->config = config;
}

KConfigGroup KPluginInfo::config() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->config;
}

QVariant KPluginInfo::property(const QString &key) const
{
    KPLUGININFO_ISVALID_ASSERTION;
    if (d->service) {
        return d->service->property(key);
    }
    return d->metaData.value(key);
}

QVariantMap KPluginInfo::properties() const
{
    return d->metaData;
}

void KPluginInfo::save(KConfigGroup config)
{
    KPLUGININFO_ISVALID_ASSERTION;
    //qDebug() << Q_FUNC_INFO;
    if (config.isValid()) {
        config.writeEntry(pluginName() + s_enabledKey, isPluginEnabled());
    } else {
        if (!d->config.isValid()) {
            qWarning() << "no KConfigGroup, cannot save";
            return;
        }
        d->config.writeEntry(pluginName() + s_enabledKey, isPluginEnabled());
    }
}

void KPluginInfo::load(const KConfigGroup &config)
{
    KPLUGININFO_ISVALID_ASSERTION;
    //qDebug() << Q_FUNC_INFO;
    if (config.isValid()) {
        setPluginEnabled(config.readEntry(pluginName() + s_enabledKey, isPluginEnabledByDefault()));
    } else {
        if (!d->config.isValid()) {
            qWarning() << "no KConfigGroup, cannot load";
            return;
        }
        setPluginEnabled(d->config.readEntry(pluginName() + s_enabledKey, isPluginEnabledByDefault()));
    }
}

void KPluginInfo::defaults()
{
    //qDebug() << Q_FUNC_INFO;
    setPluginEnabled(isPluginEnabledByDefault());
}

uint qHash(const KPluginInfo &p)
{
    return qHash(reinterpret_cast<quint64>(p.d.data()));
}

KPluginInfo KPluginInfo::fromMetaData(const KPluginMetaData &md)
{
    QVariantMap metaData = md.rawData().toVariantMap();
    const QList<KAboutPerson> &authors = md.authors();
    if (!authors.isEmpty()) {
        metaData[s_authorKey] = authors[0].name();
        metaData[s_emailKey] = authors[0].emailAddress();
    }
    metaData[s_categoryKey] = md.category();
    metaData[s_commentKey] = md.description();
    metaData[s_dependenciesKey] = md.dependencies();
    metaData[s_enabledbyDefaultKey] = md.isEnabledByDefault();
    metaData[s_iconKey] = md.iconName();
    metaData[s_licenseKey] = md.license();
    metaData[s_nameKey] = md.name();
    metaData[s_pluginNameKey] = md.pluginId();
    metaData[s_xKDEServiceTypes] = md.serviceTypes();
    metaData[s_versionKey] = md.version();
    metaData[s_websiteKey] = md.website();
    QVariantMap result;
    result[QStringLiteral("MetaData")] = metaData;
    return KPluginInfo(QVariantList() << result, md.fileName());
}

KPluginMetaData KPluginInfo::toMetaData() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    QJsonObject metadata = QJsonObject::fromVariantMap(d->metaData);
    QJsonObject kplugin;
    /* Metadata structure is as follows:
     "KPlugin": {
        "Name": "Date and Time",
        "Description": "Date and time by timezone",
        "Icon": "preferences-system-time",
        "Authors": { "Name": "Aaron Seigo", "Email": "aseigo@kde.org" },
        "Category": "Date and Time",
        "Dependencies": [],
        "EnabledByDefault": "true",
        "License": "LGPL",
        "Id": "time",
        "Version": "1.0",
        "Website": "http://plasma.kde.org/",
        "ServiceTypes": ["Plasma/DataEngine"]
     }
     */
    kplugin[QStringLiteral("Name")] = name();
    kplugin[QStringLiteral("Description")] = comment();
    kplugin[QStringLiteral("Icon")] = icon();
    QJsonObject authors;
    authors[QStringLiteral("Name")] = author();
    authors[QStringLiteral("Email")] = email();
    kplugin[QStringLiteral("Authors")] = authors;
    kplugin[QStringLiteral("Category")] = category();
    kplugin[QStringLiteral("Dependencies")] = QJsonArray::fromStringList(dependencies());
    kplugin[QStringLiteral("EnabledByDefault")] = isPluginEnabledByDefault();
    kplugin[QStringLiteral("License")] = license();
    kplugin[QStringLiteral("Id")] = pluginName();
    kplugin[QStringLiteral("Version")] = version();
    kplugin[QStringLiteral("Website")] = website();
    kplugin[QStringLiteral("ServiceTypes")] = QJsonArray::fromStringList(serviceTypes());
    metadata[QStringLiteral("KPlugin")] = kplugin;
    return KPluginMetaData(metadata, d->libraryPath);
}

KPluginMetaData KPluginInfo::toMetaData(const KPluginInfo& info)
{
    return info.toMetaData();
}

KPluginInfo::List KPluginInfo::fromMetaData(const QVector<KPluginMetaData> &list)
{
    KPluginInfo::List ret;
    ret.reserve(list.size());
    foreach(const KPluginMetaData &md, list) {
        ret.append(KPluginInfo::fromMetaData(md));
    }
    return ret;
}

QVector<KPluginMetaData> KPluginInfo::toMetaData(const KPluginInfo::List &list)
{
    QVector<KPluginMetaData> ret;
    ret.reserve(list.size());
    foreach(const KPluginInfo &info, list) {
        ret.append(info.toMetaData());
    }
    return ret;
}


#undef KPLUGININFO_ISVALID_ASSERTION
