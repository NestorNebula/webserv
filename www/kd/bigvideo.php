<?php
    header('Content-Type: video/x-matroska');

// bad file : PROVOKE run-on (timeout)
    $path = '/home/kdonlon/Videos/dff.mp4';
    $fsiz = filesize($path);
    // WOW
    // no content-length => no memory problems (siege)
    // FCGI
    // header('Content-Length: ' . $fsiz);

    $fp = fopen($path, "rb");

    while (!feof($fp))
    {
        $data = fread($fp, 4096);
        echo $data;
    }
    fclose($fp);
?>