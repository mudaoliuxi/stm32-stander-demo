/**
 ****************************************************************************************************
 * @file        videoplayer.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-25
 * @brief       视频播放器应用代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 M100Z-M7最小系统板STM32H750版
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#include "string.h"
#include "./MJPEG/avi.h"
#include "./MJPEG/mjpeg.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/KEY/key.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/TIMER/btim.h"
#include "./TEXT/text.h"
#include "./MALLOC/malloc.h"
#include "./APP/videoplayer.h"
#include "./FATFS/exfuns/exfuns.h"

uint8_t g_frame;            /* 统计帧率 */
uint8_t *g_psaibuf;         /* 音频缓冲帧,共1帧,5K字节,由于没有音频硬件，所以实际并没有用到,仅用于读取ID用 */
volatile uint8_t g_frameup; /* 视频播放时隙控制变量,当等于1的时候,可以更新下一帧视频 */

/**
 * @brief       得到path路径下,目标文件的总个数
 * @param       path    : 路径
 * @retval      总有效文件数
 */
static uint16_t video_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    DIR tdir;                                                   /* 临时目录 */
    FILINFO *tfileinfo;                                         /* 临时文件信息 */
    
    tfileinfo = (FILINFO *)mymalloc(SRAM12, sizeof(FILINFO));   /* 申请内存 */
    res = (uint8_t)f_opendir(&tdir, (const TCHAR *)path);       /* 打开目录 */
    
    if ((res == FR_OK) && tfileinfo)
    {
        while (1)                                               /* 查询总的有效文件数 */
        {
            res = (uint8_t)f_readdir(&tdir, tfileinfo);         /* 读取目录下的一个文件 */
            
            if ((res != FR_OK) || (tfileinfo->fname[0] == 0))
            {
                break;                                          /* 错误了/到末尾了,退出 */
            }
            
            res = exfuns_file_type(tfileinfo->fname);
            
            if ((res & 0xF0) == 0x60)                           /* 取高四位,看看是不是视频文件 */
            {
                rval++;                                         /* 有效文件数增加1 */
            }
        }
    }
    
    myfree(SRAM12, tfileinfo);                                  /* 释放内存 */
    
    return rval;
}

/**
 * @brief       显示当前播放时间
 * @param       aviinfo : AVI控制结构体
 * @retval      无
 */
void video_time_show(FIL *favi, AVI_INFO *aviinfo)
{
    static uint32_t oldsec;                                                         /* 上一次的播放时间 */
    uint8_t *buf;
    uint32_t totsec = 0;                                                            /* video文件总时间 */
    uint32_t cursec;                                                                /* 当前播放时间 */
    
    totsec = (aviinfo->SecPerFrame / 1000) * aviinfo->TotalFrame;                   /* 总长度(单位:ms) */
    totsec /= 1000;                                                                 /* 秒钟数 */
    cursec = ((double)favi->fptr / favi->obj.objsize) * totsec;                     /* 当前播放到多少秒了? */
    
    if (oldsec != cursec)                                                           /* 需要更新显示时间 */
    {
        buf = mymalloc(SRAM12, 100);                                                /* 申请100字节内存 */
        oldsec = cursec; 
        
        sprintf((char *)buf, "播放时间:%02d:%02d:%02d/%02d:%02d:%02d", cursec / 3600, (cursec % 3600) / 60, cursec % 60, totsec / 3600, (totsec % 3600) / 60, totsec % 60);
        text_show_string(10, 90, lcddev.width - 10, 16, (char *)buf, 16, 0, BLUE);  /* 显示歌曲名字 */
        
        myfree(SRAM12, buf);
    }
}

/**
 * @brief       显示当前视频文件的相关信息
 * @param       aviinfo : AVI控制结构体
 * @retval      无
 */
static void video_info_show(AVI_INFO *aviinfo)
{
    uint8_t *buf;
    
    buf = mymalloc(SRAM12, 100);                                                                /* 申请100字节内存 */
    
    sprintf((char *)buf, "声道数:%d,采样率:%d", aviinfo->Channels, aviinfo->SampleRate * 10);
    text_show_string(10, 50, lcddev.width - 10, 16, (char *)buf, 16, 0, RED);                   /* 显示歌曲名字 */
    
    sprintf((char *)buf, "帧率:%d帧", 1000 / (aviinfo->SecPerFrame / 1000));
    text_show_string(10, 70, lcddev.width - 10, 16, (char *)buf, 16, 0, RED);                   /* 显示歌曲名字 */
    
    myfree(SRAM12, buf);
}

