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

## Critical Build Corruption Prevention

### Bundle Configuration Files
**Info.plist Requirements** (Critical for loading):
```xml
<!-- Required fields that MUST be populated -->
<key>CFBundleIdentifier</key>
<string>com.waterstick.vst3</string>
<key>CFBundleName</key>
<string>WaterStick</string>
<key>CFBundleShortVersionString</key>
<string>4.1.1</string>
<key>NSHumanReadableCopyright</key>
<string>© 2025 WaterStick Audio</string>
```

**moduleinfo.json Syntax** (Critical for VST3 compliance):
- **REMOVE all trailing commas** - JSON parsers will fail
- Common locations: "Flags" section, "Classes" arrays, nested objects
- Validate with `python -m json.tool moduleinfo.json` before deployment

### Ad-hoc Signing Fallback
When Developer ID certificates unavailable:
```bash
# Proven working fallback for development
codesign --force --deep --sign - /Library/Audio/Plug-Ins/VST3/WaterStick.vst3
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

- **Build Corruption Indicators**:
  - Empty CFBundleIdentifier or CFBundleName in Info.plist
  - JSON syntax errors in moduleinfo.json (trailing commas)
  - "sealed resource is missing or invalid" from Gatekeeper
  - Plugin loads but parameters not recognized by DAW

### Validation Protocol
- **VST3 Validator**: Must pass 47/47 tests for professional deployment
- **Parameter Count**: Verify parameters exported correctly
- **Bundle Integrity**: Check Info.plist and moduleinfo.json syntax
- **Code Signing Status**: Use `codesign -dv` to verify signature
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

*Last Updated: 2025-01-27 - Added critical corruption prevention knowledge*