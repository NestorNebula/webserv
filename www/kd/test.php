<?php
    header('Content-type: text/plain');
    header('Connection:close');
    // exit(66);
    // print("\r\n");
    // print_r($_GET);
    // print_r($_POST);
    // print_r($_SERVER); // ENV shows up here

    print("PHP : hello, world!\n");  
    echo getcwd();
    // gets through FCGI
    // Q: ignore exit status ... 
    // exit (11);


// WRITE_FILE
    // file_put_contents("./whereami.php.txt", "php wrote this");

    $g1 = $_GET['g1'] ?? 'g1-default';
    $g2 = $_GET['g2'] ?? 'g2-default';

    print("\nGET VARS\n");
    print("g1 : " . $g1 . PHP_EOL);
    print("g2 : " . $g2 . PHP_EOL);

    print("\nPOST VARS\n");
    $p1 = $_POST['p1'] ?? 'p1-default';
    $p2 = $_POST['p2'] ?? 'p2-default';

    print("p1 : " . $p1 . PHP_EOL);
    print("p2 : " . $p2 . PHP_EOL);

    print("\nENV\n\n");
    foreach  ($_SERVER as $k => $v)
        print ("$k = $v\n");

    $chk_hed = 'REMOTE_ADDR';
    print("\n$chk_hed : " . $_SERVER[$chk_hed] . PHP_EOL);

// POST Content-Length of 14976173 bytes exceeds the limit of 8388608 bytes in Unknown on line 0

    if (isset($_FILES['file']))
    {
        print_r($_FILES['file']); // Array

            // partial -- 
            // can't close CONN until FCGI has flushed its body
        // switch ($_FILES['file']['error']) {
        //     case UPLOAD_ERR_OK:
        //         break;
        //     case UPLOAD_ERR_NO_FILE:
        //         echo ('No file sent.');
        //         break;
        //     case UPLOAD_ERR_INI_SIZE:
        //     case UPLOAD_ERR_FORM_SIZE:
        //         echo ('Exceeded filesize limit.');
        //         break;
        //     default:
        //         echo ('Unknown errors.');
        //         break;
        // }
//  UPLOAD_ERROR_OK, value 0, means no error occurred.
//  UPLOAD_ERR_INI_SIZE, value 1, means that the size of the uploaded file exceeds the
// maximum value specified in your php.ini file with the upload_max_filesize directive.
//  UPLOAD_ERR_FORM_SIZE, value 2, means that the size of the uploaded file exceeds the
// maximum value specified in the HTML form in the MAX_FILE_SIZE element.
//  UPLOAD_ERR_PARTIAL, value 3, means that the file was only partially uploaded.
//  UPLOAD_ERR_NO_FILE, value 4, means that no file was uploaded.
//  UPLOAD_ERR_NO_TMP_DIR, value 6, means that no temporary directory is specified in the
// php.ini.
//  UPLOAD_ERR_CANT_WRITE, value 7, means that writing the file to disk failed.
//  UPLOAD_ERR_EXTENSION, value 8, means that a PHP extension stopped the file upload
// process.

// UPLOAD_ERR_PARTIAL is given when the mime boundary is not found after the file data. A possibly cause for this is that the upload was cancelled by the user (pressed ESC, etc).

        move_uploaded_file($_FILES['file']['tmp_name'], "./uploads/php-" . $_FILES['file']['name']);
        // move_uploaded_file($_FILES['file']['tmp_name'], getcwd()."/uploads/php-" . $_FILES['file']['name']);
    }
    echo getcwd();


// CWD is the working directory where php-fpm is started (or configured to change to).

// In case of chroot CWD = "".

// In any case the SCRIPT_NAME php script can be found with ./SCRIPT_NAME, from the CWD. So the undocumented not standardized SCRIPT_FILENAME should vanish! It breaks the CGI standard.
?>
