/*
 * =====================================================================================
 *
 *       Filename:  udx_analysis.c
 *
 *    Description:      
 *
 *        Version:  1.0
 *        Created:  2016年11月10日 13时59分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
//two egg
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "tinystr.h"
#include "tinyxml.h"
#include <iostream>
#include "udx_analysis.h"
#include "udx_xml.h"


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define UDXNOTIFYNAME "Notify"
#define UDXREANAME "reason"
#define UDXRESNAME "result"
#define UDXREQNAME "Request"
#define UDXRESPNAME "Response"
#define UDXCMDNAME "cmd"
#define UDXSEQNAME "seq"

#define TABLESIZE(tab) (sizeof(tab) / sizeof(tab[0]))
#define UDX_TEMP_FILE "/tmp/udx_msg.xml"

static struct udx_msg udx_param;

/*
<注册> 交换->拼接器 

struct return_status{
    int seq;
    char mixerIds[64];
    char result[16];
    char reason[64];
};
*/
udx_keyword_ctx_t register_keyword_ctx[] = {
    {"Expires", "%s", offsetof(struct udx_msg, mixer_register.expires)},
    {"RetryTimes", "%d", offsetof(struct udx_msg, mixer_register.retryTimes)},
    {"IDS@MixerInfo", "%s", offsetof(struct udx_msg, mixer_register.mixer_info.mixerID)},
    {"MaxSplitNum@MixerInfo", "%d", offsetof(struct udx_msg, mixer_register.mixer_info.maxSplitNum)},
    {"SplitMode@MixerInfo", "%d", offsetof(struct udx_msg, mixer_register.mixer_info.splitMode)},
    {"VideoResolution@MixerInfo", "%s", offsetof(struct udx_msg, mixer_register.mixer_info.videoResolution)},
    {"VideoFrameRate@MixerInfo", "%d", offsetof(struct udx_msg, mixer_register.mixer_info.videoFrameRate)},
    {"VideoEncodeType@MixerInfo", "%s", offsetof(struct udx_msg, mixer_register.mixer_info.videoEncodeType)},
    {"AudioEncodeType@MixerInfo", "%s", offsetof(struct udx_msg, mixer_register.mixer_info.audioEncodeType)},
};

//<注销> <交换->拼接器>
udx_keyword_ctx_t register_update_keyword_ctx[] = {
    {"MixerIDS", "%s", offsetof(struct udx_msg, mixer_unregister.mixerID)},
};

//<注册保活> <交换->拼接器>
udx_keyword_ctx_t unregister_keyword_ctx[] = {
    {"MixerIDS", "%s", offsetof(struct udx_msg, mixer_register_update.mixerID)},
};

udx_keyword_ctx_t return_status_keyword_ctx[] = {
    {"result", "%s", offsetof(struct udx_msg, result)},
    {"reasion", "%s", offsetof(struct udx_msg,reason)},
};

/*
<订阅反馈当前拼接器的信息> <交换->拼接器> 返回return_status
*/

udx_keyword_ctx_t get_info_keyword_ctx[] = {
};

/*
<设置分屏模式> <交换->拼接器> 返回return_status
struct split_screen{
    int seq;
    int mode;
};
*/


udx_keyword_ctx_t split_screen_keyword_ctx[] = {
    {"Mode", "%d", offsetof(struct udx_msg, split_screen.mode)},
};

/*
<图像切换> <交换->拼接器> 返回return_status
struct switch_video{
    int seq;
    int sourcIndex;
    int destIndex;
};
*/

udx_keyword_ctx_t switch_video_keyword_ctx[] = {
    {"SourceIndex", "%d", offsetof(struct udx_msg, switch_video.sourceIndex)},
    {"DestIndex", "%d", offsetof(struct udx_msg, switch_video.destIndex)},
};

/*
<单屏放大,还原> <交换->拼接器> 返回return_status
struct full_screen{
    int seq;
    int index;
    int fullScreenState;
};
*/

udx_keyword_ctx_t full_screen_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, full_screen.index)},
    {"FullScreenState", "%d", offsetof(struct udx_msg, full_screen.fullScreenState)},
};
/* 
<单屏暂停,恢复> <交换->拼接器> 返回return_status
<拼接器暂停,恢复显示> <交换->拼接器> 返回return_status
struct play_state{
    int seq;
    int index;
    int playState;
};
*/

udx_keyword_ctx_t chn_play_state_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, chn_play_state.index)},
    {"PlayState", "%d", offsetof(struct udx_msg, chn_play_state.playState)},
};

udx_keyword_ctx_t  play_state_keyword_ctx[] = {
    {"PlayState", "%d", offsetof(struct udx_msg, play_state.playState)},
};

/* 
<设置单路源声音> <交换->拼接器> 返回return_status
struct dev_audio_state{
    int seq;
    int index;
    int audioState;
    int volume;
};
*/

udx_keyword_ctx_t chn_audio_state_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, chn_audio_state.index)},
    {"AudioState", "%d", offsetof(struct udx_msg, chn_audio_state.audioState)},
    {"Volume", "%d", offsetof(struct udx_msg, chn_audio_state.volume)},
};

/* 
<设置拼接器声音> <交换->拼接器 返回return_status
struct audio_status{
    int seq;
    int audioState;
    int volume;
};
*/

udx_keyword_ctx_t audio_state_keyword_ctx[] = {
    {"Audiostate", "%d", offsetof(struct udx_msg, audio_state.audioState)},
    {"Volume", "%d", offsetof(struct udx_msg, audio_state.volume)},
};

/* 
<设置拼接器强制I帧> <交换->拼接器> 返回return_status
struct forced_i_frames{
    int seq;
};
*/

udx_keyword_ctx_t forced_i_frames_keyword_ctx[] = {
};

