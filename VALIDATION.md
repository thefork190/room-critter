# Validation record

## Completed on the Linux build host
- Procedural mesh checks: 1,989 vertices, 2,856 triangles, valid indices and finite attributes.
- PNG dimensions and Android resource/manifest XML parse checks passed.
- C++ motion bounds/reset and stride-aware 16-bit depth packing tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because this container cannot run its ptrace-based leak check.
- Independent source review covered native lifecycle, ARCore references, depth texture lifetime and shader precision. Findings were corrected.

## Android artifact
- `assembleDebug` and `lintDebug`: BUILD SUCCESSFUL. Lint: 0 errors, 30 warnings (primarily inherited sample resources/API guidance and pinned dependency updates).
- APK: Room Critter 1.0 (version code 1), `com.thefork.roomcritter`, ARM64 only, minSdk 24, targetSdk 35.
- Debug APK v2 signature verification passed (one signer). This is not a release signing identity.
- `zipalign -c -P 16 4` passed; all three bundled native libraries have ELF LOAD alignment 0x4000 (16 KB).
- APK SHA-256: `5b0aed8748071012bf50e59d952d29f6fc2c4aa296ae580a63b9ebf65c83ed7c`.
- Built with Temurin JDK 17.0.21.1, AGP 8.9.1, Gradle 8.11.1, NDK 28.2.13676358, CMake 3.22.1.
- Header snapshot and checksum are recorded in `tools/fetch_arcore_header.py`; native compilation/linking succeeded against ARCore 1.48.0.

## Pending device validation
- No emulator or physical-phone execution has been performed. All visual quality, GL shader compilation, camera/tracking behavior, real-depth occlusion, lifecycle and performance checks require the device checklist in README.md.
