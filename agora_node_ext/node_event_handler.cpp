/*
 * Copyright (c) 2017 Agora.io
 * All rights reserved.
 * Proprietry and Confidential -- Agora.io
 */

/*
 *  Created by Wang Yongli, 2017
 */

#include "node_event_handler.h"
#include <stdio.h>
#include "agora_rtc_engine.h"
#include "node_async_queue.h"
#include "node_log.h"
#include "node_uid.h"
#include "uv.h"
namespace agora {
namespace rtc {

#define FUNC_TRACE

NodeEventHandler::NodeEventHandler(NodeRtcEngine* pEngine)
    : m_engine(pEngine) {}

NodeEventHandler::~NodeEventHandler() {
  for (auto& handler : m_callbacks) {
    delete handler.second;
  }
}

#define MAKE_JS_CALL_0(ev)                                              \
  auto it = m_callbacks.find(ev);                                       \
  if (it != m_callbacks.end()) {                                        \
    Isolate* isolate = Isolate::GetCurrent();                           \
    HandleScope scope(isolate);                                         \
    Local<Context> context = isolate->GetCurrentContext();              \
    NodeEventCallback& cb = *it->second;                                \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 0, \
                                   nullptr);                            \
  }

#define MAKE_JS_CALL_1(ev, type, param)                                        \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate* isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[1]{napi_create_##type##_(isolate, param)};               \
    NodeEventCallback& cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 1, argv); \
  }

#define MAKE_JS_CALL_2(ev, type1, param1, type2, param2)                       \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate* isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[2]{napi_create_##type1##_(isolate, param1),              \
                         napi_create_##type2##_(isolate, param2)};             \
    NodeEventCallback& cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 2, argv); \
  }

#define MAKE_JS_CALL_3(ev, type1, param1, type2, param2, type3, param3)        \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate* isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[3]{napi_create_##type1##_(isolate, param1),              \
                         napi_create_##type2##_(isolate, param2),              \
                         napi_create_##type3##_(isolate, param3)};             \
    NodeEventCallback& cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 3, argv); \
  }

#define MAKE_JS_CALL_4(ev, type1, param1, type2, param2, type3, param3, type4, \
                       param4)                                                 \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate* isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[4]{                                                      \
        napi_create_##type1##_(isolate, param1),                               \
        napi_create_##type2##_(isolate, param2),                               \
        napi_create_##type3##_(isolate, param3),                               \
        napi_create_##type4##_(isolate, param4),                               \
    };                                                                         \
    NodeEventCallback& cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 4, argv); \
  }

#define MAKE_JS_CALL_5(ev, type1, param1, type2, param2, type3, param3, type4, \
                       param4, type5, param5)                                  \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate* isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[5]{                                                      \
        napi_create_##type1##_(isolate, param1),                               \
        napi_create_##type2##_(isolate, param2),                               \
        napi_create_##type3##_(isolate, param3),                               \
        napi_create_##type4##_(isolate, param4),                               \
        napi_create_##type5##_(isolate, param5),                               \
    };                                                                         \
    NodeEventCallback& cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 5, argv); \
  }

#define MAKE_JS_CALL_6(ev, type1, param1, type2, param2, type3, param3, type4, \
                       param4, type5, param5, type6, param6)                   \
  auto it = m_callbacks.find(ev);                                              \
  if (it != m_callbacks.end()) {                                               \
    Isolate *isolate = Isolate::GetCurrent();                                  \
    HandleScope scope(isolate);                                                \
    Local<Context> context = isolate->GetCurrentContext();                     \
    Local<Value> argv[6]{                                                      \
        napi_create_##type1##_(isolate, param1),                               \
        napi_create_##type2##_(isolate, param2),                               \
        napi_create_##type3##_(isolate, param3),                               \
        napi_create_##type4##_(isolate, param4),                               \
        napi_create_##type5##_(isolate, param5),                               \
        napi_create_##type6##_(isolate, param6),                               \
    };                                                                         \
    NodeEventCallback &cb = *it->second;                                       \
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 6, argv); \
  }

#define CHECK_NAPI_OBJ(obj) \
  if (obj.IsEmpty())        \
    break;

#define NODE_SET_OBJ_PROP_STRING(obj, name, val)                         \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
    CHECK_NAPI_OBJ(propName);                                            \
    Local<Value> propVal =                                               \
        String::NewFromUtf8(isolate, val, NewStringType::kInternalized)  \
            .ToLocalChecked();                                           \
    CHECK_NAPI_OBJ(propVal);                                             \
    v8::Maybe<bool> ret =                                                \
        obj->Set(isolate->GetCurrentContext(), propName, propVal);       \
    if (!ret.IsNothing()) {                                              \
      if (!ret.ToChecked()) {                                            \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }

#define NODE_SET_OBJ_PROP_UINT32(obj, name, val)                         \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
    CHECK_NAPI_OBJ(propName);                                            \
    Local<Value> propVal = v8::Uint32::New(isolate, val);                \
    CHECK_NAPI_OBJ(propVal);                                             \
    v8::Maybe<bool> ret =                                                \
        obj->Set(isolate->GetCurrentContext(), propName, propVal);       \
    if (!ret.IsNothing()) {                                              \
      if (!ret.ToChecked()) {                                            \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }

#define NODE_SET_OBJ_PROP_UID(obj, name, val)                            \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
    CHECK_NAPI_OBJ(propName);                                            \
    Local<Value> propVal = NodeUid::getNodeValue(isolate, val);          \
    CHECK_NAPI_OBJ(propVal);                                             \
    v8::Maybe<bool> ret =                                                \
        obj->Set(isolate->GetCurrentContext(), propName, propVal);       \
    if (!ret.IsNothing()) {                                              \
      if (!ret.ToChecked()) {                                            \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }

#define NODE_SET_OBJ_PROP_NUMBER(obj, name, val)                         \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
    CHECK_NAPI_OBJ(propName);                                            \
    Local<Value> propVal = v8::Number::New(isolate, val);                \
    CHECK_NAPI_OBJ(propVal);                                             \
    v8::Maybe<bool> ret =                                                \
        obj->Set(isolate->GetCurrentContext(), propName, propVal);       \
    if (!ret.IsNothing()) {                                              \
      if (!ret.ToChecked()) {                                            \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }

void NodeEventHandler::onJoinChannelSuccess_node(const char* channel,
                                                 uid_t id,
                                                 int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_JOIN_CHANNEL, string, channel, uid, id, int32,
                 elapsed);
}

void NodeEventHandler::onJoinChannelSuccess(const char* channel,
                                            uid_t uid,
                                            int elapsed) {
  FUNC_TRACE;
  std::string channelName = channel;
  node_async_call::async_call([this, channelName, uid, elapsed]() {
    this->onJoinChannelSuccess_node(channelName.c_str(), uid, elapsed);
  });
}

void NodeEventHandler::onRejoinChannelSuccess_node(const char* channel,
                                                   uid_t uid,
                                                   int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_REJOIN_CHANNEL, string, channel, uid, uid, int32,
                 elapsed);
}

void NodeEventHandler::onRejoinChannelSuccess(const char* channel,
                                              uid_t uid,
                                              int elapsed) {
  FUNC_TRACE;
  std::string channelName(channel);
  node_async_call::async_call([this, channelName, uid, elapsed]() {
    this->onRejoinChannelSuccess_node(channelName.c_str(), uid, elapsed);
  });
}

void NodeEventHandler::onWarning_node(int warn, const char* msg) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_WARNING, int32, warn, string, msg);
}

void NodeEventHandler::onWarning(int warn, const char* msg) {
  FUNC_TRACE;
  std::string message;
  if (msg)
    message.assign(msg);
  node_async_call::async_call(
      [this, warn, message]() { this->onWarning_node(warn, message.c_str()); });
}

void NodeEventHandler::onError_node(int err, const char* msg) {
  MAKE_JS_CALL_2(RTC_EVENT_ERROR, int32, err, string, msg);
}

void NodeEventHandler::onError(int err, const char* msg) {
  std::string errorDesc;
  if (msg)
    errorDesc.assign(msg);
  node_async_call::async_call(
      [this, err, errorDesc] { this->onError_node(err, errorDesc.c_str()); });
}

void NodeEventHandler::onAudioQuality_node(uid_t uid,
                                           int quality,
                                           unsigned short dealy,
                                           unsigned short lost) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_AUDIO_QUALITY, uid, uid, int32, quality, uint16,
                 dealy, uint16, lost);
}

void NodeEventHandler::onAudioQuality(uid_t uid,
                                      int quality,
                                      unsigned short dealy,
                                      unsigned short lost) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, quality, dealy, lost] {
    this->onAudioQuality_node(uid, quality, dealy, lost);
  });
}

