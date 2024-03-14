# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

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
