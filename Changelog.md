DebugEnhancer Changelog
============================
#### v1.1.0
- Fixed loading on macOS 10.10 and older due to a MacKernelSDK regression

#### v1.0.9
- Added constants for macOS 15 support

#### v1.0.8
- Added constants for macOS 14 support

#### v1.0.7
- Added constants for macOS 13 support

#### v1.0.6
- Workaround for macos 12 (Monterey) and higher: do not use log_setsize

#### v1.0.5
- Support boot-arg `-dbgenhiolog` to redirect IOLog output to kernel vprintf

#### v1.0.4
- Use method routeMultipleLong instead of routeMultiple in order to avoid conflict with HibernationFixup

#### v1.0.3
- Added constants for macOS 12 support

#### v1.0.2
- Added MacKernelSDK with Xcode 12 compatibility

#### v1.0.1
- Added constants for 11.0 support

#### v1.0.0
- Initial release
