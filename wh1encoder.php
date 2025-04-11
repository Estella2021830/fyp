<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

$servername = "localhost";
$username = "root";
$password = "";
$dbname = "gpswh1_db";
$port = 3307;

$conn = new mysqli($servername, $username, $password, $dbname, $port);

if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$sql = "CREATE TABLE IF NOT EXISTS encoder (
    id INT AUTO_INCREMENT PRIMARY KEY,
    distance FLOAT,
    speed FLOAT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)";

if ($conn->query($sql) === TRUE) {
    echo "Table encoder created successfully or already exists.<br>";
} else {
    echo "Error creating table: " . $conn->error . "<br>";
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['encoderData'])) {
    $encoderData = $_POST['encoderData'];
    echo "Received encoderData: $encoderData<br>";

    try {
        $parsedData = parseENCODER($encoderData);
        $distance = $parsedData['distance'];
        $speed = $parsedData['speed'];

        echo "distance: $distance, distance: $speed<br />";
        insertData($conn, $distance, $speed);
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No encoder data received.";
}

function insertData($conn, $distance, $speed) {
    $stmt = $conn->prepare("INSERT INTO encoder (distance, speed) VALUES (?, ?)");
    $stmt->bind_param("dd", $distance, $speed);

    if ($stmt->execute()) {
        echo "New record created successfully.<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $stmt->close();
}

function parseENCODER($encoderINFO) {
    $parts = explode(",", $encoderINFO);
    var_dump($parts);
    $dis = floatval($parts[0]);
    $pspeed = floatval($parts[1]);

    return [
        'distance' => $dis,
        'speed' => $pspeed
    ];
}

$conn->close();
?>
