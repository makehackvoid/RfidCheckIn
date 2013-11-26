<?php

header("content-type:text/plain");
include('adodb/adodb.inc.php');
$db = NewADOConnection('mysql');
$db->Connect("10.0.0.5", "checkin", "weeeeee", "rfidcheckin");
if (!$db) {
    header("Internal Server Error", true, 500);
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

    	header("OK", true, 200);
        print $res->RecordCount();
    } else {
        header("Internal Server Error", true, 500);
        print "Error inserting checkin to database";
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
	header("Bad Request", true, 400);
    print "Error invalid request";
}
