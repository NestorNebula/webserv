<?php

header('Content-type: text/plain');
header('Connection: close');
header("Cache-Control: 0");

header('Content-Length: 3');

// siege -- gets bad bytes value .. 
// when content-length NOT SET
// wait_comp .. not 100% up to snuff (?)
?>
PHP is not dead (yet)