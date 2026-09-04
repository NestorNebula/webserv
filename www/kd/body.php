<?php

header('content-type:text/plain');
foreach (getallheaders() as $name => $value) {
    print "$name: $value\r\n";
}

$body = file_get_contents("php://input");
echo "BODY[$body]\r\n";

echo "buddy";
?>