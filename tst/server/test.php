<?php

    header('Content-type: text/plain');

    // print("\r\n");
    // print_r($_GET);
    // print_r($_POST);
    // print_r($_SERVER); // ENV shows up here


    print("PHP : hello, world!\n");  

    $g1 = $_GET['g1'] ?? 'g1-default';
    $g2 = $_GET['g2'] ?? 'g2-default';

    print("\nGET VARS\n");
    print("g1 : " . $g1 . PHP_EOL);
    print("g2 : " . $g2 . PHP_EOL);

    print("\nPOST VARS\n");
    $p1 = $_POST['p1'] ?? 'p1-default';
    $p2 = $_POST['p2'] ?? 'p2-default';

    print("p1 : " . $p1 . PHP_EOL);
    print("p2 : " . $p2 . PHP_EOL);


    print("\nENV\n\n");
    foreach  ($_SERVER as $k => $v)
        print ("$k = $v\n");

// HTTP/1.1 200 OK
// Status: 500 Internal Server Error
// X-Powered-By: PHP/8.4.21
// Content-type: text/html; charset=UTF-8

// HTTP/1.1 808 err description

// Error Data

 
    $chk_hed = 'REMOTE_ADDR';
    print("\n$chk_hed : " . $_SERVER[$chk_hed] . PHP_EOL);

// PHP Warning:  PHP Request Startup: POST Content-Length of 14976173 bytes exceeds the limit of 8388608 bytes in Unknown on line 0
// PHP Warning:  Undefined array key "p1" in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 20
// PHP Warning:  Undefined array key "p2" in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 21
// PHP Warning:  Undefined array key "file" in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 35
// PHP Warning:  Undefined array key "file" in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 36
// PHP Warning:  Trying to access array offset on null in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 36
// PHP Warning:  Undefined array key "file" in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 36
// PHP Warning:  Trying to access array offset on null in /home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php on line 36


// POST Content-Length of 14976173 bytes exceeds the limit of 8388608 bytes in Unknown on line 0

    print_r($_FILES['file']); // Array
    move_uploaded_file($_FILES['file']['tmp_name'], "./uploads/" . $_FILES['file']['name']);
?>