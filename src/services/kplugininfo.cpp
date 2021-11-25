/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2003, 2007 Matthias Kretz <kretz@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kplugininfo.h"

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
#include "servicesdebug.h"
#include <QDirIterator>
#include <QJsonArray>
#include <QMimeDatabase>
#include <QStandardPaths>

#include "ksycoca.h"
#include "ksycoca_p.h"
#include <KAboutData>
#include <KDesktopFile>
#include <KPluginMetaData>
#include <kservicetypefactory_p.h>
#include <kservicetypetrader.h>

// clang-format off
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

#define GlobalQStringLiteral(name, str) inline QString name() { return QStringLiteral(str); }

namespace {

GlobalQStringLiteral(s_hiddenKey, "Hidden")
GlobalQStringLiteral(s_nameKey, "Name")
GlobalQStringLiteral(s_commentKey, "Comment")
GlobalQStringLiteral(s_iconKey, "Icon")
GlobalQStringLiteral(s_libraryKey, "X-KDE-Library")
GlobalQStringLiteral(s_authorKey, "X-KDE-PluginInfo-Author")
GlobalQStringLiteral(s_emailKey, "X-KDE-PluginInfo-Email")
GlobalQStringLiteral(s_pluginNameKey, "X-KDE-PluginInfo-Name")
GlobalQStringLiteral(s_versionKey, "X-KDE-PluginInfo-Version")
GlobalQStringLiteral(s_websiteKey, "X-KDE-PluginInfo-Website")
GlobalQStringLiteral(s_categoryKey, "X-KDE-PluginInfo-Category")
GlobalQStringLiteral(s_licenseKey, "X-KDE-PluginInfo-License")
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
GlobalQStringLiteral(s_dependenciesKey, "X-KDE-PluginInfo-Depends")
#endif
GlobalQStringLiteral(s_serviceTypesKey, "ServiceTypes")
GlobalQStringLiteral(s_xKDEServiceTypes, "X-KDE-ServiceTypes")
GlobalQStringLiteral(s_mimeTypeKey, "MimeType")
GlobalQStringLiteral(s_formFactorsKey, "X-KDE-FormFactors")
GlobalQStringLiteral(s_enabledbyDefaultKey, "X-KDE-PluginInfo-EnabledByDefault")
GlobalQStringLiteral(s_enabledKey, "Enabled")

// these keys are used in the json metadata
GlobalQStringLiteral(s_jsonDescriptionKey, "Description")
GlobalQStringLiteral(s_jsonAuthorsKey, "Authors")
GlobalQStringLiteral(s_jsonEmailKey, "Email")
GlobalQStringLiteral(s_jsonCategoryKey, "Category")
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
GlobalQStringLiteral(s_jsonDependenciesKey, "Dependencies")
#endif
GlobalQStringLiteral(s_jsonEnabledByDefaultKey, "EnabledByDefault")
GlobalQStringLiteral(s_jsonFormFactorsKey, "FormFactors")
GlobalQStringLiteral(s_jsonLicenseKey, "License")
GlobalQStringLiteral(s_jsonIdKey, "Id")
GlobalQStringLiteral(s_jsonVersionKey, "Version")
GlobalQStringLiteral(s_jsonWebsiteKey, "Website")
GlobalQStringLiteral(s_jsonMimeTypesKey, "MimeTypes")
GlobalQStringLiteral(s_jsonKPluginKey, "KPlugin")

}
// clang-format on

class KPluginInfoPrivate : public QSharedData
{
public:
    KPluginInfoPrivate()
        : hidden(false)
        , pluginenabled(false)
        , kcmservicesCached(false)
    {
    }

    static QStringList deserializeList(const QString &data);

    bool hidden : 1;
    bool pluginenabled : 1;
    mutable bool kcmservicesCached : 1;

    KPluginMetaData metaData;
    KConfigGroup config;
    KService::Ptr service;
    mutable QList<KService::Ptr> kcmservices;

    /** assigns the @p md to @c metaData, but also ensures that compatibility values are handled */
    void setMetaData(const KPluginMetaData &md, bool warnOnOldStyle);
};

// This comes from KConfigGroupPrivate::deserializeList()
QStringList KPluginInfoPrivate::deserializeList(const QString &data)
{
    if (data.isEmpty()) {
        return QStringList();
    }
    if (data == QLatin1String("\\0")) {
        return QStringList(QString());
    }
    QStringList value;
    QString val;
    val.reserve(data.size());
    bool quoted = false;
    for (int p = 0; p < data.length(); p++) {
        if (quoted) {
            val += data[p];
            quoted = false;
        } else if (data[p].unicode() == '\\') {
            quoted = true;
        } else if (data[p].unicode() == ',' || data[p].unicode() == ';') {
            val.squeeze(); // release any unused memory
            value.append(val);
            val.clear();
            val.reserve(data.size() - p);
        } else {
            val += data[p];
        }
    }
    value.append(val);
    return value;
}

