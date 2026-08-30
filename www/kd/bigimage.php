<?php

    $path = './files/e4.jpg';
    $fsiz = filesize($path);
    
    header('Content-Type: image/jpg');
    // header('Content-Length: ' . $fsiz);
// 177450

    $fp = fopen($path, "rb");

    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>