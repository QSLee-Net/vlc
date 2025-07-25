/*****************************************************************************
 * es_out.h: Input es_out functions
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * Copyright (C) 2008 Laurent Aimar
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef LIBVLC_INPUT_ES_OUT_H
#define LIBVLC_INPUT_ES_OUT_H 1

#include <vlc_common.h>
#include <vlc_es_out.h>

#include "input_internal.h"

typedef struct input_thread_t input_thread_t;

enum es_out_mode_e
{
    ES_OUT_MODE_NONE,   /* don't select anything */
    ES_OUT_MODE_ALL,    /* eg for stream output */
    ES_OUT_MODE_AUTO,   /* best audio/video or for input follow audio-track, sub-track */
    ES_OUT_MODE_PARTIAL,/* select programs given after --programs */
    ES_OUT_MODE_END     /* mark the es_out as dead */
};

enum es_out_query_private_e
{
    /* set/get mode */
    ES_OUT_PRIV_SET_MODE = ES_OUT_PRIVATE_START, /* arg1= int */

    /* Same than ES_OUT_SET_ES/ES_OUT_UNSET_ES/ES_OUT_RESTART_ES, but with vlc_es_id_t * */
    ES_OUT_PRIV_SET_ES,      /* arg1= vlc_es_id_t*                   */
    ES_OUT_PRIV_UNSET_ES,    /* arg1= vlc_es_id_t* res=can fail      */
    ES_OUT_PRIV_RESTART_ES,  /* arg1= vlc_es_id_t*                   */

    /* Get date to wait before demuxing more data */
    ES_OUT_PRIV_GET_WAKE_UP,                        /* arg1=vlc_tick_t*            res=cannot fail */

    /* Select a list of ES */
    ES_OUT_PRIV_SET_ES_LIST, /* arg1= vlc_es_id_t *const* (null terminated array) */

    ES_OUT_PRIV_SET_ES_CAT_IDS, /* arg1=es_format_category_e arg2=const char *, res=cannot fail */

    /* Stop all selected ES and save the stopped state in a context.
     * Call ES_OUT_PRIV_START_ALL_ES to release the context. */
    ES_OUT_PRIV_STOP_ALL_ES,                        /* arg1=vlc_es_id_t *** */
    /* Start all ES from the context returned by ES_OUT_PRIV_STOP_ALL_ES */
    ES_OUT_PRIV_START_ALL_ES,                       /* arg1=vlc_es_id_t ** */

    /* Get buffering state */
    ES_OUT_PRIV_GET_BUFFERING,                      /* arg1=bool*               res=cannot fail */

    /* Set delay for an ES identifier */
    ES_OUT_PRIV_SET_ES_DELAY,                       /* arg1=vlc_es_id_t *, res=cannot fail */

    /* Set delay for a ES category */
    ES_OUT_PRIV_SET_DELAY,                          /* arg1=es_category_e,      res=cannot fail */

    /* Set record state */
    ES_OUT_PRIV_SET_RECORD_STATE,                        /* arg1=bool                res=can fail */

    /* Set pause state */
    ES_OUT_PRIV_SET_PAUSE_STATE,                    /* arg1=bool b_source_paused, bool b_paused arg2=vlc_tick_t res=can fail */

    /* Set rate */
    ES_OUT_PRIV_SET_RATE,                           /* arg1=double source_rate arg2=double rate res=can fail */

    /* Set next frame */
    ES_OUT_PRIV_SET_FRAME_NEXT,                     /*                          res=can fail */

    /* Set position/time/length */
    ES_OUT_PRIV_SET_TIMES,                          /* arg1=double f_position arg2=vlc_tick_t i_time arg3=vlc_tick_t i_normal_time arg4=vlc_tick_t i_length arg5 int b_live res=cannot fail */

    /* Set jitter */
    ES_OUT_PRIV_SET_JITTER,                         /* arg1=vlc_tick_t i_pts_delay arg2= vlc_tick_t i_pts_jitter, arg2=int i_cr_average res=cannot fail */

    /* Get forced group */
    ES_OUT_PRIV_GET_GROUP_FORCED,                   /* arg1=int * res=cannot fail */

    /* Set End Of Stream */
    ES_OUT_PRIV_SET_EOS,                            /* res=cannot fail */

    /* Set a VBI/Teletext page */
    ES_OUT_PRIV_SET_VBI_PAGE,                       /* arg1=unsigned res=can fail */

    /* Set VBI/Teletext menu transparent */
    ES_OUT_PRIV_SET_VBI_TRANSPARENCY                /* arg1=bool res=can fail */
};

struct vlc_input_es_out;
struct vlc_input_es_out_ops {
    int (*priv_control)(
        struct vlc_input_es_out *out,
        input_source_t *source,
        int query,
        va_list args);
};

struct vlc_input_es_out {
    es_out_t out;

    const struct vlc_input_es_out_ops *ops;
};

static inline int
es_out_vaPrivControl(struct vlc_input_es_out *out, int query, va_list args )
{
    return out->ops->priv_control(out, NULL, query, args);
}

static inline int
es_out_PrivControl(struct vlc_input_es_out *out, int query, ...)
{
    va_list args;
    va_start( args, query );
    int result = es_out_vaPrivControl( out, query, args );
    va_end( args );
    return result;
}

static inline void
es_out_SetMode(struct vlc_input_es_out *out, int i_mode)
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_MODE, i_mode);
    assert( !i_ret );
}

