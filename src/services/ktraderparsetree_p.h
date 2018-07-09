/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#ifndef __ktrader_parse_tree_h__
#define __ktrader_parse_tree_h__

#include <QString>
#include <QStringList>
#include <QMap>

#include <kservice.h>
#include <kplugininfo.h>

namespace KTraderParse
{

class ParseTreeBase;

/**
 * @internal
 * @return 0  => Does not match
 *         1  => Does match
 *         <0 => Error
 */
int matchConstraint(const ParseTreeBase *_tree, const KService::Ptr &,
                    const KService::List &);
int matchConstraintPlugin(const ParseTreeBase *_tree, KPluginInfo _info,
                          const KPluginInfo::List &_list);

/**
 * @internal
 */
struct KSERVICE_EXPORT PreferencesMaxima {
    PreferencesMaxima()
        : iMax(0), iMin(0), fMax(0), fMin(0)
    {
    }

    enum Type { PM_ERROR, PM_INVALID_INT, PM_INVALID_DOUBLE, PM_DOUBLE, PM_INT };

    Type type;
    int iMax;
    int iMin;
    double fMax;
    double fMin;
};

/**
 * @internal
 */
class ParseContext
{
public:
    /**
     * This is NOT a copy constructor.
     */
    explicit ParseContext(const ParseContext *_ctx) : service(_ctx->service), info(_ctx->info), maxima(_ctx->maxima),
        offers(_ctx->offers), pluginOffers(_ctx->pluginOffers) {}
    ParseContext(const KService::Ptr &_service, const KService::List &_offers,
                 QMap<QString, PreferencesMaxima> &_m)
        : service(_service), info(KPluginInfo()), maxima(_m), offers(_offers), pluginOffers(KPluginInfo::List()) {}
    ParseContext(KPluginInfo _info, const KPluginInfo::List &_offers,
                 QMap<QString, PreferencesMaxima> &_m)
        : service(nullptr), info(_info), maxima(_m), offers(KService::List()), pluginOffers(_offers) {}

    bool initMaxima(const QString &_prop);

    QVariant property(const QString &_key) const;

    enum Type { T_STRING = 1, T_DOUBLE = 2, T_NUM = 3, T_BOOL = 4,
                T_STR_SEQ = 5, T_SEQ = 6
              };

    QString str;
    int i;
    double f;
    bool b;
    QList<QVariant> seq;
    QStringList strSeq;
    Type type;

    KService::Ptr service;
    KPluginInfo info;

    QMap<QString, PreferencesMaxima> &maxima;
    KService::List offers;
    KPluginInfo::List pluginOffers;
};

/**
 * @internal
 */
class ParseTreeBase : public QSharedData
{
public:
    typedef QExplicitlySharedDataPointer<ParseTreeBase> Ptr;
    ParseTreeBase() { }
    virtual ~ParseTreeBase();

    virtual bool eval(ParseContext *_context) const = 0;
};

ParseTreeBase::Ptr parseConstraints(const QString &_constr);

/**
 * @internal
 */
class ParseTreeOR : public ParseTreeBase
{
public:
    ParseTreeOR(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2)
    {
        m_pLeft = _ptr1;
        m_pRight = _ptr2;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
};

/**
 * @internal
 */
class ParseTreeAND : public ParseTreeBase
{
public:
    ParseTreeAND(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2)
    {
        m_pLeft = _ptr1;
        m_pRight = _ptr2;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
};

/**
 * @internal
 */
class ParseTreeCMP : public ParseTreeBase
{
public:
    ParseTreeCMP(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2, int _i)
    {
        m_pLeft = _ptr1;
        m_pRight = _ptr2;
        m_cmd = _i;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
    int m_cmd;
};

/**
 * @internal
 */
class ParseTreeIN : public ParseTreeBase
{
public:
    ParseTreeIN(ParseTreeBase *ptr1, ParseTreeBase *ptr2, Qt::CaseSensitivity cs, bool substring = false)
        : m_pLeft(ptr1),
          m_pRight(ptr2),
          m_cs(cs),
          m_substring(substring)
    {
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
    Qt::CaseSensitivity m_cs;
    bool m_substring;
};

/**
 * @internal
 */
class ParseTreeMATCH : public ParseTreeBase
{
public:
    ParseTreeMATCH(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2, Qt::CaseSensitivity cs)
    {
        m_pLeft = _ptr1;
        m_pRight = _ptr2;
        m_cs = cs;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
    Qt::CaseSensitivity m_cs;
};

/**
  * @internal
  *
  * A sub-sequence match is like a sub-string match except the characters
  * do not have to be contiguous. For example 'ct' is a sub-sequence of 'cat'
  * but not a sub-string. 'at' is both a sub-string and sub-sequence of 'cat'.
  * All sub-strings are sub-sequences.
  *
  * This is useful if you want to support a fuzzier search, say for instance
  * you are searching for `LibreOffice 6.0 Writer`, after typing `libre` you
  * see a list of all the LibreOffice apps, to narrow down that list you only
  * need to add `write` (so the search term is `librewrite`) instead of typing
  * out the entire app name until a distinguishing letter is reached.
  * It's also useful to allow the user to forget to type some characters.
  */
class ParseTreeSubsequenceMATCH : public ParseTreeBase
{
public:
  ParseTreeSubsequenceMATCH(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2, Qt::CaseSensitivity cs)
  {
    m_pLeft = _ptr1;
    m_pRight = _ptr2;
    m_cs = cs;
  }

