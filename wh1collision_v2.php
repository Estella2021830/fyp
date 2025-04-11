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

$sql = "CREATE TABLE IF NOT EXISTS collision (
    id INT AUTO_INCREMENT PRIMARY KEY,
    previous_acceleration FLOAT,
    current_acceleration FLOAT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)";

if ($conn->query($sql) === TRUE) {
    echo "Table collision created successfully or already exists.<br>";
} else {
    echo "Error creating table: " . $conn->error . "<br>";
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['collisionData'])) {
    $collisionData = $_POST['collisionData'];
    echo "Received collisionData: $collisionData<br>";

    try {
        $parsedData = parseCOLLISION($collisionData);
        $current_acceleration = $parsedData['currentMagnitude'];
        $previous_acceleration = $parsedData['previousMagnitude'];

        echo "previous_acceleration: $previous_acceleration, current_acceleration: $current_acceleration<br />";
        insertData($conn, $previous_acceleration, $current_acceleration);
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No collision data received.";
}

function insertData($conn, $previous_acceleration, $current_acceleration) {
    $stmt = $conn->prepare("INSERT INTO collision (previous_acceleration, current_acceleration) VALUES (?, ?)");
    $stmt->bind_param("dd", $previous_acceleration, $current_acceleration);

    if ($stmt->execute()) {
        echo "New record created successfully.<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $stmt->close();
}

function parseCOLLISION($collisionINFO) {
    $parts = explode(",", $collisionINFO);
    var_dump($parts);
    $currentACC = floatval($parts[0]);
    $previousACC = floatval($parts[1]);

    return [
        'currentMagnitude' => $currentACC,
        'previousMagnitude' => $previousACC
    ];
}

$conn->close();
?>
