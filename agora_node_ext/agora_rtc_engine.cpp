/*
 * Copyright (c) 2017 Agora.io
 * All rights reserved.
 * Proprietry and Confidential -- Agora.io
 */

/*
 *  Created by Wang Yongli, 2017
 */

#include "agora_rtc_engine.h"
#include <nan.h>
#include <string>
#include "IAgoraRtcEngine2.h"
#include "agora_video_source.h"
#include "node_napi_api.h"
#include "node_uid.h"
#include "node_video_render.h"

#if defined(__APPLE__) || defined(_WIN32)
#include "node_screen_window_info.h"
#endif

#if defined(__APPLE__)
#include <dlfcn.h>
#else
#include <windef.h>
#endif

using std::string;
namespace agora {
namespace rtc {
template <typename T>
agora::rtc::LiveTranscoding *
getLiveTranscoding(Local<Object> &obj,
                   const Nan::FunctionCallbackInfo<Value> &args, T *pEngine) {
  TranscodingUser *users = nullptr;
  RtcImage *wkImages = nullptr;
  RtcImage *bgImages = nullptr;
  LiveStreamAdvancedFeature *liveStreamAdvancedFeature = nullptr;

  std::string key = "";
  LiveTranscoding *transcoding = new LiveTranscoding();
  do {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_status status;

    nodestring extrainfo;
    int videoCodecProfile, audioSampleRateType, videoCodecType,
        audioCodecProfile;

    key = "width";
    nodestring transcodingExtraInfo;
    status =
        napi_get_object_property_int32_(isolate, obj, key, transcoding->width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, transcoding->height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "videoBitrate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             transcoding->videoBitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "videoFrameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             transcoding->videoFramerate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "lowLatency";
    status = napi_get_object_property_bool_(isolate, obj, key,
                                            transcoding->lowLatency);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "videoGop";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             transcoding->videoGop);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "videoCodecProfile";
    status =
        napi_get_object_property_int32_(isolate, obj, key, videoCodecProfile);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    transcoding->videoCodecProfile =
        (VIDEO_CODEC_PROFILE_TYPE)videoCodecProfile;

    key = "videoCodecType";
    status = napi_get_object_property_int32_(isolate, obj, key, videoCodecType);
    transcoding->videoCodecType = (VIDEO_CODEC_TYPE_FOR_STREAM)videoCodecType;

    key = "backgroundColor";
    status = napi_get_object_property_uint32_(isolate, obj, key,
                                              transcoding->backgroundColor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "audioSampleRate";
    status =
        napi_get_object_property_int32_(isolate, obj, key, audioSampleRateType);
    transcoding->audioSampleRate = (AUDIO_SAMPLE_RATE_TYPE)audioSampleRateType;
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "audioCodecProfile";
    status =
        napi_get_object_property_int32_(isolate, obj, key, audioCodecProfile);
    if (status == napi_ok) {
      transcoding->audioCodecProfile =
          (AUDIO_CODEC_PROFILE_TYPE)audioCodecProfile;
    }

    key = "audioBitrate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             transcoding->audioBitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "audioChannels";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             transcoding->audioChannels);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "transcodingExtraInfo";
    status = napi_get_object_property_nodestring_(isolate, obj, key,
                                                  transcodingExtraInfo);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    transcoding->transcodingExtraInfo = transcodingExtraInfo;

    v8::Array *advancedFeaturesArr;
    status = napi_get_object_property_array_(isolate, obj, "advancedFeatures",
                                             advancedFeaturesArr);
    if (status == napi_ok && advancedFeaturesArr->Length() > 0) {
      auto count = advancedFeaturesArr->Length();
      liveStreamAdvancedFeature = new LiveStreamAdvancedFeature[count];
      for (uint32 i = 0; i < advancedFeaturesArr->Length(); i++) {
        Local<Value> value =
            advancedFeaturesArr->Get(context, i).ToLocalChecked();
        Local<Object> advancedFeatureObj;
        status = napi_get_value_object_(isolate, value, advancedFeatureObj);
        if (advancedFeatureObj->IsNullOrUndefined()) {
          status = napi_invalid_arg;
          break;
        }
        key = "featureName";
        NodeString featureName;
        napi_get_object_property_nodestring_(isolate, advancedFeatureObj, key,
                                             featureName);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        if (featureName) {
          int len = strlen(featureName) + 1;
          char *string = new char[len];
          memset(string, 0, len);
          strcpy(string, featureName);
          liveStreamAdvancedFeature[i].featureName = string;
        }

        key = "opened";
        napi_get_object_property_bool_(isolate, advancedFeatureObj, key,
                                       liveStreamAdvancedFeature[i].opened);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
      }
      transcoding->advancedFeatures = liveStreamAdvancedFeature;
      transcoding->advancedFeatureCount = count;
    }

    v8::Array *watermarkArr;
    status = napi_get_object_property_array_(isolate, obj, "watermark",
                                             watermarkArr);
    if (status == napi_ok && watermarkArr->Length() > 0) {
      auto count = watermarkArr->Length();
      wkImages = new RtcImage[count];
      for (uint32 i = 0; i < watermarkArr->Length(); i++) {
        Local<Value> value = watermarkArr->Get(context, i).ToLocalChecked();
        Local<Object> watermarkObj;
        status = napi_get_value_object_(isolate, value, watermarkObj);
        if (watermarkObj->IsNullOrUndefined()) {
          status = napi_invalid_arg;
          break;
        }
        key = "url";
        NodeString urlStr;
        status = napi_get_object_property_nodestring_(isolate, watermarkObj,
                                                      key, urlStr);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        if (urlStr) {
          int len = strlen(urlStr) + 1;
          char *string = new char[len];
          memset(string, 0, len);
          strcpy(string, urlStr);
          wkImages[i].url = string;
        }

        key = "x";
        napi_get_object_property_int32_(isolate, watermarkObj, key,
                                        wkImages[i].x);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "y";
        napi_get_object_property_int32_(isolate, watermarkObj, key,
                                        wkImages[i].y);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "width";
        napi_get_object_property_int32_(isolate, watermarkObj, key,
                                        wkImages[i].width);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "height";
        napi_get_object_property_int32_(isolate, watermarkObj, key,
                                        wkImages[i].height);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "zOrder";
        napi_get_object_property_int32_(isolate, watermarkObj, key,
                                        wkImages[i].zOrder);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "alpha";
        napi_get_object_property_double_(isolate, watermarkObj, key,
                                        wkImages[i].alpha);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
      }
      transcoding->watermark = wkImages;
      transcoding->watermarkCount = count;
    }

    v8::Array *backgroundImageArr;
    status = napi_get_object_property_array_(isolate, obj, "backgroundImage",
                                             backgroundImageArr);
    if (status == napi_ok && backgroundImageArr->Length() > 0) {
      auto count = backgroundImageArr->Length();
      bgImages = new RtcImage[count];
      for (uint32 i = 0; i < backgroundImageArr->Length(); i++) {
        Local<Value> value =
            backgroundImageArr->Get(context, i).ToLocalChecked();
        Local<Object> advancedFeatureObj;
        status = napi_get_value_object_(isolate, value, advancedFeatureObj);
        if (advancedFeatureObj->IsNullOrUndefined()) {
          status = napi_invalid_arg;
          break;
        }
        key = "url";
        NodeString urlStr;
        napi_get_object_property_nodestring_(isolate, advancedFeatureObj, key,
                                             urlStr);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        if (urlStr) {
          int len = strlen(urlStr) + 1;
          char *string = new char[len];
          memset(string, 0, len);
          strcpy(string, urlStr);
          bgImages[i].url = string;
        }

        key = "x";
        napi_get_object_property_int32_(isolate, advancedFeatureObj, key,
                                        bgImages[i].x);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "y";
        napi_get_object_property_int32_(isolate, advancedFeatureObj, key,
                                        bgImages[i].y);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "width";
        napi_get_object_property_int32_(isolate, advancedFeatureObj, key,
                                        bgImages[i].width);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "height";
        napi_get_object_property_int32_(isolate, advancedFeatureObj, key,
                                        bgImages[i].height);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "zOrder";
        napi_get_object_property_int32_(isolate, advancedFeatureObj, key,
                                        bgImages[i].zOrder);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "alpha";
        napi_get_object_property_double_(isolate, advancedFeatureObj, key,
                                        bgImages[i].alpha);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
      }
      transcoding->backgroundImage = bgImages;
      transcoding->backgroundImageCount = count;
    }

    v8::Array *usersValue;
    status = napi_get_object_property_array_(isolate, obj, "transcodingUsers",
                                             usersValue);
    if (status == napi_ok && usersValue->Length() > 0) {
      auto count = usersValue->Length();
      users = new TranscodingUser[count];
      for (uint32 i = 0; i < count; i++) {
        Local<Value> value = usersValue->Get(context, i).ToLocalChecked();
        Local<Object> userObj;
        status = napi_get_value_object_(isolate, value, userObj);
        if (userObj->IsNullOrUndefined()) {
          status = napi_invalid_arg;
          break;
        }
        key = "uid";
        napi_get_object_property_uid_(isolate, userObj, key, users[i].uid);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "x";
        napi_get_object_property_int32_(isolate, userObj, key, users[i].x);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "y";
        napi_get_object_property_int32_(isolate, userObj, key, users[i].y);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "width";
        napi_get_object_property_int32_(isolate, userObj, key, users[i].width);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "height";
        napi_get_object_property_int32_(isolate, userObj, key, users[i].height);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "zOrder";
        napi_get_object_property_int32_(isolate, userObj, key, users[i].zOrder);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "alpha";
        napi_get_object_property_double_(isolate, userObj, key, users[i].alpha);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

        key = "audioChannel";
        napi_get_object_property_int32_(isolate, userObj, key,
                                        users[i].audioChannel);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
      }
      CHECK_NAPI_STATUS(pEngine, status);
      transcoding->transcodingUsers = users;
      transcoding->userCount = count;
    }
    return transcoding;
  } while (false);

  return nullptr;
}

DEFINE_CLASS(NodeRtcEngine);
DEFINE_CLASS(NodeRtcChannel);

/**
 * To declared class and member functions that could be used in JS layer
 * directly.
 */
void NodeRtcEngine::Init(Local<Object>& module) {
  Isolate* isolate = module->GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  BEGIN_PROPERTY_DEFINE(NodeRtcEngine, createInstance, 5)
  PROPERTY_METHOD_DEFINE(onEvent)
  PROPERTY_METHOD_DEFINE(initialize)
  PROPERTY_METHOD_DEFINE(getVersion)
  PROPERTY_METHOD_DEFINE(getErrorDescription)
  PROPERTY_METHOD_DEFINE(joinChannel)
  PROPERTY_METHOD_DEFINE(leaveChannel)
  PROPERTY_METHOD_DEFINE(renewToken)
  PROPERTY_METHOD_DEFINE(setChannelProfile)
  PROPERTY_METHOD_DEFINE(setClientRole)
  PROPERTY_METHOD_DEFINE(startEchoTest)
  PROPERTY_METHOD_DEFINE(stopEchoTest)
  PROPERTY_METHOD_DEFINE(enableLastmileTest)
  PROPERTY_METHOD_DEFINE(disableLastmileTest)
  PROPERTY_METHOD_DEFINE(enableVideo)
  PROPERTY_METHOD_DEFINE(disableVideo)
  PROPERTY_METHOD_DEFINE(startPreview)
  PROPERTY_METHOD_DEFINE(stopPreview)
  PROPERTY_METHOD_DEFINE(setVideoProfile)
  PROPERTY_METHOD_DEFINE(setVideoEncoderConfiguration)
  PROPERTY_METHOD_DEFINE(enableAudio)
  PROPERTY_METHOD_DEFINE(disableAudio)
  PROPERTY_METHOD_DEFINE(setAudioProfile)
  PROPERTY_METHOD_DEFINE(getCallId)
  PROPERTY_METHOD_DEFINE(rate)
  PROPERTY_METHOD_DEFINE(complain)
  PROPERTY_METHOD_DEFINE(setEncryptionSecret)
  PROPERTY_METHOD_DEFINE(createDataStream)
  PROPERTY_METHOD_DEFINE(sendStreamMessage)
  PROPERTY_METHOD_DEFINE(muteLocalAudioStream)
  PROPERTY_METHOD_DEFINE(muteAllRemoteAudioStreams)
  PROPERTY_METHOD_DEFINE(setDefaultMuteAllRemoteAudioStreams)
  PROPERTY_METHOD_DEFINE(muteRemoteAudioStream)
  PROPERTY_METHOD_DEFINE(muteLocalVideoStream)
  PROPERTY_METHOD_DEFINE(enableLocalVideo)
  PROPERTY_METHOD_DEFINE(enableLocalAudio)
  PROPERTY_METHOD_DEFINE(muteAllRemoteVideoStreams)
  PROPERTY_METHOD_DEFINE(setDefaultMuteAllRemoteVideoStreams)
  PROPERTY_METHOD_DEFINE(muteRemoteVideoStream)
  PROPERTY_METHOD_DEFINE(setRemoteVideoStreamType)
  PROPERTY_METHOD_DEFINE(setRemoteDefaultVideoStreamType)
  PROPERTY_METHOD_DEFINE(enableAudioVolumeIndication)
  PROPERTY_METHOD_DEFINE(stopAudioRecording)
  PROPERTY_METHOD_DEFINE(startAudioMixing)
  PROPERTY_METHOD_DEFINE(stopAudioMixing)
  PROPERTY_METHOD_DEFINE(pauseAudioMixing)
  PROPERTY_METHOD_DEFINE(resumeAudioMixing)
  PROPERTY_METHOD_DEFINE(adjustAudioMixingVolume)
  PROPERTY_METHOD_DEFINE(adjustAudioMixingPlayoutVolume)
  PROPERTY_METHOD_DEFINE(adjustAudioMixingPublishVolume)
  PROPERTY_METHOD_DEFINE(getAudioMixingPlayoutVolume)
  PROPERTY_METHOD_DEFINE(getAudioMixingPublishVolume)
  PROPERTY_METHOD_DEFINE(getAudioMixingDuration)
  PROPERTY_METHOD_DEFINE(getAudioMixingCurrentPosition)
  PROPERTY_METHOD_DEFINE(setAudioMixingPosition)
  PROPERTY_METHOD_DEFINE(getEffectsVolume)
  PROPERTY_METHOD_DEFINE(setEffectsVolume)
  PROPERTY_METHOD_DEFINE(setVolumeOfEffect)
  PROPERTY_METHOD_DEFINE(playEffect)
  PROPERTY_METHOD_DEFINE(stopEffect)
  PROPERTY_METHOD_DEFINE(stopAllEffects)
  PROPERTY_METHOD_DEFINE(preloadEffect)
  PROPERTY_METHOD_DEFINE(unloadEffect)
  PROPERTY_METHOD_DEFINE(pauseEffect)
  PROPERTY_METHOD_DEFINE(pauseAllEffects)
  PROPERTY_METHOD_DEFINE(resumeEffect)
  PROPERTY_METHOD_DEFINE(resumeAllEffects)
  PROPERTY_METHOD_DEFINE(setLocalVoicePitch)
  PROPERTY_METHOD_DEFINE(setLocalVoiceEqualization)
  PROPERTY_METHOD_DEFINE(setLocalVoiceReverb)
  PROPERTY_METHOD_DEFINE(setExternalAudioSink)
  PROPERTY_METHOD_DEFINE(setLocalPublishFallbackOption)
  PROPERTY_METHOD_DEFINE(setRemoteSubscribeFallbackOption)
  PROPERTY_METHOD_DEFINE(setAudioProfile)
  PROPERTY_METHOD_DEFINE(setExternalAudioSource)
  PROPERTY_METHOD_DEFINE(getScreenWindowsInfo)
  PROPERTY_METHOD_DEFINE(getScreenDisplaysInfo)
  PROPERTY_METHOD_DEFINE(startScreenCapture)
  PROPERTY_METHOD_DEFINE(stopScreenCapture)
  PROPERTY_METHOD_DEFINE(updateScreenCaptureRegion)
  PROPERTY_METHOD_DEFINE(setLogFile)
  PROPERTY_METHOD_DEFINE(setLogFilter)
  PROPERTY_METHOD_DEFINE(setLocalVideoMirrorMode)
  PROPERTY_METHOD_DEFINE(enableDualStreamMode)
  PROPERTY_METHOD_DEFINE(setRecordingAudioFrameParameters)
  PROPERTY_METHOD_DEFINE(setPlaybackAudioFrameParameters)
  PROPERTY_METHOD_DEFINE(setMixedAudioFrameParameters)
  PROPERTY_METHOD_DEFINE(adjustRecordingSignalVolume)
  PROPERTY_METHOD_DEFINE(adjustPlaybackSignalVolume)
  PROPERTY_METHOD_DEFINE(setHighQualityAudioParameters)
  PROPERTY_METHOD_DEFINE(enableWebSdkInteroperability)
  PROPERTY_METHOD_DEFINE(setVideoQualityParameters)
  PROPERTY_METHOD_DEFINE(enableLoopbackRecording)
  PROPERTY_METHOD_DEFINE(registerDeliverFrame)
  PROPERTY_METHOD_DEFINE(setupLocalVideo)
  PROPERTY_METHOD_DEFINE(subscribe)
  PROPERTY_METHOD_DEFINE(unsubscribe)
  PROPERTY_METHOD_DEFINE(getVideoDevices)
  PROPERTY_METHOD_DEFINE(setVideoDevice)
  PROPERTY_METHOD_DEFINE(getCurrentVideoDevice)
  PROPERTY_METHOD_DEFINE(startVideoDeviceTest)
  PROPERTY_METHOD_DEFINE(stopVideoDeviceTest)
  PROPERTY_METHOD_DEFINE(getAudioPlaybackDevices)
  PROPERTY_METHOD_DEFINE(setAudioPlaybackDevice)
  PROPERTY_METHOD_DEFINE(getPlaybackDeviceInfo)
  PROPERTY_METHOD_DEFINE(getCurrentAudioPlaybackDevice)
  PROPERTY_METHOD_DEFINE(setAudioPlaybackVolume)
  PROPERTY_METHOD_DEFINE(getAudioPlaybackVolume)
  PROPERTY_METHOD_DEFINE(getAudioRecordingDevices)
  PROPERTY_METHOD_DEFINE(setAudioRecordingDevice)
  PROPERTY_METHOD_DEFINE(getRecordingDeviceInfo)
  PROPERTY_METHOD_DEFINE(getCurrentAudioRecordingDevice)
  PROPERTY_METHOD_DEFINE(getAudioRecordingVolume)
  PROPERTY_METHOD_DEFINE(setAudioRecordingVolume)
  PROPERTY_METHOD_DEFINE(startAudioPlaybackDeviceTest)
  PROPERTY_METHOD_DEFINE(stopAudioPlaybackDeviceTest)
  PROPERTY_METHOD_DEFINE(startAudioRecordingDeviceTest)
  PROPERTY_METHOD_DEFINE(stopAudioRecordingDeviceTest)
  PROPERTY_METHOD_DEFINE(getAudioPlaybackDeviceMute)
  PROPERTY_METHOD_DEFINE(setAudioPlaybackDeviceMute)
  PROPERTY_METHOD_DEFINE(getAudioRecordingDeviceMute)
  PROPERTY_METHOD_DEFINE(setAudioRecordingDeviceMute)
  PROPERTY_METHOD_DEFINE(setEncryptionMode)
  PROPERTY_METHOD_DEFINE(addPublishStreamUrl)
  PROPERTY_METHOD_DEFINE(removePublishStreamUrl)
  PROPERTY_METHOD_DEFINE(addVideoWatermark)
  PROPERTY_METHOD_DEFINE(clearVideoWatermarks)
  PROPERTY_METHOD_DEFINE(setLiveTranscoding)
  PROPERTY_METHOD_DEFINE(addInjectStreamUrl)
  PROPERTY_METHOD_DEFINE(removeInjectStreamUrl)
  PROPERTY_METHOD_DEFINE(startScreenCapture2)
  PROPERTY_METHOD_DEFINE(stopScreenCapture2)
  PROPERTY_METHOD_DEFINE(videoSourceInitialize)
  PROPERTY_METHOD_DEFINE(videoSourceJoin)
  PROPERTY_METHOD_DEFINE(videoSourceLeave)
  PROPERTY_METHOD_DEFINE(videoSourceRenewToken)
  PROPERTY_METHOD_DEFINE(videoSourceSetChannelProfile)
  PROPERTY_METHOD_DEFINE(videoSourceSetVideoProfile)
  PROPERTY_METHOD_DEFINE(videoSourceRelease)
  PROPERTY_METHOD_DEFINE(videoSourceStartPreview)
  PROPERTY_METHOD_DEFINE(videoSourceStopPreview)
  PROPERTY_METHOD_DEFINE(videoSourceEnableWebSdkInteroperability)
  PROPERTY_METHOD_DEFINE(videoSourceEnableDualStreamMode)
  PROPERTY_METHOD_DEFINE(videoSourceSetLogFile)
  PROPERTY_METHOD_DEFINE(videoSourceSetParameter)
  PROPERTY_METHOD_DEFINE(videoSourceUpdateScreenCaptureRegion)
  PROPERTY_METHOD_DEFINE(videoSourceEnableLoopbackRecording)
  PROPERTY_METHOD_DEFINE(videoSourceEnableAudio)
  PROPERTY_METHOD_DEFINE(videoSourceEnableEncryption)
  PROPERTY_METHOD_DEFINE(videoSourceSetEncryptionMode)
  PROPERTY_METHOD_DEFINE(videoSourceSetEncryptionSecret);
  PROPERTY_METHOD_DEFINE(setBool);
  PROPERTY_METHOD_DEFINE(setInt);
  PROPERTY_METHOD_DEFINE(setUInt);
  PROPERTY_METHOD_DEFINE(setNumber);
  PROPERTY_METHOD_DEFINE(setString);
  PROPERTY_METHOD_DEFINE(setObject);
  PROPERTY_METHOD_DEFINE(getBool);
  PROPERTY_METHOD_DEFINE(getInt);
  PROPERTY_METHOD_DEFINE(getUInt);
  PROPERTY_METHOD_DEFINE(getNumber);
  PROPERTY_METHOD_DEFINE(getString);
  PROPERTY_METHOD_DEFINE(getObject);
  PROPERTY_METHOD_DEFINE(getArray);
  PROPERTY_METHOD_DEFINE(setParameters);
  PROPERTY_METHOD_DEFINE(setProfile);
  PROPERTY_METHOD_DEFINE(convertPath);
  PROPERTY_METHOD_DEFINE(setVideoRenderDimension);
  PROPERTY_METHOD_DEFINE(setHighFPS);
  PROPERTY_METHOD_DEFINE(setFPS);
  PROPERTY_METHOD_DEFINE(addToHighVideo);
  PROPERTY_METHOD_DEFINE(removeFromHighVideo);

  // plugin apis
  PROPERTY_METHOD_DEFINE(initializePluginManager);
  PROPERTY_METHOD_DEFINE(releasePluginManager);
  PROPERTY_METHOD_DEFINE(registerPlugin);
  PROPERTY_METHOD_DEFINE(unregisterPlugin);
  PROPERTY_METHOD_DEFINE(enablePlugin);
  PROPERTY_METHOD_DEFINE(getPlugins);
  PROPERTY_METHOD_DEFINE(setPluginParameter);
  PROPERTY_METHOD_DEFINE(getPluginParameter);

  // 2.3.3 apis
  PROPERTY_METHOD_DEFINE(getConnectionState);
  PROPERTY_METHOD_DEFINE(release);

  // 2.4.0 apis
  PROPERTY_METHOD_DEFINE(setBeautyEffectOptions);
  PROPERTY_METHOD_DEFINE(setLocalVoiceChanger);
  PROPERTY_METHOD_DEFINE(setLocalVoiceReverbPreset);
  PROPERTY_METHOD_DEFINE(enableSoundPositionIndication);
  PROPERTY_METHOD_DEFINE(setRemoteVoicePosition);
  PROPERTY_METHOD_DEFINE(startLastmileProbeTest);
  PROPERTY_METHOD_DEFINE(stopLastmileProbeTest);
  PROPERTY_METHOD_DEFINE(setRemoteUserPriority);
  PROPERTY_METHOD_DEFINE(startEchoTestWithInterval);
  PROPERTY_METHOD_DEFINE(startAudioDeviceLoopbackTest);
  PROPERTY_METHOD_DEFINE(stopAudioDeviceLoopbackTest);
  PROPERTY_METHOD_DEFINE(setCameraCapturerConfiguration);
  PROPERTY_METHOD_DEFINE(setLogFileSize);
  PROPERTY_METHOD_DEFINE(videosourceStartScreenCaptureByScreen);
  PROPERTY_METHOD_DEFINE(videosourceStartScreenCaptureByWindow);
  PROPERTY_METHOD_DEFINE(videosourceUpdateScreenCaptureParameters);
  PROPERTY_METHOD_DEFINE(videosourceSetScreenCaptureContentHint);

  /**
   * 2.8.0 Apis
   */
  PROPERTY_METHOD_DEFINE(registerLocalUserAccount);
  PROPERTY_METHOD_DEFINE(joinChannelWithUserAccount);
  PROPERTY_METHOD_DEFINE(getUserInfoByUserAccount);
  PROPERTY_METHOD_DEFINE(getUserInfoByUid);

  /**
   * 2.9.0 Apis
   */
  PROPERTY_METHOD_DEFINE(switchChannel);
  PROPERTY_METHOD_DEFINE(startChannelMediaRelay);
  PROPERTY_METHOD_DEFINE(updateChannelMediaRelay);
  PROPERTY_METHOD_DEFINE(stopChannelMediaRelay);

  /**
   * 2.9.0.100 Apis
   */
  PROPERTY_METHOD_DEFINE(createChannel);
  PROPERTY_METHOD_DEFINE(startScreenCaptureByScreen);
  PROPERTY_METHOD_DEFINE(startScreenCaptureByWindow);
  PROPERTY_METHOD_DEFINE(updateScreenCaptureParameters);
  PROPERTY_METHOD_DEFINE(setScreenCaptureContentHint);

  /**
   * 3.0.0 Apis
   */
  PROPERTY_METHOD_DEFINE(adjustUserPlaybackSignalVolume);

  PROPERTY_METHOD_DEFINE(setAudioMixingPitch);
  PROPERTY_METHOD_DEFINE(sendMetadata);
  PROPERTY_METHOD_DEFINE(addMetadataEventHandler);
  PROPERTY_METHOD_DEFINE(setMaxMetadataSize);
  PROPERTY_METHOD_DEFINE(registerMediaMetadataObserver);
  PROPERTY_METHOD_DEFINE(unRegisterMediaMetadataObserver);

  PROPERTY_METHOD_DEFINE(sendCustomReportMessage);
  PROPERTY_METHOD_DEFINE(enableEncryption);

  /**
   * 3.2.0 Apis
   */
  PROPERTY_METHOD_DEFINE(setAudioEffectPreset);
  PROPERTY_METHOD_DEFINE(setVoiceBeautifierPreset);
  PROPERTY_METHOD_DEFINE(setAudioEffectParameters);
  PROPERTY_METHOD_DEFINE(setClientRoleWithOptions);

  /**
   * 3.3.0 Apis
   */
  PROPERTY_METHOD_DEFINE(setCloudProxy);
  PROPERTY_METHOD_DEFINE(enableDeepLearningDenoise);
  PROPERTY_METHOD_DEFINE(setVoiceBeautifierParameters);
  PROPERTY_METHOD_DEFINE(uploadLogFile);

  /**
   * 3.3.1
   */
  PROPERTY_METHOD_DEFINE(setVoiceConversionPreset);

  /**
   * 3.4.0
   */
  PROPERTY_METHOD_DEFINE(adjustLoopbackRecordingSignalVolume);
  PROPERTY_METHOD_DEFINE(setEffectPosition);
  PROPERTY_METHOD_DEFINE(getEffectDuration);
  PROPERTY_METHOD_DEFINE(getEffectCurrentPosition);
  PROPERTY_METHOD_DEFINE(setProcessDpiAwareness);
  PROPERTY_METHOD_DEFINE(videoSourceSetProcessDpiAwareness);
  PROPERTY_METHOD_DEFINE(startAudioRecordingWithConfig);

  PROPERTY_METHOD_DEFINE(setAddonLogFile);
  PROPERTY_METHOD_DEFINE(videoSourceSetAddonLogFile);
  /*
   * 3.4.4
   */
  PROPERTY_METHOD_DEFINE(enableVirtualBackground);

  PROPERTY_METHOD_DEFINE(videoSourceStartScreenCaptureByDisplayId);
  PROPERTY_METHOD_DEFINE(startScreenCaptureByDisplayId);
  /*
   * 3.5.1
   */
  PROPERTY_METHOD_DEFINE(pauseAllChannelMediaRelay);
  PROPERTY_METHOD_DEFINE(resumeAllChannelMediaRelay);
  PROPERTY_METHOD_DEFINE(setAudioMixingPlaybackSpeed);
  PROPERTY_METHOD_DEFINE(setAudioMixingDualMonoMode);
  PROPERTY_METHOD_DEFINE(getAudioFileInfo);
  PROPERTY_METHOD_DEFINE(getAudioTrackCount);
  PROPERTY_METHOD_DEFINE(selectAudioTrack);

  /*
   * meeting
   */
  PROPERTY_METHOD_DEFINE(adjustLoopbackSignalVolume);
  PROPERTY_METHOD_DEFINE(videoSourceAdjustRecordingSignalVolume);
  PROPERTY_METHOD_DEFINE(videoSourceAdjustLoopbackRecordingSignalVolume);
  PROPERTY_METHOD_DEFINE(videoSourceMuteRemoteAudioStream);
  PROPERTY_METHOD_DEFINE(videoSourceMuteAllRemoteAudioStreams);
  PROPERTY_METHOD_DEFINE(videoSourceMuteRemoteVideoStream);
  PROPERTY_METHOD_DEFINE(videoSourceMuteAllRemoteVideoStreams);
  PROPERTY_METHOD_DEFINE(videoSourceDisableAudio);
  PROPERTY_METHOD_DEFINE(getDefaultAudioPlaybackDevices);
  PROPERTY_METHOD_DEFINE(getDefaultAudioRecordingDevices);
  PROPERTY_METHOD_DEFINE(videoSourceDisableAudio);

  /**
   * 3.5.2 && 3.6.0
  */
  PROPERTY_METHOD_DEFINE(takeSnapshot);
  PROPERTY_METHOD_DEFINE(startRtmpStreamWithoutTranscoding);
  PROPERTY_METHOD_DEFINE(startRtmpStreamWithTranscoding)
  PROPERTY_METHOD_DEFINE(updateRtmpTranscoding);
  PROPERTY_METHOD_DEFINE(stopRtmpStream);
  PROPERTY_METHOD_DEFINE(setAVSyncSource);
  PROPERTY_METHOD_DEFINE(followSystemPlaybackDevice);
  PROPERTY_METHOD_DEFINE(followSystemRecordingDevice);
  /*
  * 3.4.11
  */
  PROPERTY_METHOD_DEFINE(getScreenCaptureSources);
  /*
  * 3.6.0.2
  */
  PROPERTY_METHOD_DEFINE(setLowlightEnhanceOptions);
  PROPERTY_METHOD_DEFINE(setColorEnhanceOptions);
  PROPERTY_METHOD_DEFINE(setVideoDenoiserOptions);
  PROPERTY_METHOD_DEFINE(startEchoTestWithConfig)

  PROPERTY_METHOD_DEFINE(setLocalAccessPoint);
  PROPERTY_METHOD_DEFINE(videoSourceSetLocalAccessPoint);

  /*
  * 3.7.0
  */
  PROPERTY_METHOD_DEFINE(setScreenCaptureScenario);
  PROPERTY_METHOD_DEFINE(enableLocalVoicePitchCallback);
  PROPERTY_METHOD_DEFINE(enableWirelessAccelerate);
  PROPERTY_METHOD_DEFINE(enableContentInspect);
  PROPERTY_METHOD_DEFINE(enableSpatialAudio);
  PROPERTY_METHOD_DEFINE(setRemoteUserSpatialAudioParams);

  PROPERTY_METHOD_DEFINE(sendStreamMessageWithArrayBuffer);
  PROPERTY_METHOD_DEFINE(videoSourceSetScreenCaptureScenario);

  /**
   * SeaxEngine
   */
  PROPERTY_METHOD_DEFINE(isSeaxJoined);
  PROPERTY_METHOD_DEFINE(getAllSeaxDeviceList);
  PROPERTY_METHOD_DEFINE(enableSeaxAudioDump);
  EN_PROPERTY_DEFINE()

  module->Set(context, Nan::New<v8::String>("NodeRtcEngine").ToLocalChecked(),
              tpl->GetFunction(context).ToLocalChecked());
}

/**
 * The function is used as class constructor in JS layer
 */
void NodeRtcEngine::createInstance(const FunctionCallbackInfo<Value>& args) {
  LOG_ENTER;
  Isolate* isolate = args.GetIsolate();
  /*
   *  Called from new
   */
  if (args.IsConstructCall()) {
    NodeRtcEngine* engine = new NodeRtcEngine(isolate);
    engine->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> instance = cons->NewInstance(context).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }
  LOG_LEAVE;
}

/**
 * Constructor
 */
NodeRtcEngine::NodeRtcEngine(Isolate* isolate) : m_isolate(isolate) {
  LOG_ENTER;
  /** m_engine provide SDK functionality */
  m_engine = createAgoraRtcEngine();
  /** m_eventHandler provide SDK event handler. */
  m_eventHandler.reset(new NodeEventHandler(this));
  /** Node ADDON takes advantage of self render interface */
  m_externalVideoRenderFactory.reset(new NodeVideoRenderFactory(*this));
  /** Video/Audio Plugins */
  m_avPluginManager.reset(new IAVFramePluginManager());
  /** m_videoSourceSink provide facilities to multiple video source based on
   * multiple process */
  m_videoSourceSink.reset(createVideoSource());
  metadataObserver.reset(new NodeMetadataObserver());
  LOG_LEAVE;
}

NodeRtcEngine::~NodeRtcEngine() {
  LOG_ENTER;
  if (m_audioVdm) {
    m_audioVdm->release();
    // delete[] m_audioVdm;
    m_audioVdm = nullptr;
  }
  if (m_videoVdm) {
    m_videoVdm->release();
    // delete[] m_videoVdm;
    m_videoVdm = nullptr;
  }
  if (m_engine) {
    m_engine->release();
    m_engine = nullptr;
  }
  if (metadataObserver.get()) {
    metadataObserver.reset(nullptr);
  }
  m_videoSourceSink.reset(nullptr);
  m_externalVideoRenderFactory.reset(nullptr);
  m_eventHandler.reset(nullptr);
  m_avPluginManager.reset(nullptr);
  LOG_LEAVE;
}

void NodeRtcEngine::destroyVideoSource() {
  if (m_videoSourceSink.get())
    m_videoSourceSink->release();
}

void NodeRtcEngine::initializeSeax(std::string channel_name, uid_t uid,
                                   ChannelMediaOptions& options) {
  bool ret = false;
  do {
    int result = agora::ERR_OK;

    if (!m_seax_engine) {
      m_seax_engine = AgoraSeaxEngineCreate();

      seax::IAgoraSeaxEngineInitParam seax_params;
      if ((result = m_engine->queryInterface(AGORA_IID_MEDIA_ENGINE,
                                             (void**)&m_media_engine)) !=
          agora::ERR_OK) {
        LOG_ERROR(
            "query media engine failed "
            "with error :%d\n",
            result);
        break;
      }

      if ((result = m_engine->queryInterface(AGORA_IID_AUDIO_DEVICE_MANAGER,
                                             (void**)(&m_audio_device))) !=
          agora::ERR_OK) {
        LOG_ERROR("query audio device manager failed with error :%d\n", result);
        break;
      }

      std::string temp_logpath = m_log_path + ".seax.log";
      std::string temp_dumppath = m_log_path + ".seax";
      seax_params.rtc_engine = m_engine;
      seax_params.media_engine = m_media_engine;
      seax_params.audio_device = m_audio_device;
      seax_params.event_handler = m_eventHandler.get();
      seax_params.seax_log_dir =
          m_log_path.empty() ? nullptr : temp_logpath.c_str();
      seax_params.seax_audio_dump_dir =
          m_log_path.empty() ? nullptr : temp_dumppath.c_str();
      if ((result = m_seax_engine->Initialize(seax_params)) != agora::ERR_OK) {
        LOG_ERROR("initialize seax engine failed with error :%d\n", result);
        break;
      }
    }

    if (!m_seax_engine->IsJoinedChannel() &&
        (result = m_seax_engine->JoinChannel(channel_name, uid, options)) !=
            agora::ERR_OK) {
      LOG_ERROR("join seax channel failed with error :%d\n", result);
      break;
    }

    ret = true;
  } while (0);

  if (!ret) uninitializeSeax();
}

void NodeRtcEngine::uninitializeSeax() {
  if(!m_seax_engine) return;

  if (m_seax_engine->IsJoinedChannel()) m_seax_engine->LeaveChannel();

  m_seax_engine->Uninitialize();
  AgoraSeaxEngineDestroy(m_seax_engine);

  m_seax_engine = nullptr;
  m_media_engine = nullptr;
  m_audio_device = nullptr;
  m_seax_enabled = false;
}

NAPI_API_DEFINE_WRAPPER_PARAM_0(startEchoTest);

NAPI_API_DEFINE_WRAPPER_PARAM_0(stopEchoTest);

NAPI_API_DEFINE_WRAPPER_PARAM_0(enableLastmileTest);

NAPI_API_DEFINE_WRAPPER_PARAM_0(disableLastmileTest);

NAPI_API_DEFINE_WRAPPER_PARAM_0(enableVideo);

NAPI_API_DEFINE_WRAPPER_PARAM_0(disableVideo);

NAPI_API_DEFINE_WRAPPER_PARAM_0(startPreview);

NAPI_API_DEFINE_WRAPPER_PARAM_0(stopPreview);

NAPI_API_DEFINE_WRAPPER_PARAM_0(enableAudio);

NAPI_API_DEFINE_WRAPPER_PARAM_0(disableAudio);

NAPI_API_DEFINE_WRAPPER_PARAM_2(adjustUserPlaybackSignalVolume, uid_t, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_0(stopAudioRecording);

NAPI_API_DEFINE_WRAPPER_PARAM_0(stopAudioMixing);

NAPI_API_DEFINE_WRAPPER_PARAM_0(pauseAudioMixing);

NAPI_API_DEFINE_WRAPPER_PARAM_0(resumeAudioMixing);

NAPI_API_DEFINE_WRAPPER_PARAM_0(getEffectsVolume);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setEffectsVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_2(setVolumeOfEffect, int32, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(stopEffect, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_0(stopAllEffects);

NAPI_API_DEFINE_WRAPPER_PARAM_2(preloadEffect, int32, nodestring);

NAPI_API_DEFINE_WRAPPER_PARAM_1(unloadEffect, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(pauseEffect, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_0(pauseAllEffects);

NAPI_API_DEFINE_WRAPPER_PARAM_1(resumeEffect, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_0(resumeAllEffects);

NAPI_API_DEFINE_WRAPPER_PARAM_1(muteLocalAudioStream, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(muteAllRemoteAudioStreams, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setDefaultMuteAllRemoteAudioStreams,
                                        bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(muteLocalVideoStream, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(enableLocalVideo, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(muteAllRemoteVideoStreams, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setDefaultMuteAllRemoteVideoStreams,
                                        bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(adjustAudioMixingVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(adjustAudioMixingPlayoutVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(adjustAudioMixingPublishVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setAudioMixingPosition, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setLocalVoicePitch, double);

NAPI_API_DEFINE_WRAPPER_PARAM_3(setExternalAudioSink,
                                        bool,
                                        int32,
                                        int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setLogFile, nodestring);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setLogFilter, uint32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(enableDualStreamMode, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(adjustRecordingSignalVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(adjustPlaybackSignalVolume, int32);

NAPI_API_DEFINE_WRAPPER_PARAM_1(enableWebSdkInteroperability, bool);

NAPI_API_DEFINE_WRAPPER_PARAM_1(setVideoQualityParameters, bool);

NAPI_API_DEFINE(NodeRtcEngine, addPublishStreamUrl) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring url;
    bool transcodingEnabled;
    napi_status status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], transcodingEnabled);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->addPublishStreamUrl(url, transcodingEnabled);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE_WRAPPER_PARAM_1(removePublishStreamUrl, nodestring);
NAPI_API_DEFINE(NodeRtcEngine, addVideoWatermark) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status;

    CHECK_ARG_NUM(pEngine, args, 2);

    nodestring url;
    status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);

    agora::rtc::WatermarkOptions watermarkOptions;
    Local<Object> options;
    BEGIN_OBJECT_DEFINE(options, args[1]);

    GET_OBJECT_PROPERTY(options, bool, "visibleInPreview",
                        watermarkOptions.visibleInPreview);

    Local<Object> landscapemode;
    agora::rtc::Rectangle positionInLandscapeMode;
    BEGIN_SUB_OBJECT_DEFINE(landscapemode, options, "positionInLandscapeMode");
    GET_OBJECT_PROPERTY(landscapemode, int32, "x", positionInLandscapeMode.x);
    GET_OBJECT_PROPERTY(landscapemode, int32, "y", positionInLandscapeMode.y);
    GET_OBJECT_PROPERTY(landscapemode, int32, "width",
                        positionInLandscapeMode.width);
    GET_OBJECT_PROPERTY(landscapemode, int32, "height",
                        positionInLandscapeMode.height);
    watermarkOptions.positionInLandscapeMode = positionInLandscapeMode;

    Local<Object> portraitmode;
    agora::rtc::Rectangle positionInPortraitMode;
    BEGIN_SUB_OBJECT_DEFINE(portraitmode, options, "positionInPortraitMode");
    GET_OBJECT_PROPERTY(portraitmode, int32, "x", positionInLandscapeMode.x);
    GET_OBJECT_PROPERTY(portraitmode, int32, "y", positionInLandscapeMode.y);
    GET_OBJECT_PROPERTY(portraitmode, int32, "width",
                        positionInLandscapeMode.width);
    GET_OBJECT_PROPERTY(portraitmode, int32, "height",
                        positionInLandscapeMode.height);
    watermarkOptions.positionInPortraitMode = positionInPortraitMode;

    result = pEngine->m_engine->addVideoWatermark(url, watermarkOptions);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
NAPI_API_DEFINE_WRAPPER_PARAM_0(clearVideoWatermarks);

NAPI_API_DEFINE(NodeRtcEngine, setLiveTranscoding) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    Local<Object> obj;
    napi_status status =
        napi_get_value_object_(args.GetIsolate(), args[0], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status,
                            std::string("arg 0 is not a object"));

    auto liveTranscoding = getLiveTranscoding(obj, args, pEngine);
    if (liveTranscoding == nullptr) {
      break;
    }
    result = pEngine->m_engine->setLiveTranscoding(*liveTranscoding);
    if (liveTranscoding->watermark) {
      for (unsigned int i = 0; i < liveTranscoding->watermarkCount; i++) {
        delete liveTranscoding->watermark[i].url;
      }
      delete[] liveTranscoding->watermark;
    }
    if (liveTranscoding->backgroundImage) {
      for (unsigned int i = 0; i < liveTranscoding->backgroundImageCount; i++) {
        delete liveTranscoding->backgroundImage[i].url;
      }
      delete[] liveTranscoding->backgroundImage;
    }
    if (liveTranscoding->transcodingUsers) {
      delete[] liveTranscoding->transcodingUsers;
    }
    if (liveTranscoding->advancedFeatures) {
      for (unsigned int i = 0; i < liveTranscoding->advancedFeatureCount; i++) {
        delete liveTranscoding->advancedFeatures[i].featureName;
      }
      delete[] liveTranscoding->advancedFeatures;
    }
    delete liveTranscoding;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, addInjectStreamUrl) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_ARG_NUM(pEngine, args, 2);

    nodestring url;
    InjectStreamConfig config;
    status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Object> configObj;
    status = napi_get_value_object_(isolate, args[1], configObj);
    CHECK_NAPI_STATUS(pEngine, status);

    int audioSampleRate;
    status = napi_get_object_property_int32_(isolate, configObj, "width",
                                             config.width);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, configObj, "height",
                                             config.height);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, configObj, "videoGop",
                                             config.videoGop);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "videoFramerate", config.videoFramerate);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, configObj, "videoBitrate",
                                             config.videoBitrate);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "audioSampleRate", audioSampleRate);
    CHECK_NAPI_STATUS(pEngine, status);
    config.audioSampleRate = (AUDIO_SAMPLE_RATE_TYPE)audioSampleRate;

    status = napi_get_object_property_int32_(isolate, configObj, "audioBitrate",
                                             config.audioBitrate);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "audioChannels", config.audioChannels);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->addInjectStreamUrl(url, config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE_WRAPPER_PARAM_1(removeInjectStreamUrl, nodestring);

NAPI_API_DEFINE(NodeRtcEngine, setBeautyEffectOptions) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);
    BeautyOptions opts;

    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      int contrast_value = 1;
      status = napi_get_object_property_int32_(
          isolate, obj, "lighteningContrastLevel", contrast_value);
      CHECK_NAPI_STATUS(pEngine, status);

      switch (contrast_value) {
        case 0:
          opts.lighteningContrastLevel = BeautyOptions::LIGHTENING_CONTRAST_LOW;
          break;
        case 1:
          opts.lighteningContrastLevel =
              BeautyOptions::LIGHTENING_CONTRAST_NORMAL;
          break;
        case 2:
          opts.lighteningContrastLevel =
              BeautyOptions::LIGHTENING_CONTRAST_HIGH;
          break;
        default:
          status = napi_invalid_arg;
          break;
      }
      CHECK_NAPI_STATUS(pEngine, status);

      double lightening, smoothness, redness, sharpnessLevel;
      status = napi_get_object_property_double_(isolate, obj, "lighteningLevel",
                                                lightening);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_double_(isolate, obj, "smoothnessLevel",
                                                smoothness);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_double_(isolate, obj, "rednessLevel",
                                                redness);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_double_(isolate, obj, "sharpnessLevel",
                                                sharpnessLevel);
      CHECK_NAPI_STATUS(pEngine, status);
      opts.lighteningLevel = lightening;
      opts.smoothnessLevel = smoothness;
      opts.rednessLevel = redness;
      opts.sharpnessLevel = sharpnessLevel;
    }

    result = pEngine->m_engine->setBeautyEffectOptions(enabled, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalVoiceChanger) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int voiceChanger = 0;
    status = napi_get_value_int32_(args[0], voiceChanger);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setLocalVoiceChanger(VOICE_CHANGER_PRESET(voiceChanger));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalVoiceReverbPreset) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int preset_value = 0;
    status = napi_get_value_int32_(args[0], preset_value);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setLocalVoiceReverbPreset(AUDIO_REVERB_PRESET(preset_value));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableSoundPositionIndication) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);

    result = pEngine->m_engine->enableSoundPositionIndication(enabled);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteVoicePosition) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    uid_t uid;
    double pan = 0, gain = 0;

    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_double_(args[1], pan);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_double_(args[2], gain);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setRemoteVoicePosition(uid, pan, gain);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startLastmileProbeTest) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    LastmileProbeConfig config;
    status = napi_get_object_property_bool_(isolate, obj, "probeUplink",
                                            config.probeUplink);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "probeDownlink",
                                            config.probeDownlink);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(
        isolate, obj, "expectedUplinkBitrate", config.expectedUplinkBitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(isolate, obj,
                                              "expectedDownlinkBitrate",
                                              config.expectedDownlinkBitrate);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->startLastmileProbeTest(config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopLastmileProbeTest) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    result = pEngine->m_engine->stopLastmileProbeTest();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteUserPriority) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    uid_t uid;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    int priority = 100;
    PRIORITY_TYPE type;
    status = napi_get_value_int32_(args[1], priority);
    if (priority == 100) {
      type = PRIORITY_NORMAL;
    } else if (priority == 50) {
      type = PRIORITY_HIGH;
    } else {
      status = napi_invalid_arg;
    }
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setRemoteUserPriority(uid, type);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startEchoTestWithInterval) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int interval;
    status = napi_get_value_int32_(args[0], interval);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->startEchoTest(interval);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startEchoTestWithConfig) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  nodestring token, channelId;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    EchoTestConfiguration config;
    status = napi_get_object_property_bool_(isolate, obj, "enableAudio",
                                            config.enableAudio);
    status = napi_get_object_property_bool_(isolate, obj, "enableVideo",
                                            config.enableVideo);

    status = napi_get_object_property_nodestring_(isolate, obj, "token", token);
    CHECK_NAPI_STATUS(pEngine, status);
    config.token = token;

    status = napi_get_object_property_nodestring_(isolate, obj, "channelId",
                                                  channelId);
    CHECK_NAPI_STATUS(pEngine, status);
    config.channelId = channelId;

    result = pEngine->m_engine->startEchoTest(config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalAccessPoint) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;

  do {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    LocalAccessPointConfiguration localAccessPointConfiguration;

    v8::Array *ipList;
    v8::Array *domainList;
    std::vector<const char *> ipListVec;
    std::vector<const char *> domainListVec;
    std::vector<std::string> ipListVecString;
    std::vector<std::string> domainListVecString;

    status = napi_get_object_property_array_(isolate, obj, "ipList", ipList);
    if (status == napi_ok && ipList->Length() > 0) {
      auto count = ipList->Length();
      ipListVec.reserve(count);
      ipListVecString.reserve(count);
      for (uint32 i = 0; i < ipList->Length(); i++) {
        Local<Value> value = ipList->Get(context, i).ToLocalChecked();
        NodeString str;
        status = napi_get_value_nodestring_(value, str);
        CHECK_NAPI_STATUS(pEngine, status);
        ipListVecString.push_back(string(str));
      }
      for(int index = 0; index < count; ++index)
      {
        ipListVec.push_back(ipListVecString[index].c_str());
      }
      localAccessPointConfiguration.ipList = ipListVec.data();
      localAccessPointConfiguration.ipListSize = count;
    }

    status = napi_get_object_property_array_(isolate, obj, "domainList", domainList);
    if (status == napi_ok && domainList->Length() > 0) {
      auto count = domainList->Length();
      domainListVec.reserve(count);
      domainListVecString.reserve(count);
      for (uint32 i = 0; i < domainList->Length(); i++) {
        Local<Value> value = domainList->Get(context, i).ToLocalChecked();
        NodeString str;
        status = napi_get_value_nodestring_(value, str);
        CHECK_NAPI_STATUS(pEngine, status);
        domainListVecString.push_back(string(str));
      }
      for(int index = 0; index < count; ++index)
      {
        domainListVec.push_back(domainListVecString[index].c_str());
      }
      localAccessPointConfiguration.domainList = domainListVec.data();
      localAccessPointConfiguration.domainListSize = count;
    }

    NodeString verifyDomainNameStr;
    status = napi_get_object_property_nodestring_(isolate, obj, "verifyDomainName", verifyDomainNameStr);
    CHECK_NAPI_STATUS(pEngine, status);
    localAccessPointConfiguration.verifyDomainName = verifyDomainNameStr;

    int32_t mode;
    status = napi_get_object_property_int32_(isolate, obj, "mode", mode);
    CHECK_NAPI_STATUS(pEngine, status);
    localAccessPointConfiguration.mode = (LOCAL_PROXY_MODE)mode;

    result = pEngine->m_engine->setLocalAccessPoint(localAccessPointConfiguration);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetLocalAccessPoint) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::unique_ptr<LocalAccessPointConfigurationCmd> cmd(
      new LocalAccessPointConfigurationCmd());

  do {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    v8::Array *ipList;
    v8::Array *domainList;

    std::string ipListStr;
    std::string domainListStr;
    status = napi_get_object_property_array_(isolate, obj, "ipList", ipList);
    if (status == napi_ok && ipList->Length() > 0) {
      for (uint32 i = 0; i < ipList->Length(); i++) {
        Local<Value> value = ipList->Get(context, i).ToLocalChecked();
        NodeString str;
        status = napi_get_value_nodestring_(value, str);
        CHECK_NAPI_STATUS(pEngine, status);
        ipListStr += string(str) + IPC_STRING_PARTTERN;
      }
    }
    memset(cmd->ipList, 0, MAX_STRING_LEN);
    strncpy(cmd->ipList, ipListStr.c_str(), ipListStr.size());

    status =
        napi_get_object_property_array_(isolate, obj, "domainList", domainList);
    if (status == napi_ok && domainList->Length() > 0) {
      for (uint32 i = 0; i < domainList->Length(); i++) {
        Local<Value> value = domainList->Get(context, i).ToLocalChecked();
        NodeString str;
        status = napi_get_value_nodestring_(value, str);
        CHECK_NAPI_STATUS(pEngine, status);
        domainListStr += string(str) + IPC_STRING_PARTTERN;
      }
    }
    memset(cmd->domainList, 0, MAX_STRING_LEN);
    strncpy(cmd->domainList, domainListStr.c_str(), domainListStr.size());

    NodeString verifyDomainNameStr;
    status = napi_get_object_property_nodestring_(
        isolate, obj, "verifyDomainName", verifyDomainNameStr);
    CHECK_NAPI_STATUS(pEngine, status);
    if (verifyDomainNameStr) {
      memset(cmd->verifyDomainName, 0, MAX_STRING_LEN);
      strncpy(cmd->verifyDomainName, verifyDomainNameStr,
              std::string(verifyDomainNameStr).size());
    }
    int32_t mode;
    status = napi_get_object_property_int32_(isolate, obj, "mode", mode);
    CHECK_NAPI_STATUS(pEngine, status);
    cmd->mode = (LOCAL_PROXY_MODE)mode;

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setLocalAccessPoint(cmd);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, sendStreamMessageWithArrayBuffer) {
  LOG_ENTER;
  std::vector<uint8_t> buffer;
  int result = -1;
  uint32_t length = 0;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    int streamId;
    status = napi_get_value_int32_(args[0], streamId);
    CHECK_NAPI_STATUS(pEngine, status);
    napi_get_value_arraybuffer_(args[1], buffer, length);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->sendStreamMessage(
        streamId, (const char *)buffer.data(), length);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startAudioDeviceLoopbackTest) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int interval;
    status = napi_get_value_int32_(args[0], interval);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->startAudioDeviceLoopbackTest(interval) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopAudioDeviceLoopbackTest) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->stopAudioDeviceLoopbackTest() : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setCameraCapturerConfiguration) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    CameraCapturerConfiguration config;
    int preference = 0;

    status =
        napi_get_object_property_int32_(isolate, obj, "preference", preference);
    CHECK_NAPI_STATUS(pEngine, status);
    config.preference = (CAPTURER_OUTPUT_PREFERENCE)preference;

    status = napi_get_object_property_int32_(isolate, obj, "captureWidth",
                                             config.captureWidth);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, obj, "captureHeight",
                                             config.captureHeight);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setCameraCapturerConfiguration(config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLogFileSize) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    unsigned int size;
    status = napi_get_value_uint32_(args[0], size);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setLogFileSize(size);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setBool) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    bool value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setBool(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setInt) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    int32_t value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setInt(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setUInt) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    uint32_t value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_uint32_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setUInt(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setNumber) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    double value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_double_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setNumber(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setString) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    nodestring value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_nodestring_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setString(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setObject) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    nodestring value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_nodestring_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setObject(key, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getBool) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    bool value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getBool(key, value);
    napi_set_bool_result(args, value);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getInt) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    int32 value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getInt(key, value);
    napi_set_int_result(args, value);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getUInt) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    uint32 value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getUInt(key, value);
    args.GetReturnValue().Set(v8::Uint32::New(args.GetIsolate(), value));
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getNumber) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    double value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getNumber(key, value);
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), value));
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getString) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    agora::util::AString value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getString(key, value);
    napi_set_string_result(args, value->c_str());
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getObject) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    agora::util::AString value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getObject(key, value);
    napi_set_string_result(args, value->c_str());
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getArray) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring key;
    agora::util::AString value;
    status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->getArray(key, value);
    napi_set_string_result(args, value->c_str());
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setParameters) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring param;
    status = napi_get_value_nodestring_(args[0], param);
    LOG_INFO("setParameters %s", std::string(param).c_str());
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter ap(pEngine->m_engine);
    result = ap->setParameters(param);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setProfile) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring profile;
    bool merge;
    status = napi_get_value_nodestring_(args[0], profile);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], merge);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    result = param->setProfile(profile, merge);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, convertPath) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring path;
    agora::util::AString value;
    status = napi_get_value_nodestring_(args[0], path);
    CHECK_NAPI_STATUS(pEngine, status);
    AParameter param(pEngine->m_engine);
    param->convertPath(path, value);
    napi_set_string_result(args, value->c_str());
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setExternalAudioSource) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int sampleRate, channels;
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], sampleRate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[2], channels);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setExternalAudioSource(enabled, sampleRate, channels);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalVideoMirrorMode) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 mirrorType;
    status = napi_get_value_int32_(args[0], mirrorType);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setLocalVideoMirrorMode(
        (agora::rtc::VIDEO_MIRROR_MODE_TYPE)mirrorType);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableLoopbackRecording) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enable;
    NodeString deviceName;
    status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);
    if (!args[1]->IsNull()) {
      status = napi_get_value_nodestring_(args[1], deviceName);
      CHECK_NAPI_STATUS(pEngine, status);
      result = pEngine->m_engine->enableLoopbackRecording(enable, deviceName);
    } else {
      result = pEngine->m_engine->enableLoopbackRecording(enable);
    }

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalVoiceEqualization) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 bandFrequency, bandGain;
    status = napi_get_value_int32_(args[0], bandFrequency);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], bandGain);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setLocalVoiceEqualization(
        (AUDIO_EQUALIZATION_BAND_FREQUENCY)bandFrequency, bandGain);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalVoiceReverb) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 reverbKey, value;
    status = napi_get_value_int32_(args[0], reverbKey);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], value);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setLocalVoiceReverb((AUDIO_REVERB_TYPE)reverbKey, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLocalPublishFallbackOption) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 option;
    status = napi_get_value_int32_(args[0], option);
    CHECK_NAPI_STATUS(pEngine, status);
    result =
    pEngine->m_engine->setLocalPublishFallbackOption((STREAM_FALLBACK_OPTIONS)option);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteSubscribeFallbackOption) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 option;
    status = napi_get_value_int32_(args[0], option);
    CHECK_NAPI_STATUS(pEngine, status);
    result =
    pEngine->m_engine->setRemoteSubscribeFallbackOption((STREAM_FALLBACK_OPTIONS)option);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceInitialize) {
  LOG_ENTER;
  int result = -1;
  do {
    LOG_INFO("VideSource: %s\n", __FUNCTION__);
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    LOG_INFO("VideSource: %s check pEngine \n", __FUNCTION__);
    CHECK_NATIVE_THIS(pEngine);
    NodeString appid;
    napi_status status = napi_get_value_nodestring_(args[0], appid);
    LOG_INFO("VideSource: %s check appid \n", __FUNCTION__);
    CHECK_NAPI_STATUS(pEngine, status);

    unsigned int areaCode = AREA_CODE::AREA_CODE_GLOB;
    napi_get_value_uint32_(args[1], areaCode);

    NodeString groupId;
    napi_get_value_nodestring_(args[2], groupId);

    NodeString bundleId;
    napi_get_value_nodestring_(args[3], bundleId);

    if (!pEngine->m_videoSourceSink.get() ||
        !pEngine->m_videoSourceSink->initialize(pEngine->m_eventHandler.get(),
                                                appid, areaCode, groupId,
                                                bundleId)) {
      if (!pEngine->m_videoSourceSink.get()) {
        LOG_ERROR("VideSource: %s not get m_videoSourceSink\n", __FUNCTION__);
      } else {
        LOG_ERROR("VideSource: %s m_videoSourceSink not initialize \n",
                  __FUNCTION__);
      }
      break;
    }
    result = 0;
  } while (false);
  LOG_INFO("VideSource: %s result : %d \n", __FUNCTION__, result);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceJoin) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    Isolate* isolate = args.GetIsolate();
    NodeString key, name, chan_info;
    uid_t uid;
    napi_status status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], name);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[2], chan_info);
    CHECK_NAPI_STATUS(pEngine, status);

    status = NodeUid::getUidFromNodeValue(args[3], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Value> vChannelMediaOptions = args[4];
    Local<Object> oChannelMediaOptions;
    ChannelMediaOptions options;
    options.autoSubscribeAudio = false;
    options.autoSubscribeVideo = false;
    options.publishLocalAudio = false;
    options.publishLocalVideo = true;
    if (vChannelMediaOptions->IsObject()) {
      // with options
      status = napi_get_value_object_(isolate, vChannelMediaOptions,
                                      oChannelMediaOptions);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeAudio",
                                              options.autoSubscribeAudio);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeVideo",
                                              options.autoSubscribeVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalAudio",
                                              options.publishLocalAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalVideo",
                                              options.publishLocalVideo);
      CHECK_NAPI_STATUS(pEngine, status);
    }

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->join(key, name, chan_info, uid, options);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceLeave) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->leave();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceRenewToken) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString token;
    napi_status status = napi_get_value_nodestring_(args[0], token);
    CHECK_NAPI_STATUS(pEngine, status);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->renewVideoSourceToken(token);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetChannelProfile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int profile;
    NodeString permissionKey;
    napi_status status = napi_get_value_int32_(args[0], profile);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], permissionKey);
    CHECK_NAPI_STATUS(pEngine, status);

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setVideoSourceChannelProfile(
          (agora::rtc::CHANNEL_PROFILE_TYPE)profile, permissionKey);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetVideoProfile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int profile;
    bool swapWidthAndHeight;
    napi_status status = napi_get_value_int32_(args[0], profile);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], swapWidthAndHeight);
    CHECK_NAPI_STATUS(pEngine, status);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setVideoSourceVideoProfile(
          (agora::rtc::VIDEO_PROFILE_TYPE)profile, swapWidthAndHeight);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startScreenCapture2) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uint32_t captureFreq, bitrate;
    int top, left, bottom, right;

