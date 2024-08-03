# echo-server.py

import socket

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 8080 # Port to listen on (non-privileged ports are > 1023)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")
        conn.sendall("white".encode("utf-8"))
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print("The move was" + data.decode("utf-8", errors='replace'))
            move = input("Your move")
            conn.sendall(move.encode("utf-8"))
