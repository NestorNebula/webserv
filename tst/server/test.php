<?php


    // var_dump($_GET);
    // var_dump($_POST);
    // var_dump($_SERVER); // ENV shows up here

    print("PHP : hello, world!\n");  

    $g1 = $_GET['g1'];
    $g2 = $_GET['g2'];

    print("g1 : " . $g1 . PHP_EOL);
    print("g2 : " . $g2 . PHP_EOL);

    $p1 = $_POST['p1'];
    $p2 = $_POST['p2'];

    print("p1 : " . $p1 . PHP_EOL);
    print("p2 : " . $p2 . PHP_EOL);

    // $f = $_POST['file']; // undefined array key name="file" filename=""
    // print("file:\n" . $f . PHP_EOL);
?>