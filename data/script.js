const main = document.querySelector("main");

function showLoader(message) {
  main.innerHTML = `<div class="loader"></div><p>${message}</p>`;
}

function showWiFiForm() {
  main.innerHTML = `
    <div class="card" style="max-width:400px;margin:32px auto 0 auto;">
      <form id="wifiForm">
        <label for="ssid">Wi-Fi síť:</label>
        <select id="ssid" name="ssid"><option disabled selected>Skenování...</option></select>
        <label for="password">Heslo:</label>
        <input type="password" id="password" name="password" placeholder="Zadejte heslo" required>
        <button type="submit">Připojit</button>
      </form>
      <div id="status"></div>
      <div id="sensor-values" style="text-align:center; margin: 24px 0;">
        <div style="font-size:1.5em;">Teplota: <span id="temp-value">--</span> °C</div>
        <div style="font-size:1.5em;">Vlhkost: <span id="hum-value">--</span> %</div>
      </div>
    </div>
  `;

  const ssidSelect = document.getElementById("ssid");

  fetch("/scan")
    .then((res) => res.json())
    .then((data) => {
      ssidSelect.innerHTML = "";
      data.forEach((ssid) => {
        const opt = document.createElement("option");
        opt.value = ssid;
        opt.textContent = ssid;
        ssidSelect.appendChild(opt);
      });
    });

  document.getElementById("wifiForm").addEventListener("submit", function (e) {
    e.preventDefault();
    const ssid = ssidSelect.value;
    const password = document.getElementById("password").value;
    connectToWiFi(ssid, password);
  });
}

function connectToWiFi(ssid, password) {
  showLoader("Připojování k Wi-Fi...");
  fetch(
    `/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(
      password
    )}`
  )
    .then((res) => res.json())
    .then((data) => {
      if (data.status === "connected") {
        showConnectedScreen(data);
      } else {
        main.innerHTML = `<p>❌ Nepodařilo se připojit</p><button id="retryBtn">Zkusit znovu</button>`;
        document
          .getElementById("retryBtn")
          .addEventListener("click", showWiFiForm);
      }
    })
    .catch(() => {
      main.innerHTML = `<p>❌ Chyba připojení</p><button id="retryBtn">Zkusit znovu</button>`;
      document
        .getElementById("retryBtn")
        .addEventListener("click", showWiFiForm);
    });
}

function showConnectedScreen(data) {
  main.innerHTML = `
    <div class="card" style="max-width:400px;margin:32px auto 0 auto;">
      <p><strong>Připojeno k:</strong> ${data.ssid}</p>
      <p><strong>IP adresa:</strong> ${data.ip}</p>
      <p><strong>Brána:</strong> ${data.gateway}</p>
      <p><strong>DNS:</strong> ${data.dns}</p>
      <button id="pingBtn">ingestDevState</button>
      <button id="reconnectBtn">Změnit Wi-Fi</button>
      <div id="pingResult"></div>
      <div id="sensor-values" style="text-align:center; margin: 24px 0;">
        <div style="font-size:1.5em;">Teplota: <span id="temp-value">--</span> °C</div>
        <div style="font-size:1.5em;">Vlhkost: <span id="hum-value">--</span> %</div>
      </div>
    </div>
  `;

  document.getElementById("pingBtn").addEventListener("click", () => {
    const pingResult = document.getElementById("pingResult");
    pingResult.innerHTML = `<div class="loader"></div><p>Sending...</p>`;
    fetch("/ping")
      .then((res) => res.text())
      .then((result) => (pingResult.textContent = result))
      .catch(() => {
        pingResult.textContent = "❌ Chyba pingu";
      });
  });

  document.getElementById("reconnectBtn").addEventListener("click", () => {
    fetch("/reconnect").then(() => showWiFiForm());
  });
}

// On page load

fetch("/status")
  .then((res) => res.json())
  .then((data) => {
    if (data.status === "connected") {
      showConnectedScreen(data);
    } else {
      showWiFiForm();
    }
  });

function updateSensorValues() {
  fetch("/sensor")
    .then((res) => res.json())
    .then((data) => {
      document.getElementById("temp-value").textContent = data.temperature;
      document.getElementById("hum-value").textContent = data.humidity;
    })
    .catch(() => {
      document.getElementById("temp-value").textContent = "--";
      document.getElementById("hum-value").textContent = "--";
    });
}

function startSensorUpdater() {
  updateSensorValues();
  setInterval(updateSensorValues, 3000);
}

// Start sensor updater after rendering card
document.addEventListener("DOMContentLoaded", startSensorUpdater);