// maps the KService, QVariant and KDesktopFile keys to the new KPluginMetaData keys
template<typename T, typename Func>
static QJsonObject mapToJsonKPluginKey(const QString &name,
                                       const QString &description,
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
                                       const QStringList &dependencies,
#endif
                                       const QStringList &serviceTypes,
                                       const QStringList &formFactors,
                                       const T &data,
                                       Func accessor)
{
    /* Metadata structure is as follows:
     "KPlugin": {
        "Name": "Date and Time",
        "Description": "Date and time by timezone",
        "Icon": "preferences-system-time",
        "Authors": { "Name": "Aaron Seigo", "Email": "aseigo@kde.org" },
        "Category": "Date and Time",
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
        "Dependencies": [],
#endif
        "EnabledByDefault": "true",
        "License": "LGPL",
        "Id": "time",
        "Version": "1.0",
        "Website": "http://plasma.kde.org/",
        "ServiceTypes": ["Plasma/DataEngine"]
        "FormFactors": ["tablet", "handset"]
    }
     */
    QJsonObject kplugin;
    kplugin[s_nameKey()] = name;
    kplugin[s_jsonDescriptionKey()] = description;
    kplugin[s_iconKey()] = accessor(data, s_iconKey());
    QJsonObject authors;
    authors[s_nameKey()] = accessor(data, s_authorKey());
    authors[s_jsonEmailKey()] = accessor(data, s_emailKey());
    kplugin[s_jsonAuthorsKey()] = authors;
    kplugin[s_jsonCategoryKey()] = accessor(data, s_categoryKey());
    QJsonValue enabledByDefault = accessor(data, s_enabledbyDefaultKey());
    // make sure that enabledByDefault is bool and not string
    if (!enabledByDefault.isBool()) {
        enabledByDefault = enabledByDefault.toString().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    }
    kplugin[s_jsonEnabledByDefaultKey()] = enabledByDefault;
    kplugin[s_jsonLicenseKey()] = accessor(data, s_licenseKey());
    kplugin[s_jsonIdKey()] = accessor(data, s_pluginNameKey());
    kplugin[s_jsonVersionKey()] = accessor(data, s_versionKey());
    kplugin[s_jsonWebsiteKey()] = accessor(data, s_websiteKey());
    kplugin[s_jsonFormFactorsKey()] = QJsonArray::fromStringList(formFactors);
    kplugin[s_serviceTypesKey()] = QJsonArray::fromStringList(serviceTypes);
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
    kplugin[s_jsonDependenciesKey()] = QJsonArray::fromStringList(dependencies);
#endif
    QJsonValue mimeTypes = accessor(data, s_mimeTypeKey());
    if (mimeTypes.isString()) {
        QStringList mimeList = KPluginInfoPrivate::deserializeList(mimeTypes.toString());
        if (!mimeList.isEmpty()) {
            mimeTypes = QJsonArray::fromStringList(mimeList);
        } else {
            mimeTypes = QJsonValue();
        }
    }
    kplugin[s_jsonMimeTypesKey()] = mimeTypes;
    return kplugin;
}

// TODO: KF6 remove
static KPluginMetaData fromCompatibilityJson(const QJsonObject &json, const QString &lib, const QString &metaDataFile, bool warnOnOldStyle)
{
    // This is not added to KPluginMetaData(QJsonObject, QString) to ensure that all the compatility code
    // remains in kservice and does not increase the size of kcoreaddons
    QStringList serviceTypes = KPluginMetaData::readStringList(json, s_xKDEServiceTypes());
    if (serviceTypes.isEmpty()) {
        serviceTypes = KPluginMetaData::readStringList(json, s_serviceTypesKey());
    }
    QJsonObject obj = json;
    QString name = KPluginMetaData::readTranslatedString(json, s_nameKey());
    if (warnOnOldStyle) {
        qWarning(
            "Constructing a KPluginInfo object from old style JSON. Please use"
            " kcoreaddons_desktop_to_json() for \"%s\" instead of kservice_desktop_to_json()"
            " in your CMake code.",
            qPrintable(lib));
    }
    QString description = KPluginMetaData::readTranslatedString(json, s_commentKey());
    QStringList formfactors = KPluginMetaData::readStringList(json, s_jsonFormFactorsKey());
    QJsonObject kplugin = mapToJsonKPluginKey(name,
                                              description,
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
                                              KPluginMetaData::readStringList(json, s_dependenciesKey()),
#endif
                                              serviceTypes,
                                              formfactors,
                                              json,
                                              [](const QJsonObject &o, const QString &key) {
                                                  return o.value(key);
                                              });
    obj.insert(s_jsonKPluginKey(), kplugin);
    return KPluginMetaData(obj, lib, metaDataFile);
}

