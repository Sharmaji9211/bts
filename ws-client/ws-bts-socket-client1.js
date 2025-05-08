
        //create new  websocket client to connect with ws server
        const ws = new WebSocket('ws://localhost:81');

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            if (data.type === 'stateUpdate') {
                updateDashboard(data.data);
            }
        };

        function updateDashboard(state) {
            document.getElementById('occupancy').textContent = state.occupancy;
            document.getElementById('temperature').textContent = state.temperature.toFixed(1);
            document.getElementById('humidity').textContent = state.humidity.toFixed(1);
            document.getElementById('airQuality').textContent = state.airQuality;
            document.getElementById('lightAuto').checked = state.lightAuto;
            document.getElementById('fanAuto').checked = state.fanAuto;
        }

        function sendCommand(command, value) {
            ws.send(JSON.stringify({
                type: 'controlCommand',
                command: command,
                value: value
            }));
        }

        function toggleAutoMode(device) {
            const autoMode = document.getElementById(`${device}Auto`).checked;
            ws.send(JSON.stringify({
                type: 'controlCommand',
                command: 'mode',
                device: device,
                value: autoMode
            }));
        }
    