/*
<设置拼接源OSD> <交换->拼接器> 返回return_status index = -1 全局OSD
struct osd{
    int seq;
    int index;
    int show;
    int left;
    int top;
    int width;
    int height;
    int color;
    int fontSize;
    char text[256];
};
*/

udx_keyword_ctx_t osd_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, osd.index)},
    {"DevIDS", "%s", offsetof(struct udx_msg, osd.mixerID)},
    {"DevCH", "%d", offsetof(struct udx_msg, osd.devCH)},
    {"Show", "%d", offsetof(struct udx_msg, osd.show)},
    {"Left@OSDInfo[288:224]@OSDInfoList", "%f", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].left)},
    {"Top@OSDInfo[288:224]@OSDInfoList", "%f", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].top)},
    {"Color@OSDInfo[288:224]@OSDInfoList", "%d", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].color)},
    {"FontName@OSDInfo[288:224]@OSDInfoList", "%s", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].fontName)},
    {"FontSize@OSDInfo[288:224]@OSDInfoList", "%f", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].fontSize)},
    {"Text@OSDInfo[288:224]@OSDInfoList", "%s", offsetof(struct udx_msg, osd.osd_info_list.osd_info[0].text)},
};

/* 
<设置拼接器编码参数> <交换->拼接器> 返回return_status
struct encode_param{
    int seq;
    char mixerIds[64];
    struct{
        int width;
        int height;
        int frameRate;
        int encodeRate;
        int encodeType;
    }video;
    struct{
        int channels;
        int samplesRate;
        int encodeType;
    }audio;
};
*/

udx_keyword_ctx_t encode_param_keyword_ctx[] = {
    {"MixerIDS", "%s", offsetof(struct udx_msg, encode_param.mixerID)},
    {"Width@Video", "%d", offsetof(struct udx_msg, encode_param.video.width)},
    {"Height@Video", "%d", offsetof(struct udx_msg, encode_param.video.height)},
    {"FrameRate@Video", "%d", offsetof(struct udx_msg, encode_param.video.frameRate)},
    {"EncodeRate@Video", "%d", offsetof(struct udx_msg, encode_param.video.encodeRate)},
    {"EncodeType@Video", "%d", offsetof(struct udx_msg, encode_param.video.encodeType)},
    {"Channels@Audio", "%d", offsetof(struct udx_msg, encode_param.audio.channels)},
    {"SamplesRate@Audio", "%d", offsetof(struct udx_msg, encode_param.audio.samplesRate)},
    {"EncodeType@Audio", "%d", offsetof(struct udx_msg, encode_param.audio.encodeType)},
};

/* 
<设置拼接器分辨率> <交换->拼接器> 返回return_status
struct video_resolution{
    int seq;
    int width;
    int height;
};
*/

udx_keyword_ctx_t video_resolution_keyword_ctx[] = {
    {"Width", "%d", offsetof(struct udx_msg, encode_param.video.width)},
    {"Height", "%d", offsetof(struct udx_msg, encode_param.video.height)},
};

//<设置拼接器视频帧率> <交换->拼接器> 返回return_status
udx_keyword_ctx_t video_framerate_keyword_ctx[] = {
    {"FrameRate", "%d", offsetof(struct udx_msg, encode_param.video.frameRate)},
};

/* 
//<设置拼接器视频码率> <交换->拼接器> 返回return_status
struct bitrate{
    int seq;
    int bitRate;
};
*/

udx_keyword_ctx_t bitrate_keyword_ctx[] = {
    {"BitRate", "%d", offsetof(struct udx_msg, encode_param.video.encodeRate)},
};

//<设置拼接器音频声道数> <交换->拼接器> 返回return_status
udx_keyword_ctx_t audio_channels_keyword_ctx[] = {
    {"Channels", "%d", offsetof(struct udx_msg, encode_param.audio.channels)},
};

//<设置拼接器音频采样率> <交换->拼接器> 返回return_status
udx_keyword_ctx_t audio_samplesrate_keyword_ctx[] = {
    {"SamplesRate", "%d", offsetof(struct udx_msg, encode_param.audio.samplesRate)},
};

//<设置拼接器音频编码方式> <交换->拼接器> 返回return_status
udx_keyword_ctx_t audio_encodetype_keyword_ctx[] = {
    {"EncodeType", "%d", offsetof(struct udx_msg, encode_param.audio.encodeType)},
};
 
//<设置拼接器视频编码方式> <交换->拼接器> 返回return_status
udx_keyword_ctx_t video_encodetype_keyword_ctx[] = {
    {"EncodeType", "%d", offsetof(struct udx_msg, encode_param.video.encodeType)},
};

//<单路点播> <交换->拼接器> 返回return_status
udx_keyword_ctx_t chn_av_play_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, av_play.index)},
    {"DevIDS", "%s", offsetof(struct udx_msg, av_play.mixerID)},
    {"DevIP", "%s", offsetof(struct udx_msg, av_play.devIP)},
    {"DevCH", "%d", offsetof(struct udx_msg, av_play.devCH)},
    {"DevType", "%d", offsetof(struct udx_msg, av_stop_play.devType)},
    {"StreamType", "%d", offsetof(struct udx_msg, av_play.streamType)},
    {"AudioState", "%d", offsetof(struct udx_msg, av_play.audioState)},
    {"Volume", "%d", offsetof(struct udx_msg, av_play.volume)},
};

//<单路停止点播> <交换->拼接器> 返回return_status
udx_keyword_ctx_t chn_av_stop_play_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, av_stop_play.index)},
    {"DevIDS", "%s", offsetof(struct udx_msg, av_stop_play.mixerID)},
    {"DevIP", "%s", offsetof(struct udx_msg, av_stop_play.devIP)},
    {"DevCH", "%d", offsetof(struct udx_msg, av_stop_play.devCH)},
    {"DevType", "%d", offsetof(struct udx_msg, av_stop_play.devType)},
    {"StreamType", "%d", offsetof(struct udx_msg, av_stop_play.streamType)},
};

