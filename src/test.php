<?php

    // var_dump($_ENV);
    // echo (get_env('QUERY_STRING'));

    // $p1 = $_GET['p1'];
    // $p2 = $_GET['p2'];

    // php-cgi IS DIFFERENT
    // SERVER : needs to add headers-before ... 
    // presumably CONTENT LENGTH
    // after it has read everything into a buffer 

    print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nPHP : hello, world!\n");  

    // print($p1);
    // print($p2);
?>