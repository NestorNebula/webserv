<?php
    define('WSEOL', "<br>");
    header('Content-type: text/html');

    // print('<html>');
    if (isset($_FILES['file']))
    {
        $file_name = $_FILES['file']['name'];
        $file_path = "./upload-php-" . $file_name;
        if (file_exists($file_path))
        {
            header('Status: 409');
            print("PHP  : upload" . WSEOL);
            print("PHP  : file exists" . WSEOL);
            print("<a href='javascript:history.back();'>BACK</a>" . WSEOL);
            return (0);
        }

        print("PHP  : upload" . WSEOL);
        echo ("file : " . $file_name) . WSEOL;
        echo ("path : " . $file_path) . WSEOL;
        switch ($_FILES['file']['error']) {
            case UPLOAD_ERR_OK:
                // echo "Success" . WSEOL
                break;
            case UPLOAD_ERR_NO_FILE:
                echo "No file" . WSEOL;
                break;
            case UPLOAD_ERR_INI_SIZE:
            case UPLOAD_ERR_FORM_SIZE:
                echo "Exceeded filesize limit" . WSEOL;
                break;
            case UPLOAD_ERR_NO_TMP_DIR:
                echo "No tmp dirrectory" . WSEOL;
                break;
            default:
                echo "Unknown error " . $_FILES['file']['error'];
                break;
        }
        move_uploaded_file($_FILES['file']['tmp_name'], $file_path);
    }
    else
    {
        print("PHP : upload" . WSEOL);
        print("PHP : no file set" . WSEOL);
    }
    print("<a href='javascript:history.back();'>BACK</a>" . WSEOL);
?>
