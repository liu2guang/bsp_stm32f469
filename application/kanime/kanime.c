/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtdevice.h"
#include "rtthread.h"
#include "webclient.h"
#include "dfs_posix.h"
#include "cJson.h" 
#include "player.h" 

#define DBG_SECTION_NAME "Kanime"
#define DBG_ENABLE
#define DBG_LEVEL DBG_INFO
#define DBG_COLOR
#include <rtdbg.h>

#define Kanime_PlayMusic    "http://anime-music.jijidown.com/api/v2/music" 
#define Kanime_SearchMusic  "http://anime-music.jijidown.com/api/v2/music/search"  
#define Kanime_RespBuffSize (8*1024)
#define Kanime_ThreadStack  (2*1024)

struct kanime_music
{
    char *name;
    char *url;
    char *author;
    char *desc;
}; 
typedef struct kanime_music* kanime_music_t; 

static kanime_music_t kanime_json_parse(const char *json)
{
    kanime_music_t music = RT_NULL; 

    cJSON *root   = RT_NULL;
    cJSON *res    = RT_NULL;
    cJSON *name   = RT_NULL;
    cJSON *url    = RT_NULL;
    cJSON *author = RT_NULL;
    cJSON *anime  = RT_NULL; 
    cJSON *desc   = RT_NULL; 
    char  *string = RT_NULL; 
    
    /* 解析Json字符串 */ 
    root = cJSON_Parse((const char *)json); 
    if(root == RT_NULL)
    {
        LOG_E("parse json string failed: %s.", cJSON_GetErrorPtr());
        goto _failed; 
    }
    
    string = cJSON_Print(root); 
    LOG_D("Json: %s", string); 
    rt_free(string); 
    
    res = cJSON_GetObjectItem(root, "res");
    if(res == RT_NULL)
    {
        goto _failed; 
    }
    else
    {
        name   = cJSON_GetObjectItem(res, "title");
        url    = cJSON_GetObjectItem(res, "play_url");
        author = cJSON_GetObjectItem(res, "author");
        anime  = cJSON_GetObjectItem(res, "anime_info");

        if((anime == RT_NULL) || (url == RT_NULL) || (author == RT_NULL) || (anime == RT_NULL))
        {
            goto _failed; 
        } 
        else
        {
            desc = cJSON_GetObjectItem(anime, "desc");
            if(desc == RT_NULL)
            {
                goto _failed; 
            }
        }  
    }
    
    music = (kanime_music_t)rt_malloc(sizeof(struct kanime_music)); 
    if(music == RT_NULL)
    {
        LOG_E("kanime_music struct malloc failed, out of memory.");
        goto _failed; 
    }
    
    /* 合成结构体 */ 
    music->name   = rt_strdup(name->valuestring);
    music->url    = rt_strdup(url->valuestring);
    music->author = rt_strdup(author->valuestring);
    music->desc   = rt_strdup(desc->valuestring);
    
    if(music->url == RT_NULL)
    {
        LOG_E("url is null, parse is failed.");
        goto _failed; 
    }

    /* 释放中间使用变量 */
    if(root != RT_NULL)
    {
        cJSON_Delete(root);
        root   = RT_NULL;
        res    = RT_NULL;
        name   = RT_NULL;
        url    = RT_NULL;
        author = RT_NULL;
        anime  = RT_NULL; 
        desc   = RT_NULL; 
    }

    return music;    

_failed:
    /* 成功解析Json字符串, 但是字符串中没有URL, 也认为是失败的 */ 
    if((music != RT_NULL) && (music->url == RT_NULL))
    {
        if(music->name != RT_NULL)
        {
            rt_free(music->name); 
            music->name = RT_NULL; 
        }
        
        if(music->author != RT_NULL)
        {
            rt_free(music->author); 
            music->author = RT_NULL; 
        }
        
        if(music->desc != RT_NULL)
        {
            rt_free(music->desc); 
            music->desc = RT_NULL; 
        }

        rt_free(music); 
        music = RT_NULL; 
    }

    if(root != RT_NULL)
    {
        cJSON_Delete(root);
        root   = RT_NULL;
        res    = RT_NULL;
        name   = RT_NULL;
        url    = RT_NULL;
        author = RT_NULL;
        anime  = RT_NULL; 
        desc   = RT_NULL; 
    }

    return RT_NULL;
}