#ifdef DEBUG
udx_keyword_ctx_t test_keyword_ctx[] {
    {"MixerID", "%s", offsetof(struct udx_msg, test.mixerID)},
    {"PosID[4]@Src[40:212]", "%d", offsetof(struct udx_msg, test.src[0].posID)},
    {"PosID@Dst", "%d", offsetof(struct udx_msg, test.dst.posID)},
};
#endif

//<拼接器的位置> <交换->拼接器>
udx_keyword_ctx_t mixer_pos_info_keyword_ctx[] = {
    {"MixerIDS", "%s", offsetof(struct udx_msg, mixer_pos_info.mixerID)},
    {"xPos", "%d", offsetof(struct udx_msg, mixer_pos_info.xPos)},
    {"yPos", "%s", offsetof(struct udx_msg, mixer_pos_info.yPos)},
    {"CaseID", "%s", offsetof(struct udx_msg, mixer_pos_info.caseID)},
};

udx_keyword_ctx_t mixer_pos_info_resp_keyword_ctx[] = {
    {"Index", "%d", offsetof(struct udx_msg, mixer_pos_info.index)},
    {"CaseID", "%s", offsetof(struct udx_msg, mixer_pos_info.caseID)},
};

//<停止全部点播> <交换->拼接器> 返回return_status

//<混音器申请> <交换->拼接器> 返回return_status
udx_keyword_ctx_t audio_mix_apply_keyword_ctx[] = {
    {"MixInput", "%d", offsetof(struct udx_msg, audio_mix_apply.mixInput)},
    {"InputCodec", "%s", offsetof(struct udx_msg, audio_mix_apply.mixInput)},
    {"MixOutput", "%d", offsetof(struct udx_msg, audio_mix_apply.mixOutput)},
    {"OutputCodec", "%s", offsetof(struct udx_msg, audio_mix_apply.outputCodec)},
};

udx_keyword_ctx_t audio_mix_apply_replay_keyword_ctx[] = {
    {"PosID[]@Src", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.srcPosID)},
    {"PosID@Dst", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.dstPosID)},
};

//<混音器释放> <交换->拼接器>
udx_keyword_ctx_t audio_mix_drop_keyword_ctx[] = {
};


udx_keyword_ctx_t audio_mix_drop_repsp_keyword_ctx[] = {
    {"PosID[]@Src", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.srcPosID)},
    {"PosID@Dst", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.dstPosID)},
};

//<混音器设置> <交换->拼接器>
udx_keyword_ctx_t audio_mix_set_keyword[] = {
};


struct udx_cmd{
    char *udx_cmd_str;
    int index;
    udx_cmd_proc_func udx_cmd_proc;
}udx_cmd_map[] = {
    {"AVMixerRegister", 0,},
    {"AVMixerUnregister", 1,},
    {"AVMixerRegisterUpdate", 2,},
    {"AVMixerGetInfo", 3,}, //FIXME
    {"AVMixerStateChange", 4,},//FIXME
    {"AVMixerSetSplitMode", 5,},
    {"AVMixerSwitchVideo", 6,},
    {"AVMixerCtrlFullScreen", 7,},
    {"AVMixerCtrlDevPlayState", 8,},
    {"AVMixerCtrlPlayState", 9,},
    {"AVMixerCtrlDevAudioState", 10,},
    {"AVMixerCtrlAudioState", 11, },
    {"AVMixerCtrlForcedIFrames", 12, },
    {"AVMixerSetOSD", 13, },
    {"AVMixerSetEncodeParam", 14, },
    {"AVMixerSetResolution", 15, },
    {"AVMixerSetFrameRate", 16, },
    {"AVMixerSetBitRate", 17, },
    {"AVMixerSetChannels", 18, },
    {"AVMixerSetSamplesRate", 19, },
    {"AVMixerSetAudioEncodeType", 20, },
    {"AVMixerSetVideoEncodeType", 21, },
    {"AVMixerStartPlay", 22, },
    {"AVMixerStopPlay", 23, },
    {"AVMixerStopAllPlay", 24,},
#ifdef DEBUG
    {"AVMixerTest", 25},
    {"AVMixerGetPosInfo", 26},
#else
    {"AVMixerAudioMixApply", 25, },
    {"AVMixerAudioMixDrop", 26,},
    {"AVMixerAudioMixSet", 27, },
    {"AVMixerVideoMixApply", 28,},
    {"AVMixerVideoMixDrop", 29, },
    {"AVMixerVideoMixSet", 30, },
    {"AVMixerInvite", 31, },
    {"AVMixerAck", 32, },
    {"AVMixerBye", 33, },
    {"AVMixerUpdate", 34, },
#endif
};


