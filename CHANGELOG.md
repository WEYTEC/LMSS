# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

## [4.3.2] - 2026-07-08
- don't reposition pointer when set to the screen we're already on

## [4.3.1] - 2026-06-02

### Fixed
- don't assume a monitor with screen position 0,0 exists

## [4.3.0] - 2025-10-02

### Fixed
- fix 100% CPU usage when waiting for mouse position from USB device

## [4.2.3] - 2024-04-24

### Changed
- smarttouch firmware 8.7 (26.03.2024) or newer is required since LMSS 4.2.x

### Fixed
- fix border trigger when mouse button is pressed and released on different screens

## [4.2.2] - 2024-03-28

### Changed
- set border clearance to 1pxl

## [4.2.1] - 2024-03-28

### Fixed
- fix X root window when using manual screen layout configuration

## [4.2.0] - 2024-03-26

### Fixed
- only trigger border when we got a position response for the last border triggered

## [4.1.2] - 2024-03-14

### Changed
- border detection now also considers the root window of the screen, this allows
  setups with multiple X screens where the screen/monitor layout is not managed
  by XRandR

## [4.0.8] - 2024-03-14

### Fixed
- leaked timer file descriptor

### Changed
- send border event only once

### Added
- optional config file (`/etc/lmss.sl`) with screen layout definitions

## [4.0.6] - 2024-03-05

### Fixed
- display height calculation
- leaked epoll file descriptor

### Removed
- udev rule, not useful anymore

### Changed
- make sure lmss is only started once when started using the `.desktop` file

## [4.0.5] - 2024-03-05

### Fixed
- fix udev rule

## [4.0.4] - 2024-02-19

### Removed
- Solaris support
- libusb dependency

### Changed
- use pointer events instead of polling the pointer position
- use argparse to parse command line arguments (`-v7` becomes `-v 7`)
