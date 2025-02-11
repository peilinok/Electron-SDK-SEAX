## 3.7.0-build.527(May 26th, 2022)
- delete api
  - pauseAudio
  - resumeAudio
- replace impl
  - setLocalVoiceChanger
  - setLocalVoiceReverbPreset
  - enableSoundPositionIndication
  - setRemoteVoicePosition
  - setCameraCapturerConfiguration
  - setLogFileSize
  - setExternalAudioSource
  - setLocalVideoMirrorMode
  - enableLoopbackRecording
  - setLocalVoiceEqualization
  - setLocalVoiceReverb
  - setLocalPublishFallbackOption
  - setRemoteSubscribeFallbackOption
  - muteRemoteAudioStream
  - muteRemoteVideoStream
  - setRemoteVideoStreamType
  - setRemoteDefaultVideoStreamType
  - startAudioMixing
  - getAudioMixingCurrentPosition
  - getAudioMixingPlayoutVolume
  - getAudioMixingPublishVolume
  - setRecordingAudioFrameParameters
  - setPlaybackAudioFrameParameters
  - setMixedAudioFrameParameters
  - setHighQualityAudioParameters
- Replace the macro definition
  - stopAudioRecording
  - stopAudioMixing
  - pauseAudioMixing
  - resumeAudioMixing
  - getEffectsVolume
  - setEffectsVolume
  - setVolumeOfEffect
  - stopEffect
  - stopAllEffects
  - preloadEffect
  - unloadEffect
  - pauseEffect
  - pauseAllEffects
  - resumeEffect
  - resumeAllEffects
  - muteLocalAudioStream
  - muteAllRemoteAudioStreams
  - setDefaultMuteAllRemoteAudioStreams
  - muteLocalVideoStream
  - enableLocalVideo
  - muteAllRemoteVideoStreams
  - setDefaultMuteAllRemoteVideoStreams
  - adjustAudioMixingVolume
  - adjustAudioMixingPlayoutVolume
  - adjustAudioMixingPublishVolume
  - setAudioMixingPosition
  - setLocalVoicePitch
  - setExternalAudioSink
  - setLogFile
  - setLogFilter
  - enableDualStreamMode
  - adjustRecordingSignalVolume
  - adjustPlaybackSignalVolume
  - enableWebSdkInteroperability
  - setVideoQualityParameters
- videoSource mute remote audio after join

## 3.7.0-build.519(May 19th, 2022)
- fix load for mac Extension
- add sendStreamMessageWithArrayBuffer

## 3.7.0-build.430(April 30th, 2022)
- update native sdk
- add api: 
  - setScreenCaptureScenario
  - enableLocalVoicePitchCallback
  - enableWirelessAccelerate
  - enableContentInspect
  - enableSpatialAudio
  - setRemoteUserSpatialAudioParams
- add event:
  - Engine->localVoicePitchInHz
  - Engine->clientRoleChangeFailed
  - Engine->wlAccMessage
  - Engine->wlAccStats
  - Engine->contentInspectResult
  - Engine->proxyConnected
  - Channel->firstRemoteVideoFrame
- fix ipc send


## 3.6.1-rc.4-build.406(April 6th, 2022)
- add api: videoSourceSetLocalAccessPoint

## 3.6.1-rc.4-build.329(Mar 29th, 2022)
- update native sdk

## 3.6.1-rc.4-build.310(Mar 10th, 2022)
- update native sdk

## 3.6.1-rc.3-build.301(Mar 1th, 2022)
- add api: setLocalAccessPoint

## 3.6.1-rc.3-build.228(Jan 28th, 2022)
- change api: getScreenWindowsInfo getScreenDisplaysInfo getRealScreenDisplaysInfo
- update native sdk

## 3.6.1-rc.1-build.126(Jan 26th, 2021)
- fix doc
- add api: startEchoTestWithConfig

## 3.6.1-rc.1-build.122(Jan 22th, 2021)
- RtcImage not pass alpha and zOrder

## 3.6.1-rc.1-build.121(Jan 21th, 2021)
- fix API compatibility for windows
- update native sdk

## 3.6.0-rc.2-build.119(Jan 18th, 2021)
- fix getLiveTranscoding

## 3.6.0-rtc.2-build.114(Jan 14th, 2021)
- Vulnerability check and fix for dependent libraries: `npm audit fix --force`

## 3.6.0-rtc.2-build.111(Jan 11th, 2021)
- fix dll import for windows