static struct udx_ctx{
    char *cmd_str;
    udx_keyword_ctx_t *recv_keyword_ctx_tab;
    udx_keyword_ctx_t *send_keyword_ctx_tab;
    size_t tab_num;
    char resp;
}udx_ctx_info[] = {
    {"AVMixerRegister", return_status_keyword_ctx, register_keyword_ctx, TABLESIZE(return_status_keyword_ctx) | (TABLESIZE(register_keyword_ctx) << 16), FALSE},
    {"AVMixerUnregister", return_status_keyword_ctx, unregister_keyword_ctx, TABLESIZE(return_status_keyword_ctx) | (TABLESIZE(unregister_keyword_ctx) << 16), FALSE},
    {"AVMixerRegisterUpdate", return_status_keyword_ctx, register_update_keyword_ctx, TABLESIZE(return_status_keyword_ctx) | (TABLESIZE(register_update_keyword_ctx) << 16), FALSE},
    {"AVMixerGetInfo", get_info_keyword_ctx, NULL, TABLESIZE(get_info_keyword_ctx), FALSE},
    {"AVMixerStateChange", NULL, NULL, 0, FALSE },
    {"AVMixerSetSplitMode", split_screen_keyword_ctx, NULL,TABLESIZE(split_screen_keyword_ctx), TRUE},
    {"AVMixerSwitchVideo", switch_video_keyword_ctx, NULL,  TABLESIZE(switch_video_keyword_ctx), TRUE},
    {"AVMixerCtrlFullScreen", full_screen_keyword_ctx, NULL, TABLESIZE(full_screen_keyword_ctx), TRUE},
    {"AVMixerCtrlDevPlayState", chn_play_state_keyword_ctx, NULL, TABLESIZE(chn_play_state_keyword_ctx), TRUE},
    {"AVMixerCtrlPlayState", play_state_keyword_ctx, NULL, TABLESIZE(play_state_keyword_ctx), TRUE},
    {"AVMixerCtrlDevAudioState", chn_audio_state_keyword_ctx, NULL,TABLESIZE(chn_audio_state_keyword_ctx), TRUE},
    {"AVMixerCtrlAudioState", audio_state_keyword_ctx, NULL,TABLESIZE(audio_state_keyword_ctx), TRUE},
    {"AVMixerCtrlForcedIFrames", NULL, return_status_keyword_ctx, 0, TRUE},
    {"AVMixerSetOSD", osd_keyword_ctx, return_status_keyword_ctx, TABLESIZE(osd_keyword_ctx), TRUE},
    {"AVMixerSetEncodeParam", encode_param_keyword_ctx, return_status_keyword_ctx, TABLESIZE(encode_param_keyword_ctx), TRUE},
    {"AVMixerSetResolution", video_resolution_keyword_ctx, return_status_keyword_ctx, TABLESIZE(video_resolution_keyword_ctx), TRUE},
    {"AVMixerSetFrameRate", video_framerate_keyword_ctx, return_status_keyword_ctx, TABLESIZE(video_framerate_keyword_ctx), TRUE},
    {"AVMixerSetBitRate", bitrate_keyword_ctx, return_status_keyword_ctx, TABLESIZE(bitrate_keyword_ctx), TRUE},
    {"AVMixerSetChannels",audio_channels_keyword_ctx, return_status_keyword_ctx, TABLESIZE(audio_channels_keyword_ctx), TRUE},
    {"AVMixerSetSamplesRate", audio_samplesrate_keyword_ctx, return_status_keyword_ctx, TABLESIZE(audio_samplesrate_keyword_ctx), TRUE},
    {"AVMixerSetAudioEncodeType", audio_encodetype_keyword_ctx, return_status_keyword_ctx, TABLESIZE(audio_encodetype_keyword_ctx), TRUE},
    {"AVMixerSetVideoEncodeType", video_encodetype_keyword_ctx, return_status_keyword_ctx, TABLESIZE(video_encodetype_keyword_ctx), TRUE},
    {"AVMixerStartPlay", chn_av_play_keyword_ctx, return_status_keyword_ctx, TABLESIZE(chn_av_play_keyword_ctx), TRUE},
    {"AVMixerStopPlay", chn_av_stop_play_keyword_ctx, return_status_keyword_ctx, TABLESIZE(chn_av_stop_play_keyword_ctx), TRUE},
    {"AVMixerStopAllPlay", NULL, return_status_keyword_ctx, 0, TRUE},
#ifdef DEBUG
    {"AVMixerTest",test_keyword_ctx, test_keyword_ctx, TABLESIZE(test_keyword_ctx) | (TABLESIZE(test_keyword_ctx) << 16), TRUE},
    {"AVMixerGetPosInfo", mixer_pos_info_keyword_ctx, mixer_pos_info_resp_keyword_ctx, TABLESIZE(mixer_pos_info_keyword_ctx) |(TABLESIZE(mixer_pos_info_resp_keyword_ctx) << 16), TRUE},
#endif
    //{"AVMixerAudioMixApply", audio_mix_apply_keyword_ctx, NULL, TABLESIZE(audio_mix_apply_keyword_ctx), TRUE},
};

static int udx_cmd_item_cmp(const void *src, const void *dst)
{
   const struct udx_cmd *pSrc = src; 
   const struct udx_cmd *pDst = dst; 
   return strcmp(pSrc->udx_cmd_str, pDst->udx_cmd_str);
}

int udx_anlysis_init()
{
    qsort((void *)udx_cmd_map, TABLESIZE(udx_cmd_map), sizeof(struct udx_cmd), udx_cmd_item_cmp);
}

/*
#define UDXCMD2INDEX(cmd) ({    \
        struct udx_cmd data = {cmd, };  \
        int index;  \
        index = bsearch((void *)&data, (void *)udx_cmd_map, TABLESIZE(udx_cmd_map), sizeof(struct udx_cmd), udx_cmd_item_cmp);  \
        index  \
        })
        */

static struct udx_cmd *udx_ctx_item_get(char *command, struct udx_ctx **out_data)
{
    int index;  
    struct udx_cmd data = {command, }, *p;  
    *out_data = NULL;
    p = bsearch((void *)&data, (void *)udx_cmd_map, TABLESIZE(udx_cmd_map), sizeof(struct udx_cmd), udx_cmd_item_cmp);  
    if(p)
    {
        *out_data = &udx_ctx_info[p->index];
    }
    return p;
}

