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

#define DEBUG 1

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define UDXREANAME "reason"
#define UDXRESNAME "result"
#define UDXREQNAME "Request"
#define UDXRESPNAME "Response"
#define UDXCMDNAME "cmd"
#define UDXSEQNAME "seq"

#define TABLESIZE(tab) (sizeof(tab) / sizeof(tab[0]))
#define UDX_TEMP_FILE "/tmp/udx_msg.xml"

typedef struct {
    char *keyword;
    char *fmt;
    unsigned int member_ofset;
}udx_keyword_ctx_t;

struct udx_msg{
    int seq;
    char mixerID[64];
    char result[16];
    char reason[64];
    union{
        struct{
            char expires[8];
            int  retryTimes;
            struct{
               int maxSplitNum;
               int splitMode;
                char *videoResolution[16];
                int videoFrameRate;
                char videoEncodeType[8];
                char AudioEncodeType[8];
            }mixer_nfo;
        }mixer_register;

        struct{
           int mode; 
        }split_screen; 

        struct{
            int sourceIndex;
            int destIndex;
        }switch_video;

        struct{
            int index;
            int fullScreenState; 
        }full_screen;

        struct{
            int index;
            int playState;
        }play_state, chn_play_state;

        struct{
            int index;
            int audioState;
            int volume;
        }chn_audio_state, audio_state;

        struct{
            int index;
            int show;
            int left;
            int top;
            int width;
            int height;
            int color;
            int fontSize;
            char text[256];
        }osd;

        struct{
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
        }encode_param;
        
        struct{
            int index;
            char devIP[32];
            int devCH;
            int devType;
            int streamType;
            int audioState;
            int volume;
        
        }av_play, av_stop_play;

        struct{
           int mixInput;  
           char inputCodec[8];
           int mixOutput;
           char outputCodec[8];
        }audio_mix_apply, video_mix_apply;

        struct {
            int dstPosID;
            int srcNum;
            int srcPosID[0];
        }audio_mix_apply_resp, video_mix_drop_resp;

        struct{
            int srcNum;
            int dstNum;
            struct{
                int rtpID;
                int posID;
                char codec[8];
                char format[16];
            }src[0]; 
            struct{
                int rtpID;
                int posID;
                char codec[8];
                char format[16];
            }dst[0];
        }audio_mix_set;

        struct {
            int srcNum;
            int dstNum;
            int regionNum;
             struct{
                int rtpID;
                int posID;
                char codec[8];
                char format[16];
                char bandWidth[16];
            }src[0];        
            struct{
                int posID;
                int left;
                int top;
                char relativeSize[8];
            }region[0];
            struct{
                int rtpID;
                int posID;
                char codec[8];
                char format[16];
                char bandWidth[16];
            }dst[0];
        }video_mix_set;
    };
}udx_param;

/*
<注册> 交换->拼接器 
<注销> <交换->拼接器>
<注册保活> <交换->拼接器>

struct return_status{
    int seq;
    char mixerIds[64];
    char result[16];
    char reason[64];
};
*/

udx_keyword_ctx_t return_status_keyword_ctx[] = {
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
    {"result", "%s", offsetof(struct udx_msg, result)},
    {"reasion", "%s", offsetof(struct udx_msg,reason)},
};

/*
<订阅反馈当前拼接器的信息> <交换->拼接器> 返回return_status
*/

udx_keyword_ctx_t get_info_keyword_ctx[] = {
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
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
    {"Show", "%d", offsetof(struct udx_msg, osd.show)},
    {"Left", "%d", offsetof(struct udx_msg, osd.left)},
    {"Top", "%d", offsetof(struct udx_msg, osd.top)},
    {"Width", "%d", offsetof(struct udx_msg, osd.width)},
    {"Height", "%d", offsetof(struct udx_msg, osd.height)},
    {"Color", "%d", offsetof(struct udx_msg, osd.color)},
    {"Fontsize", "%d", offsetof(struct udx_msg, osd.fontSize)},
    {"Text", "%s", offsetof(struct udx_msg, osd.text)},

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
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
    {"Width@Video", "%d", offsetof(struct udx_msg, encode_param.video.width)},
    {"Height@Video", "%d", offsetof(struct udx_msg, encode_param.video.height)},
    {"FrameRate@Video", "%d", offsetof(struct udx_msg, encode_param.video.frameRate)},
    {"Encoderate@Video", "%d", offsetof(struct udx_msg, encode_param.video.encodeRate)},
    {"Encodetype@Video", "%d", offsetof(struct udx_msg, encode_param.video.encodeType)},
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
    {"DevIDS", "%s", offsetof(struct udx_msg, mixerID)},
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
    {"DevIDS", "%s", offsetof(struct udx_msg, mixerID)},
    {"DevIP", "%s", offsetof(struct udx_msg, av_stop_play.devIP)},
    {"DevCH", "%d", offsetof(struct udx_msg, av_stop_play.devCH)},
    {"DevType", "%d", offsetof(struct udx_msg, av_stop_play.devType)},
    {"StreamType", "%d", offsetof(struct udx_msg, av_stop_play.streamType)},
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
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
    {"PosID[]@Src", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.srcPosID)},
    {"PosID@Dst", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.dstPosID)},
};