static void kanime_free(kanime_music_t music)
{
    if(music->url != RT_NULL)
    {
        rt_free(music->url); 
        music->url = RT_NULL; 
    }
    
    if(music->name != RT_NULL)
    {
        rt_free(music->name); 
        music->name = RT_NULL; 
    }
    
    if(music->author != RT_NULL)
    {
        rt_free(music->author); 
        music->author = RT_NULL; 
    }
    
    if(music->desc != RT_NULL)
    {
        rt_free(music->desc); 
        music->desc = RT_NULL; 
    } 
    
    if(music != RT_NULL)
    {
        rt_free(music); 
        music = RT_NULL; 
    }    
}

static rt_err_t kanime_randomplay_music(void)
{
    rt_err_t ret = RT_EOK;
    
    struct webclient_session *session = RT_NULL;
    int length  = (-1); 
    int offset  = (-1);
    int residue = (-1);
    char *resp_buff = RT_NULL;
    kanime_music_t music = RT_NULL;

    session = webclient_open(Kanime_PlayMusic);
    if (session == RT_NULL)
    {
        LOG_E("open website:%s failed.", Kanime_PlayMusic);
        ret = (-RT_ERROR);
        
        goto _ret;
    }

    if (session->response != 200)
    {
        LOG_E("wrong response: %d.", session->response);
        ret = (-RT_ERROR);
        
        goto _ret;
    }

    /* 分配响应的Json字符串缓存buffer */ 
    resp_buff = (char *)rt_malloc(Kanime_RespBuffSize);
    if(resp_buff == RT_NULL)
    {
        LOG_E("resp_buff malloc failed, out of memory.");
        ret = (-RT_ENOMEM);
        
        goto _ret;
    }

    /* 读取Http响应的Json字符串 */ 
    for (offset = 0; (length > 0) || (length == (-1)); offset += length)
    {
        residue = session->content_length - offset; 
        length = webclient_read(session, (unsigned char *)resp_buff, (residue > Kanime_RespBuffSize) ? Kanime_RespBuffSize : residue);
    }
    
    music = kanime_json_parse(resp_buff); 
    if(music == RT_NULL)
    {
        LOG_E("music info parse failed.");
        ret = (-RT_ERROR);
        
        goto _ret;
    }
    
    LOG_I("name  : %s", music->name);
    LOG_I("url   : %s", music->url);
    LOG_I("author: %s", music->author);
    LOG_I("desc  : %s", music->desc);
    
    player_stop(); 
    player_set_uri(music->url); 
    player_play(); 

_ret:
    if(music != RT_NULL)
    {
        kanime_free(music); 
        music = RT_NULL; 
    }
    
    if(session != RT_NULL)
    {
        webclient_close(session);
        session = RT_NULL; 
    }

    if(resp_buff != RT_NULL)
    {
        rt_free(resp_buff);
        resp_buff = RT_NULL; 
    }

    return ret;
}

static void kanime_run(void *p)
{
    LOG_D("kanime thread start run."); 
    
    while (1)
    {
        /* 随机播放歌曲 */ 
        LOG_I("kanime random play next music.");
        kanime_randomplay_music();
        
        /* 等待播放完毕, 或者用户主动停止后再切换歌曲 */ 
        while(player_get_state() != PLAYER_STAT_STOPPED)
        {
            rt_thread_mdelay(1000);
        }
    }
}

int kanime_init(void)
{
    rt_err_t ret = (RT_EOK);
    rt_thread_t thread = RT_NULL;

    thread = rt_thread_create("kanime", kanime_run, RT_NULL, Kanime_ThreadStack, 16, 10);
    if (thread == RT_NULL)
    {
        LOG_E("kanime thread create failed.");
        ret = (-RT_ERROR);
        goto _ret;
    }

    rt_thread_startup(thread);

_ret:
    return ret;
}