#if defined(__APPLE__)
    unsigned int windowId;
    status = napi_get_value_uint32_(args[0], windowId);
    CHECK_NAPI_STATUS(pEngine, status);
#elif defined(_WIN32)
#if defined(_WIN64)
    int64_t wid;
    status = napi_get_value_int64_(args[0], wid);
#else
    uint32_t wid;
    status = napi_get_value_uint32_(args[0], wid);
#endif

    CHECK_NAPI_STATUS(pEngine, status);
    HWND windowId = (HWND)wid;
#endif
    status = napi_get_value_uint32_(args[1], captureFreq);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Object> rect;
    status = napi_get_value_object_(isolate, args[2], rect);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, rect, "top", top);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "left", left);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "bottom", bottom);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "right", right);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_uint32_(args[3], bitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    Rect region(top, left, bottom, right);

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->captureScreen(windowId, captureFreq, &region,
                                                bitrate);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopScreenCapture2) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->stopCaptureScreen();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetLogFile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString file;
    napi_status status = napi_get_value_nodestring_(args[0], file);
    CHECK_NAPI_STATUS(pEngine, status);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setLogFile(file);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceRelease) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->release();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceStartPreview) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->startPreview();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceStopPreview) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->stopPreview();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceEnableWebSdkInteroperability) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enabled;
    napi_status status = napi_ok;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->enableWebSdkInteroperability(enabled);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceEnableDualStreamMode) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enable;
    napi_status status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->enableDualStreamMode(enable) != node_ok) {
      break;
    }

    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetParameter) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring param;
    napi_status status = napi_ok;
    status = napi_get_value_nodestring_(args[0], param);
    CHECK_NAPI_STATUS(pEngine, status);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setParameters(param);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceUpdateScreenCaptureRegion) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int top, left, bottom, right;
    Local<Object> rect;
    napi_status status = napi_get_value_object_(isolate, args[0], rect);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, rect, "top", top);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "left", left);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "bottom", bottom);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "right", right);
    CHECK_NAPI_STATUS(pEngine, status);

    Rect region(top, left, bottom, right);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->updateScreenCapture(&region);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videosourceStartScreenCaptureByScreen) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::string key = "";
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    key = "windowId";
    // screenId
    ScreenIDType screen;
