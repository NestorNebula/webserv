<?php

    header('Content-type: text/plain');

    if (isset($_FILES['file']))
    {
        $file_name = $_FILES['file']['name'];
        $file_path = "./upload-php-" . $file_name;
        if (file_exists($file_path))
        {
            header('Status: 409');
            print("PHP  : upload\n\n");
            print("PHP  : file exists\n");
            return (0);
        }

        print("PHP  : upload\n\n");
        echo ("file : " . $file_name) . PHP_EOL;
        echo ("path : " . $file_path) . PHP_EOL;
        switch ($_FILES['file']['error']) {
            case UPLOAD_ERR_OK:
                // echo "Success\n";
                break;
            case UPLOAD_ERR_NO_FILE:
                echo "No file\n";
                break;
            case UPLOAD_ERR_INI_SIZE:
            case UPLOAD_ERR_FORM_SIZE:
                echo "Exceeded filesize limit\n";
                break;
            case UPLOAD_ERR_NO_TMP_DIR:
                echo "No tmp dirrectory\n";
                break;
            default:
                echo "Unknown error " . $_FILES['file']['error'];
                break;
        }
        move_uploaded_file($_FILES['file']['tmp_name'], $file_path);
    }
    else
    {
        print("PHP : upload\n\n");
        print("PHP : no file set\n");

    }
?>