## 3.6.0-rtc.2-build.110(Jan 10th, 2021)
- update native sdk

## 3.6.0-build.1230(Dec 29th, 2021)
- fix 
  - ConvertRGBToBMP for c++
  - videoSourceRelease
- modify
  - getScreenWindowInfo for chinese
  - videoSourceJoin

## 3.6.0-build.1216(Dec 16th, 2021)
* add new api:
  - setLowlightEnhanceOptions
  - setVideoDenoiserOptions
  - setColorEnhanceOptions
* add event:
  - audioDeviceTestVolumeIndication

## 3.6.0-build.1216(Dec 16th, 2021)
* add new api:
  - videoSourceDisableAudio
  - adjustLoopbackSignalVolume
  - videoSourceAdjustRecordingSignalVolume
  - videoSourceAdjustLoopbackRecordingSignalVolume
  - getDefaultAudioPlaybackDevices
  - getDefaultAudioRecordingDevices
  - getAudioTrackCount
  - selectAudioTrack
  - takeSnapshot
  - startRtmpStreamWithoutTranscoding
  - startRtmpStreamWithTranscoding
  - updateRtmpTranscoding
  - stopRtmpStream
  - setAVSyncSource
  - followSystemPlaybackDevice
  - followSystemRecordingDevice
  - getScreenCaptureSources
* modify
 - setBeautyEffectOptions
* add event:
  - videoSourceScreenCaptureInfoUpdated(For windows)

## 3.4.11-build.1202(Dec 2th, 2021)
* remove getAudioMixingFileDuration
* add getScreenCaptureSources

## 3.5.1-build.1207(Dec 7th, 2021)
* add new api:
  - videoSourceMuteRemoteAudioStream
  - videoSourceMuteRemoteVideoStream
  - videoSourceMuteAllRemoteAudioStreams
  - videoSourceMuteAllRemoteVideoStreams
* fix videoSource

## 3.5.1-build.1123 (Nov 23th, 2021)
* modify
 - getScreenDisplaysInfo
 - getRealScreenDisplaysInfo
 - getScreenWindowsInfo

## 3.5.1-build.1116 (Nov 15th, 2021)
* add new api:
  - pauseAllChannelMediaRelay
  - resumeAllChannelMediaRelay
  - setAudioMixingPlaybackSpeed
  - setAudioMixingDualMonoMode
* modify getAudioMixingFileDuration to getAudioFileInfo

## 3.5.0-rc.4-build.1105 (Nov 8th, 2021)
* Improve sdk stabilit

## 3.5.0.4-build.1013 (Oct 13th, 2021)
* Improve sdk stability

## 3.4.6-build.727 (July 27th, 2021)
* support Electron 12.0.15

## 3.4.2-rc.1-build.615 (June 15th, 2021)
* modify:
  - add areaCode for videoSourceInitialize

## 3.4.2 (May 17th, 2021)

## 3.4.1 (April 26st, 2021)
* add new api:
  - adjustLoopbackRecordingSignalVolume
  - getEffectDuration
  - setEffectPosition
  - getEffectCurrentPosition

# 3.3.1 (April 14th, 2021)
* Joining the channel occasionally failed when a user switched back to the app after keeping it in the background for a long time.
* Remote users saw a distorted image when the local user minimized the shared screen with the genie effect on Mac devices.

# 3.3.1 (Mar 12th, 2021)
* add new api:
  - setVoiceConversionPreset
  - add localAudioStateChanged, localVideoStateChanged for videoSource
  - add createDataStreamWithConfig
* modify setLiveTranscoding background to backgroundImage

## 3.3.0-build.1220 (Feb 1st, 2021)
* add new api:
  - setCloudProxy
  - enableDeepLearningDenoise
  - setVoiceBeautifierParameters
  - uploadLogFile

## 3.2.1.71 (December 29th, 2020)
* Support Electron 9.0.0, 10.2.0

