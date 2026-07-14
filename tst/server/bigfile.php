<?php

    // header('Content-Type: audio/mp3');
    // $fp = fopen('files/Kanan.mp3', "rb");

    // header('Content-Type: image/jpg');
    // $fp = fopen('files/2k_earth_daymap.jpg', "rb");

    header('Content-Type: video/mkv');
    $fp = fopen('files/Black.Mirror.S07E04.mkv', "rb");

    // one extra byte
    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);

?>