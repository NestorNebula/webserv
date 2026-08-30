<?php
header('Content-Type: text/plain');

header('Content-Length: 10');

// RULE : cgi WITHOUT CONTENT LENGTH .. 
// SHOULD : buffer 
// and : FAIL if greater-than N
while (1)
{
	echo '42 ';
}
?>