void NodeEventHandler::onAudioVolumeIndication_node(AudioVolumeInfo* speakers,
                                                    unsigned int speakerNumber,
                                                    int totalVolume) {
  FUNC_TRACE;
  auto it = m_callbacks.find(RTC_EVENT_AUDIO_VOLUME_INDICATION);
  if (it != m_callbacks.end()) {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<v8::Array> arrSpeakers = v8::Array::New(isolate, speakerNumber);
    for (unsigned int i = 0; i < speakerNumber; i++) {
      Local<Object> obj = Object::New(isolate);
      obj->Set(context, napi_create_string_(isolate, "uid"),
               napi_create_uid_(isolate, speakers[i].uid));
      obj->Set(context, napi_create_string_(isolate, "volume"),
               napi_create_uint32_(isolate, speakers[i].volume));
      obj->Set(context, napi_create_string_(isolate, "vad"),
               napi_create_uint32_(isolate, speakers[i].vad));
      arrSpeakers->Set(context, i, obj);
    }

    Local<Value> argv[3]{arrSpeakers,
                         napi_create_uint32_(isolate, speakerNumber),
                         napi_create_uint32_(isolate, totalVolume)};
    NodeEventCallback& cb = *it->second;
    cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 3, argv);
  }
  // MAKE_JS_CALL_4(RTC_EVENT_AUDIO_VOLUME_INDICATION, uid, speaker.uid, uint32,
  // speaker.volume, uint32, speakerNumber, int32, totalVolume);
}

void NodeEventHandler::onAudioVolumeIndication(const AudioVolumeInfo* speaker,
                                               unsigned int speakerNumber,
                                               int totalVolume) {
  FUNC_TRACE;
  if (speaker) {
    AudioVolumeInfo* localSpeakers = new AudioVolumeInfo[speakerNumber];
    for (unsigned int i = 0; i < speakerNumber; i++) {
      AudioVolumeInfo tmp = speaker[i];
      localSpeakers[i].uid = tmp.uid;
      localSpeakers[i].volume = tmp.volume;
      localSpeakers[i].vad = tmp.vad;
    }
    node_async_call::async_call(
        [this, localSpeakers, speakerNumber, totalVolume] {
          this->onAudioVolumeIndication_node(localSpeakers, speakerNumber,
                                             totalVolume);
          delete[] localSpeakers;
        });
  } else {
    node_async_call::async_call([this, speakerNumber, totalVolume] {
      this->onAudioVolumeIndication_node(NULL, speakerNumber, totalVolume);
    });
  }
}

void NodeEventHandler::onLeaveChannel_node(const RtcStats& stats) {
  FUNC_TRACE;
  unsigned int usercount = stats.userCount;
  LOG_INFO(
      "duration : %d, tx :%d, rx :%d, txbr :%d, rxbr :%d, txAudioBr :%d, "
      "rxAudioBr :%d, users :%d\n",
      stats.duration, stats.txBytes, stats.rxBytes, stats.txKBitRate,
      stats.rxKBitRate, stats.txAudioKBitRate, stats.rxAudioKBitRate,
      usercount);
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);
    NODE_SET_OBJ_PROP_UINT32(obj, "duration", stats.duration);
    NODE_SET_OBJ_PROP_UINT32(obj, "txBytes", stats.txBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxBytes", stats.rxBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txKBitRate", stats.txKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxKBitRate", stats.rxKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxAudioBytes", stats.rxAudioBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txAudioBytes", stats.txAudioBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxVideoBytes", stats.rxVideoBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txVideoBytes", stats.txVideoBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxAudioKBitRate", stats.rxAudioKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "txAudioKBitRate", stats.txAudioKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxVideoKBitRate", stats.rxVideoKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "txVideoKBitRate", stats.txVideoKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "lastmileDelay", stats.lastmileDelay);
    NODE_SET_OBJ_PROP_UINT32(obj, "users", usercount);
    NODE_SET_OBJ_PROP_UINT32(obj, "userCount", stats.userCount);
    NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxPacketLossRate", stats.rxPacketLossRate);
    NODE_SET_OBJ_PROP_NUMBER(obj, "cpuAppUsage", stats.cpuAppUsage);
    NODE_SET_OBJ_PROP_NUMBER(obj, "cpuTotalUsage", stats.cpuTotalUsage);
    NODE_SET_OBJ_PROP_NUMBER(obj, "gatewayRtt", stats.gatewayRtt);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryAppUsageRatio",
                             stats.memoryAppUsageRatio);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryTotalUsageRatio",
                             stats.memoryTotalUsageRatio);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryAppUsageInKbytes",
                             stats.memoryAppUsageInKbytes);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_LEAVE_CHANNEL);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onLeaveChannel(const RtcStats& stats) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, stats] { this->onLeaveChannel_node(stats); });
}

void NodeEventHandler::onRtcStats_node(const RtcStats& stats) {
  unsigned int usercount = stats.userCount;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);
    NODE_SET_OBJ_PROP_UINT32(obj, "duration", stats.duration);
    NODE_SET_OBJ_PROP_UINT32(obj, "txBytes", stats.txBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxBytes", stats.rxBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txKBitRate", stats.txKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxKBitRate", stats.rxKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxAudioBytes", stats.rxAudioBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txAudioBytes", stats.txAudioBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxVideoBytes", stats.rxVideoBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "txVideoBytes", stats.txVideoBytes);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxAudioKBitRate", stats.rxAudioKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "txAudioKBitRate", stats.txAudioKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxVideoKBitRate", stats.rxVideoKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "txVideoKBitRate", stats.txVideoKBitRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "lastmileDelay", stats.lastmileDelay);
    NODE_SET_OBJ_PROP_UINT32(obj, "users", usercount);
    NODE_SET_OBJ_PROP_UINT32(obj, "userCount", stats.userCount);
    NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxPacketLossRate", stats.rxPacketLossRate);
    NODE_SET_OBJ_PROP_NUMBER(obj, "cpuAppUsage", stats.cpuAppUsage);
    NODE_SET_OBJ_PROP_NUMBER(obj, "cpuTotalUsage", stats.cpuTotalUsage);
    NODE_SET_OBJ_PROP_NUMBER(obj, "gatewayRtt", stats.gatewayRtt);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryAppUsageRatio",
                             stats.memoryAppUsageRatio);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryTotalUsageRatio",
                             stats.memoryTotalUsageRatio);
    NODE_SET_OBJ_PROP_NUMBER(obj, "memoryAppUsageInKbytes",
                             stats.memoryAppUsageInKbytes);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_RTC_STATS);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onRtcStats(const RtcStats& stats) {
  node_async_call::async_call([this, stats] { this->onRtcStats_node(stats); });
}

void NodeEventHandler::onAudioDeviceStateChanged_node(const char* deviceId,
                                                      int deviceType,
                                                      int deviceStats) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_AUDIO_DEVICE_STATE_CHANGED, string, deviceId, int32,
                 deviceType, int32, deviceStats);
}

void NodeEventHandler::onAudioDeviceStateChanged(const char* deviceId,
                                                 int deviceType,
                                                 int deviceStats) {
  FUNC_TRACE;
  std::string id(deviceId);
  node_async_call::async_call([this, id, deviceType, deviceStats] {
    this->onAudioDeviceStateChanged_node(id.c_str(), deviceType, deviceStats);
  });
}

void NodeEventHandler::onAudioMixingFinished_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_AUDIO_MIXING_FINISHED);
}

void NodeEventHandler::onAudioMixingFinished() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onAudioMixingFinished_node(); });
}

void NodeEventHandler::onRemoteAudioMixingBegin_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_REMOTE_AUDIO_MIXING_BEGIN);
}

void NodeEventHandler::onRemoteAudioMixingBegin() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onRemoteAudioMixingBegin_node(); });
}

void NodeEventHandler::onRemoteAudioMixingEnd_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_REMOTE_AUDIO_MIXING_END);
}

void NodeEventHandler::onRemoteAudioMixingEnd() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onRemoteAudioMixingEnd_node(); });
}

void NodeEventHandler::onAudioEffectFinished_node(int soundId) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_AUDIO_EFFECT_FINISHED, int32, soundId);
}

void NodeEventHandler::onAudioEffectFinished(int soundId) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, soundId] { this->onAudioEffectFinished_node(soundId); });
}

void NodeEventHandler::onVideoDeviceStateChanged_node(const char* deviceId,
                                                      int deviceType,
                                                      int deviceState) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_VIDEO_DEVICE_STATE_CHANGED, string, deviceId, int32,
                 deviceType, int32, deviceState);
}

