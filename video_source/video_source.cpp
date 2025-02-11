/*
 * Copyright (c) 2018 Agora.io
 * All rights reserved.
 * Proprietary and Confidential -- Agora.io
 */

/*
 *  Created by Wang Yongli, 2018
 */

#include "video_source.h"
#include <sstream>
#include "IAgoraRtcEngine.h"
#include "loguru.hpp"
#include "node_log.h"
#include "video_source_event_handler.h"
#include "video_source_ipc.h"
#include "video_source_param_parser.h"
#include "video_source_render.h"
#define PROCESS_RUN_EVENT_NAME "agora_video_source_process_ready_event_name"
#define DATA_IPC_NAME "avsipc"

using agora::rtc::AREA_CODE;
using agora::rtc::RtcEngineContext;
using agora::rtc::uid_t;

AgoraVideoSource::AgoraVideoSource(const std::string& param)
    : m_initialized(false),
      m_params(param),
      m_videoProfile(agora::rtc::VIDEO_PROFILE_DEFAULT) {
  LOG_ENTER;
  LOG_LEAVE;
}

AgoraVideoSource::~AgoraVideoSource() {
  LOG_ENTER;
  m_rtcEngine.reset();
  m_eventHandler.reset();
  m_renderFactory.reset();
  m_ipc.reset();
  m_paramParser.reset();
  LOG_LEAVE;
}

std::string AgoraVideoSource::getId() {
  return m_paramParser->getParameter("id");
}

