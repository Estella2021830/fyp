<?php


$servername = "localhost";
$username = "root";
$password = "";
$dbname = "gpswh1_db";
$port = 3307;

$conn = new mysqli($servername, $username, $password, $dbname, $port);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "CREATE TABLE IF NOT EXISTS gps_sos (
    id INT AUTO_INCREMENT PRIMARY KEY,
    gps VARCHAR(255),
    latitude DOUBLE,
    longitude DOUBLE,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)";

if ($conn->query($sql) === TRUE) {
    echo "Table gps_sos created successfully or already exists.<br />";
} else {
    echo "Error creating table: " . $conn->error;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['gps_data'])) {
    $gpsData = $_POST['gps_data'];

    try {
        $parsedData = parseCGPSINFO($gpsData);
        $latitude = $parsedData['latitude'];
        $longitude = $parsedData['longitude'];

        echo "Parsed Latitude: $latitude, Longitude: $longitude<br />";

        $stmt = $conn->prepare("INSERT INTO gps_sos (`gps`, `latitude`, `longitude`) VALUES (?, ?, ?)");
        if ($stmt === false) {
            die("Prepare failed: " . $conn->error);
        }

        $stmt->bind_param("sdd", $gpsData, $latitude, $longitude);

        if ($stmt->execute()) {
            echo "New record created successfully.<br />";
        } else {
            echo "Error executing statement: " . $stmt->error;
        }

        $stmt->close();
        
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No GPS data received.";
}

function parseCGPSINFO($gpsinfo) {
    $parts = explode(",", $gpsinfo);
    var_dump($parts);

    if (count($parts) < 4) {
        throw new Exception("Invalid GPS data format");
    }

    $lat = convertToDegrees($parts[0], $parts[1]);
    $lon = convertToDegrees($parts[2], $parts[3], true);

    return [
        'latitude' => $lat,
        'longitude' => $lon
    ];
}

function convertToDegrees($value, $direction, $isLongitude = false) {
    $degreeLength = $isLongitude ? 3 : 2;
    $degrees = (int) substr($value, 0, $degreeLength);
    $minutes = (float) substr($value, $degreeLength);
    $decimal = $degrees + ($minutes / 60);

    if ($direction === "S" || $direction === "W") {
        $decimal = -$decimal;
    }

    return $decimal;
}
?>
