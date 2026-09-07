<?php

$code = $_GET['code'] ?? 200;

header('Status: ' . $code);
header('Content-Type: text/plain');
// header('Content-Length: 10');

echo "Status: $code\n";

?>