static char *strrbrk(const char *buf, const char *delim)
{
    int pos = 0;
    if(buf)
    {
        pos = strlen(buf) ;
        while(pos >= 0)
        {
            if(strstr(&buf[pos], delim)) 
            {
                memset(&buf[pos], '\0', strlen(delim)); 
                pos += strlen(delim);
                break;
            }
            pos --;
            if(pos <= 0)
                break;
        }
        if(pos >= 0 && buf[pos] != '\0')
            return &buf[pos];
    }
    return NULL;
}

static int _udx_xml_element_generate(const char *path, udx_keyword_ctx_t *ukc, TiXmlElement *father, char *addr)
{
    char *token, *str, *brk, buf[256];
    int len;
    TiXmlElement *elem = father, *e;
    len = strlen(path);
    strncpy(buf, path, 256);
    if((!strchr(buf, '@')) && elem)
    {
        char elem_txt[64] = {0};
        addr += ukc->member_ofset;
        if(brk = strstr(buf, "["))
        {
            char *value;
            char elem_name[64];
            int size;
            sscanf(buf, "%[^[]", elem_name);
            sscanf(brk, "[%d]", &size);
            for(addr ; !UDX_DATA_IS_END(addr); addr += size)
            {
                if(strcmp(ukc->fmt, "%s") != 0)
                    sprintf(elem_txt, ukc->fmt, *(int *)addr);
                else
                    strncpy(elem_txt, addr, 64);
                e = new TiXmlElement(elem_name); 
                
                e->LinkEndChild( new TiXmlText(elem_txt)); 
                elem->LinkEndChild(e); 
            }
        }
        else
        {
            if(strcmp(ukc->fmt, "%s") != 0)
                sprintf(elem_txt, ukc->fmt, *(int *)addr);
            else
                strncpy(elem_txt, addr, 64);
            e = new TiXmlElement(buf); 
            e->LinkEndChild( new TiXmlText(elem_txt)); 
            elem->LinkEndChild(e); 
        }
    }
    else
    {
        for(str = buf; (token = strrbrk(str, "@")) && (token != str) ; )
        {
            char elem_name[64];
            sscanf(token, "%[^[]", elem_name);
            if(brk = strstr(token, "["))
            {
                int size , offset;
                char *elem_addr;
                sscanf(brk, "[%d:%d]", &size, &offset);
                for(elem_addr = addr + offset ; !UDX_DATA_IS_END(elem_addr) ;elem_addr += size, addr += size)
                {
                    e = new TiXmlElement(elem_name); 
                    elem->LinkEndChild(e); 
                    _udx_xml_element_generate(str, ukc, e, addr);
                }
            }
            else
            {
                if(elem->FirstChildElement(elem_name) == NULL)
                {
                    e = new TiXmlElement(elem_name); 
                    elem->LinkEndChild(e); 
                }
                _udx_xml_element_generate(str, ukc, e, addr);

            }

        }
    }

    return 0;
}

static int _udx_xml_jk_mixer_info_depacked(TiXmlElement *root, struct udx_msg *um)
{
    TiXmlElement *mixer_info, *layout, *dbinfo, *pos;
    mixer_info = new TiXmlElement("MixerInfo"); 
    {
        layout = new TiXmlElement("Layout");
        layout->SetAttribute("MixerIDS", um->mixer_get_info.layout.mixerID);
        layout->SetAttribute("MixerType", um->mixer_get_info.layout.mixerType);
        layout->SetAttribute("SplitNum", um->mixer_get_info.layout.splitNum);
        layout->SetAttribute("VideoResolution", um->mixer_get_info.layout.videoResolution);
        layout->SetAttribute("VideoFrameRate", um->mixer_get_info.layout.videoFrameRate);
        layout->SetAttribute("EncodeRate", um->mixer_get_info.layout.encodeRate);
        layout->SetAttribute("EncodeType", um->mixer_get_info.layout.encodeType);
        layout->SetAttribute("AllOsdState", um->mixer_get_info.layout.allOsdState);
        for(int i = 0;!UDX_DATA_IS_END(&um->mixer_get_info.layout.dbInfo[i]); i ++)
        {
            dbinfo = new TiXmlElement("DBInfo");
            layout->LinkEndChild(dbinfo);
            dbinfo->SetAttribute("DBType", um->mixer_get_info.layout.dbInfo[i].dbType);
            dbinfo->SetAttribute("Volume", um->mixer_get_info.layout.dbInfo[i].volume);
            dbinfo->SetAttribute("PlayState",um->mixer_get_info.layout.dbInfo[i].playState);
            for(int j = 0; !UDX_DATA_IS_END(&um->mixer_get_info.layout.dbInfo[i].pos[j]); j ++)
            {
                pos = new TiXmlElement("Pos");
                dbinfo->LinkEndChild(pos);            
                pos->SetAttribute("Index", um->mixer_get_info.layout.dbInfo[i].pos[j].index);
                pos->SetAttribute("DevIDS", um->mixer_get_info.layout.dbInfo[i].pos[j].mixerID);
                pos->SetAttribute("DevCH", um->mixer_get_info.layout.dbInfo[i].pos[j].devCH);
                pos->SetAttribute("StreamType", um->mixer_get_info.layout.dbInfo[i].pos[j].streamType);
                pos->SetAttribute("AudioState", um->mixer_get_info.layout.dbInfo[i].pos[j].audioState);
                pos->SetAttribute("Volume", um->mixer_get_info.layout.dbInfo[i].pos[j].volume);
                pos->SetAttribute("IsFullScreen", um->mixer_get_info.layout.dbInfo[i].pos[j].isFullScreen);
                pos->SetAttribute("PlayState", um->mixer_get_info.layout.dbInfo[i].pos[j].playState);
                pos->SetAttribute("OsdState", um->mixer_get_info.layout.dbInfo[i].pos[j].osdState);
                pos->SetDoubleAttribute("LeftRate", um->mixer_get_info.layout.dbInfo[i].pos[j].leftRate);
                pos->SetDoubleAttribute("TopRate", um->mixer_get_info.layout.dbInfo[i].pos[j].topRate);
                pos->SetDoubleAttribute("WidthRate", um->mixer_get_info.layout.dbInfo[i].pos[j].widthRate);
                pos->SetDoubleAttribute("HeightRate", um->mixer_get_info.layout.dbInfo[i].pos[j].heightRate);
            }
        }
    }
    mixer_info->LinkEndChild(layout);
    root->LinkEndChild(mixer_info);
}