void KPluginInfoPrivate::setMetaData(const KPluginMetaData &md, bool warnOnOldStyle)
{
    const QJsonObject json = md.rawData();
    if (!json.contains(s_jsonKPluginKey())) {
        // "KPlugin" key does not exists -> convert from compatibility mode
        metaData = fromCompatibilityJson(json, md.fileName(), md.metaDataFileName(), warnOnOldStyle);
    } else {
        metaData = md;
    }
}

KPluginInfo::KPluginInfo(const KPluginMetaData &md)
    : d(new KPluginInfoPrivate)
{
    d->setMetaData(md, true);
    if (!d->metaData.isValid()) {
        d.reset();
    }
}

KPluginInfo::KPluginInfo(const QString &filename /*, QStandardPaths::StandardLocation resource*/)
    : d(new KPluginInfoPrivate)
{
    KDesktopFile file(/*resource,*/ filename);

    KConfigGroup cg = file.desktopGroup();
    if (!cg.exists()) {
        qCWarning(SERVICES) << filename << "has no desktop group, cannot construct a KPluginInfo object from it.";
        d.reset();
        return;
    }
    d->hidden = cg.readEntry(s_hiddenKey(), false);
    if (d->hidden) {
        return;
    }

    d->setMetaData(KPluginMetaData(file.fileName()), true);
    if (!d->metaData.isValid()) {
        qCWarning(SERVICES) << "Failed to read metadata from .desktop file" << file.fileName();
        d.reset();
    }
}

KPluginInfo::KPluginInfo(const QVariantList &args, const QString &libraryPath)
    : d(new KPluginInfoPrivate)
{
    const QString metaData = QStringLiteral("MetaData");
    for (const QVariant &v : args) {
        if (v.canConvert<QVariantMap>()) {
            const QVariantMap &m = v.toMap();
            const QVariant &_metadata = m.value(metaData);
            if (_metadata.canConvert<QVariantMap>()) {
                const QVariantMap &map = _metadata.toMap();
                if (map.value(s_hiddenKey()).toBool()) {
                    d->hidden = true;
                    break;
                }
                d->setMetaData(KPluginMetaData(QJsonObject::fromVariantMap(map), libraryPath), true);
                break;
            }
        }
    }
    if (!d->metaData.isValid()) {
        d.reset();
    }
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 88)
KPluginInfo::KPluginInfo(const KService::Ptr service)
    : d(new KPluginInfoPrivate)
{
    if (!service) {
        d = nullptr; // isValid() == false
        return;
    }
    d->service = service;
    if (service->isDeleted()) {
        d->hidden = true;
        return;
    }

    KSycoca::self()->ensureCacheValid();

    QJsonObject json;
    const auto propertyList = service->propertyNames();
    for (const QString &key : propertyList) {
        QVariant::Type t = KSycocaPrivate::self()->serviceTypeFactory()->findPropertyTypeByName(key);
        if (t == QVariant::Invalid) {
            t = QVariant::String; // default to string if the type is not known
        }
        QVariant v = service->property(key, t);
        if (v.isValid()) {
            json[key] = QJsonValue::fromVariant(v);
        }
    }
    // reintroduce the separation between MimeType= and X-KDE-ServiceTypes=
    // we could do this by modifying KService and KSyCoCa, but as this is only compatibility
    // code we just query QMimeDatabase whether a ServiceType is a valid MIME type.
    const QStringList services = service->serviceTypes();
    if (!services.isEmpty()) {
        QMimeDatabase db;
        QStringList mimeTypes;
        mimeTypes.reserve(services.size());
        QStringList newServiceTypes;
        newServiceTypes.reserve(services.size());
        for (const QString &s : services) {
            // If we have a valid mime type of a wildcard, we know it is a mime type and not a service type
            // this is for example required in the thumbnailers.
            // Also the conversion code in kcoreaddons does not check the validity.
            if (db.mimeTypeForName(s).isValid() || s.contains(QLatin1Char('*'))) {
                mimeTypes << s;
            } else {
                newServiceTypes << s;
            }
        }
        json[s_mimeTypeKey()] = QJsonArray::fromStringList(mimeTypes);
        json[s_xKDEServiceTypes()] = QJsonArray::fromStringList(newServiceTypes);
        json[s_serviceTypesKey()] = QJsonValue();
    }

    d->setMetaData(KPluginMetaData(json, service->library(), service->entryPath()), false);
    if (!d->metaData.isValid()) {
        d.reset();
    }
}
#endif

