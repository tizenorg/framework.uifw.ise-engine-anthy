/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2005 Takuro Ashie
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

#include <string.h>

#include "scim_anthy_factory.h"
#include "scim_anthy_imengine.h"
#include "scim_anthy_conversion.h"
#include "scim_anthy_utils.h"
#include "scim_anthy_prefs.h"

using namespace scim_anthy;

static void rotate_case (String &str);


//
// ConversionSegment Class
//
ConversionSegment::ConversionSegment (WideString str, int cand_id,
                                      unsigned int reading_len)
    : m_string      (str),
      m_cand_id     (cand_id),
      m_reading_len (reading_len)
{
}

ConversionSegment::~ConversionSegment ()
{
}

WideString &
ConversionSegment::get_string (void)
{
    return m_string;
}


int
ConversionSegment::get_candidate_id (void)
{
    return m_cand_id;
}

unsigned int
ConversionSegment::get_reading_length (void)
{
    return m_reading_len;
}

void
ConversionSegment::set (WideString str, int cand_id)
{
    m_string  = str;
    m_cand_id = cand_id;
}

void
ConversionSegment::set_reading_length (unsigned int len)
{
    m_reading_len = len;
}



//
// Conversion Class
//
Conversion::Conversion (AnthyInstance &anthy, Reading &reading)
    : m_anthy              (anthy),
      m_reading            (reading),
      m_anthy_context      (anthy_create_context()),
      m_start_id           (0),
      m_cur_segment        (-1),
      m_predicting         (false)
{
    set_dict_encoding (String (SCIM_ANTHY_CONFIG_DICT_ENCODING_DEFAULT));
}

Conversion::~Conversion ()
{
    anthy_release_context (m_anthy_context);
}



//
// starting and finishing
//
void
Conversion::convert (WideString source, CandidateType ctype,
                     bool single_segment)
{
    if (is_converting ())
        return;

    clear ();

    String dest;

    // convert
    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);
    if (conv_stat.nr_segment <= 0) {
        m_iconv.convert (dest, source);
        anthy_set_string (m_anthy_context, dest.c_str ());
    }

    if (single_segment)
        join_all_segments ();

    // get information about conversion string
    anthy_get_stat (m_anthy_context, &conv_stat);
    if (conv_stat.nr_segment <= 0)
        return;

    // select first segment
    m_cur_segment = 0;

    // create segments
    m_segments.clear();
    for (int i = m_start_id; i < conv_stat.nr_segment; i++) {
        struct anthy_segment_stat seg_stat;
        anthy_get_segment_stat (m_anthy_context, i, &seg_stat);
        m_segments.push_back (
            ConversionSegment (get_segment_string (i, ctype), ctype,
                               seg_stat.seg_len));
    }
}

void
Conversion::convert (CandidateType ctype, bool single_segment)
{
    convert (m_reading.get (), ctype, single_segment);
}

void
Conversion::convert (const WideString &source, bool single_segment)
{
    convert (source, SCIM_ANTHY_CANDIDATE_DEFAULT, single_segment);
}

void
Conversion::predict (void)
{
    clear ();

#ifdef HAS_ANTHY_PREDICTION
    String str;

    m_iconv.convert (str, m_reading.get ());
    anthy_set_prediction_string (m_anthy_context, str.c_str ());

    struct anthy_prediction_stat ps;
    anthy_get_prediction_stat (m_anthy_context, &ps);
    if (ps.nr_prediction > 0)
        m_predicting = true;
    else
        anthy_reset_context (m_anthy_context);
#endif /* HAS_ANTHY_PREDICTION */
}

void
Conversion::clear (int segment_id)
{
    if (segment_id < 0 || m_segments.size () <= 0 ||
        segment_id >= (int) m_segments.size () - 1)
    {
        // complete clear

        anthy_reset_context (m_anthy_context);

        m_segments.clear ();

        m_start_id    = 0;
        m_cur_segment = -1;
        m_predicting  = false;

    } else {
        // partial clear

        // remove stored segments
        ConversionSegments::iterator it = m_segments.begin ();
        m_segments.erase (it, it + segment_id + 1);

        // adjust selected segment
        int new_start_segment_id = m_start_id + segment_id + 1;
        if (m_cur_segment >= 0) {
            m_cur_segment -= new_start_segment_id - m_start_id;
            if (m_cur_segment < 0)
                m_cur_segment = 0;
        }

        // adjust offset
        unsigned int clear_len = 0;
        for (int i = m_start_id; i < new_start_segment_id; i++) {
            struct anthy_segment_stat seg_stat;
            anthy_get_segment_stat (m_anthy_context, i, &seg_stat);
            clear_len += seg_stat.seg_len;
        }
        m_reading.erase (0, clear_len, true);
        m_start_id = new_start_segment_id;
    }
}

