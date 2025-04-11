import json
import mysql.connector
from flask import Flask, request, jsonify, render_template, make_response
import numpy as np
from scipy.optimize import least_squares
import threading
import requests

app = Flask(__name__, template_folder="templates")

DB_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "",
    "database": "indoor_tracking"
}

def connect_db():
    return mysql.connector.connect(**DB_CONFIG)

latest_position = None

BEACON_POSITIONS = {
    "beacon1": (0, 0),
    "beacon2": (4.15, 0),
    "beacon3": (0, 5.35),
    "beacon4": (4.15, 5.35)
}

MAP_BOUNDS = {
    "x_min": 0,
    "x_max": 4.15,
    "y_min": 0,
    "y_max": 5.35
}

def is_within_bounds(x, y):
    return MAP_BOUNDS["x_min"] <= x <= MAP_BOUNDS["x_max"] and MAP_BOUNDS["y_min"] <= y <= MAP_BOUNDS["y_max"]

def loss_function(X, beacons, distances):
    x, y = X
    residuals = []
    for beacon, (bx, by) in beacons.items():
        if beacon in distances:
            r = distances[beacon]
            residuals.append((np.sqrt((x - bx)**2 + (y - by)**2) - r)**2)
    return residuals

def triangulate(distances):
    try:
        if len(distances) < 3:
            print("Not enough beacons for triangulation.")
            return None, None

        print(f"Triangulating: {distances}")
        x0 = np.mean([pos[0] for pos in BEACON_POSITIONS.values()])
        y0 = np.mean([pos[1] for pos in BEACON_POSITIONS.values()])
        result = least_squares(loss_function, x0=[x0, y0], args=(BEACON_POSITIONS, distances))
        x, y = result.x
        return round(x, 2), round(y, 2)
    except Exception as e:
        print(f"Triangulation error: {e}")
        return None, None

def save_to_db(x, y):
    try:
        db = connect_db()
        cursor = db.cursor()
        cursor.execute("INSERT INTO positions (x_coord, y_coord) VALUES (%s, %s)", (x, y))
        db.commit()
        cursor.close()
        db.close()
        print(f"Saved to DB: X = {x}, Y = {y}")
    except Exception as e:
        print(f"Database error: {e}")

@app.route("/")
def homepage():
    return render_template("homepage.html")

@app.route("/indoor")
def indoor_page():
    return render_template("indoor.html")

@app.route("/outdoor")
def outdoor_page():
    return render_template("outdoor.html")

@app.route("/upload_data", methods=["POST"])
def upload_data():
    global latest_position

    try:
        payload = request.get_json()
        if not payload or "distances" not in payload:
            return jsonify({"error": "Missing distances"}), 400

        distances = payload["distances"]
        if len(distances) < 3:
            return jsonify({"error": "At least 3 beacons required"}), 400

        x, y = triangulate(distances)
        if x is not None and y is not None:
            if is_within_bounds(x, y):
                latest_position = {"x": x, "y": y}
                threading.Thread(target=save_to_db, args=(x, y)).start()
            else:
                latest_position = None
                print(f"Out of bounds: {x}, {y}")

        return jsonify({"status": "Received", "x": x, "y": y}), 200

    except Exception as e:
        print(f"Upload error: {e}")
        return jsonify({"error": str(e)}), 500

@app.route("/latest_position", methods=["GET"])
def get_latest_position():
    if latest_position:
        response = make_response(jsonify(latest_position))
        response.headers["Cache-Control"] = "no-store"
        return response
    return jsonify({"error": "No data available"})

@app.route("/sensor_data", methods=["GET"])
def get_sensor_data():
    try:
        base = "http://localhost/php_script"

        fall = requests.get(f"{base}/wh1_get_fall.php").json()
        slope = requests.get(f"{base}/wh1_get_slope.php").json()
        collision = requests.get(f"{base}/wh1_get_collision.php").json()
        encoder = requests.get(f"{base}/wh1_get_encoder.php").json()

        return jsonify({
            "fall": fall,
            "slope": slope,
            "collision": collision,
            "encoder": encoder
        })

    except Exception as e:
        print("Error fetching sensor data:", e)
        return jsonify({"error": "Failed to fetch sensor data"}), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
