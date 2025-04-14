# Changelog

All notable changes to the CO2 Car Race Management System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
## [0.10.0] - 2025-04-15

### Added
- Enhanced Hardware Dashboard:
  - Robust WebSocket connection handling with automatic reconnection
  - Comprehensive error handling and logging system
  - Real-time sensor data visualization with progress bars
  - Status indicators for connection and sensor states
  - Communication log with auto-scroll and filtering
  - Hardware control panel with calibration and test tools
  - Dual lane sensor monitoring with independent status tracking
  - Visual feedback for sensor readings and detection states
  - Timer reset and test race functionality
  - Port management with dynamic refresh capability

### Changed
- Improved WebSocket implementation with better error recovery
- Enhanced sensor data display with visual progress indicators
- Updated hardware status display with more detailed information
- Reorganized hardware controls for better usability
- Improved error messaging and logging format

### Fixed
- Resolved WebSocket connection stability issues
- Fixed sensor reading display updates
- Corrected status indicator synchronization
- Addressed hardware control button state management

## [0.9.0] - 2025-04-15

### Added
- ESP32 Hardware Integration System:
  - Serial communication interface for ESP32 race timer hardware
  - JSON-based message protocol for reliable data exchange
  - Real-time sensor monitoring with visual feedback
  - Hardware control API for race officials
  - WebSocket integration for live updates
  - Comprehensive hardware management dashboard
  - Sensor calibration and diagnostics tools
  - Race timing system integration

## [0.8.0] - 2025-04-15

### Added
- Race Countdown Timer System:
  - WebSocket-based real-time synchronized countdown across all clients
  - Timer controls (start, pause, reset) for race officials
  - Customizable duration with input controls
  - Visual progress indicator with color changes based on remaining time
  - Status updates for all connected clients
  - RESTful API endpoints for timer control
  - Countdown manager utility for centralized timer management
  - Integration with existing heat management system

### Changed
- Enhanced heat detail page with improved countdown UI
- Added progress bar for visual countdown representation
- Updated WebSocket implementation for better real-time synchronization
## [0.7.0] - 2025-04-14

### Added
- Comprehensive Results Export System:
  - Excel export functionality for race data
  - Export buttons on relevant pages
  - Custom formatted reports for different data types
  - Multiple export options:
    - Complete event results export
    - Event standings export
    - Round results export
    - Heat results export
  - Properly formatted spreadsheets with headers and styling
  - Automatic filename generation based on export type

### Changed
- Enhanced UI with export buttons on event, standings, round, and heat pages
- Improved data organization in exports for better readability
- Added dedicated export routes and blueprint

## [0.6.0] - 2025-04-14

### Added
- Comprehensive check-in management system:
  - Check-in deadline management with countdown timers
  - Automated notifications for racers who haven't checked in
  - Real-time check-in status updates via WebSockets
  - Bulk check-in functionality for race officials
  - Check-in status tracking and visualization
  - Configurable check-in deadlines per event
- Scheduled background tasks:
  - Automatic check-in notification system
  - Scheduled task runner for background processes
- Configuration management system:
  - JSON-based configuration storage
  - Timezone support for deadlines

### Changed
- Enhanced Event model with check-in deadline management methods
- Improved racer management interface with check-in status indicators
- Reorganized application structure with dedicated check-in routes

## [0.5.0] - 2025-04-14

### Added
- Comprehensive "On Deck" display system:
  - Dedicated On Deck page showing current and upcoming heats
  - Color-coded lane assignment cards for better visualization
  - Real-time countdown timers for current and next heats
  - Race preparation checklist for officials
  - Visual status indicators for each lane
  - Dashboard integration with On Deck information
- Enhanced heat detail view:
  - Improved lane assignment cards with racer information
  - Visual indicators for race positions and results
  - Interactive countdown timer with reset functionality
  - Confirmation dialogs for race control actions
- New navigation elements:
  - On Deck link in main navigation
  - Quick access to full On Deck view from dashboard

### Changed
- Redesigned heat detail page with more prominent lane assignments
- Enhanced dashboard with On Deck preview section
- Improved user experience for race preparation and monitoring

## [0.4.0] - 2025-04-14

### Added
- Comprehensive real-time standings system:
  - New Standing model for tracking racer performance metrics
  - Points calculation based on race results
  - Automatic ranking with tie handling
  - Best time, average time, and race count tracking
  - Win tracking and statistics
- Enhanced event features:
  - Configurable points system per event
  - Standings visualization with charts
  - Real-time standings updates after each heat
- New user interface components:
  - Event standings page with detailed statistics
  - Heat results recording interface
  - Points visualization with Chart.js
  - Navigation links to standings throughout the application

### Changed
- Enhanced Racer model with methods for calculating points and statistics
- Improved heat detail page with results recording functionality
- Updated database schema with new standings table

## [0.3.0] - 2025-04-14

### Added
- Comprehensive race scheduling system:
  - New data models for rounds, heats, and lane assignments
  - Round-robin scheduling algorithm for fair race matchups
  - Multiple round support (qualifying, semi-finals, finals)
  - Automatic heat generation based on racer count and lane availability
- Racer check-in system:
  - Racer status tracking (checked-in vs not checked-in)
  - Check-in timestamp recording
  - Integration with race scheduling
- Enhanced user interface:
  - Race schedule generation interface
  - Round and heat detail views
  - Lane assignment displays
  - Navigation between all race components

### Changed
- Enhanced event detail page with race scheduling options
- Improved database schema with relationships between races, heats, and lanes
- Updated race result tracking to include lane assignments

## [0.2.0] - 2025-04-13

### Added
- User roles system with admin, race official, and viewer roles
- Enhanced user profiles with additional fields (full name, phone, organization, bio, last login)
- Event management improvements:
  - Event editing functionality
  - Event archiving (soft delete) with timestamp
  - Creator tracking for events
  - Role-based access control for event management
- Database migrations using Flask-Migrate

### Fixed
- Fixed date comparison issues between date and datetime objects
- Added null checks for date/time fields in templates
- Fixed race creation functionality
- Resolved issues with Racer model (group field vs class_name)

## [0.1.0] - 2025-04-13

### Added
- Initial release of the Race Management System
- User authentication system with login, register, and logout functionality
- Dashboard view showing recent events and upcoming races
- Event management:
  - List all events
  - Create new events
  - View event details
- Race management:
  - View race details
  - Start and end races
  - Live race data support via WebSocket
  - Race results tracking
- Racer management:
  - List all racers
  - Add new racers
  - Edit racer details
  - Delete racers
- Bootstrap 5 UI with responsive design
- Flash message system for user notifications
- SQLite database with Flask-SQLAlchemy
- WebSocket support for real-time race updates

### Fixed
- Resolved Python 3.13 compatibility issues with SQLAlchemy
- Fixed template syntax in JavaScript code
- Corrected circular import issues in Flask-Login setup

### Security
- Implemented user authentication with Flask-Login
- Password hashing using Werkzeug security
- Protected routes with login_required decorator
- CSRF protection in forms





## [Unreleased]
### Planned
- Advanced race statistics and analytics
- Race data validation
- Live race timing
- Car specifications tracking
- Data backup system
- Mobile app integration
- Race simulation mode for testing
