/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 1999-2001 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 1999-2005 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservice.h"
#include "kmimetypefactory_p.h"
#include "kservice_p.h"
#include "ksycoca.h"
#include "ksycoca_p.h"

#include <qplatformdefs.h>

#include <QDir>
#include <QMap>
#include <QMimeDatabase>

#include <KAuthorized>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>
#include <KShell>

#include <QDebug>
#include <QStandardPaths>

#include "kservicefactory_p.h"
#include "kservicetypefactory_p.h"
#include "kserviceutil_p.h"
#include "servicesdebug.h"

QDataStream &operator<<(QDataStream &s, const KService::ServiceTypeAndPreference &st)
{
    s << st.preference << st.serviceType;
    return s;
}
QDataStream &operator>>(QDataStream &s, KService::ServiceTypeAndPreference &st)
{
    s >> st.preference >> st.serviceType;
    return s;
}

void KServicePrivate::init(const KDesktopFile *config, KService *q)
{
    const QString entryPath = q->entryPath();
    if (entryPath.isEmpty()) {
        // We are opening a "" service, this means whatever warning we might get is going to be misleading
        m_bValid = false;
        return;
    }

    bool absPath = !QDir::isRelativePath(entryPath);

    // TODO: it makes sense to have a KConstConfigGroup I guess
    const KConfigGroup desktopGroup = const_cast<KDesktopFile *>(config)->desktopGroup();
    QMap<QString, QString> entryMap = desktopGroup.entryMap();

    entryMap.remove(QStringLiteral("Encoding")); // reserved as part of Desktop Entry Standard
    entryMap.remove(QStringLiteral("Version")); // reserved as part of Desktop Entry Standard

    q->setDeleted(desktopGroup.readEntry("Hidden", false));
    entryMap.remove(QStringLiteral("Hidden"));
    if (q->isDeleted()) {
        m_bValid = false;
        return;
    }

    m_strName = config->readName();
    entryMap.remove(QStringLiteral("Name"));
    if (m_strName.isEmpty()) {
        // Try to make up a name.
        m_strName = entryPath;
        int i = m_strName.lastIndexOf(QLatin1Char('/'));
        m_strName = m_strName.mid(i + 1);
        i = m_strName.lastIndexOf(QLatin1Char('.'));
        if (i != -1) {
            m_strName.truncate(i);
        }
    }

    m_strType = config->readType();
    entryMap.remove(QStringLiteral("Type"));
    if (m_strType.isEmpty()) {
        /*kWarning(servicesDebugArea()) << "The desktop entry file " << entryPath
          << " has no Type=... entry."
          << " It should be \"Application\" or \"Service\"";
          m_bValid = false;
          return;*/
        m_strType = QStringLiteral("Application");
    } else if (m_strType != QLatin1String("Application") && m_strType != QLatin1String("Service")) {
        qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has Type=" << m_strType << "instead of \"Application\" or \"Service\"";
        m_bValid = false;
        return;
    }

    // NOT readPathEntry, it is not XDG-compliant: it performs
    // various expansions, like $HOME.  Note that the expansion
    // behaviour still happens if the "e" flag is set, maintaining
    // backwards compatibility.
    m_strExec = desktopGroup.readEntry("Exec", QString());
    entryMap.remove(QStringLiteral("Exec"));

    if (m_strType == QLatin1String("Application")) {
        // It's an application? Should have an Exec line then, otherwise we can't run it
        if (m_strExec.isEmpty()) {
            qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has Type=" << m_strType << "but no Exec line";
            m_bValid = false;
            return;
        }
    }

    // In case Try Exec is set, check if the application is available
    if (!config->tryExec()) {
        q->setDeleted(true);
        m_bValid = false;
        return;
    }

    const QStandardPaths::StandardLocation locationType = config->locationType();

    if ((m_strType == QLatin1String("Application")) && (locationType != QStandardPaths::ApplicationsLocation) && !absPath) {
        qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has Type=" << m_strType << "but is located under \""
                            << QStandardPaths::displayName(locationType) << "\" instead of \"Applications\"";
        m_bValid = false;
        return;
    }

    if ((m_strType == QLatin1String("Service")) && (locationType != QStandardPaths::GenericDataLocation) && !absPath) {
        qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has Type=" << m_strType << "but is located under \""
                            << QStandardPaths::displayName(locationType) << "\" instead of \"Shared Data\"/kservices5";
        m_bValid = false;
        return;
    }

    // entryPath To desktopEntryName
    // (e.g. "/home/x/.qttest/share/kservices5/fakepart2.desktop" --> "fakepart2")
    QString _name = KServiceUtilPrivate::completeBaseName(entryPath);

    m_strIcon = config->readIcon();
    entryMap.remove(QStringLiteral("Icon"));
    m_bTerminal = desktopGroup.readEntry("Terminal", false); // should be a property IMHO
    entryMap.remove(QStringLiteral("Terminal"));
    m_strTerminalOptions = desktopGroup.readEntry("TerminalOptions"); // should be a property IMHO
    entryMap.remove(QStringLiteral("TerminalOptions"));
    m_strWorkingDirectory = KShell::tildeExpand(config->readPath());
    entryMap.remove(QStringLiteral("Path"));
    m_strComment = config->readComment();
    entryMap.remove(QStringLiteral("Comment"));
    m_strGenName = config->readGenericName();
    entryMap.remove(QStringLiteral("GenericName"));
    QString _untranslatedGenericName = desktopGroup.readEntryUntranslated("GenericName");
    if (!_untranslatedGenericName.isEmpty()) {
        entryMap.insert(QStringLiteral("UntranslatedGenericName"), _untranslatedGenericName);
    }

    m_lstFormFactors = desktopGroup.readEntry("X-KDE-FormFactors", QStringList());
    entryMap.remove(QStringLiteral("X-KDE-FormFactors"));

    m_lstKeywords = desktopGroup.readXdgListEntry("Keywords", QStringList());
    entryMap.remove(QStringLiteral("Keywords"));
    m_lstKeywords += desktopGroup.readEntry("X-KDE-Keywords", QStringList());
    entryMap.remove(QStringLiteral("X-KDE-Keywords"));
    categories = desktopGroup.readXdgListEntry("Categories");
    entryMap.remove(QStringLiteral("Categories"));
    // TODO KDE5: only care for X-KDE-Library in Type=Service desktop files
    // This will prevent people defining a part and an app in the same desktop file
    // which makes user-preference handling difficult.
    m_strLibrary = desktopGroup.readEntry("X-KDE-Library");
    entryMap.remove(QStringLiteral("X-KDE-Library"));
    if (!m_strLibrary.isEmpty() && m_strType == QLatin1String("Application")) {
        qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has Type=" << m_strType
                            << "but also has a X-KDE-Library key. This works for now,"
                               " but makes user-preference handling difficult, so support for this might"
                               " be removed at some point. Consider splitting it into two desktop files.";
    }

    QStringList lstServiceTypes = desktopGroup.readEntry("ServiceTypes", QStringList());
    entryMap.remove(QStringLiteral("ServiceTypes"));
    lstServiceTypes += desktopGroup.readEntry("X-KDE-ServiceTypes", QStringList());
    entryMap.remove(QStringLiteral("X-KDE-ServiceTypes"));
    lstServiceTypes += desktopGroup.readXdgListEntry("MimeType");
    entryMap.remove(QStringLiteral("MimeType"));

    if (m_strType == QLatin1String("Application") && !lstServiceTypes.contains(QLatin1String("Application")))
    // Applications implement the service type "Application" ;-)
    {
        lstServiceTypes += QStringLiteral("Application");
    }

    m_initialPreference = desktopGroup.readEntry("InitialPreference", 1);
    entryMap.remove(QStringLiteral("InitialPreference"));

    // Assign the "initial preference" to each mimetype/servicetype
    // (and to set such preferences in memory from kbuildsycoca)
    m_serviceTypes.reserve(lstServiceTypes.size());
    QListIterator<QString> st_it(lstServiceTypes);
    while (st_it.hasNext()) {
        const QString st = st_it.next();
        if (st.isEmpty()) {
            qCWarning(SERVICES) << "The desktop entry file" << entryPath << "has an empty MimeType!";
            continue;
        }
        int initialPreference = m_initialPreference;
        if (st_it.hasNext()) {
            // TODO better syntax - separate group with mimetype=number entries?
            bool isNumber;
            const int val = st_it.peekNext().toInt(&isNumber);
            if (isNumber) {
                initialPreference = val;
                st_it.next();
            }
        }
        m_serviceTypes.push_back(KService::ServiceTypeAndPreference(initialPreference, st));
    }

    if (entryMap.contains(QLatin1String("Actions"))) {
        parseActions(config, q);
    }

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
    QString dbusStartupType = desktopGroup.readEntry("X-DBUS-StartupType").toLower();
    entryMap.remove(QStringLiteral("X-DBUS-StartupType"));
    if (dbusStartupType == QLatin1String("unique")) {
        m_DBUSStartusType = KService::DBusUnique;
    } else if (dbusStartupType == QLatin1String("multi")) {
        m_DBUSStartusType = KService::DBusMulti;
    } else {
        m_DBUSStartusType = KService::DBusNone;
    }
#endif

    m_strDesktopEntryName = _name;

    // Exec lines from the KCMs always have the pattern "<program> m_strDesktopEntryName", see https://phabricator.kde.org/T13729
    const static bool hasSystemsettings = !QStandardPaths::findExecutable(QStringLiteral("systemsettings5")).isEmpty();
    if (m_strExec.isEmpty() && serviceTypes().contains(QStringLiteral("KCModule"))) {
        if (desktopGroup.readEntry("X-KDE-ParentApp") == QLatin1String("kinfocenter")) {
            m_strExec = QStringLiteral("kinfocenter ") + m_strDesktopEntryName;
        } else if (desktopGroup.readEntry("X-KDE-ParentApp") == QLatin1String("kcontrol")) {
            if (!desktopGroup.readEntry("X-KDE-System-Settings-Parent-Category").isEmpty() && hasSystemsettings) {
                m_strExec = QStringLiteral("systemsettings5 ") + m_strDesktopEntryName;
            } else {
                m_strExec = QStringLiteral("kcmshell5 ") + m_strDesktopEntryName;
            }
        }
    }
    m_bAllowAsDefault = desktopGroup.readEntry("AllowDefault", true);
    entryMap.remove(QStringLiteral("AllowDefault"));

    // allow plugin users to translate categories without needing a separate key
    auto entryIt = entryMap.find(QStringLiteral("X-KDE-PluginInfo-Category"));
    if (entryIt != entryMap.end()) {
        const QString &key = entryIt.key();
        m_mapProps.insert(key, QVariant(desktopGroup.readEntryUntranslated(key)));
        m_mapProps.insert(key + QLatin1String("-Translated"), QVariant(entryIt.value()));
        entryMap.erase(entryIt);
    }

    // Store all additional entries in the property map.
    // A QMap<QString,QString> would be easier for this but we can't
    // break BC, so we have to store it in m_mapProps.
    //  qDebug("Path = %s", entryPath.toLatin1().constData());
    auto it = entryMap.constBegin();
    for (; it != entryMap.constEnd(); ++it) {
        const QString key = it.key();
        // do not store other translations like Name[fr]; kbuildsycoca will rerun if we change languages anyway
        if (!key.contains(QLatin1Char('['))) {
            // qCDebug(SERVICES) << "  Key =" << key << " Data =" << it.value();
            if (key == QLatin1String("X-Flatpak-RenamedFrom")) {
                m_mapProps.insert(key, desktopGroup.readXdgListEntry(key));
            } else {
                m_mapProps.insert(key, QVariant(it.value()));
            }
        }
    }
}

