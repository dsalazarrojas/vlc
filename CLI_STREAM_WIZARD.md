# VLC CLI Stream Wizard

## Overview

The CLI Stream Wizard feature allows you to configure VLC streaming from the command line using the same logic as the GUI stream dialog. This feature replicates the functionality of the Qt-based "Stream Output" wizard, making it easy to set up streaming without needing the graphical interface.

## Command-Line Option

```
--stream-wizard-config=<config>
```

## Configuration Format

The configuration string uses a comma-separated key=value format:

```
profile=<profile_name>,dest=<type>,addr=<address>,port=<port>,path=<path>,transcode=<0|1>,local=<0|1>,sout_all=<0|1>
```

### Parameters

| Parameter | Type | Required | Description | Default |
|-----------|------|----------|-------------|---------|
| `profile` | string | No | Transcoding profile name (see list below) | None |
| `dest` | string | **Yes** | Destination type | - |
| `addr` or `address` | string | Depends | Destination address or file path | - |
| `port` | integer | No | Network port | Depends on destination |
| `path` | string | No | URL path (for HTTP/RTSP) | "/" |
| `mux` | string | No | Muxer override | From profile |
| `transcode` | 0 or 1 | No | Enable transcoding | 0 |
| `local` | 0 or 1 | No | Display locally while streaming | 0 |
| `sout_all` | 0 or 1 | No | Stream all elementary streams | 1 |
| `sap_name` | string | No | SAP announcement name (RTP/SRT) | - |
| `ice_mount` | string | No | Icecast mount point | - |
| `ice_password` | string | No | Icecast password | - |

### Destination Types

- `file` - Save to a file
- `http` - Stream via HTTP
- `rtsp` - Stream via RTSP
- `mmsh` - Stream via MS-WMSP (MMSH)
- `udp` - Stream via UDP (legacy)
- `rtp` - Stream via RTP
- `srt` - Stream via SRT
- `rist` - Stream via RIST
- `ice` or `icecast` - Stream to Icecast server

### Available Profiles

The following transcoding profiles are available (must be quoted if they contain spaces):

- "Video - H.264 + MP3 (MP4)"
- "Video - H.264 + AAC (MP4)"
- "Video - VP80 + Vorbis (Webm)"
- "Video - H.264 + MPGA (TS)"
- "Video - H.265 + AAC (TS)"
- "Video - H.265 + AAC (MP4)"
- "Video - Theora + Vorbis (OGG)"
- "Video - MPEG-2 + MPGA (TS)"
- "Video - WMV + WMA (ASF)"
- "Video - DIV3 + MP3 (ASF)"
- "Audio - AAC (MP4A)"
- "Audio - Vorbis (OGG)"
- "Audio - MP3"
- "Audio - FLAC"
- "Audio - CD (uncompressed)"
- "Video for MPEG4 720p TV/device"
- "Video for MPEG4 1080p TV/device"
- "Video for DivX compatible player"
- "Video for iPod SD"
- "Video for iPod HD/iPhone/PSP"
- "Video for Android SD Low"
- "Video for Android SD High"
- "Video for Android HD"
- "Video for Youtube SD"
- "Video for Youtube HD"

## Examples

### Example 1: Simple HTTP Streaming

Stream a video file via HTTP without transcoding:

```bash
vlc input.mp4 --stream-wizard-config="dest=http,addr=0.0.0.0,port=8080,path=/stream"
```

Access the stream at: `http://localhost:8080/stream`

### Example 2: HTTP Streaming with Transcoding

Stream with H.264 + MP3 transcoding:

```bash
vlc input.avi --stream-wizard-config="profile='Video - H.264 + MP3 (MP4)',dest=http,addr=0.0.0.0,port=8080,path=/live.mp4,transcode=1"
```

### Example 3: RTSP Streaming

Stream via RTSP:

```bash
vlc input.mp4 --stream-wizard-config="dest=rtsp,port=8554,path=/live"
```

Access the stream at: `rtsp://localhost:8554/live`

### Example 4: Save to File with Transcoding

Transcode and save to a file:

```bash
vlc input.avi --stream-wizard-config="profile='Video - H.264 + AAC (MP4)',dest=file,addr=/path/to/output.mp4,transcode=1"
```

### Example 5: RTP Streaming with SAP Announcement

Stream via RTP with SAP announcement:

```bash
vlc input.mp4 --stream-wizard-config="dest=rtp,addr=239.255.1.1,port=5004,sap_name='My Stream'"
```

### Example 6: Stream and Display Locally

Stream via HTTP while displaying the video locally:

```bash
vlc input.mp4 --stream-wizard-config="dest=http,addr=0.0.0.0,port=8080,path=/stream,local=1"
```

### Example 7: Icecast Streaming

Stream to an Icecast server:

```bash
vlc input.mp3 --stream-wizard-config="dest=icecast,addr=icecast.example.com,port=8000,ice_mount=mystream,ice_password='hackme'"
```

### Example 8: SRT Streaming

Stream via SRT (Secure Reliable Transport):

```bash
vlc input.mp4 --stream-wizard-config="profile='Video - H.264 + AAC (TS)',dest=srt,addr=127.0.0.1,port=7001,transcode=1"
```

## Technical Details

### Implementation

This feature is implemented using:

1. **C++ Wrapper** (`modules/gui/qt/cli_stream_wrapper.cpp`): Wraps the Qt GUI stream dialog logic
2. **C Integration** (`src/stream_wizard_cli.c`): Parses CLI parameters and calls the C++ wrapper
3. **Command-Line Option** (`src/libvlc-module.c`): Defines the `--stream-wizard-config` option

The wrapper reuses the exact same code paths as the GUI stream dialog, ensuring consistent behavior between CLI and GUI modes.

### How It Works

1. VLC parses the `--stream-wizard-config` option during initialization
2. The configuration string is parsed into a `CLIStreamParams` structure
3. The C++ wrapper function `vlc_GenerateSoutStringFromCLI()` is called
4. The wrapper builds a stream output chain using the same logic as `SoutDialog::updateChain()`
5. The generated sout chain is set as the global `sout` variable
6. VLC applies this chain to the media being played

## Notes

- Values containing spaces or special characters should be quoted (single or double quotes)
- The generated sout chain can be seen in VLC's debug log (use `-vv` for verbose output)
- This feature requires the Qt module to be compiled
- Port numbers default to standard values if not specified:
  - HTTP: 8080
  - RTSP: 8554
  - RTP: 5004
  - SRT: 7001
  - RIST: 1968
  - Icecast: 8000
  - UDP: 1234

## Troubleshooting

### Error: "Stream wizard: destination type is required"

Make sure you specify the `dest` parameter in your configuration.

### Error: "Failed to generate stream output chain"

Check that:
- Your profile name is correctly quoted and matches one of the available profiles
- Required parameters for your destination type are provided
- The address/path format is valid for the destination type

### Viewing the Generated Chain

To see the exact sout chain generated, run VLC with verbose logging:

```bash
vlc -vv input.mp4 --stream-wizard-config="..." 2>&1 | grep "Generated stream output chain"
```

## Future Enhancements

Possible future improvements:

- Support for custom profile definitions from config files
- Additional advanced streaming parameters
- Preset configurations for common use cases
- JSON-based configuration format option

## Contributing

If you find bugs or have suggestions for improvements, please submit an issue or pull request to the VLC repository.
