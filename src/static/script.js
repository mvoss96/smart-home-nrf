"use strict";
const url = "";
const static_username = "USER";
let headers = new Headers();
let allLogs = []; // Global variable to store all fetched logs

function setError(message) {
    const errorMessageElement = document.getElementById('error-message');
    errorMessageElement.textContent = message;
}


window.onload = function () {
    document.getElementById('loginForm').addEventListener('submit', submitLogin);
    // Listen to filter change and immediately re-render logs
    document.getElementById('severityFilter').addEventListener('change', () => {
        renderLogs();
    });
}

function submitLogin(event) {
    event.preventDefault();  // Prevent the form from submitting normally.

    const password = document.getElementById('password').value;
    //console.log(`submitted ${password}`)

    // Base64 encode the username and password for Basic Auth.
    headers.set('Authorization', 'Basic ' + btoa(static_username + ":" + password));

    // Try to fetch the data once to check if the login is successful.
    checkLogin();
}

async function checkLogin() {
    const loginSuccessful = await fetchAndPopulate();
    if (loginSuccessful) {
        setError("");
        document.getElementById('log-container').style.display = "block";
        fetchAndPopulate();
        fetchLogs();
        setInterval(fetchAndPopulate, 3000);
        setInterval(fetchLogs, 1000);
        document.getElementById("loginForm").style.display = "none";
        document.getElementById("devicesTable").style.display = "block";
    }
}

async function fetchAndPopulate() {
    //console.log("fetch");
    let response;
    try {
        response = await fetch(url + "/devices", { headers });
    }
    catch (error) {
        setError("404 Server not reachable!");
        return false;
    }
    if (response.status !== 200) {
        setError('Login Failed!');
        return false;  // you should return here to avoid further execution
    }
    setError("");
    const devices = await response.json();
    devices.sort((a, b) => a.id - b.id); // Sort devices by device.id
    const table = document.getElementById('devicesTable');
    while (table.rows.length > 1) { table.deleteRow(1); }
    devices.forEach(device => populateTable(device, table));
    return true
}


function populateTable(device, table) {
    console.log(device)
    let row = table.insertRow();
    row.insertCell().innerHTML = `<span id="deviceName_${device.uuid}" class="device-name">
                ID:${device.id}<br>${device.name}<br><span class="small-font">(${device.type})</span>
                </span><br><button onclick="renameDevice('${device.uuid}')">Rename</button>
                <button onclick="removeDevice('${device.uuid}')">Remove</button>`;

    let statusList = document.createElement('ul');
    let intervalElement = document.createElement('li');
    intervalElement.textContent = "status_interval: " + (device.status_interval > 0 ? (device.status_interval + "s") : "N/A");
    statusList.appendChild(intervalElement);
    for (let prop in device.status) {
        let listItem = document.createElement('li');
        if (prop === "power") { listItem.textContent = `power: ${device.status["power"] ? "on" : "Off"}`; }
        else { listItem.textContent = `${prop}: ${device.status[prop]}`; }
        statusList.appendChild(listItem);
    }
    let statusCell = row.insertCell();
    statusCell.appendChild(statusList);
    row.insertCell().textContent = device.connection_health ? device.connection_health * 100 + '%' : 'N/A';
    row.insertCell().textContent = device.battery_powered ? Math.round(device.battery_level / 2.55) + '%' : 'N/A';
    let uuidHex = device.uuid.map(num => num.toString(16).toUpperCase()).join(', '); // Converts each number to hex and joins them
    row.insertCell().innerHTML = device.uuid.join(':') + "<br>{" + uuidHex + "}";
    row.insertCell().textContent = device.version;
    row.insertCell().textContent = formatElapsedTime(device.last_seen);
}

