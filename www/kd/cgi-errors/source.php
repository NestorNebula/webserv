<?php

    header('Content-Type: text/plain');

    $files = array('error.php', 'error.pl', 'error.py', 'infinite.php', 'timeout.php');
    foreach ($files as $path)
    {
        echo "**** FILE : $path ****\n\n";
        $fp = fopen($path, "rb");

        while (!feof($fp))
        {
            $data = fread($fp, 4096);
            echo $data;
        }
        fclose($fp);
        echo "\n\n";
    }
?>