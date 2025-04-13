# CO2 Car Race Management System

A comprehensive web-based system for managing CO2 car racing events, built with Python Flask, SQLAlchemy, and SocketIO.

## Features

- **Race Management**: Schedule and manage race events with multiple heats and rounds
- **Racer Registration**: Register participants and assign them to races
- **Real-time Race Results**: View race results in real-time as they happen
- **Race Scheduling**: Automatic generation of race schedules using Young and Pope "Partial Perfect-N" charts
- **ESP32 Integration**: Direct communication with the CO2 Car Race Timer hardware
- **Results Tracking**: Track and display race results and standings
- **On-Deck Display**: Show upcoming races and racers
- **Check-in System**: Manage racer check-ins for events
- **Data Export**: Export race results to spreadsheets

## Installation

1. **Clone the repository**:
   ```
   git clone https://github.com/yourusername/CO2Car.git
   cd CO2Car/race_management_system
   ```

2. **Create a virtual environment**:
   ```
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

3. **Install dependencies**:
   ```
   pip install -r requirements.txt
   ```

4. **Set up environment variables**:
   Create a `.env` file in the root directory with the following variables:
   ```
   FLASK_APP=run.py
   FLASK_ENV=development
   SECRET_KEY=your-secret-key
   DATABASE_URI=sqlite:///race_management.db
   ```

5. **Initialize the database**:
   ```
   python -c "from app import create_app, db; app = create_app(); app.app_context().push(); db.create_all()"
   ```

6. **Run the application**:
   ```
   python run.py
   ```
   The application will be available at `http://localhost:5000`.

## ESP32 Communication

The system communicates with the ESP32-based CO2 Car Race Timer via serial connection. The communication protocol uses JSON messages for exchanging race data and commands.

### Connection Setup

1. Connect the ESP32 to your computer via USB
2. The system will automatically detect the device and establish a connection
3. Race results will be automatically synced from the device to the race management system

### API Endpoints

- `/api/device/race_result` - Receive race results from the ESP32 device
- `/api/device/status` - Receive device status updates
- `/api/races` - Get race information

## Project Structure

```
race_management_system/
├── app/                     # Application package
│   ├── __init__.py          # Application initialization
│   ├── models/              # Database models
│   │   ├── user.py          # User authentication model
│   │   └── race.py          # Race-related models
│   ├── routes/              # Route definitions
│   │   ├── main.py          # Main website routes
│   │   └── api.py           # API endpoints
│   ├── static/              # Static files (CSS, JS, images)
│   ├── templates/           # HTML templates
│   └── utils/               # Utility modules
│       └── esp32_comm.py    # ESP32 communication module
├── run.py                   # Application entry point
├── requirements.txt         # Python dependencies
└── README.md                # Project documentation
```

## Development

### Adding New Features

1. Create a new branch for your feature
2. Implement the feature with appropriate tests
3. Submit a pull request for review

### Code Style

Follow PEP 8 guidelines for Python code and use consistent naming conventions.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 