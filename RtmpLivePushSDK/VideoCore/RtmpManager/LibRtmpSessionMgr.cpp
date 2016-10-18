//
//  LibRtmpSession.cpp
//  Pods
//
//  Created by Alex.Shi on 16/3/9.
//
//

#include "LibRtmpSessionMgr.hpp"
#include "LibRtmpSession.hpp"

static unsigned int RTMP_SEND_QUEUE_MAX = 100;

#define RTMP_SESSION_SLEEP_TIME (10*1000)

namespace videocore
{
    RTMP_Queue_Manager::RTMP_Queue_Manager(){
        _iMaxQueueLength = 100;
        pthread_mutex_init(&_mConnstatMutex,NULL);
    }
    
    RTMP_Queue_Manager::RTMP_Queue_Manager(int iMaxQueueLength){
        _iMaxQueueLength = iMaxQueueLength;
        pthread_mutex_init(&_mConnstatMutex,NULL);
    }
    RTMP_Queue_Manager::~RTMP_Queue_Manager(){
        CleanQueue();
        pthread_mutex_destroy(&_mConnstatMutex);
    }
    
    int RTMP_Queue_Manager::InsertQueue(unsigned int uiType,
                                        unsigned int uiLength,
                                        unsigned int uiTimestamp,
                                        unsigned char* pData){
        int iRet = 0;
        if (_sendDataQueue.size() >= _iMaxQueueLength) {
            CleanQueue();
            iRet = 1;
        }
        pthread_mutex_lock(&_mConnstatMutex);
        RTMP_QUEUE_ITEM* pNewItem = (RTMP_QUEUE_ITEM*)malloc(sizeof(RTMP_QUEUE_ITEM));
        pNewItem->uiType = uiType;
        pNewItem->ulLength = uiLength;
        pNewItem->uiTimestamp = uiTimestamp;
        pNewItem->pRtmpBody = (unsigned char*)malloc(uiLength);
        if (pNewItem->pRtmpBody != NULL) {
            memcpy(pNewItem->pRtmpBody, pData, uiLength);
        }
        _sendDataQueue.push(pNewItem);
        pthread_mutex_unlock(&_mConnstatMutex);
        return iRet;
    }
    
    RTMP_QUEUE_ITEM* RTMP_Queue_Manager::ReadQueueAndRelease(){
        RTMP_QUEUE_ITEM* pRet = NULL;
        if (_sendDataQueue.empty()) {
            return NULL;
        }
        pthread_mutex_lock(&_mConnstatMutex);
        pRet = _sendDataQueue.front();
        _sendDataQueue.pop();
        pthread_mutex_unlock(&_mConnstatMutex);
        
        return pRet;
    }
    
    unsigned int RTMP_Queue_Manager::GetQueueLength(){
        unsigned int uiLen = _sendDataQueue.size();
        
        return uiLen;
    }
    
    void RTMP_Queue_Manager::CleanQueue(){
        RTMP_QUEUE_ITEM* pItem = NULL;
        if (_sendDataQueue.empty()) {
            return;
        }
        pthread_mutex_lock(&_mConnstatMutex);
        pItem = _sendDataQueue.front();
        while (pItem != NULL) {
            if(pItem->pRtmpBody != NULL){
                free(pItem->pRtmpBody);
            }
            free(pItem);
            _sendDataQueue.pop();
            if (_sendDataQueue.empty()) {
                break;
            }
            pItem = _sendDataQueue.front();
        }
        pthread_mutex_unlock(&_mConnstatMutex);
    }
    
    LibRtmpSessionMgr::LibRtmpSessionMgr(std::string uri, LibRTMPSessionStateCallback callback):m_callback(callback)
    ,_rtmpSession(NULL)
    ,m_jobQueue("com.videocore.librtmp")
    ,m_sendQueue("com.videocore.librtmp.send")
    ,_rtmpSendQueueManager(RTMP_SEND_QUEUE_MAX)
    ,_iEndFlag(0)
    {
        memset(_szRtmpUrl, 0, sizeof(_szRtmpUrl));
        strcpy(_szRtmpUrl, uri.c_str());
        
        _rtmpSession = new LibRtmpSession(_szRtmpUrl);
    }
    
    LibRtmpSessionMgr::~LibRtmpSessionMgr(){
        _iEndFlag = 1;
        m_sendQueue.mark_exiting();
        m_sendQueue.enqueue_sync([]() {});
        
        m_jobQueue.mark_exiting();
        m_jobQueue.enqueue_sync([]() {});
        
        if(_rtmpSession->GetConnectedFlag() != 0){
            if (0 != _rtmpSession->IsConnected()) {
                _rtmpSession->DisConnect();
            }
            if (_rtmpSession) {
                delete _rtmpSession;
            }
        }
    }
    
    int LibRtmpSessionMgr::getConnectFlag(){
        return _rtmpSession->GetConnectedFlag();
    }
    
