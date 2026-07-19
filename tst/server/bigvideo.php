<?php
    header('Content-Type: video/x-matroska');
    $fp = fopen('files/Black.Mirror.S07E04.mkv', "rb");

    // one extra byte
    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>