  bool eval(ParseContext *_context) const override;

  static bool isSubseq(const QString& pattern, const QString& text, Qt::CaseSensitivity cs);

protected:
  ParseTreeBase::Ptr m_pLeft;
  ParseTreeBase::Ptr m_pRight;
  Qt::CaseSensitivity m_cs;
};

/**
 * @internal
 */
class ParseTreeCALC : public ParseTreeBase
{
public:
    ParseTreeCALC(ParseTreeBase *_ptr1, ParseTreeBase *_ptr2, int _i)
    {
        m_pLeft = _ptr1;
        m_pRight = _ptr2;
        m_cmd = _i;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
    ParseTreeBase::Ptr m_pRight;
    int m_cmd;
};

/**
 * @internal
 */
class ParseTreeBRACKETS : public ParseTreeBase
{
public:
    explicit ParseTreeBRACKETS(ParseTreeBase *_ptr)
    {
        m_pLeft = _ptr;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
};

/**
 * @internal
 */
class ParseTreeNOT : public ParseTreeBase
{
public:
    explicit ParseTreeNOT(ParseTreeBase *_ptr)
    {
        m_pLeft = _ptr;
    }

    bool eval(ParseContext *_context) const override;

protected:
    ParseTreeBase::Ptr m_pLeft;
};

/**
 * @internal
 */
class ParseTreeEXIST : public ParseTreeBase
{
public:
    explicit ParseTreeEXIST(const char *_id)
    {
        m_id = QString::fromUtf8(_id);
    }

    bool eval(ParseContext *_context) const override;

protected:
    QString m_id;
};

/**
 * @internal
 */
class ParseTreeID : public ParseTreeBase
{
public:
    explicit ParseTreeID(const char *arg)
    {
        m_str = QString::fromUtf8(arg);
    }

    bool eval(ParseContext *_context) const override;

protected:
    QString m_str;
};

/**
 * @internal
 */
class ParseTreeSTRING : public ParseTreeBase
{
public:
    explicit ParseTreeSTRING(const char *arg)
    {
        m_str = QString::fromUtf8(arg);
    }

    bool eval(ParseContext *_context) const override;

protected:
    QString m_str;
};

/**
 * @internal
 */
class ParseTreeNUM : public ParseTreeBase
{
public:
    explicit ParseTreeNUM(int arg)
    {
        m_int = arg;
    }

    bool eval(ParseContext *_context) const override;

protected:
    int m_int;
};

/**
 * @internal
 */
class ParseTreeDOUBLE : public ParseTreeBase
{
public:
    explicit ParseTreeDOUBLE(double arg)
    {
        m_double = arg;
    }

    bool eval(ParseContext *_context) const override;

protected:
    double m_double;
};

/**
 * @internal
 */
class ParseTreeBOOL : public ParseTreeBase
{
public:
    explicit ParseTreeBOOL(bool arg)
    {
        m_bool = arg;
    }

    bool eval(ParseContext *_context) const override;

protected:
    bool m_bool;
};

/**
 * @internal
 */
class ParseTreeMAX2 : public ParseTreeBase
{
public:
    explicit ParseTreeMAX2(const char *_id)
    {
        m_strId = QString::fromUtf8(_id);
    }

    bool eval(ParseContext *_context) const override;

protected:
    QString m_strId;
};

/**
 * @internal
 */
class ParseTreeMIN2 : public ParseTreeBase
{
public:
    explicit ParseTreeMIN2(const char *_id)
    {
        m_strId = QString::fromUtf8(_id);
    }

    bool eval(ParseContext *_context) const override;

protected:
    QString m_strId;
};

}

#endif