void
Conversion::commit (int segment_id, bool learn)
{
    if (!is_converting ()) return;

    // learn
    for (unsigned int i = m_start_id;
         learn &&
             i < m_segments.size () &&
             (segment_id < 0 || (int) i <= segment_id);
         i++)
    {
        if (m_segments[i].get_candidate_id () >= 0)
            anthy_commit_segment (m_anthy_context, i,
                                  m_segments[i].get_candidate_id ());
    }

    clear (segment_id);
}



//
// getting status
//
bool
Conversion::is_converting (void)
{
    if (m_segments.size () > 0)
        return true;
    else
        return false;
}

bool
Conversion::is_predicting (void)
{
    return m_predicting;
}

WideString
Conversion::get (void)
{
    WideString str;
    ConversionSegments::iterator it;
    for (it = m_segments.begin (); it != m_segments.end (); it++)
        str += it->get_string ();
    return str;
}

unsigned int
Conversion::get_length (void)
{
    unsigned int len = 0;
    ConversionSegments::iterator it;
    for (it = m_segments.begin (); it != m_segments.end (); it++)
        len += it->get_string().length ();
    return len;
}

AttributeList
Conversion::get_attribute_list (void)
{
    AttributeList attrs;
    unsigned int pos = 0, seg_id;
    ConversionSegments::iterator it;
    for (it = m_segments.begin (), seg_id = 0;
         it != m_segments.end ();
         it++, seg_id++)
    {
        // create attribute for this segment
        if (it->get_string().length () <= 0) {
            pos += it->get_string().length ();
            continue;
        }

        if ((int) seg_id == m_cur_segment) {
            util_create_attributes (
                attrs, pos, it->get_string().length(),
                m_anthy.get_factory()->m_selected_segment_style,
                m_anthy.get_factory()->m_selected_segment_fg_color,
                m_anthy.get_factory()->m_selected_segment_bg_color);
        } else {
            util_create_attributes (
                attrs, pos, it->get_string().length(),
                m_anthy.get_factory()->m_conversion_style,
                m_anthy.get_factory()->m_conversion_fg_color,
                m_anthy.get_factory()->m_conversion_bg_color);
        }

        pos += it->get_string().length ();
    }

    return attrs;
}



//
// segments of the converted sentence
//
int
Conversion::get_nr_segments (void)
{
    if (!is_converting ()) return 0;

    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);

    return conv_stat.nr_segment - m_start_id;
}

WideString
Conversion::get_segment_string (int segment_id, int candidate_id)
{
    if (segment_id < 0) {
        if (m_cur_segment < 0)
            return WideString ();
        else
            segment_id = m_cur_segment;
    }

    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);

    if (conv_stat.nr_segment <= 0)
        return WideString ();

    if (m_start_id < 0 ||
        m_start_id >= conv_stat.nr_segment)
    {
        return WideString (); // error
    }

    if (segment_id < 0 ||
        segment_id + m_start_id >= conv_stat.nr_segment)
    {
        return WideString (); //error
    }

    // character position of the head of segment.
    unsigned int real_seg_start = 0;
    for (int i = m_start_id; i < m_start_id + segment_id; i++) {
        struct anthy_segment_stat seg_stat;
        anthy_get_segment_stat (m_anthy_context, i, &seg_stat);
        real_seg_start += seg_stat.seg_len;
    }

    int real_seg = segment_id + m_start_id;
    int cand;
    if (candidate_id <= SCIM_ANTHY_LAST_SPECIAL_CANDIDATE)
        cand = m_segments[segment_id].get_candidate_id ();
    else
        cand = candidate_id;

    // get information of this segment
    struct anthy_segment_stat seg_stat;
    anthy_get_segment_stat (m_anthy_context, real_seg, &seg_stat);

    // get string of this segment
    WideString segment_str;
    if (cand < 0) {
        get_reading_substr (segment_str, segment_id, cand,
                            real_seg_start, seg_stat.seg_len);
    } else {
        int len = anthy_get_segment (m_anthy_context, real_seg, cand, NULL, 0);
        if (len > 0) {
            char buf[len + 1];
            anthy_get_segment (m_anthy_context, real_seg, cand, buf, len + 1);
            buf[len] = '\0';
            m_iconv.convert (segment_str, buf, len);
        }
    }

    return segment_str;
}