void KServicePrivate::parseActions(const KDesktopFile *config, KService *q)
{
    const QStringList keys = config->readActions();
    if (keys.isEmpty()) {
        return;
    }

    KService::Ptr serviceClone(new KService(*q));

    for (const QString &group : keys) {
        if (group == QLatin1String("_SEPARATOR_")) {
            m_actions.append(KServiceAction(group, QString(), QString(), QString(), false, serviceClone));
            continue;
        }

        if (config->hasActionGroup(group)) {
            const KConfigGroup cg = config->actionGroup(group);
            if (!cg.hasKey("Name") || !cg.hasKey("Exec")) {
                qCWarning(SERVICES) << "The action" << group << "in the desktop file" << q->entryPath() << "has no Name or no Exec key";
            } else {
                const QMap<QString, QString> entries = cg.entryMap();

                QVariantMap entriesVariants;

                for (auto it = entries.constKeyValueBegin(); it != entries.constKeyValueEnd(); ++it) {
                    // Those are stored separately
                    if (it->first == QLatin1String("Name") || it->first == QLatin1String("Icon") || it->first == QLatin1String("Exec")
                        || it->first == QLatin1String("NoDisplay")) {
                        continue;
                    }

                    entriesVariants.insert(it->first, it->second);
                }

                KServiceAction action(group, cg.readEntry("Name"), cg.readEntry("Icon"), cg.readEntry("Exec"), cg.readEntry("NoDisplay", false), serviceClone);
                action.setData(QVariant::fromValue(entriesVariants));
                m_actions.append(action);
            }
        } else {
            qCWarning(SERVICES) << "The desktop file" << q->entryPath() << "references the action" << group << "but doesn't define it";
        }
    }
}

