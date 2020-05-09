/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kserviceoffer.h"

class KServiceOfferPrivate
{
public:
    KServiceOfferPrivate()
        : preference(-1),
          mimeTypeInheritanceLevel(0),
          pService(nullptr)
    {
    }

    int preference;
    int mimeTypeInheritanceLevel;
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 71)
    bool bAllowAsDefault = false;
#endif
    KService::Ptr pService;
};

KServiceOffer::KServiceOffer()
    : d(new KServiceOfferPrivate)
{
}

KServiceOffer::KServiceOffer(const KServiceOffer &_o)
    : d(new KServiceOfferPrivate)
{
    *d = *_o.d;
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 71)
KServiceOffer::KServiceOffer(const KService::Ptr &_service, int _pref, int mimeTypeInheritanceLevel, bool _default)
    : d(new KServiceOfferPrivate)
{
    d->pService = _service;
    d->preference = _pref;
    d->mimeTypeInheritanceLevel = mimeTypeInheritanceLevel;
    d->bAllowAsDefault = _default;
}
#endif

KServiceOffer::KServiceOffer(const KService::Ptr &_service, int _pref, int mimeTypeInheritanceLevel)
    : d(new KServiceOfferPrivate)
{
    d->pService = _service;
    d->preference = _pref;
    d->mimeTypeInheritanceLevel = mimeTypeInheritanceLevel;
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 71)
    d->bAllowAsDefault = true;
#endif
}

KServiceOffer::~KServiceOffer()
{
    delete d;
}

KServiceOffer &KServiceOffer::operator=(const KServiceOffer &rhs)
{
    if (this == &rhs) {
        return *this;
    }

    *d = *rhs.d;
    return *this;
}

bool KServiceOffer::operator< (const KServiceOffer &_o) const
{
    // First check mimetype inheritance level.
    // Direct mimetype association is preferred above association via parent mimetype
    // So, the smaller the better.
    if (d->mimeTypeInheritanceLevel != _o.d->mimeTypeInheritanceLevel) {
        return d->mimeTypeInheritanceLevel < _o.d->mimeTypeInheritanceLevel;
    }

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 71)
    // Put offers allowed as default FIRST.
    if (_o.d->bAllowAsDefault && !d->bAllowAsDefault) {
        return false;    // _o is default and not 'this'.
    }
    if (!_o.d->bAllowAsDefault && d->bAllowAsDefault) {
        return true;    // 'this' is default but not _o.
    }
    // Both offers are allowed or not allowed as default
#endif

    // Finally, use preference to sort them
    // The bigger the better, but we want the better FIRST
    return _o.d->preference < d->preference;
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 67)
bool KServiceOffer::allowAsDefault() const
{
    return d->bAllowAsDefault;
}
#endif

int KServiceOffer::preference() const
{
    return d->preference;
}

void KServiceOffer::setPreference(int p)
{
    d->preference = p;
}

KService::Ptr KServiceOffer::service() const
{
    return d->pService;
}

bool KServiceOffer::isValid() const
{
    return d->preference >= 0;
}

void KServiceOffer::setMimeTypeInheritanceLevel(int level)
{
    d->mimeTypeInheritanceLevel = level;
}

int KServiceOffer::mimeTypeInheritanceLevel() const
{
    return d->mimeTypeInheritanceLevel;
}