int
Conversion::get_selected_segment (void)
{
    return m_cur_segment;
}

void
Conversion::select_segment (int segment_id)
{
    if (!is_converting ()) return;

    if (segment_id < 0) {
        m_cur_segment = -1;
        return;
    }

    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);

    int real_segment_id = segment_id + m_start_id;

    if (segment_id >= 0 && real_segment_id < conv_stat.nr_segment)
        m_cur_segment = segment_id;
}

int
Conversion::get_segment_size (int segment_id)
{
    if (!is_converting ()) return -1;

    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);

    if (segment_id < 0) {
        if (m_cur_segment < 0)
            return -1;
        else
            segment_id = m_cur_segment;
    }
    int real_segment_id = segment_id + m_start_id;

    if (real_segment_id >= conv_stat.nr_segment)
        return -1;

    struct anthy_segment_stat seg_stat;
    anthy_get_segment_stat (m_anthy_context, real_segment_id, &seg_stat);

    return seg_stat.seg_len;
}

void
Conversion::resize_segment (int relative_size, int segment_id)
{
    if (is_predicting ()) return;
    if (!is_converting ()) return;

    struct anthy_conv_stat conv_stat;
    anthy_get_stat (m_anthy_context, &conv_stat);

    int real_segment_id;

    if (segment_id < 0) {
        if (m_cur_segment < 0)
            return;
        else
            segment_id = m_cur_segment;
        real_segment_id = segment_id + m_start_id;
    } else {
        real_segment_id = segment_id + m_start_id;
        if (m_cur_segment > segment_id)
            m_cur_segment = segment_id;
    }

    if (real_segment_id >= conv_stat.nr_segment)
        return;

    // do resize
    anthy_resize_segment (m_anthy_context, real_segment_id, relative_size);

    // reset candidates of trailing segments
    anthy_get_stat (m_anthy_context, &conv_stat);
    ConversionSegments::iterator start_iter = m_segments.begin();
    ConversionSegments::iterator end_iter   = m_segments.end();
    m_segments.erase (start_iter + segment_id, end_iter);
    for (int i = real_segment_id; i < conv_stat.nr_segment; i++) {
        struct anthy_segment_stat seg_stat;
        anthy_get_segment_stat (m_anthy_context, i, &seg_stat);
        m_segments.push_back (
            ConversionSegment (get_segment_string (i - m_start_id, 0), 0,
                               seg_stat.seg_len));
    }
}

unsigned int
Conversion::get_segment_position (int segment_id)
{
    if (segment_id < 0) {
        if (m_cur_segment < 0)
            return get_length ();
        else
            segment_id = m_cur_segment;
    }

    unsigned int pos = 0;

    for (unsigned int i = 0;
         (int) i < m_cur_segment && i < m_segments.size ();
         i++)
    {
        pos += m_segments[i].get_string().length ();
    }

    return pos;
}



//
// candidates for a segment or prediction
//
void
Conversion::get_candidates (CommonLookupTable &table, int segment_id)
{
    table.clear ();

    if (is_predicting ()) {
#ifdef HAS_ANTHY_PREDICTION
        String str;
        struct anthy_prediction_stat ps;

        anthy_get_prediction_stat (m_anthy_context, &ps);

        for (int i = 0; i < ps.nr_prediction; i++) {
            int len = anthy_get_prediction (m_anthy_context, i, NULL, 0);
            if (len <= 0)
                continue;

            char buf[len + 1];
            anthy_get_prediction (m_anthy_context, i, buf, len + 1);
            buf[len] = '\0';

            WideString cand;
            m_iconv.convert (cand, buf);

            table.append_candidate (cand);
        }
#endif /* HAS_ANTHY_PREDICTION */
    } else if (is_converting ()) {
        struct anthy_conv_stat conv_stat;
        anthy_get_stat (m_anthy_context, &conv_stat);

        if (conv_stat.nr_segment <= 0)
            return;

        if (segment_id < 0) {
            if (m_cur_segment < 0)
                return;
            else
                segment_id = m_cur_segment;
        }
        int real_segment_id = segment_id + m_start_id;

        if (real_segment_id >= conv_stat.nr_segment)
            return;

        struct anthy_segment_stat seg_stat;
        anthy_get_segment_stat (m_anthy_context, real_segment_id, &seg_stat);

        for (int i = 0; i < seg_stat.nr_candidate; i++) {
            int len = anthy_get_segment (m_anthy_context, real_segment_id, i,
                                         NULL, 0);
            if (len <= 0)
                continue;

            char buf[len + 1];
            anthy_get_segment (m_anthy_context, real_segment_id,
                               i, buf, len + 1);

            WideString cand_wide;
            m_iconv.convert (cand_wide, buf, len);

            table.append_candidate (cand_wide);
        }

        table.set_cursor_pos (get_selected_candidate ());
    }
}