bool AgoraVideoSource::initialize() {
  LOG_ENTER;
  LOG_INFO("videoSource initialize m_params: %s", m_params.c_str());
  m_paramParser.reset(new VideoSourceParamParser());
  m_paramParser->initialize(m_params);

  std::string appid = m_paramParser->getParameter("appid");
  if (appid.empty()) {
    LOG_ERROR("%s, appid is null\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  std::string id = m_paramParser->getParameter("id");
  if (id.empty()) {
    LOG_ERROR("%s, id is null\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  std::string areaCode = m_paramParser->getParameter("areaCode");

  m_ipc.reset(createAgoraIpc(this));
  if (!m_ipc->initialize(id)) {
    LOG_ERROR("%s, ipc init fail\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }
  if (!m_ipc->connect()) {
    LOG_ERROR("%s, ipc connect fail.\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  m_rtcEngine.reset(createAgoraRtcEngine());
  if (!m_rtcEngine.get()) {
    LOG_ERROR("%s, rtcengine create fail.\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  m_eventHandler.reset(new AgoraVideoSourceEventHandler(*this));
  m_renderFactory.reset(new AgoraVideoSourceRenderFactory(*this));
  RtcEngineContext context;
  context.eventHandler = m_eventHandler.get();
  context.appId = appid.c_str();
  std::istringstream os(areaCode);
  os >> context.areaCode;

  LOG_INFO("videoSource initialize areaCode: %u", context.areaCode);

  if (m_rtcEngine->initialize(context) != 0) {
    LOG_INFO("%s, AgoraVideoSource initialize failed.\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  agora::util::AutoPtr<agora::media::IMediaEngine> pMediaEngine;
  pMediaEngine.queryInterface(m_rtcEngine.get(), agora::AGORA_IID_MEDIA_ENGINE);

  if (pMediaEngine.get()) {
    pMediaEngine->registerVideoRenderFactory(m_renderFactory.get());
  } else {
    LOG_ERROR("%s, Get media engine failed.\n", __FUNCTION__);
    LOG_LEAVE;
    return false;
  }

  m_rtcEngine->disableAudio();
  m_rtcEngine->enableVideo();

  m_rtcEngine->enableLocalVideo(false);
  m_rtcEngine->muteAllRemoteVideoStreams(true);
  m_rtcEngine->muteAllRemoteAudioStreams(true);

  // prevent videosource from getting camera causing problems in windows
  agora::rtc::AParameter ap(m_rtcEngine.get());
  ap->setParameters("{\"che.video.local.camera_index\":1024}");

  m_ipc->sendMessage(AGORA_IPC_SOURCE_READY, nullptr, 0);
  m_initialized = true;
  LOG_LEAVE;
  return true;
}

node_error AgoraVideoSource::startPreview() {
  int status = 0;
  do {
    std::string id = m_paramParser->getParameter("id");
    m_ipcSender.reset(new AgoraIpcDataSender());
    if (!m_ipcSender->initialize(id + DATA_IPC_NAME)) {
      LOG_ERROR("%s, ipc sender init fail.", __FUNCTION__);
      status = -1;
      break;
    }
    agora::rtc::VideoCanvas canvas;
    canvas.uid = 0;
    canvas.renderMode = agora::rtc::RENDER_MODE_HIDDEN;
    canvas.view = m_renderFactory.get();
    m_rtcEngine->setupLocalVideo(canvas);
  } while (false);
  m_ipc->sendMessage(AGORA_IPC_START_VS_PREVIEW_COMPLETE, (char*)&status,
                     sizeof(status));
  return status == 0 ? node_ok : node_generic_error;
}

node_error AgoraVideoSource::stopPreview() {
  agora::rtc::VideoCanvas canvas;
  m_rtcEngine->setupLocalVideo(canvas);

  {
    std::lock_guard<std::mutex> lock(m_ipcSenderMutex);
    m_ipcSender.reset();
  }

  return m_ipc->sendMessage(AGORA_IPC_STOP_VS_PREVIEW_COMPLETE, nullptr, 0)
             ? node_ok
             : node_generic_error;
}

void AgoraVideoSource::notifyJoinedChannel(uid_t uid) {
  m_ipc->sendMessage(AGORA_IPC_JOIN_SUCCESS, (char*)&uid, sizeof(uid));
}

void AgoraVideoSource::notifyRenderReady() {
  m_ipc->sendMessage(AGORA_IPC_RENDER_READY, nullptr, 0);
}

void AgoraVideoSource::notifyLeaveChannel() {
  m_ipc->sendMessage(AGORA_IPC_LEAVE_CHANNEL, nullptr, 0);
}

void AgoraVideoSource::notifyRequestNewToken() {
  m_ipc->sendMessage(AGORA_IPC_RENEW_TOKEN, nullptr, 0);
}

void AgoraVideoSource::notifyLocalAudioStats(
    const agora::rtc::LocalAudioStats& audioStats) {
  std::unique_ptr<LocalAudioStatsCmd> cmd(new LocalAudioStatsCmd());
  cmd->numChannels = audioStats.numChannels;
  cmd->sentSampleRate = audioStats.sentSampleRate;
  cmd->sentBitrate = audioStats.sentBitrate;
  cmd->txPacketLossRate = audioStats.txPacketLossRate;
  m_ipc->sendMessage(AGORA_IPC_ON_LOCAL_AUDIO_STATS, (char*)cmd.get(),
                     sizeof(LocalAudioStatsCmd));
}

void AgoraVideoSource::notifyLocalVideoStats(
    const agora::rtc::LocalVideoStats& videoStats) {
  std::unique_ptr<LocalVideoStatsCmd> cmd(new LocalVideoStatsCmd());
  cmd->sentBitrate = videoStats.sentBitrate;
  cmd->sentFrameRate = videoStats.sentFrameRate;
  cmd->encoderOutputFrameRate = videoStats.encoderOutputFrameRate;
  cmd->rendererOutputFrameRate = videoStats.rendererOutputFrameRate;
  cmd->targetBitrate = videoStats.targetBitrate;
  cmd->targetFrameRate = videoStats.targetFrameRate;
  cmd->qualityAdaptIndication = videoStats.qualityAdaptIndication;
  cmd->encodedBitrate = videoStats.encodedBitrate;
  cmd->encodedFrameWidth = videoStats.encodedFrameWidth;
  cmd->encodedFrameHeight = videoStats.encodedFrameHeight;
  cmd->encodedFrameCount = videoStats.encodedFrameCount;
  cmd->codecType = videoStats.codecType;
  cmd->txPacketLossRate = videoStats.txPacketLossRate;
  cmd->captureFrameRate = videoStats.captureFrameRate;
  cmd->captureBrightnessLevel = videoStats.captureBrightnessLevel;
  m_ipc->sendMessage(AGORA_IPC_ON_LOCAL_VIDEO_STATS, (char*)cmd.get(),
                     sizeof(LocalVideoStatsCmd));
}

void AgoraVideoSource::notifyVideoSizeChanged(agora::rtc::uid_t uid,
                                              int width,
                                              int height,
                                              int rotation) {
  std::unique_ptr<VideoSizeChangedCmd> cmd(new VideoSizeChangedCmd());
  cmd->uid = uid;
  cmd->width = width;
  cmd->height = height;
  cmd->rotation = rotation;
  m_ipc->sendMessage(AGORA_IPC_ON_VIDEO_SIZECHANGED, (char*)cmd.get(),
                     sizeof(VideoSizeChangedCmd));
}

void AgoraVideoSource::notifyLocalVideoStateChanged(
    agora::rtc::LOCAL_VIDEO_STREAM_STATE localVideoState,
    agora::rtc::LOCAL_VIDEO_STREAM_ERROR error) {
  std::unique_ptr<LocalVideoStateChangedCmd> cmd(
      new LocalVideoStateChangedCmd());
  cmd->error = error;
  cmd->localVideoState = localVideoState;
  m_ipc->sendMessage(AGORA_IPC_ON_LOCAL_VIDEO_STATE_CHANGED, (char*)cmd.get(),
                     sizeof(LocalVideoStateChangedCmd));
}

void AgoraVideoSource::notifyLocalAudioStateChanged(
    agora::rtc::LOCAL_AUDIO_STREAM_STATE state,
    agora::rtc::LOCAL_AUDIO_STREAM_ERROR error) {
  std::unique_ptr<LocalAudioStateChangedCmd> cmd(
      new LocalAudioStateChangedCmd());
  cmd->error = error;
  cmd->localAudioState = state;
  m_ipc->sendMessage(AGORA_IPC_ON_LOCAL_AUDIO_STATE_CHANGED, (char*)cmd.get(),
                     sizeof(LocalAudioStateChangedCmd));
}

void AgoraVideoSource::setProcessDpiAwareness() {
  SetProcessDpiAwarenessEx();
}

void AgoraVideoSource::notifyScreenCaptureInfoUpdated(const char* cardType, int errCode)
{
  std::unique_ptr<ScreenCaptureInfoCmd> cmd(new ScreenCaptureInfoCmd());
  if (cardType) {
    strncpy(cmd->cardType, cardType, MAX_PARAMETER_LEN);
  }
  cmd->errCode = errCode;
  m_ipc->sendMessage(AGORA_IPC_ON_SCREEN_CAPTURE_INFO_UPDATED, (char*)cmd.get(), sizeof(ScreenCaptureInfoCmd));
}

void AgoraVideoSource::release() {
  delete this;
}

std::vector<std::string> split(std::string str, std::string pattern) {
  std::string::size_type pos;
  std::vector<std::string> result;

  str += pattern;
  int size = str.size();

  for (int i = 0; i < size; i++) {
    pos = str.find(pattern, i);
    if (pos < size) {
      std::string s = str.substr(i, pos - i);
      result.push_back(s);
      i = pos + pattern.size() - 1;
    }
  }
  return result;
}

void AgoraVideoSource::onMessage(unsigned int msg,
                                 char* payload,
                                 unsigned int len) {
  LOG_INFO("%s    msg: %d     len:%d", __FUNCTION__, msg, len);
  LOG_ENTER;
  if (!m_initialized) {
    LOG_ERROR("%s, no init.\n", __FUNCTION__);
    LOG_LEAVE;
    return;
  }
  LOG_INFO("%s, msg : %d\n", __FUNCTION__, msg);
  if (msg == AGORA_IPC_JOIN) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_JOIN");
    if (payload) {
      JoinChannelCmd* cmd = (JoinChannelCmd*)payload;
      joinChannel(cmd);
    }
  } else if (msg == AGORA_IPC_CAPTURE_SCREEN) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_CAPTURE_SCREEN");
    if (len != sizeof(CaptureScreenCmd)) {
      LOG_ERROR("%s, Size not equal with capture screen cmd.\n", __FUNCTION__);
      LOG_LEAVE;
      return;
    }
    CaptureScreenCmd* cmd = (CaptureScreenCmd*)payload;
    LOG_INFO("Start screen share, top : %d, left : %d, bottom :%d, right :%d\n",
             cmd->rect.top, cmd->rect.left, cmd->rect.bottom, cmd->rect.right);
    if (m_rtcEngine->startScreenCapture(cmd->windowid, cmd->captureFreq,
                                        &cmd->rect, cmd->bitrate) != 0) {
      LOG_ERROR("start screen capture failed.");
      m_rtcEngine->enableLocalVideo(false);
    } else {
      m_rtcEngine->enableLocalVideo(true);
    }
  } else if (msg == AGORA_IPC_STOP_CAPTURE_SCREEN) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_STOP_CAPTURE_SCREEN");
    m_rtcEngine->enableLocalVideo(false);
    m_rtcEngine->stopScreenCapture();
  } else if (msg == AGORA_IPC_START_VS_PREVIEW) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_START_VS_PREVIEW");
    this->startPreview();
  } else if (msg == AGORA_IPC_STOP_VS_PREVIEW) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_STOP_VS_PREVIEW");
    this->stopPreview();
  } else if (msg == AGORA_IPC_RENEW_TOKEN) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_RENEW_TOKEN");
    m_rtcEngine->renewToken(payload);
  } else if (msg == AGORA_IPC_SET_CHANNEL_PROFILE) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_SET_CHANNEL_PROFILE");
    if (payload) {
      ChannelProfileCmd* cmd = (ChannelProfileCmd*)payload;
      m_rtcEngine->setChannelProfile(cmd->profile);
      if (cmd->profile == agora::rtc::CHANNEL_PROFILE_LIVE_BROADCASTING) {
        m_rtcEngine->setClientRole(agora::rtc::CLIENT_ROLE_BROADCASTER);
      }
    }
  } else if (msg == AGORA_IPC_SET_VIDEO_RPOFILE) {
    if (len != sizeof(VideoProfileCmd)) {
      LOG_ERROR("%s, size not equal with video profile size.\n", __FUNCTION__);
      LOG_LEAVE;
      return;
    }
    VideoProfileCmd* cmd = (VideoProfileCmd*)payload;
    if (cmd->profile > agora::rtc::VIDEO_PROFILE_LANDSCAPE_4K_3) {
      LOG_ERROR("%s, set video profile with invalid value : %d", __FUNCTION__,
                cmd->profile);
    } else {
      this->m_videoProfile = (agora::rtc::VIDEO_PROFILE_TYPE)cmd->profile;
      m_rtcEngine->setVideoProfile(cmd->profile, cmd->swapWidthAndHeight);
    }
  } else if (msg == AGORA_IPC_LEAVE_CHANNEL) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_LEAVE_CHANNEL");
    m_rtcEngine->leaveChannel();
  } else if (msg == AGORA_IPC_DISCONNECT) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_DISCONNECT");
    this->exit(false);
  } else if (msg == AGORA_IPC_ENABLE_WEB_SDK_INTEROPERABILITY) {
    m_rtcEngine->enableWebSdkInteroperability((bool)*payload);
  } else if (msg == AGORA_IPC_ENABLE_DUAL_STREAM_MODE) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_ENABLE_DUAL_STREAM_MODE");
    m_rtcEngine->enableDualStreamMode((bool)*payload);
  } else if (msg == AGORA_IPC_SET_LOGFILE) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_SET_LOGFILE");
    m_rtcEngine->setLogFile((char*)payload);
  } else if (msg == AGORA_IPC_SET_PARAMETER) {
    if (len != sizeof(SetParameterCmd))
      return;
    SetParameterCmd* cmd = (SetParameterCmd*)payload;
    agora::rtc::AParameter rep(m_rtcEngine.get());
    rep->setParameters(cmd->parameters);
  } else if (msg == AGORA_IPC_UPDATE_CAPTURE_SCREEN) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_UPDATE_CAPTURE_SCREEN");
    if (payload) {
      m_rtcEngine->updateScreenCaptureRegion((const agora::rtc::Rect*)payload);
    }
  } else if (msg == AGORA_IPC_START_CAPTURE_BY_DISPLAY) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_START_CAPTURE_BY_DISPLAY");
    if (payload) {
      CaptureScreenByDisplayCmd* cmd = (CaptureScreenByDisplayCmd*)payload;
      agora::rtc::view_t excludeWindows[MAX_WINDOW_ID_COUNT] = {nullptr};
      if (cmd->excludeWindowCount > 0) {
        for (int i = 0; i < cmd->excludeWindowCount; ++i) {
          agora::rtc::view_t windowId =
              reinterpret_cast<agora::rtc::view_t>(cmd->excludeWindowList[i]);
          excludeWindows[i] = windowId;
        }
        cmd->captureParams.excludeWindowList = excludeWindows;
        cmd->captureParams.excludeWindowCount = cmd->excludeWindowCount;
      }
      int result = 0;
#if defined(_WIN32)
      result = m_rtcEngine->startScreenCaptureByScreenRect(
          cmd->screenId, cmd->regionRect, cmd->captureParams);
#elif defined(__APPLE__)
      result = m_rtcEngine->startScreenCaptureByDisplayId(
          cmd->screenId.idVal, cmd->regionRect, cmd->captureParams);
#endif
     if (result != 0) {
       LOG_ERROR("start screen capture by display failed.");
       m_rtcEngine->enableLocalVideo(false);
     } else {
       m_rtcEngine->enableLocalVideo(true);
     }
    }
  } else if (msg == AGORA_IPC_START_CAPTURE_BY_WINDOW_ID) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_START_CAPTURE_BY_WINDOW_ID");
    if (payload) {
      CaptureScreenByWinCmd* cmd = (CaptureScreenByWinCmd*)payload;
      int result = m_rtcEngine->startScreenCaptureByWindowId(
          reinterpret_cast<agora::rtc::view_t>(cmd->windowId), cmd->regionRect,
          cmd->captureParams);

      if (result != 0) {
        LOG_ERROR("start screen capture by display failed.");
          m_rtcEngine->enableLocalVideo(false);
      } else {
          m_rtcEngine->enableLocalVideo(true);
      }
    }
  } else if (msg == AGORA_IPC_START_SCREEN_CAPTURE_BY_DISPLAY_ID) {
    if (payload) {
      CaptureScreenByDisplayCmd* cmd = (CaptureScreenByDisplayCmd*)payload;
      agora::rtc::view_t excludeWindows[MAX_WINDOW_ID_COUNT] = {nullptr};
      if (cmd->excludeWindowCount > 0) {
        for (int i = 0; i < cmd->excludeWindowCount; ++i) {
          agora::rtc::view_t windowId =
              reinterpret_cast<agora::rtc::view_t>(cmd->excludeWindowList[i]);
          excludeWindows[i] = windowId;
        }
        cmd->captureParams.excludeWindowList = excludeWindows;
        cmd->captureParams.excludeWindowCount = cmd->excludeWindowCount;
      }
      int result = m_rtcEngine->startScreenCaptureByDisplayId(
                                                                       cmd->displayInfo.idVal, cmd->regionRect, cmd->captureParams);
      LOG_INFO("startScreenCaptureByDisplayId res:%d",result);                                                                       
      if (result == 0 || result == 1736 || result == -1736) {
        m_rtcEngine->enableLocalVideo(true);
      } else {
        LOG_ERROR("start screen capture by display failed.");
        m_rtcEngine->enableLocalVideo(false);
      }
    }
  } else if (msg == AGORA_IPC_UPDATE_SCREEN_CAPTURE_PARAMS) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_UPDATE_SCREEN_CAPTURE_PARAMS");
    if (payload) {
      ScreenCaptureParametersCmd* cmd = (ScreenCaptureParametersCmd*)payload;
      agora::rtc::view_t excludeWindows[MAX_WINDOW_ID_COUNT] = {nullptr};
      if (cmd->excludeWindowCount > 0) {
        for (int i = 0; i < cmd->excludeWindowCount; ++i) {
          agora::rtc::view_t windowId =
              reinterpret_cast<agora::rtc::view_t>(cmd->excludeWindowList[i]);
          excludeWindows[i] = windowId;
        }
        cmd->captureParams.excludeWindowList = excludeWindows;
        cmd->captureParams.excludeWindowCount = cmd->excludeWindowCount;
      }

      m_rtcEngine->updateScreenCaptureParameters(cmd->captureParams);
    }
  } else if (msg == AGORA_IPC_SET_SCREEN_CAPTURE_CONTENT_HINT) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_SET_SCREEN_CAPTURE_CONTENT_HINT");
    m_rtcEngine->setScreenCaptureContentHint(
        (agora::rtc::VideoContentHint)*payload);
  } else if (msg == AGORA_IPC_ENABLE_LOOPBACK_RECORDING) {
    LOG_INFO("%s    msg: %s", __FUNCTION__,
             "AGORA_IPC_ENABLE_LOOPBACK_RECORDING");
    if (len != sizeof(LoopbackRecordingCmd))
      return;
    LoopbackRecordingCmd* cmd = (LoopbackRecordingCmd*)payload;
    m_rtcEngine->enableLoopbackRecording(cmd->enabled, cmd->deviceName);
  } else if (msg == AGORA_IPC_ENABLE_AUDIO) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_ENABLE_AUDIO");
    m_rtcEngine->enableAudio();
  } else if (msg == AGORA_IPC_SET_ENCRYPTION_MODE) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_SET_ENCRYPTION_MODE");
    m_rtcEngine->setEncryptionMode((const char*)payload);
  } else if (msg == AGORA_IPC_ENABLE_ENCRYPTION) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_ENABLE_ENCRYPTION");
    EncryptionConfigCmd* cmd = (EncryptionConfigCmd*)payload;
    agora::rtc::EncryptionConfig config;
    config.encryptionKey = cmd->encryptionKey;
    config.encryptionMode = cmd->encryptionMode;
    m_rtcEngine->enableEncryption(cmd->enable, config);
  } else if (msg == AGORA_IPC_SET_ENCRYPTION_SECRET) {
    LOG_INFO("%s    msg: %s", __FUNCTION__, "AGORA_IPC_SET_ENCRYPTION_SECRET");
    m_rtcEngine->setEncryptionSecret((const char *)payload);
  } else if (msg == AGORA_IPC_SET_PROCESS_DPI_AWARE_NESS) {
    setProcessDpiAwareness();
  } else if (msg == AGORA_IPC_MUTE_REMOTE_AUDIO_STREAM) {
    MuteRemoteStreamsCmd *cmd = (MuteRemoteStreamsCmd *)payload;
    m_rtcEngine->muteRemoteAudioStream(cmd->uid, cmd->mute);
  } else if (msg == AGORA_IPC_MUTE_ALL_REMOTE_AUDIO_STREAMS) {
    bool mute = (bool)*payload;
    m_rtcEngine->muteAllRemoteAudioStreams(mute);
  } else if (msg == AGORA_IPC_MUTE_REMOTE_VIDEO_STREAM) {
    MuteRemoteStreamsCmd *cmd = (MuteRemoteStreamsCmd *)payload;
    m_rtcEngine->muteRemoteVideoStream(cmd->uid, cmd->mute);
  } else if (msg == AGORA_IPC_MUTE_ALL_REMOTE_VIDEO_STREAMS) {
    bool mute = (bool)*payload;
    m_rtcEngine->muteAllRemoteVideoStreams(mute);
  } else if (msg == AGORA_IPC_ADJUST_RECORDING_SIGNAL_VOLUME) {
    int volume = *payload;
    LOG_F(INFO, "video_source_adjustRecordingSignalVolume:%d", volume);
    m_rtcEngine->adjustRecordingSignalVolume(volume);
  } else if (msg == AGORA_IPC_SET_SCREEN_CAPTURE_SCENARIO) {
    int screenScenario = *payload;
    LOG_F(INFO, "video_source_setScreenCaptureScenario:%d", screenScenario);
    m_rtcEngine->setScreenCaptureScenario((agora::rtc::SCREEN_SCENARIO_TYPE)screenScenario);
  } else if (msg == AGORA_IPC_ADJUST_LOOPBACK_RECORDING_SIGNAL_VOLUME) {
    int volume = *payload;
    LOG_F(INFO, "video_source_adjustLoopbackRecordingSignalVolume:%d", volume);
    m_rtcEngine->adjustLoopbackRecordingSignalVolume(volume);
  } else if (msg == AGORA_IPC_DISABLE_AUDIO) {
    m_rtcEngine->disableAudio();
  } else if (msg == AGORA_IPC_SET_ADDON_LOGFILE) {
    stopLogService();
    startLogService((char *)payload);
    LOG_INFO("set addon log file %s\n", (char *)payload);
  } else if (msg == AGORA_IPC_SET_LOCAL_ACCESS_POINT) {
    LocalAccessPointConfigurationCmd *cmd =
        (LocalAccessPointConfigurationCmd *)payload;

    agora::rtc::LocalAccessPointConfiguration localAccessPointConfiguration;

    std::vector<std::string> ipListVecString =
        split(std::string(cmd->ipList), IPC_STRING_PARTTERN);
    std::vector<const char *> ipListVec;
    if (cmd->ipList) {
      auto ipListSize = ipListVecString.size() - 1;
      for (int index = 0; index < ipListSize; ++index) {
        ipListVec.push_back(ipListVecString[index].c_str());
      }
      localAccessPointConfiguration.ipList = ipListVec.data();
      localAccessPointConfiguration.ipListSize = ipListSize;
    }

    std::vector<std::string> domainListVecString =
        split(std::string(cmd->domainList), IPC_STRING_PARTTERN);
    std::vector<const char *> domainListVec;
    if (cmd->domainList) {

      auto domainListSize = domainListVecString.size() - 1;
      for (int index = 0; index < domainListSize; ++index) {
        domainListVec.push_back(domainListVecString[index].c_str());
      }
      localAccessPointConfiguration.domainList = domainListVec.data();
      localAccessPointConfiguration.domainListSize = domainListSize;
    }

    localAccessPointConfiguration.mode = cmd->mode;
    localAccessPointConfiguration.verifyDomainName = cmd->verifyDomainName;

    m_rtcEngine->setLocalAccessPoint(localAccessPointConfiguration);
    ;
    LOG_INFO("AGORA_IPC_SET_LOCAL_ACCESS_POINT");
  }

  LOG_LEAVE;
}

