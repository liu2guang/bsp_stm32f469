/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __K_NETEASE_H__ 
#define __K_NETEASE_H__ 

#include "rtthread.h" 

enum knetease_search_type
{
    K_SEARCH_TYPE_MUSIC    = 1,     /* music    */ 
    K_SEARCH_TYPE_ALBUM    = 10,    /* album    */ 
    K_SEARCH_TYPE_SINGER   = 100,   /* singer   */
    K_SEARCH_TYPE_PLAYLIST = 1000,  /* playlist */
    K_SEARCH_TYPE_USER     = 1002,  /* user     */
    K_SEARCH_TYPE_MV       = 1004,  /* mv       */
    K_SEARCH_TYPE_LYRIC    = 1006,  /* lyric    */
    K_SEARCH_TYPE_RADIO    = 1009   /* radio    */
}; 
typedef enum knetease_search_type knetease_search_type_t; 

enum knetease_quality
{
    K_QUALITY_HD = 1, 
    K_QUALITY_MD = 2,
    K_QUALITY_LD = 3
}; 
typedef enum knetease_quality knetease_quality_t; 

struct knetease_music
{
    char *song_name;
    rt_uint32_t song_id; 
    
    char *album_name; 
    rt_uint32_t album_id; 

    char *artist; 
    char *mp3_url; 
    knetease_quality_t quality; 
}; 
typedef struct knetease_music* knetease_music_t; 

/**
 * Search for netease cloud resources
 * 
 * @param keyword: Search keywords.
 * @param index  : Result number, starting from 0.
 * @param type   : Search types. @see "knetease_search_type_t".
 *
 * @return ==RT_NULL: Search failure. 
 *         !=RT_NULL: The handle to the resource found.
 * 
 * @note: 1. The handle needs to be free after use.
 */
void *knetease_search(const char *keyword, rt_uint32_t index, knetease_search_type_t type); 

// GET or POST: http://music.163.com/api/search/get/?s=kda&limit=1&sub=false&type=1  (网易云音乐Android客户端(1.5.2))
// GET or POST: http://music.163.com/api/search/pc?s=%s&offset=%d&limit=1&type=%d    

/**
 * Search for netease cloud music resources
 * 
 * @param keyword: Search music keywords.
 * @param index  : Result number, starting from 0.
 *
 * @return ==RT_NULL: Search music failure. 
 *         !=RT_NULL: The handle to the song found.
 * 
 * @note: 1. The handle needs to be free after use.
 */
knetease_music_t knetease_search_music(const char *keyword, rt_uint32_t index); 

// knetease_music_t knetease_search_album(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_singer(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_playlist(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_user(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_mv(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_lyric(const char *keyword, rt_uint32_t index); 
// knetease_music_t knetease_search_radio(const char *keyword, rt_uint32_t index); 

rt_err_t knetease_play(knetease_music_t music, knetease_quality_t quality); 
// rt_err_t knetease_play_with_id(rt_uint32_t id); 
// rt_err_t knetease_play_with_url(const char *url); 

rt_uint32_t knetease_login(const char *username, const char *password);
rt_err_t knetease_logout(rt_uint32_t uid); 

#endif 
