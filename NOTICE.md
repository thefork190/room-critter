# Third-party notices and provenance

Room Critter derives from Google ARCore Android SDK `samples/hello_ar_c`:
https://github.com/google-ar/arcore-android-sdk
Upstream snapshot: 204ddfd1715eb0cb775bf7528c9aee9d93cd4d4a (downloaded 2026-11-20).
Google sample source, retained sample assets and shaders are licensed under Apache License 2.0;
see LICENSE and individual file headers. GLM retains its license in third_party/glm/LICENSE.
ARCore runtime is obtained from Google Maven rather than stored in source.

Modified files retain the Google copyright and license header. Room Critter modifications:
plane-only single-anchor placement; original generated critter/route; direct depth controls;
readiness/tracking UI; valid-frame fallback; stride-aware depth upload/lifetime fix;
shader-program refresh; modern ARM64 build configuration; tests and documentation.
The native API header is NOT included in this source distribution. It has separate ARCore
Additional Terms: https://developers.google.com/ar/develop/terms . The setup script fetches
the recorded upstream snapshot and checks its SHA-256; this prototype calls only APIs
available in pinned runtime 1.48.0. No cloud APIs are used.

Original generated model/texture: tools/generate_critter.py, no purchased or AI-generated
art assets. Copyright 2026 Room Critter contributors. SPDX-License-Identifier: Apache-2.0.