KPluginInfo::KPluginInfo()
    : d(nullptr) // isValid() == false
{
}

bool KPluginInfo::isValid() const
{
    return d.data() != nullptr;
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

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 88)
QList<KPluginInfo> KPluginInfo::fromServices(const KService::List &services, const KConfigGroup &config)
{
    QList<KPluginInfo> infolist;
    for (KService::List::ConstIterator it = services.begin(); it != services.end(); ++it) {
        KPluginInfo info(*it);
        if (info.isValid()) {
            info.setConfig(config);
            infolist += info;
        }
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

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 81)
QList<KPluginInfo> KPluginInfo::fromKPartsInstanceName(const QString &name, const KConfigGroup &config)
{
    QStringList files;
    const QStringList dirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, name + QLatin1String("/kpartplugins"), QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.desktop"));
        while (it.hasNext()) {
            files.append(it.next());
        }
    }
    return fromFiles(files, config);
}
#endif

bool KPluginInfo::isHidden() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->hidden;
}

void KPluginInfo::setPluginEnabled(bool enabled)
{
    KPLUGININFO_ISVALID_ASSERTION;
    // qDebug() << Q_FUNC_INFO;
    d->pluginenabled = enabled;
}

bool KPluginInfo::isPluginEnabled() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    // qDebug() << Q_FUNC_INFO;
    return d->pluginenabled;
}

bool KPluginInfo::isPluginEnabledByDefault() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    // qDebug() << Q_FUNC_INFO;
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
    return d->metaData.metaDataFileName();
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

QStringList KPluginInfo::formFactors() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData.formFactors();
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

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
QStringList KPluginInfo::dependencies() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
    QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    return d->metaData.dependencies();
    QT_WARNING_POP
}
#endif

QStringList KPluginInfo::serviceTypes() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    // KService/KPluginInfo include the MIME types in serviceTypes()
    return d->metaData.serviceTypes() + d->metaData.mimeTypes();
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 70)
KService::Ptr KPluginInfo::service() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->service;
}
#endif

QList<KService::Ptr> KPluginInfo::kcmServices() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    if (!d->kcmservicesCached) {
        d->kcmservices =
            KServiceTypeTrader::self()->query(QStringLiteral("KCModule"), QLatin1Char('\'') + pluginName() + QLatin1String("' in [X-KDE-ParentComponents]"));
        // qDebug() << "found" << d->kcmservices.count() << "offers for" << d->pluginName;

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
    QVariant result = d->metaData.rawData().value(key).toVariant();
    if (result.isValid()) {
        KSycoca::self()->ensureCacheValid();
        const QVariant::Type t = KSycocaPrivate::self()->serviceTypeFactory()->findPropertyTypeByName(key);

        // special case if we want a stringlist: split values by ',' or ';' and construct the list
        if (t == QVariant::StringList) {
            if (result.canConvert<QString>()) {
                result = KPluginInfoPrivate::deserializeList(result.toString());
            } else if (result.canConvert<QVariantList>()) {
                const QVariantList list = result.toList();
                QStringList newResult;
                for (const QVariant &value : list) {
                    newResult += value.toString();
                }
                result = newResult;
            } else {
                qCWarning(SERVICES) << "Cannot interpret" << result << "into a string list";
            }
        }
        return result;
    }

    // If the key was not found check compatibility for old key names and print a warning
    // These warnings should only happen if JSON was generated with kcoreaddons_desktop_to_json
    // but the application uses KPluginTrader::query() instead of KPluginLoader::findPlugins()
    // TODO: KF6 remove
#define RETURN_WITH_DEPRECATED_WARNING(ret)                                                                                                                    \
    qWarning("Calling KPluginInfo::property(\"%s\") is deprecated, use KPluginInfo::" #ret " in \"%s\" instead.",                                              \
             qPrintable(key),                                                                                                                                  \
             qPrintable(d->metaData.fileName()));                                                                                                              \
    return ret;
    if (key == s_authorKey()) {
        RETURN_WITH_DEPRECATED_WARNING(author());
    } else if (key == s_categoryKey()) {
        RETURN_WITH_DEPRECATED_WARNING(category());
    } else if (key == s_commentKey()) {
        RETURN_WITH_DEPRECATED_WARNING(comment());
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
    } else if (key == s_dependenciesKey()) {
        RETURN_WITH_DEPRECATED_WARNING(dependencies());
#endif
    } else if (key == s_emailKey()) {
        RETURN_WITH_DEPRECATED_WARNING(email());
    } else if (key == s_enabledbyDefaultKey()) {
        RETURN_WITH_DEPRECATED_WARNING(isPluginEnabledByDefault());
    } else if (key == s_libraryKey()) {
        RETURN_WITH_DEPRECATED_WARNING(libraryPath());
    } else if (key == s_licenseKey()) {
        RETURN_WITH_DEPRECATED_WARNING(license());
    } else if (key == s_nameKey()) {
        RETURN_WITH_DEPRECATED_WARNING(name());
    } else if (key == s_pluginNameKey()) {
        RETURN_WITH_DEPRECATED_WARNING(pluginName());
    } else if (key == s_serviceTypesKey()) {
        RETURN_WITH_DEPRECATED_WARNING(serviceTypes());
    } else if (key == s_versionKey()) {
        RETURN_WITH_DEPRECATED_WARNING(version());
    } else if (key == s_websiteKey()) {
        RETURN_WITH_DEPRECATED_WARNING(website());
    } else if (key == s_xKDEServiceTypes()) {
        RETURN_WITH_DEPRECATED_WARNING(serviceTypes());
    } else if (key == s_formFactorsKey()) {
        RETURN_WITH_DEPRECATED_WARNING(formFactors());
    }
#undef RETURN_WITH_DEPRECATED_WARNING
    // not a compatibility key -> return invalid QVariant
    return result;
}

