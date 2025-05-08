const WebSocket = require('ws');
const express = require('express');
const path = require('path');

// Create HTTP server
const app = express();
const PORT = 81;
const server = app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});

// Serve static files for dashboard
app.use(express.static(path.join(__dirname, 'public')));

// Create WebSocket server
const wss = new WebSocket.Server({ server });

// Store connected clients and latest sensor data
const clients = new Set();
let classroomState = {
  occupancy: 0,
  temperature: 0,
  humidity: 0,
  airQuality: 0,
  lightState: false,
  fanState: false,
  lightAuto: true,
  fanAuto: true,
  lastUpdate: null
};

wss.on('connection', (ws) => {
  console.log('New client connected');
  clients.add(ws);

  // Send initial state to new client
  ws.send(JSON.stringify({
    type: 'init',
    data: classroomState
  }));

  // Heartbeat system
  const heartbeatInterval = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.ping();
    }
  }, 15000);

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log(data);
      // Handle different message types
      switch(data.type) {
        case 'sensorUpdate': // From ESP32
          handleSensorUpdate(data);
          break;
          
        case 'controlCommand': // From Dashboard
          forwardControlCommand(data);
          break;
          
        case 'pong':
          // Update last heartbeat
          ws.isAlive = true;
          break;
      }
    } catch (e) {
      console.error('Error processing message:', e);
    }
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    clients.delete(ws);
    clearInterval(heartbeatInterval);
  });

  ws.on('error', (error) => {
    console.error('WebSocket error:', error);
  });

  ws.on('pong', () => {
    ws.isAlive = true;
  });
});

// Handle sensor updates from ESP32
function handleSensorUpdate(data) {
  // Update classroom state
  classroomState = {
    ...classroomState,
    ...data,
    lastUpdate: new Date().toISOString()
  };

  // Broadcast to all dashboard clients
  broadcastToDashboards({
    type: 'stateUpdate',
    data: classroomState
  });
}

// Forward control commands to ESP32
function forwardControlCommand(data) {
  clients.forEach(client => {
    if (client.deviceType === 'esp32' && client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify({
        type: 'control',
        command: data.command,
        value: data.value
      }));
    }
  });
}

// Broadcast to all dashboard clients
function broadcastToDashboards(message) {
  clients.forEach(client => {
    if (client.deviceType === 'dashboard' && client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(message));
    }
  });
}

// Cleanup dead connections
setInterval(() => {
  clients.forEach(client => {
    if (!client.isAlive) {
      client.terminate();
      clients.delete(client);
      return;
    }
    client.isAlive = false;
  });
}, 30000);

// Identify device type (ESP32/Dashboard)
wss.on('headers', (headers, req) => {
  const deviceType = req.url === '/esp32' ? 'esp32' : 'dashboard';
  headers.push(`X-Device-Type: ${deviceType}`);
});