static int _udx_xml_jk_mixer_status_depacked(TiXmlElement *root, struct udx_msg *um)
{
    TiXmlElement *mixer_data, *layout, *pos;
    mixer_data = new TiXmlElement("MixerData"); 
    {
        layout = new TiXmlElement("Layout");
        layout->SetAttribute("DBType", um->mixer_data.layout.dbType);
        layout->SetAttribute("SplitNum", um->mixer_data.layout.splitNum);
        layout->SetAttribute("SplitMode", um->mixer_data.layout.splitMode);
        layout->SetAttribute("MixerIDS", um->mixer_data.layout.mixerID);
        layout->SetAttribute("MixerType", um->mixer_data.layout.mixerType);
        layout->SetAttribute("Volume", um->mixer_data.layout.volume);
        layout->SetAttribute("VideoResolution", um->mixer_data.layout.videoResolution);
        layout->SetAttribute("VideoFrameRate", um->mixer_data.layout.videoFrameRate);
        layout->SetAttribute("EncodeRate", um->mixer_data.layout.encodeRate);
        layout->SetAttribute("EncodeType", um->mixer_data.layout.encodeType);
        layout->SetAttribute("DbState", um->mixer_data.layout.dbState);
        layout->SetAttribute("AllOsdState", um->mixer_data.layout.allOsdState);
        for(int i = 0;!UDX_DATA_IS_END(&um->mixer_data.layout.pos[i]); i ++)
        {
            pos = new TiXmlElement("pos");
            layout->LinkEndChild(pos);
            pos->SetAttribute("Index", um->mixer_data.layout.pos[i].index);
            pos->SetAttribute("DevIDS", um->mixer_data.layout.pos[i].mixerID);
            pos->SetAttribute("DevCH", um->mixer_data.layout.pos[i].devCH);
            pos->SetAttribute("StreamType", um->mixer_data.layout.pos[i].streamType);
            pos->SetAttribute("AudioState", um->mixer_data.layout.pos[i].audioState);
            pos->SetAttribute("Volume", um->mixer_data.layout.pos[i].volume);
            pos->SetAttribute("IsFullScreen", um->mixer_data.layout.pos[i].isFullScreen);
            pos->SetAttribute("OsdState", um->mixer_data.layout.pos[i].osdState);
            pos->SetDoubleAttribute("LeftRate", um->mixer_data.layout.pos[i].leftRate);
            pos->SetDoubleAttribute("TopRate", um->mixer_data.layout.pos[i].topRate);
            pos->SetDoubleAttribute("WidthRate", um->mixer_data.layout.pos[i].widthRate);
            pos->SetDoubleAttribute("HeightRate", um->mixer_data.layout.pos[i].heightRate);
        }
    }
    mixer_data->LinkEndChild(layout);
    root->LinkEndChild(mixer_data);
}

#ifdef DEBUG
static int udx_xml_jk_mixer_info_data_fill(struct udx_msg *um)
{ 
    strcpy(um->mixer_get_info.layout.mixerID, "YSPQ123");
    um->mixer_get_info.layout.mixerType = 2;
    um->mixer_get_info.layout.splitNum = 3;
    strcpy(um->mixer_get_info.layout.videoResolution, "1920*1080");
    um->mixer_get_info.layout.videoFrameRate = 4;
    um->mixer_get_info.layout.encodeRate = 5;
    um->mixer_get_info.layout.encodeType = 6;
    um->mixer_get_info.layout.allOsdState = 7;
    um->mixer_get_info.layout.dbInfo[0].dbType = 8;
    um->mixer_get_info.layout.dbInfo[0].volume = 9;
    um->mixer_get_info.layout.dbInfo[0].playState = 10;
    um->mixer_get_info.layout.dbInfo[0].pos[0].index = 100;
    um->mixer_get_info.layout.dbInfo[0].pos[0].widthRate = 3.9900;
    um->mixer_get_info.layout.dbInfo[0].pos[1].index = 1000;
    UDX_DATA_SET_END(&um->mixer_get_info.layout.dbInfo[0].pos[2]);
    um->mixer_get_info.layout.dbInfo[1].dbType = 18;
    um->mixer_get_info.layout.dbInfo[1].volume = 19;
    um->mixer_get_info.layout.dbInfo[1].playState = 20;
    um->mixer_get_info.layout.dbInfo[1].pos[0].index = 200;
    um->mixer_get_info.layout.dbInfo[1].pos[1].index = 2000;
    UDX_DATA_SET_END(&um->mixer_get_info.layout.dbInfo[1].pos[2]);
    UDX_DATA_SET_END(&um->mixer_get_info.layout.dbInfo[2]);
    
    return 0;

}