void KServicePrivate::load(QDataStream &s)
{
    qint8 def;
    qint8 term;
    qint8 dst;
    qint8 initpref;

    // WARNING: THIS NEEDS TO REMAIN COMPATIBLE WITH PREVIOUS KService 5.x VERSIONS!
    // !! This data structure should remain binary compatible at all times !!
    // You may add new fields at the end. Make sure to update KSYCOCA_VERSION
    // number in ksycoca.cpp
    // clang-format off
    s >> m_strType >> m_strName >> m_strExec >> m_strIcon
      >> term >> m_strTerminalOptions
      >> m_strWorkingDirectory >> m_strComment >> def >> m_mapProps
      >> m_strLibrary
      >> dst
      >> m_strDesktopEntryName
      >> initpref
      >> m_lstKeywords >> m_strGenName
      >> categories >> menuId >> m_actions >> m_serviceTypes
      >> m_lstFormFactors;
    // clang-format on

    m_bAllowAsDefault = bool(def);
    m_bTerminal = bool(term);
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
    m_DBUSStartusType = static_cast<KService::DBusStartupType>(dst);
#endif
    m_initialPreference = initpref;

    m_bValid = true;
}

void KServicePrivate::save(QDataStream &s)
{
    KSycocaEntryPrivate::save(s);
    qint8 def = m_bAllowAsDefault;
    qint8 initpref = m_initialPreference;
    qint8 term = m_bTerminal;
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
    qint8 dst = qint8(m_DBUSStartusType);
#else
    qint8 dst = 0;
#endif

    // WARNING: THIS NEEDS TO REMAIN COMPATIBLE WITH PREVIOUS KService 5.x VERSIONS!
    // !! This data structure should remain binary compatible at all times !!
    // You may add new fields at the end. Make sure to update KSYCOCA_VERSION
    // number in ksycoca.cpp
    s << m_strType << m_strName << m_strExec << m_strIcon << term << m_strTerminalOptions << m_strWorkingDirectory << m_strComment << def << m_mapProps
      << m_strLibrary << dst << m_strDesktopEntryName << initpref << m_lstKeywords << m_strGenName << categories << menuId << m_actions << m_serviceTypes
      << m_lstFormFactors;
}

