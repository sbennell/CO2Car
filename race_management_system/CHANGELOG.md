# Changelog

All notable changes to the CO2 Car Race Management System will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
- Race scheduling system
- Advanced race statistics and analytics
- User roles and permissions
- Export race results to PDF/Excel
- Email notifications for race events
- Mobile app integration
- Race simulation mode for testing
