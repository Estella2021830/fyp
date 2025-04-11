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

$sql = "CREATE TABLE IF NOT EXISTS slope_detection (
    id INT AUTO_INCREMENT PRIMARY KEY,
    pitch FLOAT,
    slope_detection VARCHAR(255),
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
)";

if ($conn->query($sql) === TRUE) {
    echo "Table slope_detection created successfully or already exists.<br>";
} else {
    echo "Error creating table: " . $conn->error . "<br>";
}

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['slopeData'])) {
    $slopeData = $_POST['slopeData'];
    echo "Received slopeData: $slopeData<br>";

    try {
        $parsedData = parseSLOPE($slopeData);
        $pitch = $parsedData['pitch'];
        $slope_state_code = $parsedData['slope_detection'];
        $slope_detection = '';

        switch ($slope_state_code) {
            case 0:
                $slope_detection = "Going up a slope";
                break;
            case 2:
                $slope_detection = "On level ground";
                break;
            case 1:
                $slope_detection = "Going down a slope";
                break;
            default:
                $slope_detection = "Unknown state";
                break;
        }

        echo "pitch: $pitch, slope_detection: $slope_detection<br />";
        insertData($conn, $pitch, $slope_detection);
    
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No slope data received.";
}

function insertData($conn, $pitch, $slope_detection) {
    $stmt = $conn->prepare("INSERT INTO slope_detection (pitch, slope_detection) VALUES (?, ?)");
    $stmt->bind_param("ds", $pitch, $slope_detection);

    if ($stmt->execute()) {
        echo "New record created successfully.<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $stmt->close();
}

function parseSLOPE($slopeINFO) {
    $parts = explode(",", $slopeINFO);
    var_dump($parts);
    $pit = floatval($parts[0]);
    $state = intval($parts[1]);

    return [
        'pitch' => $pit,
        'slope_detection' => $state
    ];
}

$conn->close();
?>
