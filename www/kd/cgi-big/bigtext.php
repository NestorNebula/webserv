<?php
    header('Content-Type: text/html');

    $path = '../files/asz.html';
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