#ifdef _WIN32
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    }
    Local<Object> screenRectObj;
    status = napi_get_value_object_(isolate, args[0], screenRectObj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    Rectangle screenRect;
    key = "x";
    status = napi_get_object_property_int32_(isolate, screenRectObj, key,
                                             screenRect.x);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "y";
    status = napi_get_object_property_int32_(isolate, screenRectObj, key,
                                             screenRect.y);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "width";
    status = napi_get_object_property_int32_(isolate, screenRectObj, key,
                                             screenRect.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status = napi_get_object_property_int32_(isolate, screenRectObj, key,
                                             screenRect.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    screen = screenRect;
#elif defined(__APPLE__)
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> displayIdObj;
    status = napi_get_value_object_(isolate, args[0], displayIdObj);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(isolate, displayIdObj, "id",
                                              screen.idVal);
    CHECK_NAPI_STATUS(pEngine, status);
#endif

    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle regionRect;
    key = "x";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.x);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "y";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.y);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, regionRect.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             regionRect.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    // capture parameters
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "frameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "bitrate";
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "captureMouseCursor";
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "windowFocus";
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    captureParams.dimensions = dimensions;

    key = "excludeWindowList";
    std::vector<agora::rtc::IRtcEngine::WindowIDType> excludeWindows;
    Local<Name> keyName = String::NewFromUtf8(isolate, "excludeWindowList",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> excludeWindowList =
        obj->Get(context, keyName).ToLocalChecked();
    if (!excludeWindowList->IsNull() && excludeWindowList->IsArray()) {
      auto excludeWindowListValue = v8::Array::Cast(*excludeWindowList);
      for (int i = 0; i < excludeWindowListValue->Length(); ++i) {
        agora::rtc::IRtcEngine::WindowIDType windowId;
        Local<Value> value =
            excludeWindowListValue->Get(context, i).ToLocalChecked();
#if defined(__APPLE__)
        status = napi_get_value_uint32_(value, windowId);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
#elif defined(_WIN32)
#if defined(_WIN64)
        int64_t wid;
        status = napi_get_value_int64_(value, wid);
#else
        uint32_t wid;
        status = napi_get_value_uint32_(value, wid);
#endif

        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        windowId = (HWND)wid;
#endif
        excludeWindows.push_back(windowId);
      }
    }
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->startScreenCaptureByScreen(
          screen, regionRect, captureParams, excludeWindows);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceStartScreenCaptureByDisplayId) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::string key = "";
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    key = "windowId";
    // screenId
    DisplayInfo displayInfo;

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> displayIdObj;
    status = napi_get_value_object_(isolate, args[0], displayIdObj);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(isolate, displayIdObj, "id",
                                              displayInfo.idVal);
    CHECK_NAPI_STATUS(pEngine, status);

    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle regionRect;
    key = "x";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.x);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "y";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.y);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, regionRect.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             regionRect.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    // capture parameters
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "frameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "bitrate";
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "captureMouseCursor";
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "windowFocus";
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    captureParams.dimensions = dimensions;

    key = "excludeWindowList";
    std::vector<agora::rtc::IRtcEngine::WindowIDType> excludeWindows;
    Local<Name> keyName = String::NewFromUtf8(isolate, "excludeWindowList",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> excludeWindowList =
        obj->Get(context, keyName).ToLocalChecked();
    if (!excludeWindowList->IsNull() && excludeWindowList->IsArray()) {
      auto excludeWindowListValue = v8::Array::Cast(*excludeWindowList);
      for (int i = 0; i < excludeWindowListValue->Length(); ++i) {
        agora::rtc::IRtcEngine::WindowIDType windowId;
        Local<Value> value =
            excludeWindowListValue->Get(context, i).ToLocalChecked();
#if defined(__APPLE__)
        status = napi_get_value_uint32_(value, windowId);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
#elif defined(_WIN32)
#if defined(_WIN64)
        int64_t wid;
        status = napi_get_value_int64_(value, wid);
#else
        uint32_t wid;
        status = napi_get_value_uint32_(value, wid);
#endif

        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        windowId = (HWND)wid;
#endif
        excludeWindows.push_back(windowId);
      }
    }
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->startScreenCaptureByDisplayId(
          displayInfo, regionRect, captureParams, excludeWindows);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startScreenCaptureByDisplayId) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::string key = "";
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    key = "windowId";
    // screenId
    DisplayInfo displayInfo;

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> displayIdObj;
    status = napi_get_value_object_(isolate, args[0], displayIdObj);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(isolate, displayIdObj, "id",
                                              displayInfo.idVal);
    CHECK_NAPI_STATUS(pEngine, status);

    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle regionRect;
    key = "x";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.x);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "y";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.y);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, regionRect.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             regionRect.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    // capture parameters
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "frameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "bitrate";
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "captureMouseCursor";
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    key = "windowFocus";
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    captureParams.dimensions = dimensions;

    key = "excludeWindowList";
    std::vector<agora::rtc::IRtcEngine::WindowIDType> excludeWindows;
    Local<Name> keyName = String::NewFromUtf8(isolate, "excludeWindowList",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> excludeWindowList =
        obj->Get(context, keyName).ToLocalChecked();
    if (!excludeWindowList->IsNull() && excludeWindowList->IsArray()) {
      auto excludeWindowListValue = v8::Array::Cast(*excludeWindowList);
      for (int i = 0; i < excludeWindowListValue->Length(); ++i) {
        agora::rtc::IRtcEngine::WindowIDType windowId;
        Local<Value> value =
            excludeWindowListValue->Get(context, i).ToLocalChecked();
#if defined(__APPLE__)
        status = napi_get_value_uint32_(value, windowId);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
#elif defined(_WIN32)
#if defined(_WIN64)
        int64_t wid;
        status = napi_get_value_int64_(value, wid);
#else
        uint32_t wid;
        status = napi_get_value_uint32_(value, wid);
#endif

        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        windowId = (HWND)wid;
#endif
        excludeWindows.push_back(windowId);
      }
    }
    result = pEngine->m_engine->startScreenCaptureByDisplayId(
          displayInfo.idVal, regionRect, captureParams);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videosourceStartScreenCaptureByWindow) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::string key = "";
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    key = "windowSymbol";
    agora::rtc::IRtcEngine::WindowIDType windowId;
    // screenId
#if defined(__APPLE__)
    status = napi_get_value_uint32_(args[0], windowId);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
#elif defined(_WIN32)
#if defined(_WIN64)
    int64_t wid;
    status = napi_get_value_int64_(args[0], wid);
#else
    uint32_t wid;
    status = napi_get_value_uint32_(args[0], wid);
#endif

    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    windowId = (HWND)wid;