void NodeEventHandler::onVideoDeviceStateChanged(const char* deviceId,
                                                 int deviceType,
                                                 int deviceState) {
  FUNC_TRACE;
  std::string id(deviceId);
  node_async_call::async_call([this, id, deviceType, deviceState] {
    this->onVideoDeviceStateChanged_node(id.c_str(), deviceType, deviceState);
  });
}

void NodeEventHandler::onNetworkQuality_node(uid_t uid,
                                             int txQuality,
                                             int rxQuality) {
  // event_log("uid : %d, txQuality :%d, rxQuality :%d\n", uid, txQuality,
  // rxQuality);
  MAKE_JS_CALL_3(RTC_EVENT_NETWORK_QUALITY, uid, uid, int32, txQuality, int32,
                 rxQuality);
}

void NodeEventHandler::onNetworkQuality(uid_t uid,
                                        int txQuality,
                                        int rxQuality) {
  node_async_call::async_call([this, uid, txQuality, rxQuality] {
    this->onNetworkQuality_node(uid, txQuality, rxQuality);
  });
}

void NodeEventHandler::onLastmileQuality_node(int quality) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_LASTMILE_QUALITY, int32, quality);
}

void NodeEventHandler::onLastmileQuality(int quality) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, quality] { this->onLastmileQuality_node(quality); });
}

void NodeEventHandler::onFirstLocalVideoFrame_node(int width,
                                                   int height,
                                                   int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_FIRST_LOCAL_VIDEO_FRAME, int32, width, int32, height,
                 int32, elapsed);
}

void NodeEventHandler::onFirstLocalVideoFrame(int width,
                                              int height,
                                              int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, width, height, elapsed] {
    this->onFirstLocalVideoFrame_node(width, height, elapsed);
  });
}

void NodeEventHandler::onFirstRemoteVideoDecoded_node(uid_t uid,
                                                      int width,
                                                      int height,
                                                      int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_FIRST_REMOTE_VIDEO_DECODED, uid, uid, int32, width,
                 int32, height, int32, elapsed);
}

void NodeEventHandler::onFirstRemoteVideoDecoded(uid_t uid,
                                                 int width,
                                                 int height,
                                                 int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, width, height, elapsed] {
    this->onFirstRemoteVideoDecoded_node(uid, width, height, elapsed);
  });
}

void NodeEventHandler::onVideoSizeChanged_node(uid_t uid,
                                               int width,
                                               int height,
                                               int rotation) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_VIDEO_SIZE_CHANGED, uid, uid, int32, width, int32,
                 height, int32, rotation);
}

void NodeEventHandler::onVideoSizeChanged(uid_t uid,
                                          int width,
                                          int height,
                                          int rotation) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, width, height, rotation] {
    this->onVideoSizeChanged_node(uid, width, height, rotation);
  });
}

void NodeEventHandler::onRemoteVideoStateChanged_node(
    uid_t uid,
    REMOTE_VIDEO_STATE state,
    REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_REMOTE_VIDEO_STATE_CHANGED, uid, uid, int32, state,
                 int32, reason, int32, elapsed);
}

void NodeEventHandler::onRemoteVideoStateChanged(
    uid_t uid,
    REMOTE_VIDEO_STATE state,
    REMOTE_VIDEO_STATE_REASON reason,
    int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, state, reason, elapsed] {
    this->onRemoteVideoStateChanged_node(uid, state, reason, elapsed);
  });
}

void NodeEventHandler::onFirstRemoteVideoFrame_node(uid_t uid,
                                                    int width,
                                                    int height,
                                                    int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_FIRST_REMOTE_VIDEO_FRAME, uid, uid, int32, width,
                 int32, height, int32, elapsed);
}

void NodeEventHandler::onUserJoined_node(uid_t uid, int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_JOINED, uid, uid, int32, elapsed);
}

void NodeEventHandler::onUserJoined(uid_t uid, int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, elapsed] { this->onUserJoined_node(uid, elapsed); });
}

void NodeEventHandler::onUserOffline_node(uid_t uid,
                                          USER_OFFLINE_REASON_TYPE reason) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_OFFLINE, uid, uid, int32, reason);
}

void NodeEventHandler::onUserOffline(uid_t uid,
                                     USER_OFFLINE_REASON_TYPE reason) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, reason] { this->onUserOffline_node(uid, reason); });
}

void NodeEventHandler::onUserMuteAudio_node(uid_t uid, bool muted) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_MUTE_AUDIO, uid, uid, bool, muted);
}

void NodeEventHandler::onUserMuteAudio(uid_t uid, bool muted) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, muted] { this->onUserMuteAudio_node(uid, muted); });
}

void NodeEventHandler::onUserMuteVideo_node(uid_t uid, bool muted) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_MUTE_VIDEO, uid, uid, bool, muted);
}

void NodeEventHandler::onUserMuteVideo(uid_t uid, bool muted) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, muted] { this->onUserMuteVideo_node(uid, muted); });
}

void NodeEventHandler::onUserEnableVideo_node(uid_t uid, bool enabled) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_ENABLE_VIDEO, uid, uid, bool, enabled);
}

void NodeEventHandler::onUserEnableVideo(uid_t uid, bool enabled) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, enabled] { this->onUserEnableVideo_node(uid, enabled); });
}

void NodeEventHandler::onUserEnableLocalVideo_node(uid_t uid, bool enabled) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_USER_ENABLE_LOCAL_VIDEO, uid, uid, bool, enabled);
}

void NodeEventHandler::onUserEnableLocalVideo(uid_t uid, bool enabled) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, enabled] {
    this->onUserEnableLocalVideo_node(uid, enabled);
  });
}

void NodeEventHandler::onApiCallExecuted_node(const char* api, int error) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_APICALL_EXECUTED, string, api, int32, error);
}

void NodeEventHandler::onApiCallExecuted(int err,
                                         const char* api,
                                         const char* result) {
  FUNC_TRACE;
  std::string apiName(api);
  node_async_call::async_call([this, apiName, err] {
    this->onApiCallExecuted_node(apiName.c_str(), err);
  });
}

void NodeEventHandler::onLocalVideoStats_node(const LocalVideoStats& stats) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);

    NODE_SET_OBJ_PROP_UINT32(obj, "sentBitrate", stats.sentBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "sentFrameRate", stats.sentFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "targetBitrate", stats.targetBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "targetFrameRate", stats.targetFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "encoderOutputFrameRate",
                             stats.encoderOutputFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rendererOutputFrameRate",
                             stats.rendererOutputFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "qualityAdaptIndication",
                             stats.qualityAdaptIndication);
    NODE_SET_OBJ_PROP_UINT32(obj, "encodedBitrate", stats.encodedBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameWidth", stats.encodedFrameWidth);
    NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameHeight",
                             stats.encodedFrameHeight);
    NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameCount", stats.encodedFrameCount);
    NODE_SET_OBJ_PROP_UINT32(obj, "codecType", stats.codecType);
    NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "captureFrameRate", stats.captureFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "captureBrightnessLevel",
                             (int)stats.captureBrightnessLevel);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_LOCAL_VIDEO_STATS);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onLocalVideoStats(const LocalVideoStats& stats) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, stats] { this->onLocalVideoStats_node(stats); });
}

void NodeEventHandler::onRemoteVideoStats_node(const RemoteVideoStats& stats) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);
    NODE_SET_OBJ_PROP_UID(obj, "uid", stats.uid);
    NODE_SET_OBJ_PROP_UINT32(obj, "delay", stats.delay);
    NODE_SET_OBJ_PROP_UINT32(obj, "width", stats.width);
    NODE_SET_OBJ_PROP_UINT32(obj, "height", stats.height);
    NODE_SET_OBJ_PROP_UINT32(obj, "receivedBitrate", stats.receivedBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "decoderOutputFrameRate",
                             stats.decoderOutputFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rendererOutputFrameRate",
                             stats.rendererOutputFrameRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "rxStreamType", stats.rxStreamType);
    NODE_SET_OBJ_PROP_UINT32(obj, "totalFrozenTime", stats.totalFrozenTime);
    NODE_SET_OBJ_PROP_UINT32(obj, "frozenRate", stats.frozenRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "packetLossRate", stats.packetLossRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "totalActiveTime", stats.totalActiveTime);
    NODE_SET_OBJ_PROP_UINT32(obj, "publishDuration", stats.publishDuration);
    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_REMOTE_VIDEO_STATS);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onRemoteVideoStats(const RemoteVideoStats& stats) {
  FUNC_TRACE;
  printf("frame rate : %d, bitrate : %d, width %d, height %d\n",
         stats.rendererOutputFrameRate, stats.receivedBitrate, stats.width,
         stats.height);
  node_async_call::async_call(
      [this, stats] { this->onRemoteVideoStats_node(stats); });
}

