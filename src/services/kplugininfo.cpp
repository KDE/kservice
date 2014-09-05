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

// these keys are used in the json metadata
static const char s_jsonDescriptionKey[] = "Description";
static const char s_jsonAuthorsKey[] = "Authors";
static const char s_jsonEmailKey[] = "Email";
static const char s_jsonCategoryKey[] = "Category";
static const char s_jsonDependenciesKey[] = "Dependencies";
static const char s_jsonEnabledByDefaultKey[] = "EnabledByDefault";
static const char s_jsonLicenseKey[] = "License";
static const char s_jsonIdKey[] = "Id";
static const char s_jsonVersionKey[] = "Version";
static const char s_jsonWebsiteKey[] = "Website";
static const char s_jsonKPluginKey[] = "KPlugin";

class KPluginInfoPrivate : public QSharedData
{
public:
    KPluginInfoPrivate()
        : hidden(false)
        , pluginenabled(false)
        , kcmservicesCached(false)
    {}

    QString entryPath; // the filename of the file containing all the info

    bool hidden : 1;
    bool pluginenabled : 1;
    mutable bool kcmservicesCached : 1;

    KPluginMetaData metaData;
    KConfigGroup config;
    KService::Ptr service;
    mutable QList<KService::Ptr> kcmservices;
};

// maps the KService, QVariant and KDesktopFile keys to the new KPluginMetaData keys
template<typename T, typename Func>
static QJsonObject mapToJsonKPluginKey(const QString &name, const QString &description,
        const QStringList &dependencies, const QStringList &serviceTypes, const T &data, Func accessor)
{
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
    QJsonObject kplugin;
    kplugin[s_nameKey] = name;
    kplugin[s_jsonDescriptionKey] = description;
    kplugin[s_iconKey] = accessor(data, s_iconKey);
    QJsonObject authors;
    authors[s_nameKey] = accessor(data, s_authorKey);
    authors[s_jsonEmailKey] = accessor(data, s_emailKey);
    kplugin[s_jsonAuthorsKey] = authors;
    kplugin[s_jsonCategoryKey] = accessor(data, s_categoryKey);
    const bool enabledByDefault = accessor(data, s_enabledbyDefaultKey).compare("true", Qt::CaseInsensitive) == 0;
    kplugin[s_jsonEnabledByDefaultKey] = enabledByDefault;
    kplugin[s_jsonLicenseKey] = accessor(data, s_licenseKey);
    kplugin[s_jsonIdKey] = accessor(data, s_pluginNameKey);
    kplugin[s_jsonVersionKey] = accessor(data, s_versionKey);
    kplugin[s_jsonWebsiteKey] = accessor(data, s_websiteKey);
    kplugin[s_serviceTypesKey] = QJsonArray::fromStringList(serviceTypes);
    kplugin[s_jsonDependenciesKey] = QJsonArray::fromStringList(dependencies);
    return kplugin;
}

KPluginInfo::KPluginInfo(const KPluginMetaData &md)
    :d(new KPluginInfoPrivate)
{
    if (!md.isValid()) {
        d.reset();
        return;
    }
    d->metaData = md;
}

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

    QStringList serviceTypes = cg.readEntry(s_xKDEServiceTypes, QStringList());
    if (serviceTypes.isEmpty()) {
        serviceTypes = cg.readEntry(s_serviceTypesKey, QStringList());
    }
    QJsonObject json;
    QJsonObject kplugin = mapToJsonKPluginKey(file.readName(), file.readComment(),
            cg.readEntry(s_dependenciesKey, QStringList()), serviceTypes, cg,
            [](const KConfigGroup &cg, const char *key) { return cg.readEntryUntranslated(key); });
    json[s_jsonKPluginKey] = kplugin;

    d->metaData = KPluginMetaData(json, cg.readEntry(s_libraryKey));
}

KPluginInfo::KPluginInfo(const QVariantList &args, const QString &libraryPath)
    : d(new KPluginInfoPrivate)
{
    static const QString metaData = QStringLiteral("MetaData");
    d->entryPath = QFileInfo(libraryPath).absoluteFilePath();

    foreach (const QVariant &v, args) {
        if (v.canConvert<QVariantMap>()) {
            const QVariantMap &m = v.toMap();
            const QVariant &_metadata = m.value(metaData);
            if (_metadata.canConvert<QVariantMap>()) {
                const QVariantMap &map = _metadata.toMap();
                if (map.value(s_hiddenKey).toBool()) {
                    d->hidden = true;
                    break;
                }

                QStringList serviceTypes = map.value(s_xKDEServiceTypes).toStringList();
                if (serviceTypes.isEmpty()) {
                    serviceTypes = map.value(s_serviceTypesKey).toStringList();
                }
                QJsonObject json = QJsonObject::fromVariantMap(map);
                if (!json.value(s_jsonKPluginKey).isObject()) {
                    // if the converted JSON object does not have a "KPlugin" key yet it means
                    // that we are converting from a .desktop style variant map
                    QString name = KPluginMetaData::readTranslatedString(json, s_nameKey);
                    QString description = KPluginMetaData::readTranslatedString(json, s_commentKey);
                    QJsonObject kplugin = mapToJsonKPluginKey(name, description,
                            map.value(s_dependenciesKey, QStringList()).toStringList(), serviceTypes, json,
                            [](const QJsonObject &o, const char *key) { return o.value(key).toString(); });
                    json[s_jsonKPluginKey] = kplugin;
                }
                d->metaData = KPluginMetaData(json, libraryPath);
                break;
            }
        }
    }
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

    if (service->isDeleted()) {
        d->hidden = true;
        return;
    }

    QJsonObject json;
    QJsonObject kplugin = mapToJsonKPluginKey(service->name(), service->comment(),
            service->property(s_dependenciesKey).toStringList(), service->serviceTypes(), service,
            [](const KService::Ptr service, const char *key) { return service->property(key).toString(); });
    json[s_jsonKPluginKey] = kplugin;

    d->metaData = KPluginMetaData(json, service->library());
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
    return d->metaData.isEnabledByDefault();
}

QString KPluginInfo::name() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.name();
}

QString KPluginInfo::comment() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.description();
}

QString KPluginInfo::icon() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.iconName();
}

QString KPluginInfo::entryPath() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->entryPath;
}

QString KPluginInfo::author() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    const QList<KAboutPerson> &authors = d->metaData.authors();
    return authors.isEmpty() ? QString() : authors[0].name();
}

QString KPluginInfo::email() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    const QList<KAboutPerson> &authors = d->metaData.authors();
    return authors.isEmpty() ? QString() : authors[0].emailAddress();
}

QString KPluginInfo::category() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.category();
}

QString KPluginInfo::pluginName() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.pluginId();
}

QString KPluginInfo::libraryPath() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.fileName();
}

QString KPluginInfo::version() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.version();
}

QString KPluginInfo::website() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.website();
}

QString KPluginInfo::license() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.license();
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
    return d->metaData.dependencies();
}

QStringList KPluginInfo::serviceTypes() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.serviceTypes();
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
    return d->metaData.rawData().value(key).toVariant();
}

QVariantMap KPluginInfo::properties() const
{
    return d->metaData.rawData().toVariantMap();
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
    KPluginInfo ret;
    return KPluginInfo(md);
}

KPluginMetaData KPluginInfo::toMetaData() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData;
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