    void LibRtmpSessionMgr::pushBuffer(const uint8_t* const data, size_t size, IMetadata& metadata){
        if(_iEndFlag){
            return;
        }
        const LibRTMPMetadata_t inMetadata = static_cast<const LibRTMPMetadata_t&>(metadata);
        
        uint64_t ts = inMetadata.getData<kLibRTMPMetadataTimestamp>() ;
//        const int streamId = inMetadata.getData<kLibRTMPMetadataMsgStreamId>();
//        unsigned int uiDataLength = inMetadata.getData<kLibRTMPMetadataMsgLength>();
        unsigned int uiMsgTypeId  = inMetadata.getData<kLibRTMPMetadataMsgTypeId>();
        
        if (0 == _rtmpSession->GetConnectedFlag()){
            return;
        }
        std::shared_ptr<Buffer> buf = std::make_shared<Buffer>(size);
        buf->put(const_cast<uint8_t*>(data), size);
        
        m_jobQueue.enqueue([=]() {
            if(_iEndFlag){
                return;
            }
            if((RTMP_PT_AUDIO != uiMsgTypeId) && (RTMP_PT_VIDEO !=  uiMsgTypeId)){
                return;
            }
            unsigned char* pSendData = NULL;
            buf->read(&pSendData, size);
            int iRet = _rtmpSendQueueManager.InsertQueue(uiMsgTypeId,
                                                         size,
                                                         (unsigned int)ts,
                                                         pSendData);
            if (iRet != 0) {
                printf("##RTMP send queue length[%d] is over %d\r\n",
                       _rtmpSendQueueManager.GetQueueLength(), RTMP_SEND_QUEUE_MAX);
            }
        });
    }
    
    void LibRtmpSessionMgr::setSessionParameters(IMetadata& parameters){
        LibRTMPSessionParameters_t& parms = dynamic_cast<LibRTMPSessionParameters_t&>(parameters);
        _bitRate = parms.getData<kLibRTMPSessionParameterVideoBitrate>();
        _frameDuration = parms.getData<kLibRTMPSessionParameterFrameDuration>();
        _frameHeight = parms.getData<kLibRTMPSessionParameterHeight>();
        _frameWidth = parms.getData<kLibRTMPSessionParameterWidth>();
        _audioSampleRate = parms.getData<kLibRTMPSessionParameterAudioFrequency>();
        _audioStereo = parms.getData<kLibRTMPSessionParameterStereo>() ? 2 : 1;
        
        m_sendQueue.enqueue([=]() {
            ClientState_t iFlag = kClientStateNone;
            while(!_iEndFlag){
                if ((0 == _rtmpSession->IsConnected()) || (0 == _rtmpSession->GetConnectedFlag())){
                    _rtmpSession->Connect();
                    if (0 != _rtmpSession->IsConnected()) {
                        if(_iEndFlag){
                            break;
                        }
                        if(iFlag != kClientStateSessionStarted){
                            m_callback(*this, kClientStateSessionStarted);
                            iFlag = kClientStateSessionStarted;
                        }
                        usleep(RTMP_SESSION_SLEEP_TIME);
                        continue;
                    }else{
                        if (kClientStateSessionStarted != iFlag) {
                            if(_iEndFlag){
                                break;
                            }
                            if(iFlag != kClientStateHandshake0){
                                m_callback(*this, kClientStateHandshake0);
                                iFlag = kClientStateHandshake0;
                            }
                        }
                        usleep(RTMP_SESSION_SLEEP_TIME);
                        continue;
                    }
                }
                if ((0 == _rtmpSession->IsConnected()) && (0 != _rtmpSession->GetConnectedFlag())){
                    _rtmpSession->SetConnectedFlag(FALSE);

                    if (0 == _rtmpSession->IsConnected()) {
                        _rtmpSession->Connect();
                    }
                    if (0 != _rtmpSession->IsConnected()) {
                        if(_iEndFlag){
                            break;
                        }
                        if(iFlag != kClientStateSessionStarted){
                            m_callback(*this, kClientStateSessionStarted);
                            iFlag = kClientStateSessionStarted;
                        }
                        usleep(RTMP_SESSION_SLEEP_TIME);
                        continue;
                    }else{
                        if (kClientStateSessionStarted != iFlag) {
                            if(_iEndFlag){
                                break;
                            }
                            if(iFlag != kClientStateHandshake0){
                                m_callback(*this, kClientStateHandshake0);
                                iFlag = kClientStateHandshake0;
                            }
                        }
                        usleep(RTMP_SESSION_SLEEP_TIME);
                        continue;
                    }
                }
                if(0 == _rtmpSession->GetConnectedFlag()){
                    usleep(RTMP_SESSION_SLEEP_TIME);
                    continue;
                }
                RTMP_QUEUE_ITEM* pItem = _rtmpSendQueueManager.ReadQueueAndRelease();
                if (pItem == NULL) {
                    usleep(RTMP_SESSION_SLEEP_TIME);
                    continue;
                }
                unsigned int uiMsgTypeId = pItem->uiType;
                int size                 = (int)pItem->ulLength;
                unsigned int ts          = pItem->uiTimestamp;
                unsigned char* pSendData = pItem->pRtmpBody;
                
                if(RTMP_PT_AUDIO == uiMsgTypeId){
                    if (0 != _rtmpSession->GetConnectedFlag()) {
                        //printf("SendAudioRawData...\r\n");
                        _rtmpSession->SendAudioRawData(pSendData, (int)size, (unsigned int)ts);
                        //printf("SendAudioRawData return %d\r\n", iRet);
                    }
                }else if (RTMP_PT_VIDEO ==  uiMsgTypeId){
                    if (0 != _rtmpSession->GetConnectedFlag()) {
                        //printf("SendVideoRawData...\r\n");
                        _rtmpSession->SendVideoRawData(pSendData, (int)size, (unsigned int)ts);
                        //printf("SendVideoRawData return %d\r\n", iRet);
                    }
                }
                if (pSendData) {
                    free(pSendData);
                }
                if (pItem){
                    free(pItem);
                }
                usleep(RTMP_SESSION_SLEEP_TIME);
            }
        });
    }
    
    void LibRtmpSessionMgr::setEndFlag(int iFlag){
        _iEndFlag = iFlag;
        m_sendQueue.mark_exiting();
        m_sendQueue.enqueue_sync([]() {});
    }
    
    void LibRtmpSessionMgr::setBandwidthCallback(BandwidthCallback callback){
        
    }
}