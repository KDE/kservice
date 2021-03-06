/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// TODO: Torben: On error free memory!   (r932881 might serve as inspiration)

extern "C" {
#include "ktraderparse_p.h"

void KTraderParse_mainParse(const char *_code);
}

#include "ktraderparsetree_p.h"

#include <assert.h>
#include <stdlib.h>

#include "servicesdebug.h"
#include <QThreadStorage>

namespace KTraderParse
{
struct ParsingData {
    ParseTreeBase::Ptr ptr;
    QByteArray buffer;
};

}

using namespace KTraderParse;

Q_GLOBAL_STATIC(QThreadStorage<ParsingData *>, s_parsingData)

ParseTreeBase::Ptr KTraderParse::parseConstraints(const QString &_constr)
{
    ParsingData *data = new ParsingData();
    s_parsingData()->setLocalData(data);
    data->buffer = _constr.toUtf8();
    KTraderParse_mainParse(data->buffer.constData());
    ParseTreeBase::Ptr ret = data->ptr;
    s_parsingData()->setLocalData(nullptr);
    return ret;
}

void KTraderParse_setParseTree(void *_ptr1)
{
    ParsingData *data = s_parsingData()->localData();
    data->ptr = static_cast<ParseTreeBase *>(_ptr1);
}

void KTraderParse_error(const char *err)
{
    qCWarning(SERVICES) << "Parsing" << s_parsingData()->localData()->buffer << "gave:" << err;
}

void *KTraderParse_newOR(void *_ptr1, void *_ptr2)
{
    return new ParseTreeOR(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2));
}

void *KTraderParse_newAND(void *_ptr1, void *_ptr2)
{
    return new ParseTreeAND(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2));
}

void *KTraderParse_newCMP(void *_ptr1, void *_ptr2, int _i)
{
    return new ParseTreeCMP(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2), _i);
}

void *KTraderParse_newIN(void *_ptr1, void *_ptr2, int _cs)
{
    return new ParseTreeIN(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2), _cs == 1 ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void *KTraderParse_newSubstringIN(void *_ptr1, void *_ptr2, int _cs)
{
    return new ParseTreeIN(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2), _cs == 1 ? Qt::CaseSensitive : Qt::CaseInsensitive, true);
}

void *KTraderParse_newMATCH(void *_ptr1, void *_ptr2, int _cs)
{
    return new ParseTreeMATCH(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2), _cs == 1 ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void *KTraderParse_newSubsequenceMATCH(void *_ptr1, void *_ptr2, int _cs)
{
    return new ParseTreeSubsequenceMATCH(static_cast<ParseTreeBase *>(_ptr1),
                                         static_cast<ParseTreeBase *>(_ptr2),
                                         _cs == 1 ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

void *KTraderParse_newCALC(void *_ptr1, void *_ptr2, int _i)
{
    return new ParseTreeCALC(static_cast<ParseTreeBase *>(_ptr1), static_cast<ParseTreeBase *>(_ptr2), _i);
}

void *KTraderParse_newBRACKETS(void *_ptr1)
{
    return new ParseTreeBRACKETS(static_cast<ParseTreeBase *>(_ptr1));
}

void *KTraderParse_newNOT(void *_ptr1)
{
    return new ParseTreeNOT(static_cast<ParseTreeBase *>(_ptr1));
}

void *KTraderParse_newEXIST(char *_ptr1)
{
    ParseTreeEXIST *t = new ParseTreeEXIST(_ptr1);
    free(_ptr1);
    return t;
}

void *KTraderParse_newID(char *_ptr1)
{
    ParseTreeID *t = new ParseTreeID(_ptr1);
    free(_ptr1);
    return t;
}

void *KTraderParse_newSTRING(char *_ptr1)
{
    ParseTreeSTRING *t = new ParseTreeSTRING(_ptr1);
    free(_ptr1);
    return t;
}

void *KTraderParse_newNUM(int _i)
{
    return new ParseTreeNUM(_i);
}

void *KTraderParse_newFLOAT(float _f)
{
    return new ParseTreeDOUBLE(_f);
}

void *KTraderParse_newBOOL(char _b)
{
    return new ParseTreeBOOL(_b);
}

void *KTraderParse_newMAX2(char *_id)
{
    ParseTreeMAX2 *t = new ParseTreeMAX2(_id);
    free(_id);
    return t;
}

void *KTraderParse_newMIN2(char *_id)
{
    ParseTreeMIN2 *t = new ParseTreeMIN2(_id);
    free(_id);
    return t;
}

void KTraderParse_destroy(void *node)
{
    ParsingData *data = s_parsingData()->localData();
    ParseTreeBase *p = reinterpret_cast<ParseTreeBase *>(node);
    if (p != data->ptr.data()) {
        delete p;
    }
}
