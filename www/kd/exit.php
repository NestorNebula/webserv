<?php
header('Content-type: text/plain');
header('Connection:close');
// header('Connection: keep-alive');
// header('Content-Length: 0');
	$x = -1;
	echo "PHP : will exit ($x)\n";
	exit($x);
	// die();
?>