void NodeEventHandler::onCameraReady_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_CAMERA_READY);
}

void NodeEventHandler::onCameraReady() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onCameraReady_node(); });
}

void NodeEventHandler::onCameraFocusAreaChanged_node(int x,
                                                     int y,
                                                     int width,
                                                     int height) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_CAMERA_FOCUS_AREA_CHANGED, int32, x, int32, y, int32,
                 width, int32, height);
}

void NodeEventHandler::onCameraFocusAreaChanged(int x,
                                                int y,
                                                int width,
                                                int height) {
  FUNC_TRACE;
  node_async_call::async_call([this, x, y, width, height] {
    this->onCameraFocusAreaChanged_node(x, y, width, height);
  });
}

void NodeEventHandler::onCameraExposureAreaChanged_node(int x,
                                                        int y,
                                                        int width,
                                                        int height) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_CAMERA_FOCUS_AREA_CHANGED, int32, x, int32, y, int32,
                 width, int32, height);
}

void NodeEventHandler::onCameraExposureAreaChanged(int x,
                                                   int y,
                                                   int width,
                                                   int height) {
  FUNC_TRACE;
  node_async_call::async_call([this, x, y, width, height] {
    this->onCameraExposureAreaChanged_node(x, y, width, height);
  });
}

void NodeEventHandler::onVideoStopped_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_VIDEO_STOPPED);
}

void NodeEventHandler::onVideoStopped() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onVideoStopped_node(); });
}

void NodeEventHandler::onConnectionLost_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_CONNECTION_LOST);
}

void NodeEventHandler::onConnectionLost() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onConnectionLost_node(); });
}

void NodeEventHandler::onConnectionInterrupted_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_CONNECTION_INTERRUPTED);
}

void NodeEventHandler::onConnectionInterrupted() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onConnectionInterrupted_node(); });
}

void NodeEventHandler::onConnectionBanned_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_CONNECTION_BANNED);
}

void NodeEventHandler::onConnectionBanned() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onConnectionBanned_node(); });
}

void NodeEventHandler::onStreamMessage_node(uid_t uid,
                                            int streamId,
                                            const char* data,
                                            size_t length) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_STREAM_MESSAGE, uid, uid, int32, streamId, string,
                 data, int32, length);
}

void NodeEventHandler::onStreamMessage(uid_t uid,
                                       int streamId,
                                       const char* data,
                                       size_t length) {
  FUNC_TRACE;
  std::string msg(data);
  node_async_call::async_call([this, uid, streamId, msg, length] {
    this->onStreamMessage_node(uid, streamId, msg.c_str(), length);
  });
}

void NodeEventHandler::onStreamMessageError_node(uid_t uid,
                                                 int streamId,
                                                 int code,
                                                 int missed,
                                                 int cached) {
  FUNC_TRACE;
  MAKE_JS_CALL_5(RTC_EVENT_STREAM_MESSAGE_ERROR, uid, uid, int32, streamId,
                 int32, code, int32, missed, int32, cached);
}

void NodeEventHandler::onStreamMessageError(uid_t uid,
                                            int streamId,
                                            int code,
                                            int missed,
                                            int cached) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, streamId, code, missed, cached] {
    this->onStreamMessageError_node(uid, streamId, code, missed, cached);
  });
}

void NodeEventHandler::onMediaEngineLoadSuccess_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_MEDIA_ENGINE_LOAD_SUCCESS);
}

void NodeEventHandler::onMediaEngineLoadSuccess() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onMediaEngineLoadSuccess_node(); });
}

void NodeEventHandler::onMediaEngineStartCallSuccess_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_MEDIA_ENGINE_STARTCALL_SUCCESS);
}

void NodeEventHandler::onMediaEngineStartCallSuccess() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onMediaEngineStartCallSuccess_node(); });
}

void NodeEventHandler::onRequestToken_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_REQUEST_TOKEN);
}

void NodeEventHandler::onRequestToken() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onRequestToken_node(); });
}

void NodeEventHandler::onTokenPrivilegeWillExpire_node(const char* token) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_TOKEN_PRIVILEGE_WILL_EXPIRE, string, token);
}

void NodeEventHandler::onTokenPrivilegeWillExpire(const char* token) {
  FUNC_TRACE;
  std::string sToken(token);
  node_async_call::async_call([this, sToken] {
    this->onTokenPrivilegeWillExpire_node(sToken.c_str());
  });
}

void NodeEventHandler::onFirstLocalAudioFrame_node(int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_FIRST_LOCAL_AUDIO_FRAME, int32, elapsed);
}

void NodeEventHandler::onFirstLocalAudioFrame(int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, elapsed] { this->onFirstLocalAudioFrame_node(elapsed); });
}

void NodeEventHandler::onFirstRemoteAudioFrame_node(uid_t uid, int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_FIRST_REMOTE_AUDIO_FRAME, uid, uid, int32, elapsed);
}

void NodeEventHandler::onFirstRemoteAudioDecoded_node(uid_t uid, int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_FIRST_REMOTE_AUDIO_DECODED, uid, uid, int32,
                 elapsed);
}

void NodeEventHandler::onFirstRemoteAudioDecoded(uid_t uid, int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, elapsed] {
    this->onFirstRemoteAudioDecoded_node(uid, elapsed);
  });
}

void NodeEventHandler::onStreamPublished_node(const char* url, int error) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_STREAM_PUBLISHED, string, url, int32, error);
}

void NodeEventHandler::onStreamPublished(const char* url, int error) {
  std::string mUrl = std::string(url);
  node_async_call::async_call([this, mUrl, error] {
    this->onStreamPublished_node(mUrl.c_str(), error);
  });
}

void NodeEventHandler::onStreamUnpublished(const char* url) {
  std::string mUrl = std::string(url);
  node_async_call::async_call(
      [this, mUrl] { this->onStreamUnpublished_node(mUrl.c_str()); });
}

void NodeEventHandler::onStreamUnpublished_node(const char* url) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_STREAM_UNPUBLISHED, string, url);
}

void NodeEventHandler::onTranscodingUpdated_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_TRANSCODING_UPDATED);
}

void NodeEventHandler::onTranscodingUpdated() {
  FUNC_TRACE;
  node_async_call::async_call([this] { this->onTranscodingUpdated_node(); });
}

void NodeEventHandler::onStreamInjectedStatus_node(const char* url,
                                                   uid_t uid,
                                                   int status) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_STREAM_INJECT_STATUS, string, url, uid, uid, int32,
                 status);
}

void NodeEventHandler::onStreamInjectedStatus(const char* url,
                                              uid_t uid,
                                              int status) {
  FUNC_TRACE;
  node_async_call::async_call([this, url, uid, status] {
    this->onStreamInjectedStatus_node(url, uid, status);
  });
}

void NodeEventHandler::onLocalPublishFallbackToAudioOnly_node(
    bool isFallbackOrRecover) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_LOCAL_PUBLISH_FALLBACK_TO_AUDIO_ONLY, bool,
                 isFallbackOrRecover);
}

void NodeEventHandler::onLocalPublishFallbackToAudioOnly(
    bool isFallbackOrRecover) {
  FUNC_TRACE;
  node_async_call::async_call([this, isFallbackOrRecover] {
    this->onLocalPublishFallbackToAudioOnly_node(isFallbackOrRecover);
  });
}

void NodeEventHandler::onRemoteSubscribeFallbackToAudioOnly_node(
    uid_t uid,
    bool isFallbackOrRecover) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_REMOTE_SUBSCRIBE_FALLBACK_TO_AUDIO_ONLY, uid, uid,
                 bool, isFallbackOrRecover);
}

void NodeEventHandler::onRemoteSubscribeFallbackToAudioOnly(
    uid_t uid,
    bool isFallbackOrRecover) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, isFallbackOrRecover] {
    this->onRemoteSubscribeFallbackToAudioOnly_node(uid, isFallbackOrRecover);
  });
}

void NodeEventHandler::onActiveSpeaker_node(uid_t uid) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_ACTIVE_SPEAKER, uid, uid);
}

void NodeEventHandler::onActiveSpeaker(uid_t uid) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid] { this->onActiveSpeaker_node(uid); });
}

void NodeEventHandler::onClientRoleChanged_node(CLIENT_ROLE_TYPE oldRole,
                                                CLIENT_ROLE_TYPE newRole) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_CLIENT_ROLE_CHANGED, int32, oldRole, int32, newRole);
}

