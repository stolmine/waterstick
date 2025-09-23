# WaterStick VST3 Build & Distribution Guide

## Build Environment Requirements
- macOS Ventura or later
- Xcode 15.x
- CMake 3.25+
- VST3 SDK (latest version)

## Code Signing & Distribution Challenges

### AMFI (Apple Mobile File Integrity) Validation
**Problem**:
- Audio plugins require strict code signing validation
- Default signing methods often fail for complex VST3 plugins

**Solution**:
- Use custom entitlements specific to audio plugins
- Implement comprehensive code signing configuration in CMake

### Signing Configuration
```cmake
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
```

### Entitlements File
Create `waterstick.entitlements` with:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.app-sandbox</key>
    <true/>
    <key>com.apple.security.network.client</key>
    <true/>
    <key>com.apple.security.files.user-selected.read-write</key>
    <true/>
    <key>com.apple.security.device.audio-input</key>
    <true/>
</dict>
</plist>
```

## Build Process

### CMake Configuration
1. Generate Xcode project
```bash
mkdir build && cd build
cmake -G "Xcode" ..
```

2. Build Plugin
```bash
cmake --build . --config Release
```

### Code Signing Commands
```bash
# Sign the VST3 plugin
codesign --force --deep --sign "Developer ID Application" \
    --entitlements waterstick.entitlements \
    build/VST3/Release/WaterStick.vst3
```

## Plugin Installation

### DAW Installation Procedure
1. Close all running DAWs
2. Clear plugin cache:
   - Logic Pro: Delete `/Users/[username]/Library/Caches/Logic`
   - Ableton: Delete `/Users/[username]/Library/Caches/Ableton`
   - FL Studio: Delete plugin cache in Preferences

3. Copy plugin:
```bash
cp -R build/VST3/Release/WaterStick.vst3 \
    "/Library/Audio/Plug-Ins/VST3/"
```

## Troubleshooting

### Common Issues
- **Error -423**: Adhoc signing rejection
  - Ensure complete entitlements
  - Verify developer certificate
  - Check SDK compatibility

- **CMS Blob Errors**:
  - Regenerate development certificates
  - Update Xcode command-line tools
  - Verify Apple Developer account

### Validation
- Run VST3 plugin validator
- Test in multiple DAWs
- Verify parameter automation
- Check CPU performance

## Best Practices
- Always backup before major build
- Use version control
- Document build environment
- Test thoroughly after each build

## Performance Monitoring
- Monitor CPU usage
- Check for audio glitches
- Validate low-latency performance

*Last Updated: 2025-09-23*