////

KService::KService(const QString &_name, const QString &_exec, const QString &_icon)
    : KSycocaEntry(*new KServicePrivate(QString()))
{
    Q_D(KService);
    d->m_strType = QStringLiteral("Application");
    d->m_strName = _name;
    d->m_strExec = _exec;
    d->m_strIcon = _icon;
    d->m_bTerminal = false;
    d->m_bAllowAsDefault = true;
    d->m_initialPreference = 10;
}

KService::KService(const QString &_fullpath)
    : KSycocaEntry(*new KServicePrivate(_fullpath))
{
    Q_D(KService);

    KDesktopFile config(_fullpath);
    d->init(&config, this);
}

KService::KService(const KDesktopFile *config, const QString &entryPath)
    : KSycocaEntry(*new KServicePrivate(entryPath.isEmpty() ? config->fileName() : entryPath))
{
    Q_D(KService);

    d->init(config, this);
}

KService::KService(QDataStream &_str, int _offset)
    : KSycocaEntry(*new KServicePrivate(_str, _offset))
{
    Q_D(KService);
    KService::Ptr serviceClone(new KService(*this));
    for (KServiceAction &action : d->m_actions) {
        action.setService(serviceClone);
    }
}

KService::KService(const KService &other)
    : KSycocaEntry(*new KServicePrivate(*other.d_func()))
{
}

KService::~KService()
{
}

bool KService::hasServiceType(const QString &serviceType) const
{
    Q_D(const KService);

    if (!d->m_bValid) {
        return false; // (useless) safety test
    }
    const KServiceType::Ptr ptr = KServiceType::serviceType(serviceType);
    if (!ptr) {
        return false;
    }
    const int serviceOffset = offset();
    // doesn't seem to work:
    // if ( serviceOffset == 0 )
    //    serviceOffset = serviceByStorageId( storageId() );
    if (serviceOffset) {
        KSycoca::self()->ensureCacheValid();
        return KSycocaPrivate::self()->serviceFactory()->hasOffer(ptr->offset(), ptr->serviceOffersOffset(), serviceOffset);
    }

    // fall-back code for services that are NOT from ksycoca
    // For each service type we are associated with, if it doesn't
    // match then we try its parent service types.
    const QString serviceTypeName = ptr->name();
    auto matchFunc = [&serviceTypeName](const ServiceTypeAndPreference &typePref) {
        const QString &st = typePref.serviceType;
        // qCDebug(SERVICES) << "    has " << typePref;
        if (st == serviceTypeName) {
            return true;
        }

        // also the case of parent servicetypes
        KServiceType::Ptr p = KServiceType::serviceType(st);
        if (p && p->inherits(serviceTypeName)) {
            return true;
        }

        return false;
    };

    return std::any_of(d->m_serviceTypes.cbegin(), d->m_serviceTypes.cend(), matchFunc);
}

