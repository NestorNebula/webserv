<?php

$code = $_POST['code'] ?? 200;

header('Status: ' . $code);
header('Content-Type: text/plain');

echo "PHP Status: $code\n";

?>
