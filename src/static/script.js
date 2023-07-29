"use strict";
const url = "";
const static_username = "USER";
let headers = new Headers();
window.onload = function () {
    document.getElementById('loginForm').addEventListener('submit', submitLogin);
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
        setInterval(fetchAndPopulate, 3000);
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
        alert("404 Server not reachable!");
        return false;
    }
    if (response.status !== 200) {
        alert('Login Failed!');
        return false;  // you should return here to avoid further execution
    }
    const devices = await response.json();
    const table = document.getElementById('devicesTable');
    while (table.rows.length > 1) { table.deleteRow(1); }
    devices.forEach(device => populateTable(device, table));
    return true
}


function populateTable(device, table) {
    let row = table.insertRow();
    row.insertCell().innerHTML = `<span id="deviceName_${device.uuid}" class="device-name">
                ID:${device.id}<br>${device.name}<br><span class="small-font">(${device.type})</span>
                </span><br><button onclick="renameDevice('${device.uuid}')">Rename</button>
                <button onclick="removeDevice('${device.uuid}')">Remove</button>`;

    let statusList = document.createElement('ul');
    for (let prop in device.status) {
        let listItem = document.createElement('li');
        if (prop === "power") { listItem.textContent = `Power: ${device.status["power"] ? "on" : "Off"}`; }
        else { listItem.textContent = `${prop.charAt(0).toUpperCase() + prop.slice(1)}: ${device.status[prop]}`; }
        statusList.appendChild(listItem);
    }
    let statusCell = row.insertCell();
    statusCell.appendChild(statusList);
    row.insertCell().textContent = device.battery_powered ? Math.round(device.battery_level / 2.55) + '%' : 'N/A';
    row.insertCell().textContent = device.uuid.join(':');
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

    if (elapsedTime < 5) {
        formattedTime = "< 5 s";
    } else if (elapsedTime < 60) {
        formattedTime = `${elapsedTime} s`;
    } else if (elapsedTime < 3600) {
        formattedTime = `${Math.floor(elapsedTime / 60)} m`;
    } else if (elapsedTime < 86400) { // 24 * 60 * 60 = 86400 seconds in a day
        formattedTime = `${Math.floor(elapsedTime / 3600)} h`;
    } else {
        formattedTime = `${Math.floor(elapsedTime / 86400)} d`;
    }
    return formattedTime;
}