bool KService::hasMimeType(const QString &mimeType) const
{
    Q_D(const KService);
    QMimeDatabase db;
    const QString mime = db.mimeTypeForName(mimeType).name();
    if (mime.isEmpty()) {
        return false;
    }
    int serviceOffset = offset();
    if (serviceOffset) {
        KSycoca::self()->ensureCacheValid();
        KMimeTypeFactory *factory = KSycocaPrivate::self()->mimeTypeFactory();
        const int mimeOffset = factory->entryOffset(mime);
        const int serviceOffersOffset = factory->serviceOffersOffset(mime);
        if (serviceOffersOffset == -1) {
            return false;
        }
        return KSycocaPrivate::self()->serviceFactory()->hasOffer(mimeOffset, serviceOffersOffset, serviceOffset);
    }

    auto matchFunc = [&mime](const ServiceTypeAndPreference &typePref) {
        // qCDebug(SERVICES) << "    has " << typePref;
        if (typePref.serviceType == mime) {
            return true;
        }
        // TODO: should we handle inherited MIME types here?
        // KMimeType was in kio when this code was written, this is the only reason it's not done.
        // But this should matter only in a very rare case, since most code gets KServices from ksycoca.
        // Warning, change hasServiceType if you implement this here (and check kbuildservicefactory).
        return false;
    };

    // fall-back code for services that are NOT from ksycoca
    return std::any_of(d->m_serviceTypes.cbegin(), d->m_serviceTypes.cend(), matchFunc);
}

QVariant KServicePrivate::property(const QString &_name) const
{
    return property(_name, QMetaType::UnknownType);
}

// Return a string QVariant if string isn't null, and invalid variant otherwise
// (the variant must be invalid if the field isn't in the .desktop file)
// This allows trader queries like "exist Library" to work.
static QVariant makeStringVariant(const QString &string)
{
    // Using isEmpty here would be wrong.
    // Empty is "specified but empty", null is "not specified" (in the .desktop file)
    return string.isNull() ? QVariant() : QVariant(string);
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
QVariant KService::property(const QString &_name, QVariant::Type t) const
{
    Q_D(const KService);
    return d->property(_name, (QMetaType::Type)t);
}
#endif

QVariant KService::property(const QString &_name, QMetaType::Type t) const
{
    Q_D(const KService);
    return d->property(_name, t);
}

QVariant KServicePrivate::property(const QString &_name, QMetaType::Type t) const
{
    if (_name == QLatin1String("Type")) {
        return QVariant(m_strType); // can't be null
    } else if (_name == QLatin1String("Name")) {
        return QVariant(m_strName); // can't be null
    } else if (_name == QLatin1String("Exec")) {
        return makeStringVariant(m_strExec);
    } else if (_name == QLatin1String("Icon")) {
        return makeStringVariant(m_strIcon);
    } else if (_name == QLatin1String("Terminal")) {
        return QVariant(m_bTerminal);
    } else if (_name == QLatin1String("TerminalOptions")) {
        return makeStringVariant(m_strTerminalOptions);
    } else if (_name == QLatin1String("Path")) {
        return makeStringVariant(m_strWorkingDirectory);
    } else if (_name == QLatin1String("Comment")) {
        return makeStringVariant(m_strComment);
    } else if (_name == QLatin1String("GenericName")) {
        return makeStringVariant(m_strGenName);
    } else if (_name == QLatin1String("ServiceTypes")) {
        return QVariant(serviceTypes());
    } else if (_name == QLatin1String("AllowAsDefault")) {
        return QVariant(m_bAllowAsDefault);
    } else if (_name == QLatin1String("InitialPreference")) {
        return QVariant(m_initialPreference);
    } else if (_name == QLatin1String("Library")) {
        return makeStringVariant(m_strLibrary);
    } else if (_name == QLatin1String("DesktopEntryPath")) { // can't be null
        return QVariant(path);
    } else if (_name == QLatin1String("DesktopEntryName")) {
        return QVariant(m_strDesktopEntryName); // can't be null
    } else if (_name == QLatin1String("Categories")) {
        return QVariant(categories);
    } else if (_name == QLatin1String("Keywords")) {
        return QVariant(m_lstKeywords);
    } else if (_name == QLatin1String("FormFactors")) {
        return QVariant(m_lstFormFactors);
    }

    // Ok we need to convert the property from a QString to its real type.
    // Maybe the caller helped us.
    if (t == QMetaType::UnknownType) {
        // No luck, let's ask KServiceTypeFactory what the type of this property
        // is supposed to be.
        // ######### this looks in all servicetypes, not just the ones this service supports!
        KSycoca::self()->ensureCacheValid();
        t = KSycocaPrivate::self()->serviceTypeFactory()->findPropertyTypeByName(_name);
        if (t == QMetaType::UnknownType) {
            qCDebug(SERVICES) << "Request for unknown property" << _name;
            return QVariant(); // Unknown property: Invalid variant.
        }
    }

    auto it = m_mapProps.constFind(_name);
    if (it == m_mapProps.cend() || !it.value().isValid()) {
        // qCDebug(SERVICES) << "Property not found " << _name;
        return QVariant(); // No property set.
    }

    if (t == QMetaType::QString) {
        return it.value(); // no conversion necessary
    } else {
        // All others
        // For instance properties defined as StringList, like MimeTypes.
        // XXX This API is accessible only through a friend declaration.

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return KConfigGroup::convertToQVariant(_name.toUtf8().constData(), it.value().toString().toUtf8(), QVariant(static_cast<QVariant::Type>(t)));
#else
        return KConfigGroup::convertToQVariant(_name.toUtf8().constData(), it.value().toString().toUtf8(), QVariant(QMetaType(t)));
#endif
    }
}

QStringList KServicePrivate::propertyNames() const
{
    static const QStringList defaultKeys = {
        QStringLiteral("Type"),
        QStringLiteral("Name"),
        QStringLiteral("Comment"),
        QStringLiteral("GenericName"),
        QStringLiteral("Icon"),
        QStringLiteral("Exec"),
        QStringLiteral("Terminal"),
        QStringLiteral("TerminalOptions"),
        QStringLiteral("Path"),
        QStringLiteral("ServiceTypes"),
        QStringLiteral("AllowAsDefault"),
        QStringLiteral("InitialPreference"),
        QStringLiteral("Library"),
        QStringLiteral("DesktopEntryPath"),
        QStringLiteral("DesktopEntryName"),
        QStringLiteral("Keywords"),
        QStringLiteral("FormFactors"),
        QStringLiteral("Categories"),
    };

    return m_mapProps.keys() + defaultKeys;
}

KService::List KService::allServices()
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceFactory()->allServices();
}