void NodeEventHandler::onClientRoleChanged(CLIENT_ROLE_TYPE oldRole,
                                           CLIENT_ROLE_TYPE newRole) {
  FUNC_TRACE;
  node_async_call::async_call([this, oldRole, newRole] {
    this->onClientRoleChanged_node(oldRole, newRole);
  });
}

void NodeEventHandler::onAudioDeviceVolumeChanged_node(
    MEDIA_DEVICE_TYPE deviceType,
    int volume,
    bool muted) {
  FUNC_TRACE;
  MAKE_JS_CALL_3(RTC_EVENT_AUDIO_DEVICE_VOLUME_CHANGED, int32, deviceType,
                 int32, volume, bool, muted);
}

void NodeEventHandler::onAudioDeviceVolumeChanged(MEDIA_DEVICE_TYPE deviceType,
                                                  int volume,
                                                  bool muted) {
  FUNC_TRACE;
  node_async_call::async_call([this, deviceType, volume, muted] {
    this->onAudioDeviceVolumeChanged_node(deviceType, volume, muted);
  });
}

void NodeEventHandler::onRemoteAudioTransportStats_node(
    agora::rtc::uid_t uid,
    unsigned short delay,
    unsigned short lost,
    unsigned short rxKBitRate) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_REMOTE_AUDIO_TRANSPORT_STATS, uid, uid, uint16,
                 delay, uint16, lost, uint16, rxKBitRate);
}

void NodeEventHandler::onRemoteVideoTransportStats_node(
    agora::rtc::uid_t uid,
    unsigned short delay,
    unsigned short lost,
    unsigned short rxKBitRate) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_REMOTE_VIDEO_TRANSPORT_STATS, uid, uid, uint16,
                 delay, uint16, lost, uint16, rxKBitRate);
}

void NodeEventHandler::onRemoteAudioStats_node(const RemoteAudioStats& stats) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);
    NODE_SET_OBJ_PROP_UID(obj, "uid", stats.uid);
    NODE_SET_OBJ_PROP_UINT32(obj, "quality", stats.quality);
    NODE_SET_OBJ_PROP_UINT32(obj, "networkTransportDelay",
                             stats.networkTransportDelay);
    NODE_SET_OBJ_PROP_UINT32(obj, "jitterBufferDelay", stats.jitterBufferDelay);
    NODE_SET_OBJ_PROP_UINT32(obj, "audioLossRate", stats.audioLossRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "numChannels", stats.numChannels);
    NODE_SET_OBJ_PROP_UINT32(obj, "receivedSampleRate",
                             stats.receivedSampleRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "receivedBitrate", stats.receivedBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "totalFrozenTime", stats.totalFrozenTime);
    NODE_SET_OBJ_PROP_UINT32(obj, "frozenRate", stats.frozenRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "totalActiveTime", stats.totalActiveTime);
    NODE_SET_OBJ_PROP_UINT32(obj, "publishDuration", stats.publishDuration);
    NODE_SET_OBJ_PROP_UINT32(obj, "qoeQuality", stats.qoeQuality);
    NODE_SET_OBJ_PROP_UINT32(obj, "qualityChangedReason",
                             stats.qualityChangedReason);
    NODE_SET_OBJ_PROP_UINT32(obj, "mosValue", stats.mosValue);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_REMOTE_AUDIO_STATS);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onRemoteAudioTransportStats(agora::rtc::uid_t uid,
                                                   unsigned short delay,
                                                   unsigned short lost,
                                                   unsigned short rxKBitRate) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, delay, lost, rxKBitRate] {
    this->onRemoteAudioTransportStats_node(uid, delay, lost, rxKBitRate);
  });
}

void NodeEventHandler::onRemoteVideoTransportStats(agora::rtc::uid_t uid,
                                                   unsigned short delay,
                                                   unsigned short lost,
                                                   unsigned short rxKBitRate) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, delay, lost, rxKBitRate] {
    this->onRemoteVideoTransportStats_node(uid, delay, lost, rxKBitRate);
  });
}

void NodeEventHandler::onRemoteAudioStats(const RemoteAudioStats& stats) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, stats] { this->onRemoteAudioStats_node(stats); });
}

void NodeEventHandler::onMicrophoneEnabled_node(bool enabled) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_MICROPHONE_ENABLED, bool, enabled);
}

void NodeEventHandler::onMicrophoneEnabled(bool enabled) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, enabled] { this->onMicrophoneEnabled_node(enabled); });
}

void NodeEventHandler::onConnectionStateChanged_node(
    CONNECTION_STATE_TYPE state,
    CONNECTION_CHANGED_REASON_TYPE reason) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_CONNECTION_STATE_CHANED, int32, state, int32,
                 reason);
}

void NodeEventHandler::onConnectionStateChanged(
    CONNECTION_STATE_TYPE state,
    CONNECTION_CHANGED_REASON_TYPE reason) {
  FUNC_TRACE;
  node_async_call::async_call([this, state, reason] {
    this->onConnectionStateChanged_node(state, reason);
  });
}

void NodeEventHandler::onNetworkTypeChanged_node(NETWORK_TYPE type) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_NETWORK_TYPE_CHANGED, int32, type);
}

void NodeEventHandler::onNetworkTypeChanged(NETWORK_TYPE type) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, type] { this->onNetworkTypeChanged_node(type); });
}

void NodeEventHandler::onVideoSourceJoinedChannel(agora::rtc::uid_t uid) {
  FUNC_TRACE;
  m_engine->getRtcEngine()->muteRemoteVideoStream(uid, true);
  node_async_call::async_call(
      [this, uid] { this->onVideoSourceJoinedChannel_node(uid); });
}

void NodeEventHandler::onVideoSourceJoinedChannel_node(agora::rtc::uid_t uid) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_VIDEO_SOURCE_JOIN_SUCCESS, uid, uid);
}

void NodeEventHandler::onVideoSourceRequestNewToken() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onVideoSourceRequestToken_node(); });
}

void NodeEventHandler::onVideoSourceRequestToken_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_VIDEO_SOURCE_REQUEST_NEW_TOKEN);
}

void NodeEventHandler::onVideoSourceExit() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onVideoSourceLeaveChannel_node(); });
  m_engine->destroyVideoSource();
}

void NodeEventHandler::onVideoSourceLeaveChannel() {
  FUNC_TRACE;
  node_async_call::async_call(
      [this] { this->onVideoSourceLeaveChannel_node(); });
}

void NodeEventHandler::onVideoSourceLeaveChannel_node() {
  FUNC_TRACE;
  MAKE_JS_CALL_0(RTC_EVENT_VIDEO_SOURCE_LEAVE_CHANNEL);
}

void NodeEventHandler::onVideoSourceLocalAudioStats(
    const LocalAudioStats& stats) {
  node_async_call::async_call([this, stats] {
    do {
      Isolate* isolate = Isolate::GetCurrent();
      HandleScope scope(isolate);
      Local<Context> context = isolate->GetCurrentContext();
      Local<Object> obj = Object::New(isolate);
      CHECK_NAPI_OBJ(obj);

      NODE_SET_OBJ_PROP_UINT32(obj, "numChannels", stats.numChannels);
      NODE_SET_OBJ_PROP_UINT32(obj, "sentSampleRate", stats.sentSampleRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "sentBitrate", stats.sentBitrate);
      NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);

      Local<Value> arg[1] = {obj};
      auto it = m_callbacks.find(RTC_EVENT_LOCAL_AUDIO_STATS);
      if (it != m_callbacks.end()) {
        it->second->callback.Get(isolate)->Call(
            context, it->second->js_this.Get(isolate), 1, arg);
      }
    } while (false);
  });
}

void NodeEventHandler::onVideoSourceVideoSizeChanged(uid_t uid,
                                                     int width,
                                                     int height,
                                                     int rotation) {
  node_async_call::async_call([this, uid, width, height, rotation] {
    MAKE_JS_CALL_4(RTC_EVENT_VIDEO_SOURCE_VIDEO_SIZE_CHANGED, uid, uid, int32,
                   width, int32, height, int32, rotation);
  });
}

