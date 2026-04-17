from flask import Flask, jsonify, request, render_template_string
import serial
import threading
import time

SERIAL_PORT = "/dev/cu.usbserial-110"  # change if needed
BAUD_RATE = 115200

app = Flask(__name__)

latest_status = "Active"
latest_log = []
ser = None


HTML = """
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>Care Monitor Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f7f7f7;
      text-align: center;
      padding: 40px;
    }
    .card {
      max-width: 650px;
      margin: auto;
      background: white;
      border-radius: 16px;
      padding: 30px;
      box-shadow: 0 4px 14px rgba(0,0,0,0.08);
    }
    .status {
      font-size: 2rem;
      font-weight: bold;
      margin: 20px 0;
      padding: 20px;
      border-radius: 12px;
    }
    .ACTIVE { background: #dff7df; color: #1f7a1f; }
    .WARNING { background: #fff4cc; color: #9a6b00; }
    .ALERT { background: #ffd9d9; color: #b00020; }
    button {
      padding: 12px 20px;
      font-size: 1rem;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      background: #222;
      color: white;
      margin-top: 15px;
    }
    .log {
      text-align: left;
      margin-top: 25px;
      background: #fafafa;
      border-radius: 10px;
      padding: 15px;
      height: 240px;
      overflow-y: auto;
      font-family: monospace;
      font-size: 0.95rem;
      white-space: pre-wrap;
    }
    .FALL {
        background: #ffcccc;
        color: #8b0000;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>Care Monitor Dashboard</h1>
    <div id="statusBox" class="status">Connecting...</div>
    <button onclick="resetCaregiver()">Caregiver Reset</button>
    <div class="log" id="logBox"></div>
  </div>

  <script>
    async function fetchStatus() {
      const res = await fetch('/status');
      const data = await res.json();

      const statusBox = document.getElementById('statusBox');
      statusBox.textContent = data.status;
      statusBox.className = 'status ' + data.level;

      document.getElementById('logBox').textContent = data.log.join("\\n");
    }

    async function resetCaregiver() {
      await fetch('/reset', { method: 'POST' });
    }

    setInterval(fetchStatus, 1000);
    fetchStatus();
  </script>
</body>
</html>
"""


def set_status_from_line(line: str):
    global latest_status, latest_log

    latest_log.append(line)
    latest_log = latest_log[-20:]

    upper = line.upper()

    if "FALL" in upper:
        latest_status = "FALL"
    elif "ALERT" in upper:
        latest_status = "ALERT"
    elif "WARNING" in upper:
        latest_status = "WARNING"
    elif "ACTIVE" in upper:
        latest_status = "ACTIVE"

def serial_reader():
    global ser, latest_status, latest_log
    while True:
        try:
            if ser is None:
                ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
                time.sleep(2)

            line = ser.readline().decode(errors="ignore").strip()
            if line:
                print(line)
                set_status_from_line(line)

        except Exception as e:
            latest_status = "Disconnected"
            latest_log.append(f"Serial error: {e}")
            latest_log = latest_log[-20:]
            try:
                if ser:
                    ser.close()
            except Exception:
                pass
            ser = None
            time.sleep(2)


@app.route("/")
def index():
    return render_template_string(HTML)


@app.route("/status")
def status():
    if latest_status == "FALL":
        level = "ALERT"
    elif latest_status == "ALERT":
        level = "ALERT"
    elif latest_status == "WARNING":
        level = "WARNING"
    else:
        level = "ACTIVE"

    return jsonify({
        "status": latest_status,
        "level": level,
        "log": latest_log
    })

@app.route("/reset", methods=["POST"])
def reset():
    global ser
    try:
        if ser and ser.is_open:
            ser.write(b"r")
            latest_log.append("Reset sent from dashboard")
    except Exception as e:
        latest_log.append(f"Reset failed: {e}")
    return ("", 204)


if __name__ == "__main__":
    thread = threading.Thread(target=serial_reader, daemon=True)
    thread.start()
    app.run(debug=False, use_reloader=False)