QVariantMap KPluginInfo::properties() const
{
    return d->metaData.rawData().toVariantMap();
}

void KPluginInfo::save(KConfigGroup config)
{
    KPLUGININFO_ISVALID_ASSERTION;
    // qDebug() << Q_FUNC_INFO;
    if (config.isValid()) {
        config.writeEntry(pluginName() + s_enabledKey(), isPluginEnabled());
    } else {
        if (!d->config.isValid()) {
            qCWarning(SERVICES) << "no KConfigGroup, cannot save";
            return;
        }
        d->config.writeEntry(pluginName() + s_enabledKey(), isPluginEnabled());
    }
}

void KPluginInfo::load(const KConfigGroup &config)
{
    KPLUGININFO_ISVALID_ASSERTION;
    // qDebug() << Q_FUNC_INFO;
    if (config.isValid()) {
        setPluginEnabled(config.readEntry(pluginName() + s_enabledKey(), isPluginEnabledByDefault()));
    } else {
        if (!d->config.isValid()) {
            qCWarning(SERVICES) << "no KConfigGroup, cannot load";
            return;
        }
        setPluginEnabled(d->config.readEntry(pluginName() + s_enabledKey(), isPluginEnabledByDefault()));
    }
}

void KPluginInfo::defaults()
{
    // qDebug() << Q_FUNC_INFO;
    setPluginEnabled(isPluginEnabledByDefault());
}

uint qHash(const KPluginInfo &p)
{
    return qHash(reinterpret_cast<quint64>(p.d.data()));
}

KPluginInfo KPluginInfo::fromMetaData(const KPluginMetaData &md)
{
    return KPluginInfo(md);
}

KPluginMetaData KPluginInfo::toMetaData() const
{
    KPLUGININFO_ISVALID_ASSERTION;
    return d->metaData;
}

KPluginMetaData KPluginInfo::toMetaData(const KPluginInfo &info)
{
    return info.toMetaData();
}

KPluginInfo::List KPluginInfo::fromMetaData(const QVector<KPluginMetaData> &list)
{
    KPluginInfo::List ret;
    ret.reserve(list.size());
    for (const KPluginMetaData &md : list) {
        ret.append(KPluginInfo::fromMetaData(md));
    }
    return ret;
}

QVector<KPluginMetaData> KPluginInfo::toMetaData(const KPluginInfo::List &list)
{
    QVector<KPluginMetaData> ret;
    ret.reserve(list.size());
    for (const KPluginInfo &info : list) {
        ret.append(info.toMetaData());
    }
    return ret;
}

#undef KPLUGININFO_ISVALID_ASSERTION
#endif