void NodeEventHandler::onVideoSourceLocalVideoStats(
    const LocalVideoStats& stats) {
  node_async_call::async_call([this, stats] {
    do {
      Isolate* isolate = Isolate::GetCurrent();
      HandleScope scope(isolate);
      Local<Context> context = isolate->GetCurrentContext();
      Local<Object> obj = Object::New(isolate);
      CHECK_NAPI_OBJ(obj);

      NODE_SET_OBJ_PROP_UINT32(obj, "sentBitrate", stats.sentBitrate);
      NODE_SET_OBJ_PROP_UINT32(obj, "sentFrameRate", stats.sentFrameRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "targetBitrate", stats.targetBitrate);
      NODE_SET_OBJ_PROP_UINT32(obj, "targetFrameRate", stats.targetFrameRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "encoderOutputFrameRate",
                               stats.encoderOutputFrameRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "rendererOutputFrameRate",
                               stats.rendererOutputFrameRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "qualityAdaptIndication",
                               stats.qualityAdaptIndication);
      NODE_SET_OBJ_PROP_UINT32(obj, "encodedBitrate", stats.encodedBitrate);
      NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameWidth",
                               stats.encodedFrameWidth);
      NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameHeight",
                               stats.encodedFrameHeight);
      NODE_SET_OBJ_PROP_UINT32(obj, "encodedFrameCount",
                               stats.encodedFrameCount);
      NODE_SET_OBJ_PROP_UINT32(obj, "codecType", stats.codecType);
      NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "captureFrameRate", stats.captureFrameRate);
      NODE_SET_OBJ_PROP_UINT32(obj, "captureBrightnessLevel",
                               (int)stats.captureBrightnessLevel);

      Local<Value> arg[1] = {obj};
      auto it = m_callbacks.find(RTC_EVENT_VIDEO_SOURCE_LOCAL_VIDEO_STATS);
      if (it != m_callbacks.end()) {
        it->second->callback.Get(isolate)->Call(
            context, it->second->js_this.Get(isolate), 1, arg);
      }
    } while (false);
  });
}

void NodeEventHandler::onVideoSourceLocalAudioStateChanged(int state,
                                                           int error) {
  node_async_call::async_call([this, state, error] {
    MAKE_JS_CALL_2(RTC_EVENT_VIDEO_SOURCE_LOCAL_AUDIO_STATE_CHANGED, int32,
                   state, int32, error);
  });
}

void NodeEventHandler::onVideoSourceLocalVideoStateChanged(int state,
                                                           int error) {
  node_async_call::async_call([this, state, error] {
    MAKE_JS_CALL_2(RTC_EVENT_VIDEO_SOURCE_LOCAL_VIDEO_STATE_CHANGED, int32,
                   state, int32, error);
  });
}

void NodeEventHandler::addEventHandler(const std::string& eventName,
                                       Persistent<Object>& obj,
                                       Persistent<Function>& callback) {
  FUNC_TRACE;
  NodeEventCallback* cb = new NodeEventCallback();
  ;
  cb->js_this.Reset(obj);
  cb->callback.Reset(callback);
  m_callbacks.emplace(eventName, cb);
}

void NodeEventHandler::fireApiError(const char* funcName) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_API_ERROR, string, funcName);
}

void NodeEventHandler::onAudioMixingStateChanged(
    AUDIO_MIXING_STATE_TYPE state,
    AUDIO_MIXING_REASON_TYPE errorCode) {
  FUNC_TRACE;
  node_async_call::async_call([this, state, errorCode] {
    this->onAudioMixingStateChanged_node(state, errorCode);
  });
}

void NodeEventHandler::onAudioMixingStateChanged_node(
    AUDIO_MIXING_STATE_TYPE state,
    AUDIO_MIXING_REASON_TYPE errorCode) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_AUDIO_MIXING_STATE_CHANGED, int32, state, int32,
                 errorCode);
}

void NodeEventHandler::onLastmileProbeResult(
    const LastmileProbeResult& result) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, result] { this->onLastmileProbeResult_node(result); });
}

void NodeEventHandler::onLastmileProbeResult_node(
    const LastmileProbeResult& result) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);
    NODE_SET_OBJ_PROP_UINT32(obj, "state", result.state);
    NODE_SET_OBJ_PROP_UINT32(obj, "rtt", result.rtt);

    Local<Object> uplink = Object::New(isolate);
    CHECK_NAPI_OBJ(uplink);
    NODE_SET_OBJ_PROP_UINT32(uplink, "packetLossRate",
                             result.uplinkReport.packetLossRate);
    NODE_SET_OBJ_PROP_UINT32(uplink, "jitter", result.uplinkReport.jitter);
    NODE_SET_OBJ_PROP_UINT32(uplink, "availableBandwidth",
                             result.uplinkReport.availableBandwidth);

    Local<Object> downlink = Object::New(isolate);
    CHECK_NAPI_OBJ(downlink);
    NODE_SET_OBJ_PROP_UINT32(downlink, "packetLossRate",
                             result.downlinkReport.packetLossRate);
    NODE_SET_OBJ_PROP_UINT32(downlink, "jitter", result.downlinkReport.jitter);
    NODE_SET_OBJ_PROP_UINT32(downlink, "availableBandwidth",
                             result.downlinkReport.availableBandwidth);

    obj->Set(isolate->GetCurrentContext(),
             napi_create_string_(isolate, "uplinkReport"), uplink);
    obj->Set(isolate->GetCurrentContext(),
             napi_create_string_(isolate, "downlinkReport"), downlink);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_LASTMILE_PROBE_RESULT);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

/**
 * 2.8.0
 */
void NodeEventHandler::onLocalUserRegistered(uid_t uid,
                                             const char* userAccount) {
  FUNC_TRACE;
  std::string mUserAccount = std::string(userAccount);
  node_async_call::async_call([this, uid, mUserAccount] {
    this->onLocalUserRegistered_node(uid, mUserAccount.c_str());
  });
}
void NodeEventHandler::onLocalUserRegistered_node(uid_t uid,
                                                  const char* userAccount) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_LOCAL_USER_REGISTERED, uid, uid, string,
                 userAccount);
}

void NodeEventHandler::onUserInfoUpdated(uid_t uid, const UserInfo& info) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, uid, info] { this->onUserInfoUpdated_node(uid, info); });
}
void NodeEventHandler::onUserInfoUpdated_node(uid_t uid, const UserInfo& info) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);

    NODE_SET_OBJ_PROP_UID(obj, "uid", info.uid);
    NODE_SET_OBJ_PROP_STRING(obj, "userAccount", info.userAccount);

    Local<Value> arg[2] = {napi_create_uid_(isolate, uid), obj};
    auto it = m_callbacks.find(RTC_EVENT_USER_INFO_UPDATED);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 2, arg);
    }
  } while (false);
}

void NodeEventHandler::onLocalVideoStateChanged(
    LOCAL_VIDEO_STREAM_STATE localVideoState,
    LOCAL_VIDEO_STREAM_ERROR error) {
  FUNC_TRACE;
  node_async_call::async_call([this, localVideoState, error] {
    this->onLocalVideoStateChanged_node(localVideoState, error);
  });
}

void NodeEventHandler::onLocalVideoStateChanged_node(
    LOCAL_VIDEO_STREAM_STATE localVideoState,
    LOCAL_VIDEO_STREAM_ERROR error) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_LOCAL_VIDEO_STATE_CHANGED, int32, localVideoState,
                 int32, error);
}

void NodeEventHandler::onLocalAudioStats_node(const LocalAudioStats& stats) {
  FUNC_TRACE;
  do {
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = Object::New(isolate);
    CHECK_NAPI_OBJ(obj);

    NODE_SET_OBJ_PROP_UINT32(obj, "numChannels", stats.numChannels);
    NODE_SET_OBJ_PROP_UINT32(obj, "sentSampleRate", stats.sentSampleRate);
    NODE_SET_OBJ_PROP_UINT32(obj, "sentBitrate", stats.sentBitrate);
    NODE_SET_OBJ_PROP_UINT32(obj, "txPacketLossRate", stats.txPacketLossRate);

    Local<Value> arg[1] = {obj};
    auto it = m_callbacks.find(RTC_EVENT_LOCAL_AUDIO_STATS);
    if (it != m_callbacks.end()) {
      it->second->callback.Get(isolate)->Call(
          context, it->second->js_this.Get(isolate), 1, arg);
    }
  } while (false);
}

void NodeEventHandler::onLocalAudioStats(const LocalAudioStats& stats) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, stats] { this->onLocalAudioStats_node(stats); });
}

void NodeEventHandler::onLocalAudioStateChanged(
    LOCAL_AUDIO_STREAM_STATE state,
    LOCAL_AUDIO_STREAM_ERROR error) {
  FUNC_TRACE;
  node_async_call::async_call([this, state, error] {
    this->onLocalAudioStateChanged_node(state, error);
  });
}

void NodeEventHandler::onLocalAudioStateChanged_node(
    LOCAL_AUDIO_STREAM_STATE state,
    LOCAL_AUDIO_STREAM_ERROR error) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_LOCAL_AUDIO_STATE_CHANGED, int32, state, int32,
                 error);
}

