/* This file is part of the KDE libraries
   Copyright (C) 2000 Torben Weis <weis@kde.org>
   Copyright (C) 2006-2020 David Faure <faure@kde.org>

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

#ifndef KAPPLICATIONTRADER_H
#define KAPPLICATIONTRADER_H

#include <kservice.h>
#include <functional>

/**
 * @class KApplicationTrader kapplicationtrader.h <KApplicationTrader>
 *
 * The application trader is a convenient way to find installed applications
 * based on specific criteria (association with a mimetype, name contains Foo, etc.)
 *
 * Example: say that you want to get the list of all applications that can handle PNG images.
 * The code would look like:
 * \code
 * KService::List lst = KApplicationTrader::queryByMimeType("image/png");
 * \endcode
 *
 * If you want to get the preferred application for image/png you would use:
 * @code
 * KService::Ptr service = KApplicationTrader::preferredService("image/png");
 * @endcode
 *
 * @see KService
 */
namespace KApplicationTrader
{
    /**
     * Filter function, used for filtering results of query and queryByMimeType.
     */
    using FilterFunc = std::function<bool(const KService::Ptr &)>;

    /**
     * This method returns a list of services (applications) which are associated with a given mimetype.
     *
     * @param filter a callback function that returns @c true if the application
     * should be selected and @c false if it should be skipped.
     *
     * @return A list of services that satisfy the query
     * @since 5.68
     */
    KSERVICE_EXPORT KService::List query(FilterFunc filterFunc);

    /**
     * This method returns a list of services (applications) which are associated with a given mimetype.
     *
     * @param mimeType A mime type like 'text/plain' or 'text/html'.
     * @param filter a callback function that returns @c true if the application
     * should be selected and @c false if it should be skipped. Do not return
     * true for all services, this would return the complete list of all
     * installed applications (slow).
     *
     * @return A list of services that satisfy the query, sorted by preference
     * (preferred service first)
     * @since 5.68
     */
    KSERVICE_EXPORT KService::List queryByMimeType(const QString &mimeType,
                                                   FilterFunc filterFunc = {});

    /**
     * Returns the preferred service for @p mimeType
     *
     * This a convenience method for queryByMimeType(mimeType).at(0), with a check for empty.
     *
     * @param mimeType the mime type (see query())
     * @return the preferred service, or @c nullptr if no service is available
     * @since 5.68
     */
    KSERVICE_EXPORT KService::Ptr preferredService(const QString &mimeType);


    /**
     * Returns true if @p pattern matches a subsequence of the string @p text.
     * For instance the pattern "libremath" matches the text "LibreOffice Math", assuming
     * @p cs is Qt::CaseInsensitive.
     *
     * This can be useful from your filter function, e.g. with @p text being service->name().
     * @since 5.68
     */
    KSERVICE_EXPORT bool isSubsequence(const QString &pattern, const QString &text, Qt::CaseSensitivity cs = Qt::CaseSensitive);

}

#endif
