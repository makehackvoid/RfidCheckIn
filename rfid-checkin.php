<?php

header("content-type:text/plain");
include('adodb5/adodb.inc.php');
$db = NewADOConnection('mysql');                                                
$db->Connect("localhost", [username], [password], "rfidcheckin");
if (!$db) {
    echo "Failed to connect to MySQL.\n";                                           
    exit;
}

if(strlen($_GET['id']) == 12 ) {
    if(                                                                                 
        $db->Execute(                                                                       
            "INSERT INTO log (timestamp,id) values(NOW(),?)",
            array($_GET['id'])
        ) !== false
    ) {
        $res = $db->Execute("select dayofyear(timestamp) as day, id from log where dayofyear(timestamp) = dayofyear(now()) group by id;");

        print "OK\n";
        print $res->RecordCount();
        print "\n";
    } else {
        print "ERR\n2\n";
    }

} else {
    print "ERR\n1\n";
}

~                                                                               
~                                                                               
~                                                                               
~                            