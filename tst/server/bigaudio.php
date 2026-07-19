<?php
    header('Content-Type: audio/mp3');
    $fp = fopen('files/Kanan.mp3', "rb");

    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>