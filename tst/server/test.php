<?php

    // print_r($_GET);
    // print_r($_POST);
    // print_r($_SERVER); // ENV shows up here
    // exit (0);

    header('Content-type: text/plain');
    
    print("PHP : hello, world!\n");  

    $g1 = $_GET['g1'];
    $g2 = $_GET['g2'];

    print("\nGET VARS\n");
    print("g1 : " . $g1 . PHP_EOL);
    print("g2 : " . $g2 . PHP_EOL);

    print("\nPOST VARS\n");
    $p1 = $_POST['p1'];
    $p2 = $_POST['p2'];

    print("p1 : " . $p1 . PHP_EOL);
    print("p2 : " . $p2 . PHP_EOL);


    print("\nENV\n");
    foreach  ($_SERVER as $k => $v)
        print ("$k = $v\n");


    $chk_hed = 'REMOTE_ADDR';
    print("\n$chk_hed : " . $_SERVER[$chk_hed] . PHP_EOL);

    // $f = $_POST['file']; // undefined array key name="file" filename=""
    // print("file:\n" . $f . PHP_EOL);
?>