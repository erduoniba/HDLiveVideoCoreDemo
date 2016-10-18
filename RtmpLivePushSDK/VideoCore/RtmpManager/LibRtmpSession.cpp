//
//  LibRtmpSession.cpp
//  AudioEditX
//
//  Created by Alex.Shi on 16/3/8.
//  Copyright © 2016年 com.Alex. All rights reserved.
//

#include "LibRtmpSession.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <memory.h>
#include "sps_decode.h"
#include "include/librtmp/rtmp.h"

#define DATA_ITEMS_MAX_COUNT 100
#define RTMP_DATA_RESERVE_SIZE 400

#define RTMP_CONNECTION_TIMEOUT 1500
#define RTMP_RECEIVE_TIMEOUT    2

typedef struct _DataItem
{
    char* data;
    int size;
    int headlen;
}DataItem;

typedef struct _RTMPMetadata
{
    // video, must be h264 type
    unsigned int    nWidth;
    unsigned int    nHeight;
    unsigned int    nFrameRate;
    unsigned int    nSpsLen;
    unsigned char   *Sps;
    unsigned int    nPpsLen;
    unsigned char   *Pps;
} RTMPMetadata,*LPRTMPMetadata;

LibRtmpSession::LibRtmpSession(){
    
}

LibRtmpSession::LibRtmpSession(char* szRtmpUrl):_pRtmp(NULL)
,_iConnectFlag(0)
,_iMetaDataFlag(0)
,_pAdtsItems(NULL)
,_uiStartTimestamp(0)
,_uiAudioDTS(0)
,_uiVideoLastAudioDTS(0)
,_uiAudioDTSNoChangeCnt(0)
{
    strcpy(_szRtmpUrl, szRtmpUrl);
    _pAdtsItems = (DataItem*)malloc(sizeof(DataItem)*DATA_ITEMS_MAX_COUNT);
    _pNaluItems = (DataItem*)malloc(sizeof(DataItem)*DATA_ITEMS_MAX_COUNT);
    _pMetaData = (RTMPMetadata*)malloc(sizeof(RTMPMetadata));
    //pthread_mutex_init(&_mConnstatMutex,NULL);
}

LibRtmpSession::~LibRtmpSession(){
    if(_iConnectFlag != 0) {
        DisConnect();
        if (_pRtmp) {
            free(_pRtmp);
        }
        if (_pAdtsItems) {
            free(_pAdtsItems);
        }
        if (_pNaluItems) {
            free(_pNaluItems);
        }
        if (_pMetaData) {
            free(_pMetaData);
        }
    }

    //pthread_mutex_destroy(&_mConnstatMutex);
}

int LibRtmpSession::Connect(){
    int iRet = 0;
    _iConnectFlag = 0;
    
    //if (0 == pthread_mutex_trylock(&_mConnstatMutex)) {
    if (_pRtmp) {
        free(_pRtmp);
        _pRtmp = NULL;
    }
    if(!_pRtmp)
    {
        _pRtmp = RTMP_Alloc();
        if(_pRtmp)
        {
            RTMP_Init(_pRtmp);
        }else
        {
            free(_pRtmp);
            _pRtmp = NULL;
            iRet = -1;
            //pthread_mutex_unlock(&_mConnstatMutex);
            return iRet;
        }
    }
    
    if (RTMP_SetupURL(_pRtmp, (char*)_szRtmpUrl) == FALSE)
    {
        iRet = -2;
        free(_pRtmp);
        _pRtmp = NULL;
        //pthread_mutex_unlock(&_mConnstatMutex);
        return iRet;
    }
    
    RTMP_EnableWrite(_pRtmp);
    _pRtmp->Link.timeout = RTMP_RECEIVE_TIMEOUT;
    if (RTMP_ConnectEx(_pRtmp, NULL, RTMP_CONNECTION_TIMEOUT) == FALSE)
    {
        RTMP_Close(_pRtmp);
        RTMP_Free(_pRtmp);
        _pRtmp = NULL;
        iRet = -3;
        //pthread_mutex_unlock(&_mConnstatMutex);
        return iRet;
    }
    
    if (RTMP_ConnectStream(_pRtmp,10) == FALSE)
    {
        RTMP_Close(_pRtmp);
        RTMP_Free(_pRtmp);
        _pRtmp = NULL;
        iRet = -4;
        //pthread_mutex_unlock(&_mConnstatMutex);
        return iRet;
    }
    _iConnectFlag = 1;
    _iMetaDataFlag = 0;
    
    //pthread_mutex_unlock(&_mConnstatMutex);
    //}
    return iRet;
}

