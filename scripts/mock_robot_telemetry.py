#!/usr/bin/env python3
"""
ZenithOS Robotics & Telemetry Mock Server
Streams live #Z1 sensor packets to QEMU's bridged serial port (default: 127.0.0.1:4444)
and responds to interactive commands sent from Zenith Device Studio.

Usage:
    python3 scripts/mock_robot_telemetry.py [--host 127.0.0.1] [--port 4444] [--rate 5]
"""

import socket
import time
import math
import argparse
import sys
import select

def main():
    parser = argparse.ArgumentParser(description="ZenithOS Mock Robot Telemetry Streamer")
    parser.add_argument("--host", default="127.0.0.1", help="Target QEMU serial bridge host")
    parser.add_argument("--port", type=int, default=4444, help="Target QEMU serial bridge port")
    parser.add_argument("--rate", type=float, default=4.0, help="Packets per second (Hz)")
    args = parser.parse_args()

    print(f"[*] Connecting to ZenithOS COM1 serial socket at {args.host}:{args.port}...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((args.host, args.port))
    except ConnectionRefusedError:
        print(f"[!] Error: Unable to connect to {args.host}:{args.port}.")
        print("    Make sure ZenithOS is running in QEMU with serial bridge:")
        print("    Run: make run-serial-bios")
        sys.exit(1)

    print("[+] Connected to ZenithOS COM1 UART!")
    print("[*] Streaming #Z1 telemetry packets and listening for commands...")
    print("    Press Ctrl+C to exit.\n")

    # Send initial boot banner
    banner = "[ROBOT_CORE] STM32F405 Flight & Robotics Node Online (Firmware v2.4.1)\r\n"
    s.sendall(banner.encode("ascii"))
    time.sleep(0.1)

    seq = 1
    t0 = time.time()
    dt = 1.0 / max(args.rate, 0.5)

    recv_buffer = ""

    try:
        while True:
            # Check for incoming commands from Device Studio
            rlist, _, _ = select.select([s], [], [], 0.0)
            if rlist:
                data = s.recv(1024)
                if not data:
                    print("[!] Serial socket closed by host.")
                    break
                recv_buffer += data.decode("latin1", errors="ignore")
                while "\n" in recv_buffer or "\r" in recv_buffer:
                    line, _, remainder = recv_buffer.partition("\n")
                    if "\r" in line:
                        line = line.split("\r")[0]
                    recv_buffer = remainder
                    line = line.strip()
                    if line:
                        print(f"<- [DEV STUDIO COMMAND] '{line}'")
                        # Handle interactive commands
                        cmd_lower = line.lower()
                        if "ping" in cmd_lower:
                            resp = f"[ROBOT_CORE] PONG (seq={seq}, uptime={int(time.time() - t0)}s)\r\n"
                            s.sendall(resp.encode("ascii"))
                        elif "status" in cmd_lower:
                            resp = f"[ROBOT_CORE] STATUS: IMU=OK, BAT=12.2V, MOTORS=STANDBY, TEMP=29C\r\n"
                            s.sendall(resp.encode("ascii"))
                        elif "calib" in cmd_lower:
                            resp = "[ROBOT_CORE] CALIBRATING IMU GYRO/ACCEL... ZERO BIAS COMPLETE (0.02 deg/s)\r\n"
                            s.sendall(resp.encode("ascii"))
                        else:
                            resp = f"[ROBOT_CORE] ACK: '{line}' received\r\n"
                            s.sendall(resp.encode("ascii"))

            # Calculate simulated flight attitude
            now = time.time() - t0
            roll = int(22.0 * math.sin(now * 1.2))
            pitch = int(14.0 * math.cos(now * 0.9))
            yaw = int((now * 25.0) % 360)
            volt_mv = int(12450 - ((seq * 5) % 1500)) # 12.45V dropping slowly
            temp_c = int(28 + 3 * math.sin(now * 0.2))
            status = "ARMED" if (seq % 40 > 10) else "OK"

            packet = f"#Z1,{seq},{roll},{pitch},{yaw},{volt_mv},{temp_c},{status}\r\n"
            s.sendall(packet.encode("ascii"))
            print(f"-> {packet.strip()}", end="\r")

            seq += 1
            time.sleep(dt)

    except KeyboardInterrupt:
        print("\n[*] Stopping mock telemetry streamer.")
    finally:
        s.close()

if __name__ == "__main__":
    main()