KService::Ptr KService::serviceByDesktopPath(const QString &_name)
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceFactory()->findServiceByDesktopPath(_name);
}

KService::Ptr KService::serviceByDesktopName(const QString &_name)
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceFactory()->findServiceByDesktopName(_name);
}

KService::Ptr KService::serviceByMenuId(const QString &_name)
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceFactory()->findServiceByMenuId(_name);
}

KService::Ptr KService::serviceByStorageId(const QString &_storageId)
{
    KSycoca::self()->ensureCacheValid();
    return KSycocaPrivate::self()->serviceFactory()->findServiceByStorageId(_storageId);
}

bool KService::substituteUid() const
{
    QVariant v = property(QStringLiteral("X-KDE-SubstituteUID"), QMetaType::Bool);
    return v.isValid() && v.toBool();
}

QString KService::username() const
{
    // See also KDesktopFile::tryExec()
    QString user;
    QVariant v = property(QStringLiteral("X-KDE-Username"), QMetaType::QString);
    user = v.isValid() ? v.toString() : QString();
    if (user.isEmpty()) {
        user = QString::fromLocal8Bit(qgetenv("ADMIN_ACCOUNT"));
    }
    if (user.isEmpty()) {
        user = QStringLiteral("root");
    }
    return user;
}

bool KService::showInCurrentDesktop() const
{
    Q_D(const KService);

    const QString envVar = QString::fromLatin1(qgetenv("XDG_CURRENT_DESKTOP"));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QVector<QStringView> currentDesktops = QStringView(envVar).split(QLatin1Char(':'), Qt::SkipEmptyParts);
#else
    QVector<QStringRef> currentDesktops = envVar.splitRef(QLatin1Char(':'), Qt::SkipEmptyParts);
#endif

    const QString kde = QStringLiteral("KDE");
    if (currentDesktops.isEmpty()) {
        // This could be an old display manager, or e.g. a failsafe session with no desktop name
        // In doubt, let's say we show KDE stuff.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        currentDesktops.append(kde);
#else
        currentDesktops.append(&kde);
#endif
    }

    // This algorithm is described in the desktop entry spec

    auto it = d->m_mapProps.constFind(QStringLiteral("OnlyShowIn"));
    if (it != d->m_mapProps.cend()) {
        const QVariant &val = it.value();
        if (val.isValid()) {
            const QStringList aList = val.toString().split(QLatin1Char(';'));
            return std::any_of(currentDesktops.cbegin(), currentDesktops.cend(), [&aList](const auto desktop) {
                return aList.contains(desktop);
            });
        }
    }

    it = d->m_mapProps.constFind(QStringLiteral("NotShowIn"));
    if (it != d->m_mapProps.cend()) {
        const QVariant &val = it.value();
        if (val.isValid()) {
            const QStringList aList = val.toString().split(QLatin1Char(';'));
            return std::none_of(currentDesktops.cbegin(), currentDesktops.cend(), [&aList](const auto desktop) {
                return aList.contains(desktop);
            });
        }
    }

    return true;
}

