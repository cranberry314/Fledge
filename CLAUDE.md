# Fledge

An iPhone app that flies a toy 2.4 GHz drone through a relay:

    iPhone app --Bluetooth LE--> ESP32 --SPI--> nRF24L01+ --2.4 GHz--> drone

The drone itself is never modified.

## This repo is public

Keep personal information out of every committed file, commit message and
issue: no real names, email addresses, Apple team IDs or device identifiers,
and nothing about who uses the app. Personal context lives in
`CLAUDE.local.md` and `ios/Local.xcconfig`, which git ignores. Run
`git status --ignored` before committing, and never add those two files.

## Layout

- `ios/project.yml`: XcodeGen generates `ios/Fledge.xcodeproj` from it.
  The `.xcodeproj` is gitignored and overwritten on every `xcodegen` run.
  Never edit it by hand, and don't trust settings changed in Xcode's UI
  unless they are also in `project.yml`.
- `ios/Fledge/` is a synced folder: new files there join the app without
  regenerating. Run `cd ios && xcodegen` only after changing `project.yml`.
- `ios/Fledge/Tuning.swift` is the one place settings live. One setting per
  line, with a short emoji comment above each one saying what it does, so a
  beginner can change a number and see the effect. Don't move settings into
  other files, and don't add clever code to this one.
- `ios/Signing.xcconfig` includes the gitignored `ios/Local.xcconfig`, which
  holds `DEVELOPMENT_TEAM`. Signing is Automatic.
- `firmware/`: ESP32 code. Not written yet; language still undecided
  (Arduino C++ or MicroPython). Ask before choosing.

## Code style

Keep the code simple enough for a beginner to read. Prefer plain, obvious
code over abstraction. Bluetooth and motion code stay in their own files,
so nobody needs to open them to change how Fledge flies.

Swift 5 language mode, MainActor by default, iOS 26.0 minimum, iPhone only,
portrait.

## Building from the command line

    cd ios
    xcodegen                      # only if project.yml changed
    xcodebuild -project Fledge.xcodeproj -scheme Fledge \
      -destination 'generic/platform=iOS Simulator' -derivedDataPath build build

To build for a connected iPhone, use `-destination 'id=<device UDID>'`
together with `-allowProvisioningUpdates`. Then install and launch it:

    xcrun devicectl list devices
    xcrun devicectl device install app --device <UDID> build/Build/Products/Debug-iphoneos/Fledge.app
    xcrun devicectl device process launch --device <UDID> <bundle id>

The simulator has no Bluetooth, so the drone link can only be tested on a
real phone.
