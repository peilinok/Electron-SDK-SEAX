#pragma once
#include "IAgoraMediaEngine.h"
#include "IAgoraRtcEngine.h"
#include <string>
#include <map>
#include <node.h>
#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <dlfcn.h>
#endif
class IAVFramePlugin;

#define MAX_PLUGIN_ID   512

typedef struct tag_agora_plugin_info {
    char id[MAX_PLUGIN_ID];
    void * pluginModule;
    IAVFramePlugin* instance;
    bool enabled;
}agora_plugin_info;

class IAVFramePluginManager :
    // public agora::media::IVideoFrameObserver,
    public agora::media::IAudioFrameObserver
{
public:
    IAVFramePluginManager();
    ~IAVFramePluginManager();

    virtual bool onCaptureVideoFrame(agora::media::base::VideoFrame& videoFrame);
    bool onRenderVideoFrame(agora::rtc::uid_t uid, agora::rtc::conn_id_t connectionId, agora::media::base::VideoFrame& videoFrame);
    virtual bool onScreenCaptureVideoFrame(agora::media::base::VideoFrame& videoFrame);
    virtual bool onTranscodedVideoFrame(agora::media::IVideoFrameObserver::VideoFrame& videoFrame);

    virtual bool onRecordAudioFrame(agora::media::IAudioFrameObserver::AudioFrame& audioFrame);
    virtual bool onPlaybackAudioFrame(agora::media::IAudioFrameObserver::AudioFrame& audioFrame);
    virtual bool onMixedAudioFrame(agora::media::IAudioFrameObserver::AudioFrame& audioFrame);
    virtual bool onPlaybackAudioFrameBeforeMixing(unsigned int uid, agora::media::IAudioFrameObserver::AudioFrame& audioFrame);
    
    void registerPlugin(agora_plugin_info& plugin);
    void unregisterPlugin(std::string& pluginId);
    bool hasPlugin(std::string& pluginId);
    bool enablePlugin(std::string& pluginId, bool enabled);
    bool getPlugin(std::string& pluginId, agora_plugin_info& pluginInfo);
    std::vector<std::string> getPlugins();
    int release();
private:
    std::map<std::string, agora_plugin_info> m_mapPlugins;
};