#endif
    key = "rect";
    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    Rectangle regionRect;
    key = "x";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.x);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "y";
    status = napi_get_object_property_int32_(isolate, obj, key, regionRect.y);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, regionRect.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, regionRect.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);

    // capture parameters
    key = "param";
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "frameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "bitrate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "captureMouseCursor";
    status = napi_get_object_property_bool_(isolate, obj, key,
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "windowFocus";
    status = napi_get_object_property_bool_(isolate, obj, key,
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    captureParams.dimensions = dimensions;

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->startScreenCaptureByWindow(
          windowId, regionRect, captureParams);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videosourceUpdateScreenCaptureParameters) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  std::string key = "";
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    // capture parameters
    key = "param";
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    key = "width";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.width);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "height";
    status =
        napi_get_object_property_int32_(isolate, obj, key, dimensions.height);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "frameRate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "bitrate";
    status = napi_get_object_property_int32_(isolate, obj, key,
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "captureMouseCursor";
    status = napi_get_object_property_bool_(isolate, obj, key,
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    key = "windowFocus";
    status = napi_get_object_property_bool_(isolate, obj, key,
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
    captureParams.dimensions = dimensions;

    std::vector<agora::rtc::IRtcEngine::WindowIDType> excludeWindows;
    Local<Name> keyName = String::NewFromUtf8(isolate, "excludeWindowList",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> excludeWindowList =
        obj->Get(context, keyName).ToLocalChecked();
    key = "excludeWindowList";
    if (!excludeWindowList->IsNull() && excludeWindowList->IsArray()) {
      auto excludeWindowListValue = v8::Array::Cast(*excludeWindowList);
      for (int i = 0; i < excludeWindowListValue->Length(); ++i) {
        agora::rtc::IRtcEngine::WindowIDType windowId;
        Local<Value> value =
            excludeWindowListValue->Get(context, i).ToLocalChecked();
#if defined(__APPLE__)
        status = napi_get_value_uint32_(value, windowId);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
#elif defined(_WIN32)
#if defined(_WIN64)
        int64_t wid;
        status = napi_get_value_int64_(value, wid);
#else
        uint32_t wid;
        status = napi_get_value_uint32_(value, wid);
#endif

        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        windowId = (HWND)wid;
#endif
        excludeWindows.push_back(windowId);
      }
    }
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->updateScreenCaptureParameters(captureParams,
                                                                excludeWindows);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videosourceSetScreenCaptureContentHint) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    VideoContentHint hint;
    int value = 0;
    napi_get_value_int32_(args[0], value);
    CHECK_NAPI_STATUS(pEngine, status);

    switch (value) {
      case 0:
        hint = CONTENT_HINT_NONE;
        break;
      case 1:
        hint = CONTENT_HINT_MOTION;
        break;
      case 2:
        hint = CONTENT_HINT_DETAILS;
        break;
    }

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setScreenCaptureContentHint(hint);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceEnableLoopbackRecording) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      bool enable;
      napi_status status = napi_get_value_bool_(args[0], enable);
      CHECK_NAPI_STATUS(pEngine, status);

      nodestring deviceName;
      status = napi_get_value_nodestring_(args[1], deviceName);
      CHECK_NAPI_STATUS(pEngine, status);

      if (deviceName == NULL) {
        pEngine->m_videoSourceSink->enableLoopbackRecording(enable, NULL);
        result = 0;
      } else {
        string mDeviceName(deviceName);
        pEngine->m_videoSourceSink->enableLoopbackRecording(
            enable, mDeviceName.c_str());
        result = 0;
      }
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceEnableAudio) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->enableAudio() != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceEnableEncryption) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enable;
    status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    EncryptionConfig encryptionConfig;
    int encryptionMode;
    status = napi_get_object_property_int32_(isolate, obj, "encryptionMode",
                                             encryptionMode);
    CHECK_NAPI_STATUS(pEngine, status);
    nodestring encryptionKey;
    status = napi_get_object_property_nodestring_(isolate, obj, "encryptionKey",
                                                  encryptionKey);
    CHECK_NAPI_STATUS(pEngine, status);

    napi_get_object_property_arraybuffer_(isolate, obj, "encryptionKdfSalt",
                                          encryptionConfig.encryptionKdfSalt);
    CHECK_NAPI_STATUS(pEngine, status);

    encryptionConfig.encryptionMode = (ENCRYPTION_MODE)encryptionMode;
    encryptionConfig.encryptionKey = encryptionKey;
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->enableEncryption(
            enable, encryptionConfig) != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetEncryptionMode) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring encryptionMode;
    status = napi_get_value_nodestring_(args[0], encryptionMode);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->setEncryptionMode(encryptionMode) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetEncryptionSecret) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring secret;
    status = napi_get_value_nodestring_(args[0], secret);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->setEncryptionSecret(secret) != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, leaveChannel) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    result = pEngine->m_engine->leaveChannel();

    if (pEngine->m_seax_enabled) pEngine->uninitializeSeax();

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableLocalAudio) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    bool enable;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->enableLocalAudio(enable);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, renewToken) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    NodeString newkey;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_get_value_nodestring_(args[0], newkey);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->renewToken(newkey);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, registerDeliverFrame) {
  LOG_ENTER;
  int result = false;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeVideoFrameTransporter* pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      result = pTransporter->initialize(isolate, args);
    }
  } while (false);
  napi_set_bool_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, initialize) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString appid, logPath;
    napi_status status = napi_get_value_nodestring_(args[0], appid);
    CHECK_NAPI_STATUS(pEngine, status);
    unsigned int areaCode;
    status = napi_get_value_uint32_(args[1], areaCode);
    CHECK_NAPI_STATUS(pEngine, status);
    RtcEngineContext context;
    LogConfig logConfig;
    context.eventHandler = pEngine->m_eventHandler.get();
    context.appId = appid;
    context.areaCode = areaCode;

    Local<Value> vLogConfigs = args[2];
    Local<Object> oLogConfigs;
    if (vLogConfigs->IsObject()) {
      status = napi_get_value_object_(isolate, vLogConfigs, oLogConfigs);
      CHECK_NAPI_STATUS(pEngine, status);

      // with valid options
      int fileSize = -1, level = 1;
      status = napi_get_object_property_nodestring_(isolate, oLogConfigs,
                                                    "filePath", logPath);
      if (status == napi_ok) {
        logConfig.filePath = logPath;
        pEngine->m_log_path = logPath;
        LOG_INFO("log config: file path %s", logConfig.filePath);
      } else {
        LOG_WARNING("log config: no file path found");
      }
      status = napi_get_object_property_int32_(isolate, oLogConfigs, "fileSize",
                                               fileSize);
      if (status == napi_ok) {
        logConfig.fileSize = fileSize;
      }
      status =
          napi_get_object_property_int32_(isolate, oLogConfigs, "level", level);
      if (status == napi_ok) {
        logConfig.level = (LOG_LEVEL)level;
      }
      context.logConfig = logConfig;
    } else {
      LOG_INFO("no logConfig found");
    }

    LOG_INFO("begin initialization...");
    int suc = pEngine->m_engine->initialize(context);
    if (0 != suc) {
      LOG_ERROR("Rtc engine initialize failed with error :%d\n", suc);
      break;
    }

    agora::util::AutoPtr<agora::media::IMediaEngine> pMediaEngine;
    pMediaEngine.queryInterface(pEngine->m_engine, AGORA_IID_MEDIA_ENGINE);
    if (pMediaEngine) {
      pMediaEngine->registerVideoRenderFactory(
          pEngine->m_externalVideoRenderFactory.get());
    }

    IRtcEngine3* m_engine2 = (IRtcEngine3*)pEngine->m_engine;
    m_engine2->setAppType(AppType(3));
    pEngine->m_engine->enableVideo();
    pEngine->m_engine->enableLocalVideo(true);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getVersion) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int build = 0;
    const char* strVersion = pEngine->m_engine->getVersion(&build);
    Local<Object> obj = Object::New(args.GetIsolate());
    CHECK_NAPI_OBJECT(obj);
    Local<Value> key = String::NewFromUtf8(args.GetIsolate(), "version",
                                           NewStringType::kInternalized)
                           .ToLocalChecked();
    CHECK_NAPI_OBJECT(key);
    Local<Value> value = String::NewFromUtf8(args.GetIsolate(), strVersion,
                                             NewStringType::kInternalized)
                             .ToLocalChecked();
    CHECK_NAPI_OBJECT(value);
    obj->Set(args.GetIsolate()->GetCurrentContext(), key, value);
    Local<Value> buildKey = String::NewFromUtf8(args.GetIsolate(), "build",
                                                NewStringType::kInternalized)
                                .ToLocalChecked();
    CHECK_NAPI_OBJECT(buildKey);
    Local<Value> buildValue = Integer::New(args.GetIsolate(), build);
    CHECK_NAPI_OBJECT(buildValue);
    obj->Set(args.GetIsolate()->GetCurrentContext(), buildKey, buildValue);
    args.GetReturnValue().Set(obj);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setEncryptionMode) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring mode;
    napi_status status = napi_get_value_nodestring_(args[0], mode);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setEncryptionMode(mode);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getErrorDescription) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int errorCode;
    napi_status status = napi_get_value_int32_(args[0], errorCode);
    CHECK_NAPI_STATUS(pEngine, status);
    const char* desc = pEngine->m_engine->getErrorDescription(errorCode);
    Local<Value> descValue = String::NewFromUtf8(args.GetIsolate(), desc,
                                                 NewStringType::kInternalized)
                                 .ToLocalChecked();
    CHECK_NAPI_OBJECT(descValue);
    args.GetReturnValue().Set(descValue);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, joinChannel) {
  LOG_ENTER;
  int result = -1;
  NodeString key, name, chan_info;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    napi_status status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], name);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[2], chan_info);
    CHECK_NAPI_STATUS(pEngine, status);

    status = NodeUid::getUidFromNodeValue(args[3], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    std::string extra_info = "";

    if (chan_info && strlen(chan_info) > 0) {
      extra_info = "Electron_";
      extra_info += chan_info;
    } else {
      extra_info = "Electron";
    }

    Local<Value> vChannelMediaOptions = args[4];
    Local<Object> oChannelMediaOptions;
    if (vChannelMediaOptions->IsObject()) {
      // with options
      status = napi_get_value_object_(isolate, vChannelMediaOptions,
                                      oChannelMediaOptions);
      CHECK_NAPI_STATUS(pEngine, status);

      ChannelMediaOptions options;
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeAudio",
                                              options.autoSubscribeAudio);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeVideo",
                                              options.autoSubscribeVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalAudio",
                                              options.publishLocalAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalVideo",
                                              options.publishLocalVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(
          isolate, oChannelMediaOptions, "enableSeax", pEngine->m_seax_enabled);
      CHECK_NAPI_STATUS(pEngine, status);

      // enable seax engine
      if (pEngine->m_seax_enabled)
        pEngine->initializeSeax((const char*)name, uid, options);

      result = pEngine->m_engine->joinChannel(key, name, extra_info.c_str(),
                                              uid, options);
    } else {
      // without options
      result =
          pEngine->m_engine->joinChannel(key, name, extra_info.c_str(), uid);
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, switchChannel) {
  LOG_ENTER;
  int result = -1;
  NodeString key, name;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], name);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Value> vChannelMediaOptions = args[2];
    Local<Object> oChannelMediaOptions;
    if (vChannelMediaOptions->IsObject()) {
      // with options
      status = napi_get_value_object_(isolate, vChannelMediaOptions,
                                      oChannelMediaOptions);
      CHECK_NAPI_STATUS(pEngine, status);

      ChannelMediaOptions options;
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeAudio",
                                              options.autoSubscribeAudio);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeVideo",
                                              options.autoSubscribeVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalAudio",
                                              options.publishLocalAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalVideo",
                                              options.publishLocalVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      result = pEngine->m_engine->switchChannel(key, name, options);
    } else {
      // without options
      result = pEngine->m_engine->switchChannel(key, name);
    }

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setChannelProfile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    unsigned int profile = 0;
    status = napi_get_value_uint32_(args[0], profile);
    CHECK_NAPI_STATUS(pEngine, status);
    result =
        pEngine->m_engine->setChannelProfile(CHANNEL_PROFILE_TYPE(profile));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setClientRole) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    unsigned int role;
    status = napi_get_value_uint32_(args[0], role);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setClientRole(CLIENT_ROLE_TYPE(role));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVideoProfile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    unsigned int profile;
    bool swapWandH;
    napi_get_param_2(args, uint32, profile, bool, swapWandH);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setVideoProfile(VIDEO_PROFILE_TYPE(profile),
                                                swapWandH);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVideoEncoderConfiguration) {
  LOG_ENTER;
  int result = -1;
  do {
    napi_status status = napi_ok;
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    VideoDimensions dimensions;
    VideoEncoderConfiguration config;

    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             dimensions.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             dimensions.height);
    CHECK_NAPI_STATUS(pEngine, status);
    config.dimensions = dimensions;
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             config.bitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "minBitrate",
                                             config.minBitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "minFrameRate",
                                             config.minFrameRate);
    CHECK_NAPI_STATUS(pEngine, status);

    int frameRateVal;
    FRAME_RATE frameRate;
    status = napi_get_object_property_int32_(isolate, obj, "frameRate",
                                             frameRateVal);
    CHECK_NAPI_STATUS(pEngine, status);

    config.frameRate = (FRAME_RATE)frameRateVal;

    int orientationModeVal;
    ORIENTATION_MODE orientationMode;
    status = napi_get_object_property_int32_(isolate, obj, "orientationMode",
                                             orientationModeVal);
    CHECK_NAPI_STATUS(pEngine, status);

    switch (orientationModeVal) {
      case 0:
        orientationMode = ORIENTATION_MODE_ADAPTIVE;
        break;
      case 1:
        orientationMode = ORIENTATION_MODE_FIXED_LANDSCAPE;
        break;
      case 2:
        orientationMode = ORIENTATION_MODE_FIXED_PORTRAIT;
        break;
      default:
        status = napi_invalid_arg;
        break;
    }
    CHECK_NAPI_STATUS(pEngine, status);
    config.orientationMode = orientationMode;

    int degradationPrefValue;
    DEGRADATION_PREFERENCE degradationPref;
    status = napi_get_object_property_int32_(
        isolate, obj, "degradationPreference", degradationPrefValue);
    CHECK_NAPI_STATUS(pEngine, status);

    switch (degradationPrefValue) {
      case 0:
        degradationPref = MAINTAIN_QUALITY;
        break;
      case 1:
        degradationPref = MAINTAIN_FRAMERATE;
        break;
      case 2:
        degradationPref = MAINTAIN_BALANCED;
        break;
      default:
        status = napi_invalid_arg;
        break;
    }
    CHECK_NAPI_STATUS(pEngine, status);
    config.degradationPreference = degradationPref;

    int mirrorMode;
    status =
        napi_get_object_property_int32_(isolate, obj, "mirrorMode", mirrorMode);
    config.mirrorMode = VIDEO_MIRROR_MODE_TYPE(mirrorMode);

    result = pEngine->m_engine->setVideoEncoderConfiguration(config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioProfile) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    unsigned int profile, scenario;
    napi_get_param_2(args, uint32, profile, uint32, scenario);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setAudioProfile(AUDIO_PROFILE_TYPE(profile),
                                                AUDIO_SCENARIO_TYPE(scenario));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getCallId) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    util::AString callId;
    if (-ERR_FAILED != pEngine->m_engine->getCallId(callId)) {
      napi_set_string_result(args, callId->c_str());
    }
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, rate) {
  LOG_ENTER;
  NodeString callId, desc;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    int rating;
    napi_get_value_nodestring_(args[0], callId);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], rating);
    CHECK_NAPI_STATUS(pEngine, status);
    napi_get_value_nodestring_(args[2], desc);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->rate(callId, rating, desc);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, complain) {
  LOG_ENTER;
  NodeString callId, desc;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;

    napi_get_value_nodestring_(args[0], callId);
    CHECK_NAPI_STATUS(pEngine, status);
    napi_get_value_nodestring_(args[1], desc);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->complain(callId, desc);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setEncryptionSecret) {
  LOG_ENTER;
  NodeString secret;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    napi_status status = napi_ok;
    CHECK_NATIVE_THIS(pEngine);
    napi_get_value_nodestring_(args[0], secret);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setEncryptionSecret(secret);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, createDataStream) {
  LOG_ENTER;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    int result = -1;
    int streamId;

    if (!args[0]->IsObject()) {
      bool reliable, ordered;
      napi_get_param_2(args, bool, reliable, bool, ordered);
      CHECK_NAPI_STATUS(pEngine, status);
      result =
          pEngine->m_engine->createDataStream(&streamId, reliable, ordered);
    } else {
      Local<Object> obj;
      DataStreamConfig config;
      status = napi_get_value_object_(isolate, args[0], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, obj, "syncWithAudio",
                                              config.syncWithAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, obj, "ordered",
                                              config.ordered);
      CHECK_NAPI_STATUS(pEngine, status);

      result = pEngine->m_engine->createDataStream(&streamId, config);
    }

    if (result < 0) {
      napi_set_int_result(args, result);
    } else {
      napi_set_int_result(args, streamId);
    }
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, sendStreamMessage) {
  LOG_ENTER;
  NodeString msg;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    int streamId;
    status = napi_get_value_int32_(args[0], streamId);
    CHECK_NAPI_STATUS(pEngine, status);
    napi_get_value_nodestring_(args[1], msg);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->sendStreamMessage(streamId, msg, strlen(msg));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, muteRemoteAudioStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    bool mute;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], mute);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->muteRemoteAudioStream(uid, mute);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, subscribe) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    nodestring channel;
    status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], channel);
    CHECK_NAPI_STATUS(pEngine, status);

    std::string sChannel = channel ? std::string(channel) : "";

    auto context =
        new NodeRenderContext(NODE_RENDER_TYPE_REMOTE, uid, sChannel);
    if (!context) {
      LOG_ERROR("Failed to allocate NodeRenderContext\n");
      break;
    }
    VideoCanvas canvas;
    canvas.uid = uid;
    canvas.renderMode = RENDER_MODE_HIDDEN;
    canvas.view = (view_t)context;
    if (channel) {
      strlcpy(canvas.channelId, channel, agora::rtc::MAX_CHANNEL_ID_LENGTH);
    }
    pEngine->m_engine->setupRemoteVideo(canvas);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, unsubscribe) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    nodestring channel;
    status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], channel);
    CHECK_NAPI_STATUS(pEngine, status);
    VideoCanvas canvas;
    canvas.uid = uid;
    if (channel) {
      strlcpy(canvas.channelId, channel, agora::rtc::MAX_CHANNEL_ID_LENGTH);
    }
    pEngine->m_engine->setupRemoteVideo(canvas);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setupLocalVideo) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    auto context = new NodeRenderContext(NODE_RENDER_TYPE_LOCAL);
    VideoCanvas canvas;
    canvas.uid = 0;
    canvas.renderMode = RENDER_MODE_HIDDEN;
    canvas.view = (view_t)context;
    pEngine->m_engine->setupLocalVideo(canvas);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVideoRenderDimension) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeRenderType type;
    int renderType, width, height;
    nodestring channelId;
    agora::rtc::uid_t uid;
    napi_status status = napi_ok;
    status = napi_get_value_int32_(args[0], renderType);
    CHECK_NAPI_STATUS(pEngine, status);
    if (renderType < NODE_RENDER_TYPE_LOCAL ||
        renderType > NODE_RENDER_TYPE_VIDEO_SOURCE) {
      LOG_ERROR("Invalid render type : %d\n", renderType);
      break;
    }
    type = (NodeRenderType)renderType;
    status = NodeUid::getUidFromNodeValue(args[1], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[2], width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[3], height);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_nodestring_(args[4], channelId);
    CHECK_NAPI_STATUS(pEngine, status);

    std::string sChannelId = channelId ? std::string(channelId) : "";

    auto* pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      pTransporter->setVideoDimension(type, uid, sChannelId, width, height);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setHighFPS) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uint32_t fps;
    status = napi_get_value_uint32_(args[0], fps);
    CHECK_NAPI_STATUS(pEngine, status);
    if (fps == 0) {
      status = napi_invalid_arg;
      break;
    }
    auto pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      pTransporter->setHighFPS(fps);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setFPS) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uint32_t fps;
    status = napi_get_value_uint32_(args[0], fps);
    CHECK_NAPI_STATUS(pEngine, status);
    if (fps == 0) {
      status = napi_invalid_arg;
      break;
    }
    auto pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      pTransporter->setFPS(fps);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, addToHighVideo) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    agora::rtc::uid_t uid;
    nodestring channelId;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_nodestring_(args[1], channelId);
    CHECK_NAPI_STATUS(pEngine, status);

    std::string sChannelId = channelId ? std::string(channelId) : "";

    auto pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      pTransporter->addToHighVideo(uid, sChannelId);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, removeFromHighVideo) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    agora::rtc::uid_t uid;
    nodestring channelId;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_nodestring_(args[1], channelId);
    CHECK_NAPI_STATUS(pEngine, status);

    std::string sChannelId = channelId ? std::string(channelId) : "";
    auto pTransporter = getNodeVideoFrameTransporter();
    if (pTransporter) {
      pTransporter->removeFromeHighVideo(uid, sChannelId);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getConnectionState) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    CONNECTION_STATE_TYPE type = pEngine->m_engine->getConnectionState();
    result = type;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, release) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_invalid_arg;
  stopLogService();
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool sync = false;
    napi_get_value_bool_(args[0], sync);

    LOG_F(INFO, "release engine :%d", sync);
    if (pEngine->m_audioVdm) {
      pEngine->m_audioVdm->release();
      // delete[] m_audioVdm;
      pEngine->m_audioVdm = nullptr;
    }
    if (pEngine->m_videoVdm) {
      pEngine->m_videoVdm->release();
      // delete[] m_videoVdm;
      pEngine->m_videoVdm = nullptr;
    }
    pEngine->uninitializeSeax();
    if (pEngine->m_engine) {
      pEngine->m_engine->release(sync);
      pEngine->m_engine = nullptr;
    }
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->release();
    }
    pEngine->m_videoSourceSink.reset(nullptr);
    pEngine->m_externalVideoRenderFactory.reset(nullptr);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, muteRemoteVideoStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    bool mute;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_bool_(args[1], mute);
    CHECK_NAPI_STATUS(pEngine, status);


    result = pEngine->m_engine->muteRemoteVideoStream(uid, mute);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteVideoStreamType) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    uid_t uid;
    int streamType;
    status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_value_int32_(args[1], streamType);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setRemoteVideoStreamType(uid, REMOTE_VIDEO_STREAM_TYPE(streamType));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteDefaultVideoStreamType) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine;
    napi_status status = napi_ok;
    int streamType;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_1(args, int32, streamType);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setRemoteDefaultVideoStreamType(
        REMOTE_VIDEO_STREAM_TYPE(streamType));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE_WRAPPER_PARAM_3(enableAudioVolumeIndication,
                                int32,
                                int32,
                                bool);

NAPI_API_DEFINE(NodeRtcEngine, startAudioMixing) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    NodeString filepath;
    bool loopback, replace;
    int cycle, startPos = 0;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_5(args, nodestring, filepath, bool, loopback, bool, replace,
                     int32, cycle, int32, startPos);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->startAudioMixing(filepath, loopback, replace, cycle, startPos);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioMixingCurrentPosition) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int position = pEngine->m_engine->getAudioMixingCurrentPosition();
    napi_set_int_result(args, position);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioMixingPlayoutVolume) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int volume = pEngine->m_engine->getAudioMixingPlayoutVolume();
    napi_set_int_result(args, volume);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioMixingPublishVolume) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int volume = pEngine->m_engine->getAudioMixingPublishVolume();
    napi_set_int_result(args, volume);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRecordingAudioFrameParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int sampleRate, channel, mode, samplesPerCall;
    napi_get_param_4(args, int32, sampleRate, int32, channel, int32, mode,
                     int32, samplesPerCall);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setRecordingAudioFrameParameters(
        sampleRate, channel, RAW_AUDIO_FRAME_OP_MODE_TYPE(mode),
        samplesPerCall);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setPlaybackAudioFrameParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    int sampleRate, channel, mode, samplesPerCall;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_4(args, int32, sampleRate, int32, channel, int32, mode,
                     int32, samplesPerCall);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setPlaybackAudioFrameParameters(
        sampleRate, channel, RAW_AUDIO_FRAME_OP_MODE_TYPE(mode),
        samplesPerCall);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setMixedAudioFrameParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    int sampleRate, samplesPerCall;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_2(args, int32, sampleRate, int32, samplesPerCall);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setMixedAudioFrameParameters(sampleRate, samplesPerCall);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setHighQualityAudioParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    bool fullband, stereo, fullBitrate;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_3(args, bool, fullband, bool, stereo, bool, fullBitrate);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setHighQualityAudioParameters(fullband, stereo, fullBitrate);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
#if defined(__APPLE__) || defined(_WIN32)
NAPI_API_DEFINE(NodeRtcEngine, startScreenCapture) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int captureFreq, bitrate;
    int top, left, bottom, right;

#if defined(__APPLE__)
    unsigned int windowId;
    status = napi_get_value_uint32_(args[0], windowId);
    CHECK_NAPI_STATUS(pEngine, status);
#elif defined(_WIN32)
#if defined(_WIN64)
    int64_t wid;
    status = napi_get_value_int64_(args[0], wid);
#else
    int32_t wid;
    status = napi_get_value_int32_(args[0], wid);
#endif

    CHECK_NAPI_STATUS(pEngine, status);
    HWND windowId = (HWND)wid;
#endif
    status = napi_get_value_int32_(args[1], captureFreq);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Object> rect;
    status = napi_get_value_object_(isolate, args[2], rect);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, rect, "top", top);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "left", left);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "bottom", bottom);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "right", right);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_int32_(args[3], bitrate);
    CHECK_NAPI_STATUS(pEngine, status);

    Rect region(top, left, bottom, right);
    result = pEngine->m_engine->startScreenCapture(windowId, captureFreq,
                                                   &region, bitrate);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopScreenCapture) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    result = pEngine->m_engine->stopScreenCapture();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, updateScreenCaptureRegion) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int top, left, bottom, right;
    Local<Object> rect;
    napi_status status = napi_get_value_object_(isolate, args[0], rect);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, rect, "top", top);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "left", left);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "bottom", bottom);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, rect, "right", right);
    CHECK_NAPI_STATUS(pEngine, status);

    Rect region(top, left, bottom, right);

    result = pEngine->m_engine->updateScreenCaptureRegion(&region);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
#endif