void LibRtmpSession::DisConnect(){
    //pthread_mutex_lock(&_mConnstatMutex);
    if(_pRtmp)
    {
        RTMP_Close(_pRtmp);
        RTMP_Free(_pRtmp);
        
        _pRtmp = NULL;

        _iConnectFlag  = 0;
        _iMetaDataFlag = 0;
    }
    //pthread_mutex_unlock(&_mConnstatMutex);
}

int LibRtmpSession::IsConnected(){
    
    if(_pRtmp == NULL){
        return 0;
    }
    //pthread_mutex_lock(&_mConnstatMutex);
    int iRet = RTMP_IsConnected(_pRtmp);
    //pthread_mutex_unlock(&_mConnstatMutex);
    return iRet;
}

int LibRtmpSession::getSampleRateType(int iSampleRate){
    int iRetType = 4;
    
    switch (iSampleRate) {
        case 96000:
            iRetType = 0;
            break;
        case 88200:
            iRetType = 1;
            break;
        case 64000:
            iRetType = 2;
            break;
        case 48000:
            iRetType = 3;
            break;
        case 44100:
            iRetType = 4;
            break;
        case 32000:
            iRetType = 5;
            break;
        case 24000:
            iRetType = 6;
            break;
        case 22050:
            iRetType = 7;
            break;
        case 16000:
            iRetType = 8;
            break;
        case 12000:
            iRetType = 9;
            break;
        case 11025:
            iRetType = 10;
            break;
        case 8000:
            iRetType = 11;
            break;
        case 7350:
            iRetType = 12;
            break;
    }
    return iRetType;
}
void LibRtmpSession::MakeAudioSpecificConfig(char* pConfig, int aactype, int sampleRate, int channels){
    unsigned short result = 0;
    result += aactype;
    result = result << 4;
    result += sampleRate;
    result = result << 4;
    result += channels;
    result = result <<3;
    int size = sizeof(result);
    
    if ((aactype == 5) || (aactype == 29)) {
        result |= 0x01;
    }
    memcpy(pConfig,&result,size);
    
    unsigned char low,high;
    low = pConfig[0];
    high = pConfig[1];
    pConfig[0] = high;
    pConfig[1] = low;
//    if (pData == NULL) {
//        return;
//    }
//    
//    unsigned short usResult = 0;
//    
//    usResult |= aactype<<11;
//    usResult |= sampleRate<<7;
//    usResult |= channels<<3;
//
//    pData[0] = (char)(usResult>>8);
//    pData[1] = usResult&0xff;
//    
//    if ((aactype == 5) || (aactype == 29)) {
//        pData[1] |= 0x01;
//    }
//    return;
}

