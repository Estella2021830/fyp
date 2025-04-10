<?php
// Enable error reporting
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Database credentials
$servername = "localhost";
$username = "root";
$password = ""; // Your password
$dbname = "gpswh1_db"; // Your intended database name
$port = 3307; // Update the port number if needed

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname, $port);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Create table if it doesn't exist
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

// Get data from the POST request
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['fallData'])) {
    // Retrieve tilt angle from the POST request
    $fallData = $_POST['fallData'];
    echo "Received fallData: $fallData<br>"; // Debugging output

    // Parse fall data
    try {
        $parsedData = parseFALL($fallData);
        $tilt = $parsedData['Tilt'];
        
        // Output parsed values
        echo "Tilt angle: $tilt<br />";

        // Insert data into the database
        insertData($conn, $tilt);
    } catch (Exception $e) {
        echo "Error: " . $e->getMessage();
    }
} else {
    echo "No fall data received.";    
}

// Function to insert data into the database
function insertData($conn, $tilt) {
    // Prepare the insert statement with the correct number of placeholders
    $stmt = $conn->prepare("INSERT INTO fall_detection (TiltAngle) VALUES (?)");
    
    // Bind two parameters
    $stmt->bind_param("d", $tilt); 

    if ($stmt->execute()) {
        echo "New record created successfully.<br>";
    } else {
        echo "Error: " . $stmt->error . "<br>";
    }

    $stmt->close();
}

// Function to parse fall data
function parseFALL($fallINFO) {
    // Split the string by commas
    $parts = explode(",", $fallINFO);
    
    // Debugging: output the parsed parts
    var_dump($parts); 
    $tilt = floatval($parts[0]); // Ensure tilt is a float

    return [
        'Tilt' => $tilt, // Changed from semicolon to comma
    ];
}

// Close connection
$conn->close();
?>