bool KService::showOnCurrentPlatform() const
{
    Q_D(const KService);
    const QString platform = QCoreApplication::instance()->property("platformName").toString();
    if (platform.isEmpty()) {
        return true;
    }

    auto it = d->m_mapProps.find(QStringLiteral("X-KDE-OnlyShowOnQtPlatforms"));
    if ((it != d->m_mapProps.end()) && (it->isValid())) {
        const QStringList aList = it->toString().split(QLatin1Char(';'));
        if (!aList.contains(platform)) {
            return false;
        }
    }

    it = d->m_mapProps.find(QStringLiteral("X-KDE-NotShowOnQtPlatforms"));
    if ((it != d->m_mapProps.end()) && (it->isValid())) {
        const QStringList aList = it->toString().split(QLatin1Char(';'));
        if (aList.contains(platform)) {
            return false;
        }
    }
    return true;
}

bool KService::noDisplay() const
{
    if (qvariant_cast<bool>(property(QStringLiteral("NoDisplay"), QMetaType::Bool))) {
        return true;
    }

    if (!showInCurrentDesktop()) {
        return true;
    }

    if (!showOnCurrentPlatform()) {
        return true;
    }

    if (!KAuthorized::authorizeControlModule(storageId())) {
        return true;
    }

    return false;
}

QString KService::untranslatedGenericName() const
{
    QVariant v = property(QStringLiteral("UntranslatedGenericName"), QMetaType::QString);
    return v.isValid() ? v.toString() : QString();
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 87)
QString KService::parentApp() const
{
    Q_D(const KService);
    auto it = d->m_mapProps.constFind(QStringLiteral("X-KDE-ParentApp"));
    if (it != d->m_mapProps.cend()) {
        const QVariant &val = it.value();
        if (val.isValid()) {
            return val.toString();
        }
    }

    return {};
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 87)
QString KService::pluginKeyword() const
{
    Q_D(const KService);
    QMap<QString, QVariant>::ConstIterator it = d->m_mapProps.find(QStringLiteral("X-KDE-PluginKeyword"));
    if ((it == d->m_mapProps.end()) || (!it->isValid())) {
        return QString();
    }

    return it->toString();
}
#endif

QString KService::docPath() const
{
    Q_D(const KService);

    for (const QString &str : {QStringLiteral("X-DocPath"), QStringLiteral("DocPath")}) {
        auto it = d->m_mapProps.constFind(str);
        if (it != d->m_mapProps.cend()) {
            const QVariant variant = it.value();
            Q_ASSERT(variant.isValid());
            const QString path = variant.toString();
            if (!path.isEmpty()) {
                return path;
            }
        }
    }

    return {};
}

bool KService::allowMultipleFiles() const
{
    Q_D(const KService);
    // Can we pass multiple files on the command line or do we have to start the application for every single file ?
    return (d->m_strExec.contains(QLatin1String("%F")) //
            || d->m_strExec.contains(QLatin1String("%U")) //
            || d->m_strExec.contains(QLatin1String("%N")) //
            || d->m_strExec.contains(QLatin1String("%D")));
}

QStringList KService::categories() const
{
    Q_D(const KService);
    return d->categories;
}

QString KService::menuId() const
{
    Q_D(const KService);
    return d->menuId;
}

void KService::setMenuId(const QString &_menuId)
{
    Q_D(KService);
    d->menuId = _menuId;
}

QString KService::storageId() const
{
    Q_D(const KService);
    return d->storageId();
}

// not sure this is still used anywhere...
QString KService::locateLocal() const
{
    Q_D(const KService);
    if (d->menuId.isEmpty() //
        || entryPath().startsWith(QLatin1String(".hidden")) //
        || (QDir::isRelativePath(entryPath()) && d->categories.isEmpty())) {
        return KDesktopFile::locateLocal(entryPath());
    }

    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/applications/") + d->menuId;
}

QString KService::newServicePath(bool showInMenu, const QString &suggestedName, QString *menuId, const QStringList *reservedMenuIds)
{
    Q_UNUSED(showInMenu); // TODO KDE5: remove argument

    QString base = suggestedName;
    QString result;
    for (int i = 1; true; i++) {
        if (i == 1) {
            result = base + QStringLiteral(".desktop");
        } else {
            result = base + QStringLiteral("-%1.desktop").arg(i);
        }

        if (reservedMenuIds && reservedMenuIds->contains(result)) {
            continue;
        }

        // Lookup service by menu-id
        KService::Ptr s = serviceByMenuId(result);
        if (s) {
            continue;
        }

        if (!QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("applications/") + result).isEmpty()) {
            continue;
        }

        break;
    }
    if (menuId) {
        *menuId = result;
    }

    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/applications/") + result;
}

