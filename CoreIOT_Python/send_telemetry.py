"""Bài 1 - Gửi telemetry giả lập (nhiệt độ, độ ẩm) lên CoreIOT qua MQTT.

CoreIOT (nền ThingsBoard):
    Host     : app.coreiot.io, port 1883
    Username : ACCESS_TOKEN của device (password để trống)
    Topic    : v1/devices/me/telemetry
    Payload  : {"temperature": 30.5, "humidity": 65}

Cách chạy (PowerShell):
    pip install -r requirements.txt
    $env:COREIOT_ACCESS_TOKEN = "ACCESS_TOKEN"
    python send_telemetry.py                  # gửi mỗi 5 s, Ctrl+C để dừng
    python send_telemetry.py --count 10 --interval 2
"""

import argparse
import json
import os
import random
import sys
import time

import paho.mqtt.client as mqtt

TELEMETRY_TOPIC = "v1/devices/me/telemetry"


def parse_args():
    p = argparse.ArgumentParser(description="Send simulated telemetry to CoreIOT via MQTT")
    p.add_argument("--host", default="app.coreiot.io", help="MQTT broker (default: app.coreiot.io)")
    p.add_argument("--port", type=int, default=1883, help="MQTT port (default: 1883)")
    p.add_argument("--token", default=os.environ.get("COREIOT_ACCESS_TOKEN"),
                   help="Device access token (default: env COREIOT_ACCESS_TOKEN)")
    p.add_argument("--interval", type=float, default=5.0, help="Seconds between messages (default: 5)")
    p.add_argument("--count", type=int, default=0, help="Number of messages, 0 = run forever")
    return p.parse_args()


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[MQTT] Connected to {userdata['host']}:{userdata['port']}")
    else:
        # "Not authorized" / "Bad user name or password" => sai access token
        print(f"[MQTT] Connect failed: {reason_code}")


def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"[MQTT] Disconnected: {reason_code}")


class SensorSimulator:
    """Sinh dữ liệu giả lập thay đổi từ từ (random walk), giống cảm biến thật."""

    def __init__(self):
        self.temperature = 28.0
        self.humidity = 65.0

    def read(self):
        self.temperature = min(max(self.temperature + random.uniform(-0.5, 0.5), 20.0), 40.0)
        self.humidity = min(max(self.humidity + random.uniform(-2.0, 2.0), 30.0), 95.0)
        return round(self.temperature, 1), round(self.humidity, 1)


def main():
    args = parse_args()
    if not args.token:
        sys.exit("Missing access token: set env COREIOT_ACCESS_TOKEN or pass --token")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,
                         userdata={"host": args.host, "port": args.port})
    client.username_pw_set(args.token)          # CoreIOT: username = access token
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    client.connect(args.host, args.port, keepalive=60)
    client.loop_start()                         # luồng nền xử lý mạng MQTT

    sensor = SensorSimulator()
    sent = 0
    try:
        while args.count == 0 or sent < args.count:
            temperature, humidity = sensor.read()
            payload = json.dumps({"temperature": temperature, "humidity": humidity})

            info = client.publish(TELEMETRY_TOPIC, payload, qos=1)
            info.wait_for_publish(timeout=10)
            sent += 1
            status = "OK" if info.is_published() else "FAILED"
            print(f"[{time.strftime('%H:%M:%S')}] #{sent} publish {TELEMETRY_TOPIC} {payload} -> {status}")

            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopped by user")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