NAPI_API_DEFINE(NodeRtcEngine, onEvent) {
  // LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    NodeString eventName;
    status = napi_get_value_nodestring_(args[0], eventName);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!args[1]->IsFunction()) {
      LOG_ERROR("Function expected");
      break;
    }

    Local<Function> callback = args[1].As<Function>();
    if (callback.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }
    Persistent<Function> persist;
    persist.Reset(callback);
    Local<Object> obj = args.This();
    Persistent<Object> persistObj;
    persistObj.Reset(obj);
    pEngine->m_eventHandler->addEventHandler((char*)eventName, persistObj,
                                             persist);
  } while (false);
  // LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getVideoDevices) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_videoVdm) {
      pEngine->m_videoVdm = new AVideoDeviceManager(pEngine->m_engine);
    }
    IVideoDeviceManager* vdm = pEngine->m_videoVdm->get();
    auto vdc = vdm ? vdm->enumerateVideoDevices() : nullptr;
    int count = vdc ? vdc->getCount() : 0;
    Local<v8::Array> devices = v8::Array::New(args.GetIsolate(), count);
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    for (int i = 0; i < count; i++) {
      Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
      vdc->getDevice(i, deviceName, deviceId);
      auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
      dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
      devices->Set(context, i, dev);
      deviceName[0] = '\0';
      deviceId[0] = '\0';
    }
    args.GetReturnValue().Set(devices);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVideoDevice) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString deviceId;
    status = napi_get_value_nodestring_(args[0], deviceId);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_videoVdm) {
      pEngine->m_videoVdm = new AVideoDeviceManager(pEngine->m_engine);
    }
    IVideoDeviceManager* vdm = pEngine->m_videoVdm->get();
    result = vdm ? vdm->setDevice(deviceId) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getCurrentVideoDevice) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};

    if (!pEngine->m_videoVdm) {
      pEngine->m_videoVdm = new AVideoDeviceManager(pEngine->m_engine);
    }
    IVideoDeviceManager* vdm = pEngine->m_videoVdm->get();
    if (vdm) {
      vdm->getDevice(deviceId);
    }
    napi_set_string_result(args, deviceId);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startVideoDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    auto context = new NodeRenderContext(NODE_RENDER_TYPE_DEVICE_TEST);

    if (!pEngine->m_videoVdm) {
      pEngine->m_videoVdm = new AVideoDeviceManager(pEngine->m_engine);
    }
    IVideoDeviceManager* vdm = pEngine->m_videoVdm->get();
    result = vdm ? vdm->startDeviceTest(context) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopVideoDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_videoVdm) {
      pEngine->m_videoVdm = new AVideoDeviceManager(pEngine->m_engine);
    }
    IVideoDeviceManager* vdm = pEngine->m_videoVdm->get();
    result = vdm ? vdm->stopDeviceTest() : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioPlaybackDevices) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumeratePlaybackDevices() : nullptr;
    int count = pdc ? pdc->getCount() : 0;
    Local<v8::Array> devices = v8::Array::New(args.GetIsolate(), count);
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    for (int i = 0; i < count; i++) {
      Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
      pdc->getDevice(i, deviceName, deviceId);
      auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
      dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
      devices->Set(context, i, dev);
      deviceName[0] = '\0';
      deviceId[0] = '\0';
    }
    args.GetReturnValue().Set(devices);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioPlaybackDevice) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString deviceId;
    status = napi_get_value_nodestring_(args[0], deviceId);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setPlaybackDevice(deviceId) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getPlaybackDeviceInfo) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumeratePlaybackDevices() : nullptr;
    int count = pdc ? pdc->getCount() : 0;

    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    Local<v8::Array> devices = v8::Array::New(args.GetIsolate(), 1);
    if (count > 0) {
      Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
      adm->getPlaybackDeviceInfo(deviceId, deviceName);

      auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
      dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
      devices->Set(context, 0, dev);
      deviceName[0] = '\0';
      deviceId[0] = '\0';
    }
    args.GetReturnValue().Set(devices);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getCurrentAudioPlaybackDevice) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getPlaybackDevice(deviceId);
    }
    napi_set_string_result(args, deviceId);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioPlaybackVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int volume;
    status = napi_get_value_int32_(args[0], volume);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setPlaybackDeviceVolume(volume) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioPlaybackVolume) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int volume;

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getPlaybackDeviceVolume(&volume);
    }
    napi_set_int_result(args, volume);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioRecordingDevices) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumerateRecordingDevices() : nullptr;
    int count = pdc ? pdc->getCount() : 0;
    Local<v8::Array> devices = v8::Array::New(args.GetIsolate(), count);
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    for (int i = 0; i < count; i++) {
      Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
      pdc->getDevice(i, deviceName, deviceId);
      auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
      dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
      devices->Set(context, i, dev);
      deviceName[0] = '\0';
      deviceId[0] = '\0';
    }
    args.GetReturnValue().Set(devices);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioRecordingDevice) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString deviceId;
    status = napi_get_value_nodestring_(args[0], deviceId);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setRecordingDevice(deviceId) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getRecordingDeviceInfo) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumerateRecordingDevices() : nullptr;
    int count = pdc ? pdc->getCount() : 0;

    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    Local<v8::Array> devices = v8::Array::New(args.GetIsolate(), 1);
    if (count > 0) {
      Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
      adm->getRecordingDeviceInfo(deviceId, deviceName);

      auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                        NewStringType::kInternalized)
                    .ToLocalChecked();
      dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
      dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
      devices->Set(context, 0, dev);
      deviceName[0] = '\0';
      deviceId[0] = '\0';
    }
    args.GetReturnValue().Set(devices);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getCurrentAudioRecordingDevice) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getRecordingDevice(deviceId);
    }
    napi_set_string_result(args, deviceId);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioRecordingVolume) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int volume;

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getRecordingDeviceVolume(&volume);
    }
    napi_set_int_result(args, volume);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioRecordingVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int volume;
    status = napi_get_value_int32_(args[0], volume);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setRecordingDeviceVolume(volume) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startAudioPlaybackDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString filePath;
    status = napi_get_value_nodestring_(args[0], filePath);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->startPlaybackDeviceTest(filePath) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopAudioPlaybackDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->stopPlaybackDeviceTest() : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startAudioRecordingDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int indicateInterval;
    status = napi_get_value_int32_(args[0], indicateInterval);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->startRecordingDeviceTest(indicateInterval) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopAudioRecordingDeviceTest) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->stopRecordingDeviceTest() : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioPlaybackDeviceMute) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool mute;

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getPlaybackDeviceMute(&mute);
    }
    napi_set_bool_result(args, mute);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioPlaybackDeviceMute) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool mute;
    status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setPlaybackDeviceMute(mute) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioRecordingDeviceMute) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool mute;

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    if (adm) {
      adm->getRecordingDeviceMute(&mute);
    }
    napi_set_bool_result(args, mute);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioRecordingDeviceMute) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool mute;
    status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager* adm = pEngine->m_audioVdm->get();
    result = adm ? adm->setRecordingDeviceMute(mute) : -1;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, initializePluginManager) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    agora::media::IMediaEngine* pMediaEngine = nullptr;
    pEngine->getRtcEngine()->queryInterface(agora::AGORA_IID_MEDIA_ENGINE,
                                            (void**)&pMediaEngine);
    if (pEngine->m_avPluginManager.get()) {
      pMediaEngine->registerVideoFrameObserver(
          pEngine->m_avPluginManager.get());
      pMediaEngine->registerAudioFrameObserver(
          pEngine->m_avPluginManager.get());
      pEngine->getRtcEngine()->registerPacketObserver(
          pEngine->m_avPluginManager.get());
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, releasePluginManager) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    agora::media::IMediaEngine* pMediaEngine = nullptr;
    pEngine->getRtcEngine()->queryInterface(agora::AGORA_IID_MEDIA_ENGINE,
                                            (void**)&pMediaEngine);
    pMediaEngine->registerVideoFrameObserver(NULL);
    pMediaEngine->registerAudioFrameObserver(NULL);
     pEngine->getRtcEngine()->registerPacketObserver(NULL);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, registerPlugin) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    Isolate* isolate = args.GetIsolate();

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    napi_status status = napi_ok;
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    nodestring pluginId, pluginFilePath;
    string mPluginId, mPluginFilePath, mPluginFolderPath;
    status = napi_get_object_property_nodestring_(isolate, obj, "id", pluginId);
    CHECK_NAPI_STATUS(pEngine, status);
    mPluginId = pluginId;
    CHECK_PLUGIN_INFO_NOT_EXIST(pEngine, mPluginId);  // has exist => break

    status = napi_get_object_property_nodestring_(isolate, obj, "path",
                                                  pluginFilePath);
    mPluginFilePath = pluginFilePath;

    agora_plugin_info pluginInfo;

    strncpy(pluginInfo.id, mPluginId.c_str(), MAX_PLUGIN_ID);
    //                pluginInfo.id = mPluginId.c_str();

    const size_t last_slash_idx = mPluginFilePath.find_last_of("\\/");
    if (std::string::npos != last_slash_idx) {
      mPluginFolderPath = mPluginFilePath.substr(0, last_slash_idx + 1);
    }

#ifdef WIN32
    // AddDllDirectory(mPluginFolderPath.c_str());
    char* wPluginFilePath = U2G(mPluginFilePath.c_str());
    pluginInfo.pluginModule =
        LoadLibraryEx(wPluginFilePath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    delete[] wPluginFilePath;
    DWORD error = GetLastError();
    LOG_ERROR("LoadLibrary Error :%ld", error);
    CHECK_PLUGIN_MODULE_EXIST(pluginInfo);

    createAgoraAVFramePlugin createPlugin =
        (createAgoraAVFramePlugin)GetProcAddress(
            (HMODULE)pluginInfo.pluginModule, "createAVFramePlugin");
    if (!createPlugin) {
      FreeLibrary((HMODULE)pluginInfo.pluginModule);
      pluginInfo.pluginModule = NULL;
      LOG_ERROR(
          "Error :%s, :%d,  GetProcAddress \"createAVFramePlugin\" Failed\n",
          __FUNCTION__, __LINE__, pluginInfo.id);
      break;
    }
#else
    pluginInfo.pluginModule = dlopen(mPluginFilePath.c_str(), RTLD_LAZY);
    CHECK_PLUGIN_MODULE_EXIST(pluginInfo);

    createAgoraAVFramePlugin createPlugin = (createAgoraAVFramePlugin)dlsym(
        pluginInfo.pluginModule, "createAVFramePlugin");
    if (!createPlugin) {
      dlclose(pluginInfo.pluginModule);
      pluginInfo.pluginModule = NULL;
      LOG_ERROR(
          "Error :%s, :%d,  GetProcAddress \"createAVFramePlugin\" Failed\n",
          __FUNCTION__, __LINE__, pluginInfo.id);
      break;
    }
#endif

    pluginInfo.instance = createPlugin();
    CHECK_PLUGIN_INSTANCE_EXIST(pluginInfo);

#ifdef WIN32

    char* wPluginFolderPath = U2G(mPluginFolderPath.c_str());
    if (pluginInfo.instance->load(wPluginFolderPath) != 0) {
      LOG_ERROR(
          "Error :%s, :%d, plugin: \"%s\"  IAudioFramePlugin::load Failed\n",
          __FUNCTION__, __LINE__, pluginInfo.id);
      break;
    }
    delete[] wPluginFolderPath;
#else
    if (pluginInfo.instance->load(mPluginFolderPath.c_str()) != 0) {
      LOG_ERROR("Error :%s, :%d, plugin: \"%s\"  IAVFramePlugin::load Failed\n",
                __FUNCTION__, __LINE__, pluginInfo.id);
      break;
    }
#endif

    pluginInfo.enabled = false;

    pEngine->m_avPluginManager->registerPlugin(pluginInfo);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, unregisterPlugin) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    std::string pluginId;
    napi_status status = napi_ok;
    READ_PLUGIN_ID(pEngine, status, args[0], pluginId);
    CHECK_PLUGIN_INFO_EXIST(pEngine, pluginId);  // not exist

    pEngine->m_avPluginManager->unregisterPlugin(pluginId);
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enablePlugin) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    napi_status status = napi_ok;
    std::string pluginId;
    READ_PLUGIN_ID(pEngine, status, args[0], pluginId);
    CHECK_PLUGIN_INFO_EXIST(pEngine, pluginId);

    bool enabled = false;
    status = napi_get_value_bool_(args[1], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    agora_plugin_info pluginInfo;
    pEngine->m_avPluginManager->getPlugin(pluginId, pluginInfo);
    CHECK_PLUGIN_INSTANCE_EXIST(pluginInfo);

    if (enabled) {
      result = pluginInfo.instance->enable();
    } else {
      result = pluginInfo.instance->disable();
    }

    if (result != 0) {
      LOG_ERROR(
          "Error :%s, :%d, plugin: \"%s\"  IAVFramePlugin::enablePlugin return "
          "non-zero %d\n",
          __FUNCTION__, __LINE__, pluginInfo.id, result);
      break;
    }
    pEngine->m_avPluginManager->enablePlugin(pluginId, enabled);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getPlugins) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    std::vector<std::string> plugins = pEngine->m_avPluginManager->getPlugins();
    Local<v8::Array> result = v8::Array::New(isolate, plugins.size());
    int idx = 0;
    for (auto it = plugins.begin(); it != plugins.end(); it++, idx++) {
      // found nth element..print and break.
      std::string name = *it;
      agora_plugin_info pluginInfo;
      pEngine->m_avPluginManager->getPlugin(name, pluginInfo);
      Local<Object> obj = Object::New(isolate);
      obj->Set(context, napi_create_string_(isolate, "id"),
               napi_create_string_(isolate, pluginInfo.id));
      result->Set(context, idx, obj);
    }
    args.GetReturnValue().Set(result);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setPluginParameter) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    napi_status status = napi_ok;
    std::string pluginId;
    READ_PLUGIN_ID(pEngine, status, args[0], pluginId);
    CHECK_PLUGIN_INFO_EXIST(pEngine, pluginId);

    nodestring param;
    status = napi_get_value_nodestring_(args[1], param);
    CHECK_NAPI_STATUS(pEngine, status);

    agora_plugin_info pluginInfo;
    pEngine->m_avPluginManager->getPlugin(pluginId, pluginInfo);
    CHECK_PLUGIN_INSTANCE_EXIST(pluginInfo);
    result = pluginInfo.instance->setParameter(param);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getPluginParameter) {
  LOG_ENTER;
  std::string result = "";
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    CHECK_PLUGIN_MANAGER_EXIST(pEngine);

    napi_status status = napi_ok;
    std::string pluginId;
    READ_PLUGIN_ID(pEngine, status, args[0], pluginId);
    CHECK_PLUGIN_INFO_EXIST(pEngine, pluginId);

    nodestring paramKey;
    status = napi_get_value_nodestring_(args[1], paramKey);
    CHECK_NAPI_STATUS(pEngine, status);

    agora_plugin_info pluginInfo;
    pEngine->m_avPluginManager->getPlugin(pluginId, pluginInfo);
    CHECK_PLUGIN_INSTANCE_EXIST(pluginInfo);
    result = std::string(pluginInfo.instance->getParameter(paramKey));
  } while (false);
  napi_set_string_result(args, result.c_str());
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, sendCustomReportMessage) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring id;
    status = napi_get_value_nodestring_(args[0], id);
    CHECK_NAPI_STATUS(pEngine, status);
    nodestring category;
    status = napi_get_value_nodestring_(args[1], category);
    CHECK_NAPI_STATUS(pEngine, status);
    nodestring event;
    status = napi_get_value_nodestring_(args[2], event);
    CHECK_NAPI_STATUS(pEngine, status);
    nodestring label;
    status = napi_get_value_nodestring_(args[3], label);
    CHECK_NAPI_STATUS(pEngine, status);
    int value;
    status = napi_get_value_int32_(args[4], value);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->sendCustomReportMessage(id, category, event,
                                                        label, value);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

#define CHECK_NAPI_OBJ(obj) \
  if (obj.IsEmpty())        \
    break;

#define NODE_SET_OBJ_PROP_UINT32(isolate, obj, name, val)                \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
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

#define NODE_SET_OBJ_PROP_Number(isolate, obj, name, val)                \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
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

#define NODE_SET_OBJ_PROP_BOOL(isolate, obj, name, val)                  \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
    Local<Value> propVal = v8::Boolean::New(isolate, val);               \
    CHECK_NAPI_OBJ(propVal);                                             \
    v8::Maybe<bool> ret =                                                \
        obj->Set(isolate->GetCurrentContext(), propName, propVal);       \
    if (!ret.IsNothing()) {                                              \
      if (!ret.ToChecked()) {                                            \
        break;                                                           \
      }                                                                  \
    }                                                                    \
  }

#define NODE_SET_OBJ_PROP_String(isolate, obj, name, val)                \
  {                                                                      \
    Local<Value> propName =                                              \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized) \
            .ToLocalChecked();                                           \
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

#define NODE_SET_OBJ_WINDOWINFO_DATA(isolate, obj, name, info)                \
  {                                                                           \
    Local<Value> propName =                                                   \
        String::NewFromUtf8(isolate, name, NewStringType::kInternalized)      \
            .ToLocalChecked();                                                \
    Local<v8::ArrayBuffer> buff = v8::ArrayBuffer::New(isolate, info.length); \
    memcpy(buff->GetContents().Data(), info.buffer, info.length);             \
    Local<v8::Uint8Array> dataarray =                                         \
        v8::Uint8Array::New(buff, 0, info.length);                            \
    v8::Maybe<bool> ret =                                                     \
        obj->Set(isolate->GetCurrentContext(), propName, dataarray);          \
    if (!ret.IsNothing()) {                                                   \
      if (!ret.ToChecked()) {                                                 \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  }

NAPI_API_DEFINE(NodeRtcEngine, getScreenWindowsInfo) {
  LOG_ENTER;
  NodeEventCallback *cb = new NodeEventCallback();
  cb->callback.Reset(args[0].As<Function>());
  cb->js_this.Reset(args.This());
  Isolate *isolate = args.GetIsolate();

  agora::rtc::node_async_call::async_call([isolate, cb]() {
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<v8::Array> infos = v8::Array::New(isolate);
    do {
      std::vector<ScreenWindowInfo> allWindows = getAllWindowInfo();
      for (unsigned int i = 0; i < allWindows.size(); ++i) {
        ScreenWindowInfo windowInfo = allWindows[i];
        Local<v8::Object> obj = Object::New(isolate);
#ifdef _WIN32
        UINT32 windowId = (UINT32)windowInfo.windowId;
#elif defined(__APPLE__)
        unsigned int windowId = windowInfo.windowId;
#endif
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "windowId", windowId);
        NODE_SET_OBJ_PROP_String(isolate, obj, "name", windowInfo.name.c_str());
        NODE_SET_OBJ_PROP_String(isolate, obj, "ownerName",
                                 windowInfo.ownerName.c_str());
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "width", windowInfo.width);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "height", windowInfo.height);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "x", windowInfo.x);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "y", windowInfo.y);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "originWidth",
                                 windowInfo.originWidth);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "originHeight",
                                 windowInfo.originHeight);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "processId",
                                 windowInfo.processId);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "currentProcessId",
                                 windowInfo.currentProcessId);

        if (windowInfo.imageData) {
          buffer_info imageInfo;
          imageInfo.buffer = windowInfo.imageData;
          imageInfo.length = windowInfo.imageDataLength;
          NODE_SET_OBJ_WINDOWINFO_DATA(isolate, obj, "image", imageInfo);
          free(windowInfo.imageData);
        }
        infos->Set(context, i, obj);
      }
    } while (false);
    Local<v8::Value> rec[1] = {infos};
    cb->callback.Get(isolate)->Call(context, cb->js_this.Get(isolate), 1, rec);
    delete cb;
  });
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getScreenDisplaysInfo) {
  LOG_ENTER;
  NodeEventCallback *cb = new NodeEventCallback();
  cb->callback.Reset(args[0].As<Function>());
  cb->js_this.Reset(args.This());
  Isolate *isolate = args.GetIsolate();

  agora::rtc::node_async_call::async_call([isolate, cb]() {
    HandleScope scope(isolate);
    Local<Context> context = isolate->GetCurrentContext();
    Local<v8::Array> infos = v8::Array::New(isolate);
    do {
      std::vector<ScreenDisplayInfo> allDisplays = getAllDisplayInfo();
      for (unsigned int i = 0; i < allDisplays.size(); ++i) {
        ScreenDisplayInfo displayInfo = allDisplays[i];
        Local<v8::Object> obj = Object::New(isolate);
#ifdef WIN32 // __WIN32
        DisplayInfo displayId = displayInfo.displayInfo;
#else
        ScreenIDType displayId = displayInfo.displayId;
#endif
        Local<v8::Object> displayIdObj = Object::New(isolate);
        NODE_SET_OBJ_PROP_Number(isolate, displayIdObj, "x", displayInfo.x);
        NODE_SET_OBJ_PROP_Number(isolate, displayIdObj, "y", displayInfo.y);
        NODE_SET_OBJ_PROP_UINT32(isolate, displayIdObj, "width",
                                 displayInfo.width);
        NODE_SET_OBJ_PROP_UINT32(isolate, displayIdObj, "height",
                                 displayInfo.height);
        NODE_SET_OBJ_PROP_UINT32(isolate, displayIdObj, "id", displayId.idVal);
        Local<Value> propName =
            String::NewFromUtf8(isolate, "displayId",
                                NewStringType::kInternalized)
                .ToLocalChecked();
        obj->Set(context, propName, displayIdObj);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "width", displayInfo.width);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "height", displayInfo.height);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "x", displayInfo.x);
        NODE_SET_OBJ_PROP_Number(isolate, obj, "y", displayInfo.y);
        NODE_SET_OBJ_PROP_BOOL(isolate, obj, "isMain", displayInfo.isMain);
        NODE_SET_OBJ_PROP_BOOL(isolate, obj, "isActive", displayInfo.isActive);
        NODE_SET_OBJ_PROP_BOOL(isolate, obj, "isBuiltin",
                               displayInfo.isBuiltin);
        if (displayInfo.imageData) {
          buffer_info imageInfo;
          imageInfo.buffer = displayInfo.imageData;
          imageInfo.length = displayInfo.imageDataLength;
          NODE_SET_OBJ_WINDOWINFO_DATA(isolate, obj, "image", imageInfo);
          free(displayInfo.imageData);
        }
        infos->Set(context, i, obj);
      }
    } while (false);
    Local<v8::Value> rec[1] = {infos};
    cb->callback.Get(isolate)->Call(context, cb->js_this.Get(isolate), 1, rec);
    delete cb;
  });
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, registerLocalUserAccount) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    NodeString appId, userAccount;

    napi_status status = napi_get_value_nodestring_(args[0], appId);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], userAccount);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->registerLocalUserAccount(appId, userAccount);
  } while (false);
  napi_set_array_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, joinChannelWithUserAccount) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    NodeString token, channel, userAccount;

    napi_status status = napi_get_value_nodestring_(args[0], token);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[1], channel);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_nodestring_(args[2], userAccount);
    CHECK_NAPI_STATUS(pEngine, status);

    if (args[3]->IsNullOrUndefined()) {
      LOG_F(INFO, "joinChannelWithUserAccount no mediaOption");
      result = pEngine->m_engine->joinChannelWithUserAccount(token, channel,
                                                             userAccount);
    } else if (args[3]->IsObject()) {
      Local<Object> oChannelMediaOptions;
      status = napi_get_value_object_(isolate, args[3], oChannelMediaOptions);
      CHECK_NAPI_STATUS(pEngine, status);

      ChannelMediaOptions options;
      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeAudio",
                                              options.autoSubscribeAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "autoSubscribeVideo",
                                              options.autoSubscribeVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalAudio",
                                              options.publishLocalAudio);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                              "publishLocalVideo",
                                              options.publishLocalVideo);
      CHECK_NAPI_STATUS(pEngine, status);

      LOG_F(INFO, "joinChannelWithUserAccount with mediaOption");
      result = pEngine->m_engine->joinChannelWithUserAccount(
          token, channel, userAccount, options);
    } else {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
  } while (false);
  napi_set_array_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getUserInfoByUserAccount) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    NodeString userAccount;
    UserInfo userInfo;

    napi_status status = napi_get_value_nodestring_(args[0], userAccount);
    CHECK_NAPI_STATUS(pEngine, status);

    result =
        pEngine->m_engine->getUserInfoByUserAccount(userAccount, &userInfo);
    Local<v8::Object> obj = Object::New(isolate);
    NODE_SET_OBJ_PROP_UINT32(isolate, obj, "errorCode", result);

    Local<v8::Object> userObj = Object::New(isolate);
    NODE_SET_OBJ_PROP_UINT32(isolate, userObj, "uid", userInfo.uid);
    NODE_SET_OBJ_PROP_String(isolate, userObj, "userAccount",
                             userInfo.userAccount);
    Local<Value> propName =
        String::NewFromUtf8(isolate, "userInfo", NewStringType::kInternalized)
            .ToLocalChecked();
    obj->Set(context, propName, userObj);
    args.GetReturnValue().Set(obj);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getUserInfoByUid) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();

    uid_t uid;
    UserInfo userInfo;

    napi_status status = napi_get_value_uint32_(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->getUserInfoByUid(uid, &userInfo);
    Local<v8::Object> obj = Object::New(isolate);
    NODE_SET_OBJ_PROP_UINT32(isolate, obj, "errorCode", result);

    Local<v8::Object> userObj = Object::New(isolate);
    NODE_SET_OBJ_PROP_UINT32(isolate, userObj, "uid", userInfo.uid);
    NODE_SET_OBJ_PROP_String(isolate, userObj, "userAccount",
                             userInfo.userAccount);
    Local<Value> propName =
        String::NewFromUtf8(isolate, "userInfo", NewStringType::kInternalized)
            .ToLocalChecked();
    obj->Set(context, propName, userObj);
    args.GetReturnValue().Set(obj);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startChannelMediaRelay) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    ChannelMediaRelayConfiguration config;
    Local<Object> obj;

    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    string sourceChannel, sourceToken;
    std::vector<string> destChannels, destTokens;
    std::vector<ChannelMediaInfo> destInfos;

    // srcInfo
    Local<Name> keyName = String::NewFromUtf8(args.GetIsolate(), "srcInfo",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Value> srcInfoValue =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    ChannelMediaInfo srcInfo;
    if (!srcInfoValue->IsNullOrUndefined()) {
      NodeString channelName, token;
      Local<Object> objSrcInfo =
          srcInfoValue->ToObject(context).ToLocalChecked();
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), objSrcInfo, "uid",
                                    srcInfo.uid);

      if (channelName != nullptr) {
        sourceChannel = string(channelName);
        srcInfo.channelName = sourceChannel.c_str();
      } else {
        srcInfo.channelName = nullptr;
      }
      if (token != nullptr) {
        sourceToken = (string)token;
        srcInfo.token = sourceToken.c_str();
      } else {
        srcInfo.token = nullptr;
      }
    }

    LOG_F(INFO, "startChannelMediaRelay src channelName: %s, token : %s",
          srcInfo.channelName, srcInfo.token);
    // destInfos
    keyName = String::NewFromUtf8(args.GetIsolate(), "destInfos",
                                  NewStringType::kInternalized)
                  .ToLocalChecked();
    Local<Value> objDestInfos =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    if (objDestInfos->IsNullOrUndefined() || !objDestInfos->IsArray()) {
      status = napi_invalid_arg;
      break;
    }
    auto destInfosValue = v8::Array::Cast(*objDestInfos);
    int destInfoCount = destInfosValue->Length();
    destInfos.resize(destInfoCount);
    destChannels.resize(destInfoCount);
    destTokens.resize(destInfoCount);
    for (uint32 i = 0; i < destInfoCount; i++) {
      // ChannelMediaInfo destInfo;
      Local<Value> value = destInfosValue->Get(context, i).ToLocalChecked();
      Local<Object> destInfoObj = value->ToObject(context).ToLocalChecked();
      if (destInfoObj->IsNullOrUndefined()) {
        status = napi_invalid_arg;
        break;
      }
      NodeString channelName, token;
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), destInfoObj, "uid",
                                    destInfos[i].uid);
      if (channelName) {
        destChannels[i] = string(channelName);
        destInfos[i].channelName = destChannels[i].c_str();
      } else {
        destChannels[i] = "";
      }
      if (token) {
        destTokens[i] = string(token);
        destInfos[i].token = destTokens[i].c_str();
      } else {
        destTokens[i] = "";
      }
    }
    config.srcInfo = &srcInfo;
    config.destInfos = &destInfos[0];
    config.destCount = destInfoCount;
    for (int i = 0; i < destInfoCount; i++) {
      LOG_F(INFO,
            "startChannelMediaRelay src channelName: %s, token: %s,  dest "
            "channelName: %s, token : %s",
            config.srcInfo->channelName, config.srcInfo->token,
            config.destInfos[i].channelName, config.destInfos[i].token);
    }
    result = pEngine->m_engine->startChannelMediaRelay(config);

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, updateChannelMediaRelay) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    Isolate* isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    ChannelMediaRelayConfiguration config;
    Local<Object> obj;
    napi_status status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    string sourceChannel, sourceToken;
    std::vector<string> destChannels, destTokens;
    std::vector<ChannelMediaInfo> destInfos;

    // srcInfo
    Local<Name> keyName = String::NewFromUtf8(args.GetIsolate(), "srcInfo",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Value> srcInfoValue =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    ChannelMediaInfo srcInfo;
    if (!srcInfoValue->IsNullOrUndefined()) {
      NodeString channelName, token;
      Local<Object> objSrcInfo =
          srcInfoValue->ToObject(context).ToLocalChecked();
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), objSrcInfo, "uid",
                                    srcInfo.uid);

      if (channelName != nullptr) {
        sourceChannel = string(channelName);
        srcInfo.channelName = sourceChannel.c_str();
      } else {
        srcInfo.channelName = nullptr;
      }
      if (token != nullptr) {
        sourceToken = (string)token;
        srcInfo.token = sourceToken.c_str();
      } else {
        srcInfo.token = nullptr;
      }
    }

    // destInfos
    keyName = String::NewFromUtf8(args.GetIsolate(), "destInfos",
                                  NewStringType::kInternalized)
                  .ToLocalChecked();
    Local<Value> objDestInfos =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    if (objDestInfos->IsNullOrUndefined() || !objDestInfos->IsArray()) {
      status = napi_invalid_arg;
      break;
    }
    auto destInfosValue = v8::Array::Cast(*objDestInfos);
    int destInfoCount = destInfosValue->Length();
    destInfos.resize(destInfoCount);
    destChannels.resize(destInfoCount);
    destTokens.resize(destInfoCount);
    for (uint32 i = 0; i < destInfoCount; i++) {
      // ChannelMediaInfo destInfo;
      Local<Value> value = destInfosValue->Get(context, i).ToLocalChecked();
      Local<Object> destInfoObj = value->ToObject(context).ToLocalChecked();
      if (destInfoObj->IsNullOrUndefined()) {
        status = napi_invalid_arg;
        break;
      }
      NodeString channelName, token;
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), destInfoObj, "uid",
                                    destInfos[i].uid);
      if (channelName) {
        destChannels[i] = string(channelName);
        destInfos[i].channelName = destChannels[i].c_str();
      } else {
        destChannels[i] = "";
      }
      if (token) {
        destTokens[i] = string(token);
        destInfos[i].token = destTokens[i].c_str();
      } else {
        destTokens[i] = "";
      }
    }
    config.srcInfo = &srcInfo;
    config.destInfos = &destInfos[0];
    config.destCount = destInfoCount;

    result = pEngine->m_engine->updateChannelMediaRelay(config);
    for (int i = 0; i < destInfoCount; i++) {
      LOG_F(INFO,
            "updateChannelMediaRelay src channelName: %s, token: %s,  dest "
            "channelName: %s, token : %s",
            config.srcInfo->channelName, config.srcInfo->token,
            config.destInfos[i].channelName, config.destInfos[i].token);
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopChannelMediaRelay) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    result = pEngine->m_engine->stopChannelMediaRelay();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, createChannel) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring channelName;
    napi_status status = napi_get_value_nodestring_(args[0], channelName);
    CHECK_NAPI_STATUS(pEngine, status);

    IRtcEngine2* engine = (IRtcEngine2*)(pEngine->m_engine);
    IChannel* pChannel = engine->createChannel(channelName);

    if (!pChannel) {
      break;
    }

    // Prepare constructor template
    Local<Object> jschannel = NodeRtcChannel::Init(isolate, pChannel);
    args.GetReturnValue().Set(jschannel);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startScreenCaptureByWindow) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    agora::rtc::IRtcEngine::WindowIDType windowId;
    // screenId
