<?php
    header('Content-Type: audio/mp3');

    $path = 'files/Kanan.mp3';
    $fsiz = filesize($path);
    header('Content-Length: ' . $fsiz);

    $fp = fopen($path, "rb");

    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>