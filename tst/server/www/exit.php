<?php

header('Content-type: text/plain');
// header('Connection: keep-alive');
// error : should still have added keep-alive
// header('Content-Length: 0');

// fuck : SOMETIMES (200) SOMETIMES (404)
// the (404) definitely CONFUSED THINGS ..
// seems to be where the HANG begins ...
echo "shit";

// but .. we got data and started sending ..  .. before cgi is FINISHED ... 
// which .. maybe .. we shouldn't ..

	exit(3);
?>