#if defined(__APPLE__)
    status = napi_get_value_uint32_(args[0], windowId);
    CHECK_NAPI_STATUS(pEngine, status);
#elif defined(_WIN32)
#if defined(_WIN64)
    int64_t wid;
    status = napi_get_value_int64_(args[0], wid);
#else
    uint32_t wid;
    status = napi_get_value_uint32_(args[0], wid);
#endif

    CHECK_NAPI_STATUS(pEngine, status);
    windowId = (HWND)wid;
#endif

    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle regionRect;
    status = napi_get_object_property_int32_(isolate, obj, "x", regionRect.x);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "y", regionRect.y);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             regionRect.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             regionRect.height);
    CHECK_NAPI_STATUS(pEngine, status);

    // capture parameters
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             dimensions.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             dimensions.height);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "frameRate",
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS(pEngine, status);
    captureParams.dimensions = dimensions;

    result = pEngine->m_engine->startScreenCaptureByWindowId(
        reinterpret_cast<agora::rtc::view_t>(windowId), regionRect,
        captureParams);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startScreenCaptureByScreen) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    // screenId
    ScreenIDType screen;
#ifdef _WIN32
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> screenRectObj;
    status = napi_get_value_object_(isolate, args[0], screenRectObj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle screenRect;
    status = napi_get_object_property_int32_(isolate, screenRectObj, "x",
                                             screenRect.x);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, screenRectObj, "y",
                                             screenRect.y);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, screenRectObj, "width",
                                             screenRect.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, screenRectObj, "height",
                                             screenRect.height);
    CHECK_NAPI_STATUS(pEngine, status);
    screen = screenRect;
#elif defined(__APPLE__)
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> displayIdObj;
    status = napi_get_value_object_(isolate, args[0], displayIdObj);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uint32_(isolate, displayIdObj, "id",
                                              screen.idVal);
    CHECK_NAPI_STATUS(pEngine, status);
#endif

    // regionRect
    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    Rectangle regionRect;
    status = napi_get_object_property_int32_(isolate, obj, "x", regionRect.x);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "y", regionRect.y);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             regionRect.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             regionRect.height);
    CHECK_NAPI_STATUS(pEngine, status);

    // capture parameters
    if (!args[2]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    status = napi_get_value_object_(isolate, args[2], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             dimensions.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             dimensions.height);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "frameRate",
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS(pEngine, status);
    captureParams.dimensions = dimensions;

#if defined(_WIN32)
    result = pEngine->m_engine->startScreenCaptureByScreenRect(
        screen, regionRect, captureParams);
#elif defined(__APPLE__)
    result = pEngine->m_engine->startScreenCaptureByDisplayId(
        screen.idVal, regionRect, captureParams);
#endif
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, updateScreenCaptureParameters) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    // capture parameters
    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    ScreenCaptureParameters captureParams;
    VideoDimensions dimensions;
    status = napi_get_object_property_int32_(isolate, obj, "width",
                                             dimensions.width);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "height",
                                             dimensions.height);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "frameRate",
                                             captureParams.frameRate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "bitrate",
                                             captureParams.bitrate);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "captureMouseCursor",
                                            captureParams.captureMouseCursor);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_bool_(isolate, obj, "windowFocus",
                                            captureParams.windowFocus);
    CHECK_NAPI_STATUS(pEngine, status);
    captureParams.dimensions = dimensions;

    result = pEngine->m_engine->updateScreenCaptureParameters(captureParams);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setScreenCaptureContentHint) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    VideoContentHint hint;
    int value = 0;
    status = napi_get_value_int32_(args[0], value);
    CHECK_NAPI_STATUS(pEngine, status);

    switch (value) {
      case 0:
        hint = CONTENT_HINT_NONE;
        break;
      case 1:
        hint = CONTENT_HINT_MOTION;
        break;
      case 2:
        hint = CONTENT_HINT_DETAILS;
        break;
    }

    result = pEngine->m_engine->setScreenCaptureContentHint(hint);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
/**
 * 3.0.1 Apis
 */
NAPI_API_DEFINE_WRAPPER_PARAM_1(setAudioMixingPitch, int32);

NAPI_API_DEFINE(NodeRtcEngine, registerMediaMetadataObserver) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    LOG_F(INFO, "registerMediaMetadataObserver");
    if (!(pEngine->metadataObserver.get())) {
      pEngine->metadataObserver.reset(new NodeMetadataObserver());
    }
    result = pEngine->m_engine->registerMediaMetadataObserver(
        pEngine->metadataObserver.get(),
        IMetadataObserver::METADATA_TYPE::VIDEO_METADATA);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, unRegisterMediaMetadataObserver) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    LOG_F(INFO, "unRegisterMediaMetadataObserver");
    result = pEngine->m_engine->registerMediaMetadataObserver(
        nullptr, IMetadataObserver::METADATA_TYPE::VIDEO_METADATA);
    if (pEngine->metadataObserver.get()) {
      pEngine->metadataObserver.get()->clearData();
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, sendMetadata) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pEngine, status);
    }

    if (!pEngine->metadataObserver.get()) {
      result = -100;
      break;
    }

    unsigned int uid = 0;
    double timeStampMs = 0;
    unsigned int size;
    nodestring buffer;
    char* _buffer;

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    // status = napi_get_object_property_uid_(isolate, obj, "uid", uid);
    // CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_uid_(isolate, obj, "size", size);
    CHECK_NAPI_STATUS(pEngine, status);
    status =
        napi_get_object_property_nodestring_(isolate, obj, "buffer", buffer);
    _buffer = buffer;
    CHECK_NAPI_STATUS(pEngine, status);
    // status = napi_get_object_property_double_(isolate, obj, "timeStampMs",
    // timeStampMs); CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->metadataObserver.get()->sendMetadata(
        uid, size, reinterpret_cast<unsigned char*>(_buffer), timeStampMs);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, addMetadataEventHandler) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!args[0]->IsFunction()) {
      LOG_ERROR("Function expected");
      break;
    }

    Local<Function> callback = args[0].As<Function>();
    if (callback.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }

    Local<Function> callback2 = args[1].As<Function>();
    if (callback2.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }

    Persistent<Function> persist;
    persist.Reset(callback);

    Persistent<Function> persist2;
    persist2.Reset(callback2);

    Local<Object> obj = args.This();
    Persistent<Object> persistObj;
    persistObj.Reset(obj);
    result = pEngine->metadataObserver->addEventHandler(persistObj, persist,
                                                        persist2);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setMaxMetadataSize) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int maxSize;
    status = napi_get_value_int32_(args[0], maxSize);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->metadataObserver.get()->setMaxMetadataSize(maxSize);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableEncryption) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    int encryptionMode;
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, obj, "encryptionMode",
                                             encryptionMode);
    CHECK_NAPI_STATUS(pEngine, status);

    nodestring encryptionKey;
    status = napi_get_object_property_nodestring_(isolate, obj, "encryptionKey",
                                                  encryptionKey);
    CHECK_NAPI_STATUS(pEngine, status);

    EncryptionConfig config;
    napi_get_object_property_arraybuffer_(isolate, obj, "encryptionKdfSalt",
                                          config.encryptionKdfSalt);
    CHECK_NAPI_STATUS(pEngine, status);

    config.encryptionMode = (ENCRYPTION_MODE)encryptionMode;
    config.encryptionKey = encryptionKey;
    result = pEngine->m_engine->enableEncryption(enabled, config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioEffectPreset) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int preset;
    status = napi_get_value_int32_(args[0], preset);
    CHECK_NAPI_STATUS(pEngine, status);
    result =
        pEngine->m_engine->setAudioEffectPreset(AUDIO_EFFECT_PRESET(preset));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVoiceBeautifierPreset) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int preset;
    status = napi_get_value_int32_(args[0], preset);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setVoiceBeautifierPreset(
        VOICE_BEAUTIFIER_PRESET(preset));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioEffectParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int preset;
    status = napi_get_value_int32_(args[0], preset);
    CHECK_NAPI_STATUS(pEngine, status);

    int param1;
    status = napi_get_value_int32_(args[1], param1);
    CHECK_NAPI_STATUS(pEngine, status);

    int param2;
    status = napi_get_value_int32_(args[2], param2);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setAudioEffectParameters(
        AUDIO_EFFECT_PRESET(preset), param1, param2);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setClientRoleWithOptions) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    unsigned int role;
    status = napi_get_value_uint32_(args[0], role);
    CHECK_NAPI_STATUS(pEngine, status);

    ClientRoleOptions opts;

    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      int audienceLatencyLevel = (int)AUDIENCE_LATENCY_LEVEL_ULTRA_LOW_LATENCY;

      status = napi_get_object_property_int32_(
          isolate, obj, "audienceLatencyLevel", audienceLatencyLevel);
      CHECK_NAPI_STATUS(pEngine, status);

      switch (audienceLatencyLevel) {
        case 1:
          opts.audienceLatencyLevel = AUDIENCE_LATENCY_LEVEL_LOW_LATENCY;
          break;
        case 2:
          opts.audienceLatencyLevel = AUDIENCE_LATENCY_LEVEL_ULTRA_LOW_LATENCY;
          break;
        default:
          status = napi_invalid_arg;
          break;
      }
      CHECK_NAPI_STATUS(pEngine, status);
    }
    result = pEngine->m_engine->setClientRole((CLIENT_ROLE_TYPE)role, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

// 3.3.0 APIs
NAPI_API_DEFINE(NodeRtcEngine, setCloudProxy) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int type;
    status = napi_get_value_int32_(args[0], type);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->setCloudProxy(CLOUD_PROXY_TYPE(type));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE_WRAPPER_PARAM_1(enableDeepLearningDenoise, bool);

NAPI_API_DEFINE(NodeRtcEngine, setVoiceBeautifierParameters) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int preset, param1, param2;
    status = napi_get_value_int32_(args[0], preset);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_int32_(args[1], param1);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_value_int32_(args[2], param2);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setVoiceBeautifierParameters(
        VOICE_BEAUTIFIER_PRESET(preset), param1, param2);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, uploadLogFile) {
  LOG_ENTER;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_status status = napi_ok;
    util::AString requestId;

    int result = pEngine->m_engine->uploadLogFile(requestId);
    if (result < 0) {
      napi_set_string_result(args, "");
    } else {
      napi_set_string_result(args, requestId->c_str());
    }
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVoiceConversionPreset) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int preset;

    status = napi_get_value_int32_(args[0], preset);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setVoiceConversionPreset(
        VOICE_CONVERSION_PRESET(preset));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
/*
 * 3.4.0
 */
NAPI_API_DEFINE(NodeRtcEngine, getAudioMixingDuration) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    result = pEngine->m_engine->getAudioMixingDuration();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, adjustLoopbackRecordingSignalVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int volume;

    napi_get_param_1(args, int32, volume);

    result = pEngine->m_engine->adjustLoopbackRecordingSignalVolume(volume);
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, setEffectPosition) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int soundId, pos;

    napi_get_param_2(args, int32, soundId, int32, pos);

    result = pEngine->m_engine->setEffectPosition(soundId, pos);
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, getEffectDuration) {
  LOG_ENTER;
  int result = -1;
  NodeString filePath;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    napi_get_param_1(args, nodestring, filePath);
    result = pEngine->m_engine->getEffectDuration(filePath);
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, getEffectCurrentPosition) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int soundId;

    napi_get_param_1(args, int32, soundId);

    result = pEngine->m_engine->getEffectCurrentPosition(soundId);
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, playEffect) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString filePath;
    int soundId, loopCount, gain, startPos = 0;
    double pitch, pan;
    bool publish;

    napi_get_param_8(args, int32, soundId, nodestring, filePath, int32,
                     loopCount, double, pitch, double, pan, int32, gain, bool,
                     publish, int32, startPos);

    result = pEngine->m_engine->playEffect(soundId, filePath, loopCount, pitch,
                                           pan, gain, publish, startPos);
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setProcessDpiAwareness) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    result = SetProcessDpiAwarenessEx();
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetProcessDpiAwareness) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setProcessDpiAwareness();
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startAudioRecordingWithConfig) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_status status = napi_ok;
    Isolate *isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (args[0]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[0], obj);
      CHECK_NAPI_STATUS(pEngine, status);
      NodeString filePath;
      napi_get_object_property_nodestring_(isolate, obj, "filePath", filePath);
      CHECK_NAPI_STATUS(pEngine, status);

      int recordingQuality;
      napi_get_object_property_int32_(isolate, obj, "recordingQuality",
                                      recordingQuality);
      CHECK_NAPI_STATUS(pEngine, status);

      int recordingPosition;
      napi_get_object_property_int32_(isolate, obj, "recordingPosition",
                                      recordingPosition);
      CHECK_NAPI_STATUS(pEngine, status);

      int recordingSampleRate;
      napi_get_object_property_int32_(isolate, obj, "recordingSampleRate", recordingSampleRate);
      CHECK_NAPI_STATUS(pEngine, status);

      int recordingChannel;
      napi_get_object_property_int32_(isolate, obj, "recordingChannel",
                                      recordingChannel);
      CHECK_NAPI_STATUS(pEngine, status);

      AudioRecordingConfiguration config;
      config.filePath = (char *)filePath;
      config.recordingQuality = (AUDIO_RECORDING_QUALITY_TYPE)recordingQuality;
      config.recordingPosition = (AUDIO_RECORDING_POSITION)recordingPosition;
      config.recordingSampleRate = recordingSampleRate;
      config.recordingChannel = recordingChannel;
      result = pEngine->m_engine->startAudioRecording(config);
    }

  } while (false);
  napi_set_int_result(args, result);

  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAddonLogFile) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring path;
    napi_get_param_1(args, nodestring, path);
    string sPath;
    sPath = path ? string(path) : "";
    stopLogService();
    result = startLogService(sPath.c_str());
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetAddonLogFile) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    nodestring path;
    napi_get_param_1(args, nodestring, path);
    CHECK_NAPI_STATUS(pEngine, status);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->setAddonLogFile(path) != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