static int udx_xml_jk_mixer_status_data_fill(struct udx_msg *um)
{
    strcpy(um->cmd, "AVMixerStateChange");
    um->seq = 16;
    um->mixer_data.layout.dbType = 1;
    um->mixer_data.layout.splitNum = 2;
    um->mixer_data.layout.splitMode = 3;
    strcpy(um->mixer_data.layout.mixerID, "YSPQ125");
    um->mixer_data.layout.mixerType = 4;
    um->mixer_data.layout.volume = 5;
    strcpy(um->mixer_data.layout.videoResolution, "1920*1080");
    um->mixer_data.layout.videoFrameRate = 6;
    um->mixer_data.layout.encodeRate = 7;
    um->mixer_data.layout.encodeType= 8;
    um->mixer_data.layout.dbState = 9;
    um->mixer_data.layout.allOsdState = 10;
    um->mixer_data.layout.pos[0].index= 11;
    strcpy(um->mixer_data.layout.pos[0].mixerID, "YSPQ126");
    um->mixer_data.layout.pos[0].devCH = 12;
    um->mixer_data.layout.pos[0].streamType = 13;
    um->mixer_data.layout.pos[0].audioState = 14;
    um->mixer_data.layout.pos[0].volume= 15;
    um->mixer_data.layout.pos[0].isFullScreen= 16;
    um->mixer_data.layout.pos[0].osdState= 17;
    um->mixer_data.layout.pos[0].leftRate = 0.23;
    um->mixer_data.layout.pos[0].topRate= 1.23;
    um->mixer_data.layout.pos[0].widthRate = 2.23;
    um->mixer_data.layout.pos[0].heightRate = 3.23;
    um->mixer_data.layout.pos[1].index= 21;
    strcpy(um->mixer_data.layout.pos[1].mixerID, "YSPQ127");
    um->mixer_data.layout.pos[1].devCH = 22;
    um->mixer_data.layout.pos[1].streamType = 23;
    um->mixer_data.layout.pos[1].audioState = 24;
    um->mixer_data.layout.pos[1].volume= 25;
    um->mixer_data.layout.pos[1].isFullScreen= 26;
    um->mixer_data.layout.pos[1].osdState= 27;
    um->mixer_data.layout.pos[1].leftRate = 2.23;
    um->mixer_data.layout.pos[1].topRate= 2.23;
    um->mixer_data.layout.pos[1].widthRate = 2.23;
    um->mixer_data.layout.pos[1].heightRate = 2.23;
    UDX_DATA_SET_END(&um->mixer_data.layout.pos[2]);
}
#endif

static int _udx_xml_packed(struct udx_ctx *udx_item, TiXmlElement *root)
{
     
    int i;
    udx_keyword_ctx_t *key_tab;
    key_tab = udx_item->send_keyword_ctx_tab;
    if(key_tab)
    {
        for(i = 0; i < ((udx_item->tab_num >> 16) & 0xffff); i ++)
        {
            _udx_xml_element_generate(key_tab[i].path, &key_tab[i], root,(char *)&udx_param); 
        }
    }
    return 0;
}
#define UDX_REQ 1
#define UDX_RESP 0
static int udx_xml_generate(char *out_xml, int *ret_len, int type)
{
    TiXmlDocument doc; 
    struct udx_ctx *uc;
    struct udx_cmd *um;
    TiXmlElement * root;
    if(type == UDX_REQ)
    {
        if(strcmp(udx_param.cmd, "AVMixerStateChange") == 0)
            root = new TiXmlElement(UDXNOTIFYNAME); 
        else
            root = new TiXmlElement(UDXREQNAME); 
        root->SetAttribute(UDXCMDNAME, udx_param.cmd);
        root->SetAttribute(UDXSEQNAME, udx_param.seq);
    }
    else if(type == UDX_RESP)
    {
        root = new TiXmlElement(UDXRESPNAME); 
        root->SetAttribute(UDXCMDNAME, udx_param.cmd);
        root->SetAttribute(UDXSEQNAME, udx_param.seq);
        root->SetAttribute(UDXRESNAME, udx_param.result);
        root->SetAttribute(UDXREANAME, udx_param.reason);
    }

    doc.LinkEndChild(root); 

    um = udx_ctx_item_get(udx_param.cmd, &uc);
    if(um)
    {
        if(strcmp(udx_param.cmd, "AVMixerGetInfo") == 0)
        {
#ifdef DEBUG
            udx_xml_jk_mixer_info_data_fill(&udx_param);
#endif
            _udx_xml_jk_mixer_info_depacked(root, &udx_param);
        }
        else if(strcmp(udx_param.cmd, "AVMixerStateChange") == 0)
        {
            _udx_xml_jk_mixer_status_depacked(root, &udx_param);
        }
        else
            _udx_xml_packed(uc, root);  
       TiXmlPrinter printer;
       doc.Accept(&printer);
       if(out_xml && ret_len)
       {
           sprintf(out_xml, "%s", printer.CStr());
           *ret_len = strlen(out_xml);
       }
       else
           doc.Print();
    }
}


static TiXmlElement *_udx_xml_element_analysis(const char *path, udx_keyword_ctx_t *ukc, TiXmlElement *father, char *addr)
{
    char *token, *str, *brk, buf[256];
    int len;
    TiXmlElement *elem = father;
    len = strlen(path);
    strncpy(buf, path, 256);
    if((!strchr(buf, '@')) && elem)
    {
        addr += ukc->member_ofset;
        if(brk = strstr(buf, "["))
        {
            char *value;
            int size;
            char elem_name[64];
            sscanf(buf, "%[^[]", elem_name);
            sscanf(brk, "[%d]", &size);
            for(elem = elem->FirstChildElement(elem_name); elem; elem = elem->NextSiblingElement(elem_name))
            {
                value = elem->GetText();
                sscanf(value, ukc->fmt, addr);
                addr += size; 
            }
            UDX_DATA_SET_END(addr);
        }
        else
        {
            char *value;
            elem = elem->FirstChildElement(buf);
            value = elem->GetText();
            sscanf(value, ukc->fmt, addr);
        }
    }
    else
    {
        for(str = buf; (token = strrbrk(str, "@")) && (token != str) ; )
        {
            char elem_name[64];
            sscanf(token, "%[^[]", elem_name);
            if(brk = strstr(token, "["))
            {
                int size, offset;
                char *elem_addr;
                sscanf(brk, "[%d:%d]", &size, &offset);
                for(elem = elem->FirstChildElement(elem_name); elem; elem = elem->NextSiblingElement(elem_name))
                {
                    _udx_xml_element_analysis(str, ukc, elem, addr);
                    addr += size;            
                }
                elem_addr = addr + offset;
                UDX_DATA_SET_END(elem_addr);
            }
            else
            {
                elem = elem->FirstChildElement(elem_name); 
                _udx_xml_element_analysis(str, ukc, elem, addr);
            }

        }
    }
    return 0;
}

