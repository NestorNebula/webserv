<?php
    header('Content-Type: video/x-matroska');

// bad file : PROVOKE run-on (timeout)
    $path = '/home/kdonlon/Videos/ultra.mp4';
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