## 3.2.0 （December 1st, 2020）
* Audio profile: To improve the audio experience for multi-person meetings, this release adds AUDIO_SCENARIO_MEETING(8) in setAudioProfile.
* Screen sharing:
* To allow a user to enable shared slides in presentation mode, this release adds support for enabling the shared window (such as slides, web video, or web document) in full-screen mode during the window sharing.
* This release adds the LOCAL_VIDEO_STREAM_ERROR_SCREEN_CAPTURE_WINDOW_CLOSED(12) error code, notifying you that a window shared by the window ID has been closed, or a full-screen window shared by the window ID * * has exited full-screen mode.
* This release optimizes smoothness and image quality of screen sharing with the CONTENT_HINT_DETAILS type in a poor network environment.
* To improve the usability of the APIs related to voice beautifier and audio effects, this release deprecates setLocalVoiceChanger and setLocalVoiceReverbPreset, and adds the following methods instead:
* setVoiceBeautifierPreset: Compared with setLocalVoiceChanger, this method deletes audio effects such as a little boy’s voice and a more spatially resonant voice.
* setAudioEffectPreset: Compared with setLocalVoiceReverbPreset, this method adds audio effects such as the 3D voice, the pitch correction, a little boy’s voice and a more spatially resonant voice.
* setAudioEffectParameters: This method sets detailed parameters for a specified audio effect. In this release, the supported audio effects are the 3D voice and pitch correction.


## 3.1.1-hotfix.2 (September 21th, 2020)
#### :house: Intrenal
* 3.1.1 hotfix
* fix bug of videoSource screen share

## 3.1.1-hotfix.1 (September 17th, 2020)
#### :house: Intrenal
* 3.1.1 hotfix
* Add new api for screen share
  - videoSourceEnableEncryption
  - videoSourceSetEncryptionMode
  - videoSourceSetEncryptionSecret

## 3.1.1-build.914 (August 28th, 2020)
#### :house: Intrenal
* 3.1.1 SDK upgrade
* Add new parameter for CaptureParam:
  - windowFocus (Whether to bring the window to the front)
  - excludeWindowList (A list of IDs of windows to be blocked.)
  - excludeWindowCount (The number of windows to be blocked.)

## 3.0.1-beta.1 (June 23rd, 2020)
#### :house: Internal
* 3.0.1 SDK upgrade
* support multi render
* support electron 9.0.0

## 3.0.0-build.473 (April 22nd, 2020)
#### :house: Internal
* make glError configurable to improve performance when it's off


