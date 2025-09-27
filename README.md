# Smart Desk Dashboard

A professional web dashboard for monitoring your Arduino-based smart desk environmental sensors. This system tracks light levels, temperature, distance, and user presence to provide an overall environmental score for your workspace.

## Features

- **Real-time Monitoring**: Live data from Arduino sensors via serial communication
- **Environmental Scoring**: Intelligent algorithm that scores workspace conditions (0-100)
- **Interactive Dashboard**: Modern, responsive web interface with charts and visualizations
- **Historical Data**: SQLite database for data persistence and trend analysis
- **WebSocket Updates**: Real-time data streaming to connected clients
- **Professional UI**: Clean, modern design with status indicators and animations

## Sensors Monitored

- **Light Sensor (LDR)**: Monitors ambient lighting conditions
- **Temperature Sensor (Thermistor)**: Tracks workspace temperature
- **Distance Sensor (HC-SR04)**: Measures user distance from desk
- **RFID Sensor**: Detects user presence

## Environmental Scoring Algorithm

The system calculates an environmental score (0-100) based on:

- **Light Quality** (0-30 points): Optimal lighting conditions
- **Temperature Comfort** (0-30 points): 20-25°C range preferred
- **Posture Distance** (0-25 points): 35-75cm optimal range
- **User Presence** (0-15 points): Active workspace usage bonus

## Installation

### Prerequisites

- Node.js (v14 or higher)
- Arduino with sensors connected
- Windows/Linux/Mac with serial port access

### Setup

1. **Install Dependencies**
   ```bash
   npm install
   ```

2. **Connect Arduino**
   - Upload your `smartdesk.ino` sketch to Arduino
   - Connect sensors as defined in the code
   - Note the COM port (Windows) or device path (Linux/Mac)

3. **Start the Server**
   ```bash
   npm start
   ```

4. **Access Dashboard**
   - Open browser to `http://localhost:3000`
   - The system will automatically detect Arduino connection

## Arduino Wiring

### Light Sensor (LDR)
- Connect LDR between 5V and analog pin A0
- Connect 10kΩ resistor between A0 and GND

### Temperature Sensor (Thermistor)
- Connect thermistor between 5V and analog pin A1
- Connect 10kΩ resistor between A1 and GND

### Distance Sensor (HC-SR04)
- VCC → 5V
- GND → GND
- Trig → Digital Pin 9
- Echo → Digital Pin 8

### RFID Sensor (MFRC522)
- VCC → 3.3V
- GND → GND
- RST → Digital Pin 5
- SS → Digital Pin 10

### LEDs (Status Indicators)
- LED1 (Temperature) → Digital Pin 13
- LED2 (Distance) → Digital Pin 12
- LED3 (Light) → Digital Pin 11

### Button
- Connect button between Digital Pin 7 and GND
- Internal pullup enabled

## API Endpoints

- `GET /api/current` - Current sensor data
- `GET /api/history?hours=24` - Historical data
- `GET /api/stats` - 24-hour statistics

## WebSocket Events

- `sensorData` - Real-time sensor updates
- `connect` - Client connection established
- `disconnect` - Client disconnected

## Dashboard Features

### Main Dashboard
- **Environmental Score**: Large gauge showing overall workspace health
- **Sensor Cards**: Individual sensor readings with status indicators
- **Real-time Charts**: Historical trends and data visualization
- **Statistics**: 24-hour averages and performance metrics

### Responsive Design
- Mobile-friendly interface
- Adaptive grid layouts
- Touch-optimized controls

## Data Storage

The system uses SQLite database (`smartdesk.db`) to store:
- Sensor readings with timestamps
- Environmental scores
- Historical trends
- Performance statistics

## Troubleshooting

### Arduino Not Detected
1. Check USB connection
2. Verify COM port in Device Manager
3. Ensure Arduino IDE can connect
4. Try different USB cable/port

### No Data Updates
1. Verify serial communication (115200 baud)
2. Check Arduino serial output
3. Restart server after Arduino connection
4. Check firewall/antivirus blocking

### Database Issues
1. Delete `smartdesk.db` to reset
2. Check file permissions
3. Ensure sufficient disk space

## Development

### Running in Development Mode
```bash
npm run dev
```

### File Structure
```
smartdesk/
├── server.js              # Main server file
├── package.json           # Dependencies
├── smartdesk.db          # SQLite database
├── public/
│   ├── index.html        # Dashboard HTML
│   ├── styles.css        # CSS styles
│   └── app.js            # Frontend JavaScript
└── README.md             # This file
```

## Contributing

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

## License

MIT License - see LICENSE file for details

## Support

For issues and questions:
1. Check troubleshooting section
2. Review Arduino serial output
3. Check server console logs
4. Verify sensor connections

---

**Smart Desk Dashboard** - Professional IoT workspace monitoring solution