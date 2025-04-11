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

$sql = "CREATE TABLE IF NOT EXISTS fall_detection (
    id INT AUTO_INCREMENT PRIMARY KEY,
    TiltAngle FLOAT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)";

if ($conn->query($sql) === TRUE) {
    echo "Table falldetection created successfully or already exists.<br>";
} else {
    echo "Error creating table: " . $conn->error . "<br>";
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['fallData'])) {
    $fallData = $_POST['fallData'];
    echo "Received fallData: $fallData<br>";

    try {
        $parsedData = parseFALL($fallData);
        $tilt = $parsedData['Tilt'];
        echo "Tilt angle: $tilt<br />";
        insertData($conn, $tilt);
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No fall data received.";
}

function insertData($conn, $tilt) {
    $stmt = $conn->prepare("INSERT INTO fall_detection (TiltAngle) VALUES (?)");
    $stmt->bind_param("d", $tilt);

    if ($stmt->execute()) {
        echo "New record created successfully.<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $stmt->close();
}

function parseFALL($fallINFO) {
    $parts = explode(",", $fallINFO);
    var_dump($parts);
    $tilt = floatval($parts[0]);

    return [
        'Tilt' => $tilt,
    ];
}

$conn->close();
?>