//<混音器释放> <交换->拼接器>
udx_keyword_ctx_t audio_mix_drop_keyword_ctx[] = {
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
};


udx_keyword_ctx_t audio_mix_drop_repsp_keyword_ctx[] = {
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
    {"PosID[]@Src", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.srcPosID)},
    {"PosID@Dst", "%d", offsetof(struct udx_msg, audio_mix_apply_resp.dstPosID)},
};

//<混音器设置> <交换->拼接器>
udx_keyword_ctx_t audio_mix_set_keyword[] = {
    {"MixerID", "%s", offsetof(struct udx_msg, mixerID)},
};

struct udx_cmd{
    char *udx_cmd_str;
    int index;
    int cmd;
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
};


static struct udx_ctx{
    char *cmd_str;
    udx_keyword_ctx_t *recv_keyword_ctx_tab;
    udx_keyword_ctx_t *send_keyword_ctx_tab;
    size_t tab_num;
    char resp;
}udx_ctx_info[] = {
    {"AVMixerRegister", return_status_keyword_ctx, NULL, TABLESIZE(return_status_keyword_ctx), FALSE},
    {"AVMixerUnregister", return_status_keyword_ctx, NULL, TABLESIZE(return_status_keyword_ctx), FALSE},
    {"AVMixerRegisterUpdate", return_status_keyword_ctx, NULL, TABLESIZE(return_status_keyword_ctx), FALSE},
    {"AVMixerGetInfo", get_info_keyword_ctx, NULL, TABLESIZE(get_info_keyword_ctx), FALSE},
    {"AVMixerStateChange", NULL, NULL, 0, FALSE },
    {"AVMixerSetSplitMode", split_screen_keyword_ctx, return_status_keyword_ctx,TABLESIZE(split_screen_keyword_ctx), TRUE},
    {"AVMixerSwitchVideo", switch_video_keyword_ctx, return_status_keyword_ctx,  TABLESIZE(switch_video_keyword_ctx), TRUE},
    {"AVMixerCtrlFullScreen", full_screen_keyword_ctx, return_status_keyword_ctx, TABLESIZE(full_screen_keyword_ctx), TRUE},
    {"AVMixerCtrlDevPlayState", chn_play_state_keyword_ctx, return_status_keyword_ctx, TABLESIZE(chn_play_state_keyword_ctx), TRUE},
    {"AVMixerCtrlPlayState", play_state_keyword_ctx, return_status_keyword_ctx, TABLESIZE(play_state_keyword_ctx), TRUE},
    {"AVMixerCtrlDevAudioState", chn_audio_state_keyword_ctx, return_status_keyword_ctx,TABLESIZE(chn_audio_state_keyword_ctx), TRUE},
    {"AVMixerCtrlAudioState", audio_state_keyword_ctx, return_status_keyword_ctx,TABLESIZE(audio_state_keyword_ctx), TRUE},
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

static int udx_ctx_item_get(char *command, struct udx_ctx **out_data)
{
    int index;  
    struct udx_cmd data = {command, }, *p;  
    p = bsearch((void *)&data, (void *)udx_cmd_map, TABLESIZE(udx_cmd_map), sizeof(struct udx_cmd), udx_cmd_item_cmp);  
    if(p)
    {
        *out_data = &udx_ctx_info[p->index];
        return p->index;
    }
    *out_data = NULL;
    return -1;
}

static int _udx_xml_packed(struct udx_ctx *udx_item)
{
    
    TiXmlDocument doc;  
    TiXmlElement * root ;
    if(udx_param.seq >= 0)
    {
        root = new TiXmlElement( UDXRESPNAME );  
        root->SetAttribute(UDXREQNAME, udx_item->cmd_str);
        root->SetAttribute(UDXSEQNAME, udx_param.seq);
        root->SetAttribute(UDXRESNAME, udx_param.result);
        root->SetAttribute(UDXREANAME, udx_param.reason);
        doc.LinkEndChild( root );  
    }
  
    doc.Print();
    return 0;

}

static char *strrtok(const char *str, const char *delim)
{
    static char buf[4096];
    static int pos = 0;
    if(str)
    {
        strcpy(buf, str);
        pos = strlen(str) + 1;
    }
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
    return NULL;
}

TiXmlElement *_udx_xml_element(char *path, TiXmlElement *root)
{
    char *token, *str;
    TiXmlElement *elem = root;
    for(str = path; token = strrtok(str, "@"); str = NULL)
    {
        char elem_name[64];
        sscanf(token, "%[^[]", elem_name);
        elem = elem->FirstChildElement(elem_name); 
    }
    return elem;
}

static int _udx_xml_depacked( struct udx_ctx *udx_item, TiXmlElement *root)
{
    int i;
    udx_keyword_ctx_t *key_tab;
    key_tab = udx_item->recv_keyword_ctx_tab;
    for(i = 0; i < udx_item->tab_num; i ++)
    {
        char keyword[64];
        TiXmlElement *elem;
        char *value = NULL;
        elem = _udx_xml_element(key_tab[i].keyword, root);
        sscanf(key_tab[i].keyword, "%[^[@]", keyword); 
        value = elem->GetText();
        if(value)
        {
            char *addr = (char *)&udx_param + key_tab[i].member_ofset;
            sscanf(value, key_tab[i].fmt, addr);
        }
    }
}

void udx_xml_depacked(char *xml_cmd)
{
    struct udx_ctx *udx_ctx_item;
    struct udx_msg msg_ctx;
    char cmd[64] = {0};
    TiXmlDocument doc;
    bool loadOkay = doc.LoadFile(UDX_TEMP_FILE);
    TiXmlElement *root = doc.RootElement();
    if(loadOkay)
    {
        doc.Print();
    }
    else
    {
        printf("xml error\n");
    }
    if(strcmp(UDXREQNAME, root->Value()) == 0)
    {
        strcpy(cmd, root->Attribute(UDXCMDNAME));
        udx_param.seq = atoi( root->Attribute(UDXSEQNAME));
        if(cmd[0] != '\0')
        {
            struct udx_ctx *uc;
            udx_ctx_item_get(cmd, &uc);
            _udx_xml_depacked(uc, root);
            //printf("%d\t%s\t%s%d\t%d\t%d\t%d\t%d", udx_param.av_play.index, udx_param.mixerID, udx_param.av_play.devIP, udx_param.av_play.devCH, udx_param.av_play.devType, udx_param.av_play.streamType, udx_param.av_play.audioState, udx_param.av_play.volume);
           // _udx_xml_packed(uc);
        }
    }
}

#ifdef DEBUG
void write_app_settings_doc( )  
{  
    TiXmlDocument doc;  
    TiXmlElement* msg;
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
    doc.LinkEndChild( decl );  
 
    TiXmlElement * root = new TiXmlElement( "MyApp" );  
    doc.LinkEndChild( root );  

    TiXmlComment * comment = new TiXmlComment();
    comment->SetValue(" Settings for MyApp " );  
    root->LinkEndChild( comment );  
 
    TiXmlElement * msgs = new TiXmlElement( "Messages" );  
    root->LinkEndChild( msgs );  
 
    msg = new TiXmlElement( "Welcome" );  
    msg->LinkEndChild( new TiXmlText( "Welcome to MyApp" ));  
    msgs->LinkEndChild( msg );  
 
    msg = new TiXmlElement( "Farewell" );  
    msg->LinkEndChild( new TiXmlText( "Thank you for using MyApp" ));  
    msgs->LinkEndChild( msg );  
 
    TiXmlElement * windows = new TiXmlElement( "Windows" );  
    root->LinkEndChild( windows );  

    TiXmlElement * window;
    window = new TiXmlElement( "Window" );  
    windows->LinkEndChild( window );  
    window->SetAttribute("name", "MainFrame");
    window->SetAttribute("x", 5);
    window->SetAttribute("y", 15);
    window->SetAttribute("w", 400);
    window->SetAttribute("h", 250);

    TiXmlElement * cxn = new TiXmlElement( "Connection" );  
    root->LinkEndChild( cxn );  
    cxn->SetAttribute("ip", "192.168.0.1");
    cxn->SetDoubleAttribute("timeout", 123.456); // floating point attrib
    
    doc.SaveFile( "appsettings.xml" );  
} 
int main(int argc, char *argv[])
{
/*
    char udx_xml[4096];
    FILE *fp;
    fp = fopen(argv[1], "rt");
    fread(udx_xml, 1, 4096, fp);
    fclose(fp);
    printf("%s\n", udx_xml);

    */
    char *str, *src ;
    for(src = "hello@world@china"; str = strrtok(src, "@"); src = NULL)
        printf("%s\n", str);
    for(src = "hello1#world1#china1"; str = strrtok(src, "#"); src = NULL)
        printf("%s\n", str);
 udx_anlysis_init();
    udx_xml_depacked(NULL);

    write_app_settings_doc();   
}
#endif
