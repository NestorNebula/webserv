<?php
	header('Content-type: text/plain');
	header('Connection:close');

	$code = $_POST['code'] ?? 0;

	if (php_sapi_name() == 'fpm-fcgi')
	{
		echo "PHP : exit does not mean the same thing in fastcgi\n";
	}
	else
	{
		echo "PHP : will exit ($code)\n";
	}
	exit(intval($code));

	echo "you should not be here\n";
?>