int LibRtmpSession::SendAudioSpecificConfig(int aactype, int sampleRate, int channels)
{
    char* szAudioSpecData;
    int iSpeclen = 0;
    
    if ((aactype == 5) || (aactype == 29)) {
        iSpeclen = 4;
    }else{
        iSpeclen = 2;
    }
    szAudioSpecData = (char*)malloc(iSpeclen);
    memset(szAudioSpecData, 0, iSpeclen);
    MakeAudioSpecificConfig(szAudioSpecData, aactype, getSampleRateType(sampleRate), channels);
    
    unsigned char* body;
    int len;
    len = iSpeclen+2;
    
    int rtmpLength = len;
    RTMPPacket rtmp_pack;
    RTMPPacket_Reset(&rtmp_pack);
    RTMPPacket_Alloc(&rtmp_pack,rtmpLength);
    
    body = (unsigned char *)rtmp_pack.m_body;
    body[0] = 0xAF;
    body[1] = 0x00;
    
    memcpy(&body[2],szAudioSpecData,iSpeclen);
    free(szAudioSpecData);
    
    rtmp_pack.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    rtmp_pack.m_nBodySize = len;
    rtmp_pack.m_nChannel = 0x04;
    rtmp_pack.m_nTimeStamp = 0;
    rtmp_pack.m_hasAbsTimestamp = 0;
    rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    
    if(_pRtmp)
        rtmp_pack.m_nInfoField2 = _pRtmp->m_stream_id;
    
    int iRet = RtmpPacketSend(&rtmp_pack);
    
    return iRet;
}

int LibRtmpSession::RtmpPacketSend(RTMPPacket* packet)
{
    int iRet = 0;
    
    //if (0 == pthread_mutex_trylock(&_mConnstatMutex)) {
        iRet = RTMP_SendPacket(_pRtmp,packet,0);
    //    pthread_mutex_unlock(&_mConnstatMutex);
    //}

    return iRet;
}

int LibRtmpSession::SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)
{
    int rtmpLength = size;
    RTMPPacket rtmp_pack;
    RTMPPacket_Reset(&rtmp_pack);
    RTMPPacket_Alloc(&rtmp_pack,rtmpLength);

    rtmp_pack.m_nBodySize = size;
    memcpy(rtmp_pack.m_body,data,size);
    rtmp_pack.m_hasAbsTimestamp = 0;
    rtmp_pack.m_packetType = nPacketType;
    
    if(_pRtmp)
        rtmp_pack.m_nInfoField2 = _pRtmp->m_stream_id;
    
    rtmp_pack.m_nChannel = 0x04;
    
    rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size !=4)
    {
        rtmp_pack.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    }
    rtmp_pack.m_nTimeStamp = nTimestamp;

    int nRet = RtmpPacketSend(&rtmp_pack);

    RTMPPacket_Free(&rtmp_pack);
    return nRet;
}

int LibRtmpSession::SendVideoSpsPps(unsigned char *pps,int pps_len,unsigned char * sps,int sps_len)
{
    unsigned char * body=NULL;
    int iIndex = 0;
    
    int rtmpLength = 1025;
    RTMPPacket rtmp_pack;
    RTMPPacket_Reset(&rtmp_pack);
    RTMPPacket_Alloc(&rtmp_pack,rtmpLength);
    
    body = (unsigned char *)rtmp_pack.m_body;

    body[iIndex++] = 0x17;
    body[iIndex++] = 0x00;
    
    body[iIndex++] = 0x00;
    body[iIndex++] = 0x00;
    body[iIndex++] = 0x00;
    
    body[iIndex++] = 0x01;
    body[iIndex++] = sps[1];
    body[iIndex++] = sps[2];
    body[iIndex++] = sps[3];
    body[iIndex++] = 0xff;
    
    /*sps*/
    body[iIndex++]   = 0xe1;
    body[iIndex++] = (sps_len >> 8) & 0xff;
    body[iIndex++] = sps_len & 0xff;
    memcpy(&body[iIndex],sps,sps_len);
    iIndex +=  sps_len;
    
    /*pps*/
    body[iIndex++]   = 0x01;
    body[iIndex++] = (pps_len >> 8) & 0xff;
    body[iIndex++] = (pps_len) & 0xff;
    memcpy(&body[iIndex], pps, pps_len);
    iIndex +=  pps_len;
    
    rtmp_pack.m_packetType = RTMP_PACKET_TYPE_VIDEO;
    rtmp_pack.m_nBodySize = iIndex;
    rtmp_pack.m_nChannel = 0x04;
    rtmp_pack.m_nTimeStamp = 0;
    rtmp_pack.m_hasAbsTimestamp = 0;
    rtmp_pack.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    
    if(_pRtmp)
        rtmp_pack.m_nInfoField2 = _pRtmp->m_stream_id;
    
    int iRet = RtmpPacketSend(&rtmp_pack);
    
    //free(packet);
    
    return iRet;
}

