/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __parse_h__
#define __parse_h__

/*
 * Functions definition for yacc
 */
void KTraderParse_mainParse(const char *_code);
void KTraderParse_setParseTree(void *_ptr1);
void KTraderParse_error(const char *err);
void *KTraderParse_newOR(void *_ptr1, void *_ptr2);
void *KTraderParse_newAND(void *_ptr1, void *_ptr2);
void *KTraderParse_newCMP(void *_ptr1, void *_ptr2, int _i);
void *KTraderParse_newIN(void *_ptr1, void *_ptr2, int _cs);
void *KTraderParse_newSubstringIN(void *_ptr1, void *_ptr2, int _cs);
void *KTraderParse_newMATCH(void *_ptr1, void *_ptr2, int _cs);
void *KTraderParse_newSubsequenceMATCH(void *_ptr1, void *_ptr2, int _cs);
void *KTraderParse_newCALC(void *_ptr1, void *_ptr2, int _i);
void *KTraderParse_newBRACKETS(void *_ptr1);
void *KTraderParse_newNOT(void *_ptr1);
void *KTraderParse_newEXIST(char *_ptr1);
void *KTraderParse_newID(char *_ptr1);
void *KTraderParse_newSTRING(char *_ptr1);
void *KTraderParse_newNUM(int _i);
void *KTraderParse_newFLOAT(float _f);
void *KTraderParse_newBOOL(char _b);

void *KTraderParse_newWITH(void *_ptr1);
void *KTraderParse_newMAX(void *_ptr1);
void *KTraderParse_newMIN(void *_ptr1);
void *KTraderParse_newMAX2(char *_id);
void *KTraderParse_newMIN2(char *_id);
void *KTraderParse_newCI(void *_ptr1);
void *KTraderParse_newFIRST();
void *KTraderParse_newRANDOM();

void KTraderParse_destroy(void *);

#endif
