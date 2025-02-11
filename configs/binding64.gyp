{
    'target_defaults': {
        'default_configuration': 'Release'
    },
    'targets': [
    {
        'target_name': 'VideoSource',
        'type': 'executable',
        'defines': [
        'UNICODE'
        ],
        'include_dirs': [
        './common',
        './common/libyuv/include',
        "<!(node -e \"require('nan')\")"
        ],
        'sources': [
        './common/loguru.hpp',
        './common/loguru.cpp',
        './common/ipc_shm.h',
        './common/video_source_ipc.h',
        './common/video_source_ipc.cpp',
        './common/node_log.h',
        './common/node_log.cpp',
        './common/node_process.h',
        './common/node_error.h',
        './common/windows_system_api.h',
        './common/windows_system_api.cpp',
        './video_source/video_source.cpp',
        './video_source/video_source.h',
        './video_source/video_source_event_handler.cpp',
        './video_source/video_source_event_handler.h',
        './video_source/video_source_param_parser.cpp',
        './video_source/video_source_param_parser.h',
        './video_source/video_source_render.cpp',
        './video_source/video_source_render.h',
        './video_source/video_source_transporter.cpp',
        './video_source/video_source_transporter.h',
        './sdk/include/IAgoraRtcEngine.h',
        './common/libyuv/source/compare_common.cc',
        './common/libyuv/source/compare.cc',
        './common/libyuv/source/convert_argb.cc',
        './common/libyuv/source/convert_from_argb.cc',
        './common/libyuv/source/convert_from.cc',
        './common/libyuv/source/convert_jpeg.cc',
        './common/libyuv/source/convert_to_argb.cc',
        './common/libyuv/source/convert_to_i420.cc',
        './common/libyuv/source/convert.cc',
        './common/libyuv/source/cpu_id.cc',
        './common/libyuv/source/mjpeg_decoder.cc',
        './common/libyuv/source/mjpeg_validate.cc',
        './common/libyuv/source/planar_functions.cc',
        './common/libyuv/source/rotate_any.cc',
        './common/libyuv/source/rotate_argb.cc',
        './common/libyuv/source/rotate_common.cc',
        './common/libyuv/source/rotate.cc',
        './common/libyuv/source/row_any.cc',
        './common/libyuv/source/row_common.cc',
        './common/libyuv/source/scale_any.cc',
        './common/libyuv/source/scale_argb.cc',
        './common/libyuv/source/scale_common.cc',
        './common/libyuv/source/scale.cc',
        './common/libyuv/source/video_common.cc'
        ],
        'conditions': [
            [
            'OS=="win"',
            {
                'library_dirs': [
                  './sdk/lib/win/x64/lib',
                ],
                'link_settings': {
                    'libraries': [
                        '-lagora_rtc_sdk.lib',
                        '-lws2_32.lib'
                    ]
                },
                'link_settings!': [
                        '-liojs.lib',
                ],
                'sources': [
                    './common/node_process_win.cpp',
                    './common/libyuv/source/compare_win.cc',
                    './common/libyuv/source/rotate_win.cc',
                    './common/libyuv/source/row_win.cc',
                    './common/libyuv/source/scale_win.cc',
                    './resources/resource.h',
                    './resources/VideoSource.rc',
                    './video_source/main_win.cpp'
                ],
                'include_dirs': [
                './sdk/include'
                ],
                'defines!': [
                '_USING_V110_SDK71_',
                '_HAS_EXCEPTIONS=0'
                ],
                'configurations': {
                    'Release': {
                        'msvs_settings': {
                            'VCCLCompilerTool': {
                                'ExceptionHandling': '0',
                                'AdditionalOptions': [
                                    '/EHsc'
                                ]
                            },
                            'VCManifestTool': {
                                'EmbedManifest': 'true',
                                'AdditionalManifestFiles': '../Resources/dpi_aware.manifest'
                            }
                        }
                    },
                    'Debug': {
                        'msvs_settings': {
                            'VCCLCompilerTool': {
                                'ExceptionHandling': '0',
                                'AdditionalOptions': [
                                    '/EHsc'
                                ]
                            },
                            'VCManifestTool': {
                                'EmbedManifest': 'true',
                                'AdditionalManifestFiles': '../Resources/dpi_aware.manifest'
                            }
                        }
                    }
                }
            }
            ],
            [
            'OS=="mac"',
            {
                'mac_framework_dirs': [
                '../sdk/lib/mac'
                ],
                'link_settings': {
                    'libraries': [
                    'libresolv.9.dylib',
                    'Accelerate.framework',
                    'AgoraAIDenoiseExtension.framework',
                    'AgoraCIExtension.framework',
                    'AgoraCore.framework',
                    'AgoraDav1dExtension.framework',
                    'AgoraFDExtension.framework',
                    'Agorafdkaac.framework',
                    'Agoraffmpeg.framework',
                    'AgoraFullAudioFormatExtension.framework',
                    'AgoraRtcKit.framework',
                    'AgoraSoundTouch.framework',
                    'AgoraSpatialAudioExtension.framework',
                    'AgoraVideoProcessExtension.framework',
                    'AgoraVideoSegmentationExtension.framework',
                    'av1.framework',
                    'CoreWLAN.framework',
                    'Cocoa.framework',
                    'VideoToolbox.framework',
                    'SystemConfiguration.framework',
                    'IOKit.framework',
                    'CoreVideo.framework',
                    'CoreMedia.framework',
                    'OpenGL.framework',
                    'CoreGraphics.framework',
                    'CFNetwork.framework',
                    'AudioToolbox.framework',
                    'CoreAudio.framework',
                    'Foundation.framework',
                    'AVFoundation.framework',
                    ]
                },
                'include_dirs': [
                './sdk/lib/mac/AgoraRtcKit.framework/Headers'
                ],
                'sources': [
                    './common/node_process_unix.cpp',
                    './common/libyuv/source/compare_gcc.cc',
                    './common/libyuv/source/rotate_gcc.cc',
                    './common/libyuv/source/row_gcc.cc',
                    './common/libyuv/source/scale_gcc.cc',
                    './video_source/main_mac.mm'
                ],
                'defines!': [
                '_HAS_EXCEPTIONS=0',
                '-std=gnu++14'
                ],
                'OTHER_CFLAGS' : [
                    '-std=c++11',
                    '-stdlib=libc++',
                    '-fexceptions'
                ],
                'xcode_settings': {
                    'MACOSX_DEPLOYMENT_TARGET': '10.13',
                    'FRAMEWORK_SEARCH_PATHS': [
                    './sdk/lib/mac'
                    ],
                    "DEBUG_INFORMATION_FORMAT": "dwarf-with-dsym",
                    'OTHER_LDFLAGS': [ '-Wl,-rpath,@loader_path']
                },

            }
            ]		]
    },
    {
        'target_name': 'agora_node_ext',
        'include_dirs': [
        './common',
        './common/libyuv/include',
        "<!(node -e \"require('nan')\")"
        ],
        'sources': [
        './common/loguru.hpp',
        './common/loguru.cpp',
        './common/ipc_shm.h',
        './common/node_log.cpp',
        './common/node_log.h',
        './common/video_source_ipc.cpp',
        './common/video_source_ipc.h',
        './common/node_event.h',
        './common/node_event.cpp',
        './common/node_process.h',
        './common/node_error.h',
        './common/windows_system_api.h',
        './common/windows_system_api.cpp',
        './agora_node_ext/agora_node_ext.cpp',
        './agora_node_ext/agora_node_ext.h',
        './agora_node_ext/agora_rtc_engine.cpp',
        './agora_node_ext/agora_rtc_engine.h',
        './agora_node_ext/agora_video_source.cpp',
        './agora_node_ext/agora_video_source.h',
        './agora_node_ext/node_async_queue.cpp',
        './agora_node_ext/node_async_queue.h',
        './agora_node_ext/node_event_handler.cpp',
        './agora_node_ext/node_event_handler.h',
        './agora_node_ext/node_channel_event_handler.cpp',
        './agora_node_ext/node_channel_event_handler.h',
        './agora_node_ext/node_napi_api.cpp',
        './agora_node_ext/node_napi_api.h',
        './agora_node_ext/node_uid.h',
        './agora_node_ext/node_video_render.cpp',
        './agora_node_ext/node_video_render.h',
        './agora_node_ext/node_video_stream_channel.cpp',
        './agora_node_ext/node_video_stream_channel.h',
        './agora_node_ext/AVPlugin/IAVFramePlugin.h',
        './agora_node_ext/AVPlugin/IAVFramePluginManager.h',
        './agora_node_ext/AVPlugin/IAVFramePluginManager.cpp',
        './agora_node_ext/node_metadata_observer.h',
        './agora_node_ext/node_metadata_observer.cpp',
        './common/libyuv/source/compare_common.cc',
        './common/libyuv/source/compare.cc',
        './common/libyuv/source/convert_argb.cc',
        './common/libyuv/source/convert_from_argb.cc',
        './common/libyuv/source/convert_from.cc',
        './common/libyuv/source/convert_jpeg.cc',
        './common/libyuv/source/convert_to_argb.cc',
        './common/libyuv/source/convert_to_i420.cc',
        './common/libyuv/source/convert.cc',
        './common/libyuv/source/cpu_id.cc',
        './common/libyuv/source/mjpeg_decoder.cc',
        './common/libyuv/source/mjpeg_validate.cc',
        './common/libyuv/source/planar_functions.cc',
        './common/libyuv/source/rotate_any.cc',
        './common/libyuv/source/rotate_argb.cc',
        './common/libyuv/source/rotate_common.cc',
        './common/libyuv/source/rotate.cc',
        './common/libyuv/source/row_any.cc',
        './common/libyuv/source/row_common.cc',
        './common/libyuv/source/scale_any.cc',
        './common/libyuv/source/scale_argb.cc',
        './common/libyuv/source/scale_common.cc',
        './common/libyuv/source/scale.cc',
        './common/libyuv/source/video_common.cc'
        ],
        'conditions': [
            [
            'OS=="win"',
            {
                'copies': [{
                    'destination': '<(PRODUCT_DIR)',
                    'files': [
                        './sdk/lib/win/x64/dll/agora_rtc_sdk.dll',
                        './sdk/lib/win/x64/dll/av1.dll',
                        './sdk/lib/win/x64/dll/libagora-core.dll',
                        './sdk/lib/win/x64/dll/libagora-ffmpeg.dll',
                        './sdk/lib/win/x64/dll/libagora-soundtouch.dll',
                        './sdk/lib/win/x64/dll/libagora-wgc.dll',
                        './sdk/lib/win/x64/dll/libagora_ai_denoise_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_ci_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_dav1d_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_fd_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_fdkaac.dll',
                        './sdk/lib/win/x64/dll/libagora_full_audio_format_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_mpg123.dll',
                        './sdk/lib/win/x64/dll/libagora_segmentation_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_spatial_audio_extension.dll',
                        './sdk/lib/win/x64/dll/libagora_video_process_extension.dll',
                        './sdk/lib/win/x64/dll/libhwcodec.dll',
                    ]
                }],
                'library_dirs': [
                    './sdk/lib/win/x64/lib',
                ],
                'link_settings': {
                    'libraries': [
                        '-lagora_rtc_sdk.lib',
                        '-lws2_32.lib',
                        '-lRpcrt4.lib',
						            '-lgdiplus.lib',
                        '-lseax_engine.lib',
                        '-lAgoraCommonAudio.lib',
                        '-llibagora-core.lib'
                    ]
                },
                'defines' : [
                  'AGORARTC_EXPORT'
                ],
                'defines!': [
                '_USING_V110_SDK71_',
                '_HAS_EXCEPTIONS=0'
                ],
                'sources': [
                    './common/node_process_win.cpp',
                    './sdk/include/IAgoraRtcEngine.h',
                    './sdk/include/IAgoraSeaxEngine.h',
                    './common/libyuv/source/compare_win.cc',
                    './common/libyuv/source/rotate_win.cc',
                    './common/libyuv/source/row_win.cc',
                    './common/libyuv/source/scale_win.cc',
					          './agora_node_ext/node_screen_window_info_win.cpp',
                    './agora_node_ext/node_screen_window_info.h',
                    './agora_node_ext/win_enumer.h',
                    './agora_node_ext/win_enumer.cpp'
                ],
                'include_dirs': [
                './sdk/include',
                './extra/internal'
                ],
                'configurations': {
                    'Release': {
                        'msvs_settings': {
                            'VCCLCompilerTool': {
                                'ExceptionHandling': '0',
                                'AdditionalOptions': [
                                    '/EHsc'
                                ]
                            }
                        }
                    },
                    'Debug': {
                        'msvs_settings': {
                            'VCCLCompilerTool': {
                                'ExceptionHandling': '0',
                                'AdditionalOptions': [
                                    '/EHsc'
                                ]
                            }
                        }
                    }
                }
            }
            ],
            [
            'OS=="mac"',
            {
                'mac_framework_dirs': [
                    '../sdk/lib/mac'
                ],
                'library_dirs': [
                    '../sdk/lib/mac',
                ],
                'copies': [{
                    'destination': '<(PRODUCT_DIR)',
                    'files': [
                        './sdk/lib/mac/AgoraAIDenoiseExtension.framework',
                        './sdk/lib/mac/AgoraCIExtension.framework',
                        './sdk/lib/mac/AgoraCore.framework',
                        './sdk/lib/mac/AgoraDav1dExtension.framework',
                        './sdk/lib/mac/AgoraFDExtension.framework',
                        './sdk/lib/mac/Agorafdkaac.framework',
                        './sdk/lib/mac/Agoraffmpeg.framework',
                        './sdk/lib/mac/AgoraFullAudioFormatExtension.framework',
                        './sdk/lib/mac/AgoraRtcKit.framework',
                        './sdk/lib/mac/AgoraSoundTouch.framework',
                        './sdk/lib/mac/AgoraSpatialAudioExtension.framework',
                        './sdk/lib/mac/AgoraVideoProcessExtension.framework',
                        './sdk/lib/mac/AgoraVideoSegmentationExtension.framework',
                        './sdk/lib/mac/av1.framework',
                    ]
                }],
                'link_settings': {
                    'libraries': [
                    'libresolv.9.dylib',
                    'Accelerate.framework',
                    'AgoraAIDenoiseExtension.framework',
                    'AgoraCIExtension.framework',
                    'AgoraCore.framework',
                    'AgoraDav1dExtension.framework',
                    'AgoraFDExtension.framework',
                    'Agorafdkaac.framework',
                    'Agoraffmpeg.framework',
                    'AgoraFullAudioFormatExtension.framework',
                    'AgoraRtcKit.framework',
                    'AgoraSoundTouch.framework',
                    'AgoraSpatialAudioExtension.framework',
                    'AgoraVideoProcessExtension.framework',
                    'AgoraVideoSegmentationExtension.framework',
                    'av1.framework',
                    'CoreWLAN.framework',
                    'Cocoa.framework',
                    'VideoToolbox.framework',
                    'SystemConfiguration.framework',
                    'IOKit.framework',
                    'CoreVideo.framework',
                    'CoreMedia.framework',
                    'OpenGL.framework',
                    'CoreGraphics.framework',
                    'CFNetwork.framework',
                    'AudioToolbox.framework',
                    'CoreAudio.framework',
                    'Foundation.framework',
                    'AVFoundation.framework',
                    'libseax_engine.a'
                    ]
                },
                'sources': [
                    './common/node_process_unix.cpp',
                    './common/libyuv/source/compare_gcc.cc',
                    './common/libyuv/source/rotate_gcc.cc',
                    './common/libyuv/source/row_gcc.cc',
                    './common/libyuv/source/scale_gcc.cc',
                    './agora_node_ext/node_screen_window_info_mac.cpp',
                    './agora_node_ext/node_screen_window_info.h'
                ],
                'include_dirs': [
                './sdk/lib/mac/AgoraRtcKit.framework/Headers',
                './extra/internal',
                './sdk/include'
                ],
                'defines!': [
                    '_NOEXCEPT',
                    '-std=c++11'
                ],
                'OTHER_CFLAGS' : [
                    '-std=c++11',
                    '-stdlib=libc++',
                    '-fexceptions'
                ],
                'xcode_settings': {
                    'MACOSX_DEPLOYMENT_TARGET': '10.11',
                    'EXECUTABLE_EXTENSION': 'node',
                    'FRAMEWORK_SEARCH_PATHS': [
                    './sdk/lib/mac'
                    ],
                    "DEBUG_INFORMATION_FORMAT": "dwarf-with-dsym",
                    'OTHER_LDFLAGS': [ '-Wl,-rpath,@loader_path']
                },
            }
            ]
        ]
    },
    ]
}
