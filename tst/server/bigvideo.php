<?php
    header('Content-Type: video/x-matroska');

    $path = 'files/Black.Mirror.S07E04.mkv';
    $fsiz = filesize($path);
    header('Content-Length: ' . $fsiz);

    $fp = fopen($path, "rb");

    // one extra byte
    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>