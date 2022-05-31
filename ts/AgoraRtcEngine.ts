﻿import { VideoSourceType } from "./Private/AgoraBase";
import { IMediaPlayer } from "./Private/IAgoraMediaPlayer";
import {
  ChannelMediaOptions,
  IRtcEngineEventHandler,
  RtcEngineContext,
} from "./Private/IAgoraRtcEngine";
import { RtcConnection } from "./Private/IAgoraRtcEngineEx";
import { IRtcEngineExImpl } from "./Private/impl/IAgoraRtcEngineExImpl";
import { getBridge, handlerRTCEvent } from "./Private/internal/IrisApiEngine";
import {
  handlerMPKEvent,
  MediaPlayerInternal,
} from "./Private/internal/MediaPlayerInternal";
import AgoraRendererManager from "./Renderer/RendererManager";
import {
  CallBackModule,
  Channel,
  ContentMode,
  RendererVideoConfig,
  RENDER_MODE,
} from "./Types";
import { AgoraEnv, deprecate, logDebug, logError, logWarn } from "./Utils";

/**
 * The AgoraRtcEngine class.
 */
export class AgoraRtcEngine extends IRtcEngineExImpl {
  constructor() {
    super();

    logDebug("AgoraRtcEngine constructor()");
  }

  override initialize(context: RtcEngineContext): number {
    if (AgoraEnv.isInitializeEngine) {
      logWarn("initialize: already initialize rtcEngine");
      return -1;
    }
    AgoraEnv.isInitializeEngine = true;
    const bridge = getBridge();
    bridge.InitializeEnv();
    bridge.OnEvent(
      CallBackModule.RTC,
      "call_back_with_buffer",
      handlerRTCEvent
    );
    bridge.OnEvent(
      CallBackModule.MPK,
      "call_back_with_buffer",
      handlerMPKEvent
    );
    const ret = super.initialize(context);
    return ret;
  }
  override release(sync = false): void {
    if (!AgoraEnv.isInitializeEngine) {
      logWarn("release: rtcEngine have not initialize");
      return;
    }
    AgoraEnv.isInitializeEngine = false;
    super.release(sync);
    getBridge().ReleaseEnv();
  }

  createMediaPlayer(): IMediaPlayer {
    if (!AgoraEnv.isInitializeEngine) {
      logError("createMediaPlayer: rtcEngine have not initialize");
    }
    // @ts-ignore
    const mediaPlayerId = super.createMediaPlayer() as number;
    return new MediaPlayerInternal(mediaPlayerId);
  }

  override joinChannelEx(
    token: string,
    connection: RtcConnection,
    options: ChannelMediaOptions,
    eventHandler?: IRtcEngineEventHandler
  ): number {
    if (eventHandler) {
      this.registerEventHandler(eventHandler);
    }

    return super.joinChannelEx(token, connection, options, eventHandler!);
  }

  override setupLocalVideo(rendererConfig: RendererVideoConfig): number {
    return AgoraRendererManager.setupLocalVideo(rendererConfig);
  }
  override setupRemoteVideo(rendererConfig: RendererVideoConfig): number {
    return AgoraRendererManager.setupRemoteVideo(rendererConfig);
  }
  setupVideo(rendererConfig: RendererVideoConfig): void {
    AgoraRendererManager.setupVideo(rendererConfig);
  }

  destroyRendererByView(view: Element): void {
    AgoraRendererManager.destroyRendererByView(view);
  }

  destroyRendererByConfig(
    videoSourceType: VideoSourceType,
    channelId?: Channel,
    uid?: number
  ) {
    AgoraRendererManager.destroyRenderersByConfig(
      videoSourceType,
      channelId,
      uid
    );
  }

  setRenderOption(
    view: HTMLElement,
    contentMode = ContentMode.Fit,
    mirror: boolean = false
  ): void {
    AgoraRendererManager.setRenderOption(view, contentMode, mirror);
  }

  setRenderOptionByConfig(rendererConfig: RendererVideoConfig): void {
    AgoraRendererManager.setRenderOptionByConfig(rendererConfig);
  }

  setRenderMode(mode = RENDER_MODE.WEBGL): void {
    AgoraRendererManager.setRenderMode(mode);
  }

  // @mark old api will deprecate
  // @mark old api will deprecate
  // @mark old api will deprecate
  // @mark old api will deprecate
  // @mark old api will deprecate
  // @mark old api will deprecate
  // @mark old api will deprecate

  _setupLocalVideo(view: Element): number {
    deprecate("_setupLocalVideo", "setupVideo or setupLocalVideo");
    this.setupLocalVideo({
      videoSourceType: VideoSourceType.VideoSourceCamera,
    });
    return 0;
  }

  _setupViewContentMode(
    uid: number | "local" | "videosource",
    mode: 0 | 1,
    channelId: string
  ): number {
    deprecate(
      "_setupViewContentMode",
      "setRenderOptionByConfig or setRenderOption"
    );

    const contentMode = mode === 1 ? ContentMode.Fit : ContentMode.Cropped;
    const mirror = false;
    switch (uid) {
      case "local":
        this.setRenderOptionByConfig({
          videoSourceType: VideoSourceType.VideoSourceCamera,
          rendererOptions: {
            contentMode,
            mirror,
          },
        });
        break;
      case "videosource":
        this.setRenderOptionByConfig({
          videoSourceType: VideoSourceType.VideoSourceScreen,
          rendererOptions: {
            contentMode,
            mirror,
          },
        });
        break;
      default:
        this.setRenderOptionByConfig({
          videoSourceType: VideoSourceType.VideoSourceRemote,
          channelId,
          rendererOptions: {
            contentMode,
            mirror,
          },
        });
        break;
    }
    return 0;
  }
  _destroyRenderView(
    key: "local" | "videosource" | number,
    channelId: string | undefined,
    view: Element,
    onFailure?: (err: Error) => void
  ) {
    deprecate(
      "_destroyRenderView",
      "destroyRendererByView or destroyRendererByConfig"
    );
    switch (key) {
      case "local":
        break;
      case "videosource":
        break;

      default:
        break;
    }
  }
  _subscribe(
    uid: number,
    view: Element,
    options?: {
      append: boolean;
    }
  ) {
    deprecate("_subscribe", "setupVideo or setupRemoteVideo");
  }
  _setupLocalVideoSource(view: HTMLElement) {
    deprecate("_setupLocalVideoSource", "setupVideo or setupLocalVideo");
    this.setupVideo({
      videoSourceType: VideoSourceType.VideoSourceScreen,
      view,
    });
  }
  _setupRemoteVideo(
    uid: number,
    view?: HTMLElement,
    channelId?: string,
    options?: {
      append: boolean;
    }
  ) {
    deprecate("_setupRemoteVideo", "setupVideo or setupRemoteVideo");
    this.setupVideo({
      videoSourceType: VideoSourceType.VideoSourceRemote,
      view,
      channelId,
    });
  }
}
