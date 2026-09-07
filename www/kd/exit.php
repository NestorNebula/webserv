<?php
	header('Content-type: text/plain');
	header('Connection:close');

	$code = $_GET['code'] ?? 0;

	echo "PHP : will exit ($code)\n";
	exit(intval($code));
?>''