static inline int
es_out_SetEs(struct vlc_input_es_out *out, vlc_es_id_t *id)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_ES, id);
}

static inline int
es_out_UnsetEs(struct vlc_input_es_out *out, vlc_es_id_t *id)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_UNSET_ES, id);
}

static inline int
es_out_RestartEs(struct vlc_input_es_out *out, vlc_es_id_t *id)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_RESTART_ES, id);
}

static inline vlc_tick_t
es_out_GetWakeup(struct vlc_input_es_out *out)
{
    vlc_tick_t i_wu;
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_GET_WAKE_UP, &i_wu);

    assert( !i_ret );
    return i_wu;
}

static inline int
es_out_SetEsList(struct vlc_input_es_out *out,
                 enum es_format_category_e cat,
                 vlc_es_id_t **ids)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_ES_LIST, cat, ids);
}

static inline void
es_out_SetEsCatIds(struct vlc_input_es_out *out,
                   enum es_format_category_e cat,
                   const char *str_ids)
{
    int ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_ES_CAT_IDS,
                                 cat, str_ids);
    assert( ret == VLC_SUCCESS );
}

static inline int
es_out_StopAllEs(struct vlc_input_es_out *out,
                 vlc_es_id_t ***context)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_STOP_ALL_ES, context);
}

static inline int
es_out_StartAllEs(struct vlc_input_es_out *out, vlc_es_id_t **context )
{
    return es_out_PrivControl(out, ES_OUT_PRIV_START_ALL_ES, context);
}

static inline bool
es_out_GetBuffering(struct vlc_input_es_out *out)
{
    bool b;
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_GET_BUFFERING, &b);

    assert( !i_ret );
    return b;
}

static inline bool
es_out_IsEmpty(struct vlc_input_es_out *out)
{
    bool b;
    int i_ret = es_out_Control(&out->out, ES_OUT_IS_EMPTY, &b);

    assert( !i_ret );
    return b;
}

static inline void
es_out_SetEsDelay(struct vlc_input_es_out *out,
                  vlc_es_id_t *es,
                  vlc_tick_t i_delay)
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_ES_DELAY, es, i_delay);
    assert( !i_ret );
}

static inline void
es_out_SetDelay(struct vlc_input_es_out *out,
                int i_cat,
                vlc_tick_t i_delay)
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_DELAY, i_cat, i_delay);
    assert( !i_ret );
}

static inline int
es_out_SetRecordState(struct vlc_input_es_out *out,
                      bool b_record,
                      const char *dir_path)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_RECORD_STATE, b_record, dir_path);
}

static inline int
es_out_SetPauseState(struct vlc_input_es_out *out,
                     bool b_source_paused,
                     bool b_paused,
                     vlc_tick_t i_date)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_PAUSE_STATE, b_source_paused, b_paused, i_date);
}

static inline int
es_out_SetRate(struct vlc_input_es_out *out, float source_rate, float rate)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_RATE, source_rate, rate);
}

static inline int
es_out_SetFrameNext(struct vlc_input_es_out *out )
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_FRAME_NEXT);
}

static inline void
es_out_SetTimes(struct vlc_input_es_out *out, double f_position,
                vlc_tick_t i_time, vlc_tick_t i_normal_time,
                vlc_tick_t i_length, bool b_live  )
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_TIMES, f_position, i_time,
                                   i_normal_time, i_length, b_live);
    assert( !i_ret );
}

static inline void
es_out_SetJitter(struct vlc_input_es_out *out,
                 vlc_tick_t i_pts_delay, vlc_tick_t i_pts_jitter,
                 int i_cr_average)
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_JITTER,
                                   i_pts_delay, i_pts_jitter, i_cr_average);
    assert( !i_ret );
}

static inline int
es_out_GetGroupForced(struct vlc_input_es_out *out)
{
    int i_group;
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_GET_GROUP_FORCED, &i_group);
    assert( !i_ret );
    return i_group;
}

static inline void
es_out_Eos(struct vlc_input_es_out *out)
{
    int i_ret = es_out_PrivControl(out, ES_OUT_PRIV_SET_EOS);
    assert( !i_ret );
}

static inline int
es_out_SetVbiPage(struct vlc_input_es_out *out, vlc_es_id_t *id,
                  unsigned page)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_VBI_PAGE, id, page);
}

static inline int
es_out_SetVbiTransparency(struct vlc_input_es_out *out, vlc_es_id_t *id,
                          bool enabled)
{
    return es_out_PrivControl(out, ES_OUT_PRIV_SET_VBI_TRANSPARENCY, id,
                              enabled);
}

struct vlc_input_es_out *
input_EsOutNew(input_thread_t *, input_source_t *main_source, float rate,
               enum input_type input_type);

struct vlc_input_es_out *
input_EsOutTimeshiftNew(input_thread_t *, struct vlc_input_es_out *, float i_rate);

struct vlc_input_es_out *
input_EsOutSourceNew(struct vlc_input_es_out *master_out, input_source_t *in);

es_out_id_t *vlc_es_id_get_out(vlc_es_id_t *id);
const input_source_t *vlc_es_id_GetSource(vlc_es_id_t *id);

static inline void
vlc_input_es_out_Delete(struct vlc_input_es_out *out)
{
    es_out_Delete(&out->out);
}

#endif
