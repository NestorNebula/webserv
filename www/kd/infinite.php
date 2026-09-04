<?php
header('Content-Type: text/plain');

header('Content-Length: 10');

// RULE : cgi WITHOUT CONTENT LENGTH .. 
// SHOULD : buffer 
// and : FAIL if greater-than N

// with content length : SHOULD BE RESPECTED
// so .. we ARE going to have to look at WAIT_COMP
// so .. server .. should cut it off at content-length

// REAL : SHOULD : always buffer-until-done
// then .. check exit code 
// 
while (1)
{
	echo '42 ';
}
?>
