<?php

header("content-type:text/plain");
include('adodb5/adodb.inc.php');
$db = NewADOConnection('mysql');
$db->Connect("localhost", "checkin", "weeeeee", "rfidcheckin");
if (!$db) {
    echo "Failed to connect to MySQL.\n";
    exit;
}

if(array_key_exists('id',$_GET) and strlen($_GET['id']) == 12 ) {
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
} else if(array_key_exists('report',$_GET)) {
    switch( $_GET['report'] ) {
        case 'daily':
            $res = $db->Execute("select date(timestamp) as day, id from log where dayofyear(timestamp) >= dayofyear(now()) - 30 group by dayofyear(timestamp), id;");
            if( ! $res ) {
                print $db->ErrorMsg();
                die();
            }
            while( $row = $res->FetchRow() ) {
                $data[$row['day']]++;
            }
            print json_encode($data);
            print "\n";
        break;
        case 'last':
            $res = $db->Execute("select id from log order by timestamp DESC limit 1;");
            if( ! $res ) {
                print $db->ErrorMsg();
                die();
            }
            while( $row = $res->FetchRow() ) {
                print json_encode(array('last_id_seen' => $row['id']));
            }
            print "\n";
        break;
    }
} else {
    print "ERR\n1\n";
}
