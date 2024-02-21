# Change Log
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased]

### Added
- optional config file (`/etc/lmss.sl`) with screen layout definitions

## [4.0.4] - 2024-02-19

### Removed
- Solaris support
- libusb dependency

### Changed
- use pointer events instead of polling the pointer position
- use argparse to parse command line arguments (`-v7` becomes `-v 7`)
