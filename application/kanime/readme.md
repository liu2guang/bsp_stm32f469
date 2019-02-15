// https://github.com/JxiaoC/animeMusic

/*
{
    msg: "ok",
    code: 0,
    res: {
        anime_info: {
            bg: "http://i1.fuimg.com/510372/a7d92b4163395fd3.jpg",
            year: 2014,
            id: "5b8369c4b02de2130c916266",
            title: "境界触发者",
            atime: 1425915035,
            desc: "某一天﹐通往异世界的门打开了﹐从异世界而来的侵略者“近界民”﹐蹂躏门附近的地区﹐街上被恐怖所覆盖。突然出现的迷之团体“BORDER”击退了“近界民”﹐为了对抗陆续来袭的“近界民”﹐并在这边的世界建成基地。 　　四年后﹐“近界民”空闲游真从异世界来到日本寻找父亲的熟人﹐并遇上三云修。 修为了解“近界民”的真相﹐决定指引及监视人生路不熟的游真﹐两人的故事随着“BORDER”与“近界民”的战斗展开。",
            logo: "http://i1.fuimg.com/510372/29d718b13038e23b.jpg",
            month: 10,
        },
        play_url: "http://anime-music.files.jijidown.com/5b84dfd7b02de2088268793e_128.mp3?t=1538018792&sign=50FDEEE90BCB0612BD9C0C2E3CE4FF46",
        type: "其他",
        recommend: true,
        title: "GIRIGIRI",
        atime: 1535434711,
        id: "5b84dfd7b02de2088268793e",
        author: "未知",
    },
} */

// 随机返回一首音乐
// GET https://anime-music.jijidown.com/api/v2/music

// 返回指定ID的信息
// GET https://anime-music.jijidown.com/api/v2/music/5b84dfd7b02de2088268793e

// 搜索歌曲
// GET https://anime-music.jijidown.com/api/v2/music/search?key=%E7%9A%84&limit=5&page=1

// 1. 随机FM播放
// 2. 喜欢收藏, 取消收藏
// 3. 播放喜欢的指定歌曲
// 4. 下载歌曲到本地