int
Conversion::get_selected_candidate (int segment_id)
{
    if (is_predicting ()) {
#ifdef HAS_ANTHY_PREDICTION
        struct anthy_prediction_stat ps;

        anthy_get_prediction_stat (m_anthy_context, &ps);

        if (ps.nr_prediction <= 0)
            return -1;

        if (segment_id < 0) {
            if (m_cur_segment < 0)
                return -1;
            else
                segment_id = m_cur_segment;
        } else if (segment_id >= ps.nr_prediction) {
            return -1;
        }

        return m_segments[segment_id].get_candidate_id ();
#endif /* HAS_ANTHY_PREDICTION */

    } else if (is_converting ()) {
        struct anthy_conv_stat cs;

        anthy_get_stat (m_anthy_context, &cs);

        if (cs.nr_segment <= 0)
            return -1;

        if (segment_id < 0) {
            if (m_cur_segment < 0)
                return -1;
            else
                segment_id = m_cur_segment;
        } else if (segment_id >= cs.nr_segment) {
            return -1;
        }

        return m_segments[segment_id].get_candidate_id ();
    }

    return -1;
}

void
Conversion::select_candidate (int candidate_id, int segment_id)
{
    if (is_predicting ()) {
#ifdef HAS_ANTHY_PREDICTION
        if (candidate_id < SCIM_ANTHY_CANDIDATE_DEFAULT)
            return;

        struct anthy_prediction_stat ps;
        anthy_get_prediction_stat (m_anthy_context, &ps);

        if (ps.nr_prediction <= 0)
            return;

        if (!is_converting ()) {
            m_cur_segment = 0;
            m_segments.push_back (ConversionSegment (
                                      get_prediction_string (0), 0,
                                      m_reading.get_length ()));
        }

        if (candidate_id < ps.nr_prediction) {
            m_segments[0].set (get_prediction_string (candidate_id),
                               candidate_id);
        }
#endif /* HAS_ANTHY_PREDICTION */

    } else if (is_converting ()) {
        if (candidate_id <= SCIM_ANTHY_LAST_SPECIAL_CANDIDATE)
            return;

        struct anthy_conv_stat conv_stat;
        anthy_get_stat (m_anthy_context, &conv_stat);

        if (conv_stat.nr_segment <= 0)
            return;

        if (segment_id < 0) {
            if (m_cur_segment < 0)
                return;
            else
                segment_id = m_cur_segment;
        }
        int real_segment_id = segment_id + m_start_id;

        if (segment_id >= conv_stat.nr_segment)
            return;

        struct anthy_segment_stat seg_stat;
        anthy_get_segment_stat (m_anthy_context, real_segment_id, &seg_stat);

        if (candidate_id == SCIM_ANTHY_CANDIDATE_HALF) {
            switch (m_segments[segment_id].get_candidate_id ()) {
            case SCIM_ANTHY_CANDIDATE_LATIN:
            case SCIM_ANTHY_CANDIDATE_WIDE_LATIN:
                candidate_id = SCIM_ANTHY_CANDIDATE_LATIN;
                break;
            default:
                candidate_id = SCIM_ANTHY_CANDIDATE_HALF_KATAKANA;
                break;
            }
        }

        if (candidate_id < seg_stat.nr_candidate) {
            m_segments[segment_id].set (get_segment_string (segment_id,
                                                            candidate_id),
                                        candidate_id);
        }
    }
}



//
// preferences
//
bool
Conversion::set_dict_encoding (String type)
{
#ifdef HAS_ANTHY_CONTEXT_SET_ENCODING
    if (!strcasecmp (type.c_str (), "UTF-8") ||
        !strcasecmp (type.c_str (), "UTF8"))
    {
        anthy_context_set_encoding (m_anthy_context, ANTHY_UTF8_ENCODING);
    } else {
        anthy_context_set_encoding (m_anthy_context, ANTHY_EUC_JP_ENCODING);
    }
#endif /* HAS_ANTHY_CONTEXT_SET_ENCODING */

    if (m_iconv.set_encoding (type.c_str ())) {
        return true;
    } else {
        return m_iconv.set_encoding ("EUC-JP");
    }
}