void NodeEventHandler::onRemoteAudioStateChanged(
    uid_t uid,
    REMOTE_AUDIO_STATE state,
    REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, uid, state, reason, elapsed] {
    this->onRemoteAudioStateChanged_node(uid, state, reason, elapsed);
  });
}

void NodeEventHandler::onRemoteAudioStateChanged_node(
    uid_t uid,
    REMOTE_AUDIO_STATE state,
    REMOTE_AUDIO_STATE_REASON reason,
    int elapsed) {
  FUNC_TRACE;
  MAKE_JS_CALL_4(RTC_EVENT_REMOTE_AUDIO_STATE_CHANGED, uid, uid, int32, state,
                 int32, reason, int32, elapsed);
}

void NodeEventHandler::onChannelMediaRelayEvent(
    CHANNEL_MEDIA_RELAY_EVENT code) {
  FUNC_TRACE;
  node_async_call::async_call(
      [this, code] { this->onChannelMediaRelayEvent_node(code); });
}

void NodeEventHandler::onChannelMediaRelayEvent_node(
    CHANNEL_MEDIA_RELAY_EVENT code) {
  FUNC_TRACE;
  MAKE_JS_CALL_1(RTC_EVENT_CHANNEL_MEDIA_RELAY_EVENT, int32, code);
}

void NodeEventHandler::onChannelMediaRelayStateChanged(
    CHANNEL_MEDIA_RELAY_STATE state,
    CHANNEL_MEDIA_RELAY_ERROR code) {
  FUNC_TRACE;
  node_async_call::async_call([this, state, code] {
    this->onChannelMediaRelayStateChanged_node(state, code);
  });
}

void NodeEventHandler::onChannelMediaRelayStateChanged_node(
    CHANNEL_MEDIA_RELAY_STATE state,
    CHANNEL_MEDIA_RELAY_ERROR code) {
  FUNC_TRACE;
  MAKE_JS_CALL_2(RTC_EVENT_CHANNEL_MEDIA_RELAY_STATE, int32, state, int32,
                 code);
}

void NodeEventHandler::onRtmpStreamingStateChanged(
    const char *url, agora::rtc::RTMP_STREAM_PUBLISH_STATE state,
    agora::rtc::RTMP_STREAM_PUBLISH_ERROR_TYPE errCode) {
  FUNC_TRACE;
  std::string sUrl(url);
  node_async_call::async_call([this, sUrl, state, errCode] {
    MAKE_JS_CALL_3(RTC_EVENT_RTMP_STREAMING_STATE_CHANGED, string, sUrl.c_str(),
                   int32, state, int32, errCode)
  });
}

void NodeEventHandler::onFirstLocalAudioFramePublished(int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, elapsed] {
    MAKE_JS_CALL_1(RTC_EVENT_FIRST_LOCAL_AUDIO_FRAME_PUBLISH, int32, elapsed);
  });
}

void NodeEventHandler::onFirstLocalVideoFramePublished(int elapsed) {
  FUNC_TRACE;
  node_async_call::async_call([this, elapsed] {
    MAKE_JS_CALL_1(RTC_EVENT_FIRST_LOCAL_VIDEO_FRAME_PUBLISH, int32, elapsed);
  });
}

void NodeEventHandler::onRtmpStreamingEvent(const char* url,
                                            RTMP_STREAMING_EVENT eventCode) {
  FUNC_TRACE;
  std::string mUrl(url);
  node_async_call::async_call([this, mUrl, eventCode] {
    MAKE_JS_CALL_2(RTC_EVENT_RTMP_STREAMING_EVENT, string, mUrl.c_str(), int32,
                   (int)eventCode);
  });
}

void NodeEventHandler::onAudioPublishStateChanged(const char* channel,
                                                  STREAM_PUBLISH_STATE oldState,
                                                  STREAM_PUBLISH_STATE newState,
                                                  int elapseSinceLastState) {
  FUNC_TRACE;
  std::string mChannel(channel);
  node_async_call::async_call(
      [this, mChannel, oldState, newState, elapseSinceLastState] {
        MAKE_JS_CALL_4(RTC_EVENT_AUDIO_PUBLISH_STATE_CHANGED, string,
                       mChannel.c_str(), int32, (int)oldState, int32,
                       (int)newState, int32, elapseSinceLastState);
      });
}

void NodeEventHandler::onVideoPublishStateChanged(const char* channel,
                                                  STREAM_PUBLISH_STATE oldState,
                                                  STREAM_PUBLISH_STATE newState,
                                                  int elapseSinceLastState) {
  FUNC_TRACE;
  std::string mChannel(channel);
  node_async_call::async_call(
      [this, mChannel, oldState, newState, elapseSinceLastState] {
        MAKE_JS_CALL_4(RTC_EVENT_VIDEO_PUBLISH_STATE_CHANGED, string,
                       mChannel.c_str(), int32, oldState, int32, newState,
                       int32, elapseSinceLastState);
      });
}

void NodeEventHandler::onAudioSubscribeStateChanged(
    const char* channel,
    uid_t uid,
    STREAM_SUBSCRIBE_STATE oldState,
    STREAM_SUBSCRIBE_STATE newState,
    int elapseSinceLastState) {
  FUNC_TRACE;
  std::string mChannel(channel);
  node_async_call::async_call(
      [this, mChannel, uid, oldState, newState, elapseSinceLastState] {
        MAKE_JS_CALL_5(RTC_EVENT_AUDIO_SUBSCRIBE_STATE_CHANGED, string,
                       mChannel.c_str(), uid, uid, int32, oldState, int32,
                       newState, int32, elapseSinceLastState);
      });
}

void NodeEventHandler::onVideoSubscribeStateChanged(
    const char* channel,
    uid_t uid,
    STREAM_SUBSCRIBE_STATE oldState,
    STREAM_SUBSCRIBE_STATE newState,
    int elapseSinceLastState) {
  FUNC_TRACE;
  std::string mChannel(channel);
  node_async_call::async_call(
      [this, mChannel, uid, oldState, newState, elapseSinceLastState] {
        MAKE_JS_CALL_5(RTC_EVENT_VIDEO_SUBSCRIBE_STATE_CHANGED, string,
                       mChannel.c_str(), uid, uid, int32, oldState, int32,
                       newState, int32, elapseSinceLastState);
      });
}

void NodeEventHandler::onAudioRouteChanged(AUDIO_ROUTE_TYPE routing) {
  FUNC_TRACE;
  node_async_call::async_call([this, routing] {
    MAKE_JS_CALL_1(RTC_EVENT_AUDIO_ROUTE_CHANGED, int32, (int)routing);
  });
}

// 3.3.0 callbacks
void NodeEventHandler::onUploadLogResult(const char* requestId,
                                         bool success,
                                         UPLOAD_ERROR_REASON reason) {
  FUNC_TRACE;

  std::string mRequestId(requestId);
  node_async_call::async_call([this, mRequestId, success, reason] {
    MAKE_JS_CALL_3(RTC_EVENT_UPLOAD_LOG_RESULT, string, mRequestId.c_str(),
                   bool, success, int32, reason);
  });
}
// 3.4.5
void NodeEventHandler::onVirtualBackgroundSourceEnabled(
    bool enabled,
    VIRTUAL_BACKGROUND_SOURCE_STATE_REASON reason) {
  FUNC_TRACE;
  ;
  node_async_call::async_call([this, enabled, reason] {
    MAKE_JS_CALL_2(RTC_EVENT_VIRTUAL_BACKGROUND_SOURCE_ENABLED, bool, enabled,
                   int32, reason);
  });
}
// 3.5.1
void NodeEventHandler::onRequestAudioFileInfo(const agora::rtc::AudioFileInfo& info, AUDIO_FILE_INFO_ERROR error) {
  FUNC_TRACE;
  std::string filePath(info.filePath);
  node_async_call::async_call([this, info, filePath, error] {
      FUNC_TRACE;
      do {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Context> context = isolate->GetCurrentContext();
        Local<Object> obj = Object::New(isolate);
        CHECK_NAPI_OBJ(obj);

        NODE_SET_OBJ_PROP_STRING(obj, "filePath", filePath.c_str());
        NODE_SET_OBJ_PROP_UINT32(obj, "durationMs", info.durationMs);

        Local<Value> arg[2] = {obj, napi_create_int32_(isolate, error)};
        auto it = m_callbacks.find(RTC_EVENT_REQUEST_AUDIO_FILE_INFO);
        if (it != m_callbacks.end()) {
          it->second->callback.Get(isolate)->Call(
              context, it->second->js_this.Get(isolate), 2, arg);
        }
      } while (false);
  });
}
/* meeting */
void NodeEventHandler::onVideoSourceScreenCaptureInfoUpdated(
    ScreenCaptureInfoCmd &info) {
  std::string cardType(info.cardType);
  auto errCode = info.errCode;
  node_async_call::async_call([this, cardType, errCode] {
    do {
      Isolate *isolate = Isolate::GetCurrent();
      HandleScope scope(isolate);
      Local<Context> context = isolate->GetCurrentContext();
      Local<Object> obj = Object::New(isolate);
      CHECK_NAPI_OBJ(obj);

      NODE_SET_OBJ_PROP_STRING(obj, "cardType", cardType.c_str());
      NODE_SET_OBJ_PROP_UINT32(obj, "errCode", errCode);

      Local<Value> arg[1] = {obj};
      auto it =
          m_callbacks.find(RTC_EVENT_VIDEO_SOURCE_SCREEN_CAPTURE_INFO_UPDATED);
      if (it != m_callbacks.end()) {
        it->second->callback.Get(isolate)->Call(
            context, it->second->js_this.Get(isolate), 1, arg);
      }
    } while (false);
  });
}