/**
 * @brief       显示视频基本信息显示
 * @param       name    : 视频名字
 * @param       index   : 当前索引
 * @param       total   : 总文件数
 * @retval      无
 */
 static void video_bmsg_show(uint8_t *name, uint16_t index, uint16_t total)
{
    uint8_t *buf;
    
    buf = mymalloc(SRAM12, 100);                                                /* 申请100字节内存 */
    
    sprintf((char *)buf, "文件名:%s", name);
    text_show_string(10, 10, lcddev.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示文件名 */
    
    sprintf((char *)buf, "索引:%d/%d", index, total);
    text_show_string(10, 30, lcddev.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示索引 */
    
    myfree(SRAM12, buf);
}

/**
 * @brief       播放一个MJPEG文件
 * @param       pname   : 文件名
 * @retval      执行结果
 * @arg         KEY0_PRES , 下一曲
 * @arg         KEY1_PRES , 上一曲
 * @arg         其他      , 错误
 */
uint8_t video_play_mjpeg(uint8_t *pname)
{
    uint8_t *framebuf;
    uint8_t *pbuf;
    FIL *favi;
    uint8_t  res = 0;
    uint32_t offset = 0;
    uint32_t nr;
    uint8_t key;
    
    g_psaibuf = (uint8_t *)mymalloc(SRAM4, AVI_AUDIO_BUF_SIZE);                                                                                 /* 申请音频内存 */
    framebuf = (uint8_t *)mymalloc(SRAMIN, AVI_VIDEO_BUF_SIZE);                                                                                 /* 申请视频buf */
    favi = (FIL *)mymalloc(SRAM12, sizeof(FIL));                                                                                                /* 申请favi内存 */
    
    if ((g_psaibuf == NULL) || (framebuf == NULL) || (favi == NULL))
    {
        printf("memory error!\r\n");
        res = 0xFF;
    }
    
    memset(g_psaibuf, 0, AVI_AUDIO_BUF_SIZE);
    
    while (res == 0)
    {
        res = (uint8_t)f_open(favi, (const TCHAR *)pname, FA_READ);                                                                             /* 打开文件 */
        
        if (res == 0)
        {
            pbuf = framebuf;
            res = (uint8_t)f_read(favi, pbuf, AVI_VIDEO_BUF_SIZE, &nr);                                                                         /* 开始读取 */
            
            if (res != 0)
            {
                printf("fread error:%d\r\n", res);
                break;
            }
            
            res = avi_init(pbuf, AVI_VIDEO_BUF_SIZE);                                                                                           /* avi解析 */
            
            if (res != 0)
            {
                printf("avi err:%d\r\n", res);
                break;
            }
            
            video_info_show(&g_avix);
            btim_tim7_int_init(g_avix.SecPerFrame / 100 - 1, 24000 - 1);                                                                        /* 10Khz计数频率,加1是100us */
            offset = avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");                                                                           /* 寻找movi ID */
            avi_get_streaminfo(pbuf + offset + 4);                                                                                              /* 获取流信息 */
            f_lseek(favi, offset + 12);                                                                                                         /* 跳过标志ID,读地址偏移到流数据开始处 */
            res = mjpeg_init((lcddev.width - g_avix.Width) / 2, 110 + (lcddev.height - 110 - g_avix.Height) / 2, g_avix.Width, g_avix.Height);  /* JPG解码初始化 */
            
            while (1)                                                                                                                           /* 播放循环 */
            {
                if (g_avix.StreamID == AVI_VIDS_FLAG)                                                                                           /* 视频流 */
                {
                    pbuf = framebuf;
                    f_read(favi, pbuf, g_avix.StreamSize + 8, &nr);                                                                             /* 读入整帧+下一数据流ID信息 */
                    res = mjpeg_decode(pbuf, g_avix.StreamSize);
                    
                    if (res != 0)
                    {
                        printf("decode error!\r\n");
                    }
                    
                    while (g_frameup == 0);                                                                                                     /* 等待时间到达(在TIM7的中断里面设置为1) */
                    
                    g_frameup = 0;                                                                                                              /* 标志清零 */
                    g_frame++;
                }
                else                                                                                                                            /* 音频流 */
                {
                    video_time_show(favi, &g_avix);                                                                                             /* 显示当前播放时间 */
                    f_read(favi, g_psaibuf, g_avix.StreamSize + 8, &nr);                                                                        /* 填充g_psaibuf */
                    pbuf = g_psaibuf;
                }
                
                key = key_scan(0);
                if (key != 0)
                {
                    res = key;
                    break;
                }
                
                if (avi_get_streaminfo(pbuf + g_avix.StreamSize) != 0)                                                                          /* 读取下一帧 流标志 */
                {
                    pbuf = framebuf;
                    res = f_read(favi, pbuf, AVI_VIDEO_BUF_SIZE, &nr);                                                                          /* 开始读取 */
                    
                    if (res == 0 && nr == AVI_VIDEO_BUF_SIZE)                                                                                   /* 读取成功,且读取了指定长度的数据 */
                    {
                        offset = avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "00dc");                                                               /* 寻找AVI_VIDS_FLAG,00dc */
                        avi_get_streaminfo(pbuf + offset);                                                                                      /* 获取流信息 */
                        
                        if (offset)
                        {
                            f_lseek(favi, (favi->fptr - AVI_VIDEO_BUF_SIZE) + offset + 8);
                        }
                    }
                    else
                    {
                        printf("g_frame error \r\n");
                        res = WKUP_PRES;
                        break;
                    }
                }
            }
            
            TIM7->CR1 &= RESET;                                                                                                                 /* 关闭定时器7 */
            lcd_set_window(0, 0, lcddev.width, lcddev.height);                                                                                  /* 恢复窗口 */
            mjpeg_free();                                                                                                                       /* 释放内存 */
            f_close(favi);
        }
    }
    
    myfree(SRAM4, g_psaibuf);
    myfree(SRAMIN, framebuf);
    myfree(SRAM12, favi);
    
    return res;
}

/**
 * @brief       播放视频
 * @param       无
 * @retval      无
 */
void video_play(void)
{
    uint8_t res;
    DIR vdir;
    FILINFO *vfileinfo;
    uint8_t *pname;
    uint16_t totavinum;
    uint16_t curindex;
    uint8_t key;
    uint32_t temp;
    uint32_t *voffsettbl;
    
    while (f_opendir(&vdir, "0:/VIDEO") != FR_OK)                               /* 打开视频文件夹 */
    {
        text_show_string(60, 190, 240, 16, "VIDEO文件夹错误!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(60, 190, 240, 206, WHITE);                                     /* 清除显示 */
        delay_ms(200);
    }
    
    totavinum = video_get_tnum("0:/VIDEO");                                     /* 得到总有效文件数 */
    
    while (totavinum == 0)                                                      /* 视频文件总数为0 */
    {
        text_show_string(60, 190, 240, 16, "没有视频文件!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(60, 190, 240, 146, WHITE);                                     /* 清除显示 */
        delay_ms(200);
    }
    
    vfileinfo = (FILINFO *)mymalloc(SRAM12, sizeof(FILINFO));                   /* 为长文件缓存区分配内存 */
    pname = (uint8_t *)mymalloc(SRAM12, 2 * FF_MAX_LFN + 1);                    /* 为带路径的文件名分配内存 */
    voffsettbl = (uint32_t *)mymalloc(SRAM12, 4 * totavinum);                   /* 申请4*totavinum个字节的内存,用于存放视频文件索引 */
    
    while ((vfileinfo == NULL) || (pname == NULL) || (voffsettbl == NULL))      /* 内存分配出错 */
    {
        text_show_string(60, 190, 240, 16, "内存分配失败!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(60, 190, 240, 146, WHITE);                                     /* 清除显示 */
        delay_ms(200);
    }
    
    res = (uint8_t)f_opendir(&vdir, "0:/VIDEO");                                /* 打开目录 */
    
    if (res == FR_OK)
    {
        curindex = 0;                                                           /* 当前索引为0 */
        
        while (1)                                                               /* 全部查询一遍 */
        {
            temp = vdir.dptr;                                                   /* 记录当前offset */
            res = (uint8_t)f_readdir(&vdir, vfileinfo);                              /* 读取目录下的一个文件 */
            
            if ((res != FR_OK) || (vfileinfo->fname[0] == 0))
            {
                break;                                                          /* 错误了/到末尾了,退出 */
            }
            
            res = exfuns_file_type(vfileinfo->fname);
            
            if ((res & 0xF0) == 0x60)                                           /* 取高四位,看看是不是音乐文件 */
            {
                voffsettbl[curindex] = temp;                                    /* 记录索引 */
                curindex++;
            }
        }
    }
    
    curindex = 0;                                                               /* 从0开始显示 */
    res = (uint8_t)f_opendir(&vdir, "0:/VIDEO");                                /* 打开目录 */
    
    while (res == FR_OK)                                                        /* 打开成功 */
    {
        dir_sdi(&vdir, voffsettbl[curindex]);                                   /* 改变当前目录索引 */
        res = (uint8_t)f_readdir(&vdir, vfileinfo);                             /* 读取目录下的一个文件 */
        
        if ((res != FR_OK) || (vfileinfo->fname[0] == 0))
        {
            break;                                                              /* 错误了/到末尾了,退出 */
        }
        
        strcpy((char *)pname, "0:/VIDEO/");                                     /* 复制路径(目录) */
        strcat((char *)pname, (const char *)vfileinfo->fname);                  /* 将文件名接在后面 */
        lcd_clear(WHITE);                                                       /* 先清屏 */
        video_bmsg_show((uint8_t *)vfileinfo->fname, curindex + 1, totavinum);  /* 显示名字,索引等信息 */
        key = video_play_mjpeg(pname);                                          /* 播放这个音频文件 */
        
        if (key == KEY0_PRES)                                                   /* 上一个视频 */
        {
            if (curindex != 0)
            {
                curindex--;
            }
            else
            {
                curindex = totavinum - 1;
            }
        }
        else if (key == WKUP_PRES)                                              /* 下一个视频 */
        {
            curindex++;
            
            if (curindex >= totavinum)
            {
                curindex = 0;                                                   /* 到末尾的时候,自动从头开始 */
            }
        }
        else
        {
            break;                                                              /* 产生了错误 */
        }
    }
    
    myfree(SRAM12, vfileinfo);                                                  /* 释放内存 */
    myfree(SRAM12, pname);                                                      /* 释放内存 */
    myfree(SRAM12, voffsettbl);                                                 /* 释放内存 */
}