int LibRtmpSession::SendH264Packet(unsigned char *data,unsigned int size,int bIsKeyFrame,unsigned int nTimeStamp)
{
    if(data == NULL && size<11)
    {
        return FALSE;
    }
    
    unsigned char *body = (unsigned char*)malloc(size+9);
    memset(body,0,size+9);
    
    int i = 0;
    if(bIsKeyFrame)
    {
        body[i++] = 0x17;// 1:Iframe  7:AVC
        body[i++] = 0x01;// AVC NALU
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;

        memcpy(&body[i],data,size);
    }
    else
    {
        body[i++] = 0x27;// 2:Pframe  7:AVC
        body[i++] = 0x01;// AVC NALU
        body[i++] = 0x00;
        body[i++] = 0x00;
        body[i++] = 0x00;
        memcpy(&body[i],data,size);
    }
    
    int bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);
    
    free(body);
    
    return bRet;
}

int LibRtmpSession::SendAACData(unsigned char* buf, int size, unsigned int timeStamp)
{
    if(_pRtmp == NULL)
        return -1;
    
    if (size <= 0)
    {
        return -2;
    }
    
    unsigned char * body;
    
    int rtmpLength = size+2;
    RTMPPacket rtmp_pack;
    RTMPPacket_Reset(&rtmp_pack);
    RTMPPacket_Alloc(&rtmp_pack,rtmpLength);

    body = (unsigned char *)rtmp_pack.m_body;
    
    /*AF 01 + AAC RAW data*/
    body[0] = 0xAF;
    body[1] = 0x01;
    memcpy(&body[2],buf,size);
    
    rtmp_pack.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    rtmp_pack.m_nBodySize = size+2;
    rtmp_pack.m_nChannel = 0x04;
    rtmp_pack.m_nTimeStamp = timeStamp;
    rtmp_pack.m_hasAbsTimestamp = 0;
    rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    
    if(_pRtmp)
        rtmp_pack.m_nInfoField2 = _pRtmp->m_stream_id;
    
    return RtmpPacketSend(&rtmp_pack);
}

int LibRtmpSession::SendAudioRawData(unsigned char* pBuff, int len, unsigned int ts){
    int rtmpLength = len;
    RTMPPacket* pRtmp_pack = (RTMPPacket*)malloc(sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE+ rtmpLength+RTMP_DATA_RESERVE_SIZE);
    memset(pRtmp_pack, 0, sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE+ rtmpLength+RTMP_DATA_RESERVE_SIZE);
    
    pRtmp_pack->m_body = ((char*)pRtmp_pack) + sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + RTMP_DATA_RESERVE_SIZE/2;
    
    /*AAC RAW data*/
    memcpy(pRtmp_pack->m_body,pBuff,rtmpLength);
    
    pRtmp_pack->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    pRtmp_pack->m_nBodySize = rtmpLength;
    pRtmp_pack->m_nChannel = 0x04;
    pRtmp_pack->m_nTimeStamp = ts;
    pRtmp_pack->m_hasAbsTimestamp = 0;
    pRtmp_pack->m_headerType = RTMP_PACKET_SIZE_LARGE;
    
    if(_pRtmp)
        pRtmp_pack->m_nInfoField2 = _pRtmp->m_stream_id;
    
    int iRet = RtmpPacketSend(pRtmp_pack);
    
    free(pRtmp_pack);
    return iRet;
}

