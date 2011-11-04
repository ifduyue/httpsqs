<?php
include_once("httpsqs_client.php");
$httpsqs = new httpsqs("127.0.0.1", 1218, "mypass123", "utf-8");
$result = $httpsqs->put("your_queue_name1", urlencode("text_message1"));
echo "###1.put result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->get("your_queue_name1");
echo "###2.get result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->put("your_queue_name1", urlencode("text_message2"));
echo "###3.put result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->gets("your_queue_name1");
echo "###4.gets result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->status("your_queue_name1");
echo "###5.status result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->status_json("your_queue_name1");
echo "###6.status_json result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->view("your_queue_name1", 1);
echo "###7.view result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->reset("your_queue_name1");
echo "###8.reset result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->maxqueue("your_queue_name1", 5000000);
echo "###9.maxqueue result:\r\n";
var_dump($result);
echo "\r\n\r\n";

$result = $httpsqs->synctime(10);
echo "###10.synctime result:\r\n";
var_dump($result);
echo "\r\n\r\n";
?>