//
// Utilities
//
void
Conversion::get_reading_substr (WideString &string,
                                int segment_id,
                                int candidate_id,
                                int seg_start,
                                int seg_len)
{
    int prev_cand = 0;

    if (segment_id < (int) m_segments.size ())
        prev_cand = m_segments[segment_id].get_candidate_id ();

    switch ((CandidateType) candidate_id) {
    case SCIM_ANTHY_CANDIDATE_LATIN:
        if (prev_cand == SCIM_ANTHY_CANDIDATE_LATIN) {
            String str = utf8_wcstombs (m_segments[segment_id].get_string ());
            rotate_case (str);
            string = utf8_mbstowcs (str);
        } else {
            string = m_reading.get (seg_start, seg_len,
                                    SCIM_ANTHY_STRING_LATIN);
        }
        break;

    case SCIM_ANTHY_CANDIDATE_WIDE_LATIN:
        if (prev_cand == SCIM_ANTHY_CANDIDATE_WIDE_LATIN) {
            String str;
            util_convert_to_half (str, m_segments[segment_id].get_string ());
            rotate_case (str);
            util_convert_to_wide (string, str);
        } else {
            string = m_reading.get (seg_start, seg_len,
                                    SCIM_ANTHY_STRING_WIDE_LATIN);
        }
        break;

    case SCIM_ANTHY_CANDIDATE_KATAKANA:
        string = m_reading.get (seg_start, seg_len,
                                SCIM_ANTHY_STRING_KATAKANA);
        break;

    case SCIM_ANTHY_CANDIDATE_HALF_KATAKANA:
        string = m_reading.get (seg_start, seg_len,
                                SCIM_ANTHY_STRING_HALF_KATAKANA);
        break;

    case SCIM_ANTHY_CANDIDATE_HALF:
        // shouldn't reach to this entry
        string = m_reading.get (seg_start, seg_len,
                                SCIM_ANTHY_STRING_HALF_KATAKANA);
        break;

    case SCIM_ANTHY_CANDIDATE_HIRAGANA:
    default:
        string = m_reading.get (seg_start, seg_len,
                                SCIM_ANTHY_STRING_HIRAGANA);
        break;
    }
}

WideString
Conversion::get_prediction_string (int candidate_id)
{
#ifdef HAS_ANTHY_PREDICTION
    if (!is_predicting ())
        return WideString ();

    struct anthy_prediction_stat ps;
    anthy_get_prediction_stat (m_anthy_context, &ps);

    if (ps.nr_prediction <= 0)
        return WideString ();

    int len = anthy_get_prediction (m_anthy_context, candidate_id, NULL, 0);
    if (len <= 0)
        return WideString ();

    char buf[len + 1];
    anthy_get_prediction (m_anthy_context, candidate_id, buf, len + 1);
    buf[len] = '\0';

    WideString cand;
    m_iconv.convert (cand, buf);

    return cand;
#else /* HAS_ANTHY_PREDICTION */
    return WideString ();
#endif /* HAS_ANTHY_PREDICTION */
}

void
Conversion::join_all_segments (void)
{
    do {
        struct anthy_conv_stat conv_stat;
        anthy_get_stat (m_anthy_context, &conv_stat);
        int nr_seg = conv_stat.nr_segment - m_start_id;

        if (nr_seg > 1)
            anthy_resize_segment (m_anthy_context, m_start_id, 1);
        else
            break;
    } while (true);
}

static void
rotate_case (String &str)
{
    bool is_mixed = false;
    for (unsigned int i = 1; i < str.length (); i++) {
        if ((isupper (str[0]) && islower (str[i])) ||
            (islower (str[0]) && isupper (str[i])))
        {
            is_mixed = true;
            break;
        }
    }

    if (is_mixed) {
        // Anthy -> anthy, anThy -> anthy
        for (unsigned int i = 0; i < str.length (); i++)
            str[i] = tolower (str[i]);
    } else if (isupper (str[0])) {
        // ANTHY -> Anthy
        for (unsigned int i = 1; i < str.length (); i++)
            str[i] = tolower (str[i]);
    } else {
        // anthy -> ANTHY
        for (unsigned int i = 0; i < str.length (); i++)
            str[i] = toupper (str[i]);
    }
}