bool KService::isApplication() const
{
    Q_D(const KService);
    return d->m_strType == QLatin1String("Application");
}

QString KService::exec() const
{
    Q_D(const KService);
    if (d->m_strType == QLatin1String("Application") && d->m_strExec.isEmpty()) {
        qCWarning(SERVICES) << "The desktop entry file" << entryPath() << "has Type=" << d->m_strType << "but has no Exec field.";
    }
    return d->m_strExec;
}

QString KService::library() const
{
    Q_D(const KService);
    return d->m_strLibrary;
}

QString KService::icon() const
{
    Q_D(const KService);
    return d->m_strIcon;
}

QString KService::terminalOptions() const
{
    Q_D(const KService);
    return d->m_strTerminalOptions;
}

bool KService::terminal() const
{
    Q_D(const KService);
    return d->m_bTerminal;
}

bool KService::runOnDiscreteGpu() const
{
    QVariant prop = property(QStringLiteral("PrefersNonDefaultGPU"), QMetaType::Bool);
    if (!prop.isValid()) {
        // For backwards compatibility
        prop = property(QStringLiteral("X-KDE-RunOnDiscreteGpu"), QMetaType::Bool);
    }

    return prop.isValid() && prop.toBool();
}

QString KService::desktopEntryName() const
{
    Q_D(const KService);
    return d->m_strDesktopEntryName;
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
KService::DBusStartupType KService::dbusStartupType() const
{
    Q_D(const KService);
    return d->m_DBUSStartusType;
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 63)
QString KService::path() const
{
    Q_D(const KService);
    return d->m_strWorkingDirectory;
}
#endif

QString KService::workingDirectory() const
{
    Q_D(const KService);
    return d->m_strWorkingDirectory;
}

QString KService::comment() const
{
    Q_D(const KService);
    return d->m_strComment;
}

QString KService::genericName() const
{
    Q_D(const KService);
    return d->m_strGenName;
}

QStringList KService::keywords() const
{
    Q_D(const KService);
    return d->m_lstKeywords;
}

QStringList KServicePrivate::serviceTypes() const
{
    QStringList ret;
    ret.reserve(m_serviceTypes.size());

    std::transform(m_serviceTypes.cbegin(), m_serviceTypes.cend(), std::back_inserter(ret), [](const KService::ServiceTypeAndPreference &typePref) {
        Q_ASSERT(!typePref.serviceType.isEmpty());
        return typePref.serviceType;
    });

    return ret;
}

QStringList KService::serviceTypes() const
{
    Q_D(const KService);
    return d->serviceTypes();
}

QStringList KService::mimeTypes() const
{
    Q_D(const KService);

    QMimeDatabase db;
    QStringList ret;

    for (const KService::ServiceTypeAndPreference &s : d->m_serviceTypes) {
        const QString servType = s.serviceType;
        if (db.mimeTypeForName(servType).isValid()) { // keep only mimetypes, filter out servicetypes
            ret.append(servType);
        }
    }
    return ret;
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 67)
bool KService::allowAsDefault() const
{
    Q_D(const KService);
    return d->m_bAllowAsDefault;
}
#endif

int KService::initialPreference() const
{
    Q_D(const KService);
    return d->m_initialPreference;
}

void KService::setTerminal(bool b)
{
    Q_D(KService);
    d->m_bTerminal = b;
}

void KService::setTerminalOptions(const QString &options)
{
    Q_D(KService);
    d->m_strTerminalOptions = options;
}

void KService::setExec(const QString &exec)
{
    Q_D(KService);

    if (!exec.isEmpty()) {
        d->m_strExec = exec;
        d->path.clear();
    }
}

void KService::setWorkingDirectory(const QString &workingDir)
{
    Q_D(KService);

    if (!workingDir.isEmpty()) {
        d->m_strWorkingDirectory = workingDir;
        d->path.clear();
    }
}

QVector<KService::ServiceTypeAndPreference> &KService::_k_accessServiceTypes()
{
    Q_D(KService);
    return d->m_serviceTypes;
}

QList<KServiceAction> KService::actions() const
{
    Q_D(const KService);
    return d->m_actions;
}

#if KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 86)
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 86)
KService::operator KPluginName() const
{
    if (!isValid()) {
        return KPluginName::fromErrorString(i18n("The provided service is not valid"));
    }

    if (library().isEmpty()) {
        return KPluginName::fromErrorString(i18n("The service '%1' provides no library or the Library key is missing", entryPath()));
    }

    return KPluginName(library());
}
#endif
#endif

QString KService::aliasFor() const
{
    return KServiceUtilPrivate::completeBaseName(property(QStringLiteral("X-KDE-AliasFor"), QMetaType::QString).toString());
}
