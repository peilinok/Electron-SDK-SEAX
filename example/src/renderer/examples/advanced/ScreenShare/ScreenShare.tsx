import React from 'react';
import {
  ChannelProfileType,
  ClientRoleType,
  createAgoraRtcEngine,
  IRtcEngineEventHandler,
  IRtcEngineEx,
  LocalVideoStreamError,
  LocalVideoStreamState,
  RtcConnection,
  RtcStats,
  ScreenCaptureSourceInfo,
  UserOfflineReasonType,
  VideoSourceType,
} from 'electron-agora-rtc-ng';
import { SketchPicker } from 'react-color';

import Config from '../../../config/agora.config';

import {
  BaseComponent,
  BaseVideoComponentState,
} from '../../../components/BaseComponent';
import {
  AgoraButton,
  AgoraDivider,
  AgoraDropdown,
  AgoraImage,
  AgoraSlider,
  AgoraSwitch,
  AgoraTextInput,
  AgoraView,
} from '../../../components/ui';
import RtcSurfaceView from '../../../components/RtcSurfaceView';
import { rgbImageBufferToBase64 } from '../../../utils/base64';
import { ScreenCaptureSourceType } from '../../../../../../ts/Private/IAgoraRtcEngine';

interface State extends BaseVideoComponentState {
  token2: string;
  uid2: number;
  sources?: ScreenCaptureSourceInfo[];
  targetSource?: ScreenCaptureSourceInfo;
  width: number;
  height: number;
  frameRate: number;
  bitrate: number;
  captureMouseCursor: boolean;
  windowFocus: boolean;
  excludeWindowList: number[];
  highLightWidth: number;
  highLightColor: number;
  enableHighLight: boolean;
  startScreenCapture: boolean;
  publishScreenCapture: boolean;
}

