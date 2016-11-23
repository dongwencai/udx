/*
 * =====================================================================================
 *
 *       Filename:  udx_analysis.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年11月18日 12时34分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __UDX_ANALYSIS_H__
#define __UDX_ANALYSIS_H__

#define DEBUG 1

typedef struct {
    char *path;
    char *fmt;
    unsigned int member_ofset;
}udx_keyword_ctx_t;

#define UDX_DATA_SET_END(addr) (*(int *)addr = 0x646E65)
#define UDX_DATA_IS_END(addr) (*(int *)addr == 0x646E65)  //0x646E65字符串end

struct udx_msg{
    int seq;
    char cmd[64];
    char result[16];
    char reason[64];
    union{
        struct{
            char expires[8];
            int  retryTimes;
            struct{
                char mixerID[64];
               int maxSplitNum;
               int splitMode;
                char videoResolution[16];
                int videoFrameRate;
                char videoEncodeType[8];
                char audioEncodeType[8];
            }mixer_info;
        }mixer_register;

        struct{
            struct{
                char mixerID[64];
                int mixerType;
                int splitNum;
                int splitMode;
                char videoResolution[16];
                int videoFrameRate;
                int encodeRate;
                int encodeType;
                int allOsdState;
                struct{
                    int dbType;
                    int volume;
                    int playState;
                    struct{
                        int index; 
                        char mixerID[64];
                        int devCH;
                        int streamType;
                        int audioState;
                        int volume;
                        int isFullScreen;
                        int playState;
                        int osdState;
                        float leftRate;
                        float topRate;
                        float widthRate;
                        float heightRate;
                    }pos[17];
                }dbInfo[3];
            }layout;
        }mixer_get_info;

        struct{
            struct{
                int dbType;
                int splitNum;
                int splitMode;
                char mixerID[64];
                int mixerType;
                int volume;
                char videoResolution[16];
                int videoFrameRate;
                int encodeRate;
                int encodeType;
                int dbState;
                int allOsdState;
                struct{
                    int index; 
                    char mixerID[64];
                    int devCH;
                    int streamType;
                    int audioState;
                    int volume;
                    int isFullScreen;
                    int osdState;
                    float leftRate;
                    float topRate;
                    float widthRate;
                    float heightRate;
                }pos[17];
            }layout; 
        }mixer_data;

        struct{
            char mixerID[64];
        }mixer_unregister, mixer_register_update;

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
            char mixerID[64];
            int devCH;
            int show;
            struct{
                struct{
                    float left;
                    float top;
                    int color;
                    char fontName[16];
                    float fontSize;
                    char text[256];
                }osd_info[8];            
            }osd_info_list;
        }osd;

        struct{
            char mixerID[64];
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
            char mixerID[64];
            int index;
            char devIP[32];
            int devCH;
            int devType;
            int streamType;
            int audioState;
            int volume;
        
        }av_play, av_stop_play;

        struct{
            char mixerID[64];
            int xPos;
            int yPos;
            int nFocus;
            char caseID[64];
            int index;
        }mixer_pos_info;

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
#ifdef DEBUG        
        struct{
            char mixerID[64];
            struct{
                int posID[10]; 
            }src[10];
            struct{
                int posID; 
            }dst;
        }test;
#endif
    };
    int end;
};

typedef int(*udx_cmd_proc_func)(struct udx_msg *);

#endif