int LibRtmpSession::SendAudioData(unsigned char* pBuff, int len){
    int cnt = 0;
    int i;
    DataItem* pAdtsItems = (DataItem*)_pAdtsItems;
    
    for(i=0; i<len; i++)
    {
        unsigned char Data1 = pBuff[i];
        unsigned char Data2 = pBuff[i+1];
        if((Data1==0xFF) && (Data2==0xF1))
        {
            pAdtsItems[cnt].data = (char*)(pBuff+i);
            pAdtsItems[cnt].headlen = 7;
            i++;
            cnt++;
            if(cnt >= DATA_ITEMS_MAX_COUNT)
            {
                break;
            }
        }
    }
    
    for(i=0; i<cnt;i++)
    {
        if(i < cnt-1)
        {
            pAdtsItems[i].size = pAdtsItems[i+1].data - pAdtsItems[i].data;
            
        }
        else
        {
            pAdtsItems[i].size =(char*)(pBuff+len) - pAdtsItems[i].data;
        }
    }

    for(i=0; i<cnt; i++)
    {
        if(pAdtsItems[i].size > 0)
        {
            if (_uiStartTimestamp == 0) {
                _uiStartTimestamp = RTMP_GetTime();
            }else{
                _uiAudioDTS = RTMP_GetTime()-_uiStartTimestamp;
            }
            SendAACData((unsigned char*)(pAdtsItems[i].data+7),pAdtsItems[i].size-7, _uiAudioDTS);
        }
    }
    return cnt;
}

int LibRtmpSession::SeparateNalus(unsigned char* pBuff, int len)
{
    int cnt = 0;
    int i = 0;

    for(i=0; i< len; i++)
    {
        if(pBuff[i] == 0)
        {
            //00 00 00 01
            if((pBuff[i+1]==0) && (pBuff[i+2] == 0) && (pBuff[i+3] == 1))
            {
                _pNaluItems[cnt].data = (char*)(pBuff+i);
                _pNaluItems[cnt].headlen = 4;
                i += 3;
                cnt++;
            }
            //00 00 01
            else if ((pBuff[i+1]==0) && (pBuff[i+2] == 1))
            {
                _pNaluItems[cnt].data = (char*)(pBuff+i);
                _pNaluItems[cnt].headlen = 3;
                i += 2;
                cnt++;
            }
            if(cnt >= DATA_ITEMS_MAX_COUNT)
            {
                break;
            }
        }
    }

    for(i=0; i<cnt;i++)
    {
        if(i < cnt-1)
        {
            _pNaluItems[i].size = _pNaluItems[i+1].data - _pNaluItems[i].data;
        }
        else
        {
            _pNaluItems[i].size =(char*)(pBuff+len) - _pNaluItems[i].data;
        }
        
        _pNaluItems[i].data[3] = (_pNaluItems[i].size-4) & 0xFF;
        _pNaluItems[i].data[2] = ((_pNaluItems[i].size-4) >> 8) & 0xFF;
        _pNaluItems[i].data[1] = ((_pNaluItems[i].size-4) >> 16) & 0xFF;
        _pNaluItems[i].data[0] = ((_pNaluItems[i].size-4) >> 24) & 0xFF;
    }
    return cnt;
}

int LibRtmpSession::SendVideoRawData(unsigned char* buf, int videodatalen, unsigned int ts){
    int rtmpLength = videodatalen;
    
    RTMPPacket* pRtmp_pack = (RTMPPacket*)malloc(sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE+ rtmpLength+RTMP_DATA_RESERVE_SIZE);
    memset(pRtmp_pack, 0, sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE+ rtmpLength+RTMP_DATA_RESERVE_SIZE);
    
    pRtmp_pack->m_nBodySize = videodatalen;
    pRtmp_pack->m_hasAbsTimestamp = 0;
    pRtmp_pack->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    
    if(_pRtmp)
        pRtmp_pack->m_nInfoField2 = _pRtmp->m_stream_id;
    
    pRtmp_pack->m_nChannel = 0x04;
    
    pRtmp_pack->m_headerType = RTMP_PACKET_SIZE_LARGE;
    pRtmp_pack->m_nTimeStamp = ts;
    pRtmp_pack->m_body = ((char*)pRtmp_pack) + sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE+RTMP_DATA_RESERVE_SIZE/2;
    memcpy(pRtmp_pack->m_body,buf,videodatalen);
    
    int iRet = RtmpPacketSend(pRtmp_pack);
    
    free(pRtmp_pack);

    return iRet;
}