bool AgoraVideoSource::joinChannel(JoinChannelCmd* cmd) {

  agora::rtc::ChannelMediaOptions options;
  options.autoSubscribeAudio = cmd->autoSubscribeAudio;
  options.autoSubscribeVideo = cmd->autoSubscribeVideo;
  options.publishLocalAudio = cmd->publishLocalAudio;
  options.publishLocalVideo = cmd->publishLocalVideo;
  return m_rtcEngine->joinChannel(
      cmd->token ? cmd->token : "", cmd->cname ? cmd->cname : "",
      cmd->chan_info ? cmd->chan_info : "", cmd->uid, options);
}

void AgoraVideoSource::exit(bool notifySink) {
  m_rtcEngine->unregisterEventHandler(this->m_eventHandler.get());
  m_ipc->disconnect();
  LOG_F(INFO, "VideoSource::leaveChannel");
  m_rtcEngine->leaveChannel();
  LOG_F(INFO, "VideoSource::release");
  m_rtcEngine->release(true);
  //CSD-42301: change call order
  {
    // fix CSD-8509
    // std::lock_guard<std::mutex> lock(m_ipcSenderMutex);
    m_ipcSender.reset();
  }
  LOG_F(INFO, "VideoSource::exit");
  stopLogService();
  ::exit(0);
}