static int _udx_xml_depacked( struct udx_ctx *udx_item, TiXmlElement *root)
{
    int i;
    udx_keyword_ctx_t *key_tab;
    key_tab = udx_item->recv_keyword_ctx_tab;
    if(key_tab)
    {
        for(i = 0; i < (udx_item->tab_num & 0xffff); i ++)
        {
            _udx_xml_element_analysis(key_tab[i].path, &key_tab[i], root,(char *)&udx_param); 
        }
    }
    return 0;
}


struct udx_cmd* udx_xml_depacked(char *xml_cmd)
{
    char cmd[64] = {0};
    char loadOkay;
    TiXmlDocument doc;

    if(xml_cmd)
        loadOkay = doc.Parse(xml_cmd);
    else
        loadOkay = doc.LoadFile(UDX_TEMP_FILE);

    TiXmlElement *root = doc.RootElement();
    if(loadOkay)
    {
        doc.Print();
        if((strcmp(UDXRESPNAME, root->Value()) == 0))
        {
            strcpy(udx_param.result, root->Attribute(UDXRESNAME));
            strcpy(udx_param.reason, root->Attribute(UDXREANAME));
        }
        strcpy(cmd, root->Attribute(UDXCMDNAME));
        udx_param.seq = atoi( root->Attribute(UDXSEQNAME));
        strncpy(udx_param.cmd, cmd, 64);
        if(cmd[0] != '\0')
        {
            struct udx_ctx *uc;
            struct udx_cmd *um;

            um = udx_ctx_item_get(cmd, &uc);
            if(um)
            {
                _udx_xml_depacked(uc, root);
                return um;
            }
        }
    }
    else
    {
        printf("xml error\n");
    }
    return NULL;
}

static inline int udx_cmd_proc_ret_status_set(int status)
{
    if(status == 0)
    {
        strcpy(udx_param.result, "success");
        strcpy(udx_param.reason, "ok");
    }
    else
    {
        char buf[64];
        strcpy(udx_param.result, "failed");
        sprintf(buf, "%x", status);
        //存放错误码
        strcpy(udx_param.reason, buf);
    }
    return 0;
}

int udx_process_response(int *pRet)
{
    return strcasecmp(udx_param.result, "success") == 0;
}

int udx_generate_request(UDX_MSG_S * pMsg, char ** ret_buf, int *ret_len)
{
    memcpy(&udx_param, pMsg, sizeof(udx_param));
    return udx_xml_generate(*ret_buf, ret_len, UDX_REQ);
}

int udx_process_xml_msg(const char * xml_buf, int total_len, char ** ret_buf, int *ret_len)
{
    struct udx_cmd *um;
    um = udx_xml_depacked(xml_buf);
    //if(um->udx_cmd_proc)
    {
        //if(udx_cmd_proc_ret_status_set( um->udx_cmd_proc(&udx_param)))
        {
            udx_cmd_proc_ret_status_set(0);
            udx_xml_generate(*ret_buf, ret_len, UDX_RESP);
        }
    }
}

#ifdef DEBUG
void register_data()
{
    strcpy(udx_param.cmd, "AVMixerRegister");
    strcpy(udx_param.mixer_register.expires, "5s");
    udx_param.mixer_register.retryTimes = 3;
    strcpy(udx_param.mixer_register.mixer_info.mixerID, "YSPQ123");
    udx_param.mixer_register.mixer_info.maxSplitNum = 2;
    udx_param.mixer_register.mixer_info.splitMode = 4;
    strcpy(udx_param.mixer_register.mixer_info.videoResolution, "1024*768");
    udx_param.mixer_register.mixer_info.videoFrameRate = 5;
    strcpy(udx_param.mixer_register.mixer_info.videoEncodeType, "H.264");
    strcpy(udx_param.mixer_register.mixer_info.audioEncodeType, "G.711U");
}

void unregister_data()
{
    strcpy(udx_param.cmd, "AVMixerUnregister");
    strcpy(udx_param.mixer_unregister.mixerID, "YSPQ124");
}

void register_update_data()
{
    strcpy(udx_param.cmd, "AVMixerRegisterUpdate");
    strcpy(udx_param.mixer_unregister.mixerID, "YSPQ125");
}
int main(int argc, char *argv[])
{
    char xml[4096] , *p, out_xml[4096], *q;
    int len;
    udx_anlysis_init();

    if(argv[1])
    {
        FILE *fp;
        fp = fopen(argv[1], "rt");
        fread(xml, 1, 4096, fp);
        fclose(fp);
        //udx_xml_depacked(xml);
        //udx_xml_generate(xml, &len, UDX_RESP);
        p = xml;
        q = out_xml;
        udx_process_xml_msg(xml, 4096, &q, &len);
        /*
        udx_xml_jk_mixer_status_data_fill(&udx_param);
       udx_generate_request(&udx_param, &q, &len); 
       */
        printf("%s\n", out_xml);
        //register_data();
        //unregister_data();
        //register_update_data();
    }
    else
    {
    }
}
#endif