NAPI_API_DEFINE(NodeRtcEngine, enableVirtualBackground) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_status status = napi_ok;
    Isolate* isolate = args.GetIsolate();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);
    if (!args[1]->IsObject()) {
      break;
    }

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pEngine, status);

    VirtualBackgroundSource backgroundSource;
    nodestring source;
    status =
        napi_get_object_property_nodestring_(isolate, obj, "source", source);
    CHECK_NAPI_STATUS(pEngine, status);
    backgroundSource.source = source;
    int background_source_type;
    status = napi_get_object_property_int32_(
        isolate, obj, "background_source_type", background_source_type);
    CHECK_NAPI_STATUS(pEngine, status);
    backgroundSource.background_source_type =
        (VirtualBackgroundSource::BACKGROUND_SOURCE_TYPE)background_source_type;

    status = napi_get_object_property_uint32_(isolate, obj, "color",
                                              backgroundSource.color);
    CHECK_NAPI_STATUS(pEngine, status);

    int blur_degree = VirtualBackgroundSource::BLUR_DEGREE_HIGH;
    status = napi_get_object_property_int32_(isolate, obj, "blur_degree", blur_degree);
    backgroundSource.blur_degree = (VirtualBackgroundSource::BACKGROUND_BLUR_DEGREE)blur_degree;

    result =
        pEngine->m_engine->enableVirtualBackground(enabled, backgroundSource);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, pauseAllChannelMediaRelay) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    result = pEngine->m_engine->pauseAllChannelMediaRelay();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, resumeAllChannelMediaRelay) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    result = pEngine->m_engine->resumeAllChannelMediaRelay();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioMixingPlaybackSpeed) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int speed = 100;
    napi_get_param_1(args, int32, speed);
    result = pEngine->m_engine->setAudioMixingPlaybackSpeed(speed);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAudioMixingDualMonoMode) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int mode;
    napi_get_param_1(args, int32, mode);
    result = pEngine->m_engine->setAudioMixingDualMonoMode((agora::media::AUDIO_MIXING_DUAL_MONO_MODE)mode);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioFileInfo) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {

    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    NodeString filePath;
    napi_get_param_1(args, nodestring, filePath);
    result = pEngine->m_engine->getAudioFileInfo(filePath);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAudioTrackCount) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    result = pEngine->m_engine->getAudioTrackCount();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, selectAudioTrack) {
  LOG_ENTER;
  int result = -1;
  napi_status status = napi_ok;
  do {
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int index;
    napi_get_param_1(args, int32, index);
    result = pEngine->m_engine->selectAudioTrack(index);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceMuteRemoteAudioStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    uid_t uid;
    napi_status status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    bool mute;
    status = napi_get_value_bool_(args[1], mute);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceMuteRemoteAudioStream: %d", mute);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->muteRemoteAudioStream(uid, mute) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceMuteAllRemoteAudioStreams) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool mute;
    napi_status status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceMuteAllRemoteAudioStreams: %d", mute);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->muteAllRemoteAudioStreams(mute) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceMuteRemoteVideoStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    uid_t uid;
    napi_status status = NodeUid::getUidFromNodeValue(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    bool mute;
    status = napi_get_value_bool_(args[1], mute);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceMuteRemoteVideoStream: %d", mute);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->muteRemoteVideoStream(uid, mute) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceMuteAllRemoteVideoStreams) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool mute;
    napi_status status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceMuteAllRemoteVideoStreams: %d", mute);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->muteAllRemoteVideoStreams(mute) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, adjustLoopbackSignalVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 volume;
    napi_status status = napi_get_value_int32_(args[0], volume);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "adjustLoopbackRecordingSignalVolume:%d", volume);
    result = pEngine->m_engine->adjustLoopbackRecordingSignalVolume(volume);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceAdjustLoopbackRecordingSignalVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 volume;
    napi_status status = napi_get_value_int32_(args[0], volume);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceAdjustLoopbackRecordingSignalVolume:%d", volume);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->adjustLoopbackRecordingSignalVolume(
            volume) != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceAdjustRecordingSignalVolume) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    int32 volume;
    napi_status status = napi_get_value_int32_(args[0], volume);
    CHECK_NAPI_STATUS(pEngine, status);
    LOG_F(INFO, "videoSourceAdjustRecordingSignalVolume:%d", volume);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->adjustRecordingSignalVolume(volume) !=
            node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceDisableAudio) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (!pEngine->m_videoSourceSink.get() ||
        pEngine->m_videoSourceSink->disableAudio() != node_ok) {
      break;
    }
    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getDefaultAudioPlaybackDevices) {
  LOG_ENTER;
  do {
    NodeRtcEngine *pEngine = nullptr;
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager *adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumeratePlaybackDevices() : nullptr;
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
    if (!pdc) {
      args.GetReturnValue().Set(dev);
      return;
    }
    pdc->getDefaultDevice(deviceName, deviceId);
    auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                      NewStringType::kInternalized)
                  .ToLocalChecked();
    auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                      NewStringType::kInternalized)
                  .ToLocalChecked();
    dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
    dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
    deviceName[0] = '\0';
    deviceId[0] = '\0';

    args.GetReturnValue().Set(dev);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getDefaultAudioRecordingDevices) {
  LOG_ENTER;
  do {
    NodeRtcEngine *pEngine = nullptr;
    Isolate *isolate = args.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    IAudioDeviceManager *adm = pEngine->m_audioVdm->get();
    auto pdc = adm ? adm->enumerateRecordingDevices() : nullptr;
    char deviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char deviceId[MAX_DEVICE_ID_LENGTH] = {0};
    Local<v8::Object> dev = v8::Object::New(args.GetIsolate());
    if (!pdc) {
      args.GetReturnValue().Set(dev);
      return;
    }
    pdc->getDefaultDevice(deviceName, deviceId);
    auto dn = v8::String::NewFromUtf8(args.GetIsolate(), deviceName,
                                      NewStringType::kInternalized)
                  .ToLocalChecked();
    auto di = v8::String::NewFromUtf8(args.GetIsolate(), deviceId,
                                      NewStringType::kInternalized)
                  .ToLocalChecked();
    dev->Set(context, Nan::New<String>("devicename").ToLocalChecked(), dn);
    dev->Set(context, Nan::New<String>("deviceid").ToLocalChecked(), di);
    deviceName[0] = '\0';
    deviceId[0] = '\0';
    args.GetReturnValue().Set(dev);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, takeSnapshot) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring channel;
    napi_status status = napi_get_value_nodestring_(args[0], channel);
    CHECK_NAPI_STATUS(pEngine, status);

    uid_t uid;
    status = NodeUid::getUidFromNodeValue(args[1], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    nodestring filePath;
    status = napi_get_value_nodestring_(args[2], filePath);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->takeSnapshot(channel, uid, filePath);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startRtmpStreamWithoutTranscoding) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring url;
    napi_status status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->startRtmpStreamWithoutTranscoding(url);

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, startRtmpStreamWithTranscoding) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring url;
    napi_status status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);

    Local<Object> obj;
    status = napi_get_value_object_(args.GetIsolate(), args[1], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status,
                            std::string("arg 1 is not a object"));

    auto liveTranscoding = getLiveTranscoding(obj, args, pEngine);
    if (liveTranscoding == nullptr) {
      break;
    }
    result = pEngine->m_engine->startRtmpStreamWithTranscoding(
        url, *liveTranscoding);
    if (liveTranscoding->watermark) {
        for (unsigned int i=0; i<liveTranscoding->watermarkCount; i++) {
            delete liveTranscoding->watermark[i].url;
        }
      delete[] liveTranscoding->watermark;
    }
    if (liveTranscoding->backgroundImage) {
        for (unsigned int i=0; i<liveTranscoding->backgroundImageCount; i++) {
            delete liveTranscoding->backgroundImage[i].url;
        }
      delete[] liveTranscoding->backgroundImage;
    }
    if (liveTranscoding->transcodingUsers) {
      delete[] liveTranscoding->transcodingUsers;
    }
    if (liveTranscoding->advancedFeatures) {
        for (unsigned int i=0; i<liveTranscoding->advancedFeatureCount; i++) {
            delete liveTranscoding->advancedFeatures[i].featureName;
        }
      delete[] liveTranscoding->advancedFeatures;
    }
    delete liveTranscoding;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, updateRtmpTranscoding) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    Local<Object> obj;
    napi_status status =
        napi_get_value_object_(args.GetIsolate(), args[0], obj);
    CHECK_NAPI_STATUS_PARAM(pEngine, status,
                            std::string("arg 0 is not a object"));

    auto liveTranscoding = getLiveTranscoding(obj, args, pEngine);
    if (liveTranscoding == nullptr) {
      break;
    }
    result = pEngine->m_engine->updateRtmpTranscoding(*liveTranscoding);
    if (liveTranscoding->watermark) {
      for (unsigned int i = 0; i < liveTranscoding->watermarkCount; i++) {
        delete liveTranscoding->watermark[i].url;
      }
      delete[] liveTranscoding->watermark;
    }
    if (liveTranscoding->backgroundImage) {
      for (unsigned int i = 0; i < liveTranscoding->backgroundImageCount; i++) {
        delete liveTranscoding->backgroundImage[i].url;
      }
      delete[] liveTranscoding->backgroundImage;
    }
    if (liveTranscoding->transcodingUsers) {
      delete[] liveTranscoding->transcodingUsers;
    }
    if (liveTranscoding->advancedFeatures) {
      for (unsigned int i = 0; i < liveTranscoding->advancedFeatureCount; i++) {
        delete liveTranscoding->advancedFeatures[i].featureName;
      }
      delete[] liveTranscoding->advancedFeatures;
    }
    delete liveTranscoding;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, stopRtmpStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring url;
    napi_status status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->stopRtmpStream(url);

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setAVSyncSource) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring channelId;
    napi_status status = napi_get_value_nodestring_(args[0], channelId);
    CHECK_NAPI_STATUS(pEngine, status);

    uid_t uid;
    status = napi_get_value_uid_t_(args[1], uid);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setAVSyncSource(channelId, uid);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, followSystemPlaybackDevice) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enable;
    napi_status status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    result = pEngine->m_audioVdm->get()->followSystemPlaybackDevice(enable);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, followSystemRecordingDevice) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enable;
    napi_status status = napi_get_value_bool_(args[0], enable);
    CHECK_NAPI_STATUS(pEngine, status);

    if (!pEngine->m_audioVdm) {
      pEngine->m_audioVdm = new AAudioDeviceManager(pEngine->m_engine);
    }
    result = pEngine->m_audioVdm->get()->followSystemRecordingDevice(enable);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getScreenCaptureSources) {
  LOG_ENTER;
  int result = -1;
  NodeRtcEngine *pEngine = nullptr;
  napi_status status = napi_ok;
  Isolate *isolate = args.GetIsolate();
  Local<v8::Array> infos = v8::Array::New(isolate);
  Local<Context> context = isolate->GetCurrentContext();
  do {
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    if (!args[0]->IsObject()) {
      break;
    }

    Local<Object> thumbSizeObj;
    status = napi_get_value_object_(isolate, args[0], thumbSizeObj);
    CHECK_NAPI_STATUS(pEngine, status);

#if defined(_WIN32)
    SIZE thumbSize;
    SIZE iconSize;
#else
    agora::rtc::SIZE thumbSize;
    agora::rtc::SIZE iconSize;
#endif
    int32_t thumbWidth, thumbHeight, iconWidth, iconHeight;
    status = napi_get_object_property_int32_(isolate, thumbSizeObj, "width",
                                             thumbWidth);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, thumbSizeObj, "height",
                                             thumbHeight);
    CHECK_NAPI_STATUS(pEngine, status)

    Local<Object> iconSizeSizeObj;
    status = napi_get_value_object_(isolate, args[1], iconSizeSizeObj);
    CHECK_NAPI_STATUS(pEngine, status);

    status = napi_get_object_property_int32_(isolate, iconSizeSizeObj, "width",
                                             iconWidth);
    CHECK_NAPI_STATUS(pEngine, status);
    status = napi_get_object_property_int32_(isolate, iconSizeSizeObj, "height",
                                             iconHeight);
    CHECK_NAPI_STATUS(pEngine, status);
#if defined(_WIN32)
    thumbSize.cx = thumbWidth;
    thumbSize.cy = thumbHeight;
    iconSize.cx = iconWidth;
    iconSize.cy = iconHeight;
#else
    thumbSize.width = thumbWidth;
    thumbSize.height = thumbHeight;
    iconSize.width = iconWidth;
    iconSize.height = iconHeight;
#endif

    bool includeScreen;
    status = napi_get_value_bool_(args[2], includeScreen);
    CHECK_NAPI_STATUS(pEngine, status);

    auto list = pEngine->m_engine->getScreenCaptureSources(thumbSize, iconSize,
                                                           includeScreen);

    for (int i = 0; i < list->getCount(); i++) {
      Local<v8::Object> obj = Object::New(isolate);
      agora::rtc::ScreenCaptureSourceInfo info = list->getSourceInfo(i);

      int type = info.type;
      auto sourceId = info.sourceId;

      std::string sourceName(info.sourceName == nullptr ? "" : info.sourceName);
      std::string processPath(info.processPath == nullptr ? ""
                                                          : info.processPath);
      std::string sourceTitle(info.sourceTitle == nullptr ? ""
                                                          : info.sourceTitle);
      bool primaryMonitor = info.primaryMonitor;

      NODE_SET_OBJ_PROP_UINT32(isolate, obj, "type", type);
      NODE_SET_OBJ_PROP_UINT32(isolate, obj, "sourceId",
                               (unsigned long)sourceId);
      NODE_SET_OBJ_PROP_String(isolate, obj, "sourceName", sourceName.c_str());
      NODE_SET_OBJ_PROP_String(isolate, obj, "processPath",
                               processPath.c_str());
      NODE_SET_OBJ_PROP_String(isolate, obj, "sourceTitle",
                               sourceTitle.c_str());
      NODE_SET_OBJ_PROP_BOOL(isolate, obj, "primaryMonitor", primaryMonitor);

      Local<v8::Object> iconImageObj = Object::New(isolate);
      if (info.iconImage.buffer) {
        BufferInfo iconImageInfo;

        ConvertRGBToBMP(
            const_cast<unsigned char *>(
                reinterpret_cast<const unsigned char *>(info.iconImage.buffer)),
            iconImageInfo, info.iconImage.width, info.iconImage.height);
        NODE_SET_OBJ_WINDOWINFO_DATA(isolate, iconImageObj, "buffer",
                                     iconImageInfo);
        free(iconImageInfo.buffer);

        NODE_SET_OBJ_PROP_UINT32(isolate, iconImageObj, "width",
                                 info.iconImage.width);
        NODE_SET_OBJ_PROP_UINT32(isolate, iconImageObj, "height",
                                 info.iconImage.height);

        Local<Value> propName =
            String::NewFromUtf8(isolate, "iconImage",
                                NewStringType::kInternalized)
                .ToLocalChecked();
        obj->Set(isolate->GetCurrentContext(), propName, iconImageObj);
      }

      Local<v8::Object> thumbImageObj = Object::New(isolate);
      if (info.thumbImage.buffer) {
        BufferInfo thumbImageInfo;

        ConvertRGBToBMP(
            const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(
                info.thumbImage.buffer)),
            thumbImageInfo, info.thumbImage.width, info.thumbImage.height);
        NODE_SET_OBJ_WINDOWINFO_DATA(isolate, thumbImageObj, "buffer",
                                     thumbImageInfo);
        free(thumbImageInfo.buffer);

        NODE_SET_OBJ_PROP_UINT32(isolate, thumbImageObj, "width",
                                 info.thumbImage.width);
        NODE_SET_OBJ_PROP_UINT32(isolate, thumbImageObj, "height",
                                 info.thumbImage.height);
        Local<Value> propName =
            String::NewFromUtf8(isolate, "thumbImage",
                                NewStringType::kInternalized)
                .ToLocalChecked();
        obj->Set(isolate->GetCurrentContext(), propName, thumbImageObj);
      }

      infos->Set(context, i, obj);
    }
    list->release();

  } while (false);
  napi_set_array_result(args, infos);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setLowlightEnhanceOptions) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    int mode = 0, level = 0;

    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_int32_(isolate, obj, "mode", mode);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_int32_(isolate, obj, "level", level);
      CHECK_NAPI_STATUS(pEngine, status);
    }
    agora::rtc::LowLightEnhanceOptions opts(
        (agora::rtc::LowLightEnhanceOptions::LOW_LIGHT_ENHANCE_MODE)mode,
        (agora::rtc::LowLightEnhanceOptions::LOW_LIGHT_ENHANCE_LEVEL)level);
    result = pEngine->m_engine->setLowlightEnhanceOptions(enabled, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setVideoDenoiserOptions) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    int mode = 0, level = 0;

    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_int32_(isolate, obj, "mode", mode);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_int32_(isolate, obj, "level", level);
      CHECK_NAPI_STATUS(pEngine, status);
    }
    agora::rtc::VideoDenoiserOptions opts(
        (agora::rtc::VideoDenoiserOptions::VIDEO_DENOISER_MODE)mode,
        (agora::rtc::VideoDenoiserOptions::VIDEO_DENOISER_LEVEL)level);
    result = pEngine->m_engine->setVideoDenoiserOptions(enabled, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setColorEnhanceOptions) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    double strengthLevel = 0, skinProtectLevel = 0;
    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_double_(isolate, obj, "strengthLevel",
                                                strengthLevel);
      CHECK_NAPI_STATUS(pEngine, status);
      status = napi_get_object_property_double_(
          isolate, obj, "skinProtectLevel", skinProtectLevel);
      CHECK_NAPI_STATUS(pEngine, status);
    }
    agora::rtc::ColorEnhanceOptions opts(strengthLevel, skinProtectLevel);
    result = pEngine->m_engine->setColorEnhanceOptions(enabled, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

/*
 * 3.7.0
 */
NAPI_API_DEFINE(NodeRtcEngine, setScreenCaptureScenario) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int32_t screenScenario;
    status = napi_get_value_int32_(args[0], screenScenario);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->setScreenCaptureScenario(
        (SCREEN_SCENARIO_TYPE)screenScenario);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, videoSourceSetScreenCaptureScenario) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int32_t screenScenario;
    status = napi_get_value_int32_(args[0], screenScenario);
    CHECK_NAPI_STATUS(pEngine, status);

    if (pEngine->m_videoSourceSink.get()) {
      pEngine->m_videoSourceSink->setScreenCaptureScenario(screenScenario);
      result = 0;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableLocalVoicePitchCallback) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    int32_t interval;
    status = napi_get_value_int32_(args[0], interval);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->enableLocalVoicePitchCallback(interval);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableWirelessAccelerate) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    result = pEngine->m_engine->enableWirelessAccelerate(enabled);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableContentInspect) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  Isolate *isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);

    ContentInspectConfig config;
    nodestring extraInfo;
    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pEngine, status);

      status = napi_get_object_property_nodestring_(isolate, obj, "extraInfo",
                                                    extraInfo);
      CHECK_NAPI_STATUS(pEngine, status);
      config.extraInfo = extraInfo;

      v8::Array *modulesArr;
      status =
          napi_get_object_property_array_(isolate, obj, "modules", modulesArr);
      auto hasValue = status == napi_ok && modulesArr->Length() > 0;
      if (!hasValue) {
        status = napi_invalid_arg;
        CHECK_NAPI_STATUS(pEngine, status);
      }

      auto count = modulesArr->Length();
      config.moduleCount = count;

      for (uint32 i = 0; i < modulesArr->Length(); i++) {
        Local<Value> value = modulesArr->Get(context, i).ToLocalChecked();
        Local<Object> moduleObj;
        status = napi_get_value_object_(isolate, value, moduleObj);
        if (moduleObj->IsNullOrUndefined()) {
          status = napi_invalid_arg;
          CHECK_NAPI_STATUS(pEngine, status);
        }

        std::string key = "type";
        int32_t type;
        status = napi_get_object_property_int32_(isolate, moduleObj, key, type);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
        config.modules[i].type = (ContentInspectType)type;

        key = "interval";
        status = napi_get_object_property_int32_(isolate, moduleObj, key,
                                                 config.modules[i].interval);
        CHECK_NAPI_STATUS_PARAM(pEngine, status, key);
      }
      CHECK_NAPI_STATUS(pEngine, status);
    }

    result = pEngine->m_engine->enableContentInspect(enabled, config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableSpatialAudio) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pEngine, status);
    result = pEngine->m_engine->enableSpatialAudio(enabled);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, setRemoteUserSpatialAudioParams) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine *pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);
    uid_t uid;
    napi_status status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pEngine, status);
    media::SpatialAudioParams params;

    if (args[1]->IsObject()) {
      Local<Object> spatialAudioParamsObj;
      status = napi_get_value_object_(isolate, args[1], spatialAudioParamsObj);

      double speaker_azimuth, speaker_elevation, speaker_distance;
      int speaker_orientation;
      bool enable_blur, enable_air_absorb;

      status = napi_get_object_property_double_(
          isolate, spatialAudioParamsObj, "speaker_azimuth", speaker_azimuth);
      if (status == napi_ok) {
        params.speaker_azimuth = speaker_azimuth;
      }
      status = napi_get_object_property_double_(isolate, spatialAudioParamsObj,
                                                "speaker_elevation",
                                                speaker_elevation);
      if (status == napi_ok) {
        params.speaker_elevation = speaker_elevation;
      }

      status = napi_get_object_property_double_(
          isolate, spatialAudioParamsObj, "speaker_distance", speaker_distance);
      if (status == napi_ok) {
        params.speaker_distance = speaker_distance;
      }

      status = napi_get_object_property_int32_(isolate, spatialAudioParamsObj,
                                               "speaker_orientation",
                                               speaker_orientation);
      if (status == napi_ok) {
        params.speaker_orientation = speaker_orientation;
      }

      status = napi_get_object_property_bool_(isolate, spatialAudioParamsObj,
                                              "enable_blur", enable_blur);
      if (status == napi_ok) {
        params.enable_blur = enable_blur;
      }

      status = napi_get_object_property_bool_(isolate, spatialAudioParamsObj,
                                              "enable_air_absorb",
                                              enable_air_absorb);
      if (status == napi_ok) {
        params.enable_air_absorb = enable_air_absorb;
      }
    }

    result = pEngine->m_engine->setRemoteUserSpatialAudioParams(uid, params);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, isSeaxJoined) {
  LOG_ENTER;
  int result = 0;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    if (pEngine->m_seax_engine)
      result = pEngine->m_seax_engine->IsJoinedChannel();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, getAllSeaxDeviceList) {
  LOG_ENTER;
  Isolate* isolate = args.GetIsolate();
  NodeRtcEngine* pEngine = nullptr;
  napi_get_native_this(args, pEngine);
  Local<v8::Array> arrDevice;

  do {
    CHECK_NATIVE_THIS(pEngine);

    if (pEngine->m_seax_engine) {
      HandleScope scope(isolate);
      Local<Context> context = isolate->GetCurrentContext();

      std::list<seax::DeviceInfo> device_list;
      int ret = pEngine->m_seax_engine->GetAllDeviceList(device_list);
      if (ret != agora::ERR_OK)
        LOG_ERROR("get seax device list failed with %d\r\n", ret);

      arrDevice = v8::Array::New(isolate, device_list.size());
      int index = 0;
      for (auto& device : device_list) {
        Local<Object> obj = Object::New(isolate);
        NODE_SET_OBJ_PROP_String(isolate, obj, "id", device.device_id.c_str());
        NODE_SET_OBJ_PROP_Number(isolate, obj, "role", device.device_role);
        NODE_SET_OBJ_PROP_String(isolate, obj, "channel",
                                 device.channel_id.c_str());
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "uid", device.local_uid);
        NODE_SET_OBJ_PROP_UINT32(isolate, obj, "hostUid", device.host_uid);

        arrDevice->Set(context, index, obj);
        index++;
      }
    }
  } while (0);

  napi_set_array_result(args, arrDevice);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcEngine, enableSeaxAudioDump) {
  LOG_ENTER;
  do {
    Isolate *isolate = args.GetIsolate();
    NodeRtcEngine* pEngine = nullptr;
    napi_get_native_this(args, pEngine);
    CHECK_NATIVE_THIS(pEngine);

    nodestring dumpPath;
    napi_status status = napi_get_value_nodestring_(args[0], dumpPath);
    CHECK_NAPI_STATUS(pEngine, status);

    bool enable = false;
    status = napi_get_value_bool_(args[1], enable);
    CHECK_NAPI_STATUS(pEngine, status);

    if (pEngine->m_seax_engine)
      pEngine->m_seax_engine->EnableSeaxAudioDump((const char*)dumpPath, enable);

  } while (false);
  LOG_LEAVE;
}

/**
 * NodeRtcChannel
 */

/**
 * To declared class and member functions that could be used in JS layer
 * directly.
 */
Local<Object> NodeRtcChannel::Init(Isolate* isolate, IChannel* pChannel) {
  Local<Context> context = isolate->GetCurrentContext();
  BEGIN_PROPERTY_DEFINE(NodeRtcChannel, createInstance, 5)
  PROPERTY_METHOD_DEFINE(onEvent)
  PROPERTY_METHOD_DEFINE(joinChannel)
  PROPERTY_METHOD_DEFINE(joinChannelWithUserAccount)
  PROPERTY_METHOD_DEFINE(publish)
  PROPERTY_METHOD_DEFINE(unpublish)
  PROPERTY_METHOD_DEFINE(channelId)
  PROPERTY_METHOD_DEFINE(getCallId)
  PROPERTY_METHOD_DEFINE(renewToken)
  PROPERTY_METHOD_DEFINE(setEncryptionMode)
  PROPERTY_METHOD_DEFINE(setEncryptionSecret)
  PROPERTY_METHOD_DEFINE(setClientRole)
  PROPERTY_METHOD_DEFINE(setRemoteUserPriority)
  PROPERTY_METHOD_DEFINE(setRemoteVoicePosition)
  PROPERTY_METHOD_DEFINE(setRemoteRenderMode)
  PROPERTY_METHOD_DEFINE(setDefaultMuteAllRemoteAudioStreams)
  PROPERTY_METHOD_DEFINE(setDefaultMuteAllRemoteVideoStreams)
  PROPERTY_METHOD_DEFINE(muteAllRemoteAudioStreams)
  PROPERTY_METHOD_DEFINE(muteRemoteAudioStream)
  PROPERTY_METHOD_DEFINE(muteAllRemoteVideoStreams)
  PROPERTY_METHOD_DEFINE(muteRemoteVideoStream)
  PROPERTY_METHOD_DEFINE(setRemoteVideoStreamType)
  PROPERTY_METHOD_DEFINE(setRemoteDefaultVideoStreamType)
  PROPERTY_METHOD_DEFINE(createDataStream)
  PROPERTY_METHOD_DEFINE(sendStreamMessage)
  PROPERTY_METHOD_DEFINE(addPublishStreamUrl)
  PROPERTY_METHOD_DEFINE(removePublishStreamUrl)
  PROPERTY_METHOD_DEFINE(setLiveTranscoding)
  PROPERTY_METHOD_DEFINE(addInjectStreamUrl)
  PROPERTY_METHOD_DEFINE(removeInjectStreamUrl)
  PROPERTY_METHOD_DEFINE(startChannelMediaRelay)
  PROPERTY_METHOD_DEFINE(updateChannelMediaRelay)
  PROPERTY_METHOD_DEFINE(stopChannelMediaRelay)
  PROPERTY_METHOD_DEFINE(getConnectionState)
  PROPERTY_METHOD_DEFINE(leaveChannel)
  PROPERTY_METHOD_DEFINE(release)
  PROPERTY_METHOD_DEFINE(adjustUserPlaybackSignalVolume)

  PROPERTY_METHOD_DEFINE(sendMetadata);
  PROPERTY_METHOD_DEFINE(addMetadataEventHandler);
  PROPERTY_METHOD_DEFINE(setMaxMetadataSize);
  PROPERTY_METHOD_DEFINE(registerMediaMetadataObserver);
  PROPERTY_METHOD_DEFINE(unRegisterMediaMetadataObserver);
  PROPERTY_METHOD_DEFINE(enableEncryption);
  PROPERTY_METHOD_DEFINE(setClientRoleWithOptions);
  /*
   * 3.4.5
   */
  PROPERTY_METHOD_DEFINE(muteLocalAudioStream);
  PROPERTY_METHOD_DEFINE(muteLocalVideoStream);
  PROPERTY_METHOD_DEFINE(sendStreamMessageWithArrayBuffer);

  EN_PROPERTY_DEFINE()

  Local<Function> cons = tpl->GetFunction(context).ToLocalChecked();
  Local<v8::External> argChannel =
      Local<v8::External>::New(isolate, v8::External::New(isolate, pChannel));
  Local<v8::Value> argv[1] = {argChannel};
  Local<Object> jschannel =
      cons->NewInstance(context, 1, argv).ToLocalChecked();
  return jschannel;
}

/**
 * The function is used as class constructor in JS layer
 */
void NodeRtcChannel::createInstance(const FunctionCallbackInfo<Value>& args) {
  LOG_ENTER;
  Isolate* isolate = args.GetIsolate();

  Local<v8::External> argChannel = Local<v8::External>::Cast(args[0]);
  IChannel* pChannel = static_cast<IChannel*>(argChannel->Value());
  NodeRtcChannel* channel = new NodeRtcChannel(isolate, pChannel);
  channel->Wrap(args.This());
  args.GetReturnValue().Set(args.This());

  LOG_LEAVE;
}

/**
 * Constructor
 */
NodeRtcChannel::NodeRtcChannel(Isolate* isolate, IChannel* pChannel)
    : m_isolate(isolate), m_channel(pChannel) {
  LOG_ENTER;
  metadataObserver.reset(new NodeMetadataObserver());
  /** m_eventHandler provide SDK event handler. */
  m_eventHandler.reset(new NodeChannelEventHandler(this));

  m_channel->setChannelEventHandler(m_eventHandler.get());
  LOG_LEAVE;
}