export default class ScreenShare
  extends BaseComponent<{}, State>
  implements IRtcEngineEventHandler
{
  // @ts-ignore
  protected engine?: IRtcEngineEx;

  protected createState(): State {
    return {
      appId: Config.appId,
      enableVideo: true,
      channelId: Config.channelId,
      token: Config.token,
      uid: Config.uid,
      joinChannelSuccess: false,
      remoteUsers: [],
      startPreview: false,
      token2: '',
      uid2: 0,
      sources: [],
      targetSource: undefined,
      width: 1920,
      height: 1080,
      frameRate: 15,
      bitrate: 0,
      captureMouseCursor: true,
      windowFocus: false,
      excludeWindowList: [],
      highLightWidth: 0,
      highLightColor: 0xff8cbf26,
      enableHighLight: false,
      startScreenCapture: false,
      publishScreenCapture: false,
    };
  }

  /**
   * Step 1: initRtcEngine
   */
  protected async initRtcEngine() {
    const { appId } = this.state;
    if (!appId) {
      this.error(`appId is invalid`);
    }

    this.engine = createAgoraRtcEngine() as IRtcEngineEx;
    this.engine.registerEventHandler(this);
    this.engine.initialize({
      appId,
      // Should use ChannelProfileLiveBroadcasting on most of cases
      channelProfile: ChannelProfileType.ChannelProfileLiveBroadcasting,
    });

    // Need to enable video on this case
    // If you only call `enableAudio`, only relay the audio stream to the target channel
    this.engine.enableVideo();

    // Start preview before joinChannel
    this.engine.startPreview();
    this.setState({ startPreview: true });

    this.getScreenCaptureSources();
  }

  /**
   * Step 2: joinChannel
   */
  protected joinChannel() {
    const { channelId, token, uid } = this.state;
    if (!channelId) {
      this.error('channelId is invalid');
      return;
    }
    if (uid < 0) {
      this.error('uid is invalid');
      return;
    }

    // start joining channel
    // 1. Users can only see each other after they join the
    // same channel successfully using the same app id.
    // 2. If app certificate is turned on at dashboard, token is needed
    // when joining channel. The channel name and uid used to calculate
    // the token has to match the ones used for channel join
    this.engine?.joinChannel(token, channelId, uid, {
      // Make myself as the broadcaster to send stream to remote
      clientRoleType: ClientRoleType.ClientRoleBroadcaster,
    });
  }

  /**
   * Step 3-1: getScreenCaptureSources
   */
  getScreenCaptureSources = () => {
    const sources = this.engine?.getScreenCaptureSources(
      { width: 1920, height: 1080 },
      { width: 64, height: 64 },
      true
    );
    this.setState({
      sources,
      targetSource: sources[0],
    });
  };

  /**
   * Step 3-2: startScreenCapture
   */
  startScreenCapture = () => {
    const {
      targetSource,
      width,
      height,
      frameRate,
      bitrate,
      captureMouseCursor,
      windowFocus,
      excludeWindowList,
      highLightWidth,
      highLightColor,
      enableHighLight,
    } = this.state;

    if (!targetSource) {
      this.error('targetSource is invalid');
      return;
    }

    if (
      targetSource.type ===
      ScreenCaptureSourceType.ScreencapturesourcetypeScreen
    ) {
      this.engine?.startScreenCaptureByDisplayId(
        targetSource.sourceId,
        {},
        {
          dimensions: { width, height },
          frameRate,
          bitrate,
          captureMouseCursor,
          excludeWindowList,
          excludeWindowCount: excludeWindowList.length,
          highLightWidth,
          highLightColor,
          enableHighLight,
        }
      );
    } else {
      this.engine?.startScreenCaptureByWindowId(
        targetSource.sourceId,
        {},
        {
          dimensions: { width, height },
          frameRate,
          bitrate,
          windowFocus,
          highLightWidth,
          highLightColor,
          enableHighLight,
        }
      );
    }
  };

  /**
   * Step 3-2 (Optional): updateScreenCaptureParameters
   */
  updateScreenCaptureParameters = () => {
    const {
      width,
      height,
      frameRate,
      bitrate,
      captureMouseCursor,
      windowFocus,
      excludeWindowList,
      highLightWidth,
      highLightColor,
      enableHighLight,
    } = this.state;
    this.engine?.updateScreenCaptureParameters({
      dimensions: { width, height },
      frameRate,
      bitrate,
      captureMouseCursor,
      windowFocus,
      excludeWindowList,
      excludeWindowCount: excludeWindowList.length,
      highLightWidth,
      highLightColor,
      enableHighLight,
    });
  };

  /**
   * Step 3-4: publishScreenCapture
   */
  publishScreenCapture = () => {
    const { channelId, token2, uid2 } = this.state;
    if (!channelId) {
      this.error('channelId is invalid');
      return;
    }
    if (uid2 <= 0) {
      this.error('uid2 is invalid');
      return;
    }

    // publish media player stream
    this.engine?.joinChannelEx(
      token2,
      { channelId, localUid: uid2 },
      {
        clientRoleType: ClientRoleType.ClientRoleBroadcaster,
        publishScreenTrack: true,
      }
    );
  };

  /**
   * Step 3-5: stopScreenCapture
   */
  stopScreenCapture = () => {
    this.engine?.stopScreenCapture();
  };

  /**
   * Step 3-6: unpublishScreenCapture
   */
  unpublishScreenCapture = () => {
    const { channelId, uid2 } = this.state;
    this.engine?.leaveChannelEx({ channelId, localUid: uid2 });
  };

  /**
   * Step 4: leaveChannel
   */
  protected leaveChannel() {
    this.engine?.leaveChannel();
  }

  /**
   * Step 5: releaseRtcEngine
   */
  protected releaseRtcEngine() {
    this.engine?.unregisterEventHandler(this);
    this.engine?.release();
  }

  onJoinChannelSuccess(connection: RtcConnection, elapsed: number) {
    const { uid2 } = this.state;
    if (connection.localUid === uid2) {
      this.info(
        'onJoinChannelSuccess',
        'connection',
        connection,
        'elapsed',
        elapsed
      );
      this.setState({ publishScreenCapture: true });
      return;
    }
    super.onJoinChannelSuccess(connection, elapsed);
  }

  onLeaveChannel(connection: RtcConnection, stats: RtcStats) {
    this.info('onLeaveChannel', 'connection', connection, 'stats', stats);
    const { uid2 } = this.state;
    if (connection.localUid === uid2) {
      this.setState({ publishScreenCapture: false });
      return;
    }
    const state = this.createState();
    delete state.sources;
    delete state.targetSource;
    this.setState(this.createState());
  }

  onUserJoined(connection: RtcConnection, remoteUid: number, elapsed: number) {
    const { uid2 } = this.state;
    if (connection.localUid === uid2 || remoteUid === uid2) return;
    super.onUserJoined(connection, remoteUid, elapsed);
  }

  onUserOffline(
    connection: RtcConnection,
    remoteUid: number,
    reason: UserOfflineReasonType
  ) {
    const { uid2 } = this.state;
    if (connection.localUid === uid2 || remoteUid === uid2) return;
    super.onUserOffline(connection, remoteUid, reason);
  }

  onLocalVideoStateChanged(
    source: VideoSourceType,
    state: LocalVideoStreamState,
    error: LocalVideoStreamError
  ) {
    this.info(
      'onLocalVideoStateChanged',
      'source',
      source,
      'state',
      state,
      'error',
      error
    );
    if (source === VideoSourceType.VideoSourceScreen) {
      switch (state) {
        case LocalVideoStreamState.LocalVideoStreamStateStopped:
        case LocalVideoStreamState.LocalVideoStreamStateFailed:
          this.setState({
            startScreenCapture: false,
          });
          break;
        case LocalVideoStreamState.LocalVideoStreamStateCapturing:
        case LocalVideoStreamState.LocalVideoStreamStateEncoding:
          this.setState({ startScreenCapture: true });
          break;
      }
    }
  }

  protected renderUsers(): React.ReactNode {
    const { startScreenCapture } = this.state;
    return (
      <>
        {super.renderUsers()}
        {startScreenCapture ? (
          <RtcSurfaceView
            canvas={{
              uid: 0,
              sourceType: VideoSourceType.VideoSourceScreen,
            }}
          />
        ) : undefined}
      </>
    );
  }

  protected renderConfiguration(): React.ReactNode {
    const {
      sources,
      targetSource,
      uid2,
      width,
      height,
      frameRate,
      bitrate,
      captureMouseCursor,
      windowFocus,
      excludeWindowList,
      highLightWidth,
      highLightColor,
      enableHighLight,
      publishScreenCapture,
    } = this.state;
    return (
      <>
        <AgoraDropdown
          title={'targetSource'}
          items={sources.map((value) => {
            return {
              value: value.sourceId,
              label: value.sourceName,
            };
          })}
          value={targetSource?.sourceId}
          onValueChange={(value, index) => {
            this.setState({ targetSource: sources[index] });
          }}
        />
        {targetSource ? (
          <AgoraImage
            source={rgbImageBufferToBase64(targetSource.thumbImage)}
          />
        ) : undefined}
        <AgoraTextInput
          editable={!publishScreenCapture}
          onChangeText={(text) => {
            if (isNaN(+text)) return;
            this.setState({ uid2: +text });
          }}
          placeholder={`uid2 (must > 0)`}
          value={uid2 > 0 ? uid2.toString() : ''}
        />
        <AgoraView>
          <AgoraTextInput
            onChangeText={(text) => {
              if (isNaN(+text)) return;
              this.setState({ width: +text });
            }}
            placeholder={`width (defaults: ${this.createState().width})`}
            value={width === this.createState().width ? '' : width.toString()}
          />
          <AgoraTextInput
            onChangeText={(text) => {
              if (isNaN(+text)) return;
              this.setState({ height: +text });
            }}
            placeholder={`height (defaults: ${this.createState().height})`}
            value={
              height === this.createState().height ? '' : height.toString()
            }
          />
        </AgoraView>
        <AgoraTextInput
          onChangeText={(text) => {
            if (isNaN(+text)) return;
            this.setState({ frameRate: +text });
          }}
          placeholder={`frameRate (defaults: ${this.createState().frameRate})`}
          value={
            frameRate === this.createState().frameRate
              ? ''
              : frameRate.toString()
          }
        />
        <AgoraTextInput
          onChangeText={(text) => {
            if (isNaN(+text)) return;
            this.setState({ bitrate: +text });
          }}
          placeholder={`bitrate (defaults: ${this.createState().bitrate})`}
          value={
            bitrate === this.createState().bitrate ? '' : bitrate.toString()
          }
        />
        {targetSource?.type ===
        ScreenCaptureSourceType.ScreencapturesourcetypeScreen ? (
          <>
            <AgoraSwitch
              title={`captureMouseCursor`}
              value={captureMouseCursor}
              onValueChange={(value) => {
                this.setState({ captureMouseCursor: value });
              }}
            />
            <AgoraDivider />
          </>
        ) : undefined}
        {targetSource?.type ===
        ScreenCaptureSourceType.ScreencapturesourcetypeWindow ? (
          <>
            <AgoraSwitch
              title={`windowFocus`}
              value={windowFocus}
              onValueChange={(value) => {
                this.setState({ windowFocus: value });
              }}
            />
            <AgoraDivider />
          </>
        ) : undefined}
        {targetSource?.type ===
        ScreenCaptureSourceType.ScreencapturesourcetypeScreen ? (
          <>
            <AgoraDropdown
              title={'excludeWindowList'}
              items={sources
                .filter(
                  (value) =>
                    value.type ===
                    ScreenCaptureSourceType.ScreencapturesourcetypeWindow
                )
                .map((value) => {
                  return {
                    value: value.sourceId,
                    label: value.sourceName,
                  };
                })}
              value={excludeWindowList}
              onValueChange={(value, index) => {
                if (excludeWindowList.indexOf(+value) === -1) {
                  this.setState({
                    excludeWindowList: [...excludeWindowList, +value],
                  });
                } else {
                  this.setState({
                    excludeWindowList: excludeWindowList.filter(
                      (v) => v !== +value
                    ),
                  });
                }
              }}
            />
            <AgoraDivider />
          </>
        ) : undefined}
        <AgoraSwitch
          title={`enableHighLight`}
          value={enableHighLight}
          onValueChange={(value) => {
            this.setState({ enableHighLight: value });
          }}
        />
        {enableHighLight ? (
          <>
            <AgoraDivider />
            <AgoraSlider
              title={`highLightWidth ${highLightWidth}`}
              minimumValue={0}
              maximumValue={50}
              step={1}
              value={highLightWidth}
              onSlidingComplete={(value) => {
                this.setState({
                  highLightWidth: value,
                });
              }}
            />
            <AgoraDivider />
            <SketchPicker
              onChangeComplete={({ hex }) => {
                this.setState({
                  highLightColor: +hex.replace('#', '0x'),
                });
              }}
              color={`#${highLightColor?.toString(16)}`}
            />
          </>
        ) : undefined}
      </>
    );
  }

  protected renderAction(): React.ReactNode {
    const { startScreenCapture, publishScreenCapture } = this.state;
    return (
      <>
        <AgoraButton
          title={`${startScreenCapture ? 'stop' : 'start'} Screen Capture`}
          onPress={
            startScreenCapture
              ? this.stopScreenCapture
              : this.startScreenCapture
          }
        />
        <AgoraButton
          disabled={!startScreenCapture}
          title={'updateScreenCaptureParameters'}
          onPress={this.updateScreenCaptureParameters}
        />
        <AgoraButton
          title={`${
            publishScreenCapture ? 'unpublish' : 'publish'
          } Screen Capture`}
          onPress={
            publishScreenCapture
              ? this.unpublishScreenCapture
              : this.publishScreenCapture
          }
        />
      </>
    );
  }
}
