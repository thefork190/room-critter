#!/usr/bin/env python3
"""Fetch Google's native header after you review ARCore Additional Terms.
This script does not accept agreements on your behalf.
"""
from pathlib import Path
import hashlib, urllib.request
URL = "https://raw.githubusercontent.com/google-ar/arcore-android-sdk/204ddfd1715eb0cb775bf7528c9aee9d93cd4d4a/libraries/include/arcore_c_api.h"
SHA256 = "0dfc2ae3f70acaca7b1fa28e9303a2ce8f1212836809f21306e9226a4d383b50"
data = urllib.request.urlopen(URL, timeout=60).read()
if hashlib.sha256(data).hexdigest() != SHA256:
    raise SystemExit("Header checksum mismatch; nothing written")
p = Path(__file__).resolve().parent.parent / "libraries/include/arcore_c_api.h"
p.parent.mkdir(parents=True, exist_ok=True)
p.write_bytes(data)
print("Verified official ARCore header; separate terms: https://developers.google.com/ar/develop/terms")