int LibRtmpSession::SendVideoData(unsigned char* buf, int videodatalen){
    int itemscnt = SeparateNalus(buf,videodatalen);
    int iRet = 0;
    
    if((!_pMetaData->Sps) || (!_pMetaData->Pps))
    {
        if(itemscnt > 0)
        {
            int i=0;
            for(i=0; i<itemscnt;i++)
            {
                if(((_pNaluItems[i].data[4]&0x1f) == 7) && (!_pMetaData->Sps))
                {
                    _pMetaData->nSpsLen = _pNaluItems[i].size - _pNaluItems[i].headlen;
                    _pMetaData->Sps = (unsigned char*)malloc(_pMetaData->nSpsLen);
                    memcpy(_pMetaData->Sps, _pNaluItems[i].data + _pNaluItems[i].headlen, _pNaluItems[i].size - _pNaluItems[i].headlen);

                    int width = 0,height = 0, fps=0;
                    int* Width = &width;
                    int* Height = &height;
                    int* Fps = &fps;
                    
                    h264_decode_sps(_pMetaData->Sps,_pMetaData->nSpsLen,&Width,&Height,&Fps);
                    _pMetaData->nWidth = width;
                    _pMetaData->nHeight = height;
                    //printf("GetSPS Width:%d, Height:%d spslen:%d\n",width, height,metaData.nSpsLen);
                    if(fps)
                        _pMetaData->nFrameRate = fps;
                    else
                        _pMetaData->nFrameRate = 20;
                }
                if(((_pNaluItems[i].data[4]&0x1f) == 8) && (!_pMetaData->Pps))
                {
                    _pMetaData->nPpsLen = _pNaluItems[i].size - _pNaluItems[i].headlen;
                    _pMetaData->Pps = (unsigned char*)malloc(_pMetaData->nPpsLen);
                    memcpy(_pMetaData->Pps, _pNaluItems[i].data+_pNaluItems[i].headlen, _pNaluItems[i].size - _pNaluItems[i].headlen);
                }
            }
        }
        if((_pMetaData->Sps) && (_pMetaData->Pps))
        {
            SendVideoSpsPps(_pMetaData->Pps,_pMetaData->nPpsLen,_pMetaData->Sps,_pMetaData->nSpsLen);
            _iMetaDataFlag = 1;
        }
    }
    if(!_iMetaDataFlag)
    {
        return -1;
    }
    
    if(itemscnt > 0)
    {
        int i=0;
        int isKey = 0;
        for(i=0; i<itemscnt; i++)
        {
            if(_pNaluItems[i].size == 0)
            {
                continue;
            }
            
            isKey  = ((_pNaluItems[i].data[4]&0x1f) == 0x05) ? TRUE : FALSE;
            if(isKey)
            {
                break;
            }
        }

        if(_uiVideoLastAudioDTS == 0)
        {
            _uiVideoLastAudioDTS = _uiAudioDTS;
        }
        
        if(_uiVideoLastAudioDTS == _uiAudioDTS)
            _uiAudioDTSNoChangeCnt++;
        _uiVideoLastAudioDTS = _uiAudioDTS;
        
        if(_uiAudioDTSNoChangeCnt > 50)
        {
            iRet = SendH264Packet(buf, videodatalen, isKey, RTMP_GetTime());
        }
        else
        {
            iRet =SendH264Packet(buf, videodatalen, isKey, _uiAudioDTS);
        }
        
    }

    return 0;
}
