/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2004-2005 Takuro Ashie
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __SCIM_ANTHY_DEFAULT_TABLES_H__
#define __SCIM_ANTHY_DEFAULT_TABLES_H__

typedef struct _ConvRule
{
    const char *string;
    const char *result;
    const char *cont;
} ConvRule;

typedef struct _HiraganaKatakanaRule
{
    const char *hiragana;
    const char *katakana;
    const char *half_katakana;
} HiraganaKatakanaRule;

typedef struct _WideRule
{
    const char *code;
    const char *wide;
} WideRule;

typedef struct _KeyCodeToCharRule
{
    unsigned int  code;
    const char   *kana;
} KeyCodeToCharRule;

typedef struct _VoicedConsonantRule
{
    const char *string;
    const char *voiced;
    const char *half_voiced;
} VoicedConsonantRule;

typedef struct _NicolaRule
{
    const char *key;
    const char *single;
    const char *left_shift;
    const char *right_shift;
} NicolaRule;

// fundamental table
extern ConvRule scim_anthy_romaji_typing_rule[];
extern ConvRule scim_anthy_romaji_double_consonant_rule[];
extern ConvRule scim_anthy_kana_typing_rule[];
extern ConvRule scim_anthy_kana_voiced_consonant_rule[];

// symbol & number
extern ConvRule scim_anthy_half_symbol_rule[];
extern ConvRule scim_anthy_wide_symbol_rule[];
extern ConvRule scim_anthy_half_number_rule[];
extern ConvRule scim_anthy_wide_number_rule[];

// period table
extern ConvRule scim_anthy_romaji_ja_period_rule[];
extern ConvRule scim_anthy_romaji_wide_period_rule[];
extern ConvRule scim_anthy_romaji_half_period_rule[];

extern ConvRule scim_anthy_kana_ja_period_rule[];
extern ConvRule scim_anthy_kana_wide_period_rule[];
extern ConvRule scim_anthy_kana_half_period_rule[];

// comma table
extern ConvRule scim_anthy_romaji_ja_comma_rule[];
extern ConvRule scim_anthy_romaji_wide_comma_rule[];
extern ConvRule scim_anthy_romaji_half_comma_rule[];

extern ConvRule scim_anthy_kana_ja_comma_rule[];
extern ConvRule scim_anthy_kana_wide_comma_rule[];
extern ConvRule scim_anthy_kana_half_comma_rule[];

// bracket table
extern ConvRule scim_anthy_romaji_ja_bracket_rule[];
extern ConvRule scim_anthy_romaji_wide_bracket_rule[];

extern ConvRule scim_anthy_kana_ja_bracket_rule[];
extern ConvRule scim_anthy_kana_wide_bracket_rule[];

// slash table
extern ConvRule scim_anthy_romaji_ja_slash_rule[];
extern ConvRule scim_anthy_romaji_wide_slash_rule[];

extern ConvRule scim_anthy_kana_ja_slash_rule[];
extern ConvRule scim_anthy_kana_wide_slash_rule[];

// misc
extern HiraganaKatakanaRule scim_anthy_hiragana_katakana_table[];
extern WideRule             scim_anthy_wide_table[];
extern VoicedConsonantRule  scim_anthy_voiced_consonant_table[];

// key code
extern KeyCodeToCharRule    scim_anthy_keypad_table[];
extern KeyCodeToCharRule    scim_anthy_kana_table[];

// nicola
extern NicolaRule           scim_anthy_nicola_table[];

#endif /* __SCIM_ANTHY_DEFAULT_TABLES_H__ */
