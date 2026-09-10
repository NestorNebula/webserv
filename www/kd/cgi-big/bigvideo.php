<?php
    header('Content-Type: video/x-matroska');

    $path = '../files/ultra.mp4';
    $fsiz = filesize($path);
    header('Content-Length: ' . $fsiz);

    $fp = fopen($path, "rb");

    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>