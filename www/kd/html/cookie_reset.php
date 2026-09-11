<?php
    define('WSEOL', PHP_EOL);

    header('Content-type: text/plain');

    $exp = time() - 60;
    setcookie("wstimecookie", 0, $exp, "/");
    setcookie("counter", 0, $exp, "/");

    if (isset($_SERVER['HTTP_REFERER']))
    {
        header('Location: ' . $_SERVER['HTTP_REFERER']);
    }
    else
    {
        echo "HTTP_REFERER : " . $_SERVER['HTTP_REFERER'] . WSEOL;
        echo "HTTP_COOKIE : " . $_SERVER['HTTP_COOKIE'] . WSEOL;
    }
// $_SERVER['$_SERVER['HTTP_COOKIE']']
?>