NodeRtcChannel::~NodeRtcChannel() {
  LOG_ENTER;
  if (m_channel) {
    m_channel->release();
    m_channel = nullptr;
  }
  m_eventHandler.reset(nullptr);

  if (metadataObserver.get()) {
    metadataObserver.reset(nullptr);
  }
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, onEvent) {
  // LOG_ENTER;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    NodeString eventName;
    status = napi_get_value_nodestring_(args[0], eventName);
    CHECK_NAPI_STATUS(pChannel, status);

    if (!args[1]->IsFunction()) {
      LOG_ERROR("Function expected");
      break;
    }

    Local<Function> callback = args[1].As<Function>();
    if (callback.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }
    Persistent<Function> persist;
    persist.Reset(callback);
    Local<Object> obj = args.This();
    Persistent<Object> persistObj;
    persistObj.Reset(obj);
    pChannel->m_eventHandler->addEventHandler((char*)eventName, persistObj,
                                              persist);
  } while (false);
  // LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, joinChannel) {
  LOG_ENTER;
  int result = -1;
  NodeString key, name, chan_info;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    uid_t uid;
    napi_status status = napi_get_value_nodestring_(args[0], key);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_value_nodestring_(args[1], chan_info);
    CHECK_NAPI_STATUS(pChannel, status);

    status = NodeUid::getUidFromNodeValue(args[2], uid);
    CHECK_NAPI_STATUS(pChannel, status);

    Local<Value> vChannelMediaOptions = args[3];
    if (!vChannelMediaOptions->IsObject()) {
      pChannel->m_eventHandler->fireApiError(__FUNCTION__);
      break;
    }
    Local<Object> oChannelMediaOptions;
    status = napi_get_value_object_(isolate, vChannelMediaOptions,
                                    oChannelMediaOptions);
    CHECK_NAPI_STATUS(pChannel, status);

    ChannelMediaOptions options;
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "autoSubscribeAudio",
                                            options.autoSubscribeAudio);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "autoSubscribeVideo",
                                            options.autoSubscribeVideo);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "publishLocalAudio",
                                            options.publishLocalAudio);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "publishLocalVideo",
                                            options.publishLocalVideo);
    CHECK_NAPI_STATUS(pChannel, status);

    std::string extra_info = "";

    if (chan_info && strlen(chan_info) > 0) {
      extra_info = "Electron_";
      extra_info += chan_info;
    } else {
      extra_info = "Electron";
    }

    result =
        pChannel->m_channel->joinChannel(key, extra_info.c_str(), uid, options);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, joinChannelWithUserAccount) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    NodeString token, channel, userAccount;

    napi_status status = napi_get_value_nodestring_(args[0], token);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_value_nodestring_(args[1], userAccount);
    CHECK_NAPI_STATUS(pChannel, status);

    Local<Value> vChannelMediaOptions = args[2];
    if (!vChannelMediaOptions->IsObject()) {
      pChannel->m_eventHandler->fireApiError(__FUNCTION__);
      break;
    }
    Local<Object> oChannelMediaOptions;
    status = napi_get_value_object_(isolate, vChannelMediaOptions,
                                    oChannelMediaOptions);
    CHECK_NAPI_STATUS(pChannel, status);

    ChannelMediaOptions options;
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "autoSubscribeAudio",
                                            options.autoSubscribeAudio);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "autoSubscribeVideo",
                                            options.autoSubscribeVideo);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "publishLocalAudio",
                                            options.publishLocalAudio);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_bool_(isolate, oChannelMediaOptions,
                                            "publishLocalVideo",
                                            options.publishLocalVideo);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->joinChannelWithUserAccount(token, userAccount,
                                                             options);
  } while (false);
  napi_set_array_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, channelId) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    const char* channelId = pChannel->m_channel->channelId();
    napi_set_string_result(args, channelId);
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, getCallId) {
  LOG_ENTER;
  napi_status status = napi_ok;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    util::AString callId;
    if (-ERR_FAILED != pChannel->m_channel->getCallId(callId)) {
      napi_set_string_result(args, callId->c_str());
    }
  } while (false);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setClientRole) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    unsigned int role;
    napi_status status = napi_get_value_uint32_(args[0], role);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->setClientRole(CLIENT_ROLE_TYPE(role));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setRemoteUserPriority) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    uid_t uid;
    napi_status status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pChannel, status);

    unsigned int priority = 100;
    status = napi_get_value_uint32_(args[1], priority);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->setRemoteUserPriority(
        uid, PRIORITY_TYPE(priority));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setRemoteRenderMode) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    uid_t uid;
    status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pChannel, status);

    unsigned int renderMode;
    status = napi_get_value_uint32_(args[1], renderMode);
    CHECK_NAPI_STATUS(pChannel, status);

    unsigned int mirrorMode;
    status = napi_get_value_uint32_(args[2], mirrorMode);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->setRemoteRenderMode(
        uid, RENDER_MODE_TYPE(renderMode), VIDEO_MIRROR_MODE_TYPE(mirrorMode));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_CHANNEL_DEFINE_WRAPPER_1(renewToken, nodestring);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(setEncryptionSecret, nodestring);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(setEncryptionMode, nodestring);
NAPI_API_CHANNEL_DEFINE_WRAPPER_3(setRemoteVoicePosition,
                                  int32,
                                  double,
                                  double);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(setDefaultMuteAllRemoteAudioStreams, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(setDefaultMuteAllRemoteVideoStreams, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(muteAllRemoteAudioStreams, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_2(muteRemoteAudioStream, uid_t, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(muteAllRemoteVideoStreams, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_2(muteRemoteVideoStream, uid_t, bool);

NAPI_API_DEFINE(NodeRtcChannel, setRemoteVideoStreamType) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    uid_t uid;
    status = napi_get_value_uid_t_(args[0], uid);
    CHECK_NAPI_STATUS(pChannel, status);

    unsigned int streamType;
    status = napi_get_value_uint32_(args[1], streamType);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->setRemoteVideoStreamType(
        uid, REMOTE_VIDEO_STREAM_TYPE(streamType));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setRemoteDefaultVideoStreamType) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    unsigned int streamType;
    status = napi_get_value_uint32_(args[0], streamType);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->setRemoteDefaultVideoStreamType(
        REMOTE_VIDEO_STREAM_TYPE(streamType));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, createDataStream) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;
    int streamId;
    if (!args[0]->IsObject()) {
      bool reliable, ordered;
      napi_get_param_2(args, bool, reliable, bool, ordered);
      CHECK_NAPI_STATUS(pChannel, status);
      result =
          pChannel->m_channel->createDataStream(&streamId, reliable, ordered);
    } else {
      Local<Object> obj;
      DataStreamConfig config;
      status = napi_get_value_object_(isolate, args[0], obj);
      CHECK_NAPI_STATUS(pChannel, status);
      status = napi_get_object_property_bool_(isolate, obj, "syncWithAudio",
                                              config.syncWithAudio);
      CHECK_NAPI_STATUS(pChannel, status);
      status = napi_get_object_property_bool_(isolate, obj, "ordered",
                                              config.ordered);
      CHECK_NAPI_STATUS(pChannel, status);
      result = pChannel->m_channel->createDataStream(&streamId, config);
    }

    if (result == 0) {
      result = streamId;
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, sendStreamMessage) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    NodeString msg;
    int streamId;
    status = napi_get_value_int32_(args[0], streamId);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_value_nodestring_(args[1], msg);
    CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->m_channel->sendStreamMessage(streamId, msg, strlen(msg));
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_CHANNEL_DEFINE_WRAPPER_2(addPublishStreamUrl, nodestring, bool);
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(removePublishStreamUrl, nodestring);
NAPI_API_DEFINE(NodeRtcChannel, setLiveTranscoding) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);

    Local<Object> obj;
    napi_status status =
        napi_get_value_object_(args.GetIsolate(), args[0], obj);
    CHECK_NAPI_STATUS_PARAM(pChannel, status,
                            std::string("arg 0 is not a object"));

    auto liveTranscoding = getLiveTranscoding(obj, args, pChannel);
    if (liveTranscoding == nullptr) {
      break;
    }
    result = pChannel->m_channel->setLiveTranscoding(*liveTranscoding);
    if (liveTranscoding->watermark) {
      for (unsigned int i = 0; i < liveTranscoding->watermarkCount; i++) {
        delete liveTranscoding->watermark[i].url;
      }
      delete[] liveTranscoding->watermark;
    }
    if (liveTranscoding->backgroundImage) {
      for (unsigned int i = 0; i < liveTranscoding->backgroundImageCount; i++) {
        delete liveTranscoding->backgroundImage[i].url;
      }
      delete[] liveTranscoding->backgroundImage;
    }
    if (liveTranscoding->transcodingUsers) {
      delete[] liveTranscoding->transcodingUsers;
    }
    if (liveTranscoding->advancedFeatures) {
      for (unsigned int i = 0; i < liveTranscoding->advancedFeatureCount; i++) {
        delete liveTranscoding->advancedFeatures[i].featureName;
      }
      delete[] liveTranscoding->advancedFeatures;
    }
    delete liveTranscoding;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, addInjectStreamUrl) {
  LOG_ENTER;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    nodestring url;
    InjectStreamConfig config;
    status = napi_get_value_nodestring_(args[0], url);
    CHECK_NAPI_STATUS(pChannel, status);

    if (!args[1]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pChannel, status);
    }
    Local<Object> configObj;
    status = napi_get_value_object_(isolate, args[1], configObj);
    CHECK_NAPI_STATUS(pChannel, status);

    int audioSampleRate;
    status = napi_get_object_property_int32_(isolate, configObj, "width",
                                             config.width);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(isolate, configObj, "height",
                                             config.height);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(isolate, configObj, "videoGop",
                                             config.videoGop);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "videoFramerate", config.videoFramerate);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(isolate, configObj, "videoBitrate",
                                             config.videoBitrate);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "audioSampleRate", audioSampleRate);
    CHECK_NAPI_STATUS(pChannel, status);
    config.audioSampleRate = AUDIO_SAMPLE_RATE_TYPE(audioSampleRate);

    status = napi_get_object_property_int32_(isolate, configObj, "audioBitrate",
                                             config.audioBitrate);
    CHECK_NAPI_STATUS(pChannel, status);

    status = napi_get_object_property_int32_(
        isolate, configObj, "audioChannels", config.audioChannels);
    CHECK_NAPI_STATUS(pChannel, status);

    result = pChannel->m_channel->addInjectStreamUrl(url, config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
NAPI_API_CHANNEL_DEFINE_WRAPPER_1(removeInjectStreamUrl, nodestring);

NAPI_API_DEFINE(NodeRtcChannel, startChannelMediaRelay) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    Local<Context> context = isolate->GetCurrentContext();
    ChannelMediaRelayConfiguration config;
    Local<Object> obj;

    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pChannel, status);

    string sourceChannel, sourceToken;
    std::vector<string> destChannels, destTokens;
    std::vector<ChannelMediaInfo> destInfos;

    // srcInfo
    Local<Name> keyName = String::NewFromUtf8(args.GetIsolate(), "srcInfo",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Value> srcInfoValue =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    ChannelMediaInfo srcInfo;
    if (!srcInfoValue->IsNullOrUndefined()) {
      NodeString channelName, token;
      Local<Object> objSrcInfo =
          srcInfoValue->ToObject(context).ToLocalChecked();
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), objSrcInfo, "uid",
                                    srcInfo.uid);

      if (channelName != nullptr) {
        sourceChannel = string(channelName);
        srcInfo.channelName = sourceChannel.c_str();
      } else {
        srcInfo.channelName = nullptr;
      }
      if (token != nullptr) {
        sourceToken = (string)token;
        srcInfo.token = sourceToken.c_str();
      } else {
        srcInfo.token = nullptr;
      }
    }

    LOG_F(INFO, "startChannelMediaRelay src channelName: %s, token : %s",
          srcInfo.channelName, srcInfo.token);
    // destInfos
    keyName = String::NewFromUtf8(args.GetIsolate(), "destInfos",
                                  NewStringType::kInternalized)
                  .ToLocalChecked();
    Local<Value> objDestInfos =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    if (objDestInfos->IsNullOrUndefined() || !objDestInfos->IsArray()) {
      status = napi_invalid_arg;
      break;
    }
    auto destInfosValue = v8::Array::Cast(*objDestInfos);
    int destInfoCount = destInfosValue->Length();
    destInfos.resize(destInfoCount);
    destChannels.resize(destInfoCount);
    destTokens.resize(destInfoCount);
    for (uint32 i = 0; i < destInfoCount; i++) {
      // ChannelMediaInfo destInfo;
      Local<Value> value = destInfosValue->Get(context, i).ToLocalChecked();
      Local<Object> destInfoObj = value->ToObject(context).ToLocalChecked();
      if (destInfoObj->IsNullOrUndefined()) {
        status = napi_invalid_arg;
        break;
      }
      NodeString channelName, token;
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), destInfoObj, "uid",
                                    destInfos[i].uid);
      if (channelName) {
        destChannels[i] = string(channelName);
        destInfos[i].channelName = destChannels[i].c_str();
      } else {
        destChannels[i] = "";
      }
      if (token) {
        destTokens[i] = string(token);
        destInfos[i].token = destTokens[i].c_str();
      } else {
        destTokens[i] = "";
      }
    }
    config.srcInfo = &srcInfo;
    config.destInfos = &destInfos[0];
    config.destCount = destInfoCount;
    for (int i = 0; i < destInfoCount; i++) {
      LOG_F(INFO,
            "startChannelMediaRelay src channelName: %s, token: %s,  dest "
            "channelName: %s, token : %s",
            config.srcInfo->channelName, config.srcInfo->token,
            config.destInfos[i].channelName, config.destInfos[i].token);
    }
    result = pChannel->m_channel->startChannelMediaRelay(config);

  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, updateChannelMediaRelay) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    Local<Context> context = isolate->GetCurrentContext();

    ChannelMediaRelayConfiguration config;
    Local<Object> obj;
    napi_status status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pChannel, status);

    string sourceChannel, sourceToken;
    std::vector<string> destChannels, destTokens;
    std::vector<ChannelMediaInfo> destInfos;

    // srcInfo
    Local<Name> keyName = String::NewFromUtf8(args.GetIsolate(), "srcInfo",
                                              NewStringType::kInternalized)
                              .ToLocalChecked();
    Local<Value> srcInfoValue =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    ChannelMediaInfo srcInfo;
    if (!srcInfoValue->IsNullOrUndefined()) {
      NodeString channelName, token;
      Local<Object> objSrcInfo =
          srcInfoValue->ToObject(context).ToLocalChecked();
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), objSrcInfo,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), objSrcInfo, "uid",
                                    srcInfo.uid);

      if (channelName != nullptr) {
        sourceChannel = string(channelName);
        srcInfo.channelName = sourceChannel.c_str();
      } else {
        srcInfo.channelName = nullptr;
      }
      if (token != nullptr) {
        sourceToken = (string)token;
        srcInfo.token = sourceToken.c_str();
      } else {
        srcInfo.token = nullptr;
      }
    }

    // destInfos
    keyName = String::NewFromUtf8(args.GetIsolate(), "destInfos",
                                  NewStringType::kInternalized)
                  .ToLocalChecked();
    Local<Value> objDestInfos =
        obj->Get(args.GetIsolate()->GetCurrentContext(), keyName)
            .ToLocalChecked();
    if (objDestInfos->IsNullOrUndefined() || !objDestInfos->IsArray()) {
      status = napi_invalid_arg;
      break;
    }
    auto destInfosValue = v8::Array::Cast(*objDestInfos);
    int destInfoCount = destInfosValue->Length();
    destInfos.resize(destInfoCount);
    destChannels.resize(destInfoCount);
    destTokens.resize(destInfoCount);
    for (uint32 i = 0; i < destInfoCount; i++) {
      // ChannelMediaInfo destInfo;
      Local<Value> value = destInfosValue->Get(context, i).ToLocalChecked();
      Local<Object> destInfoObj = value->ToObject(context).ToLocalChecked();
      if (destInfoObj->IsNullOrUndefined()) {
        status = napi_invalid_arg;
        break;
      }
      NodeString channelName, token;
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "channelName", channelName);
      napi_get_object_property_nodestring_(args.GetIsolate(), destInfoObj,
                                           "token", token);
      napi_get_object_property_uid_(args.GetIsolate(), destInfoObj, "uid",
                                    destInfos[i].uid);
      if (channelName) {
        destChannels[i] = string(channelName);
        destInfos[i].channelName = destChannels[i].c_str();
      } else {
        destChannels[i] = "";
      }
      if (token) {
        destTokens[i] = string(token);
        destInfos[i].token = destTokens[i].c_str();
      } else {
        destTokens[i] = "";
      }
    }
    config.srcInfo = &srcInfo;
    config.destInfos = &destInfos[0];
    config.destCount = destInfoCount;

    result = pChannel->m_channel->updateChannelMediaRelay(config);
    for (int i = 0; i < destInfoCount; i++) {
      LOG_F(INFO,
            "updateChannelMediaRelay src channelName: %s, token: %s,  dest "
            "channelName: %s, token : %s",
            config.srcInfo->channelName, config.srcInfo->token,
            config.destInfos[i].channelName, config.destInfos[i].token);
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}
NAPI_API_CHANNEL_DEFINE_WRAPPER(stopChannelMediaRelay);

NAPI_API_DEFINE(NodeRtcChannel, getConnectionState) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status;

    result = pChannel->m_channel->getConnectionState();
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_CHANNEL_DEFINE_WRAPPER(publish);
NAPI_API_CHANNEL_DEFINE_WRAPPER(unpublish);
NAPI_API_CHANNEL_DEFINE_WRAPPER(leaveChannel);

NAPI_API_DEFINE(NodeRtcChannel, release) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    if (pChannel->m_channel) {
      pChannel->m_channel->release();
      pChannel->m_channel = nullptr;
    }

    result = 0;
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

/**
 * 3.0.0 Apis
 */
NAPI_API_CHANNEL_DEFINE_WRAPPER_2(adjustUserPlaybackSignalVolume, uid_t, int32);

NAPI_API_DEFINE(NodeRtcChannel, registerMediaMetadataObserver) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    LOG_F(INFO, "NodeRtcChannel  registerMediaMetadataObserver");
    if (!(pChannel->metadataObserver.get())) {
      pChannel->metadataObserver.reset(new NodeMetadataObserver());
    }
    result = pChannel->m_channel->registerMediaMetadataObserver(
        pChannel->metadataObserver.get(),
        IMetadataObserver::METADATA_TYPE::VIDEO_METADATA);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, unRegisterMediaMetadataObserver) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    LOG_F(INFO, "NodeRtcChannel unRegisterMediaMetadataObserver");
    result = pChannel->m_channel->registerMediaMetadataObserver(
        nullptr, IMetadataObserver::METADATA_TYPE::VIDEO_METADATA);
    if (pChannel->metadataObserver.get()) {
      pChannel->metadataObserver.get()->clearData();
    }
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, sendMetadata) {
  LOG_ENTER;
  napi_status status = napi_ok;
  int result = -1;
  do {
    Isolate* isolate = args.GetIsolate();
    NodeRtcChannel* pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    if (!args[0]->IsObject()) {
      status = napi_invalid_arg;
      CHECK_NAPI_STATUS(pChannel, status);
    }

    if (!pChannel->metadataObserver.get()) {
      result = -100;
      break;
    }

    unsigned int uid = 0;
    unsigned int size;
    nodestring buffer;
    double timeStampMs = 0;
    char* _buffer;

    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[0], obj);
    CHECK_NAPI_STATUS(pChannel, status);
    // status = napi_get_object_property_uid_(isolate, obj, "uid", uid);
    // CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_uid_(isolate, obj, "size", size);
    CHECK_NAPI_STATUS(pChannel, status);
    status =
        napi_get_object_property_nodestring_(isolate, obj, "buffer", buffer);
    _buffer = buffer;
    CHECK_NAPI_STATUS(pChannel, status);
    // status = napi_get_object_property_double_(isolate, obj, "timeStampMs",
    // timeStampMs); CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->metadataObserver.get()->sendMetadata(
        uid, size, reinterpret_cast<unsigned char*>(_buffer), timeStampMs);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, addMetadataEventHandler) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    if (!args[0]->IsFunction()) {
      LOG_ERROR("Function expected");
      break;
    }

    Local<Function> callback = args[0].As<Function>();
    if (callback.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }

    Local<Function> callback2 = args[1].As<Function>();
    if (callback2.IsEmpty()) {
      LOG_ERROR("Function expected.");
      break;
    }

    Persistent<Function> persist;
    persist.Reset(callback);

    Persistent<Function> persist2;
    persist2.Reset(callback2);

    Local<Object> obj = args.This();
    Persistent<Object> persistObj;
    persistObj.Reset(obj);
    result = pChannel->metadataObserver->addEventHandler(persistObj, persist,
                                                         persist2);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setMaxMetadataSize) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    int maxSize;
    status = napi_get_value_int32_(args[0], maxSize);
    CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->metadataObserver.get()->setMaxMetadataSize(maxSize);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, enableEncryption) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    Isolate* isolate = args.GetIsolate();
    bool enabled;
    status = napi_get_value_bool_(args[0], enabled);
    CHECK_NAPI_STATUS(pChannel, status);

    int encryptionMode;
    Local<Object> obj;
    status = napi_get_value_object_(isolate, args[1], obj);
    CHECK_NAPI_STATUS(pChannel, status);
    status = napi_get_object_property_int32_(isolate, obj, "encryptionMode",
                                             encryptionMode);
    CHECK_NAPI_STATUS(pChannel, status);

    nodestring encryptionKey;
    status = napi_get_object_property_nodestring_(isolate, obj, "encryptionKey",
                                                  encryptionKey);
    CHECK_NAPI_STATUS(pChannel, status);

    EncryptionConfig config;

    napi_get_object_property_arraybuffer_(isolate, obj, "encryptionKdfSalt",
                                          config.encryptionKdfSalt);
    CHECK_NAPI_STATUS(pChannel, status);

    config.encryptionMode = (ENCRYPTION_MODE)encryptionMode;
    config.encryptionKey = encryptionKey;
    result = pChannel->m_channel->enableEncryption(enabled, config);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, setClientRoleWithOptions) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);

    Isolate* isolate = args.GetIsolate();
    unsigned int role;
    status = napi_get_value_uint32_(args[0], role);
    CHECK_NAPI_STATUS(pChannel, status);

    ClientRoleOptions opts;

    if (args[1]->IsObject()) {
      Local<Object> obj;
      status = napi_get_value_object_(isolate, args[1], obj);
      CHECK_NAPI_STATUS(pChannel, status);

      int audienceLatencyLevel = (int)AUDIENCE_LATENCY_LEVEL_ULTRA_LOW_LATENCY;

      status = napi_get_object_property_int32_(
          isolate, obj, "audienceLatencyLevel", audienceLatencyLevel);
      CHECK_NAPI_STATUS(pChannel, status);

      switch (audienceLatencyLevel) {
        case 1:
          opts.audienceLatencyLevel = AUDIENCE_LATENCY_LEVEL_LOW_LATENCY;
          break;
        case 2:
          opts.audienceLatencyLevel = AUDIENCE_LATENCY_LEVEL_ULTRA_LOW_LATENCY;
          break;
        default:
          status = napi_invalid_arg;
          break;
      }
      CHECK_NAPI_STATUS(pChannel, status);
    }
    result = pChannel->m_channel->setClientRole((CLIENT_ROLE_TYPE)role, opts);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, muteLocalAudioStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    bool mute;
    status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->m_channel->muteLocalAudioStream(mute);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, muteLocalVideoStream) {
  LOG_ENTER;
  int result = -1;
  do {
    NodeRtcChannel* pChannel = nullptr;
    napi_status status = napi_ok;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    bool mute;
    status = napi_get_value_bool_(args[0], mute);
    CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->m_channel->muteLocalVideoStream(mute);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

NAPI_API_DEFINE(NodeRtcChannel, sendStreamMessageWithArrayBuffer) {
  LOG_ENTER;
  std::vector<uint8_t> buffer;
  int result = -1;
  uint32_t length = 0;
  do {
    NodeRtcChannel *pChannel = nullptr;
    napi_get_native_channel(args, pChannel);
    CHECK_NATIVE_CHANNEL(pChannel);
    napi_status status = napi_ok;
    int streamId;
    status = napi_get_value_int32_(args[0], streamId);
    CHECK_NAPI_STATUS(pChannel, status);
    napi_get_value_arraybuffer_(args[1], buffer, length);
    CHECK_NAPI_STATUS(pChannel, status);
    result = pChannel->m_channel->sendStreamMessage(
        streamId, (const char *)buffer.data(), length);
  } while (false);
  napi_set_int_result(args, result);
  LOG_LEAVE;
}

}  // namespace rtc
}  // namespace agora

#if defined(_WIN32)
/*
 * '_cups_strlcpy()' - Safely copy two strings.
 */

size_t                   /* O - Length of string */
strlcpy(char* dst,       /* O - Destination string */
        const char* src, /* I - Source string */
        size_t size)     /* I - Size of destination string buffer */
{
  size_t srclen; /* Length of source string */

  /*
   * Figure out how much room is needed...
   */
  size--;

  srclen = strlen(src);

  /*
   * Copy the appropriate amount...
   */

  if (srclen > size)
    srclen = size;

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';

  return (srclen);
}

#endif