void AgoraVideoSource::run() {
  LOG_ENTER;
#ifdef _WIN32
  std::string idstr = m_paramParser->getParameter("pid");
#else
  std::string idstr = m_paramParser->getParameter("fd");
#endif
  if (idstr.empty()) {
    LOG_ERROR("%s, pid is null\n", __FUNCTION__);
    LOG_LEAVE;
    return;
  }
  m_process.reset(INodeProcess::OpenNodeProcess(std::atoi(idstr.c_str())));
  if (!m_process.get()) {
    LOG_ERROR("Process open fail.\n");
    LOG_LEAVE;
    return;
  }
  m_process->Monitor([this](INodeProcess*) {
    LOG_WARNING("%s, remote process ext.\n", __FUNCTION__);
    this->exit(false);
  });
  m_ipc->run();
  LOG_LEAVE;
}

bool AgoraVideoSource::sendData(char* payload, int len) {
  if (!payload || len == 0)
    return false;

  std::lock_guard<std::mutex> lock(m_ipcSenderMutex);

  if (m_ipcSender) {
    m_ipcSender->sendData(payload, len);
    return true;
  } else {
    LOG_WARNING("IPC Sender not initialized before send data.");
    return false;
  }
}

agora::rtc::VIDEO_PROFILE_TYPE AgoraVideoSource::getVideoProfile() {
  return m_videoProfile;
}

void initLogService() {
  std::string currentPath;
  INodeProcess::getCurrentModuleFileName(currentPath);
  std::string logFile = currentPath + ".txt";
  startLogService(logFile.c_str());
}

void run(std::string param) {
  auto videoSource = new AgoraVideoSource(param);
  videoSource->initialize();
  videoSource->run();
  videoSource->exit(true);
}