// 3.5.2
void NodeEventHandler::onSnapshotTaken(const char* channel, uid_t uid, const char* filePath, int width, int height, int errCode) {
  std::string channelIdStr(channel);
  std::string filePathStr(filePath);
  node_async_call::async_call([this, channelIdStr, uid, filePathStr, width, height, errCode] {
    MAKE_JS_CALL_6(RTC_EVENT_SNAPSHOT_TAKEN, string, channelIdStr.c_str(), uid, uid, string, filePathStr.c_str(), int32, width, int32, height, int32, errCode);
  });
}

// 3.6.0.2
void NodeEventHandler::onAudioDeviceTestVolumeIndication(AudioDeviceTestVolumeType volumeType, int volume) {
  node_async_call::async_call([this, volumeType, volume] {
    MAKE_JS_CALL_2(RTC_EVENT_AUDIO_DEVICE_TEST_VOLUME_INDICATION, int32, volumeType, int32, volume);
  });
}

// 3.7.0
void NodeEventHandler::onLocalVoicePitchInHz(int pitchInHz) {

  node_async_call::async_call([this, pitchInHz] {
    MAKE_JS_CALL_1(RTC_EVENT_LOCAL_VOICE_PITCH_IN_HZ, int32, pitchInHz);
  });
}

void NodeEventHandler::onClientRoleChangeFailed(
    CLIENT_ROLE_CHANGE_FAILED_REASON reason, CLIENT_ROLE_TYPE currentRole) {
  node_async_call::async_call([this, reason, currentRole] {
    MAKE_JS_CALL_2(RTC_EVENT_CLIENT_ROLE_CHANGE_FAILED, int32, reason, int32,
                   currentRole);
  });
}
void NodeEventHandler::onWlAccMessage(WLACC_MESSAGE_REASON reason,
                                      WLACC_SUGGEST_ACTION action,
                                      const char *wlAccMsg) {
  std::string msg(wlAccMsg);

  node_async_call::async_call([this, reason, action, msg] {
    MAKE_JS_CALL_3(RTC_EVENT_WL_ACC_MESSAGE, int32, reason, int32, action,
                   string, msg.c_str());
  });
}



void NodeEventHandler::onWlAccStats(WlAccStats currentStats,
                                    WlAccStats averageStats) {
  FUNC_TRACE;
  node_async_call::async_call([this, currentStats, averageStats] {
      FUNC_TRACE;
      do {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Context> context = isolate->GetCurrentContext();
        Local<Object> obj1 = Object::New(isolate);
        Local<Object> obj2 = Object::New(isolate);

        NODE_SET_OBJ_PROP_NUMBER(obj1, "e2eDelayPercent", currentStats.e2eDelayPercent);
        NODE_SET_OBJ_PROP_NUMBER(obj1, "frozenRatioPercent", currentStats.frozenRatioPercent);
        NODE_SET_OBJ_PROP_NUMBER(obj1, "lossRatePercent", currentStats.lossRatePercent);

        NODE_SET_OBJ_PROP_NUMBER(obj2, "e2eDelayPercent", averageStats.e2eDelayPercent);
        NODE_SET_OBJ_PROP_NUMBER(obj2, "frozenRatioPercent", averageStats.frozenRatioPercent);
        NODE_SET_OBJ_PROP_NUMBER(obj2, "lossRatePercent", averageStats.lossRatePercent);

        Local<Value> arg[2] = {obj1,obj2};
        auto it = m_callbacks.find(RTC_EVENT_WL_ACC_STATS);
        if (it != m_callbacks.end()) {
          it->second->callback.Get(isolate)->Call(
              context, it->second->js_this.Get(isolate), 2, arg);
        }
      } while (false);
  });
}

void NodeEventHandler::onContentInspectResult(CONTENT_INSPECT_RESULT result) {

  node_async_call::async_call([this, result] {
    MAKE_JS_CALL_1(RTC_EVENT_CONTENT_INSPECT_RESULT, int32, result);
  });
}
void NodeEventHandler::onProxyConnected(const char *channel, uid_t uid,
                                        PROXY_TYPE proxyType,
                                        const char *localProxyIp, int elapsed) {
  std::string channelIdStr(channel);
  std::string localProxyIpStr(localProxyIp);

  node_async_call::async_call(
      [this, channelIdStr, uid, proxyType, localProxyIpStr, elapsed] {
        MAKE_JS_CALL_5(RTC_EVENT_PROXY_CONNECTED, string, channelIdStr.c_str(),
                       uid, uid, int32, proxyType, string,
                       localProxyIpStr.c_str(), int32, elapsed);
      });
}

void NodeEventHandler::OnError(std::string device_id,
                                       seax::ERR_CODE err_code) {
  FUNC_TRACE;
  node_async_call::async_call([this, device_id, err_code] {
    MAKE_JS_CALL_2(SEAX_EVENT_ON_ERROR, string, device_id.c_str(), int32,
                   static_cast<int>(err_code));
  });
}

void NodeEventHandler::OnStateMsgUpdate(char* state_msg, int len) {
  FUNC_TRACE;
  std::string stateMsg(state_msg, len);
  node_async_call::async_call([this, stateMsg] {
    MAKE_JS_CALL_1(SEAX_EVENT_ON_STATE, string, stateMsg.c_str());
  });
}

/*
 * @brief Triggered after confirmed device role
 * @param device_id : device Id
 * @param role :  0: client;  1: host
 */
void NodeEventHandler::OnDeviceRoleConfirmed(
    const std::string& device_id, uint32_t local_uid, uint32_t host_uid,
    int32_t role) {
  FUNC_TRACE;
  node_async_call::async_call([this, device_id, local_uid, host_uid, role] {
    MAKE_JS_CALL_4(SEAX_EVENT_ON_ROLE_CONFIRMED, string, device_id.c_str(),
                   uint32, local_uid, uint32, host_uid, int32, role);
  });
}

/*
 * @brief Triggered after received device list from server device role
 * @param dev_list : lastest device list
 */
void NodeEventHandler::OnUpdateDevList(
    const std::list<seax::DeviceInfo>& dev_list) {
  node_async_call::async_call([this, dev_list] {
    FUNC_TRACE;

    auto it = m_callbacks.find(SEAX_EVENT_ON_DEVICE_LIST_UPDATED);
    if (it != m_callbacks.end()) {
      Isolate* isolate = Isolate::GetCurrent();
      HandleScope scope(isolate);
      Local<Context> context = isolate->GetCurrentContext();
      Local<v8::Array> arrDevice = v8::Array::New(isolate, dev_list.size());

      int index = 0;
      for (auto& device : dev_list) {
        Local<Object> obj = Object::New(isolate);
        NODE_SET_OBJ_PROP_STRING(obj, "id", device.device_id.c_str());
        NODE_SET_OBJ_PROP_UID(obj, "role", device.device_role);
        NODE_SET_OBJ_PROP_STRING(obj, "channel", device.channel_id.c_str());
        NODE_SET_OBJ_PROP_UINT32(obj, "uid", device.local_uid);
        NODE_SET_OBJ_PROP_UINT32(obj, "hostUid", device.host_uid);

        arrDevice->Set(context, index, obj);
        index++;
      }

      Local<Value> argv[1]{arrDevice};
      NodeEventCallback& cb = *it->second;
      cb.callback.Get(isolate)->Call(context, cb.js_this.Get(isolate), 1, argv);
    }
  });
}

} // namespace rtc
} // namespace agora