## 3.0.0 (March 20th, 2020)
#### :house: Internal
* Update Native SDK to v3.0.0 with lots of new features and optimization. You can go to [office doc](https://docs.agora.io/en/Interactive%20Broadcast/release_electron_video?platform=Electron) for detail.

## 2.9.0-rc.102-build.0314
* Support Win64

## 2.9.0-rc.102 (Mar 2nd, 2020)
* Win SDK upgrade to 2.9.0.102 build 3151
* Mac SDK upgrade to 2.9.0.102 build 1279
* Add new APIs:
  - getEffectCurrentPosition
  - setEffectPosition
  - getEffectDuration
  - adjustEffectPlayoutVolume
  - adjustEffectPublishVolume
  - getEffectPlayoutVolume
  - getEffectPublishVolume
* Support new Plugin APIs:
  - getParameter
* Plugin now supports int return value for potential error handling

## 2.9.0-rc.101 (Feb 14th, 2020)
* Added videoSourceEnableAudio & videoSourceEnableLoopbackRecording
* Release

## 2.9.0-rc.101-beta.1 (Jan 18th, 2020)
* Update Windows SDK to v2.9.0.101 build 86
* Update Mac SDK to v2.9.0.101 build 1001
* Fixed videosource token issue
* Support 6.1.7 & 7.1.2

## 2.9.0-hotfix.2 (Sep 17th, 2019)
#### :bug: Bug Fix
* Fixed the issue that plugin won't load when there's chinese character in the plugin path

## 2.9.0
#### :house: Internal
* Update Native SDK to v2.9.0 with lots of new features and optimization. You can go to [office doc](https://docs.agora.io/en/Interactive%20Broadcast/release_electron_video?platform=Electron) for detail.
* Fixed firstRemoteVideoDecoded event, before as an replacement addStream will be fired

## 2.8.0 (July 10th, 2019)
#### :house: Internal
* Update Native SDK to v2.8.0 with lots of new features and optimization. You can go to [office doc](https://docs.agora.io/en/Interactive%20Broadcast/release_electron_video?platform=Electron) for detail.
* Fixed aspect ratio issue when stream rotation is 90

#### :memo: Documentation
* Document revision by [kelzr](https://github.com/kelzr)


## 2.4.1-alpha (June 24th, 2019)
#### :house: Internal
* setRenderMode can recv 3 as param which refers to use custom Renderer (set with new API `setCustomRenderer`)
* add new Api `enableLocalAudio` which is similar to `enableLocalVideo`
* rename RemoteVideoStats.receivedFrameRate to `rendererOutputFrameRate`
* These Apis will be deprecated in a few versions:
  * startScreenCapture
  * startScreenCapture2 => videoSourceStartScreenCaptureBy(Screen|Window)
  * pauseAudio => disableAudio
  * resumeAudio => enableAudio
  * setHighQualityAudioParameters => setAudioProfile

#### :warning: Breaking Changes
* These event will be removed
  * connectionInterrupted
  * connectionBanned
  * audioQuality
  * audioMixingFinished
  * refreshRecordingServiceStatus

#### ::arrow_up:: SDK Upgrade
* Use 2.4.1 native sdk


## 2.4.0-beta.2 (May 9th, 2019)
#### :house: Internal
* Add codes to prevent videosource from getting camera causing problems in windows
* Add api `refreshRender`. Support instantly refreshing view when view size is changed. Useful for low frame rates
* Update internal scripts according to ci.
* Added getScreenDisplaysInfo to support videosourceStartScreenCaptureByDisplay, you can use this to share fullscreen from one of your monitors (in case you have multiple)

#### :bug: Bug Fix
* Use type `Element` instead of `HTMLElement`
* Fixed param for api `setClientRole`
* Fix missing RemoteVideoTransportStats + RemoteAudioTransportStats event
* Fix TranscodingUser cannot be added issue for rtmp streaming

## 2.4.0-alpha (Apr 11st, 2019)
#### :house: Internal
* Add Missing Api for 2.3.*
  * Add `getConnectionState` api
  * Add Event `remoteAudioStats`

* Add 2.4 Api
  * Add `setLogFileSize`
  * Add `setBeautyEffectOptions`
  * Add `setLocalVoiceChanger`
  * Add `setLocalVoiceReverbPreset`
  * Add `enableSoundPositionIndication`
  * Add `setRemoteVoicePosition`
  * Add `startLastmileProbeTest`
  * Add `stopLastmileProbeTest`
  * Add `setRemoteUserPriority`
  * Add `startEchoTestWithInterval`
  * Add `startAudioDeviceLoopbackTest`
  * Add `stopAudioDeviceLoopbackTest`
  * Add `setCameraCapturerConfiguration`
  * Add `videosourceStartScreenCaptureByScreen`
  * Add `videosourceStartScreenCaptureByWindow`
  * Add `videosourceUpdateScreenCaptureParameters`
  * Add `videosourceSetScreenCaptureContentHint`
  * Add event `audioMixingStateChanged`
  * Add event `lastmileProbeResult`
* Add `release` Api

* Modify Api
  * `setVideoEncoderConfiguration` will recv a param with type `VideoEncoderConfiguration`
  * `LocalVideoStats` add three properties: `targetBitrate,targetFrameRate,qualityAdaptIndication`

#### :warning: Notice
* You should call `videosourceSetVideoProfile` once before or after `videosourceStartScreenCaptureByWindow` to make it work properly in temp.
* `videosourceStartScreenCaptureByScreen` need a param called `ScreenSymbol` which differs on Mac and Windows. And a method will be provided in future to get this param on the two platforms.

#### :bug: Bug Fix
* fixed the problem that `setHighFps` not work
* fixed the problem which will prevent videosource from releasing

#### :memo: Documentation
* Add doc and type for the api above.

## 2.3.3-alpha.12 (March 19th, 2019)
#### :house: Internal
* Add Api
  * Add Event `groupAudioVolumeIndication` to provide all the speakers' volume as an array periodically

* Optimize type declaration in ts file.
* Revert frame handler for internal problem.

## 2.3.3-alpha.10 (Feb 13th, 2019)
#### :house: Internal
* Update native sdk (macos/windows) to 2.3.3 for optimization of screen sharing.
* Optimize robust for renderer operation
* Use enum as param for `setVideoProfile` & `videoSourceSetVideoProfile`
* Refactor and optimize command line tools

#### :bug: Bug Fix
* Fixed wrong strategy of stride and width in C++.
* Destroy renderer properly when useroffline emitted.

## 2.3.2-alpha (Jan 17th, 2019)
#### :house: Internal
* Upgrade Agora Native SDK to 2.3.2 (both OSX and Windows), visit Agora Official Website for API CHANGELOG.
* Support typescript (Use typescript to do refactor and generate d.ts for better develop experience).

## 2.0.8-rc.5 (Jan 7th, 2019)
#### :house: Internal
* Add `videoSourceSetLogFile` api (similiar to setLogFile)
* Support electron 4.0.0

#### :bug: Bug Fix
* Fixed overflow of uint32 uid.

## 2.0.8-rc.5-alpha (Nov 19th, 2018)
#### :house: Internal
* support multi version of prebuilt addon
  `In temp, you can switch prebuilt addon version by npm config or .npmrc, set agora_electron_dependent=<electron version in your app>, built with 1.8.3 for electron ranges from 1.8.3 to <3.0.0, and 3.0.6 for electron >= 3.0.0`
* more detail info when doing building or downloading

## 2.0.8-rc.4 (Nov 13rd, 2018)
#### :bug: Bug Fix
* Optimize resource release for webgl context.

#### :house: Internal
* Add Play Effect Related Api:
  * getEffectsVolume
  * setEffectsVolume
  * setVolumeOfEffect
  * playEffect
  * stopEffect
  * stopAllEffects
  * preloadEffect
  * unloadEffect
  * pauseEffect
  * pauseAllEffects
  * resumeEffect
  * resumeAllEffects

## 2.0.8-rc.3 (Nov 2nd, 2018)
#### :bug: Bug Fix
* Fixed webgl context related problems

#### :house: Internal
* New Api:
  * setRenderMode(mode) - Set default rendering mode, 1 to webGL, 2 to software rendering. Default to be webGL.

## 2.0.8-rc.2 (Nov 2nd, 2018)
#### :bug: Bug Fix
* Webgl render will cause some problem and will be fixed in next version, now we switch to use software rendering.

## 2.0.8-rc.1 (Oct 23th, 2018)
#### :house: Internal
* Update agora windows sdk to 2.0.8


## 2.0.7-rc.7 (Oct 23th, 2018)
#### :house: Internal
* Now canvas zoom will be re-calculated when the size of container changes.
* Update test demo with window sharing.

#### :bug: Bug fix
* Fixed a typo in implementation for getScreenWindowInfos (weight => height)

## 2.0.7-rc.7 (Sep 18th, 2018)
#### :house: Internal
* Add new api `getScreenWindowsInfo` to provide window info and id. You can use this to implement sharing windows (You can only share the whole screen before).
* Refactor renderer module. Now video source can be rendered without webgl.
* Optimize unit tests.
* Modify the build script for windows. (Use VS 2015 as msbuilder) 
* Add new api `enableLoopbackRecording` to enable loopback recording. Once enabled, the SDK collects all local sounds.


## 2.0.7-rc.6 (August 6, 2018)
#### :house: Internal
* Fixed a potential risk that will pend the promise.

#### :bug: Bug Fix
* Fixed a crash in ipc which will influnece screen sharing.

#### :memo: Documentation
* Now we provide complete [Javascript API Reference](./docs/apis.md)!


## 2.0.7-rc.3 (August 1, 2018)
#### :house: Internal
* Now we remove `build` folder and re-download everytime you run npm install.

## 2.0.7-rc.1 (July 26, 2018)
> Release for e-Education

#### :house: Internal
* Use 2.0.7 for Windows and 2.2.3 for Mac, which have done special optimization for e-Edu scenario. 

* Docs and unit-tests are nearly completed.

* Modify api:
  * setClientRole(CLIENT_ROLE_TYPE role, const char* permissionKey)

* Remove apis:
  * onStreamPublished
  * onStreamUnpublished
  * onTranscodingUpdated
  * onStreamInjectedStatus
  * addPublishStreamUrl
  * removePublishStreamUrl
  * setLiveTranscoding
  * addVideoWatermark
  * clearVideoWatermarks
  * addInjectStreamUrl
  * removeInjectStreamUrl
  * registerEventHandler
  * unregisterEventHandler
  * getEffectsVolume
  * setEffectsVolume
  * setVolumeOfEffect
  * playEffect
  * stopEffect
  * stopAllEffects
  * preloadEffect
  * unloadEffect
  * pauseEffect
  * pauseAllEffects
  * resumeEffect
  * resumeAllEffects
  * setLocalVoicePitch 
  * setLocalVoiceEqualization
  * setLocalVoiceReverb
  * enableLoopbackRecording


## 2.2.1-rc.1 (July 17, 2018)

#### :house: Internal

* Now we download built C++ addon instead of doing build when installing dependencies
* From now on we will use 2.2.1-rc.* as version label, and this will be a relatively stable version.

