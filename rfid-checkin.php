<?php

header("content-type:text/plain");
include('adodb5/adodb.inc.php');
$db = NewADOConnection('mysql');                                                
$db->Connect("localhost", [username], [password], "rfidcheckin");
if (!$db) {
    echo "Failed to connect to MySQL.\n";
   exit;
}

if(strlen($_GET['id']) == 14 ) {
    if( $db->Execute(
            "INSERT INTO log (timestamp,id) values(NOW(),?)",
            array($_GET['id'])
        ) !== false
    ) {
        $res = $db->Execute("select dayofyear(timestamp) as day, id from log where dayofyear(timestamp) = dayofyear(now()) group by id;");

        print "good stuff!\n";
        print $res->RecordCount();
    } else {
        print "that didn't work\n";
        print $db->ErrorMsg()+ "\n";
    }

} else {
    print "no good\n";
}