async function renameDevice(uuid) {
    uuid = uuid.replaceAll(",", "-");
    let newName = prompt("Enter new device name:");
    if (newName !== null) {
        const requestHeaders = new Headers(headers);
        requestHeaders.set('Content-Type', 'application/json');
        const renameOptions = {
            method: 'PUT',
            headers: requestHeaders,
            body: JSON.stringify({ name: newName })
        };
        try {
            const response = await fetch(`${url}/devices/${uuid}/name`, renameOptions);
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            await fetchAndPopulate(); // Await the fetchAndPopulate function
        } catch (error) {
            console.error('Error:', error);
        }
    }
}


async function removeDevice(uuid) {
    uuid = uuid.replaceAll(",", "-");
    if (confirm(`Do you want to remove Device ${uuid} ?`) === false) { return; }
    const requestHeaders = new Headers(headers);
    requestHeaders.set('Content-Type', 'application/json');
    const deleteOptions = {
        method: 'DELETE',
        headers: requestHeaders,
        body: ""
    };
    try {
        const response = await fetch(`${url}/devices/${uuid}`, deleteOptions);
        if (!response.ok) {
            throw new Error("HTTP error " + response.status);
        }
        await fetchAndPopulate(); // Await the fetchAndPopulate function
    } catch (error) {
        console.error('Error:', error);
    }
}


function formatElapsedTime(last) {
    let currentTime = new Date();
    let elapsedTime = Math.floor((currentTime - new Date(last)) / 1000);
    let formattedTime;

    // if (elapsedTime < 5) {
    //     formattedTime = "< 5s";
    // } else 
    if (elapsedTime < 60) {
        formattedTime = `${elapsedTime}s`;
    } else if (elapsedTime < 3600) {
        formattedTime = `${Math.floor(elapsedTime / 60)}m`;
    } else if (elapsedTime < 86400) { // 24 * 60 * 60 = 86400 seconds in a day
        formattedTime = `${Math.floor(elapsedTime / 3600)}h`;
    } else {
        formattedTime = `${Math.floor(elapsedTime / 86400)}d`;
    }
    return formattedTime;
}

async function fetchLogs() {
    try {
        const response = await fetch(url + "/logs", { headers });
        if (response.status !== 200) {
            setError('Failed to fetch logs!');
            return;
        }
        allLogs = await response.json(); // Update the global variable
        renderLogs(); // Call the render function to update the UI
    } catch (error) {
        setError("Could not fetch logs!");
        console.error('Error:', error);
    }
}

function applyFilter(logs) {
    const selectedSeverity = document.getElementById('severityFilter').value;
    if (selectedSeverity === "warnings") {
        return logs.filter(log => log.severity.toLowerCase() === "warning" || log.severity.toLowerCase() === "error" || log.severity.toLowerCase() === "critical");
    }
    return logs; // Return unfiltered logs if no filter is selected
}

function renderLogs() {
    const logsDiv = document.getElementById('logs');
    const isAtBottom = logsDiv.scrollHeight - logsDiv.clientHeight <= logsDiv.scrollTop + 1;

    logsDiv.innerHTML = '';
    let filteredLogs = applyFilter(allLogs); // Apply the filters
    let i = 1;
    filteredLogs.forEach(log => {
        const logElement = document.createElement('div');

        // Formatting the log entry
        const timestampElement = document.createElement('span');
        timestampElement.className = 'log-timestamp';
        timestampElement.textContent = `${i++} [${formatElapsedTime(1000 * log.timestamp)}] `;

        const severityElement = document.createElement('span');
        severityElement.className = `log-severity log-${log.severity.toLowerCase()}`;
        severityElement.textContent = `${log.severity} `;

        const messageElement = document.createElement('span');
        messageElement.className = 'log-message';
        messageElement.textContent = log.message;

        // Append the formatted elements to the logElement
        logElement.appendChild(timestampElement);
        logElement.appendChild(severityElement);
        logElement.appendChild(messageElement);

        logsDiv.appendChild(logElement);
    });

    // Auto-scroll to the bottom only if the user is already at the bottom
    if (isAtBottom) {
        logsDiv.scrollTop = logsDiv.scrollHeight;
    }
}

