# Room Critter — first-week Android AR prototype

A native C++/OpenGL ES 3 critter in your room, hosted by a small Java Android activity.
Based on Google's official `hello_ar_c` sample, **not** a video overlay or simulated depth.

## What is implemented
- Scan a horizontal floor/table; tap a detected **upward-facing plane** to anchor one buddy.
- Original procedural mint/pink critter, approximately 25 cm tall. It bobs along a fixed
  70 cm side-to-side route centered on the anchor. Tap elsewhere to relocate; Reset clears it.
- **Depth ON/OFF** compares the same route with/without real ARCore depth occlusion.
- Runtime status distinguishes tracking paused, unsupported depth, depth off, waiting for
  a depth image, and active depth. Missing depth renders unoccluded rather than inventing it.
- ARCore automatic 16-bit depth texture with display-geometry UV transform and Google's
  depth comparison shader. No cloud anchors, account, API key, analytics or app uploads.
- Camera permission only in the app manifest. Google Play Services for AR must be installed;
  it is a separate service with its own behavior/terms.

This is **not** navigation, collision avoidance, room reconstruction, a persistent pet or
scene understanding. The route can cross table edges. Use a clear floor for the first test.

## Build

Requirements: JDK 17+ (a full JDK including javac, not only a JRE), Android SDK Platform 35, Build Tools 35.0.0,
NDK 28.2.13676358, CMake 3.22.1. Gradle 8.11.1 / Android Gradle Plugin 8.9.1,
ARCore 1.48.0. ARM64 only. Minimum Android 7/API 24; target API 35.

Also review ARCore Additional Terms before fetching its separately licensed native header:
https://developers.google.com/ar/develop/terms . The header is not redistributed here.

Review/accept Android SDK terms yourself or explicitly authorize your assistant to do so:
https://developer.android.com/studio/terms . SDK installation is not silently licensed.

```sh
python3 tools/fetch_arcore_header.py
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;28.2.13676358' 'cmake;3.22.1'
# Set ANDROID_HOME, or put sdk.dir=/your/android/sdk in local.properties.
./gradlew --no-daemon :app:assembleDebug :app:lintDebug
```

Output: `app/build/outputs/apk/debug/app-debug.apk`. Gradle's normal local debug signing
is for personal testing, not distribution; never commit a keystore. Application ID:
`com.thefork.roomcritter`. Retained Java/JNI namespace is Google's sample namespace to
keep the baseline recognizable; it does not affect the unique installed app ID.

Host-only tests (no Android device or SDK needed):
```sh
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -Iapp/src/main/cpp tests/native_tests.cc -o /tmp/room-critter-tests
/tmp/room-critter-tests
# In ptrace-based containers only: ASAN_OPTIONS=detect_leaks=0 /tmp/room-critter-tests
python3 tools/validate_assets.py
```

Regenerate original mesh/texture: `python3 tools/generate_critter.py` (Python stdlib only).

## Install on OnePlus 15

Google lists OnePlus 15 with Depth API support: https://developers.google.com/ar/devices .
Runtime detection remains authoritative; support and readiness are different.

1. Update Google Play Services for AR via Google Play if requested.
2. Transfer the verified debug APK to the phone and open it. Android may ask you to allow
   installation from that specific source; make that choice yourself, then turn it back off.
   Alternatively, with your own USB debugging authorization: `adb install -r app-debug.apk`.
3. Open Room Critter and grant camera permission. Use a well-lit textured floor, moving slowly
   sideways to establish tracking/depth, not just rotating in place. Tap the plane grid.
4. No phone run has been performed by the cloud builder. See `VALIDATION.md` for actual checks.

## Phone acceptance test (must be performed, not inferred from compilation)

- Place the critter on a floor in front of a chair/table. Walk around it: anchor should remain
  fixed, with natural scale, no mirroring/rotation jump. Only one critter after repeated taps.
- Position the fixed route to pass **behind** a chair/table leg. Leave the phone at the same
  viewpoint and compare a complete cycle with depth ON, then OFF. ON should hide the portion
  behind real geometry; OFF should visibly draw it over that object. Record both runs.
- Repeat on a chair seat/table edge, thin legs, low-texture/reflective surfaces and lower light;
  record depth limitations, edge halos and timing rather than assuming perfect occlusion.
- Confirm status shows waiting until a real depth frame arrives. Unsupported-depth fallback
  needs a genuinely unsupported device or a test seam; it is not verified by OnePlus support.
- Cover camera / move too quickly: tracking pauses, critter stops being drawn. Recover slowly.
- Deny permission, then relaunch and grant; background/foreground; rotate device; repeatedly
  toggle depth, relocate, reset, and open/close app. No crash, ghost anchors or stale depth.
- Check thermal behavior and frame pacing for five minutes. Report phone OS, AR service
  version, battery/heat, latency and glitches. This prototype makes no performance guarantee.

## Attribution / modifications

See `NOTICE.md`, Apache `LICENSE`, and `third_party/glm/LICENSE`. All original upstream
copyright headers are preserved. New helpers, generated critter and tests are Apache-2.0.
